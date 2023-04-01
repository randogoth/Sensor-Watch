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

#ifndef DUAL_TIMER_FACE_H_
#define DUAL_TIMER_FACE_H_

#include "movement.h"

/*
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
 *
 */

typedef struct {
    uint8_t centiseconds : 7;  // 0-59
    uint8_t seconds : 6;  // 0-59
    uint8_t minutes : 6;  // 0-59
    uint8_t hours : 5;    // 0-23
    uint8_t days : 7;    // 0-99
} dual_timer_duration_t;

typedef struct {
    uint32_t start_ticks[2];
    uint32_t stop_ticks[2];
    bool running[2];
    bool show;
} dual_timer_state_t;

void dual_timer_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void dual_timer_face_activate(movement_settings_t *settings, void *context);
bool dual_timer_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void dual_timer_face_resign(movement_settings_t *settings, void *context);

#define dual_timer_face ((const watch_face_t){ \
    dual_timer_face_setup, \
    dual_timer_face_activate, \
    dual_timer_face_loop, \
    dual_timer_face_resign, \
    NULL, \
})

#endif // DUAL_TIMER_FACE_H_

