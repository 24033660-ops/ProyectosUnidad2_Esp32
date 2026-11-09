#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// ASIGNACIÓN DE PINES
// ==========================================
#define LM35_PIN     34   // Sensor de temperatura LM35 (GPIO 34 - ADC1)
#define POT_MIN_PIN  32   // Potenciómetro 1: Ajusta T_min (GPIO 32)
#define POT_MAX_PIN  33   // Potenciómetro 2: Ajusta T_max (GPIO 33)

#define RELE_FOCO    2    // Relé 1: Foco / Calentador (GPIO 2 - D2)
#define RELE_PELTIER 15   // Relé 2: Peltier / Enfriador (GPIO 15 - D15)

#define LCD_SDA      21   // LCD I2C SDA
#define LCD_SCL      22   // LCD I2C SCL
#define LCD_ADDR     0x27 // Dirección I2C LCD

// Módulo de Relés Active LOW
#define RELE_ON      LOW
#define RELE_OFF     HIGH

// Limites configurados
const int TEMP_RANGO_MIN = 25; // Mínima temperatura elegible
const int TEMP_RANGO_MAX = 60; // Máxima temperatura elegible

// ==========================================
// OBJETOS Y VARIABLES
// ==========================================
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

float temperaturaActual = 0.0;
int tempMin = 25; // Ajustado por Potenciómetro 1
int tempMax = 60; // Ajustado por Potenciómetro 2

enum EstadoCiclo { CALENTANDO, ENFRIANDO };
EstadoCiclo estadoCiclo = CALENTANDO;

unsigned long ultimaLectura = 0;
const unsigned long INTERVALO_LECTURA = 300; // Refresco cada 300 ms

// Lectura promediada del LM35 para estabilidad
float leerLM35() {
  long sumaADC = 0;
  const int MUESTRAS = 30;

  for (int i = 0; i < MUESTRAS; i++) {
    sumaADC += analogRead(LM35_PIN);
    delayMicroseconds(100);
  }

  float adcPromedio = (float)sumaADC / MUESTRAS;
  float milivoltios = (adcPromedio / 4095.0) * 3300.0;
  return milivoltios / 10.0; // 10 mV = 1 °C
}

// Lectura de ambos potenciómetros
void leerPotenciometros() {
  int valorPot1 = analogRead(POT_MIN_PIN);
  int valorPot2 = analogRead(POT_MAX_PIN);

  // Mapear lecturas entre 25 °C y 60 °C
  tempMin = map(valorPot1, 0, 4095, TEMP_RANGO_MIN, TEMP_RANGO_MAX);
  tempMax = map(valorPot2, 0, 4095, TEMP_RANGO_MIN, TEMP_RANGO_MAX);

  // Protección: Asegurar que T_max siempre sea al menos 1°C superior a T_min
  if (tempMax <= tempMin) {
    tempMax = tempMin + 1;
  }
}

// Lógica de control del ciclo del horno (Histéresis)
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

  // Control físico de salidas
  if (estadoCiclo == CALENTANDO) {
    digitalWrite(RELE_FOCO, RELE_ON);     // Encender Foco
    digitalWrite(RELE_PELTIER, RELE_OFF); // Apagar Peltier
  } else {
    digitalWrite(RELE_FOCO, RELE_OFF);   // Apagar Foco
    digitalWrite(RELE_PELTIER, RELE_ON);  // Encender Peltier
  }
}

// Pantalla LCD
void actualizarLCD() {
  char linea0[17];
  char linea1[17];

  // Línea 0: Temperatura actual del LM35 y estado del ciclo [CAL/ENF]
  snprintf(linea0, sizeof(linea0), "T:%4.1f%cC   [%s]", 
           temperaturaActual, (char)223, estadoCiclo == CALENTANDO ? "CAL" : "ENF");

  // Línea 1: Umbral Min (Pot 1) y Umbral Max (Pot 2)
  snprintf(linea1, sizeof(linea1), "Min:%2d%cC  Max:%2d%cC",
           tempMin, (char)223, tempMax, (char)223);

  lcd.setCursor(0, 0);
  lcd.print(linea0);
  lcd.setCursor(0, 1);
  lcd.print(linea1);
}

void setup() {
  Serial.begin(115200);

  pinMode(LM35_PIN, INPUT);
  pinMode(POT_MIN_PIN, INPUT);
  pinMode(POT_MAX_PIN, INPUT);

  pinMode(RELE_FOCO, OUTPUT);
  pinMode(RELE_PELTIER, OUTPUT);
  digitalWrite(RELE_FOCO, RELE_OFF);
  digitalWrite(RELE_PELTIER, RELE_OFF);

  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(" Horno 2 Pots   ");
  lcd.setCursor(0, 1);
  lcd.print(" Iniciando...   ");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long ahora = millis();

  if (ahora - ultimaLectura >= INTERVALO_LECTURA) {
    ultimaLectura = ahora;

    temperaturaActual = leerLM35();
    leerPotenciometros();
    controlarCiclo();
    actualizarLCD();

    Serial.printf("LM35: %.1f C | T_Min (Pot1): %d C | T_Max (Pot2): %d C | Estado: %s\n",
                  temperaturaActual, tempMin, tempMax,
                  estadoCiclo == CALENTANDO ? "CALENTANDO" : "ENFRIANDO");
  }
}
