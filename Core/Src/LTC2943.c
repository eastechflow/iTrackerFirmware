/*************************************************************************
* COPYRIGHT - All Rights Reserved.
* 2019 Eastech Flow Controls, Inc.
*
* OWNER:
* Eastech Flow Controls, Inc.
* 4250 S. 76th E. Avenue
* Tulsa, Oklahoma 74145
*
* toll-free: 	(800) 226-3569
* phone: 	(918) 664-1212
* fax: 	(918) 664-8494
* email: 	info@eastechflow.com
* website: 	http://eastechflow.com/
*
* OBLIGATORY: All information contained herein is, and remains
* the property of EASTECH FLOW CONTROLS, INC. The intellectual
* and technical concepts contained herein are proprietary to
* EASTECH FLOW CONTROLS, INC. and its suppliers and may be covered
* by U.S. and Foreign Patents, patents in process, and are protected
* by trade secret or copyright law.
*
* Dissemination, transmission, forwarding or reproduction of this
* content in any way is strictly forbidden.
*************************************************************************/

#include "main.h"
#include "i2c.h"
#include "LTC2943.h"
#include "quadspi.h"
#include "factory.h"
#include "rtc.h"
#include "debug_helper.h"
#include "trace.h"

#if 1
#define LTC2943_I2C_INSTANCE         0
#define LTC294X_WORK_DELAY              10      /* Update delay in seconds */
#define LTC294X_MAX_VALUE               0xFFFF
#define LTC294X_MID_SUPPLY              0x7FFF
#define LTC2941_MAX_PRESCALER_EXP       7
#define LTC2943_MAX_PRESCALER_EXP       6
#define LTC294X_REG_STATUS              0x00
#define LTC294X_REG_CONTROL             0x01
#define LTC294X_REG_ACC_CHARGE_MSB      0x02
#define LTC294X_REG_ACC_CHARGE_LSB      0x03
#define LTC294X_REG_THRESH_HIGH_MSB     0x04
#define LTC294X_REG_THRESH_HIGH_LSB     0x05
#define LTC294X_REG_THRESH_LOW_MSB      0x06
#define LTC294X_REG_THRESH_LOW_LSB      0x07
#define LTC294X_REG_VOLTAGE_MSB 		0x08
#define LTC294X_REG_VOLTAGE_LSB			0x09
#define LTC294X_REG_CURRENT_MSB			0x0E
#define LTC294X_REG_CURRENT_LSB			0x0F
#define LTC294X_REG_TEMPERATURE_MSB		0x14
#define LTC294X_REG_TEMPERATURE_LSB		0x15
#define LTC2943_REG_CONTROL_MODE_MASK 			(BIT(7) | BIT(6))
#define LTC2943_REG_CONTROL_MODE_SCAN 			BIT(7)
#define LTC294X_REG_CONTROL_PRESCALER_MASK      (BIT(5) | BIT(4) | BIT(3))
#define LTC294X_REG_CONTROL_SHUTDOWN_MASK       (BIT(0))
#define LTC294X_REG_CONTROL_PRESCALER_SET(x) \
         ((x << 3) & LTC294X_REG_CONTROL_PRESCALER_MASK)
#define LTC294X_REG_CONTROL_ALCC_CONFIG_DISABLED        0
#define LTC2943_NUM_REGS        0x18

typedef struct s_ltc2943_cntl {
	unsigned int Shutdown :1;
	unsigned int ALCC :2;
	unsigned int Prescaler_M :3;
	unsigned int ADC_Mode :2;
} s_ltc2943_cntl;

typedef struct s_ltc2943_status {
	unsigned int UnderVoltage :1;
	unsigned int Voltage :1;
	unsigned int Charge_Low :1;
	unsigned int Charge_High :1;
	unsigned int Temperature :1;
	unsigned int Accu_Rollover :1;
	unsigned int Current_Limit :1;
	unsigned int NU :1;
} s_ltc2943_status;

typedef struct s_ltc2943_regs
{
	union {
		s_ltc2943_status REG_STATUS; //0x0000
		uint8_t reg_status;
	};

	union {
		s_ltc2943_cntl REG_CNTL; //0x0000
		uint8_t reg_cntl;
	};
} s_ltc2943_regs;

#endif

