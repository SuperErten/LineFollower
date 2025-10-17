#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

const char* ssid     = "Tibo123";
const char* password = "876543210";

WebServer server(80);
Preferences prefs; // voor opslag in flash

// Application state
bool running = false;
float P = 0.0;
float I = 0.0;
float D = 0.0;

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
    // PID-regeling zou hier komen
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
