// АВТОСГЕНЕРИРОВАННЫЙ ФАЙЛ.
// Источник: src/ui/screens.h + src/ui/screens.c
// НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ. Запуск: python tools/ui_meta_gen/generate_ui_meta.py

#include "common_app/generated/ui_object_map.generated.h"

#include "ui/screens.h"
#include "ui/ui.h"

namespace demo {

uint32_t screen32_current_page_id() {
    lv_obj_t* active = lv_scr_act();
    if (active == objects.load) return 1u;
    if (active == objects.main_menu) return 2u;
    if (active == objects.def_page1) return 3u;
    if (active == objects.def_page2) return 4u;
    if (active == objects.def_page3) return 5u;
    if (active == objects.def_page4) return 6u;
    return 1u;
}

bool screen32_load_page_by_target(void* pageTarget) {
    lv_obj_t* target = static_cast<lv_obj_t*>(pageTarget);
    if (target == nullptr) {
        return false;
    }
    if (target == objects.load) { loadScreen(SCREEN_ID_LOAD); return true; }
    if (target == objects.main_menu) { loadScreen(SCREEN_ID_MAIN_MENU); return true; }
    if (target == objects.def_page1) { loadScreen(SCREEN_ID_DEF_PAGE1); return true; }
    if (target == objects.def_page2) { loadScreen(SCREEN_ID_DEF_PAGE2); return true; }
    if (target == objects.def_page3) { loadScreen(SCREEN_ID_DEF_PAGE3); return true; }
    if (target == objects.def_page4) { loadScreen(SCREEN_ID_DEF_PAGE4); return true; }
    return false;
}

}  // namespace demo