s_ltc2943_regs ltc2943_Regs;





#define I2C_ADDRESS	0x64U	//  0x30F

// There is a two-step initialization process for the LTC2943:
//
// 1. Initialize the pre-scaler (M) value to match the sense resistor.  (Normally this is done as close to power on as possible.)
// 2. Initialize the accumulator to match the last saved battery guage value.
//
// The firmware cannot control when the LTC2943 gets power.
// For example, if USB is used to power-on the device, then batteries are added,
// the MCU power-on process has long-since executed.  Subsequently, the fw
// attempts to talk to the LTC2943 which previously was unresponsive and uninitialized by fw.
// This causes the battery percentage to be reported as 93% when on USB (after batteries are removed again).
//
static int M_initialized = 0;
static int battery_accumulator_initialized = 0;
static uint32_t last_adc_trigger_time = 0;

float ltc2943_last_voltage_reported = 0;
float ltc2943_last_current_reported = 0;
float ltc2943_last_temperature_reported = 0;
uint16_t ltc2943_last_raw_acc_reported = 0;
int ltc2943_last_percentage_reported = 50;



#if 0
int convert_bin_to_uAh(int Q);
int convert_uAh_to_bin(int uAh);
int convert_bin_to_uAh(int Q) {
	return ((Q * (Qlsb / 10))) / 100;
}

int convert_uAh_to_bin(int uAh) {
	int Q;
	Q = (uAh * 100) / (Qlsb / 10);
	return (Q < LTC294X_MAX_VALUE) ? Q : LTC294X_MAX_VALUE;
}
#endif

static void reset_state_machine(void)
{
	M_initialized = 0;
	battery_accumulator_initialized = 0;
	last_adc_trigger_time = 0;
}


static void check_for_late_power_on(void)
{
	// The LTC2943 may turn on long after the MCU has been powered on.
	// This can be induced by starting with USB, then adding batteries.
	// When that happens, the pre-scaler (M) and accumulator register
	// must be corrected.  Since the amount of time the LTC2943 has been
	// running cannot be determined, the accumulator value must be discarded.
	// For one thing, the M prescaler is not correct for the sense resistor
	// and any count would necessarily be wrong.

	if (M_initialized == 0)
	{
		ltc2943_init_hw();
	}
	if (battery_accumulator_initialized == 0)
	{
		ltc2943_init_battery_accumulator(0);
	}
}



static int8_t ltc2943_write_8bit(uint8_t reg, uint8_t data)
{
	HAL_StatusTypeDef HAL_status;
	uint16_t dev_addr = (I2C_ADDRESS << 1);

	HAL_status = HAL_I2C_Mem_Write(&hi2c2, dev_addr, (uint16_t) reg, 1, &data, 1, LTC2943_TIMEOUT_MS);  // Note:  if an i2c error occurs, this timeout threshold is never reached

	if (HAL_status != HAL_OK)
	{
		reset_state_machine();
	}

    return HAL_status;
}

static int8_t ltc2943_write_16bit(uint8_t reg, uint16_t data)
{
	HAL_StatusTypeDef HAL_status;
	i2c_txBuff[0] = (uint8_t) (data >> 8);
	i2c_txBuff[1] = (uint8_t) (data);
	uint16_t tx_buff;

	tx_buff = (I2C_ADDRESS << 1);

    HAL_status = HAL_I2C_Mem_Write(&hi2c2, tx_buff, (uint16_t) reg, 1, i2c_txBuff, 2, LTC2943_TIMEOUT_MS);  // Note:  if an i2c error occurs, this timeout threshold is never reached

	if (HAL_status != HAL_OK)
	{
		reset_state_machine();
	}

    return HAL_status;
}


static int8_t ltc2943_read_8bit(uint8_t reg, uint8_t *value)
{
	HAL_StatusTypeDef HAL_status;

	uint16_t tx_buff;
	uint8_t rx_buff[2];

	tx_buff = (I2C_ADDRESS << 1);

    HAL_status = HAL_I2C_Mem_Read(&hi2c2, tx_buff, (uint16_t) reg, 1, rx_buff, 1, LTC2943_TIMEOUT_MS);  // Note:  if an i2c error occurs, this timeout threshold is never reached

    if (HAL_status == HAL_OK )
    {
        *value = (uint8_t) (rx_buff[0]);
    }
    else
    {
    	reset_state_machine();
    }

	return HAL_status;
}



