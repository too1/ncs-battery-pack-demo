# Battery Pack Demo

### Overview
********
This application implements the board controller firmware for the nRF device in the battery pack demo. The nRF controller sets up a Bluetooth connection for configuration, and controls the nPM, button, LED and 5V boost converter. 

Currently the Bluetooth peripheral uses the Nordic UART Service, to allow standard Nordic applications to be used as a controller. 

PMIC events will be forwarded to the NUS service, allowing the application to follow the various events. 

The following commands are also implemented:

| Command | Purpose | Description |
| ------- | ------- | ----------- |
| "Rbv" | Read Battery Voltage | Reads the voltage of the battery through the nPM, and returns the result to the app |
| "Reset" | Reset nRF | Will reset the nRF device, breaking the Bluetooth connection. Can be used to get out of a buggy state, until the firmware handles this locally |
| "SetvNN" | Set Output Voltage | This will set the output voltage of the BUCK0 converter, which will be provided through a connector on the battery pack, and can power external boards. Supports voltages in the 1.0-3.3V range. The number NN is the voltage in tens of a volt, ie to set the voltage to 2.5V send "Setv25"|

### Requirements
************
This sample has been tested on the Nordic NordicSemiconductor nRF52840DK (nrf52840dk_nrf52840) board with nPM EK.

Built in nRF Connect SDK v2.1.2, with the PMIC libraries patched in.

### TODO
********

- Improved nPM interaction. Figure out why the battery voltage always reads the same. 
- Figure out a way to recover from I2C bus errors
- Set the nRF BUCK voltage to 3.3V
- Provide access to the charging current, whenever the battery is being charged
- Provide an easy way to read charging state (charging, not charging)
- Implement PWM for an RGB LED, and reflect important system states through the LED
- Add support for the 5V boost converter
- Add flash libraries to support storing configuration data permanently in flash
- DFU support
- If ever a custom mobile app is implemented, create a dedicated Bluetooth service rather than using NUS
