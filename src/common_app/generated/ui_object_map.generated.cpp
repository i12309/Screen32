// AUTO-GENERATED FILE.
// Source: src/ui/screens.h + src/ui/screens.c
// DO NOT EDIT MANUALLY. Run: python tools/ui_meta_gen/generate_ui_meta.py

#include "common_app/generated/ui_object_map.generated.h"

#include "ui/screens.h"

namespace demo {
namespace {

bool bind_page(screenlib::adapter::UiObjectMap& objectMap, uint32_t pageId, lv_obj_t* pageObj) {
    return pageObj != nullptr && objectMap.bindPage(pageId, pageObj);
}

bool bind_element(screenlib::adapter::UiObjectMap& objectMap,
                  uint32_t elementId,
                  uint32_t pageId,
                  lv_obj_t* obj,
                  Screen32BoundElement* outElements,
                  size_t outCapacity,
                  size_t& outCount) {
    if (obj == nullptr) {
        return false;
    }
    if (!objectMap.bindElement(elementId, obj)) {
        return false;
    }

    if (outElements != nullptr && outCount < outCapacity) {
        outElements[outCount].elementId = elementId;
        outElements[outCount].pageId = pageId;
        outElements[outCount].obj = obj;
        outElements[outCount].descriptor = screen32_find_element_descriptor(elementId);
        outCount++;
    }

    return true;
}

}  // namespace

bool screen32_bind_generated_ui_map(screenlib::adapter::UiObjectMap& objectMap,
                                  Screen32BoundElement* outElements,
                                  size_t outCapacity,
                                  size_t* outCount) {
    objectMap.clear();
    size_t boundCount = 0;
    bool allOk = true;

    // Page map
    allOk = bind_page(objectMap, 1u, objects.load) && allOk;
    allOk = bind_page(objectMap, 2u, objects.main_menu) && allOk;
    allOk = bind_page(objectMap, 3u, objects.def_page1) && allOk;
    allOk = bind_page(objectMap, 4u, objects.def_page2) && allOk;
    allOk = bind_page(objectMap, 5u, objects.def_page3) && allOk;
    allOk = bind_page(objectMap, 6u, objects.def_page4) && allOk;

    // Element map

    allOk = bind_element(objectMap, 702393654u, 1u, objects.obj0, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 831058084u, 1u, objects.obj1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 672468609u, 1u, objects.obj2, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 961230478u, 1u, objects.obj3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 569930443u, 1u, objects.obj4, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 708221742u, 1u, objects.obj5, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1015067754u, 1u, objects.obj6, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1048767685u, 1u, objects.obj7, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 973007217u, 1u, objects.obj8, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 826558505u, 1u, objects.obj9, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1033385314u, 2u, objects.c_main_menu, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1002020473u, 2u, objects.b_main_task, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 803691612u, 2u, objects.b_main_profile, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 707959544u, 2u, objects.b_main_net, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1024626306u, 2u, objects.b_main_service, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1061781130u, 2u, objects.b_main_stats, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 600635099u, 2u, objects.b_main_support, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 809266216u, 3u, objects.c_page, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 984274153u, 3u, objects.c_bar, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 669373145u, 3u, objects.back, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 828797990u, 3u, objects.title, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1052683807u, 3u, objects.obj10, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1049216991u, 3u, objects.sig_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 738287735u, 3u, objects.b_sig1_2, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1027720798u, 3u, objects.b_sig1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 883773008u, 3u, objects.b_sig1_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1035025291u, 3u, objects.next_2, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1013391020u, 3u, objects.c_button, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 774593373u, 3u, objects.b1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1067973892u, 3u, objects.b2, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1055168153u, 3u, objects.b3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 763424694u, 3u, objects.b4, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 778514036u, 3u, objects.b5, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1049958006u, 3u, objects.b6, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 689186219u, 4u, objects.c_page_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 958843489u, 4u, objects.c_bar_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1037205539u, 4u, objects.back_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1032884038u, 4u, objects.title_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 552382372u, 4u, objects.obj11, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 963860651u, 4u, objects.next_3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 888856230u, 4u, objects.next_4, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1071541276u, 4u, objects.next_5, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1003462613u, 4u, objects.c_button_1, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 928380825u, 4u, objects.obj12, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 855614194u, 4u, objects.obj13, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 889370051u, 4u, objects.obj14, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 805038689u, 4u, objects.obj15, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 633908857u, 4u, objects.obj16, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 751779747u, 4u, objects.obj17, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 809122073u, 4u, objects.obj18, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 997194724u, 4u, objects.obj19, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 827418395u, 4u, objects.obj20, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 757999713u, 4u, objects.obj21, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 720039174u, 4u, objects.obj22, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 925328050u, 4u, objects.obj23, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 684417468u, 5u, objects.c_page_2, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 546217965u, 5u, objects.c_bar_3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 839392768u, 5u, objects.back_3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 684863696u, 5u, objects.title_3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 688134333u, 5u, objects.obj24, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 624416224u, 5u, objects.next_7, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 921401456u, 5u, objects.next_8, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 544800465u, 5u, objects.next_9, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 537248280u, 5u, objects.c_button_2, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 974709305u, 5u, objects.obj25, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 571348835u, 5u, objects.obj26, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 663846222u, 5u, objects.obj27, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 711912014u, 5u, objects.obj28, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1023533249u, 5u, objects.obj29, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 884455281u, 5u, objects.obj30, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 695345223u, 5u, objects.obj31, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 912866085u, 5u, objects.obj32, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1013369354u, 5u, objects.obj33, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1039332213u, 5u, objects.obj34, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 926206120u, 5u, objects.obj35, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 643798759u, 5u, objects.obj36, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1066962440u, 5u, objects.obj37, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 833779242u, 5u, objects.obj38, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 852970469u, 5u, objects.obj39, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1065795151u, 5u, objects.obj40, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 916570524u, 6u, objects.c_page_3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 800731870u, 6u, objects.c_bar_4, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 931023985u, 6u, objects.back_4, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 585286522u, 6u, objects.title_4, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 948043762u, 6u, objects.obj41, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 920710893u, 6u, objects.next_10, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1033400519u, 6u, objects.next_11, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 600400769u, 6u, objects.next_12, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 937141394u, 6u, objects.c_button_3, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 858556140u, 6u, objects.obj42, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 826682513u, 6u, objects.obj43, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 956681407u, 6u, objects.obj44, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 980757621u, 6u, objects.obj45, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 832380889u, 6u, objects.obj46, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 892387305u, 6u, objects.obj47, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 925217866u, 6u, objects.obj48, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 947227178u, 6u, objects.obj49, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 842828958u, 6u, objects.obj50, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 1015280446u, 6u, objects.obj51, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 846541941u, 6u, objects.obj52, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 985756782u, 6u, objects.obj53, outElements, outCapacity, boundCount) && allOk;
    allOk = bind_element(objectMap, 965814789u, 6u, objects.obj54, outElements, outCapacity, boundCount) && allOk;

    if (outCount != nullptr) {
        *outCount = boundCount;
    }

    return allOk;
}

const Screen32BoundElement* screen32_find_bound_element(const Screen32BoundElement* elements,
                                                     size_t count,
                                                     uint32_t elementId) {
    if (elements == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < count; ++i) {
        if (elements[i].elementId == elementId) {
            return &elements[i];
        }
    }
    return nullptr;
}

}  // namespace demo
