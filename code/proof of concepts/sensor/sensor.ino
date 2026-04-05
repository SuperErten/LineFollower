#include <QTRSensors.h>

QTRSensors qtr;

constexpr uint8_t PIN_DI_START_STOP = 13;
constexpr uint8_t PIN_DO_LED_RED    = 14;
constexpr uint8_t PIN_DO_LED_GREEN  = 27;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

volatile bool stateRobot = 0;
volatile bool interruptTriggerd = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

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

  delay(250);
}

void calibrateQTR() {

  digitalWrite(LED_BUILTIN, HIGH);

  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
  }
  digitalWrite(LED_BUILTIN, LOW); 
}

void startRobot() {

  readValuesQTR();
}

void setup() {

  Serial.begin(115200);

  pinMode(PIN_DI_START_STOP, INPUT_PULLUP);
  pinMode(PIN_DO_LED_GREEN, OUTPUT);
  pinMode(PIN_DO_LED_RED , OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){26, 36, 39, 34, 35, 32, 33, 25}, SensorCount); // 

  attachInterrupt(digitalPinToInterrupt(PIN_DI_START_STOP), toggleState, FALLING);
  updateLEDs();
  calibrateQTR();
}

void loop() {

  if (interruptTriggerd) {
    interruptTriggerd = 0;
    unsigned long now = millis();
    
    if (now - lastDebounceTime > debounceDelay) {
      stateRobot = !stateRobot;
      lastDebounceTime = now;
      updateLEDs();
    }
  }

  if (stateRobot) {
    startRobot();
  }
}