#ifndef PAC1954_H_
#define PAC1954_H_


#include "hal/i2c/i2c.h"


// ---- Sample Mode values for the CTRL register (4-bit field) ----
#define PAC1954_MODE_1024SPS_ADAPTIVE   0x00
#define PAC1954_MODE_256SPS_ADAPTIVE    0x01
#define PAC1954_MODE_64SPS_ADAPTIVE     0x02
#define PAC1954_MODE_8SPS_ADAPTIVE      0x03
#define PAC1954_MODE_1024SPS            0x04
#define PAC1954_MODE_256SPS             0x05
#define PAC1954_MODE_64SPS              0x06
#define PAC1954_MODE_8SPS               0x07
#define PAC1954_MODE_SINGLE_SHOT        0x08
#define PAC1954_MODE_SINGLE_SHOT_8X     0x09
#define PAC1954_MODE_FAST               0x0A
#define PAC1954_MODE_BURST              0x0B
#define PAC1954_MODE_SLEEP              0x0F


// ---- ACCUM_CONFIG values (2 bit per channel - NOTE: ASSUMPTION) ----
#define PAC1954_ACCUM_POWER   0x00
#define PAC1954_ACCUM_VSENSE  0x01
#define PAC1954_ACCUM_VBUS    0x02
#define PAC1954_ACCUM_NONE    0x03


/*! \brief Configuration struct for the PAC1954 current sensor module from Microchip Technology
* \param i2c                    Pointer to I2C interface
* \param gpio_pwrdwn            Power down GPIO pin (unused = 255)
* \param gpio_alert             Alert GPIO pin (unused = 255)
* \param adr                    I2C address of the sensor
* \param num_channels           Number of channels to read (0: auto-detect, 1-4: selected)
* \param sample_mode            4-bit sample mode value, see PAC1954_MODE_* defines
* \param enable_bipolar_voltage Flag for bipolar voltage measurement (all channels, NOTE: see ASSUMPTION in pac1954.c)
* \param enable_bipolar_current Flag for bipolar current measurement (all channels, NOTE: see ASSUMPTION in pac1954.c)
* \param accum_config           Raw value for the ACCUM_CONFIG register, see PAC1954_ACCUM_* defines (NOTE: see ASSUMPTION in pac1954.c)
* \param init_done              Initialization done flag
*/
typedef struct {
    i2c_rp2_t *i2c;
    uint8_t gpio_pwrdwn;
    uint8_t gpio_alert;
    uint8_t adr;
    uint8_t num_channels;
    uint8_t sample_mode;
    bool enable_bipolar_voltage;
    bool enable_bipolar_current;
    uint8_t accum_config;
    bool init_done;
} pac1954_t;


/*! \brief Wait after sending a refresh command, before making a new request to the PAC1954 sensor
*/
void pac1954_wait(void);


/*! \brief Get the I2C address of the PAC1954 sensor module
* NOTE - ASSUMPTION: assumes the PAC195x uses the same address-select
* resistor table as the PAC193x (common scheme across the Microchip PAC19xx
* family). Please verify against the datasheet's address selection table.
* \param resistor_value Value of the resistor connected to the ADR pin (0=GND, ..., 65535 = VDD)
* \return uint8_t with the I2C address (0x10 - 0x1F)
*/
uint8_t pac1954_get_i2c_address(uint32_t resistor_value);


/*! \brief Set the sample mode (rate/adaptive/single-shot/fast/burst/sleep) in the CTRL register
* \param config         Pointer to the configuration struct
* \param sample_mode    4-bit value, see PAC1954_MODE_* defines
* \return true if set successfully, false otherwise
*/
bool pac1954_set_sample_mode(pac1954_t *config, uint8_t sample_mode);


/*! \brief Set the accumulator configuration (ACCUM_CONFIG register)
* NOTE - ASSUMPTION: bit packing (2 bit per channel) not conclusively verified.
* \param config         Pointer to the configuration struct
* \param accum_config   Raw value, see PAC1954_ACCUM_* defines (applied to all channels equally)
* \return true if set successfully, false otherwise
*/
bool pac1954_set_accum_config(pac1954_t *config, uint8_t accum_config);


/*! \brief Set the signal polarity (NEG_PWR_FSR register) for voltage/current measurement
* NOTE - ASSUMPTION: bit position within the 16-bit register is modeled on the
* PAC193x's 1-byte POLARITY register (low byte: bit[3:0] = bipolar voltage,
* bit[7:4] = bipolar current, applied to all channels), high byte is written
* as 0 (no extended negative-FSR range used). Please verify.
* \param config                 Pointer to the configuration struct
* \param use_bipolar_current    true = bipolar [-100mV ... +100mV], false = unipolar [0 ... 100mV]
* \param use_bipolar_voltage    true = bipolar [-32V ... +32V], false = unipolar [0 ... 32V]
* \return true if set successfully, false otherwise
*/
bool pac1954_set_polarity(pac1954_t *config, bool use_bipolar_current, bool use_bipolar_voltage);


