/*
  MASCARA DE TERROR - version completa
  - Servo SG90        : mueve la mandibula
  - 4 LEDs            : ojos (2 por ojo)
  - HC-SR501 (PIR)    : detecta movimiento y activa todo
  - DFPlayer Mini     : (MP3-TF-16P) reproduce la risa

  Libreria necesaria: "DFRobotDFPlayerMini" (Administrador de librerias)
  Audio: microSD en FAT32 con el archivo 0001.mp3 en la raiz.
*/

#include <Servo.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ---------- PINES ----------
const int PIN_SERVO = 9;
const int PIN_PIR   = 7;
const int LEDS[4]   = {3, 5, 6, 11};   // pines con PWM (~)
SoftwareSerial dfSerial(2, 4);         // RX=2 (<- TX del DFPlayer), TX=4 (-> RX del DFPlayer)

// ---------- AJUSTES ----------
const int BOCA_CERRADA = 90;   // angulo con la boca cerrada
const int BOCA_ABIERTA = 50;   // angulo con la boca abierta (ajusta a tu mascara)
const int BRILLO_REPOSO = 0;   // 0 = ojos apagados en espera (prueba 10-20 para brillo tenue)
const int VOLUMEN = 27;        // 0 a 30
const unsigned long DURACION_SUSTO = 5000;   // ms (ponlo igual a lo que dure tu audio)
const unsigned long ENFRIAMIENTO   = 3000;   // ms de pausa despues del susto

Servo mandibula;
DFRobotDFPlayerMini reproductor;
bool audioListo = false;

void ojos(int brillo) {
  for (int i = 0; i < 4; i++) analogWrite(LEDS[i], brillo);
}

void setup() {
  Serial.begin(9600);
  dfSerial.begin(9600);

  pinMode(PIN_PIR, INPUT);
  for (int i = 0; i < 4; i++) pinMode(LEDS[i], OUTPUT);

  mandibula.attach(PIN_SERVO);
  mandibula.write(BOCA_CERRADA);
  randomSeed(analogRead(A0));

  // Iniciar el DFPlayer
  if (reproductor.begin(dfSerial)) {
    reproductor.volume(VOLUMEN);
    audioListo = true;
    Serial.println(F("DFPlayer listo"));
  } else {
    Serial.println(F("Error: revisa cables del DFPlayer y la microSD"));
  }

  // El PIR necesita ~30 s para calibrarse: los ojos parpadean mientras tanto
  for (int i = 0; i < 30; i++) {
    ojos(i % 2 ? 30 : 0);
    delay(1000);
  }
  ojos(BRILLO_REPOSO);
}

void loop() {
  ojos(BRILLO_REPOSO);

  if (digitalRead(PIN_PIR) == HIGH) {
    susto();
  }
  delay(50);
}

void susto() {
  if (audioListo) reproductor.play(1);   // reproduce 0001.mp3

  unsigned long inicio = millis();

  while (millis() - inicio < DURACION_SUSTO) {
    // Ojos a tope con parpadeos aleatorios
    for (int i = 0; i < 4; i++) {
      analogWrite(LEDS[i], random(0, 3) == 0 ? 80 : 255);
    }

    // Boca abre y cierra de forma irregular
    mandibula.write(BOCA_ABIERTA + random(0, 15));
    delay(random(100, 220));
    mandibula.write(BOCA_CERRADA - random(0, 10));
    delay(random(80, 180));
  }

  if (audioListo) reproductor.stop();
  mandibula.write(BOCA_CERRADA);
  ojos(BRILLO_REPOSO);
  delay(ENFRIAMIENTO);
}
