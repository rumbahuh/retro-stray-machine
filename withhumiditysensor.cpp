#include <DHT.h>
#include <DHT_U.h>

#include <Adafruit_Sensor.h>

#include <LiquidCrystal.h>

#define DHTTYPE DHT11

const int pin = 2;  // Interrupción -- Botón

// -- Pines de Joystick
const int joystick = 3;  // Interrupción -- Botón de joystick
const int Vx = A0;
const int Vy = A1;

const int CENTER_X_MIN =
    450;  // Me da valores diferentes cada vez que lo conecto
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

// -- Pines de sensor de humedad
const int DHTPIN = A3; // Uso analógico porque no me quedan más pines
DHT dht(DHTPIN, DHTTYPE);

const double METER_LIGHT_SPEED_CONVERSION_FACTOR = 0.0001715;

volatile bool previously_detected_person = false;
volatile bool first_iteration = true;
volatile bool first_iteration_product = true;

enum State { IDLE, START, SERVICE, ADMIN, OPTIONS, PREPARING, RETIRAR, PRICES};
volatile State currentState = IDLE;
volatile State previousState = IDLE;

/*Teniendo en cuenta que tengo configurado los pines en PULLUP*/
volatile unsigned long LOW_TO_HIGH_TIME = 0;
volatile unsigned long HIGH_TO_LOW_TIME = 0;

// Si el botón no está siendo presionado
volatile bool button_ready = false;
// Si se puede salir del estado
volatile bool exit_satisfied = false;

// -- Sobre lógica de delays de las ledes
volatile unsigned long led_start = 0;
volatile unsigned long led_time = 0;
volatile bool led_finished_non_blocking_delay = false;
volatile int counter = 0;

volatile bool led_bool = true;

volatile bool joystick_bool = false;
volatile bool actionFinished = true;

struct Product {
  const char* line;
  float price;
  int id;
  volatile bool active;
};

struct Option {
  const char* line;
  int id;
  volatile bool active;
};

volatile Product products[] = {{"i.CafeSolo", 1.00, 0, false},
                               {"ii.Cafe Cortado", 1.10, 1, false},
                               {"iii.Cafe Doble", 1.25, 2, false},
                               {"iv.Cafe Premium", 1.50, 3, false},
                               {"v.Chocolate     ", 2.00, 4, false}};

bool product_or_admin = -1;

volatile size_t PRODUCTS_SIZE = sizeof(products) / sizeof(products[0]);

const Option admin_options[] = {{"i.Ver temperatura", 0, false},
                                {"ii.Ver distancia sensor", 1, false},
                                {"iii.Ver contador", 2, false},
                                {"iv.Modificar precios", 3, false}};

volatile size_t current_product_index = 0;
volatile size_t current_admin_index = 0;

// Punteros globales para selección
volatile Product* current_product = &products[0];
volatile Option* current_admin_option = &admin_options[0];

const size_t ADMIN_OPTIONS_SIZE =
    sizeof(admin_options) / sizeof(admin_options[0]);

unsigned long START_TIME = 0;
volatile unsigned long last_check_time = 0;
volatile unsigned long now = 0;

volatile bool current_admin_option_activated = false;
volatile bool current_product_option_activated = false;

volatile int preparation_time = 0;
volatile int increasing_led_value = 0;

bool nonBlockingDelay(int target) {
  if (now - last_check_time >= 1000.0) {
    counter++;
    last_check_time = now;
  }

  if (counter >= target) {
    counter = 0;
    return true;
  }

  return false;
}

int getPulseTime() { return (LOW_TO_HIGH_TIME - HIGH_TO_LOW_TIME) / 1000.0; }

