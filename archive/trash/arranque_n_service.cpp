/*
Authoress: Rebeca Castilla
Date: 7-8/Nov/2025
Goal: Generar comportamiento de máquina expendedora
      usando arduino y ciertos sensores y actuadores
Notas: Añadiré comentarios para que los comentarios que añadiré
       a futuro en el README.md.
       Está mal tabulado sobre el estilo de C++, no tengo tests.
*/

#include <LiquidCrystal.h>

const int LED1 = 13; // Como en clase
const int pin = 2; // Lectura - error de int0, no convierte, por qué
const int PIN_TRIG = 9;
const int PIN_ECHO = 10;

const double METER_LIGHT_SPEED_CONVERSION_FACTOR = 0.0001715;
// Preguntar a profesor sobre qué pines usar mejor
LiquidCrystal lcd(12, 11, 5, 4, 3, 7);

volatile bool previously_detected_person = false;
volatile bool first_iteration = true;

enum State {IDLE, START, SERVICE};
volatile State currentState = IDLE;

void start()
{
  currentState = START;
}

void setup()
{
  // Led actuator
  pinMode(LED1, OUTPUT);
  // Por qué INPUT_PULLUP
  
  // Button actuator
  // Why pullup
  pinMode(pin, INPUT_PULLUP);
  
  // Ultrasonic Sensor
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  
  // Por qué FALLING
  // En el momento en el que pulsas el botón
  // leemos de pin, se activa la interrupción start()
  attachInterrupt(digitalPinToInterrupt(pin), start, FALLING);
  /* No queremos usar interrupciones para el ultrasonic
  porque no puedo asignar una interrupcion para cuando está
  la persona a un metro y cuando no. Como tampoco puedo hacer
  un digital read para no afectar a la velocidad del programa
  considero que es mejor añadir un if de lectura del pin..
  aunque me van a dar falsos positivos. Me quiere sonar que en clase
  lo mencionamos.
  */
  lcd.begin(16, 2);
}

double get_distance()
{
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  
  double dis = pulseIn(PIN_ECHO, HIGH);
  
  return dis * METER_LIGHT_SPEED_CONVERSION_FACTOR;
}

void loop() {
    switch(currentState) {
        case START:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("CARGANDO ...");
            for (int i=0; i<3; i++) {
                digitalWrite(LED1, HIGH);
                // Por qué delay() y no TimeOne
                delay(1000);
                digitalWrite(LED1, LOW);
                delay(1000);
            }
            // Aquí añadir watchdog reset
            currentState = SERVICE;
            break;

        case SERVICE:
            bool person_now_detected = false;
      		double distance = get_distance();
            if (distance < 1) {
              person_now_detected = true;
            }

            if (first_iteration || person_now_detected != previously_detected_person) {
              lcd.clear();
              lcd.setCursor(0,0);
              
              if (person_now_detected) {
                lcd.print("DETECTED");
              } else {
                lcd.print("ESPERANDO CLIENTE");
              }
              
              previously_detected_person = person_now_detected;
              first_iteration = false;
            }
      
            break;
    }
}