static int16_t ltc2943_read_16bit(uint8_t reg, uint16_t *value)
{
	HAL_StatusTypeDef HAL_status;
	uint16_t tx_buff;
	uint8_t rx_buff[2];

	tx_buff = (I2C_ADDRESS << 1);

    HAL_status = HAL_I2C_Mem_Read(&hi2c2, tx_buff, (uint16_t) reg, 1, rx_buff, 2, LTC2943_TIMEOUT_MS);   // Note:  if an i2c error occurs, this timeout threshold is never reached

    if (HAL_status == HAL_OK )
    {
    	*value = (uint16_t) ((rx_buff[0] << 8) | rx_buff[1]);
    }
    else
    {
    	reset_state_machine();
    }

	return HAL_status;
}

int16_t ltc2943_trigger_adc(void) // Datasheet says all three conversions are started: voltage (33 ms), current (4.5 ms), temperature (4.5 ms) and then ADC_Mode is set to 0
{
	HAL_StatusTypeDef HAL_status;

	HAL_status = ltc2943_read_8bit(LTC294X_REG_CONTROL, &ltc2943_Regs.reg_cntl);
	if (HAL_status != HAL_OK) return HAL_status;

	ltc2943_Regs.REG_CNTL.ADC_Mode = 1;

	HAL_status =ltc2943_write_8bit(LTC294X_REG_CONTROL, ltc2943_Regs.reg_cntl);
	if (HAL_status != HAL_OK) return HAL_status;

	last_adc_trigger_time = rtc_get_time();

	return HAL_status;
}



//=================  HIGH LEVEL FUNCTIONS BELOW  ============================================================





void  ltc2943_init_hw(void)  // do this to as close to power on as possible to set pre-scaler (M) correctly for sense resistor on the board
{
	int status;
	uint16_t acc;
	char msg[80];


    // Detect if we are here due to a true power-on event, or if the MCU was reset by some other means.
	// Note that an extreme case is to power on with batteries, add USB power, then remove the batteries.
	// In that case the LTC2943 is still working and the voltage reading can be used to detect if batteries are present.  See light_sleep() in main.c
	// In the more usual case of USB power, the i2c will report an error as nothing is powering on the LTC2943.
	// In that case, it obviously cannot be initialized.  However, the rest of the system needs to act as if things are still working.

	status = ltc2943_read_8bit(LTC294X_REG_STATUS, &ltc2943_Regs.reg_status);   // Expect: 0000 0001  = 0x01  (if not cleared by a previous read)
	if (status != HAL_OK)
	{
		// The LTC2943 is powered from the battery.  If there is no battery, i2c will report an error. But, that is correct operation for USB power.
		AppendStatusMessage("LTC2943 error 1.");
		return;
	}

	status = ltc2943_read_8bit(LTC294X_REG_CONTROL, &ltc2943_Regs.reg_cntl);   // Expect: 0011 1100  = 0x3C
	if (status != HAL_OK)
	{
		// The LTC2943 is powered from the battery.  If there is no battery, i2c will report an error. But, that is correct operation for USB power.
		AppendStatusMessage("LTC2943 error 2.");
		return;
	}

	status = ltc2943_read_16bit(LTC294X_REG_ACC_CHARGE_MSB, &acc);  // Expect: 0x7FFF, HOWEVER power on occurred some time in the past and the big capacitor was charged, so 0x7FFe is more likely
	if (status != HAL_OK)
	{
		// The LTC2943 is powered from the battery.  If there is no battery, i2c will report an error. But, that is correct operation for USB power.
		AppendStatusMessage("LTC2943 error 3.");
		return;
	}

	sprintf(msg,"LTC2943 status=0x%x  cntl=0x%x  acc=0x%x", ltc2943_Regs.reg_status, ltc2943_Regs.reg_cntl, acc);
	AppendStatusMessage(msg);

	// Perform logic analysis of contents of status registers w.r.t expected power-on values

	if ((ltc2943_Regs.reg_status == 0x00) && (ltc2943_Regs.reg_cntl == 0x28))
	{
		// We are here because the MCU was reset but the LTC2943 was not (the status registered is cleared).
		// This might occur due to a wake up interrupt or processor brown-out, perhaps due to a magnet wake up on older models.
		// The LTC2943 is still running and M does not need to be initialized.
		// Unclear how to treat the acc at this point, defer to other functions having more context.
		AppendStatusMessage("LTC2943 already running.");
		M_initialized = 1;
		return;
	}
	else
	{
		AppendStatusMessage("LTC2943 Power on detected. Initializing.");
	}


	ltc2943_Regs.REG_CNTL.Shutdown    = 0;  // Power on default is zero
	ltc2943_Regs.REG_CNTL.ALCC        = 0;  // Power on default is b10=Alert Mode; 00 = ALCC pin disabled
	ltc2943_Regs.REG_CNTL.Prescaler_M = 5;  // 5 selects M = 1024 ; Power on default is b111 = 4096
	//ltc2943_Regs.REG_CNTL.Prescaler_M = 7;  // 7 selects M = 4096 ; Power on default is b111 = 4096  USED FOR SENSITIVE POWER MEASUREMENTS
	ltc2943_Regs.REG_CNTL.ADC_Mode    = 0;  // Power on default is 0

	status = ltc2943_write_8bit(LTC294X_REG_CONTROL, ltc2943_Regs.reg_cntl);  // 0x28
	if (status != HAL_OK)
	{
		// Not much can be done at this point if the hardware is not working.
		AppendStatusMessage("LTC2943 Error 4.");
	}
	else
	{
		// Since we have no way of knowing how long the LTC2943 has been running with the wrong M value, set the accumulator to mid-scale
		if (ltc2943_set_raw_accumulator(0x7FFF) != HAL_OK)
		{
			// Not much can be done at this point if the hardware is not working.
			AppendStatusMessage("LTC2943 Error 5.");
		}
	}

	M_initialized = 1;
}

