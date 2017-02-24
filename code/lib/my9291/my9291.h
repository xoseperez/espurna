/*

MY9291 LED Driver Arduino library 0.1.0

Copyright (c) 2016 - 2026 MaiKe Labs
Copyright (C) 2017 - Xose Pérez

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _my9291_h
#define _my9291_h

#include <Arduino.h>

typedef enum my9291_cmd_one_shot_t {
        MY9291_CMD_ONE_SHOT_DISABLE = 0X00,
        MY9291_CMD_ONE_SHOT_ENFORCE = 0X01,
} my9291_cmd_one_shot_t;

typedef enum my9291_cmd_reaction_t {
        MY9291_CMD_REACTION_FAST = 0X00,
        MY9291_CMD_REACTION_SLOW = 0X01,
} my9291_cmd_reaction_t;

typedef enum my9291_cmd_bit_width_t {
        MY9291_CMD_BIT_WIDTH_16 = 0X00,
        MY9291_CMD_BIT_WIDTH_14 = 0X01,
        MY9291_CMD_BIT_WIDTH_12 = 0X02,
        MY9291_CMD_BIT_WIDTH_8 = 0X03,
} my9291_cmd_bit_width_t;

typedef enum my9291_cmd_frequency_t {
        MY9291_CMD_FREQUENCY_DIVIDE_1 = 0X00,
        MY9291_CMD_FREQUENCY_DIVIDE_4 = 0X01,
        MY9291_CMD_FREQUENCY_DIVIDE_16 = 0X02,
        MY9291_CMD_FREQUENCY_DIVIDE_64 = 0X03,
} my9291_cmd_frequency_t;

typedef enum my9291_cmd_scatter_t {
        MY9291_CMD_SCATTER_APDM = 0X00,
        MY9291_CMD_SCATTER_PWM = 0X01,
} my9291_cmd_scatter_t;

typedef struct {
        my9291_cmd_scatter_t scatter:1;
        my9291_cmd_frequency_t frequency:2;
        my9291_cmd_bit_width_t bit_width:2;
        my9291_cmd_reaction_t reaction:1;
        my9291_cmd_one_shot_t one_shot:1;
        unsigned char resv:1;
} __attribute__ ((aligned(1), packed)) my9291_cmd_t;

#define MY9291_COMMAND_DEFAULT { \
    .scatter = MY9291_CMD_SCATTER_APDM, \
    .frequency = MY9291_CMD_FREQUENCY_DIVIDE_1, \
    .bit_width = MY9291_CMD_BIT_WIDTH_8, \
    .reaction = MY9291_CMD_REACTION_FAST, \
    .one_shot = MY9291_CMD_ONE_SHOT_DISABLE, \
    .resv = 0 \
}

class my9291 {

    public:

        my9291(unsigned char di, unsigned char dcki, my9291_cmd_t command);
        void send(unsigned int duty_r, unsigned int duty_g, unsigned int duty_b, unsigned int duty_w);

    private:

        void _di_pulse(unsigned int times);
        void _dcki_pulse(unsigned int times);
        void _set_cmd(my9291_cmd_t command);

        my9291_cmd_t _command;
        unsigned char _pin_di;
        unsigned char _pin_dcki;

};

#endif
