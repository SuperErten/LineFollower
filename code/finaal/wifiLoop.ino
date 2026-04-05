static const char pageTemplate[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Linefollower Control</title>
    <style>
      * { margin: 0; padding: 0; box-sizing: border-box; }

      body {
        font-family: 'Segoe UI', Arial, sans-serif;
        background-color: #f5f5f5;
        color: #1a1a1a;
        min-height: 100vh;
        display: flex;
        flex-direction: column;
        align-items: center;
      }

      header {
        width: 100%;
        background-color: #fff;
        padding: 18px 40px;
        display: flex;
        align-items: center;
        justify-content: space-between;
        border-bottom: 2px solid #2563eb;
        box-shadow: 0 2px 8px rgba(0,0,0,0.06);
      }

      header h1 {
        font-size: 1.4rem;
        color: #2563eb;
        letter-spacing: 2px;
        text-transform: uppercase;
      }

      .status-badge {
        padding: 6px 16px;
        border-radius: 20px;
        font-size: 0.85rem;
        font-weight: bold;
        letter-spacing: 1px;
        text-transform: uppercase;
      }
      .status-running { background-color: #2563eb; color: #fff; }
      .status-stopped { background-color: #e0e0e0; color: #888; }

      main {
        width: 100%;
        max-width: 480px;
        padding: 40px 20px;
        display: flex;
        flex-direction: column;
        gap: 24px;
      }

      .card {
        background-color: #fff;
        border: 1px solid #e0e0e0;
        border-radius: 12px;
        padding: 28px;
        box-shadow: 0 2px 8px rgba(0,0,0,0.05);
      }

      .card h2 {
        font-size: 0.75rem;
        text-transform: uppercase;
        letter-spacing: 2px;
        color: #2563eb;
        margin-bottom: 20px;
      }

      .pid-row {
        display: flex;
        align-items: center;
        justify-content: space-between;
        margin-bottom: 16px;
      }

      .pid-row label {
        font-size: 1rem;
        color: #888;
        width: 24px;
      }

      .pid-row input[type="number"] {
        flex: 1;
        margin-left: 16px;
        padding: 10px 14px;
        background-color: #f5f5f5;
        border: 1px solid #ddd;
        border-radius: 8px;
        color: #1a1a1a;
        font-size: 1rem;
        outline: none;
        transition: border-color 0.2s;
      }

      .pid-row input[type="number"]:focus {
        border-color: #2563eb;
      }

      .btn {
        width: 100%;
        padding: 14px;
        border: none;
        border-radius: 8px;
        font-size: 1rem;
        font-weight: bold;
        letter-spacing: 1px;
        text-transform: uppercase;
        cursor: pointer;
        transition: opacity 0.2s;
        margin-top: 8px;
      }
      .btn:hover      { opacity: 0.85; }
      .btn-primary    { background-color: #2563eb; color: #fff; }
      .btn-start      { background-color: #2563eb; color: #fff; }
      .btn-stop       { background-color: #e63c3c; color: #fff; }
      .btn-calibrate  { background-color: #fff; color: #2563eb; border: 1px solid #2563eb; }
      .btn-disabled   {
        background-color: #f0f0f0;
        color: #bbb;
        border: 1px solid #ddd;
        cursor: not-allowed;
        opacity: 1;
      }

      .hint {
        font-size: 0.78rem;
        color: #bbb;
        margin-top: 10px;
        text-align: center;
      }

      footer {
        margin-top: auto;
        padding: 20px;
        font-size: 0.75rem;
        color: #aaa;
        letter-spacing: 1px;
      }
    </style>
  </head>
  <body>

    <header>
      <h1>Linefollower</h1>
      <span class="status-badge %STATUS_CLASS%">%STATUS%</span>
    </header>

    <main>

      <div class="card">
        <h2>PID Parameters</h2>
        <form action="/set" method="POST">
          <div class="pid-row">
            <label>P</label>
            <input type="number" step="0.01" name="Kp" value="%P%"> 
          </div>
          <div class="pid-row">
            <label>I</label>
            <input type="number" step="0.01" name="Ki" value="%I%">
          </div>
          <div class="pid-row">
            <label>D</label>
            <input type="number" step="0.01" name="Kd" value="%D%">
          </div>
          <input class="btn btn-primary" type="submit" value="Apply PID">
        </form>
      </div>

      <div class="card">
        <h2>Robot</h2>
        <form action="/toggle" method="POST">
          <input class="btn %BUTTON_CLASS%" type="submit" value="%BUTTON%">
        </form>
      </div>

      <div class="card">
        <h2>Sensor Kalibratie</h2>
        <form action="/calibrate" method="POST">
          <input class="btn %CALIBRATE_CLASS%" type="submit" value="Kalibreren" %CALIBRATE_DISABLED%>
        </form>
        %CALIBRATE_HINT%
      </div>

    </main>

    <footer>Synthese Project V1.3 &mdash; tibo.gent</footer>

  </body>
</html>
)rawliteral";

void sendPage() {
  String page = FPSTR(pageTemplate);
  page.replace("%STATUS%",       stateRobot ? "Running" : "Stopped");
  page.replace("%STATUS_CLASS%", stateRobot ? "status-running" : "status-stopped");
  page.replace("%Kp%",            String(Kp, 2));
  page.replace("%Ki%",            String(Ki, 2));
  page.replace("%Kd%",            String(Kd, 2));
  page.replace("%BUTTON%",       stateRobot ? "Stop" : "Start");
  page.replace("%BUTTON_CLASS%", stateRobot ? "btn-stop" : "btn-start");
  page.replace("%CALIBRATE_CLASS%",    stateRobot ? "btn-disabled"  : "btn-calibrate");
  page.replace("%CALIBRATE_DISABLED%", stateRobot ? "disabled"      : "");
  page.replace("%CALIBRATE_HINT%",     stateRobot ? "<p class='hint'>Stop de robot eerst om te kalibreren.</p>" : "");
  server.send(200, "text/html", page);
}

void handleRoot() {
  sendPage();
}

void handleToggle() {
  portENTER_CRITICAL(&stateMux);
  stateRobot = !stateRobot;
  portEXIT_CRITICAL(&stateMux);
  updateLEDs();
  sendPage();
}

void handleSet() {
  portENTER_CRITICAL(&stateMux);
  if (server.hasArg("Kp")) Kp = server.arg("Kp").toFloat();
  if (server.hasArg("Ki")) Ki = server.arg("Ki").toFloat();
  if (server.hasArg("Kd")) Kd = server.arg("Kd").toFloat();
  portEXIT_CRITICAL(&stateMux);

  prefs.begin("pid", false);
  prefs.putFloat("Kp", Kp);
  prefs.putFloat("Ki", Ki);
  prefs.putFloat("Kd", Kd);
  prefs.end(); 
  
  sendPage();
}

void handleCalibrate() {
  calibrateQTR();
  sendPage();
}

void wifiLoop(void* pvParameters) {
  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Verbinden");
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] Verbonden, IP: %s\n", WiFi.localIP().toString().c_str());

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/set",    HTTP_POST, handleSet);
  server.on("/calibrate", HTTP_POST, handleCalibrate);
  server.begin();
  Serial.println("[WiFi] Webserver gestart.");

  while (true) {
    server.handleClient();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}