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

#ifndef RANDONAUT_FACE_H_
#define RANDONAUT_FACE_H_

#include "movement.h"
#include "place_face.h"

/*
 * RANDONAUT FACE
 * ==============
 *
 * and a description of how use it
 *
 */

typedef struct {
    uint8_t mode :3;
    uint8_t location_format :3;
    uint8_t rng: 2;
} randonaut_face_mode_t;

typedef struct {
    int32_t latitude : 25;
    int32_t longitude : 25;
    uint16_t distance : 14;
    uint16_t bearing : 9;
} randonaut_coordinate_t;

typedef struct {
    // Anything you need to keep track of, put it here!
    coordinate_t location;
    randonaut_coordinate_t point;
    uint16_t radius : 14;
    uint32_t entropy;
    bool quantum;
    bool chance;
    bool file;
    randonaut_face_mode_t face;
    char scratchpad[10];
} randonaut_state_t;

void randonaut_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void randonaut_face_activate(movement_settings_t *settings, void *context);
bool randonaut_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void randonaut_face_resign(movement_settings_t *settings, void *context);

#define randonaut_face ((const watch_face_t){ \
    randonaut_face_setup, \
    randonaut_face_activate, \
    randonaut_face_loop, \
    randonaut_face_resign, \
    NULL, \
})

#endif // RANDONAUT_FACE_H_