void ltc2943_init_battery_accumulator(int From_MCU_Power_On)  // Uses global LogTrack.accumulator
{
	uint16_t consumed_since_power_on;
	uint16_t last_saved_value;
	uint16_t acc;
	float accumulator;
	float new_log_track_acc;
	int status;
	char msg[80];

	// If already initialized from some previous power-on event, do not re-initialize.
	if (battery_accumulator_initialized)  return;

    ltc2943_get_voltage(0);  // attempt to read a bunch of info from the chip (chip may not be online)

	// Check if batteries are physically present, if not, leave the raw accumulator alone.
	if (!ltc2943_are_batteries_present()) return;

	accumulator = LogTrack.accumulator;

	// Ignore impossible accumulator values.  Zero is allowed, but probably not meaningful.
	if (accumulator < 0)
	{
		AppendStatusMessage("LTC2943 error 0.");
        return;
	}

	last_saved_value = (uint16_t) (accumulator / Qlsb);  // Note:  this may be zero


	// We are here because power was indeed reset.  Must initialize the LTC2943 accumulator.
	// Find out how much was consumed since power on, then subtract that amount
	// from the last saved value to arrive at a new estimate.

	status = ltc2943_read_16bit(LTC294X_REG_ACC_CHARGE_MSB, &acc);  // Expect: 0x7FFF, HOWEVER power on occurred some time in the past and the big capacitor was charged, so 0x7FFe is more likely
	if (status != HAL_OK)
	{
		// Not much can be done if there is a hardware error here
		AppendStatusMessage("LTC2943 error 3.");
		return;
	}

	// Special case:  the ltc2943 is running, but batteries were removed.  Then, batteries are re-inserted.
	// In this case, since there is no way of knowing how long the ltc2943 was running with batteries removed,
	// must discard the acc and start as-if power is turning on for the first time (mid-scale acc).

	if (!From_MCU_Power_On) acc = 0x7FFF;

	consumed_since_power_on = 0x7FFF - acc;   // acc is usually 0x7FFe at this point due to charging up the large capacitor at power-on.

	if (last_saved_value > consumed_since_power_on)
	{
		last_saved_value = last_saved_value - consumed_since_power_on;

		if (ltc2943_set_raw_accumulator(last_saved_value) != HAL_OK)
		{
			// Hmmm....not sure what to do if this fails here.  Go ahead and update log track with new estimate
		}
	}
	else
	{
        // Hmmm...not sure what to do here. Just use the last_saved_value as is ?  It must be a pretty small value.
	}

	new_log_track_acc = last_saved_value * Qlsb;

	if (LogTrack.accumulator > new_log_track_acc)
	{
	  LogTrack.accumulator = new_log_track_acc;
	  BSP_Log_Track_Write(0);  // Do not read the LTC2943, use the new value.
	}

	battery_accumulator_initialized = 1;

	sprintf(msg, "LTC2943 acc initialized to last saved battery value 0x%x.", last_saved_value);

	AppendStatusMessage(msg);
}

