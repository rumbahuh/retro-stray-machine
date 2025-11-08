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
const int ULTRASONIC_PIN = 9;
// Preguntar a profesor sobre qué pines usar mejor
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
  // Por qué INPUT_PULLUP
  // EN el momento en el que pulsas el botón
  // read de pin, se activa la interrupción start()
  pinMode(pin, INPUT_PULLUP);
  // Queremos leer del pin así que le pedimos INPUT de config
  pinMode(ULTRASONIC_PIN, INPUT);
  // Por qué FALLING
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
            /*lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("On service");*/
            if (digitalRead(ULTRASONIC_PIN)) {
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("DETECTED");
            } else {
              lcd.clear();
              lcd.setCursor(0,0);
              // La frase es muy larga jaja
              lcd.print("ESPERANDO CLIENTE");
            }
            // currentState = IDLE;
            break;
    }
}
