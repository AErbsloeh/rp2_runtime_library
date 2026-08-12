#ifndef ADS1299_H_
#define ADS1299_H_


#include "hal/spi/spi.h"


// ---- PGA gain values for CHnSET (bits GAIN[2:0]) ----
#define ADS1299_GAIN_1   0x00
#define ADS1299_GAIN_2   0x01
#define ADS1299_GAIN_4   0x02
#define ADS1299_GAIN_6   0x03
#define ADS1299_GAIN_8   0x04
#define ADS1299_GAIN_12  0x05
#define ADS1299_GAIN_24  0x06   // default at power-up


// ---- Data rate values for CONFIG1 (bits DR[2:0]), with fMOD = fCLK / 4 ----
#define ADS1299_RATE_16KSPS  0x00
#define ADS1299_RATE_8KSPS   0x01
#define ADS1299_RATE_4KSPS   0x02
#define ADS1299_RATE_2KSPS   0x03
#define ADS1299_RATE_1KSPS   0x04
#define ADS1299_RATE_500SPS  0x05
#define ADS1299_RATE_250SPS  0x06   // default at power-up


/*! \brief Configuration struct for the ADS1299 8-channel biopotential ADC from Texas Instruments
* \param spi                       Pointer to SPI interface handler
* \param gpio_cs                   Chip-select GPIO pin (active low, required)
* \param gpio_drdy                 Data-ready GPIO pin (active low output of the ADS1299; 255 = unused, poll via RDATA instead)
* \param gpio_reset                Reset GPIO pin (active low; 255 = unused, RESET pin tied high in hardware / RESET command used instead)
* \param gpio_start                START GPIO pin (255 = unused, START/STOP command used instead)
* \param num_channels              Number of channels to read out (1-8; ADS1299 has 8)
* \param gain                      PGA gain applied to all channels, see ADS1299_GAIN_* defines
* \param data_rate                 Output data rate, see ADS1299_RATE_* defines
* \param enable_bias               Flag for enabling the BIAS drive buffer/derivation
* \param use_internal_reference    Flag for enabling the internal 4.5V reference buffer (false = external reference)
* \param rdatac_mode               Internal state flag: true while Read-Data-Continuous mode is active
* \param init_done                 Initialization done flag
*/
typedef struct {
    spi_rp2_t *spi;
    uint8_t gpio_cs;
    uint8_t gpio_drdy;
    uint8_t gpio_reset;
    uint8_t gpio_start;
    uint8_t num_channels;
    uint8_t gain;
    uint8_t data_rate;
    bool enable_bias;
    bool use_internal_reference;
    bool rdatac_mode;
    bool init_done;
} ads1299_t;


/*! \brief Wait after sending a command/register access before making a new SPI request to the ADS1299
* NOTE - ASSUMPTION: uses a fixed conservative delay; the datasheet specifies
* minimum wait times in tCLK cycles (dependent on fCLK) for several commands
* (e.g. >= 4 tCLK after SDATAC, >= 18 tCLK / ~4µs after RESET). Please verify
* against your fCLK for time-critical sequences.
*/
void ads1299_wait(void);


/*! \brief Send the WAKEUP command (exit standby mode)
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_wakeup(ads1299_t *config);


/*! \brief Send the STANDBY command (enter low-power standby mode)
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_standby(ads1299_t *config);


/*! \brief Send the RESET command (reset all registers to their power-up default)
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_reset(ads1299_t *config);


/*! \brief Send the START command (start/restart conversions); has no effect if gpio_start is used and driven externally
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_start(ads1299_t *config);


/*! \brief Send the STOP command (stop conversions)
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_stop(ads1299_t *config);


/*! \brief Enable Read Data Continuous mode (RDATAC). While active, only RDATAC-mode-compatible commands (SDATAC, STOP) are accepted by the device
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_rdatac_enable(ads1299_t *config);


/*! \brief Disable Read Data Continuous mode (SDATAC). Must be called before RREG/WREG or most other commands if RDATAC is active
* \param config             Pointer to the configuration struct
* \return true if the SPI transfer was successful
*/
bool ads1299_rdatac_disable(ads1299_t *config);


/*! \brief Read a single register
* \param config             Pointer to the configuration struct
* \param reg_addr           Register address (0x00-0x17)
* \return uint8_t with the register content
*/
uint8_t ads1299_read_register(ads1299_t *config, uint8_t reg_addr);


/*! \brief Read multiple consecutive registers
* \param config             Pointer to the configuration struct
* \param reg_addr_start     First register address to read
* \param data               Pointer to a buffer that receives the register contents
* \param num_registers      Number of registers to read
* \return true if the SPI transfer was successful
*/
bool ads1299_read_registers(ads1299_t *config, uint8_t reg_addr_start, uint8_t *data, uint8_t num_registers);


/*! \brief Write a single register
* \param config             Pointer to the configuration struct
* \param reg_addr           Register address (0x00-0x17)
* \param value              Value to write
* \return true if the SPI transfer was successful
*/
bool ads1299_write_register(ads1299_t *config, uint8_t reg_addr, uint8_t value);


/*! \brief Write multiple consecutive registers
* \param config             Pointer to the configuration struct
* \param reg_addr_start     First register address to write
* \param data               Pointer to the values to write
* \param num_registers      Number of registers to write
* \return true if the SPI transfer was successful
*/
bool ads1299_write_registers(ads1299_t *config, uint8_t reg_addr_start, uint8_t *data, uint8_t num_registers);


