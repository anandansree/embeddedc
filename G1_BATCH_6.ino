#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include "MAX30105.h"   

#define BLYNK_TEMPLATE_ID "TMPL3ZLGaZLq_"
#define BLYNK_TEMPLATE_NAME "Embcccc"
#include <BlynkSimpleEsp32.h>  

#include <WiFi.h>
#include <HTTPClient.h>
#include <FirebaseESP32.h>

#define FIREBASE_HOST "https://health-monitoring-system-2-default-rtdb.firebaseio.com/"  
#define FIREBASE_AUTH "4Lc6GJ85Q0TJYVpM2uwTNq2SvNGZs5GnsMe6gNfk"  

#define SS_PIN 5        
#define RST_PIN 4       

#define I2C_SDA 21      
#define I2C_SCL 22      

#define ECG_PIN 34   
#define LO_PLUS 25   
#define LO_MINUS 26  

char auth[] = "fqCYFLsMDAovx1UesL9oICnNy1InFpHB";  

MFRC522 rfid(SS_PIN, RST_PIN);
MAX30105 particleSensor;  

byte allowedUIDs[5][4]; 
String allowedNames[5]; 
int registeredCards = 0; 
bool rfidVerified = false; 
String currentUserName = "";

FirebaseData firebaseData;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

const char* ssid = "POCO X5 PRO";
const char* password = "asdfghjkl";

String googleSheetsURL = "https://script.google.com/macros/s/AKfycbz5mF6v_KC4lSz9snkVEqyCLDqAAkYRBsfHBcPqpaAZhwfl-wqFIpeVPIBmvptwn3pTZw/exec";  

void setup() {
    Serial.begin(115200);
    Serial.println("Starting System...");
    
    SPI.begin(18, 19, 23, 5);  
    delay(100);
    rfid.PCD_Init();
    
    Wire.begin(I2C_SDA, I2C_SCL);  
    
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {  
        Serial.println("MAX30102 NOT found! Check wiring.");
    } else {
        Serial.println("MAX30102 Initialized.");
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x0A);
        particleSensor.setPulseAmplitudeIR(0x1F);
    }
    
    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
    
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);
    
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Still connecting...");
    }
    Serial.println("Connected to WiFi");

    getAllowedUIDsFromFirebase();
    
    Serial.println("Initializing Blynk...");
    Blynk.begin(auth, ssid, password);
    Serial.println("Blynk initialization complete");
}

void loop() {
    Blynk.run();  

    if (!rfidVerified) {
        Serial.println("Waiting for RFID card...");
        
        if (!rfid.PICC_IsNewCardPresent()) {
            delay(500);
            return;
        }
        if (!rfid.PICC_ReadCardSerial()) {
            Serial.println("Card detected but UID cannot be read...");
            delay(500);
            return;
        }

        Serial.print("Card UID: ");
        for (byte i = 0; i < rfid.uid.size; i++) {
            Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(rfid.uid.uidByte[i], HEX);
        }
        Serial.println();

        if (isUIDAuthorized(rfid.uid.uidByte)) {
            Serial.println("✅ Authorized Card! Switching to BPM Monitoring...");
            rfidVerified = true;  
            pinMode(ECG_PIN, INPUT);
            pinMode(LO_PLUS, INPUT);
            pinMode(LO_MINUS, INPUT);
            calculateBPMandSpO2(); 
        } else {
            Serial.println("❌ Unauthorized Card.");
        }

        rfid.PICC_HaltA();
    }
}

void getAllowedUIDsFromFirebase() {
    registeredCards = 0;

    for (int userIndex = 0; userIndex < 5; userIndex++) {
        String basePath = "/allowedUIDs/" + String(userIndex);
        String uidPath = basePath + "/uid";
        String namePath = basePath + "/name";

        if (!Firebase.get(firebaseData, uidPath)) break;

        Serial.print("Fetched UID for user ");
        Serial.print(userIndex);
        Serial.print(": ");

        for (int i = 0; i < 4; i++) {
            String path = uidPath + "/" + String(i);
            if (Firebase.getInt(firebaseData, path)) {
                allowedUIDs[userIndex][i] = firebaseData.intData();
                Serial.print(allowedUIDs[userIndex][i], HEX);
                Serial.print(" ");
            } else {
                Serial.println("Error fetching UID part " + String(i));
                return;
            }
        }

        if (Firebase.getString(firebaseData, namePath)) {
            allowedNames[userIndex] = firebaseData.stringData();
            Serial.print(" Name: ");
            Serial.println(allowedNames[userIndex]);
        } else {
            allowedNames[userIndex] = "Unknown";
        }

        registeredCards++;
    }

    Serial.print("Total Registered Cards: ");
    Serial.println(registeredCards);
}

bool isUIDAuthorized(byte *uid) {
    for (int userIndex = 0; userIndex < registeredCards; userIndex++) {
        bool match = true;
        for (int i = 0; i < 4; i++) {
            if (uid[i] != allowedUIDs[userIndex][i]) {
                match = false;
                break;
            }
        }
        if (match) {
          currentUserName = allowedNames[userIndex]; 
          return true;
        }
    }
    return false;
}

