// The tagged EEPROM word behind oled_settings.h.

#include QMK_KEYBOARD_H
#include "oled.h"
#include "oled_settings.h"

// eeconfig_read_user() gives a zeroed word on a fresh or cleared EEPROM, and zero is a
// legitimate value for both bytes, so the stored word carries a tag. An untagged word
// means "never set" and every byte falls back to the build's default rather than
// silently selecting the first entry. The tag is the one the animation has always been
// stored under, so the flash that added the page kept everyone's animation; a word from
// before then has zero in byte 1, which is the first page.
#define U_SETTINGS_TAG  0x5ea10000u
#define U_SETTINGS_MASK 0xffff0000u

static uint32_t ee_word;
static bool     ee_loaded;

static void ee_load(void) {
    if (ee_loaded) return;
    ee_loaded = true;
    const uint32_t raw = eeconfig_read_user();
    ee_word = (raw & U_SETTINGS_MASK) == U_SETTINGS_TAG ? raw : (U_SETTINGS_TAG | U_ANIM_DEFAULT);
}

uint8_t u_setting_get(enum u_setting which) {
    ee_load();
    return (uint8_t)(ee_word >> (8 * which));
}

// Read-modify-write, always the whole word: a press of one key must never reset the
// other's byte, and a first-ever write must carry the defaults it did not touch.
void u_setting_set(enum u_setting which, uint8_t value) {
    ee_load();
    const uint32_t shift = 8u * which;
    ee_word = (ee_word & ~(0xffu << shift)) | ((uint32_t)value << shift);
    eeconfig_update_user(ee_word);
}

// Called every main loop on the slave, so the comparison is the whole cost there; the
// write happens on a press of ANIM or PAGE, or once when two halves that disagree meet.
void u_settings_follow(uint8_t anim, uint8_t view) {
    ee_load();
    const uint32_t want = U_SETTINGS_TAG | ((uint32_t)view << 8) | anim;
    if ((ee_word & 0xffffu) == (want & 0xffffu)) return;
    ee_word = want;
    eeconfig_update_user(ee_word);
}