/*! \brief Read the device ID register (address 0x00)
* NOTE - ASSUMPTION: no hard validation against a fixed expected value is
* performed, since the exact ID differs between ADS1299 variants (e.g. 4/6/8
* channel, ADS1299R). The commonly documented value for the 8-channel
* ADS1299 is 0x3E (bits [4:2] = DEV_ID, bits [1:0] = channel count code).
* Please verify against your part's datasheet and add a strict check if needed.
* \param config             Pointer to the configuration struct
* \param device_id          Pointer that receives the raw ID register content
* \return true if the SPI transfer was successful
*/
bool ads1299_read_device_id(ads1299_t *config, uint8_t *device_id);


/*! \brief Set the PGA gain for all channels (writes CHnSET for channel 0..num_channels-1)
* NOTE - ASSUMPTION: SRB2 switch is closed (bit3=1) and MUX is set to normal
* electrode input (bits[2:0]=000) for every channel; per-channel MUX/SRB2/
* power-down configuration is not exposed by this convenience function.
* \param config             Pointer to the configuration struct
* \param gain                PGA gain, see ADS1299_GAIN_* defines
* \return true if set successfully, false otherwise
*/
bool ads1299_set_gain(ads1299_t *config, uint8_t gain);


/*! \brief Set the output data rate (CONFIG1 register)
* NOTE - ASSUMPTION: reserved/fixed bits of CONFIG1 are written using the
* commonly documented power-up default pattern with only the DR[2:0] field
* replaced. Please verify against the datasheet if using daisy-chain or
* clock-output features.
* \param config             Pointer to the configuration struct
* \param data_rate           Data rate, see ADS1299_RATE_* defines
* \return true if set successfully, false otherwise
*/
bool ads1299_set_data_rate(ads1299_t *config, uint8_t data_rate);


/*! \brief Enable/disable the internal 4.5V reference buffer (CONFIG3 register)
* NOTE - ASSUMPTION: reserved/fixed bits of CONFIG3 are written using the
* commonly documented power-up default pattern (bit6 forced to 1 as
* required by the datasheet). Please verify.
* \param config                     Pointer to the configuration struct
* \param use_internal_reference     true = use internal reference, false = use external reference
* \return true if set successfully, false otherwise
*/
bool ads1299_set_reference(ads1299_t *config, bool use_internal_reference);


/*! \brief Enable/disable the BIAS drive buffer with internal bias reference (CONFIG3 register)
* NOTE - ASSUMPTION: see ads1299_set_reference; BIASREF_INT is always set to
* internal together with this flag. Please verify against the datasheet if
* an external bias reference is required.
* \param config             Pointer to the configuration struct
* \param enable_bias        true = enable BIAS buffer, false = disable
* \return true if set successfully, false otherwise
*/
bool ads1299_set_bias(ads1299_t *config, bool enable_bias);


/*! \brief Initialize the ADS1299 (GPIO setup, RESET, SDATAC, apply gain/data rate/reference/bias, ID readback)
* \param config             Pointer to the configuration struct
* \return true if initialization was successful, false otherwise
*/
bool ads1299_init(ads1299_t *config);


/*! \brief Wait for the DRDY pin to go low (new conversion result available)
* NOTE - ASSUMPTION: if gpio_drdy == 255 (unused), this function busy-waits a
* fixed worst-case delay derived from the configured data rate instead of
* polling a pin. For time-critical acquisition, wiring DRDY is recommended.
* \param config             Pointer to the configuration struct
* \param timeout_us         Maximum time to wait in microseconds
* \return true if a new sample is (assumed) ready, false on timeout
*/
bool ads1299_wait_for_data_ready(ads1299_t *config, uint32_t timeout_us);


/*! \brief Read one full data frame (status word + all channels) via the RDATA command
* NOTE: device must NOT be in RDATAC mode for RDATA to be accepted; call
* ads1299_rdatac_disable() first if needed.
* Frame layout: 3 status bytes, then 3 bytes (24-bit, two's complement) per
* channel in ascending channel order.
* Scaling to volts (float), per channel, with vref_volts = reference voltage
* in volts (4.5f for internal reference) and gain = numeric PGA gain (1,2,4,6,8,12,24):
*   voltage_V = (float)raw24_signed * (2.0f * vref_volts) / (gain * 16777216.0f)   // 16777216 = 2^24
* \param config             Pointer to the configuration struct
* \param status              Pointer that receives the 3 raw status bytes combined into the lower 24 bit of a uint32_t
* \param data                Pointer to a data array (length >= num_channels) that receives the sign-extended 24-bit raw channel values
* \return true if the SPI transfer was successful
*/
bool ads1299_read_data_all(ads1299_t *config, uint32_t *status, int32_t *data);


/*! \brief Read a single channel's raw value from the most recently fetched data frame
* Convenience wrapper around ads1299_read_data_all() that discards the other channels.
* Scaling to volts (float): see ads1299_read_data_all().
* \param config             Pointer to the configuration struct
* \param channel            Channel to read (0 .. num_channels-1)
* \return int32_t sign-extended 24-bit raw channel value
*/
int32_t ads1299_read_data_single(ads1299_t *config, uint8_t channel);


#endif