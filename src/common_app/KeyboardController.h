#pragma once

#include <stdint.h>

#include <lvgl.h>

#include "element_descriptors.generated.h"

namespace frontapp {

// Управляет локальной Screen32-клавиатурой как frontend-modal экраном без backend-навигации.
class KeyboardController {
public:
    using InputTextCallback = void (*)(void* userData, uint32_t elementId, uint32_t pageId, const char* value);

    // Задаёт callback для отправки финального input_event после READY.
    void setInputEventSink(void* userData, InputTextCallback callback);

    // Открывает клавиатуру для исходного элемента и переносит текущий текст в kbd_text.
    void open(uint32_t pageId,
              uint32_t elementId,
              lv_obj_t* source,
              Screen32KeyboardKind kind,
              uint16_t maxLength);

    // Возвращает true, пока локальный экран Keyboard открыт поверх backend-страницы.
    bool isActive() const;

    // Возвращает backend-страницу, которая должна считаться текущей при открытой клавиатуре.
    uint32_t logicalPageId(uint32_t fallbackPageId) const;

    // Проверяет, нужно ли подавить generated event с локальной страницы Keyboard.
    bool shouldSuppressGeneratedEvent(uint32_t descriptorPageId) const;

    // Читает текст напрямую из textarea/label или из первого вложенного label.
    static const char* readTextFromObject(lv_obj_t* obj);

    // Записывает текст в textarea/label или в первый label внутри кнопки/контейнера.
    static bool setTextToObject(lv_obj_t* obj, const char* text);

private:
    enum class Language : uint8_t {
        English = 0,
        Russian = 1
    };

    // Находит первый label внутри объекта, чтобы кнопки и контейнеры можно было читать как текстовые поля.
    static bool getLabelForObject(lv_obj_t* obj, lv_obj_t*& outLabel);

    // Статический LVGL callback, который перенаправляет событие в экземпляр контроллера.
    static void keyboardEventCb(lv_event_t* e);

    // Обрабатывает события LVGL keyboard: переключение языка, READY и CANCEL.
    void handleEvent(lv_event_t* e);

    // Один раз заменяет стандартный callback клавиатуры на callback контроллера.
    void installKeyboardHandler();

    // Регистрирует пользовательские раскладки RU/EN в объекте LVGL keyboard.
    void configureLayouts();

    // Возвращает текст нажатой клавиши buttonmatrix для текущего LVGL события.
    const char* selectedButtonText(lv_obj_t* keyboard) const;

    // Переключает активный режим клавиатуры и запоминает текущий язык.
    void switchMode(lv_keyboard_mode_t mode, Language language);

    // Обрабатывает нажатие кнопки клавиатуры и делегирует обычные клавиши LVGL.
    void handleValueChanged(lv_event_t* e);

    // Подтверждает ввод, записывает текст обратно в исходный элемент и отправляет input_event.
    void accept();

    // Отменяет ввод без записи текста и backend-событий.
    void cancel();

    // Возвращает локальный UI на исходную страницу без backend-навигации.
    void returnToSource();

    void* inputUserData = nullptr;
    InputTextCallback inputCallback = nullptr;
    uint32_t sourcePageId = 0;
    uint32_t sourceElementId = 0;
    lv_obj_t* sourceObj = nullptr;
    lv_obj_t* previousScreenObj = nullptr;
    Screen32KeyboardKind keyboardKind = KEYBOARD_NONE;
    uint16_t maxLength = 32;
    Language activeLanguage = Language::English;
    bool active = false;
    bool handlerInstalled = false;
};

} // namespace frontapp
