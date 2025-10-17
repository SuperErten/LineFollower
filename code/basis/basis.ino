#include <WiFi.h>
#include <WebServer.h>
#include <QTRSensors.h>
#include <Preferences.h>

#include <L298N.h>


const char* ssid     = "Tibo123";
const char* password = "876543210";

WebServer server(80);
Preferences prefs; // voor opslag in flash


// Application state
bool running = false;
float P = 0.0;
float I = 0.0;
float D = 0.0;

#define AIN1 21  //Assign the motor pins
#define BIN1 25
#define AIN2 22
#define BIN2 33
#define PWMA 23
#define PWMB 32
#define STBY 19

L298N motor1(PWMA, AIN1, AIN2);
L298N motor2(PWMB, BIN1, BIN2);

const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];
int threshold[SensorCount];

uint8_t multiP = 1;
uint8_t multiI  = 1;
uint8_t multiD = 1;
uint8_t Kpfinal;
uint8_t Kifinal;
uint8_t Kdfinal;
float Pvalue;
float Ivalue;
float Dvalue;

int val, cnt = 0, v[3];

uint16_t position;
int P, D, I, previousError, PIDvalue, error;
int lsp, rsp;
int lfspeed = 230;

// NOTE: The first pin must always be PWM capable, the second only, if the last parameter is set to "true"
// SYNTAX: IN1, IN2, min. input value, max. input value, neutral position width
// invert rotation direction, true = both pins are PWM capable
DRV8833 Motor1(motor1_in1, motor1_in2, 0, 1023, 60, false, true); // Motor 1
DRV8833 Motor2(motor2_in1, motor2_in2, 0, 1023, 60, false, true); // Motor 2

// HTML page template
String pageTemplate = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 PID Control</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 50px; }
      input[type="number"] { width: 80px; }
      .button { padding: 10px 20px; font-size: 20px; }
    </style>
  </head>
  <body>
    <h1>Linefollower Control</h1>
    <p>Status: <strong>%STATUS%</strong></p>
    <form action="/set" method="POST">
      <label for="P">P: </label>
      <input type="number" step="0.01" id="P" name="P" value="%P%">
      <br><br>
      <label for="I">I: </label>
      <input type="number" step="0.01" id="I" name="I" value="%I%">
      <br><br>
      <label for="D">D: </label>
      <input type="number" step="0.01" id="D" name="D" value="%D%">
      <br><br>
      <input class="button" type="submit" value="Apply PID">
    </form>
    <br>
    <form action="/toggle" method="POST">
      <input class="button" type="submit" value="%BUTTON%">
    </form>
  </body>
</html>
)rawliteral";

// Helper to send the page with dynamic values
void sendPage() {
  String page = pageTemplate;
  page.replace("%STATUS%", running ? "Running" : "Stopped");
  page.replace("%P%", String(P, 2));
  page.replace("%I%", String(I, 2));
  page.replace("%D%", String(D, 2));
  page.replace("%BUTTON%", running ? "Stop" : "Start");
  server.send(200, "text/html", page);
}

void handleRoot() {
  sendPage();
}

void handleToggle() {
  running = !running;
  Serial.printf("Toggled running state: %d\n", running);
  sendPage();
}

void handleSet() {
  bool updated = false;

  if (server.hasArg("P")) {
    String pArg = server.arg("P");
    P = pArg.toFloat();
    Serial.printf("New P = %f\n", P);
    updated = true;
  }
  if (server.hasArg("I")) {
    String iArg = server.arg("I");
    I = iArg.toFloat();
    Serial.printf("New I = %f\n", I);
    updated = true;
  }
  if (server.hasArg("D")) {
    String dArg = server.arg("D");
    D = dArg.toFloat();
    Serial.printf("New D = %f\n", D);
    updated = true;
  }

  // Opslaan in flash als er iets gewijzigd is
  if (updated) {
    prefs.begin("pid", false);
    prefs.putFloat("P", P);
    prefs.putFloat("I", I);
    prefs.putFloat("D", D);
    prefs.end();
    Serial.println("PID parameters saved to flash.");
  }

  sendPage();
}

void setup() {
  Serial.begin(115200);

  // WiFi verbinding
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());

  // PID-waarden laden uit flash
  prefs.begin("pid", true);
  P = prefs.getFloat("P", 1.00);  // standaardwaarden
  I = prefs.getFloat("I", 0.00);
  D = prefs.getFloat("D", 0.00);
  prefs.end();
  Serial.printf("Loaded PID: P=%.2f, I=%.2f, D=%.2f\n", P, I, D);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/set", HTTP_POST, handleSet);
  server.begin();

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  server.handleClient();

  if (running) {
    digitalWrite(LED_BUILTIN, HIGH);
    // robotControl()
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void robotControl(){
  // read calibrated sensor values and obtain a measure of the line position
  // from 0 to 4000
  position = qtr.readLineBlack(sensorValues);
  error = 2000 - position;
  while(sensorValues[0]>=980 && sensorValues[1]>=980 && sensorValues[2]>=980 && sensorValues[3]>=980 && sensorValues[4]>=980){ // A case when the line follower leaves the line
    if(previousError>0){       //Turn left if the line was to the left before
      motorDrive(-230,230);
    } else {
      motorDrive(230,-230); // Else turn right
    }
    position = qtr.readLineBlack(sensorValues);
  }
  calculationPID(error);
}

void calculationPID(int error){
    P = error;
    I = I + error;
    D = error - previousError;
    
    Pvalue = (Kp/pow(10,multiP))*P;
    Ivalue = (Ki/pow(10,multiI))*I;
    Dvalue = (Kd/pow(10,multiD))*D; 

    float PIDvalue = Pvalue + Ivalue + Dvalue;
    previousError = error;

    lsp = lfspeed - PIDvalue;
    rsp = lfspeed + PIDvalue;

    if (lsp > 255) {
      lsp = 255;
    }
    if (lsp < -255) {
      lsp = -255;
    }
    if (rsp > 255) {
      rsp = 255;
    }
    if (rsp < -255) {
      rsp = -255;
    }
    motorDrive(lsp,rsp);
}

void motorDrive(int left, int right){
  if(right>0) {
    motor2.setSpeed(right);
    motor2.forward();
  } else {
    motor2.setSpeed(right);
    motor2.backward();
  }
  
  if(left>0) {
    motor1.setSpeed(left);
    motor1.forward();
  } else {
    motor1.setSpeed(left);
    motor1.backward();
  }
}
