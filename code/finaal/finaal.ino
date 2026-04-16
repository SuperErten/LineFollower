#include <WiFi.h>
#include "Cdrv8833.h"
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

volatile bool interruptTriggerd = false;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 100; //ms

const char* ssid     = "Tibo123";
const char* password = "876543210";

volatile bool stateRobot = 0;
volatile float Kp = 0;
volatile float Ki = 0;
volatile float Kd = 0;

float P = 0, I = 0, D = 0;

unsigned long lastTime;
float inputPID, outputPID;
float ITerm, lastInputPID;
float outMinPID, outMaxPID;
float setpointPID = 4000;
int sampleTimePID = 15000; // 15 ms Sample Time

constexpr uint8_t PIN_AIN1          = 19;
constexpr uint8_t PIN_AIN2          = 18;
constexpr uint8_t CHANNEL_MOTOR_A   = 0;
constexpr bool SWAP_MOTOR_A         = 0;

constexpr uint8_t PIN_BIN1          = 16;
constexpr uint8_t PIN_BIN2          = 17;
constexpr uint8_t CHANNEL_MOTOR_B   = 1;
constexpr bool SWAP_MOTOR_B         = 0;

int baseSpeed = 35;          // basis vooruit (0..100)
int maxSpeed  = 80;          // clamp (0..100)

Preferences prefs;

WebServer server(80);

TaskHandle_t pid_compute_handle = NULL;

Cdrv8833 motorA(PIN_AIN1, PIN_AIN2, CHANNEL_MOTOR_A, SWAP_MOTOR_A);
Cdrv8833 motorB(PIN_BIN1, PIN_BIN2, CHANNEL_MOTOR_B, SWAP_MOTOR_B);

//Maar één core kan globaal data aanpassen.
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR toggleState() {

  interruptTriggerd = true;
}

static bool IRAM_ATTR timerCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data) {

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
	
  portENTER_CRITICAL(&stateMux);
  inputPID = (float)position;
  portEXIT_CRITICAL(&stateMux)


  for (uint8_t i = 0; i < SensorCount; i++) //Later Verwijderen
  {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }
  Serial.println(position);
}

void calibrateQTR() {

  digitalWrite(LED_BUILTIN, true);

  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
  }
  digitalWrite(LED_BUILTIN, false); 
}

void SetOutputLimits(double Min, double Max)
{
   if(Min > Max) return;
   outMinPID = Min;
   outMaxPID = Max;
    
   if(outputPID > outMaxPID) outputPID = outMaxPID;
   else if(outputPID < outMinPID) outputPID = outMinPID;
 
   if(ITerm> outMaxPID) ITerm= outMaxPID;
   else if(ITerm< outMinPID) ITerm= outMinPID;
}

void pidCompute(void *pvParameters) {
	
  const float dt = sampleTimePID / 1000000.0f;
  
   while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	  
    float input, setpoint, kp, ki, kd, outMin, outMax, iTerm, lastIn;
    portENTER_CRITICAL(&stateMux);
    input   = inputPID;
    setpoint= setpointPID;
    kp      = Kp;
    ki      = Ki;
    kd      = Kd;
    outMin  = outMinPID;
    outMax  = outMaxPID;
    iTerm   = ITerm;
    lastIn  = lastInputPID;
    portEXIT_CRITICAL(&stateMux);

    float error = setpoint - input;

    iTerm += (ki * error * dt);
    if (iTerm > outMax) iTerm = outMax;
    else if (iTerm < outMin) iTerm = outMin;

    
    float dInput = (input - lastIn) / dt;

    float out = kp * error + iTerm - kd * dInput;
    if (out > outMax) out = outMax;
    else if (out < outMin) out = outMin;
    
    
    portENTER_CRITICAL(&stateMux);
    ITerm        = iTerm;
    outputPID    = out;
    lastInputPID = input;
    portEXIT_CRITICAL(&stateMux);
   }
}

void setup() {

  Serial.begin(115200);

  prefs.begin("pid", false);
  Kp = prefs.getFloat("Kp", 0.0);
  Ki = prefs.getFloat("Ki", 0.0);
  Kd = prefs.getFloat("Kd", 0.0);
  prefs.end(); 
  
  pinMode(PIN_DI_START_STOP, INPUT_PULLUP);
  pinMode(PIN_DO_LED_GREEN, OUTPUT);
  pinMode(PIN_DO_LED_RED , OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  motorA.setDecayMode(drv8833DecayFast);
  motorB.setDecayMode(drv8833DecayFast);
  
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
  alarm_config.alarm_count = sampleTimePID;
  alarm_config.reload_count = 0;
  alarm_config.flags.auto_reload_on_alarm = true;
  gptimer_set_alarm_action(timer, &alarm_config);

  gptimer_event_callbacks_t cbs = {};
  cbs.on_alarm = timerCallback;
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
		
		if (!stateRobot) {
		  ITerm = 0;
		  outputPID = 0;
		  lastInputPID = inputPID;
		}
        portEXIT_CRITICAL(&stateMux);

        updateLEDs();
      }
    }

    if (stateRobot) {
      readValuesQTR();
	  
	  float out;
      portENTER_CRITICAL(&stateMux);
      out = outputPID;
      portEXIT_CRITICAL(&stateMux);
	  
	  float steer = (out / 255.0f) * 100.0f;
	  
	  int left  = (int)roundf(baseSpeed + steer);
	  int right = (int)roundf(baseSpeed - steer);
	  
	  left  = constrain(left,  -maxSpeed, maxSpeed);
	  right = constrain(right, -maxSpeed, maxSpeed);
	  
      motorA.move((int8_t)left); 
      motorB.move((int8_t)right);  
    }
	else {
	  motorA.brake();
	  motorB.brake();
	}
	vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void loop() {
}
