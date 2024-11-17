#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Stepper.h>
#include <Preferences.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Constants
#define ALERT_BUTTON_PIN 4 // GPIO pin for alert button
#define HAND_SENSOR_PIN 15 // GPIO pin for the IR hand sensor (update as needed)
#define EMERGENCY_LED_PIN 33 // GPIO pin for emergency LED

#define BUZZER_PIN 32

// Define your bot token and chat ID here
#define BOT_TOKEN "Add your Bot token"
#define CHAT_ID ""  // Replace with your chat ID


// ChatGPT API details
#define CHATGPT_API_KEY "paste API key"
#define CHATGPT_API_URL "paste the url here"

const int stepsPerRevolution = 768;
const int numDeliveryTimes = 3;
int a = 0;

// Global objects
Stepper myStepper(stepsPerRevolution, 14, 27, 13, 12);
String medicationDeliveryTimes[numDeliveryTimes];
String medicationNames[numDeliveryTimes];

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
AsyncWebServer server(80);
Preferences preferences;
WiFiClientSecure client;

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long previousMillis = 0;
const long interval = 1000;  // Interval to check the time (1 second)
int lastDeliveredMinute = -1;

unsigned long handDetectionStartTime = 0;  // Timer to track hand detection
bool isHandDetectionActive = false;  // Indicates if hand detection is ongoing

unsigned long lastAlertMillis = 0; // Timestamp for the last alert message
const long alertInterval = 60000;  // Interval for alert message (1 minute)

void setup() {
    Serial.begin(115200);
    
    // Configure the alert button pin as input with a pull-up resistor
    pinMode(ALERT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(HAND_SENSOR_PIN, INPUT); // Set the hand sensor pin as input
    pinMode(EMERGENCY_LED_PIN, OUTPUT); // Set the LED pin as output
    pinMode(BUZZER_PIN, OUTPUT); // Set the buzzer pin as output
    digitalWrite(BUZZER_PIN, LOW); // Ensure the buzzer is off initially

    // Setup WiFi, NTP, motor, and preferences
    setupWiFi();
    setupNTP();
    setupMotor();
    loadDeliveryTimesFromMemory();
    setupWebServer();
    setupOLED();
    client.setInsecure(); // Bypass certificate verification (for testing)
}

void loop() {
    // Check for emergency alert button press
    if (digitalRead(ALERT_BUTTON_PIN) == LOW) {
        // Turn on the emergency LED
        digitalWrite(EMERGENCY_LED_PIN, HIGH);
       
        if(a==0){
          sendTelegramMessage("Emergency situation detected!");
          a=1;
        }

        // Check if it's time to send an alert message
        unsigned long currentMillis = millis();
        if (currentMillis - lastAlertMillis >= alertInterval) {
            Serial.println("Emergency button pressed!");
            sendTelegramMessage("Emergency situation detected!");
            lastAlertMillis = currentMillis; // Update the last alert timestamp
        }
    } else {
        // Turn off the emergency LED
        digitalWrite(EMERGENCY_LED_PIN, LOW);
    }

    // Main loop function to check delivery times
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        timeClient.update();

        String currentTime = timeClient.getFormattedTime();
        int currentMinute = timeClient.getMinutes();

        // Compare current time with delivery times
        for (int i = 0; i < numDeliveryTimes; i++) {
            if (currentTime == medicationDeliveryTimes[i] && currentMinute != lastDeliveredMinute) {
                // Start hand detection process
                handDetectionStartTime = currentMillis;
                isHandDetectionActive = true;
                 digitalWrite(BUZZER_PIN, HIGH);  // Start the buzzer
                Serial.println("Medication time reached. Starting hand detection...");
                lastDeliveredMinute = currentMinute;
                break;  // Exit loop after starting hand detection
            }
        }
    }

    // Check for hand detection if it is active
    if (isHandDetectionActive) {
        if (isHandDetected()) {
            // If hand is detected, dispense medication
            digitalWrite(BUZZER_PIN, LOW);  // Stop the buzzer
            deliverMedication();
            isHandDetectionActive = false;  // Stop hand detection after dispensing
        } else {
            // Check if 2 minutes have passed since hand detection started
            if (millis() - handDetectionStartTime >= 120000) {  // 120000 ms = 2 minutes
                Serial.println("No hand detected for 2 minutes. Medication not dispensed.");
                digitalWrite(BUZZER_PIN, LOW);  // Stop the buzzer
                sendTelegramMessage("Medication not delivered");
                getMissedMedicationConsequences(medicationNames[lastDeliveredMinute]);
                isHandDetectionActive = false;  // Stop hand detection
            }
        }
    }
     // Display remaining time to next medication delivery on OLED
    displayRemainingTime();
}

// Function to check if a hand is detected using the IR sensor
bool isHandDetected() {
    return digitalRead(HAND_SENSOR_PIN) == LOW; // Assuming LOW indicates hand presence
}

