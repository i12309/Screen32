Нужно допилить фронтовой проект Screen32 (commit 1ccd556...) и встроить в него screenLIB как экранную сторону.

ВАЖНО:
Этот репозиторий Screen32 — это FRONTEND / SCREEN CLIENT.
Значит сюда нужно брать из screenLIB только:
- core
- client
- adapter
НЕ брать host.

Что подключить из screenLIB:
1. core
   - FrameCodec
   - ProtoCodec
   - machine.proto / machine.pb.*
   - ScreenBridge
   - ITransport
2. client
   - ScreenClient
   - CommandDispatcher
   - WebSocketClientLink
3. adapter
   - IUiAdapter
   - EezLvglAdapter
   - UiObjectMap

Не подключать:
- ScreenSystem
- ScreenManager
- PageRegistry
- host-side transports / mirror routing

--------------------------------
1. КУДА ВСТРАИВАТЬ В SCREEN32
--------------------------------

Опираться на текущую структуру Screen32:

- src/common_app/
  - app_core.*      -> оставить как ядро LVGL цикла
  - navigation.*    -> использовать как локальную навигацию для offline demo
  - shared_app.*    -> использовать как общий entrypoint app_setup/app_loop

- src/platform_esp32/
  -> сюда положить ESP32-specific startup для screen client,
     загрузку frontend config и создание UART transport при online режиме

- src/platform_web/
  -> сюда положить web-specific startup для screen client,
     загрузку frontend config и создание WebSocketClientLink при online режиме

- demo_web/
  -> использовать как web build target, не ломать существующую сборку

Новая общая интеграция должна жить в common_app, а не дублироваться отдельно под ESP32 и Web.

--------------------------------
2. КАКУЮ НОВУЮ ПРОСЛОЙКУ ДОБАВИТЬ
--------------------------------

Создать в src/common_app новый слой, например:
- frontend_runtime.h
- frontend_runtime.cpp

Назначение frontend_runtime:
- владеть ScreenClient
- владеть/инициализировать EezLvglAdapter
- создавать screen-side runtime поверх уже поднятого transport
- уметь работать как online, так и offline_demo
- быть общим для ESP32 и Web

shared_app.cpp должен вызывать frontend_runtime из app_setup()/app_loop(),
а не тащить весь screenLIB-код внутрь себя напрямую.

--------------------------------
3. КАКИЕ PAGE ИСПОЛЬЗОВАТЬ
--------------------------------

Использовать существующие страницы из generated UI как source of truth.
Не придумывать новые page ids.

Базовые страницы в текущем макете:
- SCREEN_ID_LOAD
- SCREEN_ID_MAIN_MENU
- SCREEN_ID_DEF_PAGE1
- SCREEN_ID_DEF_PAGE2
- SCREEN_ID_DEF_PAGE3
- SCREEN_ID_DEF_PAGE4

Их же использовать:
- для showPage в EezLvglAdapter
- для current_page / page_state в service responses
- для button_event / input_event.page_id

UiObjectMap должен явно маппить:
- page_id -> screen object
- element_id -> lv_obj_t*

Не дублировать page ids в нескольких местах без необходимости.
Если ids уже есть в generated screens/ui headers, использовать их как основной источник.

--------------------------------
4. КАК РАБОТАТЬ С PAGES
--------------------------------

Online режим:
- backend присылает show_page(page_id)
- ScreenClient -> CommandDispatcher -> EezLvglAdapter::showPage(page_id)
- adapter переключает экран
- button/input события уходят обратно с тем page_id, который реально активен

Offline demo:
- использовать текущий navigation.* как локальный механизм навигации
- direct routes/back routes можно оставить как есть
- offline_demo не должен зависеть от backend
- пользователь должен мочь ходить по 6 страницам локально

То есть:
- online page switching = через proto show_page
- offline page switching = через локальную navigation.*

Обе модели должны работать поверх одного и того же UI.

--------------------------------
5. FRONTEND CONFIG JSON
--------------------------------

Нужно добавить JSON config именно для фронта.

Минимальный формат:
{
  "mode": "esp32" | "wasm",
  "transport": {
    "type": "uart" | "ws_client" | "none",
    "url": "ws://127.0.0.1:81",
    "baud": 115200,
    "rxPin": 16,
    "txPin": 17
  },
  "offline_demo": 1,
  "start_page": 1
}

