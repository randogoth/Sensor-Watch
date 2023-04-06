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
#include "sunriset.h"
#include "watch.h"
#include "watch_utility.h"
#include "halaqim_face.h"

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

// STATIC FUNCTIONS AND CONSTANTS /////////////////////////////////////////////

/** @details solar phase can be a day phase between sunrise and sunset or an alternating night phase.
 *  This function calculates the start and end of the current phase based on a given geographic location.
 */
static void _planetary_solar_phase(movement_settings_t *settings, halaqim_state_t *state) {
    uint8_t phase;
    double sunrise, sunset;
    uint32_t now_epoch, sunrise_epoch, sunset_epoch, midnight_epoch;
    movement_location_t movement_location = (movement_location_t) watch_get_backup_data(1);

    // check if we have a location. If not, display error
    if (movement_location.reg == 0) {
        watch_display_string("    no Loc", 0);
        state->no_location = true;
        return;
    }

    // location detected
    state->no_location = false;

    watch_date_time date_time = watch_rtc_get_date_time(); // the current local date / time
    watch_date_time utc_now = watch_utility_date_time_convert_zone(date_time, movement_timezone_offsets[settings->bit.time_zone] * 60, 0); // the current date / time in UTC
    watch_date_time scratch_time; // scratchpad, contains different values at different times
    watch_date_time midnight;
    scratch_time.reg = midnight.reg = utc_now.reg;
    midnight.unit.hour = midnight.unit.minute = midnight.unit.second = 0; // start of the day at midnight

    // get location coordinate
    int16_t lat_centi = (int16_t)movement_location.bit.latitude;
    int16_t lon_centi = (int16_t)movement_location.bit.longitude;
    double lat = (double)lat_centi / 100.0;
    double lon = (double)lon_centi / 100.0;

    // save UTC offset
    state->utc_offset = ((double)movement_timezone_offsets[settings->bit.time_zone]) / 60.0;

    // get UNIX epoch time
    now_epoch = watch_utility_date_time_to_unix_time(utc_now, 0);
    midnight_epoch = watch_utility_date_time_to_unix_time(midnight, 0);

    // calculate sunrise and sunset of current day in decimal hours after midnight
    sun_rise_set(scratch_time.unit.year + WATCH_RTC_REFERENCE_YEAR, scratch_time.unit.month, scratch_time.unit.day, lon, lat, &sunrise, &sunset);
    
    // calculate sunrise and sunset UNIX timestamps
    sunrise_epoch = midnight_epoch + sunrise * 3600;
    sunset_epoch = midnight_epoch + sunset * 3600;

    // by default we assume it is daytime (phase 1) between sunrise and sunset
    phase = 1;
    state->night = false;
    state->phase_start = sunrise_epoch - 4320;
    state->phase_end = sunset_epoch + 1080;

    // night time calculations
    if ( now_epoch < sunrise_epoch && now_epoch < sunset_epoch ) phase = 0; // morning before dawn
    if ( now_epoch > sunrise_epoch && now_epoch >= sunset_epoch ) phase = 2; // evening after dusk

    // phase 0: we are before sunrise
    if ( phase == 0) {
        // go back to yesterday and calculate sunset
        midnight_epoch -= 86400;
        scratch_time = watch_utility_date_time_from_unix_time(midnight_epoch, 0);
        sun_rise_set(scratch_time.unit.year + WATCH_RTC_REFERENCE_YEAR, scratch_time.unit.month, scratch_time.unit.day, lon, lat, &sunrise, &sunset);
        sunset_epoch = midnight_epoch + sunset * 3600;
        // we are still in yesterday's night hours
        state->night = true;
        state->phase_start = sunset_epoch + 1080;
        state->phase_end = sunrise_epoch - 4320;
    }

    // phase 2: we are after sunset
    if ( phase == 2) {
        // skip to tomorrow and calculate sunrise
        midnight_epoch += 86400;
        scratch_time = watch_utility_date_time_from_unix_time(midnight_epoch, 0);
        sun_rise_set(scratch_time.unit.year + WATCH_RTC_REFERENCE_YEAR, scratch_time.unit.month, scratch_time.unit.day, lon, lat, &sunrise, &sunset);
        sunrise_epoch = midnight_epoch + sunrise * 3600;
        // we are still in yesterday's night hours
        state->night = true;
        state->phase_start = sunset_epoch + 1080;
        state->phase_end = sunrise_epoch - 4320;
    }
}

