// АВТОСГЕНЕРИРОВАННЫЙ ФАЙЛ.
// Источник: src/ui/screens.h + src/ui/screens.c
// НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ. Запуск: python tools/ui_meta_gen/generate_ui_meta.py

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "common_app/generated/page_ids.generated.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Screen32PageDescriptor {
    uint32_t page_id;
    const char* page_name;
    const char* object_name;
} Screen32PageDescriptor;

static const Screen32PageDescriptor g_screen32_page_descriptors[SCREEN32_PAGE_DESCRIPTOR_COUNT] = {
    {1u, "LOAD", "load"},
    {2u, "MAIN_MENU", "main_menu"},
    {3u, "DEF_PAGE1", "def_page1"},
    {4u, "DEF_PAGE2", "def_page2"},
    {5u, "DEF_PAGE3", "def_page3"},
    {6u, "DEF_PAGE4", "def_page4"},
};

static inline size_t screen32_page_descriptor_count(void) {
    return (size_t)SCREEN32_PAGE_DESCRIPTOR_COUNT;
}

static inline const Screen32PageDescriptor* screen32_find_page_descriptor(uint32_t page_id) {
    for (size_t i = 0; i < screen32_page_descriptor_count(); ++i) {
        if (g_screen32_page_descriptors[i].page_id == page_id) {
            return &g_screen32_page_descriptors[i];
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