/*! \brief Check the product ID of the PAC1954 sensor module
* \param config             Pointer to the configuration struct
* \return true if the product ID matches the configured channel count
*/
bool pac1954_check_product_id(pac1954_t *config);


/*! \brief Check the manufacturer ID of the PAC1954 sensor module (NOTE - ASSUMPTION, see pac1954.c)
* \param config             Pointer to the configuration struct
* \return true if the manufacturer ID matches
*/
bool pac1954_check_manufacturer_id(pac1954_t *config);


/*! \brief Read the revision ID of the PAC1954 sensor module (no validation, expected value not verified)
* \param config             Pointer to the configuration struct
* \param revision_id        Pointer that receives the read revision ID
* \return true if the read was successful
*/
bool pac1954_read_revision_id(pac1954_t *config, uint8_t *revision_id);


/*! \brief Determine the number of channels from the product ID
* \param config             Pointer to the configuration struct
* \return Number of channels (1-4), 0 if the product ID is unknown
*/
uint8_t pac1954_get_number_of_channels(pac1954_t *config);


/*! \brief Initialize the PAC1954 sensor module
* \param config             Pointer to the configuration struct
* \return true if initialization was successful, false otherwise
*/
bool pac1954_init(pac1954_t *config);


/*! \brief Trigger a single conversion event and update the data registers (REFRESH, resets accumulator)
* \param config             Pointer to the configuration struct
* \return true if successful, false otherwise
*/
bool pac1954_do_conversion(pac1954_t *config);


/*! \brief Send REFRESH_V: updates only the voltage/current registers, without resetting the accumulator
* \param config             Pointer to the configuration struct
* \return true if successful, false otherwise
*/
bool pac1954_do_conversion_voltage_only(pac1954_t *config);


/*! \brief Send REFRESH_G: updates all registers, but without resetting the accumulator/ACC_COUNT
* \param config             Pointer to the configuration struct
* \return true if successful, false otherwise
*/
bool pac1954_do_conversion_no_reset(pac1954_t *config);


/*! \brief Read the current voltage sample of a selected channel
* NOTE - ASSUMPTION: scaling factor assumed identical to the PAC193x (same
* 32V VBUS full-scale range and 16-bit resolution). Does not account for the
* PAC195x's optional extended/programmable negative full-scale range, which
* this driver does not configure (see pac1954_set_polarity). Please verify.
* Scaling to volts (float):
*   unipolar: voltage_V = (float)raw * 488.28e-6f                    (488.28 µV/LSB, raw range 0 .. 65535)
*   bipolar:  voltage_V = (float)(int16_t)raw * 976.563e-6f          (976.563 µV/LSB, raw as two's complement, range -32768 .. 32767)
* \param config             Pointer to the configuration struct
* \param take_rolling       true = rolling mean of the last 8 samples, false = direct value
* \param channel            Channel to read (0-3)
* \return uint16_t with the raw voltage reading
*/
uint16_t pac1954_read_voltage_single(pac1954_t *config, bool take_rolling, uint8_t channel);


/*! \brief Read the voltage sample of all channels
* NOTE - ASSUMPTION: see scaling note in pac1954_read_voltage_single.
* Scaling to volts (float), per element of data[]:
*   unipolar: voltage_V = (float)data[idx] * 488.28e-6f
*   bipolar:  voltage_V = (float)(int16_t)data[idx] * 976.563e-6f
* \param config             Pointer to the configuration struct
* \param take_rolling       true = rolling mean, false = direct value
* \param data               Pointer to data array to receive the raw results
* \return true if the data is valid
*/
bool pac1954_read_voltage_all(pac1954_t *config, bool take_rolling, uint16_t *data);


/*! \brief Read the current flow sample of a selected channel
* NOTE - ASSUMPTION: scaling factor assumed identical to the PAC193x (same
* 100mV VSENSE full-scale range and 16-bit resolution). Does not account for
* the PAC195x's optional extended/programmable negative full-scale range,
* which this driver does not configure (see pac1954_set_polarity). Please verify.
* The raw VSENSE reading is a voltage across the shunt resistor and must be
* divided by R_sh (in Ohm) to get amperes.
* Scaling to amperes (float), with r_sh_ohm = shunt resistance in Ohm:
*   unipolar: current_A = ((float)raw * 1.53e-6f) / r_sh_ohm                   (1.53 µV/(LSB*Rsh), raw range 0 .. 65535)
*   bipolar:  current_A = ((float)(int16_t)raw * 3.06e-6f) / r_sh_ohm         (3.06 µV/(LSB*Rsh), raw as two's complement, range -32768 .. 32767)
* \param config             Pointer to the configuration struct
* \param take_rolling       true = rolling mean, false = direct value
* \param channel            Channel to read (0-3)
* \return uint16_t with the raw reading (needs scaling via Rsense, see above)
*/
uint16_t pac1954_read_current_single(pac1954_t *config, bool take_rolling, uint8_t channel);


