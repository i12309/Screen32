# Память на ESP32-S3 (JC8048W550C)

## TL;DR

На плате две RAM:

- **Internal DRAM** (~290 КБ полезных) — встроена в чип. Только она годится для DMA, прерываний, драйверов железа (UART, SPI, дисплей, FreeRTOS-очереди). Очень ограничена.
- **PSRAM** (8 МБ, octal SPIRAM) — внешняя микросхема. Большая, но медленнее, и **запрещена** для прерываний/DMA.

Главное правило: **всё прикладное держим в PSRAM, internal DRAM оставляем драйверам.**

## Что куда кладётся по умолчанию

| Источник | Куда летит |
|---|---|
| Глобальные переменные / `static` массивы | Internal DRAM (.bss/.data) |
| `new` / `malloc` | Internal DRAM (MALLOC_CAP_DEFAULT = INTERNAL) |
| `lv_malloc` (lvgl) | Зависит от `lv_conf.h` (см. ниже) |
| UART/SPI driver, FreeRTOS-очереди | Только Internal DRAM |
| LCD DMA-буфер (smartdisplay 128 KB) | Только DMA-capable Internal RAM |

Поэтому каждые «100 элементов в массиве» или «куча lv_obj-ов на новой странице» молча режут DRAM, пока её не останется на драйвер.

## Симптом «памяти нет»

Признаки, что DRAM кончилась:

- `uart_driver_install -> ESP_ERR_NO_MEM` на старте.
- `Guru Meditation Error: LoadProhibited`, EXCVADDR=0x4, backtrace внутри `uartBegin` / `HardwareSerial::begin` — это баг Arduino-core: `uart=NULL; ... uart->num` в обработчике ошибки. Реальная причина — провалившийся `uart_driver_install` (или `uartSetPins`) из-за нехватки DRAM.
- Любая инициализация драйвера падает «случайно» после добавления данных/page/элементов.

Проверка точная — лог свободной DRAM:

```cpp
heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
```

Если `largest < 4 КБ` — драйвер железа уже не встанет. Раннему логу `heap=8bit` верить нельзя: там сумма с PSRAM.

## Что переносить в PSRAM

Любой прикладной объект, к которому **не обращаются из ISR** и **не используется как DMA-буфер**:

- Большие глобальные структуры состояния (`State`, контексты приложения).
- Массивы биндингов (`elementId → lv_obj_t*`, `pageId → screen*`).
- Очереди событий приложения (не FreeRTOS-овские).
- Кэши, текстовые буферы, лог-кольца.
- Кучу lvgl целиком — это самый жирный потребитель.

## Что **нельзя** переносить в PSRAM

- UART/SPI/I2C-буферы, всё, что заливается через DMA.
- Стеки задач, обслуживающих прерывания.
- Структуры FreeRTOS-объектов (queue/semaphore/timer storage).
- Объекты, к которым обращается ISR.

Если сомневаешься — оставь в DRAM. Перенос в PSRAM «неподходящего» объекта не даст ошибку компиляции, а проявится как редкий рантайм-краш.

## Рецепты переноса

### 1. Большая глобальная структура

Не делать `static State g_state;` (она уйдёт в DRAM). Делать так:

```cpp
#include <esp_heap_caps.h>
#include <new>

State* g_state_ptr = nullptr;
#define g_state (*g_state_ptr)

bool ensure_state_allocated() {
    if (g_state_ptr != nullptr) return true;
    void* mem = heap_caps_malloc(sizeof(State),
                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (mem == nullptr) {
        // PSRAM недоступна — fallback на DRAM
        mem = ::operator new(sizeof(State), std::nothrow);
    }
    if (mem == nullptr) return false;
    g_state_ptr = new (mem) State();
    return true;
}
```

Вызвать `ensure_state_allocated()` в самом начале `init()` перед любым обращением к `g_state`.

### 2. Большой массив POD-объектов

```cpp
constexpr size_t kCapacity = 180;
ButtonEventState* g_buttonStates = nullptr;

bool ensure_button_states_allocated() {
    if (g_buttonStates != nullptr) return true;
    const size_t bytes = sizeof(ButtonEventState) * kCapacity;
    void* mem = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (mem == nullptr) mem = ::operator new(bytes, std::nothrow);
    if (mem == nullptr) return false;
    g_buttonStates = static_cast<ButtonEventState*>(mem);
    for (size_t i = 0; i < kCapacity; ++i) {
        new (&g_buttonStates[i]) ButtonEventState();
    }
    return true;
}
```

### 3. Куча lvgl

Самый жирный потребитель DRAM. Lvgl создаёт по `lv_obj` для каждого элемента каждой страницы — для 26 страниц × ~10 элементов это десятки килобайт. По умолчанию `lv_malloc` идёт через `malloc`, который берёт из DRAM.

Решение — собственный пул lvgl в PSRAM. В `include/lv_conf.h`:

```c
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (512 * 1024U)         // запас под все страницы
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_POOL_INCLUDE "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
```

Это то, что реально открыло проблему этого проекта: lvgl-куча в DRAM съедала её до того, как UART успевал стартовать.

## Почему `EXT_RAM_ATTR` не работает в Arduino-esp32

В «настоящем» ESP-IDF можно повесить `EXT_RAM_ATTR` на BSS-переменную и она сама уедет в PSRAM. **В Arduino-esp32 это не работает**: в их `sdkconfig` не включён `CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY`, и макрос разворачивается в пустоту. Build-флагом это не поправить — IDF идёт прекомпилированной.

Поэтому единственный надёжный путь — ленивая аллокация через `heap_caps_malloc(MALLOC_CAP_SPIRAM)` + placement new.

## Диагностика

Строка для проверки фактической DRAM:

```cpp
SCREENLIB_LOGI(tag,
               "heap INTERNAL free=%u largest=%u, DMA free=%u largest=%u",
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
```

Здоровое состояние перед `uart_driver_install`: `INTERNAL free > 50 КБ`, `largest > 16 КБ`.

## Чек-лист при добавлении новой большой статики

Перед тем как объявить большой `static`-массив или глобальный объект:

1. Прикинь размер: `sizeof(T) * N`. Если > 1 КБ — кандидат в PSRAM.
2. Спроси: лезет ли это из ISR/DMA? Если нет — в PSRAM.
3. Реализуй через `heap_caps_malloc(MALLOC_CAP_SPIRAM)` + ленивая инициализация (рецепт 1 или 2).
4. После прошивки проверь лог `heap INTERNAL free=...` — если упало после твоего изменения, перепроверь, реально ли аллокация ушла в PSRAM (адрес в диапазоне `0x3D000000-0x3E000000` — это PSRAM; `0x3FC00000-0x3FD00000` — это DRAM).

## История проблемы (для справки)

После добавления новых page количество lvgl-объектов и размер прикладных массивов выросли. Internal DRAM забилась почти полностью (`largest=4 байта`). Первый же `uart_driver_install` на старте получил `ESP_ERR_NO_MEM`. Arduino-core при отказе вызвал NPE в обработчике ошибки и упал с `Guru Meditation: LoadProhibited` в `uartBegin:464`. Симптом выглядел как «крах при инициализации UART на пине 18», и долго списывался на пин/контракт протокола, хотя реальная причина — нехватка internal DRAM.

Решение: lvgl-куча, `g_state`, `g_buttonStates` переехали в PSRAM. UART переписан с `HardwareSerial` на прямой IDF API (`uart_param_config` / `uart_set_pin` / `uart_driver_install`), чтобы получать настоящий `esp_err_t` и не зависеть от бага Arduino-core.
