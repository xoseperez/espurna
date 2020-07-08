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

/* Set the following three defines to your needs */

#ifndef SDK_PWM_PERIOD_COMPAT_MODE
#define SDK_PWM_PERIOD_COMPAT_MODE  0
#endif

#ifndef PWM_MAX_CHANNELS
#define PWM_MAX_CHANNELS            8
#endif

#define PWM_DEBUG                   0
#define PWM_USE_NMI                 1

/* no user servicable parts beyond this point */

#define PWM_MAX_TICKS               0x7fffff
#if SDK_PWM_PERIOD_COMPAT_MODE
#define PWM_PERIOD_TO_TICKS(x)      (x * 0.2)
#define PWM_DUTY_TO_TICKS(x)        (x * 5)
#define PWM_MAX_DUTY                (PWM_MAX_TICKS * 0.2)
#define PWM_MAX_PERIOD              (PWM_MAX_TICKS * 5)
#else
#define PWM_PERIOD_TO_TICKS(x)      (x)
#define PWM_DUTY_TO_TICKS(x)        (x)
#define PWM_MAX_DUTY                PWM_MAX_TICKS
#define PWM_MAX_PERIOD              PWM_MAX_TICKS
#endif

#include <c_types.h>
#include <eagle_soc.h>
#include <ets_sys.h>
#include "libs/pwm.h"

// from SDK hw_timer.c
#define TIMER1_DIVIDE_BY_16         0x0004
#define TIMER1_ENABLE_TIMER         0x0080

struct pwm_phase {
	uint32_t ticks;    ///< delay until next phase, in 200ns units
	uint16_t on_mask;  ///< GPIO mask to switch on
	uint16_t off_mask; ///< GPIO mask to switch off
};

/* Three sets of PWM phases, the active one, the one used
 * starting with the next cycle, and the one updated
 * by pwm_start. After the update pwm_next_set
 * is set to the last updated set. pwm_current_set is set to
 * pwm_next_set from the interrupt routine during the first
 * pwm phase
 */
typedef struct pwm_phase (pwm_phase_array)[PWM_MAX_CHANNELS + 2];
static pwm_phase_array pwm_phases[3];
static struct {
	struct pwm_phase* next_set;
	struct pwm_phase* current_set;
	uint8_t current_phase;
} pwm_state;

static uint32_t pwm_period;
static uint32_t pwm_period_ticks;
static uint32_t pwm_duty[PWM_MAX_CHANNELS];
static uint16_t gpio_mask[PWM_MAX_CHANNELS];
static uint8_t pwm_channels;

// 3-tuples of MUX_REGISTER, MUX_VALUE and GPIO number
typedef uint32_t (pin_info_type)[3];

struct gpio_regs {
	uint32_t out;         /* 0x60000300 */
	uint32_t out_w1ts;    /* 0x60000304 */
	uint32_t out_w1tc;    /* 0x60000308 */
	uint32_t enable;      /* 0x6000030C */
	uint32_t enable_w1ts; /* 0x60000310 */
	uint32_t enable_w1tc; /* 0x60000314 */
	uint32_t in;          /* 0x60000318 */
	uint32_t status;      /* 0x6000031C */
	uint32_t status_w1ts; /* 0x60000320 */
	uint32_t status_w1tc; /* 0x60000324 */
};
static struct gpio_regs* gpio = (struct gpio_regs*)(0x60000300);

struct timer_regs {
	uint32_t frc1_load;   /* 0x60000600 */
	uint32_t frc1_count;  /* 0x60000604 */
	uint32_t frc1_ctrl;   /* 0x60000608 */
	uint32_t frc1_int;    /* 0x6000060C */
	uint8_t  pad[16];
	uint32_t frc2_load;   /* 0x60000620 */
	uint32_t frc2_count;  /* 0x60000624 */
	uint32_t frc2_ctrl;   /* 0x60000628 */
	uint32_t frc2_int;    /* 0x6000062C */
	uint32_t frc2_alarm;  /* 0x60000630 */
};
static struct timer_regs* timer = (struct timer_regs*)(0x60000600);

static void ICACHE_RAM_ATTR
pwm_intr_handler(void)
{
	if ((pwm_state.current_set[pwm_state.current_phase].off_mask == 0) &&
	    (pwm_state.current_set[pwm_state.current_phase].on_mask == 0)) {
		pwm_state.current_set = pwm_state.next_set;
		pwm_state.current_phase = 0;
	}

	do {
		// force write to GPIO registers on each loop
		__asm__ volatile ("" : : : "memory");

		gpio->out_w1ts = (uint32_t)(pwm_state.current_set[pwm_state.current_phase].on_mask);
		gpio->out_w1tc = (uint32_t)(pwm_state.current_set[pwm_state.current_phase].off_mask);

		uint32_t ticks = pwm_state.current_set[pwm_state.current_phase].ticks;

		pwm_state.current_phase++;

		if (ticks) {
			if (ticks >= 16) {
				// constant interrupt overhead
				ticks -= 9;
				timer->frc1_int &= ~FRC1_INT_CLR_MASK;
				WRITE_PERI_REG(&timer->frc1_load, ticks);
				return;
			}

			ticks *= 4;
			do {
				ticks -= 1;
				// stop compiler from optimizing delay loop to noop
				__asm__ volatile ("" : : : "memory");
			} while (ticks > 0);
		}

	} while (1);
}

