// The state the master gathers for the slave half: finger slots, the OS swap, the
// layer nibbles and the misfire counter. All of it runs before anything crosses the
// split link, which is the point — the link is where test coverage stops.

#include "scenario.hpp"

extern "C" {
#include "split_state.h"
#include "host_os.h"
}

class SplitState : public Scenario {
   protected:
    void begin() {
        start({a, r, s, t, w, space, n, e, i, o, u, nav_left});
        if (host_is_mac()) host_os_toggle();   // every case starts on Windows
    }
};

// A held home-row mod lights its own finger's cell, on the hand that pressed it.
TEST_F(SplitState, SlotsAreFingers) {
    TestDriver driver;
    record(driver);
    begin();

    press(t, 0);                               // left index, Shift
    at(300);
    EXPECT_EQ(u_sync_build().fingers, U_FIN_L_INDEX);
    release(t, 300);
    at(400);
    EXPECT_EQ(u_sync_build().fingers, 0);

    press(n, 400);                             // right index, also Shift
    at(700);
    EXPECT_EQ(u_sync_build().fingers, U_FIN_R_INDEX);
    release(n, 700);
}

// The whole point of "slots are fingers": changing the host OS must not move a cell.
// Only the label changes, and the label is picked from the flag, not from the mod bits.
TEST_F(SplitState, SlotsDoNotMoveWithTheOs) {
    TestDriver driver;
    record(driver);
    begin();

    press(a, 0);                               // left pinky: GUI on Windows, Ctrl on a Mac
    at(300);
    const u_split_state_t win = u_sync_build();
    EXPECT_EQ(win.fingers, U_FIN_L_PINKY);
    EXPECT_EQ(win.flags & U_FLAG_IS_MAC, 0);

    host_os_toggle();
    at(350);
    const u_split_state_t mac = u_sync_build();
    EXPECT_EQ(mac.fingers, win.fingers);            // the cell has not moved
    EXPECT_NE(mac.flags & U_FLAG_IS_MAC, 0);        // only the label would
    EXPECT_NE(mac.flags & U_FLAG_OS_MANUAL, 0);     // and it was set by hand, not detected
    EXPECT_EQ(win.host, U_HOST_WINDOWS);            // the logo crosses as its own byte
    EXPECT_EQ(mac.host, U_HOST_MAC);

    release(a, 350);
}

// A modifier held with nothing typed is the signature of a home-row misfire.
TEST_F(SplitState, MisfireNeedsNothingTypedAndLongEnough) {
    TestDriver driver;
    record(driver);
    begin();

    const uint8_t before = u_sync_build().anomaly;

    press(t, 0);                               // long, alone
    release(t, 600);
    at(650);
    const uint8_t alone = u_sync_build().anomaly;
    EXPECT_EQ((uint8_t)(alone - before), 1);

    press(t, 700);                             // long, but something was typed
    tap(u, 900);
    release(t, 1300);
    at(1350);
    EXPECT_EQ(u_sync_build().anomaly, alone);

    press(t, 1400);                            // a hold, but below the 500 ms floor
    release(t, 1700);
    at(1750);
    EXPECT_EQ(u_sync_build().anomaly, alone);
}

// Two mods held together with nothing between them is itself a misfire — the case a
// naive "was any key pressed" test misses, because the second mod is also a press.
TEST_F(SplitState, TwoModsWithNoPayloadIsAMisfire) {
    TestDriver driver;
    record(driver);
    begin();

    const uint8_t before = u_sync_build().anomaly;
    press(t, 0);                               // left Shift
    press(e, 100);                             // right Ctrl
    release(e, 800);
    release(t, 810);
    at(860);
    EXPECT_GT((uint8_t)(u_sync_build().anomaly - before), 0);
}

// Active and default layers are separate fields. A stuck default layer is the state
// the panel most needs to shout about, and it is invisible in the active one.
TEST_F(SplitState, LayersAreReportedSeparately) {
    TestDriver driver;
    record(driver);
    begin();

    EXPECT_EQ(u_sync_build().layers & 0x0F, BASE);
    EXPECT_EQ(u_sync_build().layers >> 4, BASE);

    press(space, 0);                           // momentary Nav
    at(300);
    EXPECT_EQ(u_sync_build().layers & 0x0F, NAV);
    EXPECT_EQ(u_sync_build().layers >> 4, BASE);
    release(space, 300);
}

// A real shortcut must NOT count as a misfire. This is the case that was broken:
// process_record_user does not run at a mod-tap's physical press, and when the companion
// key is what forces the hold decision, the companion arrives first — so a detector built
// on press counts sees "nothing typed" for exactly the shortcuts it should ignore.
TEST_F(SplitState, AShortcutIsNotAMisfire) {
    TestDriver driver;
    record(driver);
    begin();

    const uint8_t before = u_sync_build().anomaly;

    press(s, 0);                               // left Ctrl
    tap(u, 100);                               // right-hand key under it: a real shortcut
    release(s, 700);                           // held well past the 500 ms floor
    at(750);
    EXPECT_EQ(u_sync_build().anomaly, before); // used, so not a misfire

    press(s, 800);                             // the same hold with nothing typed
    release(s, 1500);
    at(1550);
    EXPECT_EQ((uint8_t)(u_sync_build().anomaly - before), 1);
}

// seq advances on every build. The slave tests it for inequality, never for +1.
// A build is pure: it reports the state at the instant it is called and nothing else.
// It used to advance a sequence counter, which made the number say how often the
// firmware looked rather than how often it sent — and the master looks twice per main
// loop, once to decide whether anything changed and once for its own panel. The sender
// stamps the sequence now; see "every delivered state steps the sequence by one" in
// tests/unit/test_split_sync.c.
TEST_F(SplitState, ABuildIsPure) {
    TestDriver driver;
    record(driver);
    begin();
    const u_split_state_t a = u_sync_build();
    const u_split_state_t b = u_sync_build();
    EXPECT_EQ(0, memcmp(&a, &b, sizeof(u_split_state_t)));
}