/*! \brief Read the current flow sample of all channels
* NOTE - ASSUMPTION: see scaling note in pac1954_read_current_single.
* Scaling to amperes (float), per element of data[], with r_sh_ohm = shunt resistance in Ohm:
*   unipolar: current_A = ((float)data[idx] * 1.53e-6f) / r_sh_ohm
*   bipolar:  current_A = ((float)(int16_t)data[idx] * 3.06e-6f) / r_sh_ohm
* \param config             Pointer to the configuration struct
* \param take_rolling       true = rolling mean, false = direct value
* \param data               Pointer to data array to receive the raw results
* \return true if the data is valid
*/
bool pac1954_read_current_all(pac1954_t *config, bool take_rolling, uint16_t *data);


/*! \brief Read the current power sample of a selected channel
* NOTE - ASSUMPTION: scaling factor assumed identical to the PAC193x (same
* 32V * 100mV = 3.2 V^2 full-scale power range). Does not account for the
* PAC195x's optional extended/programmable negative full-scale range, which
* this driver does not configure (see pac1954_set_polarity). Please verify.
* raw holds a 28-bit value (bit 3:0 of the register are unused/always 0, see
* implementation). Divide by R_sh (in Ohm) to get watts.
* Scaling to watts (float), with r_sh_ohm = shunt resistance in Ohm:
*   unipolar: power_W = ((float)raw * 11.921e-9f) / r_sh_ohm                         (11.921 nV^2/(LSB*Rsh), raw range 0 .. 268435455 [28 bit])
*   bipolar:  power_W = ((float)(int32_t)(raw << 4) / 16 * 23.842e-9f) / r_sh_ohm     (23.842 nV^2/(LSB*Rsh), raw as 27-bit two's complement magnitude + sign, sign-extend bit 26 before scaling)
* \param config             Pointer to the configuration struct
* \param channel            Channel to read (0-3)
* \return uint32_t with the digital power reading (only 28 bit valid)
*/
uint32_t pac1954_read_power_single(pac1954_t *config, uint8_t channel);


/*! \brief Read the power sample of all channels
* NOTE - ASSUMPTION: see scaling note in pac1954_read_power_single.
* Scaling to watts (float), per element of data[], with r_sh_ohm = shunt resistance in Ohm:
*   unipolar: power_W = ((float)data[idx] * 11.921e-9f) / r_sh_ohm
*   bipolar:  power_W = (sign-extend data[idx] as 27-bit two's complement, then * 23.842e-9f) / r_sh_ohm
* \param config             Pointer to the configuration struct
* \param data               Pointer to data array to receive the raw results
* \return true if the data is valid
*/
bool pac1954_read_power_all(pac1954_t *config, uint32_t *data);


/*! \brief Read the accumulated power value (VACC, 56-bit) of a selected channel
* NOTE: on the PAC1954 the accumulator is 56-bit (7 byte) wide, not 48-bit
* (6 byte) as on the PAC193x.
* NOTE - ASSUMPTION: the accumulator sums the same 28-bit power LSB weight
* used in pac1954_read_power_single (assumed identical to the PAC193x), once
* per sample; it does not average or scale by time. Divide by the
* accumulation count (pac1954_read_accumulation_number) to get the average
* power, and by R_sh (in Ohm) to get watts. Please verify against the datasheet.
* Scaling to average watts (float), with acc_count = pac1954_read_accumulation_number(config)
* and r_sh_ohm = shunt resistance in Ohm:
*   unipolar: avg_power_W = ((float)raw * 11.921e-9f) / (r_sh_ohm * (float)acc_count)
*   bipolar:  avg_power_W = ((float)(int64_t)sign_extend_56_to_64(raw) * 23.842e-9f) / (r_sh_ohm * (float)acc_count)
* \param config             Pointer to the configuration struct
* \param channel            Channel to read (0-3)
* \return uint64_t with the accumulated raw value (only 56 bit valid)
*/
uint64_t pac1954_read_power_accumulated_single(pac1954_t *config, uint8_t channel);


/*! \brief Read the accumulated power value of all channels
* NOTE - ASSUMPTION: see scaling note in pac1954_read_power_accumulated_single.
* Scaling to average watts (float), per element of data[], with
* acc_count = pac1954_read_accumulation_number(config) and r_sh_ohm = shunt resistance in Ohm:
*   unipolar: avg_power_W = ((float)data[idx] * 11.921e-9f) / (r_sh_ohm * (float)acc_count)
*   bipolar:  avg_power_W = (sign-extend data[idx] as 56-bit two's complement, then * 23.842e-9f) / (r_sh_ohm * (float)acc_count)
* \param config             Pointer to the configuration struct
* \param data               Pointer to data array to receive the raw results (56 bit valid per channel)
* \return true if the data is valid
*/
bool pac1954_read_power_accumulated_all(pac1954_t *config, uint64_t *data);


/*! \brief Read the accumulation count (ACC_COUNT) since the last refresh
* \param config             Pointer to the configuration struct
* \return uint32_t with the number of accumulations
*/
uint32_t pac1954_read_accumulation_number(pac1954_t *config);


#endif