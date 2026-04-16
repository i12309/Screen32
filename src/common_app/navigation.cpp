#include "common_app/navigation.h"

extern "C" {
#include "ui/ui.h"
#include "ui/screens.h"
}

/*
 * Файл src/common_app/navigation.cpp
 * Назначение: реализация переходов между экранами и обработчиков кнопок
 * для ветки исходников `src/`.
 */

namespace demo::nav {

namespace {

// Ограничения статических массивов, чтобы не использовать динамическую аллокацию
// в навигационном слое (проще и предсказуемее на MCU).
constexpr uint16_t kMaxBindings = 96;
constexpr uint8_t kMaxHistory = 16;

// Одна запись "кнопка -> действие".
struct Binding {
    lv_obj_t *button;
    Action action;
};

// Реестр всех привязок кнопок.
Binding g_bindings[kMaxBindings];
uint16_t g_binding_count = 0;

// Текущее состояние навигации.
ScreensEnum g_current_screen = SCREEN_ID_LOAD;
ScreensEnum g_history[kMaxHistory];
uint8_t g_history_size = 0;

// Преобразование enum экрана в указатель на объект экрана LVGL.
// Связка берется из сгенерированной структуры objects.
lv_obj_t *screen_to_obj(ScreensEnum screen) {
    switch (screen) {
        case SCREEN_ID_LOAD:
            return objects.load;
        case SCREEN_ID_MAIN_MENU:
            return objects.main_menu;
        case SCREEN_ID_DEF_PAGE1:
            return objects.def_page1;
        case SCREEN_ID_DEF_PAGE2:
            return objects.def_page2;
        case SCREEN_ID_DEF_PAGE3:
            return objects.def_page3;
        case SCREEN_ID_DEF_PAGE4:
            return objects.def_page4;
        default:
            return nullptr;
    }
}

// Определение текущего экрана по активному lv_scr_act().
// Если активный экран не распознан, возвращаем последнее известное значение.
ScreensEnum active_screen_to_enum() {
    lv_obj_t *active = lv_scr_act();
    if (active == objects.load) {
        return SCREEN_ID_LOAD;
    }
    if (active == objects.main_menu) {
        return SCREEN_ID_MAIN_MENU;
    }
    if (active == objects.def_page1) {
        return SCREEN_ID_DEF_PAGE1;
    }
    if (active == objects.def_page2) {
        return SCREEN_ID_DEF_PAGE2;
    }
    if (active == objects.def_page3) {
        return SCREEN_ID_DEF_PAGE3;
    }
    if (active == objects.def_page4) {
        return SCREEN_ID_DEF_PAGE4;
    }
    return g_current_screen;
}

// Добавление экрана в историю.
// Защита от дублирования подряд и от переполнения буфера истории.
void push_history(ScreensEnum screen) {
    if (g_history_size > 0 && g_history[g_history_size - 1] == screen) {
        return;
    }

    if (g_history_size < kMaxHistory) {
        g_history[g_history_size++] = screen;
        return;
    }

    for (uint8_t i = 1; i < kMaxHistory; ++i) {
        g_history[i - 1] = g_history[i];
    }
    g_history[kMaxHistory - 1] = screen;
}

// Извлечение последнего экрана из истории.
// Если история пуста, возвращаем MainMenu как безопасный fallback.
ScreensEnum pop_history() {
    if (g_history_size == 0) {
        return SCREEN_ID_MAIN_MENU;
    }
    g_history_size--;
    return g_history[g_history_size];
}

// Выполнение абстрактного Action.
void run_action(const Action &action) {
    switch (action.type) {
        case ActionType::Next:
            next();
            break;
        case ActionType::Back:
            back();
            break;
        case ActionType::GoTo:
            go_to(action.target, action.push_history);
            break;
    }
}

// Универсальный callback LVGL для нажатия кнопок, привязанных через bind_button().
void nav_button_event_cb(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    auto *binding = static_cast<Binding *>(lv_event_get_user_data(event));
    if (binding == nullptr) {
        return;
    }

    run_action(binding->action);
}

// Проверка валидности указателя кнопки.
bool button_exists(lv_obj_t *button) {
    return button != nullptr && lv_obj_is_valid(button);
}

// Поиск существующей привязки для кнопки.
Binding *find_binding(lv_obj_t *button) {
    for (uint16_t i = 0; i < g_binding_count; ++i) {
        if (g_bindings[i].button == button) {
            return &g_bindings[i];
        }
    }
    return nullptr;
}

// Проверка, что объект является кнопкой LVGL.
bool is_button(lv_obj_t *obj) {
    return lv_obj_check_type(obj, &lv_button_class);
}

// Рекурсивный проход дерева объектов для массовых привязок.
// Параметры:
// root           - корневой объект области, относительно которой считаем левую зону
// node           - текущий обходный узел
// action         - действие для назначения
// left_zone_only - если true, связываем только кнопки в левой зоне
// max_x          - ширина левой зоны (в px) от левого края root
uint16_t bind_buttons_recursive(lv_obj_t *root, lv_obj_t *node, Action action, bool left_zone_only, lv_coord_t max_x) {
    uint16_t count = 0;

    if (is_button(node)) {
        bool should_bind = true;

        if (left_zone_only) {
            lv_area_t node_area;
            lv_area_t root_area;
            lv_obj_get_coords(node, &node_area);
            lv_obj_get_coords(root, &root_area);
            // Узел считается "в левой зоне", если его левый край не выходит за предел max_x.
            should_bind = node_area.x1 <= (root_area.x1 + max_x);
        }

        if (should_bind && bind_button(node, action)) {
            count++;
        }
    }

    const uint32_t child_count = lv_obj_get_child_count(node);
    for (uint32_t i = 0; i < child_count; ++i) {
        lv_obj_t *child = lv_obj_get_child(node, static_cast<int32_t>(i));
        count += bind_buttons_recursive(root, child, action, left_zone_only, max_x);
    }

    return count;
}

} // namespace

// Фабрика действия "следующий экран".
Action next_action(bool push_history) {
    return Action{ActionType::Next, SCREEN_ID_LOAD, push_history};
}

// Фабрика действия "назад по истории".
Action back_action() {
    return Action{ActionType::Back, SCREEN_ID_LOAD, false};
}

// Фабрика действия "перейти на конкретный экран".
Action goto_action(ScreensEnum target, bool push_history) {
    return Action{ActionType::GoTo, target, push_history};
}

// Инициализация внутреннего состояния навигатора.
void init() {
    g_binding_count = 0;
    g_history_size = 0;

    g_current_screen = active_screen_to_enum();
}

// Готовность навигатора: проверяем, что экраны созданы.
bool is_ready() {
    return button_exists(objects.load) && button_exists(objects.main_menu) &&
           button_exists(objects.def_page1) && button_exists(objects.def_page2) &&
           button_exists(objects.def_page3) && button_exists(objects.def_page4);
}

// Возвращает актуальный текущий экран.
ScreensEnum current_screen() {
    g_current_screen = active_screen_to_enum();
    return g_current_screen;
}

// Основной метод перехода между экранами.
// Здесь централизованы:
// - проверка готовности
// - обновление истории
// - вызов loadScreen() из сгенерированного EEZ UI
void go_to(ScreensEnum target, bool push_history_flag) {
    if (!is_ready()) {
        return;
    }

    const ScreensEnum from = current_screen();
    if (push_history_flag && from != target) {
        push_history(from);
    }

    if (screen_to_obj(target) == nullptr) {
        // Защита от перехода на неинициализированный экран.
        return;
    }

    loadScreen(target);
    g_current_screen = target;
}

// Переход по кругу между экранами [_SCREEN_ID_FIRST.._SCREEN_ID_LAST].
void next() {
    const ScreensEnum from = current_screen();
    int next_screen = static_cast<int>(from) + 1;
    if (next_screen > static_cast<int>(_SCREEN_ID_LAST)) {
        next_screen = static_cast<int>(_SCREEN_ID_FIRST);
    }
    go_to(static_cast<ScreensEnum>(next_screen), true);
}

// Возврат к предыдущему экрану из истории.
void back() {
    const ScreensEnum target = pop_history();
    go_to(target, false);
}

// Привязка одной кнопки к действию.
// Поведение:
// - если кнопка уже зарегистрирована, обновляем действие
// - если места в реестре нет, возвращаем false
bool bind_button(lv_obj_t *button, Action action) {
    if (!button_exists(button)) {
        return false;
    }

    Binding *existing = find_binding(button);
    if (existing != nullptr) {
        existing->action = action;
        return true;
    }

    if (g_binding_count >= kMaxBindings) {
        return false;
    }

    Binding *binding = &g_bindings[g_binding_count++];
    binding->button = button;
    binding->action = action;

    // user_data указывает на Binding, чтобы callback мог выполнить нужное действие.
    lv_obj_add_event_cb(button, nav_button_event_cb, LV_EVENT_CLICKED, binding);
    return true;
}

// Привязка действия ко всем кнопкам в дереве root.
uint16_t bind_all_buttons(lv_obj_t *root, Action action) {
    if (root == nullptr || !lv_obj_is_valid(root)) {
        return 0;
    }
    return bind_buttons_recursive(root, root, action, false, 0);
}

// Привязка действия к кнопкам только в левой части root.
uint16_t bind_left_zone_buttons(lv_obj_t *root, lv_coord_t max_x, Action action) {
    if (root == nullptr || !lv_obj_is_valid(root)) {
        return 0;
    }
    return bind_buttons_recursive(root, root, action, true, max_x);
}

// Количество активных привязок.
uint16_t binding_count() {
    return g_binding_count;
}

} // namespace demo::nav
