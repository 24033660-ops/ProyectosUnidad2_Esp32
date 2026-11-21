#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ==========================================
// ASIGNACIÓN DE PINES
// ==========================================
#define DS18B20_PIN  4    // Sensor DS18B20 (GPIO 4)
#define BTN_SEL_PIN  25   // Botón para conmutar entre Min y Max (GPIO 25)

#define RELE_FOCO    2    // Relé Foco / Calentador (GPIO 2 - D2)
#define RELE_PELTIER 15   // Relé Peltier / Enfriador (GPIO 15 - D15)

#define LCD_SDA      21   // LCD I2C SDA
#define LCD_SCL      22   // LCD I2C SCL
#define LCD_ADDR     0x27 // Dirección I2C LCD

// Módulo de Relés Active LOW
#define RELE_ON      LOW
#define RELE_OFF     HIGH

// ==========================================
// CONFIGURACIÓN DEL TECLADO MATRICIAL 4x4 (Pines sin 16 ni 17)
// ==========================================
const byte FILAS = 4;
const byte COLUMNAS = 4;

char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinesFilas[FILAS]       = {19, 18, 5, 27}; // F1, F2, F3, F4 (Usa GPIO 27)
byte pinesColumnas[COLUMNAS] = {32, 14, 23, 26}; // C1, C2, C3, C4 (Usa GPIO 32)

Keypad teclado = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);

// ==========================================
// OBJETOS Y VARIABLES DE ESTADO
// ==========================================
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

float temperaturaActual = 0.0;
int tempMin = 30; // Umbral Mínimo por defecto
int tempMax = 60; // Umbral Máximo por defecto

// 0 = Editando Temp Mínima, 1 = Editando Temp Máxima
uint8_t limiteSeleccionado = 0; 

String bufferTeclado = ""; // Almacena los dígitos ingresados

enum EstadoCiclo { CALENTANDO, ENFRIANDO };
EstadoCiclo estadoCiclo = CALENTANDO;

// Control de tiempos y antirrebote
unsigned long ultimaLecturaTemp = 0;
const unsigned long INTERVALO_TEMP = 1000; // Lectura cada 1 segundo

bool estadoAnteriorBtn = HIGH;
unsigned long ultimoTiempoBoton = 0;
const unsigned long DEBOUNCE_MS = 180;

// Lógica de histéresis del horno
void controlarCiclo() {
  if (estadoCiclo == CALENTANDO) {
    if (temperaturaActual >= tempMax) {
      estadoCiclo = ENFRIANDO;
    }
  } else if (estadoCiclo == ENFRIANDO) {
    if (temperaturaActual <= tempMin) {
      estadoCiclo = CALENTANDO;
    }
  }

  if (estadoCiclo == CALENTANDO) {
    digitalWrite(RELE_FOCO, RELE_ON);
    digitalWrite(RELE_PELTIER, RELE_OFF);
  } else {
    digitalWrite(RELE_FOCO, RELE_OFF);
    digitalWrite(RELE_PELTIER, RELE_ON);
  }
}

// Actualización del display LCD
void actualizarLCD() {
  char linea0[17];
  char linea1[17];

  // Línea 0: Temperatura DS18B20 y estado [CAL/ENF]
  if (temperaturaActual < -50) {
    snprintf(linea0, sizeof(linea0), "T: Error   [%s]", 
             estadoCiclo == CALENTANDO ? "CAL" : "ENF");
  } else {
    snprintf(linea0, sizeof(linea0), "T:%4.1f%cC   [%s]", 
             temperaturaActual, (char)223, estadoCiclo == CALENTANDO ? "CAL" : "ENF");
  }

  // Línea 1: Umbrales o dígitos marcados
  if (bufferTeclado.length() > 0) {
    snprintf(linea1, sizeof(linea1), "Ingresando: %-4s", bufferTeclado.c_str());
  } else {
    snprintf(linea1, sizeof(linea1), "%cMin:%2d%cC %cMax:%2d%cC",
             limiteSeleccionado == 0 ? '*' : ' ', tempMin, (char)223,
             limiteSeleccionado == 1 ? '*' : ' ', tempMax, (char)223);
  }

  lcd.setCursor(0, 0);
  lcd.print(linea0);
  lcd.setCursor(0, 1);
  lcd.print(linea1);
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_SEL_PIN, INPUT_PULLUP);

  pinMode(RELE_FOCO, OUTPUT);
  pinMode(RELE_PELTIER, OUTPUT);
  digitalWrite(RELE_FOCO, RELE_OFF);
  digitalWrite(RELE_PELTIER, RELE_OFF);

  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();

  // Inicialización DS18B20
  sensors.begin();
  sensors.setResolution(9);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  lcd.setCursor(0, 0);
  lcd.print(" Horno DS18B20  ");
  lcd.setCursor(0, 1);
  lcd.print(" Keypad 4x4     ");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long ahora = millis();

  // 1. BOTÓN SELECTOR (GPIO 25)
  bool btnActual = digitalRead(BTN_SEL_PIN);
  if (ahora - ultimoTiempoBoton >= DEBOUNCE_MS) {
    if (btnActual == LOW && estadoAnteriorBtn == HIGH) {
      limiteSeleccionado = (limiteSeleccionado == 0) ? 1 : 0;
      bufferTeclado = ""; 
      ultimoTiempoBoton = ahora;
    }
  }
  estadoAnteriorBtn = btnActual;

  // 2. TECLADO MATRICIAL
  char tecla = teclado.getKey();
  if (tecla != NO_KEY) {
    if (tecla >= '0' && tecla <= '9') {
      if (bufferTeclado.length() < 3) {
        bufferTeclado += tecla;
      }
    } 
    else if (tecla == '#') { // Confirmar e ingresar el valor
      if (bufferTeclado.length() > 0) {
        int valorIngresado = bufferTeclado.toInt();
        
        if (limiteSeleccionado == 0) {
          if (valorIngresado < tempMax) tempMin = valorIngresado;
        } else {
          if (valorIngresado > tempMin) tempMax = valorIngresado;
        }
        bufferTeclado = "";
      }
    } 
    else if (tecla == '*') { // Borrar número
      bufferTeclado = "";
    }
    actualizarLCD();
  }

  // 3. LECTURA DEL SENSOR Y CONTROL
  if (ahora - ultimaLecturaTemp >= INTERVALO_TEMP) {
    float tempRead = sensors.getTempCByIndex(0);
    if (tempRead > -55.0 && tempRead < 125.0) {
      temperaturaActual = tempRead;
    }
    sensors.requestTemperatures();
    ultimaLecturaTemp = ahora;

    controlarCiclo();
    actualizarLCD();
  }
}
