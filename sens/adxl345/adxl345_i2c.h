#ifndef ADXL345_I2C_H_
#define ADXL345_I2C_H_


#include "hal/i2c/i2c.h"


// Handler for configuring and controlling the device configuration
typedef struct {
    i2c_rp2_t *i2c_mod;
    bool    init_done;
    // Variabes for calibrating sensor
    float offset_x;
    float offset_y;
    float offset_z;
} adxl345_i2c_rp2_t;



// More Informations from sensor: https://www.analog.com/media/en/technical-documentation/data-sheets/adxl345.pdf
static adxl345_i2c_rp2_t ADXL345_I2C_DEFAULT_CONFIG = {
    .i2c_mod = &DEVICE_I2C_DEFAULT,
    .init_done = false,
	.offset_x = 0.0,
	.offset_y = 0.0,
	.offset_z = 0.0    
};


// --------------- CALLABLE FUNCTIONS ---------------
/*! \brief Function for initialising the acceleration sensor ADXL345 from Analog Devices
*   \param  handler     Handler for setting the sensor
*   \return             Boolean if sensor is available
*/
bool ADXL345_init(adxl345_i2c_rp2_t *handler);


/*! \brief Function for reading the Chip ID of the acceleration sensor ADXL345
*   \param  handler     Handler for setting the sensor
*   \return             Boolean if sensor is available
*/
bool ADXL345_i2c_read_id(adxl345_i2c_rp2_t *handler);


/*! \brief Function for getting the acceleration data from ADXL345
*   \param  handler     Handler for setting the sensor
*   \param  x           Pointer to store the X-Axis data
*   \param  y           Pointer to store the Y-Axis data
*   \param  z           Pointer to store the Z-Axis data
*   \return             Boolean if sensor is available
*/
int ADXL345_get_acceleration(adxl345_i2c_rp2_t *handler, float *x, float *y, float *z); 


/*! \brief Function for resetting the offset values in struct for ADXL345 from Analog Devices
*   \param  handler     Handler for setting the sensor
*   \return            	None
*/
void ADXL345_reset_offset(adxl345_i2c_rp2_t *handler);


/*! \brief Function for performing a sensor calibration with defining the offsets for ADXL345 from Analog Devices
*   \param  handler     Handler for setting the sensor
*   \return             Boolean if sensor calibration is done
*/
bool ADXL345_do_calibration(adxl345_i2c_rp2_t *handler);

#endif