int ltc2943_get_battery_percentage(void)
{

    ltc2943_get_voltage(0);  // attempt to read a bunch of info from the chip (chip may not be online)

	return ltc2943_last_percentage_reported;
}

void ltc2943_set_battery_percentage(int percentage)
{
	float acc;

	if (percentage > 100) percentage = 100;
	if (percentage < 0) percentage = 0;

	if (percentage == 100)
	{
		acc = MAX_BATTERY;
	}
	else
	{
		acc = MAX_BATTERY * ((float)percentage/100.0);
	}

	ltc2943_set_battery_accumulator(acc);

	MainConfig.val = acc;
	MainConfig.battery_reset_datetime = rtc_get_time();

	saveMainConfig = 1;
}

int ltc2943_set_raw_accumulator(uint16_t data)
{
	HAL_StatusTypeDef HAL_status;

	ltc2943_Regs.REG_CNTL.Shutdown = 1;
	HAL_status = ltc2943_write_8bit(LTC294X_REG_CONTROL, ltc2943_Regs.reg_cntl);
	if (HAL_status != HAL_OK) return HAL_status;

	HAL_status = ltc2943_write_16bit(LTC294X_REG_ACC_CHARGE_MSB, data);
	if (HAL_status != HAL_OK) return HAL_status;

	ltc2943_Regs.REG_CNTL.Shutdown = 0;
	HAL_status = ltc2943_write_8bit(LTC294X_REG_CONTROL, ltc2943_Regs.reg_cntl);

	return HAL_status;
}

uint16_t ltc2943_get_raw_accumulator(void)
{
	uint16_t value;
	uint16_t stat;

	stat = ltc2943_read_16bit(LTC294X_REG_ACC_CHARGE_MSB, &value);

	// The LTC2943 is powered from the battery.  If there is no battery, i2c will report an error.
	// However, ltc2943 can be powered up on batteries, USB power added, and then the batteries removed.
	// In that case the raw acc can still be read, but it is no longer representative of the batteries.
	// Also, the batteries could be re-inserted later.  In that case, a polling mechanism needs to detect
	// when batteries are removed/inserted so the acc can be synchronized with the battery LogTrack.accumulator.
	// That logic is done elsewhere.  Consequently, any value returned by this function needs to be interpreted
	// in context.
	if (stat != HAL_OK)
	{
		value = 0;  // For diagnostics, report zero for raw acc if ltc2943 is not powered on
	}

	ltc2943_last_raw_acc_reported = value;

	return ltc2943_last_raw_acc_reported;  // this value may or may not reflect battery measurements.  It depends on if batteries are still there, and if the ltc2943 was re-initialized.

}

float ltc2943_get_mAh(uint16_t raw_accumulator)
{
	return ((float)raw_accumulator) * Qlsb;
}

float ltc2943_get_accumulator(void)
{
	float ret;
	uint16_t raw_acc;
	uint16_t stat;

	// The LTC2943 is powered from the battery.  If there is no battery, i2c will report an error. But, that is correct operation for USB power.
	// In the past, the battery was always reported as 50% when on USB power.
	stat = ltc2943_read_16bit(LTC294X_REG_ACC_CHARGE_MSB, &raw_acc);

	if (stat == HAL_OK)
	{
		ret = raw_acc * Qlsb;
	}
	else
	{
		ret = MAX_BATTERY * 0.5;
	}

	return ret;
}

int ltc2943_update_log_track(void)  // updates LogTrack.accumulator with new mAhr if batteries are present and ltc2943 contains a meaningful number
{
	int result = 0;

    ltc2943_get_voltage(0);  // attempt to read a bunch of info from the chip (chip may not be online)

    if (ltc2943_are_batteries_present())
    {

    	float mAhr;

    	mAhr = ltc2943_last_raw_acc_reported * Qlsb;

    	if (LogTrack.accumulator > mAhr )
    	{
    		LogTrack.accumulator = mAhr;
    		result = 1;
    	}

    }
	return result;
}

