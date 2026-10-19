# ESP32 SOS Alert System

A low-cost emergency alert system built on the ESP32. The board creates its own WiFi hotspot and hosts a web page, so anyone nearby can connect and trigger an SOS alert **without internet**.

## Features

- ESP32 runs as a WiFi Access Point (no router or internet needed)
- Built-in web server with an SOS page
- Physical SOS button
- Buzzer and LED alert
- Low cost and easy to deploy

## Components

| Part | Quantity |
|------|----------|
| ESP32 development board | 1 |
| Push button | 1 |
| Active buzzer | 1 |
| LED + 220Ω resistor | 1 |
| Jumper wires / breadboard | as needed |
| 5V USB power supply | 1 |

## Wiring

> Change the pins to match the numbers in your code.

| Component | ESP32 Pin |
|-----------|-----------|
| SOS button | GPIO 4 → other leg to GND |
| Buzzer (+) | GPIO 5 (− to GND) |
| LED (via 220Ω) | GPIO 2 (short leg to GND) |

## How to Use

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and add ESP32 board support.
2. Open the `.ino` file from this repository.
3. Select your board (e.g. *ESP32 Dev Module*) and the correct COM port.
4. Upload the code.
5. Open the Serial Monitor (115200 baud) to confirm the access point started.
6. On your phone or laptop, connect to the ESP32 WiFi network.
7. Turn off mobile data, then open `http://192.168.4.1` in a browser.
8. Press the SOS button on the page to trigger the alert.

## Troubleshooting

- **Page not opening:** use `http://` (not `https://`), turn off mobile data and VPN, and check the Serial Monitor for resets.
- **ESP32 keeps rebooting:** use a stable 5V supply and a good USB cable.
- **Avoid GPIO 0, 12, 15** (boot pins) and **6 to 11** (flash) for your connections.

## Future Improvements

- GPS module for live location
- GSM module (SIM800/SIM900) to send SMS alerts
- Mobile app or dashboard
- Multiple SOS nodes

## Author

**Vel murugan M**
LinkedIn: https://www.linkedin.com/in/vel-murugan-308200378?utm_source=share_via&utm_content=profile&utm_medium=member_android

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

Copyright (c) 2026 Vel Murugan M
