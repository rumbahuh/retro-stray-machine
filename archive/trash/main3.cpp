#include <LiquidCrystal.h>

const int pin = 2;  // Interrupción -- Botón

// -- Pines de Joystick
const int joystick = 3; // Interrupción -- Botón de joystick
const int Vx = A0;
const int Vy = A1;

// -- Pines de ledes
const int LED1 = 11;
const int LED2 = 6;

// -- Pines de pantalla lcd
LiquidCrystal lcd(12, 13, 5, 4, 8, 7);

// -- Pines de sensor de ultrasonidos
const int PIN_TRIG = 9;
const int PIN_ECHO = 10;

// -- Pines de sensor de humedad
// const int PIN_HUMEDAD = A3 // Uso analógico porque no me quedan más pines

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

volatile bool exit_satisfied = false;
volatile bool first_iteration_loop = true;

volatile bool joystick_bool = false;

volatile bool led_bool = true;

int getPulseTime() { return (LOW_TO_HIGH_TIME - HIGH_TO_LOW_TIME) / 1000.0; }
void button_unpressed() {
  Serial.println("Button was released");
  double pulseTime = getPulseTime();

  if (pulseTime >= 2.0 && pulseTime <= 3.0) {
    currentState = SERVICE;
    first_iteration = true;
    previousState = IDLE; // TENGO QUE cambiar logica para decir que en uno restart y el otro simplemente vuelve a service
  } else if (pulseTime > 5) {
    if (currentState == ADMIN) {
      currentState = SERVICE;
      previousState = ADMIN;
      first_iteration = true; // Sin esto no vuelve a comprobar persona!
    } else {
      currentState = ADMIN;
      previousState = SERVICE;
    }
  }

  button_ready = false;
}

void button_pressed() {
  unsigned long now = millis();

  if (digitalRead(pin) == HIGH) {
    LOW_TO_HIGH_TIME = now;
    button_ready = true;

  } else {
    HIGH_TO_LOW_TIME = now;
  }
}

void joystick_pressed() {
  joystick_bool = true;
  currentState = IDLE;

}

void setup() {
  Serial.begin(9600);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  pinMode(pin, INPUT_PULLUP);
  pinMode(joystick, INPUT_PULLUP);

  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_TRIG, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(pin), button_pressed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(joystick), joystick_pressed, FALLING);

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
  if (led_bool) {
    if (led_start == 0) {
        display("CARGANDO");
        digitalWrite(LED1, HIGH);
        led_start = millis();
        Serial.println("led started");
    }

    unsigned long now = millis();
    unsigned long elapsed = now - led_start;

    if (elapsed >= 1000) {
      digitalWrite(LED1, !digitalRead(LED1));
      led_start = now;

      if (digitalRead(LED1) == LOW) {
        counter++;
      }

      if (counter >= 3) {
        digitalWrite(LED1, LOW);
        counter = 0;
        led_start = 0;
        led_bool = false;
        previousState = START;
        currentState = SERVICE;
      }
    }
  }
  if (button_ready) {
    button_unpressed();
  }
  
  if(joystick_bool) {
    Serial.println("Joystick was pressed!!");
    joystick_bool = false;
  }

  switch (currentState) {
    case SERVICE: {
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      bool person_now_detected = false;
      double distance = get_distance();
      unsigned long millisStart = millis();

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
      first_iteration_loop = false;
      break;
    }

  case ADMIN: {
    if (previousState != currentState) {
      display("ADMIN");
      previousState = currentState;
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
    }
    break;
    }
  }
}

