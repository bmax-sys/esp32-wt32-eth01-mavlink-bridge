#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <Preferences.h>

// =====================================================
//                bmax_sys BRIDGE
//                   by bmax_sys
//             WT32-ETH01 / ESP32
// =====================================================

// WT32-ETH01 Ethernet (LAN8720)
#define ETH_PHY_TYPE   ETH_PHY_LAN8720
#define ETH_PHY_ADDR   1
#define ETH_PHY_MDC    23
#define ETH_PHY_MDIO   18
#define ETH_PHY_POWER  16
#define ETH_CLK_MODE   ETH_CLOCK_GPIO0_IN

// UART
#define UART_RX_PIN 35
#define UART_TX_PIN 14
HardwareSerial TelemetrySerial(2);

// Web auth
const char* WEB_USER = "spx";
const char* WEB_PASS = "1488";

// Objects
WebServer server(80);
WiFiUDP udp;
Preferences prefs;

// Settings
struct Settings {
  String deviceIP;
  String subnetMask;
  String targetIP;
  uint16_t udpPort;
  uint32_t baudrate;
};

Settings cfg;

const char* DEFAULT_DEVICE_IP   = "192.168.88.50";
const char* DEFAULT_SUBNET_MASK = "255.255.255.0";
const char* DEFAULT_TARGET_IP   = "192.168.88.255";
const uint16_t DEFAULT_UDP_PORT = 14550;
const uint32_t DEFAULT_BAUDRATE = 115200;

// UART buffer
uint8_t uartBuffer[512];
size_t uartBufferLength = 0;
unsigned long lastUARTByteTime = 0;

// -----------------------------------------------------
// Helpers
// -----------------------------------------------------

