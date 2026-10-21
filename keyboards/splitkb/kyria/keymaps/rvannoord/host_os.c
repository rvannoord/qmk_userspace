// The one switch for everything OS-specific: Win or Mac, set by QMK's OS detection on
// plug-in and flipped by hand with the SYS key. See host_os.h.
#include "quantum.h"
#include "host_os.h"

static bool     is_mac    = false;
static bool     is_manual = false;            // SYS has overridden detection
static u_host_t detected  = U_HOST_WINDOWS;

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

// SYS only flips between Mac and not-Mac. Each side keeps the detected flavour when
// the detected host is on that side, so a Linux machine flips Linux <-> Mac and an
// iPad iOS <-> Windows.
u_host_t host_kind(void) {
    if (is_mac) return detected == U_HOST_IOS ? U_HOST_IOS : U_HOST_MAC;
    return detected == U_HOST_LINUX ? U_HOST_LINUX : U_HOST_WINDOWS;
}

void host_os_toggle(void) {
    is_mac    = !is_mac;
    is_manual = true;
    apply_host_os();
}

bool process_detected_host_os_user(os_variant_t os) {
    detected  = os == OS_MACOS ? U_HOST_MAC
              : os == OS_IOS   ? U_HOST_IOS
              : os == OS_LINUX ? U_HOST_LINUX
                               : U_HOST_WINDOWS;
    is_mac    = os == OS_MACOS || os == OS_IOS;
    is_manual = false;
    apply_host_os();
    return true;
}
