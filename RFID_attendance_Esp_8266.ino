#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

// Define pins
#define RST_PIN 5
#define SS_PIN 4
#define BUZZER_PIN 16
#define SDA_PIN 0
#define SCL_PIN 2

// WiFi credentials
const char* ssid = "WIFI SSID";
const char* password = "WIFI password";

// Server Details
const char* serverIP = "192.168.1.1"; // Use only the IP
const int serverPort = 8000; // Define port separately
const char* endpoint = "API URL"; // API Endpoint

// Initialize LCD & RFID Module
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempt++;
    if (attempt > 20) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Failed!");
      return;
    }
  }
  Serial.println("\nWiFi Connected!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID Card");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = formatUID();
    Serial.println("Card UID: " + uid);
    
    // Get the response message from the server
    String responseMessage = sendUIDToServer(uid);

    // Check if scanning is available or not
    bool scanningAvailable = checkScanningAvailability(responseMessage);

    // Beep the buzzer accordingly
    if (scanningAvailable) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500); // Shorter beep for available scan
      digitalWrite(BUZZER_PIN, LOW);
    } else {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(2000); // Longer beep for unavailable scan
      digitalWrite(BUZZER_PIN, LOW);
    }

    // Display the server's response message on the LCD
    displayServerMessage(responseMessage);

    mfrc522.PICC_HaltA();
  }
}

// Function to format RFID UID correctly
String formatUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// Function to send RFID UID to the server using HTTP POST and get the response message
String sendUIDToServer(String uid) {
  String responseMessage = "";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullURL = "http://" + String(serverIP) + ":" + String(serverPort) + String(endpoint);
    http.begin(client, fullURL);
    http.addHeader("Content-Type", "application/json");
    
    String postData = "{\"school_id\": \"" + uid + "\", \"current_time_hour\": \"" + 7 + "\", \"current_time_minute\": \"" + 45 + "\"}"; // JSON payload with school_id
    Serial.println("Sending Data: " + postData);

    int httpResponseCode = http.POST(postData); // Send POST request with JSON body
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server Response: " + response);

      // Parse the response for the "message" field
      int messageStart = response.indexOf("\"message\"") + 11;
      int messageEnd = response.indexOf("\"", messageStart);
      if (messageStart != -1 && messageEnd != -1) {
        responseMessage = response.substring(messageStart, messageEnd);
      } else {
        responseMessage = "Error parsing response!";
      }
    } else {
      Serial.print("Error Sending Data: ");
      Serial.println(httpResponseCode);
      responseMessage = "Error sending data";
    }
    http.end();
  } else {
    Serial.println("WiFi Not Connected!");
    responseMessage = "WiFi Not Connected!";
  }

  return responseMessage;
}

// Function to check if the scanning is available based on the server response
bool checkScanningAvailability(String responseMessage) {
  // If the message contains the phrase "ID Scanning not available yet", scanning is not available
  if (responseMessage.indexOf("ID Scanning not available yet") != -1) {
    return false; // Scanning is not available
  }
  return true; // Scanning is available
}

// Function to display the server response message on the LCD
void displayServerMessage(String message) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);  // Display the message from the server
  delay(2000);  // Display the message for 2 seconds
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID Card");
}
