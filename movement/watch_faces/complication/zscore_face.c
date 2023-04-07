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

#define THRESHOLD_ZSCORE 3 // beyond this +- from 0
#define THRESHOLD_AUTOCORRELATION 0.015625 // beyond this +- from 0
#define THRESHOLD_SHANNON 0.97 // below
#define THRESHOLD_RUNS 0.05 // below

static double zscore(uint32_t *bits, size_t n);
static double compute_z_score(uint32_t* nums, size_t size);
static void _get_true_entropy(uint32_t* buffer, size_t num_words, zscore_state_t *state);
static void seed_prng(prng_t* rng, uint64_t seed);
static uint32_t prng(prng_t* rng);
static void remove_bias(uint32_t* nums, size_t size);
static double autocorrelation(uint32_t* nums, size_t size, size_t max_lag);
static double shannon_entropy(uint32_t *bits, size_t n);
static double runs_test(uint32_t *bits, size_t n);

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
    seed_prng(&state->prng, time(NULL));
    state->highest[] = {0};
    state->peak = true;
    // Handle any tasks related to your watch face coming on screen.
}

bool zscore_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    zscore_state_t *state = (zscore_state_t *)context;
    double z_score, correl, shannon, runs;
    uint32_t e;
    char sign;
    char buf[11];
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            movement_request_tick_frequency(8);
            break;
        case EVENT_TICK:
            _get_true_entropy(state->numbers, 512/32, state);
            remove_bias(state->numbers, 512);
            z_score = zscore(state->numbers, 512);
            correl = autocorrelation(state->numbers, 512, 100);
            shannon = shannon_entropy(state->numbers, 512);
            runs = runs_test(state->numbers, 512);
            printf("Z-Score: %.3f\nAutocorrelation: %.3f\nShannon Entropy: %.3f\nRuns: %.3f\n", (double)z_score, correl, shannon, runs);
            if ( 
                z_score >= THRESHOLD_ZSCORE               || 
                z_score <= THRESHOLD_ZSCORE * -1          ||
                correl  >= THRESHOLD_AUTOCORRELATION      || 
                correl  <= THRESHOLD_AUTOCORRELATION * -1 ||
                shannon <= THRESHOLD_SHANNON              ||
                runs    <= THRESHOLD_RUNS
            ) {
                state->index++;
                state->highest[state->index][0] = z_score;
                state->highest[state->index][1] = correl;
                state->highest[state->index][2] = shannon;
                state->highest[state->index][3] = runs;
                state->timestamp[state->index] = watch_rtc_get_date_time();
            }
            uint8_t integer = (uint8_t)abs(z_score);
            uint16_t decimal = (uint16_t)abs(((z_score - integer) * 10000));
            if ( state->ts ) {
                watch_set_colon();
                sprintf(buf, "%-2d%2d%2d%02d%02d", state->timestamp.unit.month, state->timestamp.unit.day, state->timestamp.unit.hour, state->timestamp.unit.minute, state->timestamp.unit.second);
                watch_display_string(buf, 0);
            } else {
                watch_clear_colon();
                sprintf(buf, "   %c%1d,%04d", z_score < 0 ? '-' : ' ', integer, decimal);
                watch_display_string(buf, 0);
                watch_set_pixel(0, 14);
                watch_set_pixel(1, 13);
                watch_set_pixel(1, 14);
                watch_set_pixel(1, 15);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->ts = !state->ts;
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->peak = !state->peak;
            break;
        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;
        default:
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

/** @brief get 32 True Random Number bits
 */
static void _get_true_entropy(uint32_t* buffer, size_t num_words, zscore_state_t *state) {
    #if !__EMSCRIPTEN__
    hri_mclk_set_APBCMASK_TRNG_bit(MCLK);
    hri_trng_set_CTRLA_ENABLE_bit(TRNG);
    #endif

    for (size_t i = 0; i < num_words; i++) {
        #if __EMSCRIPTEN__
        do {
            buffer[i] = prng(&state->prng);
        } while ( buffer[i] == 0 || buffer[i] >= INT32_MAX);
        #else
            while (!hri_trng_get_INTFLAG_reg(TRNG, TRNG_INTFLAG_DATARDY)); // Wait for TRNG data to be ready
            buffer[i] = hri_trng_read_DATA_reg(TRNG); // Read a single 32-bit word from TRNG and store it in the buffer
            printf("%d%% of entropy acquired\n", (i+1)*32*100/(num_words*32)); // Print the percentage of total requested bits acquired
        #endif
    }
    #if !__EMSCRIPTEN__
    hri_trng_clear_CTRLA_ENABLE_bit(TRNG);
    hri_mclk_clear_APBCMASK_TRNG_bit(MCLK);
    #endif
}

static void seed_prng(prng_t* rng, uint64_t seed) {
    rng->state = 0;
    rng->inc = (seed << 1u) | 1u;
    prng(rng);
}

static uint32_t prng(prng_t* rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static void remove_bias(uint32_t* nums, size_t size) {
     // Seed the PRNG
    prng_t rng;
    seed_prng(&rng, time(NULL));

    // Find the number of 1's and 0's
    uint32_t num_zeros = 0;
    uint32_t num_ones = 0;

    for (size_t i = 0; i < size; i++) {
        uint32_t rnd = prng(&rng);
        nums[i] ^= rnd;

        for (size_t j = 0; j < 32; j++) {
            if (nums[i] & (1 << j)) {
                num_ones++;
            } else {
                num_zeros++;
            }
        }
    }

    // Compute the bias correction factor
    double bias_factor = ((double)num_zeros / num_ones);
    if (bias_factor > 1.0) {
        if (num_zeros > num_ones) {
            bias_factor = (double)num_ones / num_zeros;
        } else {
            bias_factor = (double)num_zeros / num_ones;
    }

    }

    // Apply the bias correction factor
    for (size_t i = 0; i < size; i++) {
        nums[i] = ((double)nums[i] * bias_factor) + 0.5;
    }
}

static double zscore(uint32_t *bits, size_t n) {
    double sum = 0.0;
    double mean, variance, stddev;
    int i;

    // Calculate the mean
    for (i = 0; i < n; i++) {
        sum += bits[i];
    }
    mean = sum / n;

    // Calculate the variance
    sum = 0.0;
    for (i = 0; i < n; i++) {
        sum += pow(bits[i] - mean, 2);
    }
    variance = sum / (n - 1);

    // Calculate the standard deviation
    stddev = sqrt(variance);

    // Calculate the z-score
    return (bits[0] - mean) / stddev;
}

static double compute_z_score(uint32_t* nums, size_t size) {
    uint32_t num_zeros = 0;
    uint32_t num_ones = 0;

    for (size_t i = 0; i < size; i++) {
        for (size_t j = 0; j < 32; j++) {
            if (nums[i] & (1 << j)) {
                num_ones++;
            } else {
                num_zeros++;
            }
        }
    }

    // Z-Score of bitstream
    return (double)(num_ones - num_zeros) / sqrt(num_ones + num_zeros);
}

static double autocorrelation(uint32_t* nums, size_t size, size_t max_lag) {
    double mean = 0.0;
    for (size_t i = 0; i < size; i++) {
        mean += (double)nums[i] / UINT32_MAX;
    }
    mean /= size;

    double variance = 0.0;
    for (size_t i = 0; i < size; i++) {
        double x = (double)nums[i] / UINT32_MAX - mean;
        variance += x * x;
    }
    variance /= size;

    double autocorr = 1.0;
    for (size_t lag = 1; lag <= max_lag; lag++) {
        double cross_corr = 0.0;
        for (size_t i = 0; i < size - lag; i++) {
            cross_corr += ((double)nums[i] / UINT32_MAX - mean) * ((double)nums[i + lag] / UINT32_MAX - mean);
        }
        cross_corr /= (size - lag);
        autocorr -= 2.0 * cross_corr / variance;
        if (autocorr <= 0.0) {
            break;
        }
    }

    return autocorr;
}

static double shannon_entropy(uint32_t *bits, size_t n) {
    double p0 = 0.0;
    double p1 = 0.0;
    int i;

    // Count the number of 0s and 1s
    for (i = 0; i < n; i++) {
        if (bits[i] > 0x7fffffff) {
            p1++;
        } else {
            p0++;
        }
    }

    // Calculate the probabilities of 0s and 1s
    p0 /= n;
    p1 /= n;

    // Calculate the Shannon entropy
    double entropy = -p0 * log2(p0) - p1 * log2(p1);

    return entropy;
}

static double runs_test(uint32_t *bits, size_t n) {
    size_t runs = 1;
    int prev_bit = bits[0] & 1;

    // Count the number of runs
    for (size_t i = 1; i < n; i++) {
        int bit = bits[i] & 1;
        if (bit != prev_bit) {
            runs++;
        }
        prev_bit = bit;
    }

    double pi = 1.0 * runs / n;
    double tau = 2.0 / sqrt(n);
    double z = (pi - 0.5) / tau;

    double p_value = erfc(fabs(z) / sqrt(2.0));
    return p_value;
}