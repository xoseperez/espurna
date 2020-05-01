// -----------------------------------------------------------------------------
// LDR Sensor (maps to a LDRSensor)
// Copyright (C) 2019 by Altan Altay
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && LDR_SUPPORT

#pragma once

#include <Arduino.h>

#include "AnalogSensor.h"
extern "C" {
	#include "../libs/fs_math.h"
}

#define LDR_GL5516		1
#define LDR_GL5528		2
#define LDR_GL5537_1	3
#define LDR_GL5537_2	4
#define LDR_GL5539		5
#define LDR_GL5549		6
#define LDR_OTHER		99

class LDRSensor : public AnalogSensor {

	public:

		// ---------------------------------------------------------------------
		// Public
		// ---------------------------------------------------------------------

		LDRSensor() {
			_count = 1;
			_sensor_id = SENSOR_LDR_ID;
		}

		void setType(unsigned char type) {
		
			_type = type;

			switch (_type) {
				case LDR_GL5516:
					_mult_value = 29634400;
					_pow_value = 1.6689;
					break;
				case LDR_GL5537_1:
					_mult_value = 32435800;
					_pow_value = 1.4899;
					break;
				case LDR_GL5537_2:
					_mult_value = 2801820;
					_pow_value = 1.1772;
					break;
				case LDR_GL5539:
					_mult_value = 208510000;
					_pow_value = 1.4850;
					break;
				case LDR_GL5549:
					_mult_value = 44682100;
					_pow_value = 1.2750;
					break;
				case LDR_OTHER:
					_mult_value = LDR_MULTIPLICATION;
					_pow_value = LDR_POWER;
					break;
				case LDR_GL5528:
				default:
					_mult_value = 32017200;
					_pow_value = 1.5832;
					break;
			}
		}

		/*!
		* \brief setPhotocellPositionOnGround Configure the photocell as connected to 3.3V or GND
		*
		* \param on_ground (bool) True if the photocell is connected to GND, else false
		*
		*  True:          EXTERNAL ADC
		*                      ^	   ^
		*              _____   |   ___/___
		*    3.3V |---|_____|--*--|__/____|--| GND
		*               Other       /
		*             Resistor    Photocell
		*
		*  False:
		*                EXTERNAL ADC
		*                     ^	      ^
		*             _____   |   ___/___
		*    GND |---|_____|--*--|__/____|--| 3.3V
		*             Other        /
		*            Resistor    Photocell
		*/
		void setPhotocellPositionOnGround(bool on_ground) {
			_photocell_on_ground = on_ground;
		}

		void setResistor(unsigned long resistor) {
			_resistor = resistor;
		}

		/*!
		* \brief updatePhotocellParameters Redefine the photocell parameters
		*
		* \parameter mult_value (float) Multiplication parameter in "I[lux]=mult_value/(R[Ω]^pow_value)" expression
		* \parameter pow_value (float) Power parameter in "I[lux]=mult_value/(R[Ω]^pow_value)" expression
		*/
		void setPhotocellParameters(float mult_value, float pow_value) {
			if (_type == LDR_OTHER) {
				_mult_value = mult_value;
				_pow_value = pow_value;
			}
		}

		// ---------------------------------------------------------------------

		// ---------------------------------------------------------------------
		// Sensor API
		// ---------------------------------------------------------------------

		// Descriptive name of the sensor
		String description() {
			return String("LDR @ TOUT");
		}

		// Descriptive name of the slot # index
		String slot(unsigned char index) {
			return description();
		}

		// Address of the sensor (it could be the GPIO or I2C address)
		String address(unsigned char index) {
			return String("0");
		}

		// Type for slot # index
		unsigned char type(unsigned char index) {
			if (index == 0) return MAGNITUDE_LUX;
			return MAGNITUDE_NONE;
		}

		// Current value for slot # index
		double value(unsigned char index) {

			double current_lux = 0;

			if (index == 0) {
				
				unsigned long photocell_resistor = 0;

				// sampled reading
				double read = _read();

				float ratio = ((float)1024/(float)read) - 1;
				if (_photocell_on_ground) {
					photocell_resistor = _resistor / ratio;
				} else {
					photocell_resistor = _resistor * ratio;
				}

				current_lux = _mult_value / (float)pow(photocell_resistor, _pow_value);
			}

			return current_lux;

		}

	protected:

		unsigned char _type = LDR_GL5528;
		bool _photocell_on_ground = false;
		unsigned long _resistor = 10000;
		float _mult_value = 0;
		float _pow_value = 0;

};

#endif // SENSOR_SUPPORT && LDR_SUPPORT
