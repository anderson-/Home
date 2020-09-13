#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Hash.h>
#include <FS.h>

#define HTTP_INFO "uptime=[{H},{M},{S}]; state=[{a},{b}]; "
#define HTTP_INFO_BUILD HTTP_INFO "build='" __DATE__ " " __TIME__ "';"
const char HTTP_INFO_SCRIPT[] PROGMEM = HTTP_INFO_BUILD;

const char *ssid = "AP1208";
const char *password = "fifigorda165";
char* host = "192.168.100.8";
const int httpPort = 8000;

HTTPClient http;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const int led = 2;
const int avr_rst = 0;
bool w = false;
bool r = false;

void post_data(String data){
  webSocket.broadcastTXT(data);

  // http.begin(host, httpPort);
  // http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // http.POST(data);
  // http.end();
}

void handleSerial() {                         // If a POST request is made to URI /login
  if( ! server.hasArg("serial") || server.arg("serial") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  Serial.println(server.arg("serial"));
  server.send(204, "text/plain", "");
}

void handleLoginP() {                          // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Username\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Login\"></form><p>Try 'John Doe' and 'password123' ...</p>");
}

void handleLogin() {                         // If a POST request is made to URI /login
  if( ! server.hasArg("username") || ! server.hasArg("password")
      || server.arg("username") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  if(server.arg("username") == "John Doe" && server.arg("password") == "password123") { // If both the username and the password are correct
    server.send(200, "text/html", "<h1>Welcome, " + server.arg("username") + "!</h1><p>Login successful</p>");
  } else {                                                                              // Username and password don't match
    server.send(401, "text/plain", "401: Unauthorized");
  }
}

void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x+= 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send ( 200, "image/svg+xml", out);
}

void resetArduino(){
  digitalWrite(avr_rst, 1);
  delay(10);
  digitalWrite(avr_rst, 0);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      // Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      // Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

      // send message to client
      webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      // Serial.printf("[%u] get Text: %s\n", num, payload);
      webSocket.sendTXT(num, "received: " + String((char*)payload));
      if(payload[0] == '#') {
        Serial.println((char*)(payload + 1));
      } else if(payload[0] == '!') {
        resetArduino();
      } else if(payload[0] == '-') {
        Serial.end();
        pinMode(1, INPUT);
        pinMode(3, INPUT);
      } else if(payload[0] == '+') {
        Serial.begin(115200);
      }
      break;
  }
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  MDNS.begin("esp8266");

  SPIFFS.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/serial", handleSerial);
  server.on("/test.svg", drawGraph);
  server.on("/l", HTTP_GET, handleLoginP);        // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/login", HTTP_POST, handleLogin); // Call the 'handleLogin' function when a POST request is made to URI "/login"
  server.on("/info.js", []() {
    String info = FPSTR(HTTP_INFO_SCRIPT);
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;
    info.replace("{H}", String(hr));
    info.replace("{M}", String(min % 60));
    info.replace("{S}", String(sec % 60));
    info.replace("{a}", String(w));
    info.replace("{b}", String(r));
    // info.replace("{}", );
    server.send(200, "application/javascript", info);
  });
  server.on("/ra", []() {
    resetArduino();
    server.send(200, "text/plain", "ok");
  });

  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      String message = "File Not Found\n\n";
      message += "URI: ";
      message += server.uri();
      message += "\nMethod: ";
      message += (server.method() == HTTP_GET) ? "GET" : "POST";
      message += "\nArguments: ";
      message += server.args();
      message += "\n";
      for(uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
      }

      Dir dir = SPIFFS.openDir("/");
      String path = String();
      message += "\n";

      while (dir.next()) {
        message += dir.fileName();
        message += " [size: ";
        message += dir.fileSize();
        message += "]\n";
      }
      message += "\n";

      FSInfo fs_info;
      SPIFFS.info(fs_info);
      message += "FS usage ";
      message += String((((float)fs_info.usedBytes)/fs_info.totalBytes)*100, 2);
      message += "%%";

      server.send(404, "text/plain", message);
    }
  });

  server.begin();
  // server.setNoDelay(true);

  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  Serial.println("Hello World");

  //OTA
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  post_data("IP address: " + WiFi.localIP());
  pinMode(led, OUTPUT);
  pinMode(avr_rst, OUTPUT);
  resetArduino();
}

void loop(void) {
  ArduinoOTA.handle();
  server.handleClient();
  webSocket.loop();
  if(Serial.available() > 4){
    String data = Serial.readStringUntil('\n');
    post_data(data);
  }
}
