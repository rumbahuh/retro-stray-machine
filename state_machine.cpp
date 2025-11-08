#include <LiquidCrystal.h>

// C++ code

const int LED1 = 13;
const int pin = 2;
const int ULTRASONIC_PIN = 9;
volatile bool service_state = false;
LiquidCrystal lcd(12, 11, 5, 4, 3, 7);
enum State {IDLE, START, SERVICE};
volatile State currentState = IDLE;

void start()
{
  currentState = START;
}

void setup()
{
  pinMode(LED1, OUTPUT);
  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin), start, FALLING);
  lcd.begin(16, 2);
}

void loop() {
    switch(currentState) {
        case START:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("CARGANDO ...");
            for (int i=0; i<3; i++) {
                digitalWrite(LED1, HIGH);
                delay(1000);
                digitalWrite(LED1, LOW);
                delay(1000);
            }
            currentState = SERVICE;
            break;

        case SERVICE:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("On service");
            // service code
            currentState = IDLE;
            break;
    }
}