/** @details A planetary hour is one of exactly twelve hours of a solar phase. Its length varies.
 *  This function calculates the current planetary hour and divides it up into relative minutes and seconds.
 *  It also calculates the current planetary ruler of the hour and of the day.
 */
static void _planetary_time(movement_event_t event, movement_settings_t *settings, halaqim_state_t *state) {
    char buf[14];
    char ruler[3];
    double night_hour_count = 0.0;
    uint8_t weekday, planet, planetary_hour;
    double hour_duration, current_hour, current_heleq, current_rega;

    // get current time and convert to UTC
    state->scratch = watch_utility_date_time_convert_zone(watch_rtc_get_date_time(), movement_timezone_offsets[settings->bit.time_zone] * 60, 0); 

    // when current phase ends calculate the next phase
    if ( watch_utility_date_time_to_unix_time(state->scratch, 0) >= state->phase_end ) {
        _planetary_solar_phase(settings, state);
        return;
    }

    // calculate the duration of a planetary hour during this solar phase
    hour_duration = (( state->phase_end - state->phase_start)) / 12.0;

    // which planetary hour are we in?

    // RTC only provides full second precision, so we have to manually add subseconds with each tick
    current_hour = ((( watch_utility_date_time_to_unix_time(state->scratch, 0) ) + event.subsecond * (state->regaim ? 0.0303030303 : 0.111111111)) - state->phase_start ) / hour_duration;
    planetary_hour = floor(current_hour) + ( state->night ? 0 : 12 );
    current_hour  += night_hour_count; //adjust for 24hr display
    current_heleq = modf(current_hour, &current_hour) * 1080.0;
    current_rega = modf(current_heleq, &current_heleq) * 76.0;

    // the day changes after sunrise, so if we are at night it is yesterday's planetary day
    // hence we take the datetime object of when the last solar phase started as the current day
    // and then fill in the hours and minutes
    state->scratch = watch_utility_date_time_from_unix_time(state->phase_start, 0);
    state->scratch.unit.hour = floor(current_hour);
    state->scratch.unit.minute = floor(current_heleq);
    state->scratch.unit.second = floor(current_rega);

    // what weekday is it (0 - 6)
    weekday = watch_utility_get_iso8601_weekday_number(state->scratch.unit.year, state->scratch.unit.month, state->scratch.unit.day) - 1;
    
    watch_set_colon();

    if ( state->regaim )
        sprintf(buf, "%02d  %2d%04d", (uint8_t)floor(current_rega), (uint8_t)floor(current_hour), (uint8_t)floor(current_heleq));
    else
        sprintf(buf, "ZM  %2d%04d", (uint8_t)floor(current_hour), (uint8_t)floor(current_heleq));
    watch_display_string(buf, 0);

}

// PUBLIC WATCH FACE FUNCTIONS ////////////////////////////////////////////////

void halaqim_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(halaqim_state_t));
        memset(*context_ptr, 0, sizeof(halaqim_state_t));
    }
}

void halaqim_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    if (watch_tick_animation_is_running()) watch_stop_tick_animation();

#if __EMSCRIPTEN__
    int16_t browser_lat = EM_ASM_INT({ return lat; });
    int16_t browser_lon = EM_ASM_INT({ return lon; });
    if ((watch_get_backup_data(1) == 0) && (browser_lat || browser_lon)) {
        movement_location_t browser_loc;
        browser_loc.bit.latitude = browser_lat;
        browser_loc.bit.longitude = browser_lon;
        watch_store_backup_data(browser_loc.reg, 1);
    }
#endif

    halaqim_state_t *state = (halaqim_state_t *)context;
    
    // calculate phase
    _planetary_solar_phase(settings, state);
}

bool halaqim_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    halaqim_state_t *state = (halaqim_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _planetary_time(event, settings, state);
            if ( state->regaim )
                movement_request_tick_frequency( 32 );
            else
                movement_request_tick_frequency( 8 );
            break;
        case EVENT_TICK:
            _planetary_time(event, settings, state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->regaim = !state->regaim;
            if ( state->regaim )
                movement_request_tick_frequency( 32 );
            else
                movement_request_tick_frequency( 8 );
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void halaqim_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    movement_request_tick_frequency( 1 );
}

