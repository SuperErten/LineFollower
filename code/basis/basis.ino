#include <WiFi.h>
#include <WebServer.h>
#include <QTRSensors.h>
#include <Preferences.h>

constexpr uint8_t PIN_DI_START_STOP = 13;
constexpr uint8_t PIN_DO_LED_RED    = 14;
constexpr uint8_t PIN_DO_LED_GREEN  = 27;

QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

volatile bool interruptTriggerd = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

const char* ssid     = "Tibo123";
const char* password = "876543210";

volatile bool stateRobot = 0;
volatile float P = 0;
volatile float I = 0;
volatile float D = 0;

WebServer server(80);

Preferences prefs;

//Maar één core kan globaal data aanpassen.
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR toggleState() {
    interruptTriggerd = 1;
}

void updateLEDs() {

  digitalWrite(PIN_DO_LED_RED,   !stateRobot);
  digitalWrite(PIN_DO_LED_GREEN,  stateRobot);
}

void readValuesQTR() {

    uint16_t position = qtr.readLineBlack(sensorValues);

  for (uint8_t i = 0; i < SensorCount; i++) //Later Verwijderen
  {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }
  Serial.println(position);

  vTaskDelay(250 / portTICK_PERIOD_MS);
}

void calibrateQTR() {

  digitalWrite(LED_BUILTIN, 1);

  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
  }
  digitalWrite(LED_BUILTIN, 0); 
}

void startRobot() {

  readValuesQTR();
}

void setup() {

  Serial.begin(115200);

  prefs.begin("pid", false);
  P = prefs.getFloat("P", 0.0);
  I = prefs.getFloat("I", 0.0);
  D = prefs.getFloat("D", 0.0);
  prefs.end(); 
  
  pinMode(PIN_DI_START_STOP, INPUT_PULLUP);
  pinMode(PIN_DO_LED_GREEN, OUTPUT);
  pinMode(PIN_DO_LED_RED , OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){26, 36, 39, 34, 35, 32, 33, 25}, SensorCount); // 

  attachInterrupt(digitalPinToInterrupt(PIN_DI_START_STOP), toggleState, FALLING);
  updateLEDs();

  // Draaien op 2 aparte core's wifi o en main op 1
  xTaskCreatePinnedToCore(wifiLoop,  "wifiLoop",  8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mainLoop, "mainLoop", 4096, NULL, 1, NULL, 1);
}

void mainLoop(void* pvParameters) {
  while (true) {
    if (interruptTriggerd) {
      interruptTriggerd = false;
      unsigned long now = millis();

      if (now - lastDebounceTime > debounceDelay) {
        lastDebounceTime = now;
        portENTER_CRITICAL(&stateMux);
        stateRobot = !stateRobot;
        portEXIT_CRITICAL(&stateMux);

        updateLEDs();
      }
    }

    if (stateRobot) {
      startRobot();
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void loop() {

}
