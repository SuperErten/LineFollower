constexpr uint8_t PIN_DI_START_STOP = 13;
constexpr uint8_t PIN_DO_LED_RED    = 14;
constexpr uint8_t PIN_DO_LED_GREEN  = 27;

volatile bool stateRobot = 0;
volatile bool interruptTriggerd = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void setup() {

  Serial.begin(115200);

  pinMode(PIN_DI_START_STOP, INPUT_PULLUP);
  pinMode(PIN_DO_LED_GREEN, OUTPUT);
  pinMode(PIN_DO_LED_RED , OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_DI_START_STOP), toggleState, FALLING);
}

void loop() {

  if (interruptTriggerd) {
    interruptTriggerd = 0;
    unsigned long now = millis();
    
    if (now - lastDebounceTime > debounceDelay) {
      stateRobot = !stateRobot;
      lastDebounceTime = now;
    }
  }

  if (stateRobot == 0) {
    digitalWrite(PIN_DO_LED_RED , 1);
    digitalWrite(PIN_DO_LED_GREEN, 0);
  }

  if (stateRobot) {
    digitalWrite(PIN_DO_LED_RED, 0);
    digitalWrite(PIN_DO_LED_GREEN, 1);
    startRobot();
  }
}

void toggleState() {

    interruptTriggerd = 1;
}

void startRobot() {

  Serial.print("Robot Started");
}