/**
 * period: initial period (base unit 1us OR 200ns)
 * duty: array of initial duty values, may be NULL, may be freed after pwm_init
 * pwm_channel_num: number of channels to use
 * pin_info_list: array of pin_info
 */
void ICACHE_FLASH_ATTR
pwm_init(uint32_t period, uint32_t *duty, uint32_t pwm_channel_num,
              uint32_t (*pin_info_list)[3])
{
	int i, j, n;

	pwm_channels = pwm_channel_num;
	if (pwm_channels > PWM_MAX_CHANNELS)
		pwm_channels = PWM_MAX_CHANNELS;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < (PWM_MAX_CHANNELS + 2); j++) {
			pwm_phases[i][j].ticks = 0;
			pwm_phases[i][j].on_mask = 0;
			pwm_phases[i][j].off_mask = 0;
		}
	}
	pwm_state.current_set = pwm_state.next_set = 0;
	pwm_state.current_phase = 0;

	uint32_t all = 0;
	// PIN info: MUX-Register, Mux-Setting, PIN-Nr
	for (n = 0; n < pwm_channels; n++) {
		pin_info_type* pin_info = &pin_info_list[n];
		PIN_FUNC_SELECT((*pin_info)[0], (*pin_info)[1]);
		gpio_mask[n] = 1 << (*pin_info)[2];
		all |= 1 << (*pin_info)[2];
		if (duty)
			pwm_set_duty(duty[n], n);
	}
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, all);
	GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, all);

	pwm_set_period(period);

#if PWM_USE_NMI
	ETS_FRC_TIMER1_NMI_INTR_ATTACH(pwm_intr_handler);
#else
	ETS_FRC_TIMER1_INTR_ATTACH(pwm_intr_handler, NULL);
#endif
	TM1_EDGE_INT_ENABLE();

	timer->frc1_int &= ~FRC1_INT_CLR_MASK;
	timer->frc1_ctrl = 0;

	pwm_start();
}

__attribute__ ((noinline))
static uint8_t ICACHE_FLASH_ATTR
_pwm_phases_prep(struct pwm_phase* pwm)
{
	uint8_t n, phases;

	for (n = 0; n < pwm_channels + 2; n++) {
		pwm[n].ticks = 0;
		pwm[n].on_mask = 0;
		pwm[n].off_mask = 0;
	}
	phases = 1;
	for (n = 0; n < pwm_channels; n++) {
		uint32_t ticks = PWM_DUTY_TO_TICKS(pwm_duty[n]);
		if (ticks == 0) {
			pwm[0].off_mask |= gpio_mask[n];
		} else if (ticks >= pwm_period_ticks) {
			pwm[0].on_mask |= gpio_mask[n];
		} else {
			if (ticks < (pwm_period_ticks/2)) {
				pwm[phases].ticks = ticks;
				pwm[0].on_mask |= gpio_mask[n];
				pwm[phases].off_mask = gpio_mask[n];
			} else {
				pwm[phases].ticks = pwm_period_ticks - ticks;
				pwm[phases].on_mask = gpio_mask[n];
				pwm[0].off_mask |= gpio_mask[n];
			}
			phases++;
		}
	}
	pwm[phases].ticks = pwm_period_ticks;

	// bubble sort, lowest to hightest duty
	n = 2;
	while (n < phases) {
		if (pwm[n].ticks < pwm[n - 1].ticks) {
			struct pwm_phase t = pwm[n];
			pwm[n] = pwm[n - 1];
			pwm[n - 1] = t;
			if (n > 2)
				n--;
		} else {
			n++;
		}
	}

#if PWM_DEBUG
        int t = 0;
	for (t = 0; t <= phases; t++) {
		ets_printf("%d @%d:   %04x %04x\n", t, pwm[t].ticks, pwm[t].on_mask, pwm[t].off_mask);
	}
#endif

	// shift left to align right edge;
	uint8_t l = 0, r = 1;
	while (r <= phases) {
		uint32_t diff = pwm[r].ticks - pwm[l].ticks;
		if (diff && (diff <= 16)) {
			uint16_t mask = pwm[r].on_mask | pwm[r].off_mask;
			pwm[l].off_mask ^= pwm[r].off_mask;
			pwm[l].on_mask ^= pwm[r].on_mask;
			pwm[0].off_mask ^= pwm[r].on_mask;
			pwm[0].on_mask ^= pwm[r].off_mask;
			pwm[r].ticks = pwm_period_ticks - diff;
			pwm[r].on_mask ^= mask;
			pwm[r].off_mask ^= mask;
		} else {
			l = r;
		}
		r++;
	}

#if PWM_DEBUG
	for (t = 0; t <= phases; t++) {
		ets_printf("%d @%d:   %04x %04x\n", t, pwm[t].ticks, pwm[t].on_mask, pwm[t].off_mask);
	}
#endif

	// sort again
	n = 2;
	while (n <= phases) {
		if (pwm[n].ticks < pwm[n - 1].ticks) {
			struct pwm_phase t = pwm[n];
			pwm[n] = pwm[n - 1];
			pwm[n - 1] = t;
			if (n > 2)
				n--;
		} else {
			n++;
		}
	}

	// merge same duty
	l = 0, r = 1;
	while (r <= phases) {
		if (pwm[r].ticks == pwm[l].ticks) {
			pwm[l].off_mask |= pwm[r].off_mask;
			pwm[l].on_mask |= pwm[r].on_mask;
			pwm[r].on_mask = 0;
			pwm[r].off_mask = 0;
		} else {
			l++;
			if (l != r) {
				struct pwm_phase t = pwm[l];
				pwm[l] = pwm[r];
				pwm[r] = t;
			}
		}
		r++;
	}
	phases = l;

#if PWM_DEBUG
	for (t = 0; t <= phases; t++) {
		ets_printf("%d @%d:   %04x %04x\n", t, pwm[t].ticks, pwm[t].on_mask, pwm[t].off_mask);
	}
#endif

	// transform absolute end time to phase durations
	for (n = 0; n < phases; n++) {
		pwm[n].ticks =
			pwm[n + 1].ticks - pwm[n].ticks;
		// subtract common overhead
		pwm[n].ticks--;
	}
	pwm[phases].ticks = 0;

	// do a cyclic shift if last phase is short
	if (pwm[phases - 1].ticks < 16) {
		for (n = 0; n < phases - 1; n++) {
			struct pwm_phase t = pwm[n];
			pwm[n] = pwm[n + 1];
			pwm[n + 1] = t;
		}
	}

#if PWM_DEBUG
	for (t = 0; t <= phases; t++) {
		ets_printf("%d +%d:   %04x %04x\n", t, pwm[t].ticks, pwm[t].on_mask, pwm[t].off_mask);
	}
	ets_printf("\n");
#endif

	return phases;
}

