#ifndef _PD_H
#define _PD_H


#include <stdint.h>

#include "usbpd.h"


struct pd_profile_t {
	uint16_t voltage;     // Vbus Voltage in millivolts
	uint16_t max_current; // Maximum current in milliamps
	uint16_t power_avail; // Available power (from supply) in watts
	uint16_t set_power;   // Maximum power in watts, set by the user
	uint16_t set_temp;    // Set temperature in celsius, set by the user
	uint16_t tip_r;       // Tip resistance in milliOhms
	uint8_t  max_duty;    // Maximum duty cycle (0-100) to stay within the power limit
};


USBPD_Result_e pd_get_result();
bool pd_negotiate(USBPD_VCC_e vcc);
bool pd_get_profile(struct pd_profile_t *profile, uint16_t min_power);

#endif // _PD_H
