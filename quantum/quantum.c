/* Copyright 2016-2017 Jack Humbert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "quantum.h"

#ifdef BLUETOOTH_ENABLE
#    include "outputselect.h"
#endif

#ifdef BACKLIGHT_ENABLE
#    include "backlight.h"
#endif

#ifdef MIDI_ENABLE
#    include "process_midi.h"
#endif

#ifdef VELOCIKEY_ENABLE
#    include "velocikey.h"
#endif

#ifdef HAPTIC_ENABLE
#    include "haptic.h"
#endif

#ifdef AUDIO_ENABLE
#    ifndef GOODBYE_SONG
#        define GOODBYE_SONG SONG(GOODBYE_SOUND)
#    endif
float goodbye_song[][2] = GOODBYE_SONG;
#    ifdef DEFAULT_LAYER_SONGS
float default_layer_songs[][16][2] = DEFAULT_LAYER_SONGS;
#    endif
#endif

uint8_t extract_mod_bits(uint16_t code) {
    switch (code) {
        case QK_MODS ... QK_MODS_MAX:
            break;
        default:
            return 0;
    }

    uint8_t mods_to_send = 0;

    if (code & QK_RMODS_MIN) {  // Right mod flag is set
        if (code & QK_LCTL) mods_to_send |= MOD_BIT(KC_RIGHT_CTRL);
        if (code & QK_LSFT) mods_to_send |= MOD_BIT(KC_RIGHT_SHIFT);
        if (code & QK_LALT) mods_to_send |= MOD_BIT(KC_RIGHT_ALT);
        if (code & QK_LGUI) mods_to_send |= MOD_BIT(KC_RIGHT_GUI);
    } else {
        if (code & QK_LCTL) mods_to_send |= MOD_BIT(KC_LEFT_CTRL);
        if (code & QK_LSFT) mods_to_send |= MOD_BIT(KC_LEFT_SHIFT);
        if (code & QK_LALT) mods_to_send |= MOD_BIT(KC_LEFT_ALT);
        if (code & QK_LGUI) mods_to_send |= MOD_BIT(KC_LEFT_GUI);
    }

    return mods_to_send;
}

void do_code16(uint16_t code, void (*f)(uint8_t)) { f(extract_mod_bits(code)); }

__attribute__((weak)) void register_code16(uint16_t code) {
    if (IS_MOD(code) || code == KC_NO) {
        do_code16(code, register_mods);
    } else {
        do_code16(code, register_weak_mods);
    }
    register_code(code);
}

__attribute__((weak)) void unregister_code16(uint16_t code) {
    unregister_code(code);
    if (IS_MOD(code) || code == KC_NO) {
        do_code16(code, unregister_mods);
    } else {
        do_code16(code, unregister_weak_mods);
    }
}

__attribute__((weak)) void tap_code16(uint16_t code) {
    register_code16(code);
    if (code == KC_CAPS_LOCK) {
        wait_ms(TAP_HOLD_CAPS_DELAY);
    } else if (TAP_CODE_DELAY > 0) {
        wait_ms(TAP_CODE_DELAY);
    }
    unregister_code16(code);
}

__attribute__((weak)) bool process_action_kb(keyrecord_t *record) { return true; }

__attribute__((weak)) bool process_record_kb(uint16_t keycode, keyrecord_t *record) { return process_record_user(keycode, record); }

__attribute__((weak)) bool process_record_user(uint16_t keycode, keyrecord_t *record) { return true; }

__attribute__((weak)) void post_process_record_kb(uint16_t keycode, keyrecord_t *record) { post_process_record_user(keycode, record); }

__attribute__((weak)) void post_process_record_user(uint16_t keycode, keyrecord_t *record) {}

void reset_keyboard(void) {
    clear_keyboard();
#if defined(MIDI_ENABLE) && defined(MIDI_BASIC)
    process_midi_all_notes_off();
#endif
#ifdef AUDIO_ENABLE
#    ifndef NO_MUSIC_MODE
    music_all_notes_off();
#    endif
    uint16_t timer_start = timer_read();
    PLAY_SONG(goodbye_song);
    shutdown_user();
    while (timer_elapsed(timer_start) < 250) wait_ms(1);
    stop_all_notes();
#else
    shutdown_user();
    wait_ms(250);
#endif
#ifdef HAPTIC_ENABLE
    haptic_shutdown();
#endif
    bootloader_jump();
}

/* Convert record into usable keycode via the contained event. */
uint16_t get_record_keycode(keyrecord_t *record, bool update_layer_cache) {
#ifdef COMBO_ENABLE
    if (record->keycode) {
        return record->keycode;
    }
#endif
    return get_event_keycode(record->event, update_layer_cache);
}

