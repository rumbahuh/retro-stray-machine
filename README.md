# retro-stray-machine
## Technical Report & Difficulties
To organize the program, I decided to implement a state machine. This allowed me to keep the code as clean as possible as well as preventing interruptions from messing with the flow of a sequential program with such high frequency as the Arduino loop() function.

Each state handles a distinct part of the system which makes it easier to maintain and extend.
### State Machine
IDLE, START, PREPARING and RETIRAR are substates that act as helpers, the latter acting only on SERVICE state. The real state machine, however, is based on SERVICE and ADMIN.

### Sensors
The only parts of the code that could block the system come from the inherent behavior of some sensor libraries, which may fail or block during execution. Since the project is thought to act as a real-time system would, such behaviour is unacceptable, for I implemented the watchdog timer, resetting it on every loop iteration to make good use of the high frequency.

Additionally, I implemented a nonBlockingDelay(int) function to assist functions that use such sensors and/or other timed logic in the means to wait until a specified counter was satisfied using a 1000ms interval.

### Interruptions, Joystick & Debounce
I used hardware interrupts to react immediately, with the exception that it was important to: 1) measure how long the push button was pressed to know if it was meant to switch to a particular state, and, 2) cancel signal noise, which means that both the joystick and push button only register input when moving from the center(or idle) position to an active state.

### LED timing for PREPARING
LED2 intensity is increased gradually using PWM to show the progress of drink preparation, so the user can see how far along it is without just looking at the LCD.

LED1 blinks three times at startup while the system is initializing, and stays on in Admin mode to signal that the machine is under administrative control.

The interesting part of this is that the system can indicate progress while still responding to buttons, joystick input, and sensor readings, which shows how multitasking can be simulated on a microcontroller.

### Differences in structs & LCD
Products and Admin Options are stored in separate structs. This was the simplest way to handle price and menu data, since parsing between chars and ints would be error-prone and unnecessarily complex. Using structs makes it easy to modify, access, or extend data without risking conversion mistakes.

As for the LCD, it is used to show system status, sensor readings, product lists, and menus. With the limit that no message to display could be longer than 16 characters (match columns on screen); if the case, those are split across the two rows of the screen to fit the display, making sure all information is readable.
I must say due to lack of time I simply added more spaces in some messages to make them fit, and I am painfully aware that a proper implementation would have been more elegant, even if not directly related to real-time systems.


## [Demo (not mirrored video)](https://youtu.be/FylNRDOHbrw)
https://github.com/user-attachments/assets/e2b0b2d0-d37c-4b72-8244-d0df8db178bc
## Description
This project is meant to implement different learning methods of Real-Time Sytems to simulate a vending machine behaviour using sensors and actuators.
It was implemented for educational purposes to practice to use non-blocking interrupts, and state machine logic on an Arduino platform.

The vending machine can:
- Detect the nearby person using the ultrasonic sensor
- Display temperature and humidity using DHT11 sensor
- Navigate and select products or admin options using a joystick
- Modify product prices in admin mode
- Simulate preparation and delivery of drinks using intensity of LED2
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
- Joystick: Used a 5-legged dual-axis joystick model found online
- DHT11: Used a 3-legged sensor model found online
- Fritzing: For wiring references and schema design
