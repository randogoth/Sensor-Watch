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
#include "zscore_face.h"
#include <math.h>
#if __EMSCRIPTEN__
#include <time.h>
#else
#include "saml22j18a.h"
#endif

static double compute_z_score(uint32_t* nums, size_t size);
static double autocorrelation(uint32_t* bitstream, int n, int lag);
static uint32_t get_true_entropy(void);
static uint32_t entropy(void);

void zscore_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(zscore_state_t));
        memset(*context_ptr, 0, sizeof(zscore_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void zscore_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    zscore_state_t *state = (zscore_state_t *)context;

    // Handle any tasks related to your watch face coming on screen.
}

bool zscore_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    zscore_state_t *state = (zscore_state_t *)context;
    double z_score;
    uint32_t e;
    char sign;
    char buf[11];
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            movement_request_tick_frequency(16);
            break;
        case EVENT_TICK:
            state->index = (state->index + 1) % 500;
            e = entropy();
            state->numbers[state->index] = e;
            if ( state->index == 0 ) {
                z_score += compute_z_score(state->numbers, 500);
                uint8_t integer = (uint8_t)abs(z_score);
                uint16_t decimal = (uint16_t)abs(((z_score - integer) * 10000));
                sprintf(buf, "Z  %c%1d,%04d", z_score < 0 ? '-' : ' ', integer, decimal);
            }
            watch_display_string(buf, 0);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            // Just in case you have need for another button.
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

void zscore_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

#include <math.h>
#include <stdint.h>

/** @brief divination method to derive a bit from 32 TRNG bits
 */
static uint32_t entropy(void) {
    uint32_t entropy;
    do { // modulo bias filter
        entropy = get_true_entropy(); // get 32 TRNG bits as stalks
    } while (entropy >= INT32_MAX);

    return entropy;
}

/** @brief get 32 True Random Number bits
 */
static uint32_t get_true_entropy(void) {
    #if __EMSCRIPTEN__
    return rand() % INT32_MAX;
    #else
    hri_mclk_set_APBCMASK_TRNG_bit(MCLK);
    hri_trng_set_CTRLA_ENABLE_bit(TRNG);

    while (!hri_trng_get_INTFLAG_reg(TRNG, TRNG_INTFLAG_DATARDY)); // Wait for TRNG data to be ready

    hri_trng_clear_CTRLA_ENABLE_bit(TRNG);
    hri_mclk_clear_APBCMASK_TRNG_bit(MCLK);
    return hri_trng_read_DATA_reg(TRNG); // Read a single 32-bit word from TRNG and return it
    #endif
}

static double compute_z_score(uint32_t* nums, size_t size) {
    // Compute the mean and standard deviation of the input numbers
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        sum += nums[i];
    }
    double mean = sum / size;
    double variance = 0.0;
    for (size_t i = 0; i < size; i++) {
        variance += pow(nums[i] - mean, 2);
    }
    variance /= size;
    double std_dev = sqrt(variance);
    // Compute the z-score using the formula: (x - mean) / std_dev
    double z_score = 0.0;
    for (size_t i = 0; i < size; i++) {
        z_score = (nums[i] - mean) / (std_dev / sqrt(size));
    }
    return z_score;
}

static double autocorrelation(uint32_t* bitstream, int n, int lag) {
    double sum = 0.0;
    double mean = 0.0;
    double variance = 0.0;

    // Calculate the mean of the bitstream
    for (int i = 0; i < n; i++) {
        mean += (double) bitstream[i];
    }
    mean /= n;

    // Calculate the variance of the bitstream
    for (int i = 0; i < n; i++) {
        variance += pow((double) bitstream[i] - mean, 2);
    }
    variance /= (n - 1);

    // Calculate the autocorrelation at the given lag
    for (int i = 0; i < n - lag; i++) {
        sum += ((double) bitstream[i] - mean) * ((double) bitstream[i + lag] - mean);
    }

    return sum / ((n - lag) * variance);
}