bool parseIP(const String& text, IPAddress& ip) {
  int a, b, c, d;

  if (sscanf(text.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
    return false;
  }

  if (a < 0 || a > 255 ||
      b < 0 || b > 255 ||
      c < 0 || c > 255 ||
      d < 0 || d > 255) {
    return false;
  }

  ip = IPAddress(a, b, c, d);
  return true;
}

void loadSettings() {
  prefs.begin("speXtr", true);

  cfg.deviceIP   = prefs.getString("deviceIP", DEFAULT_DEVICE_IP);
  cfg.subnetMask = prefs.getString("mask", DEFAULT_SUBNET_MASK);
  cfg.targetIP   = prefs.getString("targetIP", DEFAULT_TARGET_IP);
  cfg.udpPort    = prefs.getUShort("port", DEFAULT_UDP_PORT);
  cfg.baudrate   = prefs.getULong("baud", DEFAULT_BAUDRATE);

  prefs.end();
}

void saveSettings() {
  prefs.begin("speXtr", false);

  prefs.putString("deviceIP", cfg.deviceIP);
  prefs.putString("mask", cfg.subnetMask);
  prefs.putString("targetIP", cfg.targetIP);
  prefs.putUShort("port", cfg.udpPort);
  prefs.putULong("baud", cfg.baudrate);

  prefs.end();
}

bool requireAuth() {
  if (server.authenticate(WEB_USER, WEB_PASS)) {
    return true;
  }

  server.requestAuthentication();
  return false;
}

// -----------------------------------------------------
// CSS
// -----------------------------------------------------

String getCSS()
{
  return R"rawliteral(
<style>
:root{
  --yellow:#ffcc00;
  --yellow2:#ffe45a;
  --black:#000000;
  --panel:#050505;
  --grid:rgba(255,204,0,.10);
  --field:#d0d0d0;
}

*{
  box-sizing:border-box;
}

html,body{
  margin:0;
  padding:0;
  width:100%;
  min-height:100%;
  background:#000;
  color:var(--yellow);
  font-family:"Courier New",Consolas,monospace;
}

body{
  min-height:100vh;
  background-image:
    linear-gradient(var(--grid) 1px, transparent 1px),
    linear-gradient(90deg,var(--grid) 1px, transparent 1px);
  background-size:32px 32px;
}

.frame{
  min-height:calc(100vh - 24px);
  margin:12px;
  border:2px solid var(--yellow);
  position:relative;
  overflow:hidden;
  background:
    linear-gradient(180deg,rgba(255,204,0,.015),rgba(0,0,0,.01));
  box-shadow:
    0 0 0 2px #000 inset,
    0 0 0 3px rgba(255,204,0,.08) inset;
}

.frame::before,
.frame::after{
  content:"";
  position:absolute;
  width:110px;
  height:6px;
  background:var(--yellow);
  top:-2px;
}

.frame::before{left:24px;}
.frame::after{right:24px;}

.brand{
  position:absolute;
  top:22px;
  left:32px;
  z-index:10;
  text-shadow:2px 2px 0 #000;
}

.brand-main{
  font-size:28px;
  font-weight:900;
  letter-spacing:2px;
}

.brand-main span{
  font-weight:700;
}

.brand-sub{
  margin-top:4px;
  font-size:12px;
  letter-spacing:1px;
}

/* PIXEL LOGO */
.pixel-logo{
  display:flex;
  align-items:center;
  justify-content:center;
  gap:14px;
}

.pixel-mark{
  position:relative;
  width:58px;
  height:58px;
  image-rendering:pixelated;
  filter:drop-shadow(0 0 8px rgba(255,204,0,.16));
}

.pixel-mark::before{
  content:"";
  position:absolute;
  left:6px;
  top:6px;
  width:46px;
  height:46px;
  background:var(--yellow);
  clip-path:polygon(
    0 0, 42% 0, 42% 18%, 58% 18%, 58% 0, 100% 0,
    100% 38%, 82% 38%, 82% 62%, 100% 62%, 100% 100%,
    58% 100%, 58% 82%, 42% 82%, 42% 100%, 0 100%,
    0 62%, 18% 62%, 18% 38%, 0 38%
  );
}

.pixel-mark::after{
  content:"";
  position:absolute;
  left:18px;
  top:18px;
  width:22px;
  height:22px;
  background:#000;
}

.pixel-word{
  font-size:54px;
  font-weight:900;
  letter-spacing:6px;
  line-height:1;
  text-shadow:3px 3px 0 #000;
}

.splash{
  min-height:calc(100vh - 28px);
  display:flex;
  flex-direction:column;
  justify-content:center;
  align-items:center;
  padding:100px 30px 50px;
  text-align:center;
}

.splash-title{
  margin-top:28px;
  font-size:32px;
  font-weight:900;
  letter-spacing:4px;
  text-shadow:2px 2px 0 #000;
}

.loading-wrap{
  width:min(900px,72vw);
  margin-top:42px;
}

.loading{
  height:8px;
  border:1px solid var(--yellow);
  background:#181400;
  overflow:hidden;
  position:relative;
}

.loading::after{
  content:"";
  display:block;
  width:38%;
  height:100%;
  background:var(--yellow);
  animation:load 1s steps(8,end) infinite;
}

.loading-text{
  margin-top:10px;
  font-size:15px;
  font-weight:900;
  letter-spacing:3px;
}

@keyframes load{
  0%{transform:translateX(-120%);}
  100%{transform:translateX(360%);}
}

/* SETTINGS */
.settings-wrapper{
  width:100%;
  padding:125px 50px 45px;
}

.settings-panel{
  position:relative;
  width:100%;
  border:2px solid var(--yellow);
  padding:42px 42px 38px;
  background:rgba(0,0,0,.86);
  box-shadow:6px 6px 0 rgba(255,204,0,.08);
}

.panel-title{
  position:absolute;
  top:-20px;
  left:50%;
  transform:translateX(-50%);
  padding:5px 24px;
  background:#000;
  border:2px solid var(--yellow);
  font-weight:900;
  font-size:22px;
  letter-spacing:3px;
  white-space:nowrap;
}

.settings-grid{
  display:grid;
  grid-template-columns:1fr 1fr 1fr;
  gap:26px 42px;
}

.field label{
  display:block;
  margin-bottom:9px;
  font-size:15px;
  font-weight:900;
  letter-spacing:1px;
}

input,select{
  width:100%;
  height:48px;
  padding:0 14px;
  color:var(--field);
  background:#050505;
  border:2px solid var(--yellow);
  outline:none;
  border-radius:0;
  font-family:"Courier New",Consolas,monospace;
  font-size:18px;
  font-weight:700;
  box-shadow:3px 3px 0 rgba(255,204,0,.08);
}

input:focus,select:focus{
  background:#0a0a0a;
  box-shadow:
    0 0 0 2px #000,
    0 0 0 4px var(--yellow);
}

.button-wrapper{
  display:flex;
  justify-content:center;
  margin-top:34px;
}

.save-button{
  min-width:320px;
  height:52px;
  border:2px solid var(--yellow);
  background:var(--yellow);
  color:#000;
  font-family:"Courier New",Consolas,monospace;
  font-size:17px;
  font-weight:900;
  letter-spacing:2px;
  cursor:pointer;
  box-shadow:5px 5px 0 #8f7300;
}

.save-button:hover{
  background:#000;
  color:var(--yellow);
  box-shadow:5px 5px 0 #463900;
}

/* SAVED */
.saved{
  min-height:calc(100vh - 30px);
  display:flex;
  flex-direction:column;
  justify-content:center;
  align-items:center;
  text-align:center;
  padding:100px 24px 40px;
}

.check{
  width:78px;
  height:78px;
  border:3px solid var(--yellow);
  display:flex;
  align-items:center;
  justify-content:center;
  font-size:42px;
  font-weight:900;
  margin-bottom:26px;
  box-shadow:6px 6px 0 rgba(255,204,0,.08);
}

.saved-title{
  font-size:34px;
  font-weight:900;
  letter-spacing:3px;
}

.saved-sub{
  margin-top:12px;
  font-size:18px;
}

.saved-line{
  width:360px;
  height:2px;
  background:var(--yellow);
  margin:28px 0;
}

.countdown{
  width:76px;
  height:76px;
  margin-top:24px;
  border:3px solid var(--yellow);
  display:flex;
  align-items:center;
  justify-content:center;
  font-size:32px;
  font-weight:900;
}

@media(max-width:900px){
  .settings-grid{
    grid-template-columns:1fr;
  }

  .settings-wrapper{
    padding:135px 20px 30px;
  }

  .pixel-word{
    font-size:32px;
    letter-spacing:3px;
  }

  .pixel-mark{
    width:46px;
    height:46px;
  }

  .splash-title{
    font-size:20px;
    letter-spacing:2px;
  }

  .loading-wrap{
    width:84vw;
  }
}
</style>
)rawliteral";
}

