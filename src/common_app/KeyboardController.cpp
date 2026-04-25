#include "common_app/KeyboardController.h"

#include <string.h>

#include "page_descriptors.generated.h"
#include "ui/screens.h"
#include "ui/ui.h"

namespace frontapp {
namespace {

constexpr uint16_t kDefaultKeyboardMaxLength = 32;

constexpr lv_buttonmatrix_ctrl_t kKeyboardPlainKey = LV_BUTTONMATRIX_CTRL_WIDTH_1;
constexpr lv_buttonmatrix_ctrl_t kKeyboardControlKey =
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | LV_BUTTONMATRIX_CTRL_WIDTH_2);
constexpr lv_buttonmatrix_ctrl_t kKeyboardArrowKey =
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | LV_BUTTONMATRIX_CTRL_WIDTH_2);
constexpr lv_buttonmatrix_ctrl_t kKeyboardSpaceKey = LV_BUTTONMATRIX_CTRL_WIDTH_6;

static const char* const kKeyboardEnglishLowerMap[] = {
    "RU", "ABC", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", LV_SYMBOL_BACKSPACE, "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
    "z", "x", "c", "v", "b", "n", "m", ".", ",", "\n",
    LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t kKeyboardEnglishLowerCtrl[] = {
    kKeyboardControlKey, kKeyboardControlKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardControlKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardControlKey, kKeyboardArrowKey, kKeyboardSpaceKey, kKeyboardArrowKey, kKeyboardControlKey
};

static const char* const kKeyboardEnglishUpperMap[] = {
    "RU", "abc", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", LV_SYMBOL_BACKSPACE, "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", "\n",
    "Z", "X", "C", "V", "B", "N", "M", ".", ",", "\n",
    LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t kKeyboardEnglishUpperCtrl[] = {
    kKeyboardControlKey, kKeyboardControlKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardControlKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardControlKey, kKeyboardArrowKey, kKeyboardSpaceKey, kKeyboardArrowKey, kKeyboardControlKey
};

static const char* const kKeyboardRussianLowerMap[] = {
    "EN", "ABC", "й", "ц", "у", "к", "е", "н", "г", "ш", "щ", "з", "х", LV_SYMBOL_BACKSPACE, "\n",
    "ф", "ы", "в", "а", "п", "р", "о", "л", "д", "ж", "э", "\n",
    "я", "ч", "с", "м", "и", "т", "ь", "б", "ю", ".", "\n",
    LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t kKeyboardRussianLowerCtrl[] = {
    kKeyboardControlKey, kKeyboardControlKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardControlKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardControlKey, kKeyboardArrowKey, kKeyboardSpaceKey, kKeyboardArrowKey, kKeyboardControlKey
};

static const char* const kKeyboardRussianUpperMap[] = {
    "EN", "abc", "Й", "Ц", "У", "К", "Е", "Н", "Г", "Ш", "Щ", "З", "Х", LV_SYMBOL_BACKSPACE, "\n",
    "Ф", "Ы", "В", "А", "П", "Р", "О", "Л", "Д", "Ж", "Э", "\n",
    "Я", "Ч", "С", "М", "И", "Т", "Ь", "Б", "Ю", ".", "\n",
    LV_SYMBOL_CLOSE, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t kKeyboardRussianUpperCtrl[] = {
    kKeyboardControlKey, kKeyboardControlKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardControlKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey, kKeyboardPlainKey,
    kKeyboardControlKey, kKeyboardArrowKey, kKeyboardSpaceKey, kKeyboardArrowKey, kKeyboardControlKey
};

// Проверяет, что page id существует в сгенерированных descriptors.
bool isValidPageId(uint32_t pageId) {
    return screen32_find_page_descriptor(pageId) != nullptr;
}

} // namespace

// Задаёт callback для отправки финального input_event после READY.
void KeyboardController::setInputEventSink(void* userData, InputTextCallback callback) {
    inputUserData = userData;
    inputCallback = callback;
}

// Открывает локальный экран Keyboard, подготавливает textarea и не меняет backend current page.
void KeyboardController::open(uint32_t pageId,
                              uint32_t elementId,
                              lv_obj_t* source,
                              Screen32KeyboardKind kind,
                              uint16_t requestedMaxLength) {
    if (source == nullptr || !lv_obj_is_valid(source) || objects.kbd_text == nullptr || objects.kbd_key == nullptr) {
        return;
    }

    sourcePageId = pageId;
    sourceElementId = elementId;
    sourceObj = source;
    previousScreenObj = lv_screen_active();
    keyboardKind = kind;
    maxLength = requestedMaxLength > 0 ? requestedMaxLength : kDefaultKeyboardMaxLength;
    activeLanguage = Language::English;
    active = true;

    installKeyboardHandler();
    configureLayouts();

    lv_textarea_set_text(objects.kbd_text, readTextFromObject(sourceObj));
    lv_textarea_set_max_length(objects.kbd_text, maxLength);
    lv_keyboard_set_textarea(objects.kbd_key, objects.kbd_text);

    if (keyboardKind == KEYBOARD_NUMBER) {
        lv_keyboard_set_mode(objects.kbd_key, LV_KEYBOARD_MODE_NUMBER);
    } else {
        switchMode(LV_KEYBOARD_MODE_TEXT_LOWER, Language::English);
    }

    loadScreen(SCREEN_ID_KEYBOARD);
}

// Сообщает, открыт ли сейчас локальный модальный ввод.
bool KeyboardController::isActive() const {
    return active;
}

// Возвращает page id исходной backend-страницы, даже когда LVGL активен на Keyboard.
uint32_t KeyboardController::logicalPageId(uint32_t fallbackPageId) const {
    return active && sourcePageId != 0 ? sourcePageId : fallbackPageId;
}

// Проверяет, нужно ли игнорировать generated events самой страницы Keyboard.
bool KeyboardController::shouldSuppressGeneratedEvent(uint32_t descriptorPageId) const {
    return active && descriptorPageId == static_cast<uint32_t>(SCREEN_ID_KEYBOARD);
}

// Читает текст напрямую из textarea/label или из первого вложенного label.
const char* KeyboardController::readTextFromObject(lv_obj_t* obj) {
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return "";
    }

#if LV_USE_TEXTAREA
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        const char* text = lv_textarea_get_text(obj);
        return text != nullptr ? text : "";
    }
#endif

    lv_obj_t* label = nullptr;
    if (getLabelForObject(obj, label)) {
#if LV_USE_LABEL
        const char* text = lv_label_get_text(label);
        return text != nullptr ? text : "";
#endif
    }

    return "";
}

// Записывает текст в textarea/label или в первый label внутри кнопки/контейнера.
bool KeyboardController::setTextToObject(lv_obj_t* obj, const char* text) {
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

    const char* safeText = text != nullptr ? text : "";

#if LV_USE_TEXTAREA
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        lv_textarea_set_text(obj, safeText);
        return true;
    }
#endif

#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_dropdown_set_options(obj, safeText);
        return true;
    }
