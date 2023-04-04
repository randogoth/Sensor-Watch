/*
 * MIT License
 *
 * Copyright (c) 2023 Tobias Raayoni Last / @randogoth
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

// CONSTANTS //////////////////////////////////////////////////////////////////////////////////////

const uint8_t minutes[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 45, 50, 60, 70, 80, 90 }; // mins
const char the_hour[2][7] = { "on min", " sharp" };
const char the_day[4][7] = { " on t", "mornin", "  noon", "afnoon" };
const char the_week[7][7] = { "on d", "   sun", "   mon", "   tue", "   wed", "   thu", "   fri", "   sat",  };
const char the_month[4][7] = { "  on d", "   1st", "  15th", "  last"  };
const char frame[4][3] = { "in", "on", "ev", "ea"};

// STATIC DECLARATIONS ////////////////////////////////////////////////////////////////////////////

static uint8_t get_random(uint8_t num_values);
static uint32_t mnemonic_code(uint8_t num_digits);
static void set_reminder(movement_settings_t *settings, reminder_state_t *state);
void reset(reminder_state_t *state);
static void remind_me(movement_settings_t *settings, reminder_state_t *state);
static void set_reminder(movement_settings_t *settings, reminder_state_t *state);

// PUBLIC WATCH FACE FUNCTIONS ////////////////////////////////////////////////////////////////////

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
                        state->how_often = (state->how_often + 1) % 4;
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
                        state->units = 0;
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
                            if (count == 0) {
                                // exit to settings menu
                                state->set = 4;
                                state->index = 0;
                                state->units = 0;
                                state->subunits = 0;
                            }
                            // otherwise proceed to "active" page
                            else state->set = 5;
                        }
                        else
                        // go to time defaults menu
                        if ( state->units == 1 ) {
                            state->set = 6;
                            state->index = 0;
                            state->units = 0;
                            state->subunits = 0;
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
                        reset(state);
                    }
                    break;
                case 5: // ACTIVE REMINDERS ///////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the active reminders
                        // if we have zero active reminders, go back to main
                        count = 0;
                        for (i = 0; i < 10; i++)
                            if (state->active[i] == true) count++;
                        if (count == 0) {
                            // exit to settings menu
                            state->set = 4;
                            state->index = 0;
                            state->units = 0;
                            state->subunits = 0;
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
                        // exit to settings menu
                        state->set = 4;
                        state->index = 0;
                        state->units = 0;
                        state->subunits = 0;
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
                        state->subunits = 0;
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // exit to settings menu
                        state->set = 4;
                        state->index = 0;
                        state->units = 0;
                        state->subunits = 0;
                    }
                    break;
                case 7: // CHANGE TIME DEFAULT ////////////////////////////////////////////////////
                    if ( event.event_type == EVENT_ALARM_BUTTON_UP ) {
                        // flip through the options
                        if ( state->units == 0 ) // morning
                            state->morning = (state->morning + 1) % 12;
                        else
                        if ( state->units == 1 ) // afternoon
                            state->afternoon = ((state->afternoon + 1) % 12) + 12;
                    }
                    else
                    if ( event.event_type == EVENT_ALARM_LONG_PRESS ) {
                        // confirm selection and go back to settings menu screen
                        state->set = 6;
                        state->index = 0;
                        state->units = 0;
                        state->subunits = 0;
                    }
                    else
                    if ( event.event_type == EVENT_LIGHT_LONG_PRESS ) {
                        // exit to settings menu
                        state->set = 4;
                        state->index = 0;
                        state->units = 0;
                        state->subunits = 0;
                    }
                    break;
            }
            remind_me(settings, state);
            break;
        case EVENT_BACKGROUND_TASK:
            movement_play_alarm();
            sprintf(mnemonic, "MN    %d", state->code);
            watch_display_string(mnemonic, 0);
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

// STATIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////

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
    bool unique = true;

    watch_clear_display();
    switch ( state->set ) {

        case 0: // HOW OFTEN //////////////////////////////////////////////////////////////////////

            switch ( state->how_often ) {
                case REMINDER_IN:    sprintf(buf, "  %2d    in", count); break;
                case REMINDER_ON:    sprintf(buf, "  %2d    on", count); break;
                case REMINDER_EVERY: sprintf(buf, "  %2d every", count); break;
                case REMINDER_EACH:  sprintf(buf, "  %2d  each", count); break;
            }
            break;

        case 1: // TIMEFRAME //////////////////////////////////////////////////////////////////////

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

        case 2: // REFINE TIMEFRAME ///////////////////////////////////////////////////////////////

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
                sprintf(buf, "%c%c%2d%s", 
                    frame[state->how_often][0], frame[state->how_often][1], 
                    count, the_day[state->subunits = state->subunits % 4]
                );
            }
            break;

        case 3: // NEMONIC CIPHER /////////////////////////////////////////////////////////////////

            do {
                state->code = mnemonic_code(4);
                for ( uint8_t i = 0; i < 10; i++ ) {
                    if ( state->mnemo[i] == state->code )
                        unique = false;
                }
            } while ( !unique );
            sprintf(buf, "Mn    %04d", (state->code) );
            set_reminder(settings, state);
            reset(state);
            break;

        case 4: // SETTINGS MENU //////////////////////////////////////////////////////////////////

            switch ( state->units ) {
                case 0: sprintf(buf, "     activ"); break;
                case 1: sprintf(buf, "     times"); break;
                case 2: sprintf(buf, "     reset"); break;
            }
            break;

        case 5: // ACTIVE REMINDERS ///////////////////////////////////////////////////////////////

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

        case 6: // TIME DEFAULTS MENU /////////////////////////////////////////////////////////////

            switch ( state->units ) {
                case 0: sprintf(buf, "    mornin"); break;
                case 1: sprintf(buf, "    afnoon"); break;
            }
            break;

        case 7: // SET TIME DEFAULTS //////////////////////////////////////////////////////////////

            sprintf(buf, "      %2d00", state->units == 0 ? state->morning : state->afternoon);
            break;
    
    }

    watch_display_string(buf, 0);

}

static void set_reminder(movement_settings_t *settings, reminder_state_t *state) {

    // find free reminder slot
    for ( uint8_t i; i < 10; i++) {
        if ( state->active[i] == false) {
            state->index = i;
            break;
        }
    }

    // set mnemonic cipher to free slot
    state->mnemo[state->index] = state->code;
    state->code = 0;
    
    // set slot time to 0
    state->reminder[state->index].reg = 0;

    // we need some variables 
    int16_t tz = (movement_timezone_offsets[settings->bit.time_zone]) / 60;
    uint8_t weekday;

    // get current time
    watch_date_time now = watch_rtc_get_date_time();
    uint32_t epoch = watch_utility_date_time_to_unix_time(now, tz ); // also as timestamp

    // what type of reminder do we set?

    switch ( state->how_often ) {

        case REMINDER_EVERY:
        case REMINDER_EACH:

            // if it is a repeating reminder, tell the repeat manager
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
                    // same minute or at 00 seconds sharp?
                    if ( state->subunits > 0 ) {
                        state->reminder[state->index].unit.minute = 0;
                        state->reminder[state->index].unit.second = 0;
                    }
                    break;

                case REMINDER_DAYS:

                    state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch + 86400 * (state->units + 1), tz);
                    // if not at the same time of the day....
                    if ( state->subunits > 0 ) {
                        state->reminder[state->index].unit.minute = 0;
                        state->reminder[state->index].unit.second = 0;
                        if ( state->subunits == 1 ) // ...in the morning
                            state->reminder[state->index].unit.hour = state->morning;
                        if ( state->subunits == 2 ) // ...at noon
                            state->reminder[state->index].unit.hour = 12;
                        if ( state->subunits == 3 ) // ...in the afternoon
                            state->reminder[state->index].unit.hour = state->afternoon;
                    }
                    break;

                case REMINDER_WEEKS:

                    // if not on the same day of the week...
                    if ( state->subunits > 0 ){
                        // ...then on this weekday...
                        weekday = watch_utility_get_iso8601_weekday_number(now.unit.year, now.unit.month, now.unit.day) - 1;
                        epoch += ((state->subunits - 1) - weekday) * 86400;
                    }
                    epoch += 604800 * (state->units + 1);
                    state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch, tz);
                    // ...in the morning
                    state->reminder[state->index].unit.hour = state->morning;
                    state->reminder[state->index].unit.minute = 0;
                    state->reminder[state->index].unit.second = 0;
                    break;

                case REMINDER_MONTHS:

                    // check if it is next year
                    if ( now.unit.month + (state->units + 1) > 12 )
                        now.unit.year++;
                    
                    now.unit.month = ((now.unit.month + state->units) % 12) + 1;

                    // it will beep in the morning
                    now.unit.hour = state->morning;
                    now.unit.minute = 0;
                    now.unit.second = 0;

                    // on the same day or...
                    switch ( state->subunits ) {
                        case 1: now.unit.day =  1; break;   // ... on the 1st day
                        case 2: now.unit.day = 15; break;   // ...on the 15th
                        case 3: switch ( now.unit.month ) { // ...one last day
                                    case 2: now.unit.day = 28; break; // let's not do leap year shenanigans
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

        case REMINDER_ON:

            // the next matching weekday
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
            // same time or...
            state->reminder[state->index] = watch_utility_date_time_from_unix_time(epoch, tz);
            if ( state->subunits > 0 ) {
                state->reminder[state->index].unit.minute = 0;
                state->reminder[state->index].unit.second = 0;
                if ( state->subunits == 1 ) // ... in the morning
                    state->reminder[state->index].unit.hour = state->morning;
                if ( state->subunits == 2 ) // ... at noon
                    state->reminder[state->index].unit.hour = 12;
                if ( state->subunits == 3 ) // ... in the afternoon
                    state->reminder[state->index].unit.hour = state->afternoon;
            }
    }

    // all set, let's activate it
    state->active[state->index] = true;

    // debugging
    printf("%04d | [%d]: %d, %d/%d/%d %02d:%02d:%02d\n", state->mnemo[state->index], state->index,
        watch_utility_get_iso8601_weekday_number( state->reminder[state->index].unit.year, state->reminder[state->index].unit.month, state->reminder[state->index].unit.day),
        state->reminder[state->index].unit.day, state->reminder[state->index].unit.month, state->reminder[state->index].unit.year,
        state->reminder[state->index].unit.hour, state->reminder[state->index].unit.minute, state->reminder[state->index].unit.second
    );

}