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

// The axe fires when a command modifier was actually used — something was typed while
// it was held. Shift is excluded: here it is a home-row mod, part of typing a capital
// rather than an instruction, and it fires so often that an 800 ms dwell never expires.
TEST_F(SplitState, TraceCountsUsedCommandModsOnly) {
    TestDriver driver;
    record(driver);
    begin();

    const uint8_t before = u_sync_build().trace;

    // The payload key must be on the OTHER hand: Chordal Hold turns a same-hand roll
    // into letters, so s + w would resolve as a tap and never be a held modifier at all.
    press(s, 0);                               // left Ctrl, with a right-hand key under it
    tap(u, 100);
    release(s, 300);
    at(350);
    const uint8_t used = u_sync_build().trace;
    EXPECT_EQ((uint8_t)(used - before), 1);

    press(t, 400);                             // left Shift, used — excluded
    tap(u, 500);
    release(t, 700);
    at(750);
    EXPECT_EQ(u_sync_build().trace, used);

    press(s, 800);                             // Ctrl, held but never used
    release(s, 1400);
    at(1450);
    EXPECT_EQ(u_sync_build().trace, used);     // no command was issued
}

// seq advances on every build. The slave tests it for inequality, never for +1.
TEST_F(SplitState, SeqAdvances) {
    TestDriver driver;
    record(driver);
    begin();
    EXPECT_NE(u_sync_build().seq, u_sync_build().seq);
}