// Wi-Fi setup with manual password and SSID
void setupWiFi() {
    const char* ssid = "";  // Replace with your Wi-Fi SSID
    const char* password = "";  // Replace with your Wi-Fi password

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void setupNTP() {
    timeClient.begin();
    timeClient.setTimeOffset(19800);  // Adjust according to Indian Standard Time (UTC +5:30)
}

void setupMotor() {
    myStepper.setSpeed(10);  // Set motor speed
}

void loadDeliveryTimesFromMemory() {
    preferences.begin("deliveryTimes", false);
    for (int i = 0; i < numDeliveryTimes; i++) {
        medicationDeliveryTimes[i] = preferences.getString(String(i).c_str(), "06:59:00");
        medicationNames[i] = preferences.getString(("m" + String(i)).c_str(), "Unknown Medicine");

    }
    preferences.end();
}

void setupWebServer() {
    server.on("/", HTTP_GET, handleRootRequest);
    server.on("/setTimes", HTTP_POST, handleSetTimesRequest);
    server.begin();
}

void handleRootRequest(AsyncWebServerRequest *request) {
    String html = generateHtmlPage();
    request->send(200, "text/html", html);
}

String generateHtmlPage() {
    String html = "<html>";
    html += "<head>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; padding: 20px; }";
    html += "h1, h2 { color: #4CAF50; text-align: center; }";
    html += "form { background-color: #fff; padding: 20px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); max-width: 400px; margin: 0 auto; }";
    html += "input[type='text'] { width: 100%; padding: 10px; margin: 10px 0; border-radius: 5px; border: 1px solid #ccc; }";
    html += "input[type='submit'] { width: 100%; background-color: #4CAF50; color: white; padding: 10px; border: none; border-radius: 5px; cursor: pointer; }";
    html += "input[type='submit']:hover { background-color: #45a049; }";
    html += "</style>";
    html += "</head>";

    html += "<body>";
    html += "<h1>Connective Creators</h1>";  // Add the top heading
    html += "<h2>MedPill</h2>";  // Add the form heading

    // Form starts
    html += "<form action='/setTimes' method='POST'>";
    
    for (int i = 0; i < numDeliveryTimes; i++) {
        html += "<label>Time " + String(i + 1) + ":</label>";
        html += "<input type='text' name='t" + String(i + 1) + "' value='" + medicationDeliveryTimes[i] + "'>";
        html += "<label>Medicine Name " + String(i + 1) + ":</label>";
        html += "<input type='text' name='m" + String(i + 1) + "' value='" + medicationNames[i] + "'><br>";
    }

    html += "<input type='submit' value='Save'>";
    html += "</form>";
    html += "</body>";
    html += "</html>";

    return html;
}


void handleSetTimesRequest(AsyncWebServerRequest *request) {
    if (validRequestParameters(request)) {
        for (int i = 0; i < numDeliveryTimes; i++) {
            medicationDeliveryTimes[i] = request->getParam("t" + String(i + 1), true)->value();
            medicationNames[i] = request->getParam("m" + String(i + 1), true)->value();
            setPreference(String(i).c_str(), medicationDeliveryTimes[i]);
            setPreference("m" + String(i), medicationNames[i]);
        }
        request->send(200, "text/plain", "Times updated successfully");
    } else {
        request->send(400, "text/plain", "Invalid parameters");
    }
}

bool validRequestParameters(AsyncWebServerRequest *request) {
    for (int i = 1; i <= numDeliveryTimes; i++) {
        if (!request->hasParam("t" + String(i), true)) {
            return false;
        }
    }
    return true;
}

void deliverMedication() {
    myStepper.step(stepsPerRevolution);
    delay(2000);  // Adjust this delay as needed
    myStepper.step(-stepsPerRevolution);
    sendTelegramMessage("Medication delivered");
}

void sendTelegramMessage(const String &message) {
    Serial.println("Bot Token: " + String(BOT_TOKEN)); // Print bot token
    Serial.println("Chat ID: " + String(CHAT_ID));    // Print chat ID
    UniversalTelegramBot bot(BOT_TOKEN, client);
    if (bot.sendSimpleMessage(CHAT_ID, message, "Markdown")) {
        Serial.println("Message sent successfully");
    } else {
        Serial.println("Failed to send message");
    }
}
void getMissedMedicationConsequences(String medicineName) {
    // Set up JSON request for ChatGPT API
    String message = "What are the consequences of missing " + medicineName + "?";
    
    client.connect("api.openai.com", 443);
    client.println("POST " CHATGPT_API_URL " HTTP/1.1");
    client.println("Host: api.openai.com");
    client.println("Authorization: Bearer " CHATGPT_API_KEY);
    client.println("Content-Type: application/json");
    client.println("Connection: close");

    String json = "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"system\", \"content\": \"You are an assistant.\"}, {\"role\": \"user\", \"content\": \"" + message + "\"}]}";
    
    client.print("Content-Length: ");
    client.println(json.length());
    client.println();
    client.println(json);

    // Read response from ChatGPT
    String response;
    bool isResponseStarted = false;
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            isResponseStarted = true; // Start reading response after headers
            continue;
        }
        
        if (isResponseStarted) {
            response += line; // Append the response lines
        }
    }

    // If the response is empty, handle that case
    if (response.length() == 0) {
        Serial.println("No response from ChatGPT.");
        sendTelegramMessage("No response received for " + medicineName + ".");
        return;
    }

    // Parse and clean the response if needed (depending on the structure)
    // For example, if the response is JSON, you might want to extract specific fields
    String formattedResponse = response; // You can format or parse it here if necessary
    Serial.println(formattedResponse);
    
    // Send the formatted response to Telegram
    sendTelegramMessage("Consequences of missing " + medicineName + ": " + formattedResponse);
}


