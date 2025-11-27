#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>
#include <avr/wdt.h>

#define DHTTYPE DHT11

// -- INTERRUPCIONES
const int pin = 2;       // Interrupción -- Botón
const int joystick = 3;  // Interrupción -- Botón de joystick

// -- JOYSTICK
const int Vx = A0;
const int Vy = A1;

const int CENTER_X_MIN = 450;
const int CENTER_X_MAX = 510;
const int CENTER_Y_MIN = 450;
const int CENTER_Y_MAX = 510;

const int THRESHOLD_JU_XSUPREMO = 1100;  // Acotaciones de U/D variaban más
const int THRESHOLD_JU_XINFIMO = 220;
const int THRESHOLD_JU_Y = 0;

const int THRESHOLD_JD_XSUPREMO = 900;
const int THRESHOLD_JD_XINFIMO = 200;
const int THRESHOLD_JD_Y = 920;

const int THRESHOLD_JLJR_YSUPREMO = 1010;  // Acotaciones iguales para L/R
const int THRESHOLD_JLJR_YINFIMO = 243;
const int THRESHOLD_JL_X = 0;

const int THRESHOLD_JR_X = 1023;

// -- ESTADOS
enum State { IDLE, START, SERVICE, ADMIN, OPTIONS, PREPARING, RETIRAR, PRICES };
volatile State currentState = IDLE;
volatile State previousState = IDLE;

typedef enum { JS_CENTER, JS_UP, JS_DOWN, JS_LEFT, JS_RIGHT } JoystickMove;

const double TIME_RESET_SERVICE_INFIMO = 2.0;
const double TIME_RESET_SERVICE_SUPREMO = 3.0;
const double TIME_CHANGE_STATE_ADMIN = 5.0;

// -- FLAGS Y CANCELACION DE RUIDO (DEBOUNCE)
volatile bool is_idle = true;

volatile unsigned long LOW_TO_HIGH_TIME = 0;  // Teniendo en cuenta que tengo
volatile unsigned long HIGH_TO_LOW_TIME = 0;  // configurado los pines en PULLUP

volatile bool button_ready =
    false;  // Si el botón no está siendo presionado => false
volatile bool exit_satisfied = false;

// ES NECESARIO ELIMINAR EL RUIDO DEL BOTÓN DEL JOYSTICK !!! WARNING
volatile unsigned long last_joystick_press_time = 0;
const unsigned long JOYSTICK_DEBOUNCE_DELAY = 150;  // ms

// Cambiarán según las interrupciones correspondientes
// Después actuán en loop()
volatile bool led_bool = true;
volatile bool joystick_bool = false;

// -- LEDS
const int LED1 = 11;
const int LED2 = 6;

unsigned long led_start = 0;
unsigned long led_time = 0;

bool led_finished_non_blocking_delay = false;
int counter = 0;

const int HIGH_VALUE_PWM = 255;
const int LOW_VALUE_PWM = 0;
const int LED_BLINK_COUNT = 3;

// -- LCD
LiquidCrystal lcd(12, 13, 5, 4, 8, 7);
const int MAX_CHARPTR_PER_SCREEN = 16;
const int LCD_MAX_CHARS = 32;

// -- ULTRASONIDOS
const int PIN_TRIG = 9;
const int PIN_ECHO = 10;
const int PERSON_DETECTION_MAX_DISTANCE = 1;  // 1m

// Esto lo he reutilizado de mi práctica de sensores del año pasado
const double METER_SOUND_SPEED_CONVERSION_FACTOR = 0.0001715;

bool previously_detected_person =
    true;  // Empieza en true porque getDistance la primera iteracion será falsa

// -- SENSOR DE HUMEDAD
const int DHTPIN = A3;  // Uso analógico porque no me quedan más pines
DHT dht(DHTPIN, DHTTYPE);

// -- PRODUCTOS
// Me era mucho más cómodo trabajar con un struct
// pero podría haber implemenatdo otra manera
// era por comodidad
struct Product {
  const char* line;
  float price;
  int id;
  volatile bool active;
};

Product products[] = {{"i.CafeSolo", 1.00, 0, false},
                      {"ii.Cafe Cortado", 1.10, 1, false},
                      {"iii.Cafe Doble", 1.25, 2, false},
                      {"iv.Cafe Premium", 1.50, 3, false},
                      // Añado un espacio para que se vea mejor simplemente
                      {"v.Chocolate     ", 2.00, 4, false}};

size_t INITIAL_INDEX = 0;

// El size es importante para cuando trabaje con punteros
// sobre movimiento de joystick para navegar el menú
// Si no lo tengo en cuenta => undefined behaviour
volatile size_t PRODUCTS_SIZE =
    sizeof(products) / sizeof(products[INITIAL_INDEX]);