/* Convert event into usable keycode. Checks the layer cache to ensure that it
 * retains the correct keycode after a layer change, if the key is still pressed.
 * "update_layer_cache" is to ensure that it only updates the layer cache when
 * appropriate, otherwise, it will update it and cause layer tap (and other keys)
 * from triggering properly.
 */
uint16_t get_event_keycode(keyevent_t event, bool update_layer_cache) {
#if !defined(NO_ACTION_LAYER) && !defined(STRICT_LAYER_RELEASE)
    /* TODO: Use store_or_get_action() or a similar function. */
    if (!disable_action_cache) {
        uint8_t layer;

        if (event.pressed && update_layer_cache) {
            layer = layer_switch_get_layer(event.key);
            update_source_layers_cache(event.key, layer);
        } else {
            layer = read_source_layers_cache(event.key);
        }
        return keymap_key_to_keycode(layer, event.key);
    } else
#endif
        return keymap_key_to_keycode(layer_switch_get_layer(event.key), event.key);
}

/* Get keycode, and then process pre tapping functionality */
bool pre_process_record_quantum(keyrecord_t *record) {
    if (!(
#ifdef COMBO_ENABLE
            process_combo(get_record_keycode(record, true), record) &&
#endif
            true)) {
        return false;
    }
    return true;  // continue processing
}

/* Get keycode, and then call keyboard function */
void post_process_record_quantum(keyrecord_t *record) {
    uint16_t keycode = get_record_keycode(record, false);
    post_process_record_kb(keycode, record);
}

/* Core keycode function, hands off handling to other functions,
    then processes internal quantum keycodes, and then processes
    ACTIONs.                                                      */
