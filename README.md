The advancement of IoT-based healthcare solutions has significantly improved patient monitoring systems. This project presents a real-time health monitoring system using an ESP32 microcontroller integrated with Radio Frequency IDentification(RFID)-based patient identification, biometric sensors, Firebase Realtime Database, and the Blynk app for remote tracking. The system functions by scanning an RFID tag assigned to a patient, ensuring accurate identification. Multiple sensors collect vital parameters such as heart rate, oxygen level (SpO₂), and body temperature, which are then transmitted to a cloud-based Firebase database under the respective patient’s unique RFID ID. The stored data can be accessed by authorized medical professionals or caregivers, enabling real-time monitoring. Furthermore, the system integrates with Blynk, allowing users to remotely view patient health data via a mobile application. This approach reduces manual record-keeping errors, enhances patient safety, and ensures seamless remote healthcare monitoring. The proposed system is particularly useful for hospitals, elderly care facilities, and home healthcare applications, providing a cost-effective, wireless, and cloud-integrated solution for efficient patient management.![image](https://github.com/user-attachments/assets/ab30ffd5-9e2f-4ebb-af05-10bb88ae5747)

# CODE EXPLANATION 

→ #include tells the compiler to include a library. → Wire.h is the library that allows the ESP32 to communicate with I2C devices (two-wire communication).
→ SPI.h is a library that allows communication over the SPI (Serial Peripheral Interface) bus. Used here because the RFID reader (MFRC522) uses SPI communication.
→ MFRC522.h is the library specifically written for controlling the RFID RC522 module. It simplifies reading RFID cards and tags.
→ “MAX30105.h” is the header file for the library that helps communicate with the MAX30102/MAX30105 sensor. These are used for measuring heartbeat (BPM) and oxygen level (SpO₂).
→ #define creates a constant. Here, BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME are unique identifiers for your Blynk IoT project (used when connecting to Blynk’s server).
→ This includes the Blynk library version tailored for ESP32. It enables you to send/receive data to/from the Blynk App on your mobile.
→ WiFi.h: Needed to connect ESP32 to a WiFi network.
→ HTTPClient.h: Allows sending HTTP requests like GET or POST to a server.
→ FirebaseESP32.h: Library for interacting with Firebase RealTime Database from ESP32.
→ #define constants for your Firebase Database URL and the Authorization Key (Legacy Token) — required to securely interact with Firebase.
→ Define the Slave Select (SS) and Reset (RST) pins used by the RFID module on ESP32.
→ Define SDA (Data Line) and SCL (Clock Line) pins for I2C communication (for MAX30102 sensor).
→ Define the input pins for ECG sensor:
	•	ECG_PIN: Analog signal output from ECG electrodes.
	•	LO_PLUS and LO_MINUS: Detect if ECG electrodes are properly connected.
 → This auth key is used to connect your ESP32 to your Blynk app project.
 → Create objects:
	•	rfid: For accessing functions related to the RFID reader.
	•	particleSensor: For accessing heart rate and oxygen data from MAX30102 sensor.
 → Variables for RFID handling:
	•	allowedUIDs: Stores up to 5 allowed UIDs (each UID has 4 bytes).
	•	allowedNames: Names of people associated with each UID.
	•	registeredCards: Number of registered RFID cards.
	•	rfidVerified: Flag to indicate if a card was authorized.
	•	currentUserName: Store the name of the person who scanned the RFID.
 → Firebase objects:
	•	firebaseData: Used for sending/receiving data.
	•	firebaseAuth: Authorization settings.
	•	firebaseConfig: Configuration settings.
→ WiFi credentials to connect the ESP32 to your mobile hotspot called “POCO X5 PRO”.
→ URL to your Google Sheets script that accepts HTTP POST requests to save data into a spreadsheet.
→ The setup() function runs once when the ESP32 starts.
→ Start serial communication at a baud rate of 115200 for debugging (sending messages to the Serial Monitor).
→ Print a starting message for debugging.
→ Initialize the SPI bus with specific pins:
	•	SCK: 18
	•	MISO: 19
	•	MOSI: 23
	•	SS: 5
→ Short delay for stability.
→ Initialize the RFID reader module.
→ Begin I2C communication using the defined SDA and SCL pins.
→ Try to initialize the MAX30102 sensor.
If initialization fails, print an error.
→ If sensor is found:
	•	Print a success message.
	•	Run basic setup.
	•	Adjust LED intensities for the red and IR LEDs.
 → Configure the Firebase host URL and authentication token.
→ Start Firebase connection and allow automatic WiFi reconnections.
→ Connect to WiFi network.
→ Keep checking until WiFi is successfully connected.
→ Print success message after connecting to WiFi.
→ Fetch the list of allowed RFID UIDs and their names from Firebase.
→ Initialize Blynk connection using credentials and WiFi.
→ Runs continuously after setup.
→ Blynk must run continuously to maintain connection with the server.
→ If no authorized card has been detected yet, ask the user to scan.
→ Check if a new RFID card is present. If not, wait a little and return.
→ Try reading card data. If fails, wait and return.
→ Print the UID of the scanned card to Serial Monitor in Hexadecimal format.
→ If UID matches the database:
	•	Set rfidVerified to true.
	•	Set pins for ECG sensor inputs.
	•	Start measuring BPM and SpO₂.
 → If UID does not match, show unauthorized message.
 → Halt RFID card communication after reading.
