#include <LiquidCrystal.h>

// -- Pines de interrupciones
const int pin = 2;  // Botón
// cont int pin = 3; // Botón de joystick
// -- Pines de ledes
const int LED1 = 11;
// const int LED2 = 6;
// -- Pines de pantalla lcd
LiquidCrystal lcd(12, 13, 5, 4, 8, 7);
// -- Pines de sensor de ultrasonidos
const int PIN_TRIG = 9;
const int PIN_ECHO = 10;
// -- Pines de sensor de humedad
// const int PIN_HUMEDAD = // ME FALTA un pin

const double METER_LIGHT_SPEED_CONVERSION_FACTOR = 0.0001715;

volatile bool previously_detected_person = false;
volatile bool first_iteration = true;

enum State { IDLE, START, SERVICE, ADMIN, OPTIONS };
volatile State currentState = IDLE;
volatile State previousState = IDLE;

volatile unsigned long LOW_TO_HIGH_TIME = 0;
volatile unsigned long HIGH_TO_LOW_TIME = 0;
volatile bool button_ready = false;

volatile unsigned long led_start = 0;
volatile unsigned long led_time = 0;
volatile bool led_finished_non_blocking_delay = false;
volatile int counter = 0;

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
  pinMode(LED1, OUTPUT);
  pinMode(pin, INPUT_PULLUP);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_TRIG, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(pin), button_pressed, CHANGE);
  lcd.begin(16, 2);
}

double get_distance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  double dis = pulseIn(PIN_ECHO, HIGH);
  return dis * METER_LIGHT_SPEED_CONVERSION_FACTOR;
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
      unsigned long millisStart = millis();

      double pulseTime = getPulseTime();
      if (pulseTime <= 2.9) {
        if (previousState != currentState) {
          display("CARGANDO");
          counter = 0;
          digitalWrite(LED1, HIGH);
          led_start = millis();
          previousState = currentState;
        }

        if (led_start > 0) {
          unsigned long now = millis();
          unsigned long elapsed = now - led_start;

          if (elapsed >= 1000) {                     // toggle every 1 second
            digitalWrite(LED1, !digitalRead(LED1));  // toggle LED
            led_start = now;                         // reset timer

            if (digitalRead(LED1) == LOW)
              counter++;  // count full ON->OFF cycles
          }

          if (counter >= 3) {         // after 3 full ON->OFF cycles
            digitalWrite(LED1, LOW);  // ensure LED is off
            counter = 0;
            led_start = 0;
            currentState = SERVICE;
            previousState = START;
          }
        }
      } else if (pulseTime > 2.9 && pulseTime < 4.0) {
        currentState = ADMIN;
        previousState = START;
      }
      else {
        currentState = OPTIONS;
        previousState = START;
      }
      break;
  }

  case SERVICE: {
    bool person_now_detected = false;

    double distance = get_distance();
    if (distance < 1) person_now_detected = true;

    if (first_iteration || person_now_detected != previously_detected_person) {
      if (previousState != currentState) {
        if (person_now_detected)
          display("DETECTED");
        else
          display("ESPERANDO CLIENTE");

        previously_detected_person = person_now_detected;
        first_iteration = false;
      }
      previousState = currentState;
    }
  } break;

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
