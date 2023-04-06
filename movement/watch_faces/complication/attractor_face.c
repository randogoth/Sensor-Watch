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
#include "attractor_face.h"

#if __EMSCRIPTEN__
#include <time.h>
#else
#include "saml22j18a.h"
#endif

static void _get_true_entropy(uint32_t* buffer, size_t num_words) {
    #if !__EMSCRIPTEN__
    hri_mclk_set_APBCMASK_TRNG_bit(MCLK);
    hri_trng_set_CTRLA_ENABLE_bit(TRNG);
    #endif

    for (size_t i = 0; i < num_words; i++) {
        #if __EMSCRIPTEN__
        buffer[i] = rand() % INT32_MAX;
        #else
        while (!hri_trng_get_INTFLAG_reg(TRNG, TRNG_INTFLAG_DATARDY)); // Wait for TRNG data to be ready
        buffer[i] = hri_trng_read_DATA_reg(TRNG); // Read a single 32-bit word from TRNG and store it in the buffer
        #endif
        printf("%d%% of entropy acquired\n", (i+1)*32*100/(num_words*32)); // Print the percentage of total requested bits acquired
    }

    #if !__EMSCRIPTEN__
    hri_trng_clear_CTRLA_ENABLE_bit(TRNG);
    hri_mclk_clear_APBCMASK_TRNG_bit(MCLK);
    #endif
}

static int ZhabaAmp(int* numbers, int max) {
    int input[max];
    memset(input, 0, max * sizeof(int)); // initialize the input array to 0

    for (int i = 0; i < max; i++) {
        if (numbers[i] >= max) return -1; // check for out-of-range values
        input[numbers[i]]++;
    }

    int left = 0;
    int right = max - 1;
    int center = max / 2;
    int cou = 0;

    while ((right - left) > 1) {
        int sum = 0;
        cou = 0;

        // calculate the sum and count of values in the range [left, right]
        for (int i = left; i <= right; i++) {
            sum += input[i] * i;
            cou += input[i];
        }

        if (cou == 0) return left + (right - left) / 2; // handle the case when cou is 0

        center = sum / cou; // calculate the center using integer division
        int delta = center - (max / 2);

        // update the left and right indices based on delta
        if (delta > 0) {
            left = center - (cou - input[right]) / 2;
            right = center + cou / 2;
        } else if (delta < 0) {
            left = center - cou / 2;
            right = center + (cou - input[left]) / 2;
        } else {
            if (left < center) left++;
            if (right > center) right--;
        }
    }

    return center;
}


static double find_densest_cluster(int arr[], int n, int cluster_size, double cluster[], int *center_index) {
    double mean = 0.0;
    double stddev = 0.0;

    // Calculate mean
    for (int i = 0; i < n; i++) {
        mean += arr[i];
    }
    mean /= n;

    // Calculate standard deviation
    for (int i = 0; i < n; i++) {
        stddev += (arr[i] - mean) * (arr[i] - mean);
    }
    stddev = sqrt(stddev / (n - 1));

    double max_density = 0.0;
    int max_density_index = 0;
    for (int i = 0; i < n; i++) {
        double density = 0.0;
        for (int j = 0; j < n; j++) {
            if (i == j) {
                continue;
            }
            int diff = abs(arr[i] - arr[j]);
            if (diff <= cluster_size) {
                density++;
            }
        }

        // Calculate z-score
        double z_score = (density - mean) / stddev;
        cluster[i] = z_score;

        if (z_score > max_density) {
            max_density = z_score;
            max_density_index = i;
        }
    }
    *center_index = max_density_index;
    return max_density;
}


void attractor_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(attractor_state_t));
        memset(*context_ptr, 0, sizeof(attractor_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void attractor_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    attractor_state_t *state = (attractor_state_t *)context;

    // Handle any tasks related to your watch face coming on screen.
}

bool attractor_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    attractor_state_t *state = (attractor_state_t *)context;

    uint32_t numbers[512];
    double cluster[512];
    double zscore;
    int attractor;
    char buf[11];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_string("attrct", 4);
            break;
        case EVENT_TICK:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // You can use the Light button for your own purposes. Note that by default, Movement will also
            // illuminate the LED in response to EVENT_LIGHT_BUTTON_DOWN; to suppress that behavior, add an
            // empty case for EVENT_LIGHT_BUTTON_DOWN.
            break;
        case EVENT_ALARM_BUTTON_UP:
            _get_true_entropy(numbers, 512/32);
            find_densest_cluster(numbers, UINT32_MAX, 20, cluster, &attractor);
            //attractor = ZhabaAmp(numbers, UINT32_MAX);
            sprintf(buf, "%d", numbers[attractor]);
            watch_display_string(buf, 0);
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

void attractor_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}

