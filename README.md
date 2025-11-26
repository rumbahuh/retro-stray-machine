# retro-stray-machine
## Description
This project is meant to implement different learning methods of Real Time Sytems to simulate a vending machine behaviour using different sensors and actuators.
It was implemented for educational purposes as a learning practice to use non blocking programming.
## Demo
## Components
● Arduino UNO
● LCD
● Joystick
● DHT11 Sensor for Temperature/Humidity
● Ultrasonic Sensor
● Button
● 2 normal LEDs (LED1, LED2)
## Installation
From teacher:
'''
sudo apt update && sudo apt install libfuse-dev
'''
Once you download your Arduino image from the Arduino page:
'''
chmod +x arduino-ide_<version>.AppImage // Remember to change the version with yours
sudo mv arduino-ide_2.3.6_Linux_64bit.AppImage /usr/local/bin/arduino // This way you can execute it as arduino
'''
App launcher:
'''
nano ~/.local/share/applications/arduino.desktop
'''
'''
[Desktop Entry]
Type=Application
Name=IDE 2
GenericName=Arduino IDE
Comment=Open-source electronics prototyping platform
Exec=/usr/local/bin/arduino
Icon=<path/to/arduino/icon/arduino2.png>
Terminal=false
Categories=Development;IDE;Electronics;
MimeType=text/x-arduino;
Keywords=embedded electronics;electronics;avr;microcontroller;
StartupWMClass=processing-app-Base
'''
Remember to change image with yours as well as names and exec if different
## Usage
The usage is simple, once the wiring is set on your breadboard machine goes into SERVICE state.
You can navigate every menu by moving your joystick up and down. To cancel every action but preparation of the service state implementations it's as simple as moving the joystick left, and to choose an option pressing the own joystick.
As a note: To confirm price change on admin menu you must press the button a second time.
## Wiring
See schema.
## Tech Stack
Ive used Ada fruit DHT sensor library,
## Credits
For the schema, I had to search for an fritcing model for the 1PC Dual joystick. I cant upload it, but its important to know I used a four legged one.
