#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>

// ==========================================
// 1. PINES Y CALIBRACIÓN DE ZONA MUERTA
// ==========================================
#define DS18B20_PIN 4
#define SERVO_PIN   18
#define LCD_SDA     21
#define LCD_SCL     22
#define LCD_ADDR    0x27

// Calibración mecánica del Dimmer (Zona Muerta sintonizada previamente)
const int ANGULO_MIN = 80;  // 0%  de potencia PID -> 80° (Umbral inicial)
const int ANGULO_MAX = 180; // 100% de potencia PID -> 180° (Máxima potencia)

// ==========================================
// 2. CONFIGURACIÓN TECLADO MATRICIAL 4x4
// ==========================================
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

// ==========================================
// 3. OBJETOS Y VARIABLES PID
// ==========================================
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
Servo dimmerServo;

// Variables de proceso y referencia
float T_medida = 0.0;
float T_medida_anterior = 0.0;
float T_ref = 40.0;         // Setpoint inicial (°C)
float errorTemp = 0.0;

// Ganancias PID por defecto
float Kp = 10.0;            // Proporcional (%/°C)
float Ki = 0.2;             // Integral (%/(°C*s))
float Kd = 1.0;             // Derivativa (%*s/°C)

// Términos discretos del PID
float P_term = 0.0;
float I_term = 0.0;
float D_term = 0.0;
float accionPID = 0.0;      // Salida en % (0 a 100%)
int anguloServo = ANGULO_MIN;

// Periodo de Muestreo (Ts = 1 segundo)
const float Ts = 1.0; 
unsigned long ultimaLecturaPID = 0;

// Máquina de estados para edición
enum ModoSistema { MODO_NORMAL, EDIT_SP, EDIT_KP, EDIT_KI, EDIT_KD };
ModoSistema modoActual = MODO_NORMAL;

String bufferTeclado = "";
bool vistaAlternativaLCD = false;
unsigned long ultimoCambioPantalla = 0;

// ==========================================
// 4. ALGORITMO PID DISCRETO CON ANTI-WINDUP
// ==========================================
void calcularPID() {
  errorTemp = T_ref - T_medida;

  // 1. Término Proporcional
  P_term = Kp * errorTemp;

  // 2. Término Derivativo basado en la Medición (Evita el "Derivative Kick")
  float dT = (T_medida - T_medida_anterior) / Ts;
  D_term = -Kd * dT;

  // 3. Acumulación Integral tentativa
  float I_tentativo = I_term + (Ki * errorTemp * Ts);

  // 4. Salida sin saturar
  float u_unclamped = P_term + I_tentativo + D_term;

  // 5. Anti-Windup Clamping (Detiene la acumulación integral al saturar)
  if (u_unclamped >= 0.0 && u_unclamped <= 100.0) {
    I_term = I_tentativo; // Solo actualiza si está en el rango lineal
  } else {
    I_term = constrain(I_term, -50.0, 50.0); // Limita el acumulado
  }

  // 6. Saturación de la salida (0% - 100%)
  accionPID = constrain(u_unclamped, 0.0, 100.0);

  // 7. Conversión a ángulo del servomotor
  float anguloFlotante = ANGULO_MIN + (accionPID / 100.0) * (ANGULO_MAX - ANGULO_MIN);
  anguloServo = (int)constrain(anguloFlotante, (float)ANGULO_MIN, (float)ANGULO_MAX);

  // Actualizar actuador físico
  dimmerServo.write(anguloServo);

  // Guardar medición previa para el cálculo derivativo
  T_medida_anterior = T_medida;
}

// ==========================================
// 5. INTERFAZ DE USUARIO LCD
// ==========================================
void actualizarLCD() {
  char linea0[17];
  char linea1[17];

  switch (modoActual) {
    case EDIT_SP:
      snprintf(linea0, sizeof(linea0), "Set T_ref (C):  ");
      snprintf(linea1, sizeof(linea1), "> %-14s", bufferTeclado.c_str());
      break;
    case EDIT_KP:
      snprintf(linea0, sizeof(linea0), "Set Kp:         ");
      snprintf(linea1, sizeof(linea1), "> %-14s", bufferTeclado.c_str());
      break;
    case EDIT_KI:
      snprintf(linea0, sizeof(linea0), "Set Ki:         ");
      snprintf(linea1, sizeof(linea1), "> %-14s", bufferTeclado.c_str());
      break;
    case EDIT_KD:
      snprintf(linea0, sizeof(linea0), "Set Kd:         ");
      snprintf(linea1, sizeof(linea1), "> %-14s", bufferTeclado.c_str());
      break;
    case MODO_NORMAL:
    default:
      if (!vistaAlternativaLCD) {
        // Vista 1: Control (T_medida, Setpoint, Error, OUT%)
        snprintf(linea0, sizeof(linea0), "T:%4.1fC SP:%4.1f", T_medida, T_ref);
        snprintf(linea1, sizeof(linea1), "E:%4.1fC OUT:%3d%%", errorTemp, (int)accionPID);
      } else {
        // Vista 2: Diagnóstico (Ganancias PID y Posición Servo)
        snprintf(linea0, sizeof(linea0), "P:%.1f I:%.1f D:%.1f", Kp, Ki, Kd);
        snprintf(linea1, sizeof(linea1), "SERVO: %3d DEG  ", anguloServo);
      }
      break;
  }

  lcd.setCursor(0, 0);
  lcd.print(linea0);
  lcd.setCursor(0, 1);
  lcd.print(linea1);
}

