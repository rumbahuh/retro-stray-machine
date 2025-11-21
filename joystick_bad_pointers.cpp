#include <LiquidCrystal.h>

const int pin = 2;  // Interrupción -- Botón

// -- Pines de Joystick
const int joystick = 3; // Interrupción -- Botón de joystick
const int Vx = A0;
const int Vy = A1;

const int CENTER_X_MIN = 450; // Me da valores diferentes cada vez que lo conecto
const int CENTER_X_MAX = 510;
const int CENTER_Y_MIN = 450;
const int CENTER_Y_MAX = 510;

volatile bool is_idle = true;
// -- Pines de ledes
const int LED1 = 11;
const int LED2 = 6;

// -- Pines de pantalla lcd
LiquidCrystal lcd(12, 13, 5, 4, 8, 7);

// -- Pines de sensor de ultrasonidos
const int PIN_TRIG = 9;
const int PIN_ECHO = 10;

const double METER_LIGHT_SPEED_CONVERSION_FACTOR = 0.0001715;

volatile bool previously_detected_person = false;
volatile bool first_iteration = true;

// -- Pines de sensor de humedad
// const int PIN_HUMEDAD = A3 // Uso analógico porque no me quedan más pines

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

volatile bool joystick_bool = false;

volatile bool led_bool = true;

const char* const products_n_price_ptr[] = {"i. Café Solo 1€", 
                                        "ii. Café Cortado 1.10 €", 
                                        "iii. Café Doble 1.25 €",
                                        "iv. Café Premium 1.50 €",
                                        "v. Chocolate 2.00 €"};

const char* const admin_ptr[] = {"i. Ver temperatura", 
                             "i. Ver distancia sensor", 
                             "iii. Ver contador",
                             "iv. Modificar precios"};

// Punteros globales para selección
volatile const char* current_product_ptr = products_n_price_ptr[0];
volatile const char* current_admin_ptr = admin_ptr[0];

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
      first_iteration = true;
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

void joystick_read(volatile const char** current_selection, 
                           const char* const* menu_start, 
                           size_t menu_size) {
  // Create pointer read only name ptr_read_only, not modify ptr, maybe not copy
  // cause i dont want to overload the memory of the microcontroller
  const char* const * menu_end = menu_start + (menu_size - 1);

  int x = analogRead(Vx);
  int y = analogRead(Vy);

  // Para eliminar el ruido debemos notar que pasa del centro a otro lugar
  bool center_state = (x >= CENTER_X_MIN && x <= CENTER_X_MAX) &&
               (y >= CENTER_Y_MIN && y <= CENTER_Y_MAX);
  
  if (center_state) {
    is_idle = false;
    return;
  }
  
  if (is_idle) {
    return;
  }

  bool moved = false;

  if (x > 220 && x < 1100 && y == 0) {
    if (*current_selection > *menu_start) { // no podemos ser el primer char[]
      // Debemos coger un elemento de los primeros
      // up, que es menos en pointer
      *current_selection = *current_selection - 1;
      moved = true;
    }
  }

  if (x > 200 && x < 900 && y > 920 && y < 1023) { // Mi joystick a aveces capta en Y abajo 920 o 990 o 1002, así que voy a elegir un threshold tambien para la coordenada fija porque no es fija en mi caso
    if (*current_selection < *menu_end) { // no podemos ser el último char[]
      // Debemos coger un elemento de los finales
      // down, que es más en pointer
      *current_selection = *current_selection + 1;
      moved = true;
    }
  }

  if (moved) {
    is_idle = true;
    display(*current_selection);
  }
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
      break;
    }

  case ADMIN: {
    if (previousState != currentState) {
      if (first_iteration) {
        previousState = currentState;
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
        display(current_admin_ptr);
        delay(1000);
        lcd.clear();
        first_iteration = false;
        current_admin_ptr = admin_ptr[0];
      }
    }

    size_t admin_menu_size = sizeof(admin_ptr) / sizeof(admin_ptr[0]);
    joystick_read(&current_admin_ptr, admin_ptr, admin_menu_size);

    break;
    }
  }
}