void ltc2943_update_percentage(void)
{
	if (ltc2943_last_voltage_reported > 5.6)
	{
		ltc2943_last_percentage_reported = LogTrack.accumulator / MAX_BATTERY * 100;
	}
	else
	{
		ltc2943_last_percentage_reported = 50;  // report 50% if running on external power
	}
}

int ltc2943_set_battery_accumulator(float new_battery_val)
{
	unsigned short data;

	LogTrack.accumulator = new_battery_val;
	BSP_Log_Track_Write(0);  // Do not read the LTC2943, use the forced new_battery_val value.

	// Attempt to update the LTC2943 itself.  Note, this may have no effect if LTC2943 is not running.
	// Also note that if it is running, but the batteries are not there (battery voltage is below threshold) then
	// the actual contents of the LTC2943 accumulator are not truly representative of the battery drain and will not be used
	// to update the battery accumulator at the sync points.

	data = (unsigned short) (new_battery_val / Qlsb);

	return ltc2943_set_raw_accumulator(data);
}


/* NOTES FROM light_sleep() development 02-28-2023 zg
 * 1. The LTC2943 does not power on when USB power is applied.  Don't know why.  I2C reports HAL_ERROR.
 * 2. When powering on from battery or bench supply, chip is initialized.  Subsequently adding USB external power and then
 *    removing the battery keeps the chip talking at all times.
 * 3. Power on with battery, wait until deep sleep is entered, then remove batteries.  Chip is held up by something, possibly capacitance or I2C lines back-feeding.
 *    In any case, the chip talks just fine.  Can use that fact to trigger voltage conversions and monitor if voltage droops. See functionn light_sleep().
 *
 *
 */

float ltc2943_read_temperature_register(void) // returns Deg C
{
	uint16_t value;
	value = 0;
	ltc2943_read_16bit(LTC294X_REG_TEMPERATURE_MSB, &value);
	ltc2943_last_temperature_reported =  ((510.0 * (float) value / 65535.0) - 273.0);
	return ltc2943_last_temperature_reported;  // Deg C
}


float ltc2943_read_current_register(void)  // returns mA
{
	HAL_StatusTypeDef HAL_status;
	uint16_t val;
	float value;

	HAL_status = ltc2943_read_16bit(LTC294X_REG_CURRENT_MSB, &val);
	if (HAL_status == HAL_OK)
	{

		//val = 0xA840;  // Test value from datasheet, should report approximately 377.3 mA

		value = val;

		value = value - 32767.0;

		ltc2943_last_current_reported = 1000.0 * ((60.0 / 10.0) * (value / 32767.0));  // 10 mOhm sense resistor (iTracker5)

		//ltc2943_last_current_reported = 1000.0 * ((60.0 / 100.0) * (value / 32767.0));  // 100 mOhm sense resistor (SewerWatch SN3 prototype)

		//ltc2943_last_current_reported = 1000.0 * ((60.0 / 50.0) * (value / 32767.0));  // Datasheet example: 50 mOhm sense resistor  should report 377.3 mA

	}
	else
	{
		ltc2943_last_current_reported = 0;  // For diagnostics, report zero
	}

	return ltc2943_last_current_reported;
}


static float ltc2943_read_voltage_register(void)  // returns V
{
	uint16_t value = 0;

	ltc2943_read_16bit(LTC294X_REG_VOLTAGE_MSB, &value);

	ltc2943_last_voltage_reported = ((value * 23.6) / 0xFFFF);

	return ltc2943_last_voltage_reported;
}