#endif

#if LV_USE_CHECKBOX
    if (lv_obj_check_type(obj, &lv_checkbox_class)) {
        lv_checkbox_set_text(obj, safeText);
        return true;
    }
#endif

    lv_obj_t* label = nullptr;
    if (!getLabelForObject(obj, label)) {
        return false;
    }

#if LV_USE_LABEL
    lv_label_set_text(label, safeText);
    return true;
#else
    return false;
#endif
}

// Находит первый label внутри объекта, включая вложенные контейнеры.
bool KeyboardController::getLabelForObject(lv_obj_t* obj, lv_obj_t*& outLabel) {
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

#if LV_USE_LABEL
    if (lv_obj_check_type(obj, &lv_label_class)) {
        outLabel = obj;
        return true;
    }
#endif

    const uint32_t childCount = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < childCount; ++i) {
        lv_obj_t* child = lv_obj_get_child(obj, static_cast<int32_t>(i));
        if (getLabelForObject(child, outLabel)) {
            return true;
        }
    }

    return false;
}

// Статический LVGL callback достаёт экземпляр из user_data и передаёт ему событие.
void KeyboardController::keyboardEventCb(lv_event_t* e) {
    KeyboardController* controller = static_cast<KeyboardController*>(lv_event_get_user_data(e));
    if (controller != nullptr) {
        controller->handleEvent(e);
    }
}

// Маршрутизирует события клавиатуры по типу LVGL события.
void KeyboardController::handleEvent(lv_event_t* e) {
    if (e == nullptr) {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        handleValueChanged(e);
        return;
    }
    if (code == LV_EVENT_READY) {
        accept();
        return;
    }
    if (code == LV_EVENT_CANCEL) {
        cancel();
    }
}

// Удаляет стандартный callback, чтобы RU/EN не вставлялись как текст, и ставит callback контроллера.
void KeyboardController::installKeyboardHandler() {
    if (handlerInstalled || objects.kbd_key == nullptr) {
        return;
    }

    lv_obj_remove_event_cb(objects.kbd_key, lv_keyboard_def_event_cb);
    lv_obj_add_event_cb(objects.kbd_key, &KeyboardController::keyboardEventCb, LV_EVENT_ALL, this);
    handlerInstalled = true;
}

