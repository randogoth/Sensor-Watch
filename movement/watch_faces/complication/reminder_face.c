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
#include <math.h>
#include "watch_utility.h"
#include "reminder_face.h"
#if __EMSCRIPTEN__
#include <time.h>
#endif

const uint8_t minutes[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 45, 50, 60, 70, 80, 90 }; // mins
const char the_hour[2][7] = { "on min", " sharp" };
const char the_day[4][7] = { " on t", "mornin", "  noon", "afnoon" };
const char the_week[7][7] = { "on d", "   sun", "   mon", "   tue", "   wed", "   thu", "   fri", "   sat",  };
const char the_month[4][7] = { "  on d", "   1st", "  15th", "  last"  };
const char frame[4][3] = { "in", "on", "ev", "ea"};
const char month[12][3] = { "JA", "FE", "MR", "AP", "M4", "JN", "JL", "AU", "SE", "OC", "NO", "DE" };

static void set_reminder(movement_settings_t *settings, reminder_state_t *state);

static uint8_t get_random(uint8_t num_values) {
    uint8_t random = 0;
    #if __EMSCRIPTEN__
        random = rand() % num_values;
    #else
    random = arc4random_uniform(num_values);
    #endif
    return random;
}

static uint32_t mnemonic_code(uint8_t num_digits) {
    
    uint32_t repeatingDigit = get_random(10);
    uint32_t numRepeatingDigits = num_digits / 2;
    uint32_t numNonRepeatingDigits = num_digits - numRepeatingDigits;
    uint32_t digits[num_digits];

    // Fill the first half of the array with the repeating digit
    for (uint32_t i = 0; i < numRepeatingDigits; i++) {
        digits[i] = repeatingDigit;
    }

    // Fill the second half of the array with random digits
    for (uint32_t i = numRepeatingDigits; i < num_digits; i++) {
        digits[i] = get_random(10);
    }

    // Shuffle the digits using the Fisher-Yates algorithm
    for (uint32_t i = num_digits - 1; i > 0; i--) {
        uint32_t j = get_random(i + 1);
        uint32_t temp = digits[i];
        digits[i] = digits[j];
        digits[j] = temp;
    }

    // Combine the digits into a single integer
    uint32_t result = 0;
    for (uint32_t i = 0; i < num_digits; i++) {
        result = result * 10 + digits[i];
    }
    return result;
}

void reset(reminder_state_t *state) {
    state->set = state->how_often = state->when = state->units = state->subunits = 0;
}

