#include <ch32fun.h>

#define USBPD_IMPLEMENTATION
#include "usbpd.h"

#include "pd.h"


static size_t cap_count = 0;
static USBPD_SPR_CapabilitiesMessage_t *capabilities = NULL;
static USBPD_Result_e result;


USBPD_Result_e pd_get_result()
{
	return result;
}

// Initializes the Power Delivery peripheral and starts the Power Delivery
// negotiation, returns true if successful, indicating the board is connected to
// a Power Delivery source.
// As a side effect, fills the capabilities buffer with the source's capabilities.
bool pd_negotiate(USBPD_VCC_e vcc)
{
	result = USBPD_Init(vcc);
	if (result != eUSBPD_OK) {
		return false;
	}

	Delay_Ms(100);

	u32 start = funSysTick32();
	while (eUSBPD_BUSY == (result = USBPD_SinkNegotiate())) {
		u32 now = funSysTick32();
		if (now - start > Ticks_from_Ms(5000)) {
			break;
		}
		Delay_Ms(100);
	}
	if (result != eUSBPD_OK) {
		return false;
	}
	cap_count = USBPD_GetCapabilities(&capabilities);
	if (capabilities == NULL || cap_count == 0) {
		return false;
	}
	return true;
}


bool pd_get_profile(struct pd_profile_t *profile, uint16_t min_power)
{
	if (profile == NULL || min_power == 0 || min_power > 140) {
		return false;
	}
	if (capabilities == NULL || cap_count == 0) {
		return false;
	}

	u16 voltage = 0, current = 0, power = 0;
	for (u32 i = 0; i < cap_count; i++) {
		USBPD_SinkPDO_t *pdo = &capabilities->Sink[i];
		switch (pdo->Header.PDOType) {
		case eUSBPD_PDO_FIXED:
			voltage = pdo->FixedSupply.VoltageIn50mV * 50;
			current = pdo->FixedSupply.CurrentIn10mA * 10;
			power = ((u32)voltage * current) / (u32)1000000;
			break;
		case eUSBPD_PDO_BATTERY:
			voltage = pdo->BatterySupply.MaxVoltageIn50mV * 50;
			power = pdo->BatterySupply.MaxPowerIn250mW / 4;
			current = ((u32)power * 1000) / voltage;
			break;
		case eUSBPD_PDO_VARIABLE:
			// TODO: PPS
			// if (pdo->VariableSupply.MaxVoltageIn50mV/20 > max_v) {
			// 	max_v = pdo->VariableSupply.MaxVoltageIn50mV/20;
			// }
			break;
		case eUSBPD_PDO_AUGMENTED:
			// TODO: EPR
			break;
		}

		// Selects the first PDO that meets the minimum power requirement
		if (power >= min_power) {
			if (USBPD_SelectPDO(i, 0) != eUSBPD_OK) {
				return false;
			}
			profile->voltage = voltage;
			profile->max_current = current;
			profile->power_avail = power;
			return true;
		}
	}

	return false;
}
