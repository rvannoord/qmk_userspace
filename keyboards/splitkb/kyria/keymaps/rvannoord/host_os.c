// The one switch for everything OS-specific: Win or Mac, set by QMK's OS detection on
// plug-in and flipped by hand with the SYS key. See host_os.h.
#include "quantum.h"
#include "host_os.h"

static bool is_mac = false;
static bool is_manual = false;   // SYS has overridden detection

static void apply_host_os(void) {
    keymap_config.swap_lctl_lgui = is_mac;
    keymap_config.swap_rctl_rgui = is_mac;
}

bool host_is_mac(void) {
    return is_mac;
}

bool host_os_is_manual(void) {
    return is_manual;
}

void host_os_toggle(void) {
    is_mac    = !is_mac;
    is_manual = true;
    apply_host_os();
}

bool process_detected_host_os_user(os_variant_t os) {
    is_mac    = os == OS_MACOS || os == OS_IOS;
    is_manual = false;
    apply_host_os();
    return true;
}
