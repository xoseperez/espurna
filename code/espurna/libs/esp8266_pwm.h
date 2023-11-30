/*
 * Copyright (C) 2016 Stefan Brüns <stefan.bruens@rwth-aachen.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pwm_pin_info {
    uint32_t addr;
    uint32_t func;
    uint32_t pin;
};

/* pwm_init should be called only once, for now  */
void pwm_init(uint32_t period, uint32_t *duty, uint32_t pwm_channel_num, struct pwm_pin_info *pin_info_list);
void pwm_start(void);

void pwm_set_duty(uint32_t duty, uint8_t channel);
uint32_t pwm_get_duty(uint8_t channel);
void pwm_set_period(uint32_t period);
uint32_t pwm_get_period(void);

uint32_t get_pwm_version(void);
void set_pwm_debug_en(uint8_t print_en);

#ifdef __cplusplus
}
#endif
