#include "quantum.h"
#include "host_os.h"

static bool is_mac = false;

static void apply_host_os(void) {
    keymap_config.swap_lctl_lgui = is_mac;
    keymap_config.swap_rctl_rgui = is_mac;
}

bool host_is_mac(void) {
    return is_mac;
}

void host_os_toggle(void) {
    is_mac = !is_mac;
    apply_host_os();
}

bool process_detected_host_os_user(os_variant_t os) {
    is_mac = os == OS_MACOS || os == OS_IOS;
    apply_host_os();
    return true;
}
