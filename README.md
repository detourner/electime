# ElecTime

Electime is a firmware project for the Seeed Studio XIAO ESP32-C3 that measures the mains frequency and displays it on an analog needle gauge driven by a stepper motor. It also shows the current time (HH:MM:SS) on an HCMS-3977 alphanumeric display. The goal is to combine real-time frequency monitoring with a functional clock in a compact and visually engaging device.

The device:

- Get the mains frequency from webserver [GridFreqMonitor](https://github.com/detourner/GridFreqMonitor).
- Displays it on an analog needle gauge driven by a stepper motor.
- Shows the current time (HH:MM:SS) on an HP HCMS-3977 alphanumeric display.

## Features
- Stepper motor control for analog gauge display.
- Accurate timekeeping with synchronized updates.
- Clear alphanumeric display for the clock.
- Compact ESP32-C3 platform with Wi-Fi capabilities for potential network sync (NTP).

## Hardware
- Seeed Studio XIAO ESP32-C3
- Analog needle gauge + stepper motor
- HP HCMS-3977 4-character alphanumeric display
- Power supply by USBC & basic wiring

## Getting Started
### Prerequisites
- PlatformIO or Arduino IDE
- USB-C cable for programming the XIAO ESP32-C3

## Installation
- Clone the repository:
```bash
    git clone https://github.com/yourusername/electime.git
    cd electime
```

- Open in PlatformIO or Arduino IDE.
- Install required libraries (see platformio.ini or lib_deps).
- Build and upload the firmware to your XIAO ESP32-C3.

## Usage
- On power-up, the device starts measuring the mains frequency.
- The analog gauge displays the frequency in real time.
- The alphanumeric display shows the current time in HH:MM:SS format.

## License
This project is licensed under the MIT License – see the LICENSE file for details.