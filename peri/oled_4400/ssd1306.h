#ifndef SSD1306_H
#define SSD1306_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hal/i2c/i2c.h"

// ======================= DEFINITIONS =======================
#define SSD1306_HEIGHT      32
#define SSD1306_WIDTH       128
#define SSD1306_I2C_ADR     0x3C 


// ======================= TYPE DEFINITIONS =======================
/*! \brief Structure for defining the render are of the display.
* \param start_col      uint8_t value with start column definition
* \param end_col        uint8_t value with end column definition
* \param start_page     uint8_t value with start page definition
* \param end_page       uint8_t value with end page definition
*/ 
typedef struct {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
} render_area_t;


/*! \brief Structure for configuring SSD1306 (Adafruit 4400) OLED-display.
* \param i2c_mod       Pointer to I2C module configuration struct
* \param render_area   Pointer to render area struct defining the area of the display to be updated on render calls. If NULL, the entire display will be updated.
* \param init_done     Initialization done flag
 */
typedef struct {
    i2c_rp2_t *i2c_mod;
    render_area_t *render_area;
    bool init_done;
} ssd1306_t;


// ======================= FUNCTION PROTOTYPES =======================
/*! \brief Initialize the SSD1306 display.
 * \param config Pointer to the SSD1306 configuration structure.
 * \return true if initialization is successful, false otherwise.
 */
bool ssd1306_init(ssd1306_t* config);


/*! \brief Calculate the buffer length required for rendering a specific area.
 * \param area Pointer to the render area structure.
 * \return The required buffer length.
 */
uint16_t ssd1306_calculate_render_arena_buflen(render_area_t* area);


/*! \brief Send a command to the SSD1306 display.
 * \param config Pointer to the SSD1306 configuration structure.
 * \param cmd The command to send.
 */
void ssd1306_send_cmd(ssd1306_t* config, uint8_t cmd);


/*! \brief Send a list of commands to the SSD1306 display.
 * \param config Pointer to the SSD1306 configuration structure.
 * \param buf Pointer to the buffer containing the commands.
 * \param num The number of commands to send.
 */
void ssd1306_send_cmd_list(ssd1306_t* config, uint8_t* buf, int num);


/*! \brief Send a buffer of data to the SSD1306 display.
 * \param config Pointer to the SSD1306 configuration structure.
 * \param buf Pointer to the buffer containing the data to send.
 * \param buflen The length of the buffer.
 */
void ssd1306_send_buffer(ssd1306_t* config, uint8_t buf[], int buflen);


/*! \brief Render the display.
 * \param config Pointer to the SSD1306 configuration structure.
 * \param buf Pointer to the buffer containing the data to render.
 * \param area Pointer to the render area structure.
 */
void ssd1306_render(ssd1306_t* config, uint8_t* buf, render_area_t* area);


/*! \brief Set the scroll mode of the SSD1306 display.
 * \param config Pointer to the SSD1306 configuration structure.
 * \param on The scroll mode (true for on, false for off).
 */
void ssd1306_set_scroll(ssd1306_t* config, bool on);


/*! \brief Set a pixel on the SSD1306 display.
 * \param buf Pointer to the buffer containing the display data.
 * \param x The x-coordinate of the pixel.
 * \param y The y-coordinate of the pixel.
 * \param on The pixel state (true for on, false for off).
 */
void ssd1306_set_pixel(uint8_t* buf, int x, int y, bool on);


/*! \brief Draw a line on the SSD1306 display.
 * \param buf Pointer to the buffer containing the display data.
 * \param x0 The x-coordinate of the starting point.
 * \param y0 The y-coordinate of the starting point.
 * \param x1 The x-coordinate of the ending point.
 * \param y1 The y-coordinate of the ending point.
 * \param on The line state (true for on, false for off).
 */
void ssd1306_draw_line(uint8_t* buf, int x0, int y0, int x1, int y1, bool on);


/*! \brief Write a character on the SSD1306 display.
 * \param buf Pointer to the buffer containing the display data.
 * \param x The x-coordinate of the character position.
 * \param y The y-coordinate of the character position.
 * \param ch The character to write.
 */
void ssd1306_write_char(uint8_t* buf, int16_t x, int16_t y, uint8_t ch);


/*! \brief Write a string on the SSD1306 display.
 * \param buf Pointer to the buffer containing the display data.
 * \param x The x-coordinate of the string position.
 * \param y The y-coordinate of the string position.
 * \param str The string to write.
 */
void ssd1306_write_string(uint8_t* buf, int16_t x, int16_t y, char* str);


#endif