static void remind_me(movement_settings_t *settings, reminder_state_t *state) {

    uint8_t count = 0;
    for (uint8_t i = 0; i < 10; i++) {
        if (state->active[i] == false) {
            count++;
        }
    }

    char buf[11];
    watch_clear_display();
    switch ( state->set ) {
        case 0: // how often
            switch ( state->how_often ) {
                case REMINDER_IN:    sprintf(buf, "  %2d    in", count); break;
                case REMINDER_ON:    sprintf(buf, "  %2d    on", count); break;
                case REMINDER_EVERY: sprintf(buf, "  %2d every", count); break;
                case REMINDER_EACH:  sprintf(buf, "  %2d  each", count); break;
            }
            break;
        case 1: // when?
            if ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) {
                switch ( state->when ) {
                    case REMINDER_MINUTES:
                        sprintf(buf, "%c%c%2d%02d min", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, minutes[(state->units = state->units % 20)]
                        );
                        break;
                    case REMINDER_HOURS:
                        sprintf(buf, "%c%c%2d%02d hrs", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, (state->units = (state->units % 24)) + 1
                        );
                        break;
                    case REMINDER_DAYS:
                        sprintf(buf, "%c%c%2d%02d dys", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, (state->units = (state->units % 30)) + 1
                        );
                        break;
                    case REMINDER_WEEKS:
                        sprintf(buf, "%c%c%2d%02d wks", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, (state->units = (state->units % 4)) + 1
                        );
                        break;
                    case REMINDER_MONTHS:
                        sprintf(buf, "%c%c%2d%02d mth", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, (state->units = (state->units % 12)) + 1
                        );
                        break;
                }
            }
            else if ( state->how_often == REMINDER_ON || state->how_often == REMINDER_EACH ) {
                char day[4];
                switch ( state->when ) {
                    case 0: sprintf(day, "sun"); break;
                    case 1: sprintf(day, "mon"); break;
                    case 2: sprintf(day, "tue"); break;
                    case 3: sprintf(day, "wed"); break;
                    case 4: sprintf(day, "thu"); break;
                    case 5: sprintf(day, "fri"); break;
                    case 6: sprintf(day, "sat"); break;
                }
                sprintf(buf, "%c%c%2d   %3s", frame[state->how_often][0], frame[state->how_often][1], count, day );
            }
            break;
        case 2:
            if ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) {
                switch ( state->when ) {
                    case REMINDER_HOURS: // sharp, same minute
                        sprintf(buf, "%c%c%2d%s", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, the_hour[(state->subunits = state->subunits % 2)]
                        );
                        break;
                    case REMINDER_DAYS: // morning, noon, afternoon, same time
                        sprintf(buf, "%c%c%2d%s", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, the_day[(state->subunits = state->subunits % 4)]
                        );
                        break;
                    case REMINDER_WEEKS: // sun, mon, tue, wed, thu, fri, sat
                        sprintf(buf, "%c%c%2d%s", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, the_week[(state->subunits = state->subunits % 8)]
                        );
                        break;
                    case REMINDER_MONTHS: // 1st, same day
                        sprintf(buf, "%c%c%2d%s", 
                            frame[state->how_often][0], frame[state->how_often][1], 
                            count, the_month[(state->subunits = state->subunits % 4)]
                        );
                        break;
                    default:
                        state->set = 0;
                        sprintf(buf, "Mn    %04d", (state->code = mnemonic_code(4)) );
                        watch_display_string(buf, 0);
                        set_reminder(settings, state);
                        reset(state);
                        return;
                        break;
                }
            }
            else if ( state->how_often == REMINDER_ON || state->how_often == REMINDER_EACH ) {
                sprintf(buf, "%c%c%2d%s", frame[state->how_often][0], frame[state->how_often][1], count, the_day[state->subunits = state->subunits % 4]);
            }
            break;
        case 3: // show mnemonic cipher
            sprintf(buf, "Mn    %04d", (state->code = mnemonic_code(4)) );
            set_reminder(settings, state);
            reset(state);
            break;
        case 4: // settings menu
            switch ( state->units ) {
                case 0: sprintf(buf, "     activ"); break;
                case 1: sprintf(buf, "     times"); break;
                case 2: sprintf(buf, "     reset"); break;
            }
            break;
        case 5: // setup
            if ( state->when < 2 ) {
                if ( state->active[state->index] ) watch_set_indicator(WATCH_INDICATOR_BELL);
                else watch_clear_indicator(WATCH_INDICATOR_BELL);
                if ( state->repeat[state->index][0] > 1 )
                    watch_set_indicator(WATCH_INDICATOR_LAP);
                else watch_clear_indicator(WATCH_INDICATOR_LAP);
                if ( state->when == 0 ) {
                    sprintf(buf, "%-2d%2d%02d%02d%2d",
                        state->reminder[state->index].unit.month,
                        state->reminder[state->index].unit.day,
                        state->reminder[state->index].unit.hour,
                        state->reminder[state->index].unit.minute,
                        state->index + 1
                    );
                }
                if ( state->when == 1 ) // mnemonic code
                    sprintf(buf, "  %2d  %04d", state->index + 1, state->mnemo[state->index]);
            }
            break;
        case 6: // time defaults menu
            switch ( state->units ) {
                case 0: sprintf(buf, "    mornin"); break;
                case 1: sprintf(buf, "    afnoon"); break;
            }
            break;
        case 7:
            sprintf(buf, "      %2d00", state->units == 0 ? state->morning : state->afternoon);
            break;
    }
    watch_display_string(buf, 0);
}