String pixelLogoHTML()
{
  return R"rawliteral(
<div class='pixel-logo'>
  <div class='pixel-mark'></div>
  <div class='pixel-word'>bmax_sys</div>
</div>
)rawliteral";
}

String splashPage() {
  String html;
  html.reserve(12000);

  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>bmax_sys</title>";
  html += getCSS();
  html += "</head><body><div class='frame'>";

  html += "<div class='brand'>";
  html += "<div class='brand-main'>bmax_<span>sys</span></div>";
  html += "<div class='brand-sub'>by bmax_sys</div>";
  html += "</div>";

  html += "<div class='splash'>";
  html += pixelLogoHTML();
  html += "<div class='splash-title'>bmax_sys BRIDGE INTERFACE</div>";
  html += "<div class='loading-wrap'><div class='loading'></div><div class='loading-text'>LOADING...</div></div>";
  html += "</div></div>";

  html += R"rawliteral(
<script>
setTimeout(function(){
  window.location.href="/settings";
},2000);
</script>
)rawliteral";

  html += "</body></html>";
  return html;
}

String settingsPage() {
  String html;
  html.reserve(16000);

  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>bmax_sys Settings</title>";
  html += getCSS();
  html += "</head><body><div class='frame'>";

  html += "<div class='brand'>";
  html += "<div class='brand-main'>bmax_<span>sys</span></div>";
  html += "<div class='brand-sub'>by bmax_sys</div>";
  html += "</div>";


  html += "<div style='position:absolute;top:26px;right:34px;z-index:9;font-weight:900;letter-spacing:3px;font-size:18px;'>bmax_sys</div>";

  html += "<div class='settings-wrapper'>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='settings-panel'>";
  html += "<div class='panel-title'>NETWORK SETTINGS</div>";
  html += "<div class='settings-grid'>";

  html += "<div class='field'><label>DEVICE IP</label>";
  html += "<input name='deviceIP' value='" + cfg.deviceIP + "' required></div>";

  html += "<div class='field'><label>TARGET IP</label>";
  html += "<input name='targetIP' value='" + cfg.targetIP + "' required></div>";

  html += "<div class='field'><label>UART BAUDRATE</label>";
  html += "<select name='baud'>";

  const uint32_t baudRates[] = {
    9600,19200,38400,57600,115200,230400,460800,921600
  };

  for (uint32_t baud : baudRates) {
    html += "<option value='" + String(baud) + "'";
    if (cfg.baudrate == baud) html += " selected";
    html += ">" + String(baud) + "</option>";
  }

  html += "</select></div>";

  html += "<div class='field'><label>SUBNET MASK</label>";
  html += "<input name='mask' value='" + cfg.subnetMask + "' required></div>";

  html += "<div class='field'><label>UDP PORT</label>";
  html += "<input type='number' min='1' max='65535' name='port' value='" +
          String(cfg.udpPort) + "' required></div>";

  html += "</div>";

  html += "<div class='button-wrapper'>";
  html += "<button type='submit' class='save-button'>SAVE & REBOOT</button>";
  html += "</div>";

  html += "</div></form></div></div></body></html>";
  return html;
}

