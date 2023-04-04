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

#ifndef REMINDER_FACE_H_
#define REMINDER_FACE_H_

#include "movement.h"

/*
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
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

