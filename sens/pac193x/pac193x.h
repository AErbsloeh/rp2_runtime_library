#ifndef PAC193X_H_
#define PAC193X_H_


#include "hal/i2c/i2c.h"


#define PAC193X_RATE_1024SPS    0
#define PAC193X_RATE_256SPS     1
#define PAC193X_RATE_64SPS      2
#define PAC193X_RATE_8SPS       3


// More Informations in the datasheet at https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/PAC1931-Family-Data-Sheet-DS20005850E.pdf
/*! \brief Struct for configuring the current sensor module PAC193x from Microchip Technology
* \param i2c                    Pointer to I2C interface
* \param gpio_pwrdwn            Power Down GPIO pin (not used = 255)
* \param gpio_alert             Alert GPIO pin (not used = 255)
* \param adr                    I2C address of the sensor
* \param num_channels           Number of channels to read (0: auto-detect, 1-4: selected)
* \param sample_rate            Integer for definign sampling rate (0=1024, 1=256, 2=64, 3=8 SPS)
* \param enable_channels        Flag for enabling channels (if false, all channels will be disabled, otherwise all channels will be enabled)
* \param enable_sleep_mode      Flag for enabling sleep mode (if true, the sensor will go to sleep mode after each measurement, otherwise it will stay in continuous measurement mode)
* \param enable_single_shot_mode Flag for enabling single shot mode (if true, the sensor will take a single measurement and then go back to sleep mode)
* \param enable_bipolar_voltage Flag for enabling bipolar voltage measurement (if true, the voltage output must be two complements, otherwise it is unsigned)
* \param enable_bipolar_current Flag for enabling bipolar current measurement (if true, the current output must be two complements, otherwise it is unsigned)
* \param init_done              Initialization done flag
*/
typedef struct {
    i2c_rp2_t *i2c;
    uint8_t gpio_pwrdwn;
    uint8_t gpio_alert;
    uint8_t adr;
    uint8_t num_channels;
    uint8_t sample_rate;
    bool enable_channels;
    bool enable_sleep_mode;
    bool enable_single_shot_mode;
    bool enable_bipolar_voltage;
    bool enable_bipolar_current;
    bool init_done;
} pac193x_t;

/*! \brief Function for getting the I2C address of the PAC193x sensor module (More details in datasheet [Table 5-1, p.26])
* \param resistor_value Value of the resistor connected to the ADR pin (0=GND, ..., 65535 = VDD)
* \return uint8_t with the I2C address (0x10 - 0x1F)
*/
uint8_t pac193x_get_i2c_address(uint32_t resistor_value);


/*! \brief Function for defiing the single shot mode of the PAC193x sensor module
* \param config                     Pointer to the configuration struct
* \param enable_single_shot_mode    Flag for enabling single shot mode (if true, the sensor will take a single measurement and then go back to sleep mode)
* \return true if set successfully, false otherwise
*/
bool pac193x_set_single_shot_mode(pac193x_t *config, bool enable_single_shot_mode);


/*! \brief Function for defining the sleep mode of the PAC193x sensor module
* \param config             Pointer to the configuration struct
* \param enable_sleep_mode  Flag for enabling sleep mode (if true, the sensor will go to sleep mode after each measurement, otherwise it will stay in continuous measurement mode)
* \return true if set successfully, false otherwise
*/
bool pac193x_set_sleep_mode(pac193x_t *config, bool enable_sleep_mode);


/*! \brief Function for defining the sampling rate of the PAC193x sensor module
* \param config         Pointer to the configuration struct
* \param sample_rate    Integer for definign sampling rate (0=1024, 1=256, 2=64, 3=8 SPS)
* \return true if set successfully, false otherwise
*/
bool pac193x_set_sampling_rate(pac193x_t *config, uint8_t sample_rate);


/*! \brief Function for checking the revision ID of the PAC193x sensor module
* \param config             Pointer to the configuration struct
* \param enable_channels    Boolean for enabling all channels (if true, all channels are enabled, otherwise they are disabled)
* \return true if set successfully, false otherwise
*/
bool pac193x_enable_all_channels(pac193x_t *config, bool enable_channels);


/*! \brief Function for setting the signal polarity of the voltage measurement using PAC193x sensor module
* \param config                 Pointer to the configuration struct
* \param use_bipolar_voltage    Boolean for setting the polarity for measurement the bus voltage (true = bipolar [-32 ... + 32 V], false = unipolar [0 ... 32 V])
* \param use_bipolar_current    Boolean for setting the polarity for measurement the bus current (true = bipolar [-32 ... + 32 V], false = unipolar [0 ... 32 V])
* \return true if set successfully, false otherwise
*/
bool pac193x_set_polarity(pac193x_t *config, bool use_bipolar_current, bool use_bipolar_voltage);