// Setup OLED display
void setupOLED() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);  // Don't proceed, loop forever
    }
    display.clearDisplay();
    display.setTextSize(2);  // Set text size to 2 (larger)
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.display();
}

// Display remaining time to the next medication delivery on OLED
void displayRemainingTime() {
    String currentTime = timeClient.getFormattedTime();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();

    long currentTotalSeconds = currentHour * 3600 + currentMinute * 60 + currentSecond;
    
    long closestTotalSeconds = LONG_MAX; // Start with a large number
    String closestDeliveryTime;
    
    // Loop through delivery times to find the closest one
    for (int i = 0; i < numDeliveryTimes; i++) {
        String deliveryTime = medicationDeliveryTimes[i];
        int nextHour = deliveryTime.substring(0, 2).toInt();
        int nextMinute = deliveryTime.substring(3, 5).toInt();
        int nextSecond = deliveryTime.substring(6, 8).toInt();

        long nextTotalSeconds = nextHour * 3600 + nextMinute * 60 + nextSecond;

        // Adjust for the next day if needed
        if (nextTotalSeconds < currentTotalSeconds) {
            nextTotalSeconds += 24 * 3600;  // Add 24 hours in seconds
        }

        // Find the closest upcoming delivery time
        if (nextTotalSeconds < closestTotalSeconds) {
            closestTotalSeconds = nextTotalSeconds;
            closestDeliveryTime = deliveryTime;
        }
    }

    // Calculate remaining time until the closest delivery
    long remainingSeconds = closestTotalSeconds - currentTotalSeconds;

    // Check if it is time for the next dose
    if (remainingSeconds <= 1 && remainingSeconds >= 0) {
        // Display "Take Medicine" for 1 minute
        displayTakeMedicine();
    } else {
        // Convert remaining seconds to hours, minutes, and seconds
        int remainingHours = remainingSeconds / 3600;
        remainingSeconds %= 3600;
        int remainingMinutes = remainingSeconds / 60;
        int remainingSecs = remainingSeconds % 60;

        // Display the remaining time on the OLED screen
        display.clearDisplay();
        display.setTextSize(2);  // Set text size to 2 for larger display
        display.setCursor(0, 0);
        display.print("Next dose in:");
        
        // Adjust cursor position for better spacing
        display.setCursor(0, 40);  // Move cursor down further to avoid overlap
        display.print(remainingHours < 10 ? "0" : "");
        display.print(remainingHours);
        display.print(":");
        display.print(remainingMinutes < 10 ? "0" : "");
        display.print(remainingMinutes);
        display.print(":");
        display.print(remainingSecs < 10 ? "0" : "");
        display.print(remainingSecs);
        display.display();
    }
}

unsigned long previousMillis1 = 0;  // Store the last time the display was updated
const long interval1 = 1000;        // Interval for 1 second (1000 milliseconds)
int displayDuration = 0;           // Track how long the message has been displayed

void displayTakeMedicine() {
    unsigned long currentMillis = millis();  // Get the current time

    if (currentMillis - previousMillis1 >= interval1) {
        previousMillis1 = currentMillis;  // Save the last time the display was updated
        displayDuration++;               // Increment the display duration counter
        
        // Check if the message has been displayed for 60 seconds
        if (displayDuration <= 600) {
            display.clearDisplay();
            display.setTextSize(2);  // Set text size
            display.setCursor(0, 0); // Set cursor position
            display.print("Take");
            display.setCursor(0, 30); // Move cursor down
            display.print("Medicine");
            display.display();
        } else {
            // Clear the screen or do something else after 60 seconds
            display.clearDisplay();
            display.display();
        }
    }
}


String getPreference(const String &key, const String &defaultValue) {
    preferences.begin("deliveryTimes", false);
    String value = preferences.getString(key.c_str(), defaultValue);
    preferences.end();
    return value;
}

void setPreference(const String &key, const String &value) {
    preferences.begin("deliveryTimes", false);
    preferences.putString(key.c_str(), value);
    preferences.end();
}
