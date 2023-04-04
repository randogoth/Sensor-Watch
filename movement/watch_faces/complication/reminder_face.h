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

#ifndef REMINDER_FACE_H_
#define REMINDER_FACE_H_

#include "movement.h"

/*
 * REMINDER
 * ========
 *
 * Set one-time or recurring reminders in an intuitive and quick way.
 * 
 * You can set reminders that beep "in" a defined amount of time: in 25 minutes, in 3 days, 
 * in 6 months, etc.
 * 
 * You can also set reminders "on" a certain day in the next week: on Monday, on Wednesday, 
 * on Saturday, etc.
 * 
 * If you set a reminder on a certain day or in a couple of days you can refine the time to beep 
 * at the same time on that day or in the morning, at noon, or in the afternoon.
 * 
 * If the reminder lies in a couple of weeks, you can refine it to be on a certain weekday instead
 * of the same day as today.
 * 
 * If it is in a couple of months, you can refine it to beep on the 1st day of the month, on the 
 * 15th or on the last day of the month or keep it at the same day of the month as today.
 * 
 * Reminders that are so far ahead in the future as weeks or months will always beep in the morning 
 * on the selected day.
 * 
 * You can set recurring reminders "every" couple of minutes, hours, days, weeks, and months with the 
 * same refinement described above.
 * 
 * It also works with weekly events on "each" of the weekdays.
 * 
 * USAGE
 * =====
 * 
 * - Use ALARM to choose between IN, ON, EVERY, and EACH.
 * - Confirm with a LONG PRESS of ALARM.
 * - If you chose IN or EVERY then on the next screen you can use ALARM to toggle between minutes, 
 *   hours, days, weeks, and months.
 * - Here use the LIGHT button to increment the number of minutes, or days or the other time units, 
 *   confirm with LONG PRESS ALARM.
 * - On the next screen refine the timeframe if you chose days, weeks, or months. Confirm with 
 *   LONG PRESS ALARM.
 * - If you want to cancel the reminder setting at any stage you can LONG PRESS LIGHT to exit.
 * - At the end of the three steps you get a four digit mnemonic code that you can associate with the 
 *   reason you set your reminder. This same code will appear when you enter the watch face when the 
 *   reminder will fire. So if you hear an alarm, go to the Reminder watchface and see the code and 
 *   use it to remember what the reminder was all about.
 * 
 * SETUP
 * =====
 * - If you are at the first step where you can select between IN, ON,
 *   EVERY, and EACH and you LONG PRESS LIGHT you enter the SETUP menu.
 * - Use ALARM to flip through the options, LONG PRESS ALARM to select it.
 *   - ACTIVE REMINDERS shows you all currently active reminders. 
 *     - The first digit at the top left shows you the month, the one on the right the day.
 *     - The first four large digits show you the hour and minute for this reminder.
 *     - The last smaller digits indicate the index of the 10 available reminder slots
 *     - Use ALARM to flip through all active reminders
 *     - Use LIGHT to toggle to seeing the associated mnemonic code instead
 *     - LONG PRESS ALARM to deactivate the active reminder (BELL ICON shows status)
 *     - LONG PRESS LIGHT to exit to the SETUP menu
 *   - DEFAULT TIMES lets you set the times for "morning" and "afternoon"
 *     - By default they are set to 8:00 and 16:00 respectively
 *     - Use ALARM to toggle between the "morning" and "afternoon"
 *     - Use LONG PRESS ALARM to select it
 *     - On the next screen you can cycle the hour for the moment you selected
 *     - Use LONG PRESS ALARM to switch back to the selection
 *     - Use LONG PRESS LIGHT to exit to the SETUP menu
 *   - RESET ALL REMINDERS
 *     - Use with caution, this deactivates and removes all active reminders 
 *
 */

typedef enum {
    REMINDER_IN,
    REMINDER_ON,
    REMINDER_EVERY,
    REMINDER_EACH
} reminder_how_often;

typedef enum {
    REMINDER_MINUTES,
    REMINDER_HOURS,
    REMINDER_DAYS,
    REMINDER_WEEKS,
    REMINDER_MONTHS
} reminder_when;

typedef struct {
    watch_date_time reminder[10];
    bool active[10];
    uint16_t code : 14;
    uint16_t mnemo[10];
    uint8_t repeat[10][4];
    uint8_t morning : 5;
    uint8_t afternoon : 5;
    uint8_t index : 4;
    uint8_t when : 5;
    uint8_t how_often: 3;
    uint8_t set : 3;
    uint8_t units : 5;
    uint8_t subunits : 5;
} reminder_state_t;

void reminder_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void reminder_face_activate(movement_settings_t *settings, void *context);
bool reminder_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void reminder_face_resign(movement_settings_t *settings, void *context);
bool reminder_face_wants_background_task(movement_settings_t *settings, void *context);

#define reminder_face ((const watch_face_t){ \
    reminder_face_setup, \
    reminder_face_activate, \
    reminder_face_loop, \
    reminder_face_resign, \
    reminder_face_wants_background_task \
})

#endif // REMINDER_FACE_H_