bool process_record_quantum(keyrecord_t *record) {
    uint16_t keycode = get_record_keycode(record, true);

    // This is how you use actions here
    // if (keycode == KC_LEAD) {
    //   action_t action;
    //   action.code = ACTION_DEFAULT_LAYER_SET(0);
    //   process_action(record, action);
    //   return false;
    // }

#ifdef VELOCIKEY_ENABLE
    if (velocikey_enabled() && record->event.pressed) {
        velocikey_accelerate();
    }
#endif

#ifdef WPM_ENABLE
    if (record->event.pressed) {
        update_wpm(keycode);
    }
#endif

#ifdef TAP_DANCE_ENABLE
    preprocess_tap_dance(keycode, record);
#endif

    if (!(
#if defined(KEY_LOCK_ENABLE)
            // Must run first to be able to mask key_up events.
            process_key_lock(&keycode, record) &&
#endif
#if defined(DYNAMIC_MACRO_ENABLE) && !defined(DYNAMIC_MACRO_USER_CALL)
            // Must run asap to ensure all keypresses are recorded.
            process_dynamic_macro(keycode, record) &&
#endif
#if defined(AUDIO_ENABLE) && defined(AUDIO_CLICKY)
            process_clicky(keycode, record) &&
#endif
#ifdef HAPTIC_ENABLE
            process_haptic(keycode, record) &&
#endif
#if defined(VIA_ENABLE)
            process_record_via(keycode, record) &&
#endif
            process_record_kb(keycode, record) &&
#if defined(SEQUENCER_ENABLE)
            process_sequencer(keycode, record) &&
#endif
#if defined(MIDI_ENABLE) && defined(MIDI_ADVANCED)
            process_midi(keycode, record) &&
#endif
#ifdef AUDIO_ENABLE
            process_audio(keycode, record) &&
#endif
#if defined(BACKLIGHT_ENABLE) || defined(LED_MATRIX_ENABLE)
            process_backlight(keycode, record) &&
#endif
#ifdef STENO_ENABLE
            process_steno(keycode, record) &&
#endif
#if (defined(AUDIO_ENABLE) || (defined(MIDI_ENABLE) && defined(MIDI_BASIC))) && !defined(NO_MUSIC_MODE)
            process_music(keycode, record) &&
#endif
#ifdef KEY_OVERRIDE_ENABLE
            process_key_override(keycode, record) &&
#endif
#ifdef TAP_DANCE_ENABLE
            process_tap_dance(keycode, record) &&
#endif
#if defined(UNICODE_COMMON_ENABLE)
            process_unicode_common(keycode, record) &&
#endif
#ifdef LEADER_ENABLE
            process_leader(keycode, record) &&
#endif
#ifdef PRINTING_ENABLE
            process_printer(keycode, record) &&
#endif
#ifdef AUTO_SHIFT_ENABLE
            process_auto_shift(keycode, record) &&
#endif
#ifdef DYNAMIC_TAPPING_TERM_ENABLE
            process_dynamic_tapping_term(keycode, record) &&
#endif
#ifdef TERMINAL_ENABLE
            process_terminal(keycode, record) &&
#endif
#ifdef SPACE_CADET_ENABLE
            process_space_cadet(keycode, record) &&
#endif
#ifdef MAGIC_KEYCODE_ENABLE
            process_magic(keycode, record) &&
#endif
#ifdef GRAVE_ESC_ENABLE
            process_grave_esc(keycode, record) &&
#endif
#if defined(RGBLIGHT_ENABLE) || defined(RGB_MATRIX_ENABLE)
            process_rgb(keycode, record) &&
#endif
#ifdef JOYSTICK_ENABLE
            process_joystick(keycode, record) &&
#endif
#ifdef PROGRAMMABLE_BUTTON_ENABLE
            process_programmable_button(keycode, record) &&
#endif
            true)) {
        return false;
    }

    if (record->event.pressed) {
        switch (keycode) {
#ifndef NO_RESET
            case QK_BOOTLOADER:
                reset_keyboard();
                return false;
#endif
#ifndef NO_DEBUG
            case QK_DEBUG_TOGGLE:
                debug_enable ^= 1;
                if (debug_enable) {
                    print("DEBUG: enabled.\n");
                } else {
                    print("DEBUG: disabled.\n");
                }
#endif
                return false;
            case QK_CLEAR_EEPROM:
                eeconfig_init();
                return false;
#ifdef VELOCIKEY_ENABLE
            case VLK_TOG:
                velocikey_toggle();
                return false;
#endif
#ifdef BLUETOOTH_ENABLE
            case OUT_AUTO:
                set_output(OUTPUT_AUTO);
                return false;
            case OUT_USB:
                set_output(OUTPUT_USB);
                return false;
            case OUT_BT:
                set_output(OUTPUT_BLUETOOTH);
                return false;
#endif
#ifndef NO_ACTION_ONESHOT
            case ONESHOT_TOGGLE:
                oneshot_toggle();
                break;
            case ONESHOT_ENABLE:
                oneshot_enable();
                break;
            case ONESHOT_DISABLE:
                oneshot_disable();
                break;
#endif
        }
    }

    return process_action_kb(record);
}

void set_single_persistent_default_layer(uint8_t default_layer) {
#if defined(AUDIO_ENABLE) && defined(DEFAULT_LAYER_SONGS)
    PLAY_SONG(default_layer_songs[default_layer]);
#endif
    eeconfig_update_default_layer((layer_state_t)1 << default_layer);
    default_layer_set((layer_state_t)1 << default_layer);
}

layer_state_t update_tri_layer_state(layer_state_t state, uint8_t layer1, uint8_t layer2, uint8_t layer3) {
    layer_state_t mask12 = ((layer_state_t)1 << layer1) | ((layer_state_t)1 << layer2);
    layer_state_t mask3  = (layer_state_t)1 << layer3;
    return (state & mask12) == mask12 ? (state | mask3) : (state & ~mask3);
}

void update_tri_layer(uint8_t layer1, uint8_t layer2, uint8_t layer3) { layer_state_set(update_tri_layer_state(layer_state, layer1, layer2, layer3)); }

// TODO: remove legacy api
void matrix_init_quantum() { matrix_init_kb(); }
void matrix_scan_quantum() { matrix_scan_kb(); }

//------------------------------------------------------------------------------
// Override these functions in your keymap file to play different tunes on
// different events such as startup and bootloader jump

__attribute__((weak)) void startup_user() {}

__attribute__((weak)) void shutdown_user() {}

/** \brief Run keyboard level Power down
 *
 * FIXME: needs doc
 */
__attribute__((weak)) void suspend_power_down_user(void) {}
/** \brief Run keyboard level Power down
 *
 * FIXME: needs doc
 */
__attribute__((weak)) void suspend_power_down_kb(void) { suspend_power_down_user(); }