void ICACHE_FLASH_ATTR
pwm_start(void)
{
	pwm_phase_array* pwm = &pwm_phases[0];

	if ((*pwm == pwm_state.next_set) ||
	    (*pwm == pwm_state.current_set))
		pwm++;
	if ((*pwm == pwm_state.next_set) ||
	    (*pwm == pwm_state.current_set))
		pwm++;

	uint8_t phases = _pwm_phases_prep(*pwm);

        // all with 0% / 100% duty - stop timer
	if (phases == 1) {
		if (pwm_state.next_set) {
#if PWM_DEBUG
			ets_printf("PWM stop\n");
#endif
			timer->frc1_ctrl = 0;
			ETS_FRC1_INTR_DISABLE();
		}
		pwm_state.next_set = NULL;

		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, (*pwm)[0].on_mask);
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, (*pwm)[0].off_mask);

		return;
	}

	// start if not running
	if (!pwm_state.next_set) {
#if PWM_DEBUG
		ets_printf("PWM start\n");
#endif
		pwm_state.current_set = pwm_state.next_set = *pwm;
		pwm_state.current_phase = phases - 1;
		ETS_FRC1_INTR_ENABLE();
		#if defined(TIMER_REG_WRITE)
			TIMER_REG_WRITE(FRC1_LOAD_ADDRESS, 0);
		#else
			RTC_REG_WRITE(FRC1_LOAD_ADDRESS, 0);
		#endif
		timer->frc1_ctrl = TIMER1_DIVIDE_BY_16 | TIMER1_ENABLE_TIMER;
		return;
	}

	pwm_state.next_set = *pwm;
}

void ICACHE_FLASH_ATTR
pwm_set_duty(uint32_t duty, uint8_t channel)
{
	if (channel >= PWM_MAX_CHANNELS)
		return;

	if (duty > PWM_MAX_DUTY)
		duty = PWM_MAX_DUTY;

	pwm_duty[channel] = duty;
}

uint32_t ICACHE_FLASH_ATTR
pwm_get_duty(uint8_t channel)
{
	if (channel >= PWM_MAX_CHANNELS)
		return 0;
	return pwm_duty[channel];
}

void ICACHE_FLASH_ATTR
pwm_set_period(uint32_t period)
{
	pwm_period = period;

	if (pwm_period > PWM_MAX_PERIOD)
		pwm_period = PWM_MAX_PERIOD;

	pwm_period_ticks = PWM_PERIOD_TO_TICKS(period);
}

uint32_t ICACHE_FLASH_ATTR
pwm_get_period(void)
{
	return pwm_period;
}

uint32_t ICACHE_FLASH_ATTR
get_pwm_version(void)
{
	return 1;
}

void ICACHE_FLASH_ATTR
set_pwm_debug_en(uint8_t print_en)
{
	(void) print_en;
}
