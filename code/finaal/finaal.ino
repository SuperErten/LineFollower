#include <WiFi.h>
#include <WebServer.h>
#include <QTRSensors.h>
#include <Preferences.h>
#include "freertos/task.h"
#include "driver/gptimer.h" 

constexpr uint8_t PIN_DI_START_STOP = 13;
constexpr uint8_t PIN_DO_LED_RED    = 14;
constexpr uint8_t PIN_DO_LED_GREEN  = 27;

QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

volatile bool interruptTriggerd = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 100; //ms

const char* ssid     = "Tibo123";
const char* password = "876543210";

volatile bool stateRobot = 0;
volatile float Kp = 0;
volatile float Ki = 0;
volatile float Kd = 0;

float P = 0, I = 0, D = 0;

Preferences prefs;

WebServer server(80);

TaskHandle_t pid_compute_handle = NULL;

//Maar één core kan globaal data aanpassen.
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR toggleState() {

    interruptTriggerd = 1;
}

static bool IRAM_ATTR timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {

    BaseType_t higher_priority_woken = pdFALSE;
    vTaskNotifyGiveFromISR(pid_compute_handle, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
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

void pidCompute(void *pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // PID berkening
    }
}

void setup() {

  Serial.begin(115200);

  prefs.begin("pid", false);
  P = prefs.getFloat("Kp", 0.0);
  I = prefs.getFloat("Ki", 0.0);
  D = prefs.getFloat("Kd", 0.0);
  prefs.end(); 
  
  pinMode(PIN_DI_START_STOP, INPUT_PULLUP);
  pinMode(PIN_DO_LED_GREEN, OUTPUT);
  pinMode(PIN_DO_LED_RED , OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){26, 36, 39, 34, 35, 32, 33, 25}, SensorCount); // 

  attachInterrupt(digitalPinToInterrupt(PIN_DI_START_STOP), toggleState, FALLING);
  updateLEDs();

  // draaien op 2 aparte core's wifi o en main op 1
  xTaskCreatePinnedToCore(wifiLoop,  "wifiLoop",  8192, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mainLoop, "mainLoop", 4096, NULL, 1, NULL, 1);
  // Ineterupt task voor PID om tijd zo stabiel mogelijk te maken
  xTaskCreatePinnedToCore(pidCompute, "pidCompute", 4096, NULL, 5, &pid_compute_handle, 1);

  // timer voor intterupt
  gptimer_handle_t timer = NULL;

  gptimer_config_t timer_config = {};
  timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  timer_config.direction = GPTIMER_COUNT_UP;
  timer_config.resolution_hz = 1000000;
  gptimer_new_timer(&timer_config, &timer);

  gptimer_alarm_config_t alarm_config = {};
  alarm_config.alarm_count = 15000; // 10 ms Sample Time
  alarm_config.reload_count = 0;
  alarm_config.flags.auto_reload_on_alarm = true;
  gptimer_set_alarm_action(timer, &alarm_config);

  gptimer_event_callbacks_t cbs = {};
  cbs.on_alarm = timer_callback;
  gptimer_register_event_callbacks(timer, &cbs, NULL);

  gptimer_enable(timer);
  gptimer_start(timer);
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
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void loop() {
}
