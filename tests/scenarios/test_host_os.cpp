// Scenario 10: host OS handling in keymap.c and host_os.c (new setup only). OS detection runs on USB
// setup packets and can't be exercised here, so the Mac is selected with the SYS key and
// host_os_toggle().

#include "scenario.hpp"

extern "C" {
#include "host_os.h"
}
#include "manna-harbour_miryoku.h" // U_CPY, U_RDO; MIRYOKU_CLIPBOARD_WIN is set in config.h

TEST_F(Scenario, S10_HostOs) {
    TestDriver driver;
    record(driver);
    KeymapKey sys{BASE, 0, 1, U_SYS};
    KeymapKey copy{BASE, 2, 1, U_CPY};
    KeymapKey redo{BASE, 3, 1, U_RDO};
    start({sys, copy, redo, a, s});
    ASSERT_FALSE(host_is_mac());

    // Windows: Ctrl-based clipboard keys
    tap(copy, 0);
    tap(redo, 100);
    at(300);
    EXPECT_EQ(typed(), "<Ctrl+c><Ctrl+y>");

    // SYS switches to the Mac
    tap(sys, 400);
    at(500);
    EXPECT_TRUE(host_is_mac());

    reports.clear();
    tap(copy, 600);
    tap(redo, 700);
    at(900);
    EXPECT_EQ(typed(), "<GUI+c><Shift+GUI+z>");

    // The home-row mods stay put on the Mac (no Ctrl/GUI swap since 2026-10-27): held S
    // is Ctrl and held A is GUI, as on Windows
    reports.clear();
    press(s, 1000);
    release(s, 1250);
    at(1300);
    EXPECT_TRUE(mods_seen() & MOD_MASK_CTRL) << "S held should send Ctrl on the Mac";
    EXPECT_FALSE(mods_seen() & MOD_MASK_GUI) << "S held should not send GUI on the Mac";
    reports.clear();
    press(a, 1300);
    release(a, 1550);
    at(1600);
    EXPECT_TRUE(mods_seen() & MOD_MASK_GUI) << "A held should send GUI on the Mac";
    EXPECT_FALSE(mods_seen() & MOD_MASK_CTRL) << "A held should not send Ctrl on the Mac";

    // Back to Windows
    host_os_toggle();
    EXPECT_FALSE(host_is_mac());
    reports.clear();
    tap(copy, 1700);
    at(1900);
    EXPECT_EQ(typed(), "<Ctrl+c>");
}

// The two thumb keys of 2026-10-27: herdr's prefix is a real Ctrl+B and Hyper all four
// mods, the same on both hosts.
TEST_F(Scenario, S10_ThumbKeys) {
    TestDriver driver;
    record(driver);
    KeymapKey herdr{BASE, 5, 1, U_HERDR};
    KeymapKey hyper{BASE, 6, 1, KC_HYPR};
    start({herdr, hyper, w});
    ASSERT_FALSE(host_is_mac());

    for (int mac = 0; mac < 2; mac++) {
        const uint32_t t0 = mac * 1000;
        reports.clear();
        tap(herdr, t0);
        at(t0 + 100);
        press(hyper, t0 + 100);
        tap(w, t0 + 200);
        release(hyper, t0 + 300);
        at(t0 + 400);
        EXPECT_EQ(typed(), "<Ctrl+b><Ctrl+Alt+Shift+GUI+w>") << (mac ? "on the Mac" : "on Windows");
        host_os_toggle();                       // to the Mac, then back to Windows
    }
    EXPECT_FALSE(host_is_mac());
}

// The herdr key held (2026-11-11): every key tapped meanwhile is the prefix, then Alt+key,
// with a basic key, a mod-tap on either hand and the Alt key itself; held Shift goes with
// the key and stays off the prefix. Tapped alone it is still the prefix, now on release.
TEST_F(Scenario, S11_HerdrHeld) {
    TestDriver driver;
    record(driver);
    KeymapKey herdr{BASE, 5, 3, U_HERDR};
    start({herdr, w, n, r, t, u});

    press(herdr, 0);
    at(150);
    EXPECT_EQ(typed(), "") << "nothing until the herdr key is let go or a key is tapped";
    release(herdr, 150);
    at(200);
    EXPECT_EQ(typed(), "<Ctrl+b>");

    reports.clear();
    press(herdr, 1000);
    tap(w, 1100);
    tap(n, 1300);
    tap(r, 1500);
    release(herdr, 1700);
    at(1800);
    EXPECT_EQ(typed(), "<Ctrl+b><Alt+w><Ctrl+b><Alt+n><Ctrl+b><Alt+r>") << "and no prefix on release";

    reports.clear();
    press(herdr, 3000);
    press(t, 3100);                          // Shift, held past the tapping term
    tap(u, 3400);
    release(t, 3500);
    release(herdr, 3600);
    at(3700);
    EXPECT_EQ(typed(), "<Ctrl+b><Alt+Shift+u>");

    // The known cost: a roll, the next key down before the thumb lets go, counts as held.
    reports.clear();
    press(herdr, 5000);
    press(w, 5040);
    release(herdr, 5060);
    release(w, 5090);
    at(5200);
    EXPECT_EQ(typed(), "<Ctrl+b><Alt+w>");
}

// What the panel shows, which is more than what the keyboard does. Detection is called
// by hand here, as QMK calls it once the USB setup packets have been counted.
TEST_F(Scenario, S10_HostKind) {
    TestDriver driver;
    record(driver);
    start({s});

    process_detected_host_os_user(OS_LINUX);
    EXPECT_EQ(host_kind(), U_HOST_LINUX);
    EXPECT_FALSE(host_is_mac()) << "Linux behaves like Windows";
    EXPECT_FALSE(host_os_is_manual());

    host_os_toggle();                       // SYS: a Linux machine flips to the Mac...
    EXPECT_EQ(host_kind(), U_HOST_MAC);
    EXPECT_TRUE(host_is_mac());
    EXPECT_TRUE(host_os_is_manual());
    host_os_toggle();                       // ...and back to Linux, not to Windows
    EXPECT_EQ(host_kind(), U_HOST_LINUX);
    EXPECT_FALSE(host_is_mac());

    process_detected_host_os_user(OS_IOS);
    EXPECT_EQ(host_kind(), U_HOST_IOS);
    EXPECT_TRUE(host_is_mac()) << "iPadOS shortcuts are Cmd-based, so iOS is a Mac";
    host_os_toggle();
    EXPECT_EQ(host_kind(), U_HOST_WINDOWS);
    host_os_toggle();
    EXPECT_EQ(host_kind(), U_HOST_IOS);

    process_detected_host_os_user(OS_MACOS);
    EXPECT_EQ(host_kind(), U_HOST_MAC);
    process_detected_host_os_user(OS_UNSURE);
    EXPECT_EQ(host_kind(), U_HOST_WINDOWS) << "an unsure detection is Windows, as before";

    process_detected_host_os_user(OS_WINDOWS);   // leave the next test on Windows
    EXPECT_EQ(host_kind(), U_HOST_WINDOWS);
    EXPECT_FALSE(host_is_mac());
}
