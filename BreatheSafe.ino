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

// **Relay & Buzzer Setup**
#define RELAY_PIN 26
#define BUZZER_PIN 27

// **Control Variables**
bool mistManual = false;  // MistSwitch Mode (ON = Manual, OFF = Auto)
bool systemOn = false;

// **Function to Connect to Blynk**
void connectBlynk() {
    Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 8080);
    Blynk.connect();
}

// **Setup Function**
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
}

// **Function to Read Sensors & Update Blynk**
void sendSensorData() {
    if (!systemOn) return;  // Skip if system is off

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    // **Convert Gas Sensor Value to Percentage**
    int rawGasValue = analogRead(GAS_SENSOR_PIN);
    int gasPercentage = map(rawGasValue, 0, 4095, 0, 100); // Mapping 0-4095 ADC to 0-100%

    // **Determine Gas Level Quality**
    String gasQuality = (gasPercentage <= 70) ? "Good Air" : "Bad Air";

    // **Send Data to Blynk**
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, gasPercentage);
    Blynk.virtualWrite(V6, gasQuality); // Sending Gas Quality Status

    // **Display Temperature & Humidity on LCD**
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: "); 
    lcd.print(temperature);
    lcd.print((char)223); // **Degree Symbol (°)**
    lcd.print("C");
    lcd.setCursor(15, 0);  // Move to the right edge
    lcd.print(mistManual ? "M" : "A");

    lcd.setCursor(0, 1);
    lcd.print("Humid: "); lcd.print(humidity); lcd.print("%");

    delay(3000);
    
    // **Display Gas Value & Air Quality on LCD**
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gas Value: ");
    lcd.print(gasPercentage);
    lcd.print("%");
    lcd.setCursor(15, 0);  // Move to the right edge
    lcd.print(mistManual ? "M" : "A");

    lcd.setCursor(4, 1);  // **Centered Air Quality Message**
    lcd.print(gasQuality);

    delay(3000);

    // **Auto Mist Control (Only in Auto Mode)**
    if (!mistManual) {  
        bool relayState = (temperature >= 27);  // Auto Mode: Turn on mist at ≥27°C
        digitalWrite(RELAY_PIN, relayState);
    }

    // **Send Relay State to Blynk**
    Blynk.virtualWrite(V3, digitalRead(RELAY_PIN));
}

// **Function to Power ON the System**
void powerOnSystem() {
    systemOn = true;
    lcd.setBacklight(255);
    lcd.clear();
    
    // **Centered "BreatheSafe" Text**
    lcd.setCursor(2, 0);
    lcd.print("BreatheSafe");
    lcd.setCursor(5, 1);  // Centered "by Coy"
    lcd.print("by Coy");

    // **Play Power On Sound**
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
    lcd.clear();
    lcd.setBacklight(255);

    // **Centered "BreatheSafe" Text**
    lcd.setCursor(2, 0);
    lcd.print("BreatheSafe");
    lcd.setCursor(5, 1);  // Centered "by Coy"
    lcd.print("by Coy");

    // **Play Power Off Sound**
    tone(BUZZER_PIN, 1800, 100);
    delay(150);
    tone(BUZZER_PIN, 1500, 100);
    delay(150);
    tone(BUZZER_PIN, 1200, 150);
    delay(200);
    noTone(BUZZER_PIN);

    delay(3000);
    lcd.clear();
    lcd.setBacklight(0);
    lcd.noDisplay();
    digitalWrite(RELAY_PIN, LOW);

    Serial.println("System Powered OFF.");
}

// **BLYNK: MistSwitch Control (V3)**
BLYNK_WRITE(V3) {
    int state = param.asInt();
    
    if (state == 1) {  // **Manual Mode: Turn Mist ON**
        mistManual = true;
        digitalWrite(RELAY_PIN, HIGH);
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Mist Turn ON");
        Serial.println("Mist Manually Turned ON");
    } else {  // **Auto Mode: Enable Temperature Control**
        mistManual = false;
        digitalWrite(RELAY_PIN, LOW);
        lcd.clear();
        lcd.setCursor(2, 0);
        lcd.print("Mist Turn OFF");
        Serial.println("Mist Switched to Auto Mode");
    }

    delay(2000);
    sendSensorData(); // Ensure sensor values update immediately

    // **Send Updated Status to Blynk**
    Blynk.virtualWrite(V3, digitalRead(RELAY_PIN));
}

// **BLYNK: System Power Control (V4)**
BLYNK_WRITE(V4) {
    int powerState = param.asInt();
    if (powerState == 1) {
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
    sendSensorData();
    delay(5000);
}
