#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// 🛠️ Replace with your credentials and API key
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey = "sk-XXXXXXXXXXXXXXXXXXXXXXXXXXXX";  // OpenAI API key
const char* host = "api.openai.com";

ESP8266WebServer server(80);
WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setInsecure(); // ⚠️ Skips SSL certificate validation

  server.on("/", HTTP_GET, handleRoot);
  server.on("/ask", HTTP_POST, handleAsk);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

// 🧠 Web UI for user input
void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ChatGPT on ESP8266</h1>";
  html += "<form action='/ask' method='POST'>";
  html += "Ask ChatGPT: <input type='text' name='question' style='width:300px'/>";
  html += "<input type='submit' value='Ask'/>";
  html += "</form></body></html>";

  server.send(200, "text/html", html);
}

// 💬 Handles form submission and API request
void handleAsk() {
  String question = server.arg("question");
  String reply = getChatGPTResponse(question);

  String html = "<html><body>";
  html += "<h1>ChatGPT Response</h1>";
  html += "<p><strong>Q:</strong> " + question + "</p>";
  html += "<p><strong>A:</strong> " + reply + "</p>";
  html += "<br><a href='/'>Ask another</a></body></html>";

  server.send(200, "text/html", html);
}

// 📡 Send request to OpenAI and get response
String getChatGPTResponse(String question) {
  String jsonBody = "{\"model\":\"gpt-3.5-turbo\",\"messages\":[{\"role\":\"user\",\"content\":\"" + question + "\"}]}";

  if (!client.connect(host, 443)) {
    Serial.println("Connection failed.");
    return "Error: Could not connect to OpenAI.";
  }

  // Send HTTP request
  client.println("POST /v1/chat/completions HTTP/1.1");
  client.println("Host: api.openai.com");
  client.println("Content-Type: application/json");
  client.print("Authorization: Bearer ");
  client.println(apiKey);
  client.print("Content-Length: ");
  client.println(jsonBody.length());
  client.println();
  client.println(jsonBody);

  // 🔍 Skip HTTP headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // 📥 Read JSON body
  String response = "";
  while (client.available()) {
    char c = client.read();
    response += c;
  }

  Serial.println("JSON body:");
  Serial.println(response);

  client.stop();
  return response;
}

// 🧩 Parse the response and extract ChatGPT's message
String parseResponse(String jsonResponse) {
  const size_t capacity = 8192;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.f_str());
    return "Error parsing response.";
  }

  if (doc["choices"][0]["message"]["content"]) {
    return doc["choices"][0]["message"]["content"].as<String>();
  }

  return "Invalid response format.";
}