float ltc2943_get_voltage(int BlinkLed)  // This will return zero volts if the ltc2943 is not on-line
{
	HAL_StatusTypeDef HAL_status;

	if (BlinkLed)
	{
		if (ltc2943_last_percentage_reported < 5)   // note: this value can be out-of-date but is the best estimate available
		{
			STATUS_LED_OFF;  // red off
			STATUS2_LED_ON;  // yellow on
		}
		else
		{
			STATUS_LED_ON;    // red on
			STATUS2_LED_OFF;  // yellow off
		}
	}

	HAL_status = ltc2943_trigger_adc();
	if (HAL_status == HAL_OK)
	{
	  HAL_Delay(64); // Datasheet says adc conversions take max of 48 + 8 +  8 = 64 ms to complete

	  ltc2943_read_voltage_register();
	  ltc2943_read_current_register();
	  ltc2943_read_temperature_register();
	  ltc2943_get_raw_accumulator();
	}
	else
	{
		ltc2943_last_voltage_reported = 0;
		ltc2943_last_current_reported = 0;  // For diagnostics, report zero
		ltc2943_last_temperature_reported = 0;
		ltc2943_last_raw_acc_reported = 0;
	}

	ltc2943_update_percentage();

	// About 70 ms has expired, turn off leds, creating a short blink that overlaps with adc
	if (BlinkLed)
	{
		STATUS_LED_OFF;  // red off
		STATUS2_LED_OFF;  // yellow off
	}

	return ltc2943_last_voltage_reported;
}

void ltc2943_report_measurements(const char *label)
{
	char msg[80];

	int dec1, dec2, dec3, dec4, dec5, dec6;

	convfloatstr_4dec(ltc2943_last_voltage_reported, &dec1, &dec2);
	convfloatstr_1dec(ltc2943_last_current_reported, &dec3, &dec4);
	convfloatstr_1dec(ltc2943_last_temperature_reported, &dec5, &dec6);

	sprintf(msg,"%s, %d.%04d, v, %d.%01d, mA, %d.%01d, C, %u, acc",
			label,
			dec1, dec2,
			dec3, dec4,
			dec5, dec6,
			ltc2943_last_raw_acc_reported);

	//AppendStatusMessage(msg);  // how long does this take vs using trace_ ?? timestampe is nice
	trace_TimestampMsg(msg);
}

void ltc2943_trace_measurements(const char *label)  // this can take 64 ms to complete
{
	if (sd_TraceVoltageEnabled)
	{
		// Perform a measurement and extract all register values
		ltc2943_get_voltage(0);

		ltc2943_report_measurements(label);
	}
}



void ltc2943_correct_for_sleep_current(uint16_t raw_accumulator_before_sleep, uint32_t seconds_asleep)
{
	static float estimated_energy_used_during_sleep = 0.0;
	uint16_t new_accumulator_value;

	// Accumulate the estimated_energy_used_during_sleep until it is larger than one,
	// then make the adjustment to the raw accumulator and preserve anything less than one
	// for next time.

	estimated_energy_used_during_sleep += ((float)seconds_asleep) * LTC2943_mAH_per_second_of_sleep;

	if (estimated_energy_used_during_sleep >= 1.0)
	{
		new_accumulator_value = raw_accumulator_before_sleep;
		while (estimated_energy_used_during_sleep >= 1.0)
		{
			estimated_energy_used_during_sleep -= 1.0;
			if (new_accumulator_value > 0)   // corner case, don't go below zero
			{
				new_accumulator_value = new_accumulator_value - 1;
			}
		}

		if (ltc2943_set_raw_accumulator(new_accumulator_value) != HAL_OK)
		{
			//Hmmm... not much to do if this error occurs

		}


#if 0
		// Unclear if the LogTrack should be updated here, or later during normal operation.  After all, we just woke up to do something, most likely take a data point.
		// Better to do this somewhere else.
		if (ltc2943_are_batteries_present())
		{
			float new_value;
			new_value = new_accumulator_value * Qlsb;
			if (LogTrack.accumulator > new_value)
			{
				// Save the updated value
				LogTrack.accumulator = new_value;
				BSP_Log_Track_Write(0);  // Do not read the LTC2943
			}
		}
#endif

	}
}



