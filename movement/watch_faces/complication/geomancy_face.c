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
#include "divinate_face.h"
#include "geomancy_face.h"

static const uint64_t geomantic = 0x4ABF39D25E76C180;
static const uint32_t bagua = 0b00000101001110010111011100000000;

static const uint8_t wen_order[] = {
     1, 22,  7, 19, 15, 34, 44, 11, 
    14, 51, 38, 52, 61, 55, 30, 32, 
     6,  3, 28, 58, 39, 63, 46,  5, 
    45, 17, 47, 56, 31, 49, 27, 43, 
    23, 26,  2, 41, 50, 20, 16, 24, 
    35, 21, 62, 36, 54, 29, 48, 12, 
    18, 40, 59, 60, 53, 37, 57,  9, 
    10, 25,  4,  8, 33, 13, 42,  0
};

static const char figures[16][2] = {
    "VI", "Hd", "PA", "GF", 
    "PR", "AQ", "CA", "TR",
    "Td", "CO", "AM", "AL",
    "LF", "RU", "LA", "PO"
};

static void geomancy_face_display();
static nibble_t _geomancy_pick_figure();
static tribble_t _iching_pick_trigram();
static uint8_t _iching_form_hexagram();
static void _geomancy_display(nibble_t code);
static void _display_hexagram(uint8_t hexagram, char* str);
static void _fix_broken_line(uint8_t hexagram);

void geomancy_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(geomancy_state_t));
        memset(*context_ptr, 0, sizeof(geomancy_state_t));
    }
}

void geomancy_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool geomancy_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    geomancy_state_t *state = (geomancy_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_string("gm  IChing", 0);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if ( state->mode <= 1 ) state->mode = 2;
            else if ( state->mode >= 2 ) state->mode = 0;
            geomancy_face_display(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            switch ( state->mode ) {
                case 0:
                    state->mode++;
                case 1:
                    state->i_ching_hexagram = _iching_form_hexagram();
                    break;
                case 2:
                    state->mode++;
                case 3:
                    state->geomantic_figure = _geomancy_pick_figure().bits;
                    break;
                default:
                    break;
            }
            geomancy_face_display(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }
    return true;
}

void geomancy_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

static void geomancy_face_display(geomancy_state_t *state) {
    char token[7] = {0};
    nibble_t figure = *((nibble_t*) &state->geomantic_figure);
    watch_clear_display();
    switch ( state->mode ) {
        case 0:
            watch_display_string("gm  IChing", 0);
            break;
        case 1:
            _display_hexagram(state->i_ching_hexagram, token);
            watch_display_string(token, 4);
            _fix_broken_line(state->i_ching_hexagram);
            sprintf(token, "%2d", wen_order[state->i_ching_hexagram] + 1);
            watch_display_string(token, 2);
            break;
        case 2:
            watch_display_string("gm  GeomCy", 0);
            break;
        case 3:
            sprintf(token, "%c%c", figures[state->geomantic_figure][0], figures[state->geomantic_figure][1]);
            watch_display_string(token, 0);
            _geomancy_display(figure);
            break;
        default:
            break;
    }
}

static nibble_t _geomancy_pick_figure() {
    uint8_t index = (divine_bit() << 3) | (divine_bit() << 2) | (divine_bit() << 1) | divine_bit();
    nibble_t figure = {(geomantic >> (4 * (15 - index))) & 0xF};
    return figure;
}

static tribble_t _iching_pick_trigram() {
    uint8_t index = (divine_bit() << 2) | (divine_bit() << 1) | divine_bit();
    tribble_t trigram = {(bagua >> (3 * index)) & 0b111};
    printf("trigram: %d\n", trigram.bits);
    return trigram;
}

static uint8_t _iching_form_hexagram() {
    tribble_t inner = _iching_pick_trigram();
    tribble_t outer = _iching_pick_trigram();
    uint8_t hexagram = (inner.bits << 3) | outer.bits;
    return hexagram;
}

static void _geomancy_display(nibble_t code) {
    // draw geomantic figures
    bool row1 = (code.bits >> 3) & 1;
    bool row2 = (code.bits >> 2) & 1;
    bool row3 = (code.bits >> 1) & 1;
    bool row4 = code.bits & 1;

    if ( row1 ) watch_set_pixel(1, 18); else watch_set_pixel(1, 19);
    if ( row2 ) { watch_set_pixel(2, 20); watch_set_pixel(0, 21);} else watch_set_pixel(1, 20);
    if ( row3 ) watch_set_pixel(0, 22); else watch_set_pixel(1, 23);
    if ( row4 ) { watch_set_pixel(2, 1); watch_set_pixel(0, 0);} else watch_set_pixel(1, 1);
}

static void _display_hexagram(uint8_t hexagram, char* str) {
    str[6] = '\0';  // Null-terminate the string
    for (uint8_t i = 0; i < 6; i++) {
        if (hexagram & (1 << (5 - i))) {
            str[i] = '1';
        } else {
            str[i] = '=';
        }
    }
}

static void _fix_broken_line(uint8_t hexagram) {
    for (uint8_t i = 0; i < 6; i++) {
        if (!(hexagram & (1 << (5 - i)))) {
            if ( i == 1 ) watch_set_pixel(2, 20);
            if ( i == 3 ) watch_set_pixel(2, 1);
            if ( i == 4 ) watch_set_pixel(2, 2);
            if ( i == 5 ) watch_set_pixel(2, 4);
        }
    }
}