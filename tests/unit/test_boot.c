// The bootloader screen in oled.c, compiled from the real file. What a Boot double-tap
// must leave on the panel, and that it is sent at once: reset_keyboard() jumps straight
// after shutdown_user(), so there is no next main loop to send the dirty blocks.
#include <string.h>
#include "unit.h"
#include "qmk_keyboard.h"
#include "driver.h"

uint8_t  fake_oled[OLED_MATRIX_SIZE + 1];
uint16_t fake_dirty;
uint32_t fake_now;
bool     fake_is_left = true;

void oled_write_raw_byte(const char data, uint16_t index) { u_fake_write(data, index); }

// The flush, recorded: how often, whether it was told to send everything, and what was
// dirty when it ran. Sending clears the dirty blocks, as the driver's does.
static int      flushes, flushes_all;
static uint16_t dirty_at_flush;
void oled_render_dirty(bool all) {
    flushes++;
    if (all) flushes_all++;
    dirty_at_flush = fake_dirty;
    fake_dirty     = 0;
}

static bool panel_on = true;
bool is_oled_on(void) { return panel_on; }
bool oled_on(void) { return panel_on = true; }
static bool is_keyboard_master(void) { return true; }
void oled_render_master(void) {}
void oled_render_slave(void) {}

#include "oled.c"

static bool lit(uint8_t x, uint8_t y) {
    return (fake_oled[(y / 8) * OLED_DISPLAY_WIDTH + x] >> (y % 8)) & 1;
}

static int count(void) {
    int n = 0;
    for (uint8_t y = 0; y < 64; y++)
        for (uint8_t x = 0; x < OLED_DISPLAY_WIDTH; x++) n += lit(x, y);
    return n;
}

int main(void) {
    // Something already on the panel, as there always is when Boot is pressed.
    memset(fake_oled, 0x5a, OLED_MATRIX_SIZE);
    fake_dirty = 0;

    check("a soft reset returns true", shutdown_user(false));
    check("and leaves the panel alone", fake_oled[0] == 0x5a && fake_oled[OLED_MATRIX_SIZE - 1] == 0x5a);
    check("and sends nothing", flushes == 0);

    check("the jump to the bootloader returns true", shutdown_user(true));

    bool logo = true;
    const uint8_t lx = (OLED_DISPLAY_WIDTH - U_LOGO_W) / 2;
    for (uint8_t p = 0; p < U_LOGO_PAGES; p++)
        for (uint8_t i = 0; i < U_LOGO_W; i++)
            for (uint8_t k = 0; k < 8; k++)
                if (((u_qmk_logo[p * U_LOGO_W + i] >> k) & 1) != lit((uint8_t)(lx + i), (uint8_t)(U_BOOT_LOGO_Y + p * 8 + k)))
                    logo = false;
    check("the QMK logo is drawn whole, centred, from y=12", logo);

    // The label: a bar from x=22 to 104 and y=42 to 52, with BOOTLOADER cut out of it.
    check("the bar's corners are lit", lit(22, 42) && lit(104, 42) && lit(22, 52) && lit(104, 52));
    check("and it stops there", !lit(21, 47) && !lit(105, 47) && !lit(60, 41) && !lit(60, 53));
    bool cut = true;
    const char *label = "BOOTLOADER";
    for (uint8_t c = 0; c < 10; c++)
        for (uint8_t r = 0; r < 8; r++)
            for (uint8_t k = 0; k < 8; k++)
                if ((u_font8[label[c] - U_FONT_FIRST][r] & (0x80u >> k)) && lit((uint8_t)(25 + c * 8 + k), (uint8_t)(44 + r)))
                    cut = false;
    check("BOOTLOADER is cut out of it, letter by letter", cut);
    check("the old frame is gone everywhere else", !lit(0, 0) && !lit(127, 63) && !lit(64, 2) && !lit(64, 60));

    check("it is sent at once, every dirty block in one go", flushes == 1 && flushes_all == 1);
    check("after the frame was committed", dirty_at_flush != 0);
    check("which leaves nothing waiting", fake_dirty == 0);
    printf("        the screen lights %d px\n", count());

    check("the driver's one-past-the-end byte was never touched", fake_oled[OLED_MATRIX_SIZE] == 0);
    return unit_done("boot");
}