// Назначает пользовательские карты клавиатуры для английского и русского режимов.
void KeyboardController::configureLayouts() {
    if (objects.kbd_key == nullptr) {
        return;
    }

    lv_keyboard_set_map(objects.kbd_key, LV_KEYBOARD_MODE_TEXT_LOWER, kKeyboardEnglishLowerMap, kKeyboardEnglishLowerCtrl);
    lv_keyboard_set_map(objects.kbd_key, LV_KEYBOARD_MODE_TEXT_UPPER, kKeyboardEnglishUpperMap, kKeyboardEnglishUpperCtrl);
    lv_keyboard_set_map(objects.kbd_key, LV_KEYBOARD_MODE_USER_1, kKeyboardRussianLowerMap, kKeyboardRussianLowerCtrl);
    lv_keyboard_set_map(objects.kbd_key, LV_KEYBOARD_MODE_USER_2, kKeyboardRussianUpperMap, kKeyboardRussianUpperCtrl);
}

// Достаёт label выбранной кнопки клавиатуры из LVGL buttonmatrix.
const char* KeyboardController::selectedButtonText(lv_obj_t* keyboard) const {
    if (keyboard == nullptr || !lv_obj_is_valid(keyboard)) {
        return nullptr;
    }

    const uint32_t buttonId = lv_keyboard_get_selected_button(keyboard);
    if (buttonId == LV_BUTTONMATRIX_BUTTON_NONE) {
        return nullptr;
    }
    return lv_keyboard_get_button_text(keyboard, buttonId);
}

// Переключает LVGL mode и синхронизирует внутренний язык контроллера.
void KeyboardController::switchMode(lv_keyboard_mode_t mode, Language language) {
    if (objects.kbd_key == nullptr) {
        return;
    }

    activeLanguage = language;
    lv_keyboard_set_mode(objects.kbd_key, mode);
}

// Перехватывает служебные RU/EN/ABC/abc для русской раскладки, остальные клавиши отдаёт LVGL.
void KeyboardController::handleValueChanged(lv_event_t* e) {
    lv_obj_t* keyboard = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    const char* text = selectedButtonText(keyboard);
    if (text == nullptr) {
        return;
    }

    if (strcmp(text, "RU") == 0) {
        switchMode(LV_KEYBOARD_MODE_USER_1, Language::Russian);
        return;
    }
    if (strcmp(text, "EN") == 0) {
        switchMode(LV_KEYBOARD_MODE_TEXT_LOWER, Language::English);
        return;
    }

    const lv_keyboard_mode_t mode = lv_keyboard_get_mode(keyboard);
    if (strcmp(text, "ABC") == 0 && mode == LV_KEYBOARD_MODE_USER_1) {
        switchMode(LV_KEYBOARD_MODE_USER_2, Language::Russian);
        return;
    }
    if (strcmp(text, "abc") == 0 && mode == LV_KEYBOARD_MODE_USER_2) {
        switchMode(LV_KEYBOARD_MODE_USER_1, Language::Russian);
        return;
    }

    lv_keyboard_def_event_cb(e);
}

// Завершает ввод: копирует текст в исходный объект и отправляет input_event по исходным id.
void KeyboardController::accept() {
    if (!active) {
        return;
    }

    const char* text = objects.kbd_text != nullptr ? lv_textarea_get_text(objects.kbd_text) : "";
    setTextToObject(sourceObj, text);
    if (inputCallback != nullptr) {
        inputCallback(inputUserData, sourceElementId, sourcePageId, text != nullptr ? text : "");
    }
    returnToSource();
}

// Закрывает клавиатуру без изменения исходного элемента.
void KeyboardController::cancel() {
    if (!active) {
        return;
    }

    returnToSource();
}

// Возвращает LVGL на исходный экран, не отправляя backend show_page/navigation.
void KeyboardController::returnToSource() {
    const uint32_t pageId = sourcePageId;
    active = false;
    sourcePageId = 0;
    sourceElementId = 0;
    sourceObj = nullptr;
    keyboardKind = KEYBOARD_NONE;

    if (isValidPageId(pageId)) {
        previousScreenObj = nullptr;
        loadScreen(static_cast<enum ScreensEnum>(pageId));
        return;
    }

    if (previousScreenObj != nullptr && lv_obj_is_valid(previousScreenObj)) {
        lv_screen_load(previousScreenObj);
    }
    previousScreenObj = nullptr;
}

} // namespace frontapp
