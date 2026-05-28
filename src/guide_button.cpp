#include "guide_button.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "pico/time.h"

void guide_button_tick(uint8_t *interrupt_in_data) {
    static bool ps_was_pressed = false;
    static uint32_t ps_press_time = 0;
    static bool long_press_fired = false;
    static bool key_release_pending = false;
    static uint32_t key_release_time = 0;

    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    bool raw_ps_pressed = (interrupt_in_data[9] & 0x01);
    static uint32_t last_high_time = 0;
    static bool is_ps_pressed = false;

    if (raw_ps_pressed) {
        is_ps_pressed = true;
        last_high_time = current_time;
    } else if (current_time - last_high_time > 50) {
        is_ps_pressed = false;
    }

    if (key_release_pending && (current_time >= key_release_time)) {
        if (tud_hid_n_ready(1)) {
            tud_hid_n_keyboard_report(1, 0, 0, NULL);
            key_release_pending = false;
        }
    }

    if (is_ps_pressed && !ps_was_pressed) {
        ps_press_time = current_time;
        ps_was_pressed = true;
        long_press_fired = false;
    } else if (is_ps_pressed && ps_was_pressed) {
        if (!long_press_fired && (current_time - ps_press_time >= 750)) {
            if (tud_hid_n_ready(1)) {
                uint8_t keycode[6] = {HID_KEY_TAB};
                tud_hid_n_keyboard_report(1, 0, KEYBOARD_MODIFIER_LEFTGUI, keycode);
                long_press_fired = true;
                key_release_pending = true;
                key_release_time = current_time + 30;
            }
        }
    } else if (!is_ps_pressed && ps_was_pressed) {
        if (!long_press_fired) {
            if (tud_hid_n_ready(1)) {
                uint8_t keycode[6] = {HID_KEY_G};
                tud_hid_n_keyboard_report(1, 0, KEYBOARD_MODIFIER_LEFTGUI, keycode);
                key_release_pending = true;
                key_release_time = current_time + 30;
            }
        }
        ps_was_pressed = false;
    }

    interrupt_in_data[9] &= ~0x01;
}