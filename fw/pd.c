#include <ch32fun.h>

#define USBPD_IMPLEMENTATION
#include "usbpd.h"

#include "funconfig.h"
#include "display.h"

#include "pd.h"


extern u8g2_t *u8g2;

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

		u8g2_ClearBuffer(u8g2);
		u8g2_SetBitmapMode(u8g2, 1);
		u8g2_SetFontMode(u8g2, 1);
		u8g2_SetFont(u8g2, u8g2_font_5x8_tr);
		u8g2_DrawStr(u8g2, 0, 8+7, USBPD_StateToStr(USBPD_GetState()));
		u8g2_SendBuffer(u8g2);

		Delay_Ms(1);
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
			switch (pdo->Header.AugmentedType) {
				case eUSBPD_APDO_SPR_PPS:
					// TODO: SPR_PPS
					break;
				case eUSBPD_APDO_SPR_AVS:
					// TODO: SPR AVS
					break;
				case eUSBPD_APDO_EPR_AVS:
				/* TODO: EPR AVS
					voltage = pdo->EPR_AVS.MaxVoltageIn100mV * 100;
					current = pdo->EPR_AVS.PeakCurrent * 1000;
					power = pdo->EPR_AVS.PDPIn1W;
				*/
					break;
				default:
					break;
			}
			break;
		}

		// Selects the first PDO that meets the minimum power requirement
		if (power >= min_power && voltage <= BOARD_MAX_VOLTAGE) {
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
