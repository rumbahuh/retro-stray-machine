/*
Authoress: Rebeca Castilla
Date: 13-15/Nov/2025
Goal: Cambiar estado de máquina de estados en función del tiempo
      en que se presiona un botón usando interrupciones.
*/

#include <LiquidCrystal.h>

// -- Pines de interrupciones
const int pin = 2;  // Botón
// -- Pines de pantalla lcd
LiquidCrystal lcd(12, 13, 5, 4, 8, 7);

enum State { IDLE, START, SERVICE, ADMIN, OPTIONS };
volatile State currentState = IDLE;
volatile State previousState = IDLE;

volatile unsigned long LOW_TO_HIGH_TIME = 0;
volatile unsigned long HIGH_TO_LOW_TIME = 0;
volatile bool button_ready = false;

void button_pressed() {
  unsigned long now = millis();

  if (digitalRead(pin) == HIGH) {
    LOW_TO_HIGH_TIME = now;
    button_ready = true;

  } else {
    HIGH_TO_LOW_TIME = now;
  }

  currentState = IDLE;
}

void setup() {
  pinMode(pin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pin), button_pressed, CHANGE);
  lcd.begin(16, 2);
}

int getPulseTime() { return (LOW_TO_HIGH_TIME - HIGH_TO_LOW_TIME) / 1000.0; }

void display(const char* message_to_display) {
  // Uso const char* en lugar de String
  // para no alocar dinámicamente
  // ya que la RAM del microcontrolador Arduino
  // es muy limitada

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message_to_display);
}

void loop() {
  switch (currentState) {
    case IDLE: {
      if (button_ready) {
        currentState = START;
        button_ready = false;
        previousState = IDLE;
      }
      break;
    }
    case START: {
      double pulseTime = getPulseTime();

      if (pulseTime <= 2.9) {
        if (previousState != currentState) {
          display("CARGANDO");
          previousState = currentState;
        }
      } else if (pulseTime > 2.9 && pulseTime < 4.0) {
        currentState = ADMIN;
        previousState = START;
      } else {
        currentState = OPTIONS;
        previousState = START;
      }
      break;
    }

    case SERVICE: {
      if (previousState != currentState) {
        display("SERVICE");
        previousState = currentState;
      }
      break;
    }

    case ADMIN:
      if (previousState != currentState) {
        display("ADMIN");
        previousState = currentState;
      }
      break;

    case OPTIONS:
      if (previousState != currentState) {
        display("OPTIONS");
        previousState = currentState;
      }
      break;
  }
}