static void set_reminder(movement_settings_t *settings, reminder_state_t *state) {
    for ( uint8_t i; i < 10; i++) {
        if ( state->active[i] == false) {
            state->index = i;
            break;
        }
    }
    state->mnemo[state->index] = state->code;
    state->code = 0;
    state->reminder[state->index].reg = 0; // set it to 0
    int16_t tz = (movement_timezone_offsets[settings->bit.time_zone]) / 60;
    uint8_t weekday; // monday is 1
    watch_date_time now = watch_rtc_get_date_time();
    uint32_t epoch = watch_utility_date_time_to_unix_time(now, tz );
    switch ( state->how_often ) {
        case REMINDER_EVERY:
            state->repeat[state->index][0] = state->how_often;
            state->repeat[state->index][1] = state->when;
            state->repeat[state->index][2] = state->units;
            state->repeat[state->index][3] = state->subunits;
        case REMINDER_IN:
            switch ( state->when ) {
                case REMINDER_MINUTES:
                    state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch + 60 * minutes[state->units], tz);
                    break;
                case REMINDER_HOURS:
                    state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch + 3600 * (state->units + 1), tz);
                    if ( state->subunits > 0 ) { // sharp
                        state->reminder[state->index].unit.minute = 0;
                        state->reminder[state->index].unit.second = 0;
                    }
                    break;
                case REMINDER_DAYS:
                    state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch + 86400 * (state->units + 1), tz);
                    if ( state->subunits > 0 ) {
                        state->reminder[state->index].unit.minute = 0;
                        state->reminder[state->index].unit.second = 0;
                        if ( state->subunits > 1 ) // morning
                            state->reminder[state->index].unit.hour = state->morning;
                        if ( state->subunits > 2 ) // noon
                            state->reminder[state->index].unit.hour = 12;
                        if ( state->subunits > 3 ) // afternoon
                            state->reminder[state->index].unit.hour = state->afternoon;
                    }
                    break;
                case REMINDER_WEEKS:
                    if ( state->subunits > 0 ){
                        weekday = watch_utility_get_iso8601_weekday_number(now.unit.year, now.unit.month, now.unit.day) - 1;
                        epoch += ((state->subunits - 1) - weekday) * 86400;
                    }
                    epoch += 604800 * (state->units + 1);
                    state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch, tz);
                    state->reminder[state->index].unit.hour = state->morning;
                    state->reminder[state->index].unit.minute = 0;
                    state->reminder[state->index].unit.second = 0;
                    break;
                case REMINDER_MONTHS:
                    if ( now.unit.month + (state->units + 1) > 12 )
                        now.unit.year++;
                    now.unit.month = ((now.unit.month + state->units) % 12) + 1;
                    now.unit.hour = state->morning;
                    now.unit.minute = 0;
                    now.unit.second = 0;
                    switch ( state->subunits ) {
                        case 1: now.unit.day =  1; break;   //  1st
                        case 2: now.unit.day = 15; break;   // 15th
                        case 3: switch ( now.unit.month ) { // last
                                    case 2: now.unit.day = 28; break;
                                    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                                        now.unit.day = 31; break;
                                    default: now.unit.day = 30; break;
                                }
                            break;
                    }
                    state->reminder[state->index] = now;
                    break;
            }
            break;
        case REMINDER_EACH:
            state->repeat[state->index][0] = state->how_often;
            state->repeat[state->index][1] = state->when;
            state->repeat[state->index][2] = state->units;
            state->repeat[state->index][3] = state->subunits;
        case REMINDER_ON:
            weekday = watch_utility_get_iso8601_weekday_number(now.unit.year, now.unit.month, now.unit.day) - 1;
            if (weekday != state->when) {
                // Calculate the number of days left until the desired weekday
                while (weekday != state->when) {
                    epoch += 86400;
                    weekday = (weekday + 1) % 7;
                }
            } else {
                epoch += 86400 * 7;
            }
            state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch, tz);
            if ( state->subunits > 0 ) {
                state->reminder[state->index].unit.minute = 0;
                state->reminder[state->index].unit.second = 0;
                if ( state->subunits > 1 ) // morning
                    state->reminder[state->index].unit.hour = state->morning;
                if ( state->subunits > 2 ) // noon
                    state->reminder[state->index].unit.hour = 12;
                if ( state->subunits > 3 ) // afternoon
                    state->reminder[state->index].unit.hour = state->afternoon;
            }
    }
    state->active[state->index] = true;
    printf("%04d | [%d]: %d, %d/%d/%d %02d:%02d:%02d\n", state->mnemo[state->index], state->index,
        watch_utility_get_iso8601_weekday_number( state->reminder[state->index].unit.year, state->reminder[state->index].unit.month, state->reminder[state->index].unit.day),
        state->reminder[state->index].unit.day, state->reminder[state->index].unit.month, state->reminder[state->index].unit.year,
        state->reminder[state->index].unit.hour, state->reminder[state->index].unit.minute, state->reminder[state->index].unit.second
    );
}

