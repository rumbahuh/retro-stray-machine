const int PIN_VRx = A0;
const int PIN_VRy = A1;
const int PIN_MS = 9;
const int intbuttonPIN = 3;

volatile int xVal = 0;
volatile int yVal = 0;
volatile int buttonState = 0;

void setup() {
  Serial.begin(9600);
  // Necesitamos leer de estos pines porque cambian
  // de valores todo el rato
  pinMode(PIN_VRx, INPUT);
  pinMode(PIN_VRy, INPUT);
  pinMode(intbuttonPIN, INPUT_PULLUP); // Está en 1 el botón
  // ex: X: 508 | Y: 510 | Button: 1

}

void loop() {
  xVal = analogRead(PIN_VRx);
  yVal = analogRead(PIN_VRy);
  buttonState = digitalRead(intbuttonPIN);

  // X: XXX | Y: YYY | Button: 0/1
  Serial.print("X: ");
  Serial.print(xVal);
  Serial.print(" | Y: ");
  Serial.print(yVal);
  Serial.print(" | Button: ");
  // ln para que haga salto de línea
  Serial.println(buttonState);

  delay(100);
}