int ltc2943_are_batteries_present(void)
{

	// ASSERT:  ltc2943_get_voltage() has been called prior to this function


	// Tadrian and Saft Li batteries are 3.6 v each or 7.2 v for dual
	// Some "Bad" Tadrian batteries measured around 3.3 v.
	// Datasheet says 3.18v at ~100 mA at 25C. Very sharp knee at about 16 hrs
	// Datasheet also has temperature curve. Voltage drops with temperature...
	// Power supply chip has a minimum input of 3.1 V max of 17 V
	//
	// USB 1.0 5V +/-5%  4.75 to 5.25
	// USB 2.0           4.75 to 5.50
	// USB 2.0 low pwr   4.4  to 5.50
	// USB 3.x           same as 2.0 but can go up to 20V if negotiated

	// If start with batteries, then add USB, then remove batteries, the LTC2943 will continue to operate and report USB voltage levels.
	// If start with USB, then add batteries, the LTC2943 will come on and measure battery voltage levels.
	// If then remove batteries, the voltage will drop to USB levels.
	// In this case, the LTC2943 continues to operate and will report the USB voltage.
	// If on USB and never add batteries, the LTC2943 will report an I2C error.
	//
	// Minimum operating voltages:
	// 3.6  LTC2943
    // 3.8  iTracker5 and SewerWatch main power supplies
	// 4.4  minimum USB power
	//
	// If LTC2943 is operating assume batteries are still there, and were not removed.  If they were removed, then voltage drops to external power.
	// If LTC2943 is not operating assume external power.
	//
	// If LTC2943 is operating and v < 4.4  HARD STOP
	// If LTC2943 is operating and 4.4 < v < 5.6 assume external power ? or
	// If LTC2943 is operating and 5.6 < v < 6.6 assume bad batteries  ?
	// If LTC2943 is operating and 6.6 < v < x   assume good batteries

#if 0
	{
		char msg[80];
		sprintf(msg,"Battery voltage = %d", (int)ltc2943_last_voltage_reported);
		AppendStatusMessage(msg);
	}
#endif

	if (ltc2943_last_voltage_reported > 5.6)
	{
		return 1;
	}
	return 0;  // batteries not detected but the LTC2943 may still be running.
}

void ltc2943_timeslice(void)
{
	// Once per second, detect if batteries have been:
	// a) removed
	// b) inserted
	// Take appropriate action for each case.

	uint32_t duration;
	uint32_t now;

	now = rtc_get_time();

	if (now < last_adc_trigger_time)  // guard against the rtc going backwards in time
	{
		now = last_adc_trigger_time + 1;  // force a measurement
	}

	duration = now - last_adc_trigger_time;

	if (duration < 1) return;

    ltc2943_get_voltage(0);  // attempt to read a bunch of info from the chip (chip may not be online)

	if (ltc2943_are_batteries_present())  // not that in the case of an i2c error, both M_initialized and battery_accumulator_initialized are set to zero.
	{
		check_for_late_power_on();
		//check_for_weak_batteries();  // Perform a "hardstop" here when batteries go below a certain voltage, or when battery percent is below a threshold
	}
	else
	{
		// Batteries are no longer present,  but the LTC2943 may still be running and reporting a voltage.
		// If so, the M value is ok, but the accumulator can no longer
		// be used to track battery drain and must be re-initialized if batteries are added later.
		battery_accumulator_initialized = 0;
	}

	ltc2943_Enforce_HARDSTOP_Voltage_Limit(0);  // do not read voltage again, use last value
}



void ltc2943_charge_main_capacitor(uint32_t delay_ms)
{

  float v_before;

  uint32_t adjusted_delay_ms;

  adjusted_delay_ms = delay_ms;

  if (sd_TraceVoltageEnabled)
  {
	  v_before = ltc2943_get_voltage(0);  // this takes 64 ms if the ltc2943 is online (about 0.29 Tau).  Zero if not online.
	  if (v_before)
	  {
		  adjusted_delay_ms = delay_ms - 128;
	  }
	  ltc2943_report_measurements("cap1");
  }

  HAL_led_delay(adjusted_delay_ms);

  if (sd_TraceVoltageEnabled)
  {
      ltc2943_get_voltage(0);  // this takes 64 ms if the ltc2943 is online (about 0.29 Tau)
      ltc2943_report_measurements("cap2");
  }

}

void ltc2943_Enforce_HARDSTOP_Voltage_Limit(int ReadVoltage)
{
	if (ReadVoltage) ltc2943_get_voltage(0);

	if (ltc2943_last_voltage_reported == 0) return; // LTC2943 is not available, cannot read a voltage.  Must be on external power.

	if ( ltc2943_last_voltage_reported < 4.4)
	{
		// HARDSTOP due to insufficient voltage
		led_hard_stop_2();
	}
}
