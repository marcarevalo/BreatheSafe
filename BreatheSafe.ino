// **Blynk Credentials**
#define BLYNK_TEMPLATE_ID "TPL6ze3vLcBe"
#define BLYNK_TEMPLATE_NAME "BreatheSafe"
#define BLYNK_AUTH_TOKEN "_PHyqM94Jj9K-i5kFjS2u4Mcx7Ch5tmu"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <DHT.h>

// **Wi-Fi Credentials**
const char* ssid = "****";
const char* password = "HINDIPWEDE";

// **LCD Setup**
LiquidCrystal_PCF8574 lcd(0x27);

// **DHT11 Sensor Setup**
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// **Gas Sensor Setup**
#define GAS_SENSOR_PIN 34
#define BAD_AIR_THRESHOLD 200  
int sensorThreshold = 100;

// **Relay & Buzzer Setup**
#define RELAY_PIN 26
#define BUZZER_PIN 27

// **Control Variables**
bool mistManual = false;  
bool systemOn = true;  
bool alarmTriggered = false;

// **Function to Connect to Blynk**
void connectBlynk() {
    Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 8080);
    Blynk.connect();
}

// **Function to Read Gas Sensor**
int readGasSensor() {
    if (!systemOn) return 0;
    int gasValue = analogRead(GAS_SENSOR_PIN);
    Serial.print("Gas Value: ");
    Serial.println(gasValue);
    return gasValue;
}

// **Function to Determine Air Quality**
String getAirQuality(int gasValue) {
    return (gasValue < BAD_AIR_THRESHOLD) ? "Fresh Air" : "Bad Air";
}

// **Function to Power ON the System**
void powerOnSystem() {
    systemOn = true;
    
    // **Reinitialize LCD**
    lcd.begin(16, 2);
    lcd.setBacklight(255);
    lcd.clear();
    
    lcd.setCursor(2, 0);
    lcd.print("BreatheSafe");
    lcd.setCursor(5, 1);
    lcd.print("by Coy");

    tone(BUZZER_PIN, 1200, 100);
    delay(150);
    tone(BUZZER_PIN, 1500, 100);
    delay(150);
    tone(BUZZER_PIN, 1800, 150);
    delay(200);
    noTone(BUZZER_PIN);

    delay(2000);
    Serial.println("System Powered ON.");
}

// **Function to Power OFF the System**
void shutdownSystem() {
    systemOn = false;
    
    // **Display Shutdown Message on LCD**
    lcd.clear();
    lcd.setBacklight(255);
    lcd.setCursor(1, 0);
    lcd.print("System Shutting");
    lcd.setCursor(5, 1);
    lcd.print("Down...");

    tone(BUZZER_PIN, 1800, 100);
    delay(150);
    tone(BUZZER_PIN, 1500, 100);
    delay(150);
    tone(BUZZER_PIN, 1200, 150);
    delay(200);
    noTone(BUZZER_PIN);

    delay(3000);

    // **LCD remains powered but blank**
    lcd.clear();
    lcd.setBacklight(0);  

    // **Turn OFF Relay & Reset Mist Control**
    digitalWrite(RELAY_PIN, LOW);
    mistManual = false; // Reset to Auto Mode
    
    Serial.println("System Powered OFF.");
}

// **Setup Function (Device Boots Up Automatically)**
void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");

    // **Wi-Fi Connection**
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWi-Fi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // **Connect to Blynk**
    connectBlynk();

    // **Initialize Sensors & Display**
    dht.begin();
    lcd.begin(16, 2);
    lcd.setBacklight(255);

    // **Set Relay & Buzzer Pins**
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    pinMode(BUZZER_PIN, OUTPUT);
    noTone(BUZZER_PIN);

    Serial.println("System Ready!");

    // ✅ **Automatically Turn On System at Startup**
    powerOnSystem();
}

// **Function to Read Sensors & Update Blynk**
void sendSensorData() {
    if (!systemOn) return;

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int gasValue = readGasSensor();
    String gasQuality = getAirQuality(gasValue);

    // **Send Data to Blynk**
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, gasValue);
    Blynk.virtualWrite(V6, gasQuality);

    // **Display Temperature & Humidity on LCD**
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print((char)223); // Degree Symbol (°)
    lcd.print("C");
    lcd.setCursor(15, 0);  
    lcd.print(mistManual ? "M" : "A");  

    lcd.setCursor(0, 1);
    lcd.print("Humid: ");
    lcd.print(humidity);
    lcd.print("%");

    delay(3000);

    // **Display Gas Value & Air Quality on LCD**
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gas Value: ");
    lcd.print(gasValue);
    lcd.setCursor(15, 0);
    lcd.print(mistManual ? "M" : "A");

    lcd.setCursor(0, 1);
    lcd.print(gasQuality);

    delay(3000);

    // **Trigger Alarm if Gas is Bad (≥200)**
    if (gasValue >= BAD_AIR_THRESHOLD && !alarmTriggered) {
        Serial.println("ALERT: Bad Air Detected! Activating Buzzer.");
        tone(BUZZER_PIN, 1000);  
        delay(2000);  
        noTone(BUZZER_PIN);
        alarmTriggered = true;  
        Blynk.logEvent("pollution_alert", "Bad Air Detected!");
    } else if (gasValue < BAD_AIR_THRESHOLD) {
        alarmTriggered = false;  
    }

    // **Relay Control Logic (Auto Mode)**
    if (!mistManual) {  
        if (temperature >= 30) {
            digitalWrite(RELAY_PIN, HIGH);
        } else {
            digitalWrite(RELAY_PIN, LOW);
        }
    }

    // **Send Relay State to Blynk**
    Blynk.virtualWrite(V3, digitalRead(RELAY_PIN));
}

// **BLYNK: Mist Mode Control (V3)**
BLYNK_WRITE(V3) {
    int state = param.asInt();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    
    if (state == 1) {  
        mistManual = true;
        digitalWrite(RELAY_PIN, HIGH);
        lcd.print("Mist Turned ON");
    } else {  
        mistManual = false;
        digitalWrite(RELAY_PIN, LOW);
        lcd.print("Mist Turned OFF");
    }

    lcd.setCursor(15, 0);
    lcd.print(mistManual ? "M" : "A");  
    delay(2000);
    sendSensorData();
}

// **BLYNK: Power ON/OFF Control (V4)**
BLYNK_WRITE(V4) {
    int state = param.asInt();
    if (state == 1) {
        powerOnSystem();
    } else {
        shutdownSystem();
    }
}

// **Loop Function**
void loop() {
    if (Blynk.connected()) {
        Blynk.run();
    } else {
        Serial.println("Blynk Connection Failed... Retrying");
        connectBlynk();
    }
    if (systemOn) {
        sendSensorData();
    }
    delay(5000);
}
