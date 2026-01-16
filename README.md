# pico_runtime_library
This repo contains source files for different sensor / devices which can be used in Pico/RP2xxx projects.
For example you can use the corresponding project template using the [Link](https://github.com/AErbsloeh/pico_runtime_example).

Enjoy. If you have questions and suggestions, please create an issue or contact the authors.
Further examples can be found in this [repo](https://github.com/analogdevicesinc/no-OS/tree/main).

## Hardware Abstraction Layer (hal)
- [x] RP2-ADC interface
- [x] SPI interface
- [x] I2C interface
- [x] UART interface
- [x] USB interface
- [x] Clock generating with PIO module
- [x] Power handling for external devices (with/without feedback)
- [x] Timer interrupt
- [x] LED control
- [x] Pulse Width Modulaton
- [x] Timer
- [ ] Bluetooth Stack
- [ ] Wifi with different data packets like UDP, TCP
- [x] LED control of WS2812b (using PIO)
- [x] Clock Generator with GPIO output (using PIO)

## Device Libraries
- [x] ADC ADS8881
- [x] ADC AD7779
- [ ] ADC AD4858 
- [x] DAC AD57x4
- [x] DAC AD5765
- [x] DAC LTC2668
- [x] Multiplexer: ADG1408
- [x] Multiplexer: ADGS1208
- [x] Digital Potentiometer: AD528x
- [x] Digital Potentiometer: AD526x
- [x] Digital Potentiometer: AD5141 (I2C, SPI)
- [x] Digital Potentiometer: AD5142A

## Peripherie Devices
- [x] Ethernet Module CH9121
- [x] SD card support
- [ ] Ethernet module W5500 (WIP)


## Sensor Libraries
- [x] Environmeont Sensor: SHT21
- [x] Accelerator: ADXL345
- [x] Accelerator and Gyroscope: BMI270
- [x] Light: VEML7700
- [x] Distance Estimuation: VL6180x