void button_unpressed() {
  Serial.println("Button was released");
  double pulseTime = getPulseTime();

  if (pulseTime >= 2.0 && pulseTime <= 3.0) {
    currentState = SERVICE;
    first_iteration = true;
    previousState = IDLE;
  } else if (pulseTime > 5) {
    if (currentState == ADMIN) {
      currentState = SERVICE;
      previousState = ADMIN;
      first_iteration = true;  // Sin esto no vuelve a comprobar persona!

    } else {
      currentState = ADMIN;
      previousState = SERVICE;
      first_iteration = true;
      first_iteration_product = true;
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
  current_admin_option_activated = true;
  current_product_option_activated = true;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  pinMode(pin, INPUT_PULLUP);
  pinMode(joystick, INPUT_PULLUP);

  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  dht.begin();

  attachInterrupt(digitalPinToInterrupt(pin), button_pressed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(joystick), joystick_pressed, FALLING);

  lcd.begin(16, 2);
  START_TIME = millis();
}

double get_distance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  double dis = pulseIn(PIN_ECHO, 255);
  return dis * METER_LIGHT_SPEED_CONVERSION_FACTOR;
}

void display(const char* message) {
  // Uso const char* en lugar de String
  // para no alocar dinámicamente
  // ya que la RAM del microcontrolador Arduino
  // es muy limitada
  lcd.clear();

  lcd.setCursor(0, 0);
  for (int i = 0; i < 16 && message[i] != '\0'; i++) {
    lcd.print(message[i]);
  }

  int len = strlen(message);
  if (len > 16) {
    lcd.setCursor(0, 1);
    for (int i = 16; i < 32 && message[i] != '\0'; i++) {
      lcd.print(message[i]);
    }
  }
}

void displayProduct(const Product* p) {
  char price_str[10];
  char msg[32];

  // Para que no me de problemas de conversion busqué una función
  // helper
  dtostrf(p->price, 6, 2, price_str);
  snprintf(msg, sizeof(msg), "%s%seur", p->line, price_str);

  display(msg);
}

void joystick_read(int choice) {
  int x = analogRead(Vx);
  int y = analogRead(Vy);

  // Llevamos cuenta de cuando pasa del centro a otro estado
  bool center_state = (x >= CENTER_X_MIN && x <= CENTER_X_MAX) &&
                      (y >= CENTER_Y_MIN && y <= CENTER_Y_MAX);

  if (center_state) {
    is_idle = false;
    return;
  }
  if (is_idle) return;  // Así prevenimos diferentes llamadas a la vez

  bool moved_admin = false;
  bool moved_product = false;

  if ((y > 243 && y < 1010 && x == 0)) {
    /*if (current_product_option_activated) {
      current_product->active = false;  // Salir de seleccion
      displayProduct(current_product);
    }*/
    if (current_admin_option->active) {      // Se pulsó el botón del joystick
      current_admin_option->active = false;  // Salir de seleccion
      display(current_admin_option->line);
    }
  }
  // Seleccion hacia arriba
  if (x > 220 && x < 1100 && y == 0) {
    if (current_admin_option->active && current_admin_option->id == 3) {
      if (current_product->active) {
        current_product->price =
            current_product->price + 0.01;  // Aumenta en un centimo
        moved_product = true;
        is_idle = true;
      } else {
        if (current_product_index > 0 && choice == 1) {
          current_product_index--;
          moved_product = true;
          is_idle = true;
        }
      }
    } else if (!current_admin_option->active) {
      if (current_admin_index > 0 && choice == 0) {
        current_admin_index--;
        moved_admin = true;
        is_idle = true;
      }
      if (current_product_index > 0 && choice == 1) {
        current_product_index--;
        moved_product = true;
        is_idle = true;
      }
    }
  }

  // Seleccion hacia abajo
  if (x > 200 && x < 900 && y > 920 && y < 1023) {
    if (current_admin_option->active && current_admin_option->id == 3) {
      if (current_product->active) {
        current_product->price =
            current_product->price - 0.01;  // Disminuye en un centimo
        moved_product = true;
        is_idle = true;
      } else {
        if (current_product_index < PRODUCTS_SIZE - 1 && choice == 1) {
          current_product_index++;
          moved_product = true;
          is_idle = true;
        }
      }

    } else if (!current_admin_option->active) {
      if (current_admin_index < ADMIN_OPTIONS_SIZE - 1 && choice == 0) {
        current_admin_index++;
        moved_admin = true;
        is_idle = true;
      }
      if (current_product_index < PRODUCTS_SIZE - 1 && choice == 1) {
        current_product_index++;
        moved_product = true;
        is_idle = true;
      }
    }
  }

  if (moved_admin) {
    current_admin_option = &admin_options[current_admin_index];
    display(current_admin_option->line);
  }

  if (moved_product) {
    current_product = &products[current_product_index];
    displayProduct(current_product);
  }
}

void displayTemp() {
  if (now - last_check_time >= 200.0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(dht.readHumidity());
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(dht.readTemperature());
    last_check_time = now;
  }
}

void displayDist() {
  if (now - last_check_time >= 200.0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(get_distance());
    last_check_time = now;
  }
}

void seeCounter() {
  if (now - last_check_time >= 1000.0) {
    int time = (now - START_TIME) / 1000.0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(time);
    last_check_time = now;
  }
}

void modifyPrice() {
  if (first_iteration_product) {
    current_product_index = 0;
    current_product = &products[current_product_index];
    displayProduct(current_product);
    first_iteration_product = false;
  }
}

void retirarBebida() {
  if (now - last_check_time >= 1000.0) {
    counter++;
    last_check_time = now;
  }
  if (counter == 3) {
    lcd.clear();
    analogWrite(LED2, 0);
    counter = 0;
    increasing_led_value = 0;
    current_product->active = false;
    previousState = currentState;
    currentState = SERVICE;
  }
}
void prepararBebida(int time) {
  if (now - last_check_time >= 1000.0) {
    counter++;
    increasing_led_value = increasing_led_value + 255/time; // HIGH = 255, 255 / 3 = 85
    analogWrite(LED2, increasing_led_value);
    last_check_time = now;
    Serial.println("----");
    Serial.println(counter);
    Serial.println(time);
  }
  if (counter == time) {
    counter = 0;
    previousState = PREPARING;
    currentState = RETIRAR;
    display("RETIRE BEBIDA");
  }
}

void loop() {
  now = millis();

  /*Necesitamos que estas funciones se ejecuten continuamente
  si es que están activas para mantener dinamismo
  y aprovechar la alta fecuencia del bucle*/
  if (led_bool) {
    if (led_start == 0) {
      display("CARGANDO");
      analogWrite(LED1, 255);
      led_start = millis();
      Serial.println("led started");
    }

    unsigned long now = millis();
    unsigned long elapsed = now - led_start;

    if (elapsed >= 1000) {
      analogWrite(LED1, !digitalRead(LED1));
      led_start = now;

      if (digitalRead(LED1) == 0) {
        counter++;
      }

      if (counter >= 3) {
        analogWrite(LED1, 0);
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

  if (joystick_bool) {
    joystick_bool = false;

    if (product_or_admin == 0) {
      current_admin_option->active = true;

    } else if (product_or_admin == 1) {
      current_product->active = true;
    }
  }

  // La máquina de estados siempre se encontrará en un estado
  switch (currentState) {
    case SERVICE: {
      product_or_admin = 1;

      // Add an if on if  ledes on, turn off
      analogWrite(LED1, 0);
      analogWrite(LED2, 0);
      bool person_now_detected = false;
      double distance = get_distance();
      unsigned long millisStart = millis();

      // Si es la primera iteración de SERVICE al entrar
      // y si el sensor de ultrasonidos ha tenido un cambio de estado
      if (first_iteration ||
          !person_now_detected != previously_detected_person) {
        // Este if creo que me sobra
        if (previousState != currentState) {
          if (!person_now_detected) {  // Cambiar boolean (era para
                                       // implementarlo porque me daba errores)
            display("DETECTED");
            if (first_iteration) {  // Esta es redundant no?
              previousState = currentState;
              current_product_index = 0;
              current_product = &products[current_product_index];
              displayProduct(current_product);
              first_iteration = false;
            }
          } else {
            display("ESPERANDO CLIENTE");
          }
          previously_detected_person = person_now_detected;
          first_iteration = false;
        }

        previousState = currentState;
        if (!person_now_detected) {
          joystick_read(product_or_admin);

          if (current_product->active) {
            counter = 0;
            last_check_time = now;

            preparation_time = random(4, 8);
            previousState = currentState;
            currentState = PREPARING;
            display("PREPARANDO CAFE...");
          }
        }
      }
      break;
    }

    case PREPARING: {
      switch (current_product->id) {
        case 0:
          prepararBebida(preparation_time);
          break;
        case 1:
          prepararBebida(preparation_time);
          break;
        case 2:
          prepararBebida(preparation_time);
          break;
        case 3:
          prepararBebida(preparation_time);
          break;
        case 4:
          prepararBebida(preparation_time);
          break;
      }
      break;
    }

    case RETIRAR: {
      retirarBebida();
      break;
    }

    case ADMIN: {
      product_or_admin = 0;

      if (previousState != currentState) {
        if (first_iteration) {
          previousState = currentState;
          analogWrite(LED1, 255);
          analogWrite(LED2, 255);
          current_admin_index = 0;
          current_admin_option = &admin_options[current_admin_index];
          display(current_admin_option->line);
          first_iteration = false;
        }
      }
      joystick_read(product_or_admin);

      if (current_admin_option->active) {
        switch (current_admin_option->id) {
          case 0:
            displayTemp();
            break;
          case 1:
            displayDist();
            break;
          case 2:
            seeCounter();
            break;
          case 3:
            previousState = currentState;
            currentState = PRICES;
            break;
        }
      }

      break;
    }

    case PRICES: {
      product_or_admin = 1;  // Va a modificar los precios

      if (previousState != currentState) {
        if (first_iteration) {  // Esta es redundant no?
          previousState = currentState;
          current_product_index = 0;
          current_product = &products[current_product_index];
          displayProduct(current_product);
          first_iteration = false;
        }
      }
      joystick_read(product_or_admin);

      previousState = currentState;

        if (current_product->active) {
          switch (current_product->id) {
            case 0:
              Serial.println("0");
              break;
            case 1:
              Serial.println("1");
              break;
            case 2:
              Serial.println("2");
              break;
            case 3:
              Serial.println("3");
              break;
            case 4:
              Serial.println("4");
              break;
          }
      }
      break;
    }
  }
}