void procesarTeclado(char tecla) {
  if (tecla == 'A') {
    modoActual = EDIT_SP;
    bufferTeclado = "";
  } else if (tecla == 'B') {
    modoActual = EDIT_KP;
    bufferTeclado = "";
  } else if (tecla == 'C') {
    modoActual = EDIT_KI;
    bufferTeclado = "";
  } else if (tecla == 'D') {
    modoActual = EDIT_KD;
    bufferTeclado = "";
  } else if (tecla == '#') { // Confirmar e ingresar el parámetro
    if (bufferTeclado.length() > 0) {
      float val = bufferTeclado.toFloat();
      if (modoActual == EDIT_SP && val >= 0.0 && val <= 100.0) T_ref = val;
      else if (modoActual == EDIT_KP && val >= 0.0 && val <= 100.0) Kp = val;
      else if (modoActual == EDIT_KI && val >= 0.0 && val <= 50.0)  Ki = val;
      else if (modoActual == EDIT_KD && val >= 0.0 && val <= 50.0)  Kd = val;
    }
    modoActual = MODO_NORMAL;
    bufferTeclado = "";
  } else if (tecla == '*') {
    if (modoActual != MODO_NORMAL) {
      // Punto decimal
      if (bufferTeclado.indexOf('.') == -1 && bufferTeclado.length() < 6) {
        bufferTeclado += '.';
      }
    } else {
      // Alternar manualmente la vista en modo normal
      vistaAlternativaLCD = !vistaAlternativaLCD;
    }
  } else if (tecla >= '0' && tecla <= '9') {
    if (modoActual != MODO_NORMAL && bufferTeclado.length() < 6) {
      bufferTeclado += tecla;
    }
  }
}

// ==========================================
// 6. SETUP Y LOOP
// ==========================================
void setup() {
  Serial.begin(115200);

  // Inicialización Servo
  ESP32PWM::allocateTimer(0);
  dimmerServo.setPeriodHertz(50);
  dimmerServo.attach(SERVO_PIN, 500, 2400);
  dimmerServo.write(ANGULO_MIN);

  // Inicialización LCD I2C
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();

  // Inicialización DS18B20
  sensors.begin();
  sensors.setResolution(10); // Resolución a 10 bits
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  // Guardar medición inicial para evitar saltos derivativos en t=0
  float tempInit = sensors.getTempCByIndex(0);
  if (tempInit > -55.0 && tempInit < 125.0) {
    T_medida = tempInit;
    T_medida_anterior = tempInit;
  }

  lcd.setCursor(0, 0);
  lcd.print(" Control PID    ");
  lcd.setCursor(0, 1);
  lcd.print(" Temperatura    ");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long ahora = millis();

  // 1. Lectura del Teclado Matricial
  char tecla = teclado.getKey();
  if (tecla != NO_KEY) {
    procesarTeclado(tecla);
    actualizarLCD();
  }

  // 2. Alternancia automática de vistas LCD cada 4 segundos (solo en MODO_NORMAL)
  if (modoActual == MODO_NORMAL && (ahora - ultimoCambioPantalla >= 4000)) {
    vistaAlternativaLCD = !vistaAlternativaLCD;
    ultimoCambioPantalla = ahora;
    actualizarLCD();
  }

  // 3. Lazo de Control PID (Periodo de Muestreo Ts = 1.0 s)
  if (ahora - ultimaLecturaPID >= (unsigned long)(Ts * 1000)) {
    ultimaLecturaPID = ahora;

    float tempRead = sensors.getTempCByIndex(0);
    if (tempRead > -55.0 && tempRead < 125.0) {
      T_medida = tempRead;
    }
    sensors.requestTemperatures(); // Solicitar siguiente conversión para el próximo ciclo

    // Ejecutar algoritmo de control PID
    calcularPID();
    actualizarLCD();

    // Monitor Serie (Ideal para graficar en Serial Plotter)
    Serial.printf("T_medida:%.2f,T_ref:%.2f,Error:%.2f,P:%.2f,I:%.2f,D:%.2f,OUT:%.1f,Servo:%d\n",
                  T_medida, T_ref, errorTemp, P_term, I_term, D_term, accionPID, anguloServo);
  }
}