/*! \brief Initialize the PAC193x sensor module
* \param config             Pointer to the configuration struct
* \return true if initialization was successful, false otherwise
*/
bool pac193x_init(pac193x_t *config);


/*! \brief Function for triggering single conversion event and updating data registers (also in continuous process) using the PAC193x sensor module
* \param config             Pointer to the configuration struct
* \return true if initialization was successful, false otherwise
*/
bool pac193x_do_conversion(pac193x_t *config);


/*! \brief Reading the actual voltage sample at selected channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param take_rolling       Boolean if values should rolling mean of the 8 last samples or direct
* \param channel            Channel to read (0-3)
* \return uint16_t with voltage reading, scaling factor (unipolar = +32 V / 2 ** 16 bit = 488.28 µV/LSB, bipolar = +/- 32 V / 2 ** 15 bit = 976.563 µV/LSB)
*/
uint16_t pac193x_read_voltage_single(pac193x_t *config, bool take_rolling, uint8_t channel);


/*! \brief Reading the voltage sample at all channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param take_rolling       Boolean if values should rolling mean of the 8 last samples or only actual sample
* \param channel            Pointer to data array to feed-in the results, with scaling factor (unipolar = +32 V / 2 ** 16 bit = 488.28 µV/LSB, bipolar = +/- 32 V / 2 ** 15 bit = 976.563 µV/LSB)
* \return                   Boolean if data is valid
*/
bool pac193x_read_voltage_all(pac193x_t *config, bool take_rolling, uint16_t *data);


/*! \brief Reading the current flow sample of selected channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param take_rolling       Boolean if values should rolling mean of the 8 last samples or only actual sample
* \param channel            Channel to read (0-3)
* \return uint16_t with voltage read-out, scaling factor (unipolar = +100 mV / 2 ** 16 bit / R_sh = 1.53 µV/(LSB * R_sh), bipolar = +/- 100 mV / 2 ** 15 bit = 3.06 µV/(LSB * R_sh))
*/
uint16_t pac193x_read_current_single(pac193x_t *config, bool take_rolling, uint8_t channel);


/*! \brief Reading the current flow sample at all channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param take_rolling       Boolean if values should rolling mean of the 8 last samples or only actual sample
* \param channel            Pointer to data array to feed-in the results, with scaling factor (unipolar = +100 mV / 2 ** 16 bit / R_sh = 1.53 µV/(LSB * R_sh), bipolar = +/- 100 mV / 2 ** 15 bit = 3.06 µV/(LSB * R_sh))
* \return                   Boolean if data is valid
*/
bool pac193x_read_current_all(pac193x_t *config, bool take_rolling, uint16_t *data);


/*! \brief Reading the actual power sample of selected channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param channel            Channel to read (0-3)
* \return uint32_t with digital power read-out, scaling factor (unipolar = +3.2 V^2 / 2 ** 32 bit / R_sh = 0.74664 nV^2/(LSB * R_sh), bipolar = +/- 3.2 V^2 / 2 ** 31 bit / R_sh = 1.49328 nV^2/(LSB * R_sh))
*/
uint32_t pac193x_read_power_single(pac193x_t *config, uint8_t channel);


/*! \brief Reading the power sample at all channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param data               Pointer to data array to feed-in the results, with scaling factor (unipolar = +3.2 V^2 / 2 ** 32 bit / R_sh = 0.74664 nV^2/(LSB * R_sh), bipolar = +/- 3.2 V^2 / 2 ** 31 bit / R_sh = 1.49328 nV^2/(LSB * R_sh))
* \return                   Boolean if data is valid
*/
bool pac193x_read_power_all(pac193x_t *config, uint32_t *data);


/*! \brief Reading the actual power consumption (accumulated) of selected channel using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \param channel            Channel to read (0-3)
* \return uint48_t with running/ accumulate digital value from power measurement (need scaling and divison with numbers)
*/
uint64_t pac193x_read_power_accumulated_single(pac193x_t *config, uint8_t channel);


/*! \brief Reading the accumulation number from conversion using PAC193x current sensor module
* \param config             Pointer to the configuration struct
* \return uint24_t with number of accumulation numbers between two refresh requests
*/
uint32_t pac193x_read_accumulation_number(pac193x_t *config);


#endif