Обязательное поведение:
- offline_demo=1:
  - frontend стартует без обязательной связи
  - transport может быть none/null/fake
  - UI интерактивен
  - кнопки можно нажимать
  - страницы можно переключать локально
- offline_demo=0:
  - frontend работает как реальный screen client
  - transport создается по config

Где грузить config:
- для ESP32: можно начать с compile-time embedded JSON string
  или простого локального файла/константы, без усложнения
- для Web: config лежит рядом с demo_web как json-файл и читается при старте

Сразу не усложнять системой хранения.
Нужен рабочий demo, не идеальная конфигурационная платформа.

--------------------------------
6. ONLINE РЕЖИМ
--------------------------------

В online режиме нужно сделать рабочую цепочку:

transport
-> ScreenClient
-> EezLvglAdapter
-> generated EEZ/LVGL UI

Обязательные команды:
- show_page
- set_text
- set_value
- set_visible
- set_color
- set_batch

Обратные события:
- button_event
- input_event(int/string)

Service layer:
- если backend запрашивает current page / page state / device info,
  frontend должен уметь ответить через уже существующие helper-методы ScreenClient/ScreenBridge.

--------------------------------
7. OFFLINE DEMO РЕЖИМ
--------------------------------

offline_demo=1 должен быть именно режимом существующего фронта, а не отдельным проектом.

Что требуется:
- поднять UI
- дать возможность ходить по 6 страницам
- дать возможность нажимать кнопки и трогать input widgets
- backend не обязателен
- события можно:
  - логировать
  - складывать локально
  - игнорировать после генерации
  - но UI не должен ломаться

Логику кнопок пока не реализовывать полноценно.
Нужно именно показать, что frontend живой и по нему можно кликать без связи.

--------------------------------
8. КАКОЙ TRANSPORT ГДЕ ИСПОЛЬЗОВАТЬ
--------------------------------

ESP32 demo:
- online -> UART transport
- offline_demo -> none/fake/null transport

WebAssembly demo:
- online -> WebSocketClientLink
- offline_demo -> none/fake/null transport

Не тянуть host-side ScreenSystem сюда.

--------------------------------
9. EezLvglAdapter / UiObjectMap
--------------------------------

Нужно использовать уже существующий adapter из screenLIB,
но довести его подключение к реальному generated UI этого проекта.

Обязательно:
- не переписывать generated EEZ code
- не делать новый UI
- использовать существующие objects/screens
- заполнить UiObjectMap под текущие 6 форм и нужные элементы
- если объект не найден, adapter возвращает false, не падает

--------------------------------
10. ЧТО ИЗМЕНИТЬ В SCREEN32
--------------------------------

Минимально ожидаемые изменения:
- добавить зависимость/подключение screenLIB (core + client + adapter)
- добавить common_app/frontend_runtime.*
- интегрировать frontend_runtime в shared_app.*
- добавить frontend config loader
- добавить online/offline branch
- подключить EezLvglAdapter к generated UI
- подготовить demo_web config + esp32 config
- не ломать app_core и platform layers без необходимости

--------------------------------
11. ДЕМО ЧТО ДОЛЖЕН УМЕТЬ
--------------------------------

Нужен рабочий пример:

ESP32:
- прошивка поднимает UI
- online через UART работает как screen client
- offline_demo позволяет кликать локально

Web:
- wasm/demo_web поднимает UI
- online через WebSocketClientLink работает как screen client
- offline_demo позволяет кликать локально

--------------------------------
12. README / DOC
--------------------------------

Обновить README проекта Screen32:
- как собрать ESP32 demo
- как собрать web demo
- где лежит frontend json config
- как включить offline_demo=1
- чем online отличается от offline
- какие 6 страниц участвуют в demo

--------------------------------
13. ОГРАНИЧЕНИЯ
--------------------------------

Не делать сейчас:
- полную бизнес-логику кнопок
- сложный fake backend
- новую архитектуру frontend с нуля
- переписывание common_app/app_core/navigation без необходимости

Нужно:
- встроиться в существующий каркас Screen32
- использовать уже имеющийся UI и уже имеющиеся страницы
- сделать работающий demo

--------------------------------
14. КРИТЕРИЙ ГОТОВНОСТИ
--------------------------------

- Screen32 использует screenLIB как screen client side
- 6 существующих страниц реально работают
- online режим работает через transport
- offline_demo=1 работает без связи
- один и тот же UI используется в обоих режимах
- web и esp32 используют один и тот же общий runtime слой в src/common_app