# 🛰️ Smart Personal Tracker System

**Author:** JK Abhinav (7th Semester, Electronics and Communication Engineering)

## 📖 Overview
This repository contains the firmware and dashboard code for my major project: an IoT-based Smart Personal Tracker System. The system utilizes an ESP32 microcontroller integrated with GPS, GSM, and motion sensors to provide real-time location monitoring and automated emergency alerts. 

It is designed to enhance personal safety by automatically detecting immobility (potential accidents/falls) and providing manual SOS capabilities.

---

## ⚙️ How the System Functions (Step-by-Step)

The project operates in a continuous loop, handling sensor data, making logical decisions, and updating the cloud in real-time. Here is the step-by-step data flow:

### 1. Data Acquisition (The Sensors)
* **Location Tracking:** The SIM28 GPS module continuously receives satellite data. [cite_start]The ESP32 parses the `$GPGGA` NMEA sentences to extract precise Latitude and Longitude decimal coordinates [cite: 194, 202, 228-244].
* **Movement Monitoring:** The ADXL335 accelerometer outputs analog voltages for the X, Y, and Z axes. [cite_start]The ESP32 samples these values via its ADC (Analog-to-Digital Converter) and calculates the physical "delta" (change in position) to determine if the user is moving [cite: 132-146].

### 2. Core Logic & Alert Triggers (The ESP32)
The ESP32 processes the sensor data and reacts based on three primary conditions:
* [cite_start]**Normal Operation (2-Minute Heartbeat):** While the user is moving normally, the ESP32 sends a routine "MOVING" or "STILL" status update along with current coordinates to the cloud every 120 seconds[cite: 100, 101, 102].
* [cite_start]**Automated Inactivity Alert:** If the accelerometer detects no significant movement for exactly **60 seconds**, the system assumes a potential emergency (like a fall or accident)[cite: 24, 76, 77]. [cite_start]It automatically flags the status as "INACTIVITY"[cite: 86].
* [cite_start]**Manual SOS Alert:** If the user presses the physical panic button (connected to GPIO 13), it immediately interrupts normal operations and flags an "SOS" status[cite: 21, 90, 98].

### 3. Communication & Cloud Storage (GSM & Apps Script)
When an alert (Inactivity or SOS) is triggered:
* [cite_start]**SMS Dispatch:** The SIM900A GSM module sends an immediate text message to a pre-programmed emergency contact[cite: 84, 97]. [cite_start]The SMS contains the alert type and a direct, clickable Google Maps link [cite: 80-82, 92-94].
* [cite_start]**Cloud Sync:** The ESP32 sends an HTTP GET request to a Google Apps Script Web App [cite: 421-427]. The Apps Script parses this URL and securely appends the Date, Time, Latitude, Longitude, and Status into a Google Sheet database.

### 4. Real-Time Web Dashboard (The User Interface)
* [cite_start]**Live Fetching:** The web dashboard uses JavaScript (`fetchTrackerData`) to pull the latest JSON data from the Google Apps Script every 30 seconds[cite: 575, 579, 580, 633].
* **Dynamic Display:** The activity log table populates with the latest history. [cite_start]Any row with an "SOS" or "INACTIVITY" status is automatically highlighted in bold red for high visibility [cite: 586-588].
* [cite_start]**Map Integration:** The dashboard extracts the most recent coordinates and automatically updates the embedded Google Maps iframe, providing a live visual pin of the user's location [cite: 599, 603-604, 610-616].

---

## 🛠️ Hardware Components
* **ESP32 Microcontroller** (Core processing and WiFi/Cloud sync)
* **SIM28 GPS Module** (UART communication for location)
* **SIM900A GSM Module** (UART communication for SMS)
* **ADXL335 Accelerometer** (Analog sensing for movement detection)
* **Push Button** (Manual SOS trigger)

## 💻 Software & Technologies
* **Firmware:** ESP-IDF framework (C/C++), FreeRTOS for task scheduling
* **Frontend:** HTML5, CSS3, JavaScript (Fetch API)
* **Backend/Database:** Google Apps Script, Google Sheets

---

## 📊 Project Results & Demonstrations
*Note: All hardware setup photos, testing results, and dashboard screenshots are located in the separate `results.md` file included in this repository.*

---

## 🚀 Setup & Configuration Instructions
1. Clone this repository to your local machine.
2. Open `main/tracker_wifi_cloud.h` and configure your local WiFi credentials and Google Apps Script URL.
3. Open `main/tracker_gsm.h` and configure the emergency contact phone number.
4. In the `main/Tracker_Website` folder, update `script.js` with your Google Apps Script Web App URL and `index.html` with your Google Sheets link.
5. Build and flash the code to your ESP32 using the ESP-IDF framework.
