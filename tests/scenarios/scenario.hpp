// Shared fixture for the scenario tests: Miryoku's home-row keys on QMK's test matrix, a clock in
// milliseconds since the first press, and a recorder that turns keyboard reports into what the host
// would show.
#pragma once

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "keyboard_report_util.hpp"
#include "test_common.hpp"

// The test matrix is 4 rows x 10 columns and not split. The old bilateral code takes columns 0-4 as
// the left hand, tap_hold.c takes rows 0-1. Left-hand keys sit in rows 0-1 and columns 0-4,
// right-hand keys in rows 2-3 and columns 5-9, so both trees agree on which hand a key is.
constexpr layer_t BASE = 0;
constexpr layer_t NAV  = 1; // Miryoku's U_NAV is 4; only the number differs

class Scenario : public TestFixture {
   protected:
    // Left hand (rows 0-1, columns 0-4), Colemak-DH as in Miryoku
    KeymapKey a{BASE, 0, 0, LGUI_T(KC_A)};
    KeymapKey r{BASE, 1, 0, LALT_T(KC_R)};
    KeymapKey s{BASE, 2, 0, LCTL_T(KC_S)};
    KeymapKey t{BASE, 3, 0, LSFT_T(KC_T)};
    KeymapKey w{BASE, 1, 1, KC_W};
    KeymapKey space{BASE, 4, 1, LT(NAV, KC_SPC)};
    // Right hand (rows 2-3, columns 5-9)
    KeymapKey n{BASE, 6, 2, LSFT_T(KC_N)};
    KeymapKey e{BASE, 7, 2, LCTL_T(KC_E)};
    KeymapKey i{BASE, 8, 2, LALT_T(KC_I)};
    KeymapKey o{BASE, 9, 2, LGUI_T(KC_O)};
    KeymapKey u{BASE, 7, 3, KC_U};
    // Miryoku's Nav layer has Left where N is
    KeymapKey nav_left{NAV, 6, 2, KC_LEFT};
    // Not part of any scenario, only tapped by start()
    KeymapKey idle_key{BASE, 9, 3, KC_QUOT};

    std::vector<report_keyboard_t> reports;
    uint32_t                       now = 0;

    void record(TestDriver &driver) {
        EXPECT_CALL(driver, send_keyboard_mock(testing::_)).WillRepeatedly(testing::Invoke([this](report_keyboard_t &report) { reports.push_back(report); }));
    }

    // Sets the keymap and puts both trees in the same state: the last key was typed 500 ms ago, so
    // neither the old typing streak nor Flow Tap is active. Needed because the test clock restarts at
    // 0 for every test while the old bilateral code keeps its last-key time in static state.
    void start(std::initializer_list<KeymapKey> keys) {
        set_keymap(keys);
        add_key(idle_key);
        idle_key.press();
        run_one_scan_loop();
        idle_key.release();
        run_one_scan_loop();
        idle_for(500);
        now = 0;
        reports.clear();
    }

    // Runs the scan loop until `ms` after the scenario's first press.
    void at(uint32_t ms) {
        if (ms > now) {
            idle_for(ms - now);
            now = ms;
        }
    }

    void press(KeymapKey &key, uint32_t ms) {
        at(ms);
        key.press();
        run_one_scan_loop();
        now++;
    }

    void release(KeymapKey &key, uint32_t ms) {
        at(ms);
        key.release();
        run_one_scan_loop();
        now++;
    }

    void tap(KeymapKey &key, uint32_t ms) {
        press(key, ms);
        release(key, ms + 30);
    }

    // One entry per newly pressed key: letters as typed, Shift+letter in upper case, anything else
    // as <Mods+key>. For example "st", "U", "<Ctrl+t>", "<LEFT>", "<Shift+GUI+z>".
    std::string typed() const {
        std::string          out;
        std::vector<uint8_t> held;
        for (const auto &report : reports) {
            std::vector<uint8_t> keys;
            for (int k = 0; k < KEYBOARD_REPORT_KEYS; k++) {
                if (report.keys[k]) keys.push_back(report.keys[k]);
            }
            for (uint8_t key : keys) {
                if (std::find(held.begin(), held.end(), key) == held.end()) out += describe(key, report.mods);
            }
            held = keys;
        }
        return out;
    }

    // Every modifier that showed up in any report.
    uint8_t mods_seen() const {
        uint8_t mods = 0;
        for (const auto &report : reports) mods |= report.mods;
        return mods;
    }

    // Whether some report has both keys down at once.
    bool held_together(uint8_t first, uint8_t second) const {
        for (const auto &report : reports) {
            bool has_first = false, has_second = false;
            for (int k = 0; k < KEYBOARD_REPORT_KEYS; k++) {
                has_first |= report.keys[k] == first;
                has_second |= report.keys[k] == second;
            }
            if (has_first && has_second) return true;
        }
        return false;
    }

   private:
    static std::string describe(uint8_t key, uint8_t mods) {
        const bool  letter = key >= KC_A && key <= KC_Z;
        std::string name;
        if (letter) {
            name = std::string(1, (char)('a' + (key - KC_A)));
        } else if (key == KC_SPC) {
            name = "SPC";
        } else if (key == KC_LEFT) {
            name = "LEFT";
        } else {
            char hex[8];
            snprintf(hex, sizeof hex, "0x%02X", key);
            name = hex;
        }
        if (letter && (mods & ~MOD_MASK_SHIFT) == 0) {
            return mods ? std::string(1, (char)('A' + (key - KC_A))) : name;
        }
        std::string prefix;
        if (mods & MOD_MASK_CTRL) prefix += "Ctrl+";
        if (mods & MOD_MASK_ALT) prefix += "Alt+";
        if (mods & MOD_MASK_SHIFT) prefix += "Shift+";
        if (mods & MOD_MASK_GUI) prefix += "GUI+";
        return "<" + prefix + name + ">";
    }
};
