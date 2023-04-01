/*
 * MIT License
 *
 * Copyright (c) 2023 <#author_name#>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "reminder_face.h"

const char minutes[] = { 5, 10, 15, 20, 30, 40, 45, 50 }; // mins

static void remind_me(reminder_state_t *state) {
    watch_clear_display();
    switch ( state->set ) {
        case 0: // how often
            switch ( state->how_often ) {
                case REMINDER_IN:
                    watch_display_string("in", 5);
                    break;
                case REMINDER_ON:
                    watch_display_string("on", 5);
                    break;
                case REMINDER_EVERY:
                    watch_display_string("every", 5);
                    break;
                case REMINDER_EACH:
                    watch_display_string("each", 5);
                    break;
            }
            break;
        case 1: // when?
            if ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) {
                state->options = 5;
                switch ( state->when ) {
                    case REMINDER_MINUTES:
                        watch_display_string("mins", 5);
                        break;
                    case REMINDER_HOURS:
                        watch_display_string("hours", 5);
                        break;
                    case REMINDER_DAYS:
                        watch_display_string("days", 5);
                        break;
                    case REMINDER_WEEKS:
                        watch_display_string("weeks", 5);
                        break;
                    case REMINDER_MONTHS:
                        watch_display_string("monts", 5);
                        break;
                }
            }
            else if ( state->how_often == REMINDER_ON || state->how_often == REMINDER_EACH ) {
                state->options = 7;
                switch ( state->when ) {
                    case 0:
                        watch_display_string("  sun", 5);
                        break;
                    case 1:
                        watch_display_string("  mon", 5);
                        break;
                    case 2:
                        watch_display_string("  tue", 5);
                        break;
                    case 3:
                        watch_display_string("  wen", 5);
                        break;
                    case 4:
                        watch_display_string("  thu", 5);
                        break;
                    case 5:
                        watch_display_string("  fri", 5);
                        break;
                    case 6:
                        watch_display_string("  sat", 5);
                        break;
                }
            }
            break;
        case 2: // units
            if ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY ) {
                break;
            }
            else if ( state->how_often == REMINDER_ON || state->how_often == REMINDER_EACH ) {
                break;
            }
    }
}

void reminder_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(reminder_state_t));
        memset(*context_ptr, 0, sizeof(reminder_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void reminder_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    reminder_state_t *state = (reminder_state_t *)context;

    // Handle any tasks related to your watch face coming on screen.
}

bool reminder_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    reminder_state_t *state = (reminder_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_string("rmndme", 4);
            break;
        case EVENT_TICK:
            // If needed, update your display here.
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch ( state->set ) {
                case 0: // how often
                    state->how_often = (state->how_often + 1) % 4;
                    state->when = 0;
                    break;
                case 1: // timeframe
                    state->when = (state->when + 1) % state->options;
                    break;
                case 2: // units
                    state->units = (state->units + 1) % state->options;
            }
            remind_me(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->set = (state->set + 1) % 2;
            remind_me(state);
            break;
        case EVENT_TIMEOUT:
            // Your watch face will receive this event after a period of inactivity. If it makes sense to resign,
            // you may uncomment this line to move back to the first watch face in the list:
            // movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            // If you did not resign in EVENT_TIMEOUT, you can use this event to update the display once a minute.
            // Avoid displaying fast-updating values like seconds, since the display won't update again for 60 seconds.
            // You should also consider starting the tick animation, to show the wearer that this is sleep mode:
            // watch_start_tick_animation(500);
            break;
        default:
            // Movement's default loop handler will step in for any cases you don't handle above:
            // * EVENT_LIGHT_BUTTON_DOWN lights the LED
            // * EVENT_MODE_BUTTON_UP moves to the next watch face in the list
            // * EVENT_MODE_LONG_PRESS returns to the first watch face (or skips to the secondary watch face, if configured)
            // You can override any of these behaviors by adding a case for these events to this switch statement.
            return movement_default_loop_handler(event, settings);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    // Exceptions:
    //  * If you are displaying a color using the low-level watch_set_led_color function, you should return false.
    //  * If you are sounding the buzzer using the low-level watch_set_buzzer_on function, you should return false.
    // Note that if you are driving the LED or buzzer using Movement functions like movement_illuminate_led or
    // movement_play_alarm, you can still return true. This guidance only applies to the low-level watch_ functions.
    return true;
}

void reminder_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

