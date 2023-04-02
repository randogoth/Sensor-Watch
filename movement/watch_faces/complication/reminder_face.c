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

const uint8_t minutes[] = { 5, 10, 15, 20, 30, 40, 45, 50, 60, 70, 80, 90 }; // mins
const char the_hour[2][7] = { " sharp", "on min" };
const char the_day[4][7] = { "mornin", "  noon", "afnoon", " on t" };
const char the_week[7][7] = { "   sun", "   mon", "   tue", "   wed", "   thu", "   fri", "   sat",  };
const char the_month[4][7] = { "   1st", "  on d", "  15th", "  last"  };
const char frame[4][3] = { "in", "on", "ev", "ea"};

static void remind_me(reminder_state_t *state) {
    char buf[11];
    watch_clear_display();
    switch ( state->set ) {
        case 0: // how often
            switch ( state->how_often ) {
                case REMINDER_IN:
                    sprintf(buf, "        in");
                    break;
                case REMINDER_ON:
                    sprintf(buf, "        on");
                    break;
                case REMINDER_EVERY:
                    sprintf(buf, "     every");
                    break;
                case REMINDER_EACH:
                    sprintf(buf, "      each");
                    break;
            }
            break;
        case 1: // when?
            if ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) {
                switch ( state->when ) {
                    case REMINDER_MINUTES:
                        sprintf(buf, "%c%c  %02d min", frame[state->how_often][0], frame[state->how_often][1], minutes[(state->units = state->units % 12)]);
                        break;
                    case REMINDER_HOURS:
                        sprintf(buf, "%c%c  %02d hrs", frame[state->how_often][0], frame[state->how_often][1], (state->units = (state->units % 24)) + 1);
                        break;
                    case REMINDER_DAYS:
                        sprintf(buf, "%c%c  %02d dys", frame[state->how_often][0], frame[state->how_often][1], (state->units = (state->units % 30)) + 1);
                        break;
                    case REMINDER_WEEKS:
                        sprintf(buf, "%c%c  %02d wks", frame[state->how_often][0], frame[state->how_often][1], (state->units = (state->units % 4)) + 1);
                        break;
                    case REMINDER_MONTHS:
                        sprintf(buf, "%c%c  %02d mth", frame[state->how_often][0], frame[state->how_often][1], (state->units = (state->units % 12)) + 1);
                        break;
                }
            }
            else if ( state->how_often == REMINDER_ON || state->how_often == REMINDER_EACH ) {
                switch ( state->when ) {
                    case 0:
                        sprintf(buf, "%c%c     sun", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                    case 1:
                        sprintf(buf, "%c%c     mon", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                    case 2:
                        sprintf(buf, "%c%c     tue", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                    case 3:
                        sprintf(buf, "%c%c     wed", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                    case 4:
                        sprintf(buf, "%c%c     thu", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                    case 5:
                        sprintf(buf, "%c%c     fri", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                    case 6:
                        sprintf(buf, "%c%c     sat", frame[state->how_often][0], frame[state->how_often][1]);
                        break;
                }
            }
            break;
        case 2:
            if ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) {
                switch ( state->when ) {
                    case REMINDER_HOURS: // sharp, same minute
                        sprintf(buf, "%c%c  %s", frame[state->how_often][0], frame[state->how_often][1], the_hour[(state->subunits = state->subunits % 2)]);
                        break;
                    case REMINDER_DAYS: // morning, noon, afternoon, same time
                        sprintf(buf, "%c%c  %s", frame[state->how_often][0], frame[state->how_often][1], the_day[(state->subunits = state->subunits % 4)]);
                        break;
                    case REMINDER_WEEKS: // sun, mon, tue, wed, thu, fri, sat
                        sprintf(buf, "%c%c  %s", frame[state->how_often][0], frame[state->how_often][1], the_week[(state->subunits = state->subunits % 7)]);
                        break;
                    case REMINDER_MONTHS: // 1st, same day
                        sprintf(buf, "%c%c  %s", frame[state->how_often][0], frame[state->how_often][1], the_month[(state->subunits = state->subunits % 4)]);
                        break;
                    default:
                        state->set = 3;
                        sprintf(buf, "    rmndme");
                        remind_me(state);
                }
            }
            else if ( state->how_often == REMINDER_ON || state->how_often == REMINDER_EACH ) {
                sprintf(buf, "%c%c  %s", frame[state->how_often][0], frame[state->how_often][1], the_day[state->subunits = state->subunits % 4]);
            }
            break;
        case 3:
            sprintf(buf, "    rmndme");
            printf("how: %d, when: %d, units: %d, subunits: %d\n", state->how_often, state->when, state->units, state->subunits);
            state->set = state->how_often = state->when = state->units = state->subunits = 0;
            break;
    }
    watch_display_string(buf, 0);
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
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if ( ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) && state->set == 1 ) {
                state->units = state->units + 1;
                remind_me(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->first = true;
            switch ( state->set ) {
                case 0: // how often
                    state->how_often = (state->how_often + 1) % 4;
                    break;
                case 1: // timeframe
                    state->when = (state->when + 1) % 
                        (state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY ? 5 : 7);
                    break;
                case 2: // units
                    state->subunits = state->subunits + 1;
                    break;
            }
            remind_me(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if ( state->first )
                state->set = (state->set + 1) % 4;
            switch ( state->set ) {
                case 0: // how often
                    state->how_often = 0;
                    break;
                case 1: // timeframe
                    state->when = 0;
                    state->units = 0;
                    break;
                case 2: // units
                    state->subunits = 0;
                    break;
                default:
                    break;
            }
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

