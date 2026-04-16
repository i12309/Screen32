#include "common_app/frontend_service_responder.h"

#include <string.h>

#include "common_app/page_state_builder.h"

namespace demo {

namespace {

void copy_text_safe(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    dst[0] = '\0';
    if (src == nullptr) {
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

} // namespace

DeviceInfo frontend_build_device_info(FrontendMode mode) {
    DeviceInfo info = DeviceInfo_init_zero;
    info.protocol_version = 1;
    copy_text_safe(info.fw_version, sizeof(info.fw_version), "screen32-frontend");
    copy_text_safe(info.ui_version, sizeof(info.ui_version), "eez-lvgl");
    copy_text_safe(info.screen_type, sizeof(info.screen_type), "screen32");
    copy_text_safe(info.client_type, sizeof(info.client_type), frontend_mode_name(mode));
    copy_text_safe(info.device_id, sizeof(info.device_id), "screen32-demo");
    copy_text_safe(info.instance_id, sizeof(info.instance_id), "default");
    info.capabilities = 0;
    return info;
}

void frontend_handle_service_request(const Envelope& env, const FrontendServiceResponderContext& ctx) {
    if (ctx.client == nullptr) {
        return;
    }

    switch (env.which_payload) {
        case Envelope_request_device_info_tag: {
            ctx.client->sendHello(frontend_build_device_info(ctx.mode));
            break;
        }
        case Envelope_request_current_page_tag: {
            ctx.client->sendCurrentPage(screen32_current_page_id(), env.payload.request_current_page.request_id);
            break;
        }
        case Envelope_request_page_state_tag: {
            PageState state = PageState_init_zero;
            frontend_build_page_state(
                ctx.trackedElements,
                ctx.trackedCount,
                env.payload.request_page_state.page_id,
                env.payload.request_page_state.request_id,
                state);
            ctx.client->sendPageState(state);
            break;
        }
        case Envelope_request_element_state_tag: {
            ElementState state = ElementState_init_zero;
            frontend_build_element_state(
                ctx.trackedElements,
                ctx.trackedCount,
                env.payload.request_element_state.page_id,
                env.payload.request_element_state.element_id,
                env.payload.request_element_state.request_id,
                state);
            ctx.client->sendElementState(state);
            break;
        }
        default:
            break;
    }
}

} // namespace demo
