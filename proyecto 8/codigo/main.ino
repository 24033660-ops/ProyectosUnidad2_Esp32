#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>

#define DS18B20_PIN 4
#define SERVO_PIN   18
#define LCD_SDA     21
#define LCD_SCL     22
#define LCD_ADDR    0x27

// --- RE-CALIBRACIÓN MECÁNICA ---
// Si a los 90° apenas enciende, ese debe ser tu punto de partida (0% de potencia).
const int ANGULO_MIN = 90;  // 0%  de control -> 90° (Umbral de encendido)
const int ANGULO_MAX = 180; // 100% de control -> 180° (Potencia máxima)

const byte FILAS = 4;
const byte COLUMNAS = 4;

char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinesFilas[FILAS]       = {13, 12, 14, 27};
byte pinesColumnas[COLUMNAS] = {26, 25, 33, 32};

Keypad teclado = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);


OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Servo dimmerServo;

float T_medida = 0.0;
float T_ref = 40.0;         // Setpoint inicial (°C)
float Kp = 5.0;             // Ganancia Proporcional (%/°C)
float errorTemp = 0.0;      // e(t) = T_ref - T_medida
float accionControl = 0.0;  // u(t) en porcentaje (0% a 100%)
int anguloServo = ANGULO_MIN;

enum ModoSistema { MODO_NORMAL, MODO_EDIT_SP, MODO_EDIT_KP };
ModoSistema modoActual = MODO_NORMAL;

String bufferTeclado = "";
bool vistaAlternativa = false;

unsigned long ultimaLecturaTemp = 0;
const unsigned long INTERVALO_TEMP = 500;

void calcularControlProporcional() {
  errorTemp = T_ref - T_medida;

  // Ley de control proporcional: u(t) = Kp * e(t)
  float u_calculado = Kp * errorTemp;

  // Saturación del controlador (0% a 100%)
  if (u_calculado < 0.0) {
    accionControl = 0.0;
  } else if (u_calculado > 100.0) {
    accionControl = 100.0;
  } else {
    accionControl = u_calculado;
  }

  // Mapeo lineal considerando la zona muerta (90° a 180°)
  // θ = 90 + (u / 100) * (180 - 90)
  float anguloFlotante = ANGULO_MIN + (accionControl / 100.0) * (ANGULO_MAX - ANGULO_MIN);
  anguloServo = (int)constrain(anguloFlotante, (float)ANGULO_MIN, (float)ANGULO_MAX);

  // Posicionar servomotor
  dimmerServo.write(anguloServo);
}

void actualizarLCD() {
  char linea0[17];
  char linea1[17];

  if (modoActual == MODO_EDIT_SP) {
    snprintf(linea0, sizeof(linea0), "Set T_ref (°C): ");
    snprintf(linea1, sizeof(linea1), "> %-14s", bufferTeclado.c_str());
  } 
  else if (modoActual == MODO_EDIT_KP) {
    snprintf(linea0, sizeof(linea0), "Set Kp (%%/°C):  ");
    snprintf(linea1, sizeof(linea1), "> %-14s", bufferTeclado.c_str());
  } 
  else { 
    if (!vistaAlternativa) {
      snprintf(linea0, sizeof(linea0), "T:%4.1fC SP:%4.1f", T_medida, T_ref);
      snprintf(linea1, sizeof(linea1), "E:%4.1fC OUT:%3d%%", errorTemp, (int)accionControl);
    } else {
      snprintf(linea0, sizeof(linea0), "Kp: %4.1f %%/C   ", Kp);
      snprintf(linea1, sizeof(linea1), "SERVO: %3d DEG  ", anguloServo);
    }
  }

  lcd.setCursor(0, 0);
  lcd.print(linea0);
  lcd.setCursor(0, 1);
  lcd.print(linea1);
}

void procesarTeclado(char tecla) {
  if (tecla == 'A') {
    modoActual = MODO_EDIT_SP;
    bufferTeclado = "";
  } 
  else if (tecla == 'B') {
    modoActual = MODO_EDIT_KP;
    bufferTeclado = "";
  } 
  else if (tecla == 'C') {
    bufferTeclado = "";
  } 
  else if (tecla == 'D') {
    if (modoActual != MODO_NORMAL) {
      modoActual = MODO_NORMAL;
      bufferTeclado = "";
    } else {
      vistaAlternativa = !vistaAlternativa;
    }
  } 
  else if (tecla == '#') {
    if (bufferTeclado.length() > 0) {
      float valor = bufferTeclado.toFloat();
      if (modoActual == MODO_EDIT_SP) {
        if (valor >= 0.0 && valor <= 100.0) T_ref = valor;
      } else if (modoActual == MODO_EDIT_KP) {
        if (valor >= 0.0 && valor <= 50.0) Kp = valor;
      }
    }
    modoActual = MODO_NORMAL;
    bufferTeclado = "";
  } 
  else if ((tecla >= '0' && tecla <= '9') || tecla == '*') {
    if (modoActual != MODO_NORMAL) {
      char c = (tecla == '*') ? '.' : tecla;
      if (c == '.' && bufferTeclado.indexOf('.') != -1) return;
      if (bufferTeclado.length() < 6) bufferTeclado += c;
    }
  }
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  dimmerServo.setPeriodHertz(50);
  dimmerServo.attach(SERVO_PIN, 500, 2400);
  dimmerServo.write(ANGULO_MIN); // Inicia en 90° (foco apagado)

  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();

  sensors.begin();
  sensors.setResolution(9);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  lcd.setCursor(0, 0);
  lcd.print(" Control P Temp ");
  lcd.setCursor(0, 1);
  lcd.print(" ESP32 + Keypad ");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long ahora = millis();

  char tecla = teclado.getKey();
  if (tecla != NO_KEY) {
    procesarTeclado(tecla);
    actualizarLCD();
  }

  if (ahora - ultimaLecturaTemp >= INTERVALO_TEMP) {
    ultimaLecturaTemp = ahora;

    float tempRead = sensors.getTempCByIndex(0);
    if (tempRead > -55.0 && tempRead < 125.0) {
      T_medida = tempRead;
    }
    sensors.requestTemperatures();

    calcularControlProporcional();
    actualizarLCD();

    Serial.printf("T: %.2f C | SP: %.2f C | Error: %.2f C | OUT: %.1f %% | Servo: %d deg\n",
                  T_medida, T_ref, errorTemp, accionControl, anguloServo);
  }
}
