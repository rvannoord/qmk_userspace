// Scenarios 1-9 from MIGRATION.md. They run against both the old tree (bilateral combinations) and
// the new setup (Chordal Hold + Flow Tap); a scenario that passes in one and fails in the other is a
// behaviour difference. Expectations are the new setup's intended behaviour. Times are milliseconds
// from the first press. Colemak-DH home row: A=GUI R=Alt S=Ctrl T=Shift | N=Shift E=Ctrl I=Alt O=GUI.

#include "scenario.hpp"

// 1. Same-hand fast roll
TEST_F(Scenario, S1_SameHandFastRoll) {
    TestDriver driver;
    record(driver);
    start({s, t});

    press(s, 0);
    press(t, 30);
    release(s, 60);
    release(t, 90);
    at(500);

    EXPECT_EQ(typed(), "st");
}

// 2. Same-hand slow: S held past the tapping term, then T. Known difference: the old code waited 3 s
// and typed "st".
TEST_F(Scenario, S2_SameHandSlow) {
    TestDriver driver;
    record(driver);
    start({s, t});

    press(s, 0);
    press(t, 250);
    release(t, 290);
    release(s, 330);
    at(700);

    EXPECT_EQ(typed(), "<Ctrl+t>");
}

// 3. Opposite-hand roll, T released first
TEST_F(Scenario, S3_OppositeHandRoll) {
    TestDriver driver;
    record(driver);
    start({t, u});

    press(t, 0);
    press(u, 40);
    release(t, 70);
    release(u, 100);
    at(500);

    EXPECT_EQ(typed(), "tu");
}

// 4. Opposite-hand early press: U inside the 80 ms crossover window, T held past the term. T settles
// as a tap but stays down until released. Known difference: the old code sent a single tap of T.
TEST_F(Scenario, S4_OppositeHandEarlyPress) {
    TestDriver driver;
    record(driver);
    start({t, u});

    press(t, 0);
    press(u, 40);
    release(u, 100);
    release(t, 250);
    at(600);

    EXPECT_EQ(typed(), "tu");
    EXPECT_TRUE(held_together(KC_T, KC_U)) << "T should still be down when U is pressed";
}

// 5. Opposite-hand shortcut: U after the crossover window, T held past the term
TEST_F(Scenario, S5_OppositeHandShortcut) {
    TestDriver driver;
    record(driver);
    start({t, u});

    press(t, 0);
    press(u, 120);
    release(u, 230);
    release(t, 260);
    at(600);

    EXPECT_EQ(typed(), "U");
}

// 6. Typing streak: S right after a typed letter is a letter, even when held
TEST_F(Scenario, S6_TypingStreak) {
    TestDriver driver;
    record(driver);
    start({a, s});

    tap(a, 0);
    press(s, 100);
    release(s, 400);
    at(800);

    EXPECT_EQ(typed(), "as");
    EXPECT_FALSE(mods_seen() & MOD_MASK_CTRL) << "Ctrl should never be sent";
}

// 7. Shift mid-word: Shift is exempt from the typing streak
TEST_F(Scenario, S7_ShiftMidWord) {
    TestDriver driver;
    record(driver);
    start({e, t, u});

    tap(e, 0);
    press(t, 100);
    press(u, 330);
    release(u, 370);
    release(t, 400);
    at(800);

    EXPECT_EQ(typed(), "eU");
}

// 8a. GUI head start: W at 100 ms is inside GUI's 120 ms crossover window, so a roll
TEST_F(Scenario, S8a_GuiRoll) {
    TestDriver driver;
    record(driver);
    start({o, w});

    press(o, 0);
    press(w, 100);
    release(w, 150);
    release(o, 250);
    at(600);

    EXPECT_EQ(typed(), "ow");
}

// 8b. GUI head start: W at 150 ms is past the window, O held past the term
TEST_F(Scenario, S8b_GuiShortcut) {
    TestDriver driver;
    record(driver);
    start({o, w});

    press(o, 0);
    press(w, 150);
    release(w, 230);
    release(o, 260);
    at(600);

    EXPECT_EQ(typed(), "<GUI+w>");
}

// 9. Thumb layer: Space held past the term, a quick right-hand Nav key within 50 ms of it
TEST_F(Scenario, S9_ThumbLayer) {
    TestDriver driver;
    record(driver);
    start({space, n, nav_left});

    press(space, 0);
    press(n, 50);
    release(n, 120);
    release(space, 300);
    at(700);

    EXPECT_EQ(typed(), "<LEFT>");
}

// Crossover boundaries: both the old code and tap_hold.c count an opposite-hand key pressed up to and
// including 80 ms after the mod (120 ms for GUI) as a roll, and one pressed later as a shortcut.
TEST_F(Scenario, Boundary_ShiftCrossover) {
    TestDriver driver;
    record(driver);

    start({t, u});
    press(t, 0);
    press(u, 80);
    release(u, 110);
    release(t, 450);
    at(800);
    EXPECT_EQ(typed(), "tu");

    start({t, u});
    press(t, 0);
    press(u, 81);
    release(u, 111);
    release(t, 450);
    at(800);
    EXPECT_EQ(typed(), "U");
}

TEST_F(Scenario, Boundary_GuiCrossover) {
    TestDriver driver;
    record(driver);

    start({o, w});
    press(o, 0);
    press(w, 120);
    release(w, 150);
    release(o, 450);
    at(800);
    EXPECT_EQ(typed(), "ow");

    start({o, w});
    press(o, 0);
    press(w, 121);
    release(w, 151);
    release(o, 450);
    at(800);
    EXPECT_EQ(typed(), "<GUI+w>");
}
