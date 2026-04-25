#include <lvgl.h>

// Маркерная EEZ action для текстовой клавиатуры: символ нужен линковщику, логика живёт в FrontApp.
extern "C" void action_keyboard_text(lv_event_t* e) {
    (void)e;
}

// Маркерная EEZ action для числовой клавиатуры: символ нужен линковщику, логика живёт в FrontApp.
extern "C" void action_keyboard_number(lv_event_t* e) {
    (void)e;
}
