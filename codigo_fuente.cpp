#include <LiquidCrystal.h>

// C++ code

const int LED1 = 13;
const int pin = 2;
const int ULTRASONIC_PIN = 9;
volatile bool service_state = false;
LiquidCrystal lcd(12, 11, 5, 4, 3, 7);
volatile bool states[2] = {false, false};

enum StateIndex {
  START = 0,
  SERVICE = 1
};

void start()
{
  states[START] = true;
}

void setup()
{
  pinMode(LED1, OUTPUT);
  pinMode(pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin), start, FALLING);
  lcd.begin(16, 2);
}

void loop()
{
  switch (states[START]) {
    case true:
      states[START] = false;
      lcd.clear();
  	  lcd.setCursor(0,0);
      // El potenciometro va una pata a GND y otra a V0 en la vida real
  	  lcd.print("CARGANDO ...");
      for(int i = 0; i < 3; i++) {
      	digitalWrite(LED1, HIGH);
  	    delay(1000);
  	    digitalWrite(LED1, LOW);
  	    delay(1000);
  	  }
      states[SERVICE] = true;
      break;
  }

  switch (states[SERVICE]) {
    case true:
      // Código para service
      // states[SERVICE] = false;
      lcd.clear();
  	  lcd.setCursor(0,0);
  	  lcd.print("On service");
      // Ojo aquí que esto se ejecuta en el loop a toda hostia
      states[SERVICE] = false;
      break;
  }
}