void calculateBPMandSpO2() {
    Serial.println("Place your finger on the MAX30102 sensor...");
    delay(2000); 
    
    if (!particleSensor.check()) {
        Serial.println("MAX30102 sensor not responding");
        return;
    }

    const int sampleSize = 100;
    uint32_t irValues[sampleSize];
    uint32_t redValues[sampleSize];
    uint32_t timeStamps[sampleSize];

    for (int i = 0; i < sampleSize; i++) {
        unsigned long startTime = millis();
        do {
            irValues[i] = particleSensor.getIR();
            redValues[i] = particleSensor.getRed();
            Serial.print("IR Value: ");
            Serial.print(irValues[i]);
            Serial.print(" Red Value: ");
            Serial.println(redValues[i]);
        } while (irValues[i] < 50000 || redValues[i] < 17000);
        
        timeStamps[i] = millis();
        delay(100);
    }

    float r = 0, spO2;
    for (int i = 0; i < sampleSize; i++) {
        r += ((float)redValues[i] / (float)irValues[i]);
    }
    r /= sampleSize;

    float a = 50;  
    float b = 90;  

    spO2 = a * log(r + 1) + b;  
    spO2 = constrain(spO2, 0, 100);    

    Serial.print("Estimated SpO2 (Logarithmic model): ");
    Serial.println(spO2);

    Blynk.virtualWrite(V0, spO2);

    float bpmValues[10];
    for (int subset = 0; subset < 10; subset++) {
        int peakCount = 0;
        for (int i = subset * 10 + 1; i < (subset + 1) * 10 - 1; i++) {
            if (irValues[i] > irValues[i - 1] && irValues[i] > irValues[i + 1] && irValues[i] > 50000) {
                peakCount++;
            }
        }
        if (peakCount >= 2) {
            uint32_t timeDiff = timeStamps[(subset + 1) * 10 - 1] - timeStamps[subset * 10];
            bpmValues[subset] = (peakCount * 60000.0) / timeDiff;
            Serial.print("BPM (Subset ");
            Serial.print(subset + 1);
            Serial.print("): ");
            Serial.println(bpmValues[subset]);
        } else {
            bpmValues[subset] = 0;
            Serial.println("Insufficient peaks for BPM calculation.");
        }
    }
    
    float totalBPM = 0;
    int validBPMCount = 0;
    for (int i = 0; i < 10; i++) {
        if (bpmValues[i] > 0) {
            totalBPM += bpmValues[i];
            validBPMCount++;
        }
    }
    float avgBPM = (validBPMCount > 0) ? (totalBPM / validBPMCount) : 0;
    Serial.print("Average BPM: ");
    Serial.println(avgBPM);
    
    Blynk.virtualWrite(V1, avgBPM);

    if (digitalRead(LO_PLUS) == 1 || digitalRead(LO_MINUS) == 1) {
        Serial.println("Electrodes not properly connected!");
        return;
    }

    int ecgValue = analogRead(ECG_PIN);
    Blynk.virtualWrite(V2, ecgValue);
    
    const int duration = 20000; 
    unsigned long startECGTime = millis();
    Serial.println("ECG Start");

    while (millis() - startECGTime < duration) {
        if (digitalRead(LO_PLUS) == 1 || digitalRead(LO_MINUS) == 1) {
          Serial.println("Electrodes not properly connected!");
          delay(100);
          continue;
        }

    int ecgValue = analogRead(ECG_PIN);
    
    Serial.println(ecgValue);

    Blynk.virtualWrite(V2, ecgValue);

    delay(20);  
    }

    Serial.println("ECG End");

    Serial.println("---------------------------");
    Serial.print("Name: "); Serial.println(currentUserName);
    Serial.print("Final Average BPM: ");
    Serial.println(avgBPM);
    Serial.print("Final Average SpO2: ");
    Serial.println(spO2);
    Serial.print("ECG Value: ");
    Serial.println(ecgValue);
    Serial.println("---------------------------");

if (avgBPM < 70) {
    Serial.println("⚠️ You have low BPM, please go visit a doctor.");
} else if (avgBPM >= 70 && avgBPM <= 105) {
    Serial.println("✅ Your BPM is measured, You are healthy.");
} else if (avgBPM > 105) {
    Serial.println("⚠️ You have high BPM, please go visit a doctor.");
}

if (spO2 >= 95 && spO2 <= 100) {
    Serial.println("✅ Oxygen Level is good. You are healthy.");
} else if (spO2 < 95) {
    Serial.println("⚠️ Take further readings. If it is still low, go visit a doctor.");
}

    Firebase.setFloat(firebaseData, "/health/spO2", spO2);
    Firebase.setFloat(firebaseData, "/health/bpm", avgBPM);
    Firebase.setFloat(firebaseData, "/health/ecg", ecgValue);
    uploadToGoogleSheets(avgBPM, spO2, ecgValue, currentUserName);
    Serial.println("Data uploaded to Google Sheets!");
}

void uploadToGoogleSheets(float avgBPM, float spO2, int ecgValue, String name) {
    if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String jsonData = "{\"name\": \"" + name + "\", \"bpm\": " + String(avgBPM) +
                          ", \"spO2\": " + String(spO2) + ", \"ecg\": " + String(ecgValue) + "}";

        http.begin(googleSheetsURL);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response: " + response);
        } else {
            Serial.println("Error sending POST request");
        }

        http.end();
    }
}