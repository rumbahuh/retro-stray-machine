# retro-stray-machine
## Description
This project is meant to implement different learning methods of Real-Time Sytems to simulate a vending machine behaviour using sensors and actuators.
It was implemented for educational purposes to practice to use non-blocking interrupts, and state machine logic on an Arduino platform.

The vending machine can:
- Detect the nearby person using the ultrasonic sensor
- Display temperature and humidity using DHT11 sensor
- Navigate and select products or admin options using a joystick
- Modify product prices in admin mode
- Simulate preparation and delivery of drinks using intensity of LED2
## [Demo (not mirrored video)](https://youtu.be/FylNRDOHbrw)
https://github.com/user-attachments/assets/e2b0b2d0-d37c-4b72-8244-d0df8db178bc

## Technical Report & Difficulties

## Components
● Arduino UNO
● LCD
● Joystick (4 legs, dual-axis)
● DHT11 Sensor for Temperature/Humidity
● Ultrasonic Sensor (HC-SR04)
● Push Button
● 2 normal LEDs (LED1, LED2)
## Usage
The usage is simple, once the wiring is set on the breadboard, the machine goes into SERVICE state.
Use the joystick to navigate menus:
- UP/DOWN: Move selection (if modifying prices, up and down will show the increase/decrease of price in cents)
- LEFT: Cancel action (except during preparation)
- Press joyistick: Confirm selection / Confirm price change
## Wiring
See schema.

- LCD: RS=12, E=13, D4=5, D5=4, D6=8, D7=7
- Joystick: Vx=A0, Vy=A1, Button=3
- Push Button: 2
- LED1: 11
- LED2: 6
- HC-SR04: Trig=9, Echo=10
- DHT11: A3

## Tech Stack
- Arduino UNO
- Adafruit DHT Sensor Library
- LiquidCrystal Library
- Non-nlocking programming using millis() and state machines

## Credits
- Joystick: Used a 4-legged dual-axis joystick model found online
- DHT11: Used a 3-legged sensor model found online
- Fritzing: For wiring references and schema design
