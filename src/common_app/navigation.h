#pragma once

#include <stdint.h>
#include <lvgl.h>

extern "C" {
#include "ui/screens.h"
}

namespace demo::nav {

/*
 * Файл src/common_app/navigation.h
 * Назначение: объявления API навигации для ветки исходников `src/`.
 */

// Тип действия, которое выполняется при нажатии кнопки.
// Next  - переход на следующий экран по кругу
// Back  - возврат по истории
// GoTo  - переход на конкретный экран
enum class ActionType : uint8_t {
    Next,
    Back,
    GoTo
};

// Универсальная структура действия для привязки к кнопке.
// target используется для GoTo; для Next/Back поле игнорируется.
// push_history определяет, нужно ли сохранять текущий экран в историю
// перед переходом (актуально для GoTo/Next).
struct Action {
    ActionType type;
    ScreensEnum target;
    bool push_history;
};

// Фабрики действий для удобного и читаемого API.
Action next_action(bool push_history = true);
Action back_action();
Action goto_action(ScreensEnum target, bool push_history = true);

// Инициализация внутреннего состояния навигатора:
// сброс привязок, истории и синхронизация текущего экрана.
void init();

// Проверка готовности навигации: созданы ли ключевые экраны EEZ.
bool is_ready();

// Возвращает текущий экран на основе активного lv_scr_act().
ScreensEnum current_screen();

// Явный переход на target.
// push_history=true: текущий экран попадет в стек истории.
void go_to(ScreensEnum target, bool push_history = true);

// Переход на следующий экран по кругу.
void next();

// Возврат на предыдущий экран из истории.
void back();

// Привязка одной кнопки к действию.
// Если кнопка уже была привязана ранее, действие обновляется.
bool bind_button(lv_obj_t *button, Action action);

// Массовая привязка: рекурсивно проходит root и все дочерние объекты,
// выбирает объекты класса Button и назначает им действие.
uint16_t bind_all_buttons(lv_obj_t *root, Action action);

// Массовая привязка кнопок только в левой зоне root.
// max_x задает ширину зоны от левого края root.
uint16_t bind_left_zone_buttons(lv_obj_t *root, lv_coord_t max_x, Action action);

// Количество текущих зарегистрированных привязок.
uint16_t binding_count();

// Явное соответствие кнопки целевому экрану.
struct DirectRoute {
    lv_obj_t **button;
    ScreensEnum target;
};

} // namespace demo::nav