String savedPage(const String& newIP) {
  String html;
  html.reserve(12000);

  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Settings Saved</title>";
  html += getCSS();
  html += "</head><body><div class='frame'>";

  html += "<div class='brand'>";
  html += "<div class='brand-main'>bmax_<span>sys</span></div>";
  html += "<div class='brand-sub'>by bmax_sys</div>";
  html += "</div>";


  html += "<div style='position:absolute;top:26px;right:34px;z-index:9;font-weight:900;letter-spacing:3px;font-size:18px;'>bmax_sys</div>";

  html += "<div class='saved'>";
  html += "<div class='check'>✓</div>";
  html += "<div class='saved-title'>SETTINGS SAVED</div>";
  html += "<div class='saved-sub'>Device is rebooting...</div>";
  html += "<div class='saved-line'></div>";
  html += "<div>Please wait. The interface will reconnect automatically.</div>";
  html += "<div class='countdown' id='count'>5</div>";
  html += "</div></div>";

  html += "<script>";
  html += "let counter=5;";
  html += "const target='http://" + newIP + "/settings';";
  html += R"rawliteral(
const el=document.getElementById("count");
const timer=setInterval(function(){
  counter--;
  el.innerText=counter;
  if(counter<=0){
    clearInterval(timer);
    window.location.href=target;
  }
},1000);
</script>
)rawliteral";

  html += "</body></html>";
  return html;
}

// -----------------------------------------------------
// HTTP handlers
// -----------------------------------------------------


void handleRoot() {
  server.send(200, "text/html", splashPage());
}

void handleSettings() {
  if (!requireAuth()) return;
  server.send(200, "text/html", settingsPage());
}