size_t current_product_index = INITIAL_INDEX;
Product* current_product = &products[INITIAL_INDEX];

const double CHANGING_VALUE_PRICE = 0.01;  // 1 cents.
const int RETIRAR_TIME = 3;
const int MAX_PRICE_CHAR = 10;

const int PREP_TIME_MIN = 4;
const int PREP_TIME_MAX = 8;

// -- ADMIN
struct Option {
  const char* line;
  int id;
  volatile bool active;
};

const Option admin_options[] = {{"i.Ver temperatura", 0, false},
                                {"ii.Ver distancia sensor", 1, false},
                                {"iii.Ver contador", 2, false},
                                {"iv.Modificar precios", 3, false}};

const size_t ADMIN_OPTIONS_SIZE =
    sizeof(admin_options) / sizeof(admin_options[INITIAL_INDEX]);

size_t current_admin_index = INITIAL_INDEX;
Option* current_admin_option = &admin_options[INITIAL_INDEX];

// -- VARIABLES GLOBALES
int product_or_admin =
    -1;  // Para la selección de las diferentes lecturas del joystick
volatile double temporary_storage_for_mod_price_non_confirmed = 0;

// Para un delay no bloqueante
unsigned long START_TIME = 0;
unsigned long last_check_time = 0;
unsigned long now = 0;

int preparation_time = 0;
int increasing_led_value = 0;  // Intensidad

const unsigned long SECOND_CONVERSION_FACTOR = 1000;

// -- LÓGICA
bool first_iteration = true;
bool first_iteration_product = true;
bool showHumidityOnce = true;

bool nonBlockingDelay(int target) {
  if (now - last_check_time >= SECOND_CONVERSION_FACTOR) {
    counter++;
    last_check_time = now;
  }

  if (counter >= target) {
    counter = 0;
    return true;
  }

  return false;
}

int getPulseTime() {
  return (LOW_TO_HIGH_TIME - HIGH_TO_LOW_TIME) / SECOND_CONVERSION_FACTOR;
}

