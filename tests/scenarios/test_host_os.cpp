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
    start({sys, copy, redo, s});
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

    // The swap also reaches the home-row mods: held S is Cmd on the Mac
    reports.clear();
    press(s, 1000);
    release(s, 1250);
    at(1500);
    EXPECT_TRUE(mods_seen() & MOD_MASK_GUI) << "S held should send GUI on the Mac";
    EXPECT_FALSE(mods_seen() & MOD_MASK_CTRL) << "S held should not send Ctrl on the Mac";

    // Back to Windows
    host_os_toggle();
    EXPECT_FALSE(host_is_mac());
    reports.clear();
    tap(copy, 1600);
    at(1800);
    EXPECT_EQ(typed(), "<Ctrl+c>");
}