void handleSave() {
  if (!requireAuth()) return;

  if (!server.hasArg("deviceIP") ||
      !server.hasArg("mask") ||
      !server.hasArg("targetIP") ||
      !server.hasArg("port") ||
      !server.hasArg("baud")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  IPAddress testDeviceIP;
  IPAddress testMask;
  IPAddress testTargetIP;

  if (!parseIP(server.arg("deviceIP"), testDeviceIP)) {
    server.send(400, "text/plain", "Invalid Device IP");
    return;
  }

  if (!parseIP(server.arg("mask"), testMask)) {
    server.send(400, "text/plain", "Invalid subnet mask");
    return;
  }

  if (!parseIP(server.arg("targetIP"), testTargetIP)) {
    server.send(400, "text/plain", "Invalid Target IP");
    return;
  }

  long port = server.arg("port").toInt();
  long baud = server.arg("baud").toInt();

  if (port < 1 || port > 65535) {
    server.send(400, "text/plain", "Invalid UDP port");
    return;
  }

  if (baud < 1200) {
    server.send(400, "text/plain", "Invalid UART baudrate");
    return;
  }

  cfg.deviceIP   = server.arg("deviceIP");
  cfg.subnetMask = server.arg("mask");
  cfg.targetIP   = server.arg("targetIP");
  cfg.udpPort    = (uint16_t)port;
  cfg.baudrate   = (uint32_t)baud;

  saveSettings();

  server.send(200, "text/html", savedPage(cfg.deviceIP));

  Serial.println();
  Serial.println("[CONFIG] Settings saved");
  Serial.print("[CONFIG] New IP: ");
  Serial.println(cfg.deviceIP);
  Serial.println("[SYSTEM] Rebooting...");

  delay(1200);
  ESP.restart();
}

void handleNotFound() {
  server.sendHeader("Location", "/settings");
  server.send(302, "text/plain", "");
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("[WEB] HTTP server started");
}

// -----------------------------------------------------
// UART <-> UDP
// -----------------------------------------------------

void uartToUDP() {
  while (TelemetrySerial.available() &&
         uartBufferLength < sizeof(uartBuffer)) {
    uartBuffer[uartBufferLength++] = TelemetrySerial.read();
    lastUARTByteTime = millis();
  }

  if (uartBufferLength > 0 &&
      (millis() - lastUARTByteTime >= 2 ||
       uartBufferLength >= sizeof(uartBuffer))) {

    IPAddress target;

    if (parseIP(cfg.targetIP, target)) {
      udp.beginPacket(target, cfg.udpPort);
      udp.write(uartBuffer, uartBufferLength);
      udp.endPacket();
    }

    uartBufferLength = 0;
  }
}

void udpToUART() {
  int packetSize = udp.parsePacket();

  if (packetSize <= 0) return;

  uint8_t buffer[512];

  while (packetSize > 0) {
    int chunk = packetSize;

    if (chunk > (int)sizeof(buffer)) {
      chunk = sizeof(buffer);
    }

    int len = udp.read(buffer, chunk);

    if (len <= 0) break;

    TelemetrySerial.write(buffer, len);
    packetSize -= len;
  }
}

// -----------------------------------------------------
// Ethernet
// -----------------------------------------------------

void startEthernet() {
  IPAddress localIP;
  IPAddress subnetMask;

  if (!parseIP(cfg.deviceIP, localIP)) {
    localIP = IPAddress(192,168,88,50);
  }

  if (!parseIP(cfg.subnetMask, subnetMask)) {
    subnetMask = IPAddress(255,255,255,0);
  }

  Serial.println();
  Serial.println("[ETH] Starting LAN8720...");

  bool ethStarted = ETH.begin(
    ETH_PHY_TYPE,
    ETH_PHY_ADDR,
    ETH_PHY_MDC,
    ETH_PHY_MDIO,
    ETH_PHY_POWER,
    ETH_CLK_MODE
  );

  if (!ethStarted) {
    Serial.println("[ETH] ETH.begin FAILED");
    return;
  }

  Serial.println("[ETH] PHY started");

  bool configResult = ETH.config(
    localIP,
    IPAddress(0,0,0,0),
    subnetMask,
    IPAddress(0,0,0,0),
    IPAddress(0,0,0,0)
  );

  if (configResult) {
    Serial.println("[ETH] Static IP configured");
  } else {
    Serial.println("[ETH] Static IP configuration FAILED");
  }

  delay(1500);

  Serial.print("[ETH] Device IP: ");
  Serial.println(cfg.deviceIP);

  Serial.print("[ETH] Subnet: ");
  Serial.println(cfg.subnetMask);
}

// -----------------------------------------------------
// Setup / Loop
// -----------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("             bmax_sys BRIDGE");
  Serial.println("               by bmax_sys");
  Serial.println("==========================================");

  loadSettings();

  Serial.println();
  Serial.println("[CONFIG] Loaded");
  Serial.print("Device IP : "); Serial.println(cfg.deviceIP);
  Serial.print("Subnet    : "); Serial.println(cfg.subnetMask);
  Serial.print("Target IP : "); Serial.println(cfg.targetIP);
  Serial.print("UDP Port  : "); Serial.println(cfg.udpPort);
  Serial.print("UART Baud : "); Serial.println(cfg.baudrate);

  TelemetrySerial.begin(
    cfg.baudrate,
    SERIAL_8N1,
    UART_RX_PIN,
    UART_TX_PIN
  );

  Serial.println();
  Serial.println("[UART] Started");

  startEthernet();

  udp.begin(cfg.udpPort);

  Serial.print("[UDP] Listening on port ");
  Serial.println(cfg.udpPort);

  startWebServer();

  Serial.println();
  Serial.println("==========================================");
  Serial.println("[SYSTEM] READY");
  Serial.print("[WEB] Open: http://");
  Serial.println(cfg.deviceIP);
  Serial.println("==========================================");
}

void loop() {
  server.handleClient();
  uartToUDP();
  udpToUART();
  delay(1);
}