void reminder_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(reminder_state_t));
        memset(*context_ptr, 0, sizeof(reminder_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    reminder_state_t *state = (reminder_state_t *)*context_ptr;
    state->code = 0;
    state->morning = 8;
    state->afternoon = 16;
    #if __EMSCRIPTEN__
    srand(time(NULL));
    #endif
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void reminder_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    reminder_state_t *state = (reminder_state_t *)context;
    // Handle any tasks related to your watch face coming on screen.
}

bool reminder_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    reminder_state_t *state = (reminder_state_t *)context;
    char mnemonic[5];
    uint8_t i = 0;
    uint8_t count;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if ( state->code > 0 ) {
                sprintf(mnemonic, "MN    %d", state->code);
                watch_display_string(mnemonic, 0);
            } else watch_display_string("rmndme", 4);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_ALARM_BUTTON_UP:
        case EVENT_LIGHT_BUTTON_UP:
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_LIGHT_LONG_PRESS:
            switch ( state->set ) {
                case 0: // SELECT HOW OFTEN ///////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        state->first = true;
                        state->how_often = (state->how_often + 1) % 4;
                        state->code = 0;
                    } 
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // confirm selection and go to next screen
                        state->set = (state->set + 1) % 4;
                        state->when = 0;
                        state->units = 0;
                    } 
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // enter settings
                        state->set = 4;
                        state->index = 0;
                        state->when = 0;
                    }
                    break;
                case 1: // SELECT THE TIMEFRAME ///////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        state->when = (state->when + 1) % 
                            (state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY ? 5 : 7);
                    } 
                    else
                    if ( event.event_type == EVENT_LIGHT_BUTTON_UP ) {
                        // change the time units
                        if ( ( state->how_often == REMINDER_IN || state->how_often == REMINDER_EVERY) ) {
                                state->units = state->units + 1;
                                remind_me(settings, state);
                            }
                    } 
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // confirm selection and go to next screen
                        state->set = (state->set + 1) % 4;
                        state->subunits = 0;
                    } 
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // cancel selection and exit to main
                        reset(state);
                    }
                    break;
                case 2: // REFINE THE TIMEFRAME ///////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        state->subunits = state->subunits + 1;
                    }
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // confirm selection and go to next screen
                        state->set = (state->set + 1) % 4;
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // cancel selection and exit to main
                        reset(state);
                    }
                    break;
                case 3: // SHOW MNEMONIC CODE /////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // enter settings
                        state->set = 4;
                        state->index = 0;
                        state->when = 0;
                    }
                    break;
                case 4: // SETTINGS MENU //////////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        state->units = (state->units + 1) % 3;
                    }
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // confirm selection and go to next screen
                        // check how many active reminders we have
                        if ( state->units == 0 ) {
                            count = 0;
                            for (i = 0; i < 10; i++)
                                if (state->active[i] == true) count++;
                            // if none, then selecting "active" returns to main
                            if (count == 0) state->set = 0;
                            // otherwise proceed to "active" page
                            else state->set = 5;
                        }
                        else
                        // go to time defaults menu
                        if ( state->units == 1 ) {
                            state->set = 6;
                            state->units = 0;
                        }
                        else
                        // reset all reminders
                        if ( state->units == 2 )
                            for (i = 0; i < 10; i++) {
                                state->active[i] = false;
                                reset(state);
                            }     
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // exit settings
                        state->set = 0;
                    }
                    break;
                case 5: // ACTIVE REMINDERS ///////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the active reminders
                        // if we have zero active reminders, go back to main
                        // ( calculated at LONG ALARM PRESS in menu before)
                        if (count == 0) {
                            state->set = 0;
                        } else {
                            state->index = (state->index + 1) % 10;
                            // skip to active reminders
                            if ( state->active[state->index] == false )
                                do {
                                    state->index = (state->index + 1) % 10;
                                } while ( state->active[state->index] == false );
                        }
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_BUTTON_UP ) {
                        // toggle between showing the reminder time and mnemonic code
                        state->when = (state->when + 1) % 2;
                        remind_me(settings, state);
                    }
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // deactivate an active reminder 
                        state->active[state->index] = !state->active[state->index];
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // exit settings
                        state->set = 0;
                    }
                    break;
                case 6: // TIME DEFAULTS MENU /////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        state->units = (state->units + 1) % 2;
                    }
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // confirm selection and go to next screen
                        state->set = 7;
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // exit settings
                        state->set = 0;
                    }
                    break;
                case 7: // CHANGE TIME DEFAULT ////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        state->units = (state->units + 1) % 12;
                    }
                    break;
            }
            remind_me(settings, state);
            break;
        case EVENT_BACKGROUND_TASK:
            movement_play_alarm();
            sprintf(mnemonic, "MN    %d", state->code);
            watch_display_string(mnemonic, 0);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void reminder_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool reminder_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    reminder_state_t *state = (reminder_state_t *)context;
    watch_date_time now = watch_rtc_get_date_time();
    for (uint8_t i = 0; i < 10; i++) {
        if ( state->active[i]) {
            if ( state->reminder[i].unit.minute == now.unit.minute ) {
                if (state->reminder[i].unit.hour == now.unit.hour) {
                    if (state->reminder[i].unit.day == now.unit.day) {
                        if (state->reminder[i].unit.month == now.unit.month) {
                            if (state->reminder[i].unit.year == now.unit.year) {
                                state->active[i] = false;
                                state->code = state->mnemo[i];
                                if ( state->repeat[i][0] > REMINDER_ON ) { // do we need to repeat it?
                                    state->how_often = state->repeat[i][0];
                                    state->when = state->repeat[i][1];
                                    state->units = state->repeat[i][2];
                                    state->subunits = state->repeat[i][3];
                                    set_reminder(settings, state);
                                    reset(state);
                                }
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}