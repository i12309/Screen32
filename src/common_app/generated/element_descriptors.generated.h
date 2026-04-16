// AUTO-GENERATED FILE.
// Source: src/ui/screens.h + src/ui/screens.c
// DO NOT EDIT MANUALLY. Run: python scripts/generate_ui_meta.py

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common_app/generated/element_ids.generated.h"
#include "common_app/generated/page_ids.generated.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Screen32ElementType {
    SCREEN32_ELEMENT_TYPE_BUTTON = 1,
    SCREEN32_ELEMENT_TYPE_TEXT = 2,
    SCREEN32_ELEMENT_TYPE_VALUE = 3,
    SCREEN32_ELEMENT_TYPE_INPUT = 4,
    SCREEN32_ELEMENT_TYPE_VISIBLE = 5
} Screen32ElementType;

typedef struct Screen32ElementDescriptor {
    uint32_t element_id;
    const char* element_name;
    const char* object_name;
    uint32_t page_id;
    const char* page_name;
    Screen32ElementType element_type;
    bool supports_text;
    bool supports_value;
    bool supports_visible;
    bool supports_color;
    bool emits_button_event;
    bool emits_input_event;
} Screen32ElementDescriptor;

static const Screen32ElementDescriptor g_screen32_element_descriptors[SCREEN32_ELEMENT_DESCRIPTOR_COUNT] = {
    {702393654u, "SCREEN32_ELEMENT_ID_OBJ0", "obj0", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {831058084u, "SCREEN32_ELEMENT_ID_OBJ1", "obj1", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {672468609u, "SCREEN32_ELEMENT_ID_OBJ2", "obj2", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {961230478u, "SCREEN32_ELEMENT_ID_OBJ3", "obj3", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {569930443u, "SCREEN32_ELEMENT_ID_OBJ4", "obj4", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {708221742u, "SCREEN32_ELEMENT_ID_OBJ5", "obj5", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1015067754u, "SCREEN32_ELEMENT_ID_OBJ6", "obj6", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1048767685u, "SCREEN32_ELEMENT_ID_OBJ7", "obj7", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {973007217u, "SCREEN32_ELEMENT_ID_OBJ8", "obj8", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {826558505u, "SCREEN32_ELEMENT_ID_OBJ9", "obj9", 1u, "LOAD", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1033385314u, "SCREEN32_ELEMENT_ID_C_MAIN_MENU", "c_main_menu", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1002020473u, "SCREEN32_ELEMENT_ID_B_MAIN_TASK", "b_main_task", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {803691612u, "SCREEN32_ELEMENT_ID_B_MAIN_PROFILE", "b_main_profile", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {707959544u, "SCREEN32_ELEMENT_ID_B_MAIN_NET", "b_main_net", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1024626306u, "SCREEN32_ELEMENT_ID_B_MAIN_SERVICE", "b_main_service", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1061781130u, "SCREEN32_ELEMENT_ID_B_MAIN_STATS", "b_main_stats", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {600635099u, "SCREEN32_ELEMENT_ID_B_MAIN_SUPPORT", "b_main_support", 2u, "MAIN_MENU", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {809266216u, "SCREEN32_ELEMENT_ID_C_PAGE", "c_page", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {984274153u, "SCREEN32_ELEMENT_ID_C_BAR", "c_bar", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {669373145u, "SCREEN32_ELEMENT_ID_BACK", "back", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {828797990u, "SCREEN32_ELEMENT_ID_TITLE", "title", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1052683807u, "SCREEN32_ELEMENT_ID_OBJ10", "obj10", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1049216991u, "SCREEN32_ELEMENT_ID_SIG_1", "sig_1", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {738287735u, "SCREEN32_ELEMENT_ID_B_SIG1_2", "b_sig1_2", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1027720798u, "SCREEN32_ELEMENT_ID_B_SIG1", "b_sig1", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {883773008u, "SCREEN32_ELEMENT_ID_B_SIG1_1", "b_sig1_1", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1035025291u, "SCREEN32_ELEMENT_ID_NEXT_2", "next_2", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1013391020u, "SCREEN32_ELEMENT_ID_C_BUTTON", "c_button", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {774593373u, "SCREEN32_ELEMENT_ID_B1", "b1", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1067973892u, "SCREEN32_ELEMENT_ID_B2", "b2", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1055168153u, "SCREEN32_ELEMENT_ID_B3", "b3", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {763424694u, "SCREEN32_ELEMENT_ID_B4", "b4", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {778514036u, "SCREEN32_ELEMENT_ID_B5", "b5", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1049958006u, "SCREEN32_ELEMENT_ID_B6", "b6", 3u, "DEF_PAGE1", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {689186219u, "SCREEN32_ELEMENT_ID_C_PAGE_1", "c_page_1", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {958843489u, "SCREEN32_ELEMENT_ID_C_BAR_1", "c_bar_1", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {1037205539u, "SCREEN32_ELEMENT_ID_BACK_1", "back_1", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1032884038u, "SCREEN32_ELEMENT_ID_TITLE_1", "title_1", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {552382372u, "SCREEN32_ELEMENT_ID_OBJ11", "obj11", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {963860651u, "SCREEN32_ELEMENT_ID_NEXT_3", "next_3", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {888856230u, "SCREEN32_ELEMENT_ID_NEXT_4", "next_4", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1071541276u, "SCREEN32_ELEMENT_ID_NEXT_5", "next_5", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1003462613u, "SCREEN32_ELEMENT_ID_C_BUTTON_1", "c_button_1", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {928380825u, "SCREEN32_ELEMENT_ID_OBJ12", "obj12", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {855614194u, "SCREEN32_ELEMENT_ID_OBJ13", "obj13", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {889370051u, "SCREEN32_ELEMENT_ID_OBJ14", "obj14", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {805038689u, "SCREEN32_ELEMENT_ID_OBJ15", "obj15", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {633908857u, "SCREEN32_ELEMENT_ID_OBJ16", "obj16", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {751779747u, "SCREEN32_ELEMENT_ID_OBJ17", "obj17", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {809122073u, "SCREEN32_ELEMENT_ID_OBJ18", "obj18", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {997194724u, "SCREEN32_ELEMENT_ID_OBJ19", "obj19", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {827418395u, "SCREEN32_ELEMENT_ID_OBJ20", "obj20", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {757999713u, "SCREEN32_ELEMENT_ID_OBJ21", "obj21", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {720039174u, "SCREEN32_ELEMENT_ID_OBJ22", "obj22", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {925328050u, "SCREEN32_ELEMENT_ID_OBJ23", "obj23", 4u, "DEF_PAGE2", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {684417468u, "SCREEN32_ELEMENT_ID_C_PAGE_2", "c_page_2", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {546217965u, "SCREEN32_ELEMENT_ID_C_BAR_3", "c_bar_3", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {839392768u, "SCREEN32_ELEMENT_ID_BACK_3", "back_3", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {684863696u, "SCREEN32_ELEMENT_ID_TITLE_3", "title_3", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {688134333u, "SCREEN32_ELEMENT_ID_OBJ24", "obj24", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {624416224u, "SCREEN32_ELEMENT_ID_NEXT_7", "next_7", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {921401456u, "SCREEN32_ELEMENT_ID_NEXT_8", "next_8", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {544800465u, "SCREEN32_ELEMENT_ID_NEXT_9", "next_9", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {537248280u, "SCREEN32_ELEMENT_ID_C_BUTTON_2", "c_button_2", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {974709305u, "SCREEN32_ELEMENT_ID_OBJ25", "obj25", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {571348835u, "SCREEN32_ELEMENT_ID_OBJ26", "obj26", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {663846222u, "SCREEN32_ELEMENT_ID_OBJ27", "obj27", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {711912014u, "SCREEN32_ELEMENT_ID_OBJ28", "obj28", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1023533249u, "SCREEN32_ELEMENT_ID_OBJ29", "obj29", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {884455281u, "SCREEN32_ELEMENT_ID_OBJ30", "obj30", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {695345223u, "SCREEN32_ELEMENT_ID_OBJ31", "obj31", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {912866085u, "SCREEN32_ELEMENT_ID_OBJ32", "obj32", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1013369354u, "SCREEN32_ELEMENT_ID_OBJ33", "obj33", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1039332213u, "SCREEN32_ELEMENT_ID_OBJ34", "obj34", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {926206120u, "SCREEN32_ELEMENT_ID_OBJ35", "obj35", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {643798759u, "SCREEN32_ELEMENT_ID_OBJ36", "obj36", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1066962440u, "SCREEN32_ELEMENT_ID_OBJ37", "obj37", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {833779242u, "SCREEN32_ELEMENT_ID_OBJ38", "obj38", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {852970469u, "SCREEN32_ELEMENT_ID_OBJ39", "obj39", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1065795151u, "SCREEN32_ELEMENT_ID_OBJ40", "obj40", 5u, "DEF_PAGE3", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {916570524u, "SCREEN32_ELEMENT_ID_C_PAGE_3", "c_page_3", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {800731870u, "SCREEN32_ELEMENT_ID_C_BAR_4", "c_bar_4", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {931023985u, "SCREEN32_ELEMENT_ID_BACK_4", "back_4", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {585286522u, "SCREEN32_ELEMENT_ID_TITLE_4", "title_4", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {948043762u, "SCREEN32_ELEMENT_ID_OBJ41", "obj41", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {920710893u, "SCREEN32_ELEMENT_ID_NEXT_10", "next_10", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1033400519u, "SCREEN32_ELEMENT_ID_NEXT_11", "next_11", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {600400769u, "SCREEN32_ELEMENT_ID_NEXT_12", "next_12", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {937141394u, "SCREEN32_ELEMENT_ID_C_BUTTON_3", "c_button_3", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_TEXT, true, false, true, true, false, false},
    {858556140u, "SCREEN32_ELEMENT_ID_OBJ42", "obj42", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {826682513u, "SCREEN32_ELEMENT_ID_OBJ43", "obj43", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {956681407u, "SCREEN32_ELEMENT_ID_OBJ44", "obj44", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {980757621u, "SCREEN32_ELEMENT_ID_OBJ45", "obj45", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {832380889u, "SCREEN32_ELEMENT_ID_OBJ46", "obj46", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {892387305u, "SCREEN32_ELEMENT_ID_OBJ47", "obj47", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {925217866u, "SCREEN32_ELEMENT_ID_OBJ48", "obj48", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {947227178u, "SCREEN32_ELEMENT_ID_OBJ49", "obj49", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {842828958u, "SCREEN32_ELEMENT_ID_OBJ50", "obj50", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {1015280446u, "SCREEN32_ELEMENT_ID_OBJ51", "obj51", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {846541941u, "SCREEN32_ELEMENT_ID_OBJ52", "obj52", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {985756782u, "SCREEN32_ELEMENT_ID_OBJ53", "obj53", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
    {965814789u, "SCREEN32_ELEMENT_ID_OBJ54", "obj54", 6u, "DEF_PAGE4", SCREEN32_ELEMENT_TYPE_BUTTON, true, false, true, true, true, false},
};

static inline size_t screen32_element_descriptor_count(void) {
    return (size_t)SCREEN32_ELEMENT_DESCRIPTOR_COUNT;
}

static inline const Screen32ElementDescriptor* screen32_find_element_descriptor(uint32_t element_id) {
    for (size_t i = 0; i < screen32_element_descriptor_count(); ++i) {
        if (g_screen32_element_descriptors[i].element_id == element_id) {
            return &g_screen32_element_descriptors[i];
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