void suspend_power_down_quantum(void) {
#ifndef NO_SUSPEND_POWER_DOWN
// Turn off backlight
#    ifdef BACKLIGHT_ENABLE
    backlight_set(0);
#    endif

#    ifdef LED_MATRIX_ENABLE
    led_matrix_task();
#    endif
#    ifdef RGB_MATRIX_ENABLE
    rgb_matrix_task();
#    endif

    // Turn off LED indicators
    led_suspend();

// Turn off audio
#    ifdef AUDIO_ENABLE
    stop_all_notes();
#    endif

// Turn off underglow
#    if defined(RGBLIGHT_SLEEP) && defined(RGBLIGHT_ENABLE)
    rgblight_suspend();
#    endif

#    if defined(LED_MATRIX_ENABLE)
    led_matrix_set_suspend_state(true);
#    endif
#    if defined(RGB_MATRIX_ENABLE)
    rgb_matrix_set_suspend_state(true);
#    endif

#    ifdef OLED_ENABLE
    oled_off();
#    endif
#    ifdef ST7565_ENABLE
    st7565_off();
#    endif
#    if defined(POINTING_DEVICE_ENABLE)
    // run to ensure scanning occurs while suspended
    pointing_device_task();
#    endif
#endif
}

/** \brief run user level code immediately after wakeup
 *
 * FIXME: needs doc
 */
__attribute__((weak)) void suspend_wakeup_init_user(void) {}

/** \brief run keyboard level code immediately after wakeup
 *
 * FIXME: needs doc
 */
__attribute__((weak)) void suspend_wakeup_init_kb(void) { suspend_wakeup_init_user(); }

__attribute__((weak)) void suspend_wakeup_init_quantum(void) {
// Turn on backlight
#ifdef BACKLIGHT_ENABLE
    backlight_init();
#endif

    // Restore LED indicators
    led_wakeup();

// Wake up underglow
#if defined(RGBLIGHT_SLEEP) && defined(RGBLIGHT_ENABLE)
    rgblight_wakeup();
#endif

#if defined(LED_MATRIX_ENABLE)
    led_matrix_set_suspend_state(false);
#endif
#if defined(RGB_MATRIX_ENABLE)
    rgb_matrix_set_suspend_state(false);
#endif
    suspend_wakeup_init_kb();
}

/** \brief converts unsigned integers into char arrays
 *
 * Takes an unsigned integer and converts that value into an equivalent char array
 * A padding character may be specified, ' ' for leading spaces, '0' for leading zeros.
 */

const char *get_numeric_str(char *buf, size_t buf_len, uint32_t curr_num, char curr_pad) {
    buf[buf_len - 1] = '\0';
    for (size_t i = 0; i < buf_len - 1; ++i) {
        char c               = '0' + curr_num % 10;
        buf[buf_len - 2 - i] = (c == '0' && i == 0) ? '0' : (curr_num > 0 ? c : curr_pad);
        curr_num /= 10;
    }
    return buf;
}

/** \brief converts uint8_t into char array
 *
 * Takes an uint8_t, and uses an internal static buffer to render that value into a char array
 * A padding character may be specified, ' ' for leading spaces, '0' for leading zeros.
 *
 * NOTE: Subsequent invocations will reuse the same static buffer and overwrite the previous
 *       contents. Use the result immediately, instead of caching it.
 */
const char *get_u8_str(uint8_t curr_num, char curr_pad) {
    static char    buf[4]   = {0};
    static uint8_t last_num = 0xFF;
    static char    last_pad = '\0';
    if (last_num == curr_num && last_pad == curr_pad) {
        return buf;
    }
    last_num = curr_num;
    last_pad = curr_pad;
    return get_numeric_str(buf, sizeof(buf), curr_num, curr_pad);
}

/** \brief converts uint16_t into char array
 *
 * Takes an uint16_t, and uses an internal static buffer to render that value into a char array
 * A padding character may be specified, ' ' for leading spaces, '0' for leading zeros.
 *
 * NOTE: Subsequent invocations will reuse the same static buffer and overwrite the previous
 *       contents. Use the result immediately, instead of caching it.
 */
const char *get_u16_str(uint16_t curr_num, char curr_pad) {
    static char     buf[6]   = {0};
    static uint16_t last_num = 0xFF;
    static char     last_pad = '\0';
    if (last_num == curr_num && last_pad == curr_pad) {
        return buf;
    }
    last_num = curr_num;
    last_pad = curr_pad;
    return get_numeric_str(buf, sizeof(buf), curr_num, curr_pad);
}