void button_unpressed() {
  double pulseTime = getPulseTime();

  if (pulseTime >= TIME_RESET_SERVICE_INFIMO &&
      pulseTime <= TIME_RESET_SERVICE_SUPREMO && (currentState == SERVICE)) {
    currentState = SERVICE;
    previousState = IDLE;

    // Reseteo
    current_product_index = INITIAL_INDEX;
    current_product = &products[current_product_index];

    first_iteration = true;
    showHumidityOnce = true;

    displayProduct(current_product);
    is_idle = true;  // Para que no printee
  } else if (pulseTime > TIME_CHANGE_STATE_ADMIN) {
    if (currentState == ADMIN) {
      currentState = SERVICE;
      previousState = ADMIN;

      first_iteration = true;  // Sin esto no vuelve a comprobar persona!
      displayProduct(current_product);
    } else {
      currentState = ADMIN;
      previousState = SERVICE;
      first_iteration = true;
      first_iteration_product = true;

      current_admin_index = INITIAL_INDEX;
      current_admin_option = &admin_options[current_admin_index];

      display(current_admin_option->line);
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

// AHORA CON DEBOUNCE
void joystick_pressed() {
  unsigned long now = millis();

  // Comprobamos el tiempo entre el ruido
  if (now - last_joystick_press_time > JOYSTICK_DEBOUNCE_DELAY) {
    last_joystick_press_time = now;
    joystick_bool = true;
  }
  // Si es el tiempo es menor que el estipulado, es ruido
}

void setup() {
  Serial.begin(9600);
  wdt_disable();
  wdt_enable(WDTO_8S);
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
  double dis = pulseIn(PIN_ECHO, HIGH);
  return dis * METER_SOUND_SPEED_CONVERSION_FACTOR;
}

void display(const char* message) {
  // Uso const char* en lugar de String
  // para no alocar dinámicamente
  // ya que la RAM del microcontrolador Arduino
  // es muy limitada
  lcd.clear();

  lcd.setCursor(0, 0);
  for (int i = 0; i < MAX_CHARPTR_PER_SCREEN && message[i] != '\0'; i++) {
    lcd.print(message[i]);
  }

  int len = strlen(message);
  if (len > MAX_CHARPTR_PER_SCREEN) {
    lcd.setCursor(0, 1);
    for (int i = MAX_CHARPTR_PER_SCREEN;
         i < LCD_MAX_CHARS && message[i] != '\0'; i++) {
      lcd.print(message[i]);
    }
  }
}

void displayProduct(const Product* p) {
  char price_str[MAX_PRICE_CHAR];
  char msg[LCD_MAX_CHARS];

  // Para que no me de problemas de conversion busqué una función
  // helper
  dtostrf(p->price, 6, 2, price_str);
  snprintf(msg, sizeof(msg), "%s%seur", p->line, price_str);

  display(msg);
}

JoystickMove detect_move(int x, int y) {
  if (x >= CENTER_X_MIN && x <= CENTER_X_MAX && y >= CENTER_Y_MIN &&
      y <= CENTER_Y_MAX)
    return JS_CENTER;

  if (x > THRESHOLD_JU_XINFIMO && x < THRESHOLD_JU_XSUPREMO &&
      y == THRESHOLD_JU_Y)
    return JS_UP;
  if (x > THRESHOLD_JD_XINFIMO && x < THRESHOLD_JD_XSUPREMO &&
      y > THRESHOLD_JD_Y)
    return JS_DOWN;
  if (y > THRESHOLD_JLJR_YINFIMO && y < THRESHOLD_JLJR_YSUPREMO &&
      x == THRESHOLD_JL_X)
    return JS_LEFT;
  if (y > THRESHOLD_JLJR_YINFIMO && y < THRESHOLD_JLJR_YSUPREMO &&
      x == THRESHOLD_JR_X)
    return JS_RIGHT;

  return JS_CENTER;
}

void handle_service_move(JoystickMove move) {
  if (move == JS_UP && current_product_index > INITIAL_INDEX) {
    current_product_index--;
    current_product = &products[current_product_index];
    displayProduct(current_product);
  }

  if (move == JS_DOWN && current_product_index < PRODUCTS_SIZE - 1) {
    current_product_index++;
    current_product = &products[current_product_index];
    displayProduct(current_product);
  }
}

void handle_admin_move(JoystickMove move) {
  if (move == JS_UP && current_admin_index > INITIAL_INDEX &&
      !current_admin_option->active) {
    current_admin_index--;
    current_admin_option = &admin_options[current_admin_index];
    display(current_admin_option->line);
  }

  if (move == JS_DOWN && current_admin_index < ADMIN_OPTIONS_SIZE - 1 &&
      !current_admin_option->active) {
    current_admin_index++;
    current_admin_option = &admin_options[current_admin_index];
    display(current_admin_option->line);
  }

  if (move == JS_LEFT && current_admin_option->active) {
    current_admin_option->active = false;
    display(current_admin_option->line);
  }
}

void handle_prices_move(JoystickMove move) {
  if (move == JS_UP) {
    if (current_product->active) {
      current_product->price += CHANGING_VALUE_PRICE;
    } else {
      if (current_product_index > INITIAL_INDEX) {
        current_product_index--;
        current_product = &products[current_product_index];
      }
    }

    displayProduct(current_product);
  }

  if (move == JS_DOWN) {
    if (current_product->active) {
      current_product->price -= CHANGING_VALUE_PRICE;
    } else {
      if (current_product_index < PRODUCTS_SIZE - 1) {
        current_product_index++;
        current_product = &products[current_product_index];
      }
    }
    displayProduct(current_product);
  }

  if (move == JS_LEFT) {
    if (!current_product->active) {
      previousState = currentState;
      currentState = ADMIN;
      current_admin_option->active = false;
      display(current_admin_option->line);
    } else {
      current_product->price =
          temporary_storage_for_mod_price_non_confirmed;  // REVERT TO ORIGINAL
                                                          // VALUE BEFORE LAST
                                                          // MOD
      current_product->active = false;
      displayProduct(current_product);
    }
  }
}

void joystick_read(int choice) {
  int x = analogRead(Vx);
  int y = analogRead(Vy);

  JoystickMove move = detect_move(x, y);
  if (move == JS_CENTER) {
    is_idle = false;
    return;
  }
  if (is_idle) return;

  switch (currentState) {
    case SERVICE:
      handle_service_move(move);
      break;

    case ADMIN:
      handle_admin_move(move);
      break;

    case PRICES:
      handle_prices_move(move);
      break;
  }

  is_idle = true;  // Solo aceptamos cambios cuando el joystick
                   // pasa de centro a otra posición dentro de los threadholds
                   // especificados
}

void displayTemp() {
  if (nonBlockingDelay(1)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(dht.readTemperature());
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(dht.readHumidity());
  }
}

void displayDist() {
  if (nonBlockingDelay(1)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(get_distance());
  }
}

void seeCounter() {
  const int interval_time = 1;
  if (nonBlockingDelay(interval_time)) {
    int time = (now - START_TIME) / SECOND_CONVERSION_FACTOR;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(time);
  }
}

void modifyPrice() {
  if (first_iteration_product) {
    current_product_index = INITIAL_INDEX;
    current_product = &products[current_product_index];
    displayProduct(current_product);
    first_iteration_product = false;
  }
}

void retirarBebida() {
  if (now - last_check_time >= SECOND_CONVERSION_FACTOR) {
    counter++;
    last_check_time = now;
  }
  if (counter == RETIRAR_TIME) {
    displayProduct(current_product);
    analogWrite(LED2, LOW_VALUE_PWM);
    counter = 0;
    increasing_led_value = 0;
    current_product->active = false;
    previousState = currentState;
    currentState = SERVICE;
  }
}
void prepararBebida(int time) {
  if (now - last_check_time >= SECOND_CONVERSION_FACTOR) {
    counter++;
    increasing_led_value =
        increasing_led_value +
        HIGH_VALUE_PWM / time;  // HIGH = 255 tal que 255 / time = prop value
    analogWrite(LED2, increasing_led_value);
    last_check_time = now;
  }
  if (counter == time) {
    counter = 0;
    previousState = PREPARING;
    currentState = RETIRAR;
    display("RETIRE BEBIDA");
  }
}

void loop() {
  wdt_reset();  // A veces se bloquea en SERVICIO por las funcionalidades de
                // humidity
  now = millis();

  /*Necesitamos que estas funciones se ejecuten continuamente
  si es que están activas para mantener dinamismo
  y aprovechar la alta fecuencia del bucle*/
  if (led_bool) {
    if (led_start == 0) {
      display("CARGANDO");
      digitalWrite(LED1, HIGH);
      led_start = millis();
    }

    unsigned long now = millis();
    unsigned long elapsed = now - led_start;

    if (elapsed >= SECOND_CONVERSION_FACTOR) {
      digitalWrite(LED1, !digitalRead(LED1));
      led_start = now;

      if (digitalRead(LED1) == LOW) {
        counter++;
      }

      if (counter >= LED_BLINK_COUNT) {
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

  if (joystick_bool) {
    joystick_bool = false;

    if (currentState == ADMIN) {
      current_admin_option->active = true;
    } else if (currentState == SERVICE) {
      current_product->active = true;
    } else if (currentState == PRICES || currentState == SERVICE) {
      if (!current_product->active) {
        temporary_storage_for_mod_price_non_confirmed = current_product->price;
        current_product->active = true;
      } else {
        current_product->active = false;  // Salir de la seleccion
        displayProduct(current_product);  // Seguimos con el menú
      }
    }
  }

  // La máquina de estados siempre se encontrará en un estado
  switch (currentState) {
    case SERVICE: {
      product_or_admin = 1;

      // Add an if on if  ledes on, turn off
      analogWrite(LED1, LOW_VALUE_PWM);
      analogWrite(LED2, LOW_VALUE_PWM);
      double distance = get_distance();
      bool person_now_detected = (distance < PERSON_DETECTION_MAX_DISTANCE);

      unsigned long millisStart = millis();

      // Si el sensor de ultrasonidos ha tenido un cambio de estado
      if (person_now_detected != previously_detected_person) {
        // Este if creo que me sobra
        if (person_now_detected) {
          previousState = currentState;
          current_product_index = INITIAL_INDEX;
          current_product = &products[current_product_index];
          float h = dht.readHumidity();
          float t = dht.readTemperature();

          if (isnan(h) || isnan(t)) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Fallo lectura DHT");
          } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Temp: ");
            lcd.print(t);
            lcd.setCursor(0, 1);
            lcd.print("Hum: ");
            lcd.print(h);
          }
        } else {
          display("ESPERANDO CLIENTE");
          showHumidityOnce = true;
        }
        previously_detected_person = person_now_detected;
      }
      if (person_now_detected && showHumidityOnce) {
        const int time_hum_temp_action = 5;
        if (nonBlockingDelay(time_hum_temp_action)) {
          showHumidityOnce = false;
          displayProduct(current_product);
        }
      } else if (person_now_detected && !showHumidityOnce) {
        joystick_read(product_or_admin);

        if (current_product->active) {
          counter = 0;
          last_check_time = now;

          preparation_time = random(PREP_TIME_MIN, PREP_TIME_MAX);
          previousState = currentState;
          currentState = PREPARING;
          display("PREPARANDO CAFE...");
        }
      }
      break;
    }

    case PREPARING: {
      prepararBebida(preparation_time);
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
          analogWrite(LED1, HIGH_VALUE_PWM);
          analogWrite(LED2, HIGH_VALUE_PWM);
          current_admin_index = INITIAL_INDEX;
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
        if (first_iteration) {
          previousState = currentState;
          current_product_index = INITIAL_INDEX;
          current_product = &products[current_product_index];
          displayProduct(current_product);
          first_iteration = false;
        }
      }
      joystick_read(product_or_admin);

      previousState = currentState;
      break;
    }
  }
}
