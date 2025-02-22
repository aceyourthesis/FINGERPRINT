#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <time.h>
#include <ESP_Google_Sheet_Client.h>

// Use Hardware Serial on ESP32
HardwareSerial mySerial(2); // Use UART2 (RX2, TX2)
FirebaseJson response;
FirebaseJson valueRange;

// Connect sensor's RX to ESP32 TX2 (GPIO17)
// Connect sensor's TX to ESP32 RX2 (GPIO16)

// Replace with your WiFi credentials
const char* ssid = "qwertyuiop";     // Change to your WiFi network name  SmartBro_EEAA7
const char* password = "12345678"; // Change to your WiFi password  smartbro
//const char* ssid = "SmartBro_EEAA7";      // Change to your WiFi network name  
//const char* password = "smartbro";        // Change to your WiFi password  

// Google Project ID
#define PROJECT_ID "fingerprintesp32-451714"

// Service Account's client email
#define CLIENT_EMAIL "fingerprintesp32@fingerprintesp32-451714.iam.gserviceaccount.com"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDCCi2oEfTFdZCs\nCa7RetCJPCfAf0jKPGcP9d8S8ci+Xinl8GZA1GybwWG5YOOX11vxNELahKdfRM8T\nONHVF/Yb1iF0Na3VFqoaadXwFBXiQ4UMUIPJSROoXtPimimN/jQW4oOfdUrDjQfh\nSYgcsWgLOndBoCFW4dtSdvgJjKxIjfajJNNPSWvG3aCUqXj3iL9msR/FDSA/4WyC\nMZHfFgSQeBC45BLkmt0ItiAHA6FAybERcVxROfOMP8rloAJeTKicqZdmvQ0SIz4z\nyx83rZOclLKO3v+510LZ3qqW5FMSR9EuFQpMLPVqCQoDj1DLD8cOyFbPG7qyV7Tj\nMMqn2yDbAgMBAAECggEAUKzKrsZIbpCixeSAurfFXEmWfYJuUScr8obXaJj+Taf4\n45E/8lRfk6Kjbt/JwKfZoj+ITDI7JHEHZ4aIN4Du4kP1RMvMqnvx3WmDeX3gW2KX\nPrd7E4U+lxJWJjDSdibqoVksQHAT1ZodXJ2l6cgY6hI04yQguG7PMUoldlL2jnM+\nckSsi19j8lH/o1sW9Ylz0OYMRUNETgEnIpoWH9k6/yOyY+pQK9YEopwMeUsqABlf\nMMlBbUPgM7doDfKmrpM8hAGa0+1AL8NvV6pQvuVh6G0PExsrJNwBAIRj6Nx9EHXg\n+lL2CcpxT+xKKHts6yewtUDoF7+ACcvBkI5UPxDlAQKBgQDgUqe8+YsCBXeh5AJO\nTICWW7f2a9pxUjqV60yjyRkh3hZdeAxuSnRtGUqYdleiZAEsmCJHyHzUqDM/m1EG\n9sp9amTpHib4dg0KjwUNnhejYMFmeX5qt5cVg9HUk8+u1nGT5AVWK77folF3L9nF\nlzKm5AFGbMkdg6sctQlhPiE02QKBgQDdcMdboiVkBvEs8HCApZ3LS16Be1NdJARw\n75LFjkCUTMaNc2HAWqfcYymL39H/Dqin2soGCrfiFsljtHx8hS3iOL/04iPyXTe7\nbmqX5vHHN1NNSNeBA/zG/7bPRfh5uJPIfIho2Gu51d62jxwDZi281RQN5cix1uzT\ncTLxB13i0wKBgCaHC7gxQ+a8+acceijois1IQ3hXXdVIwjctiqQ2zdI7SqwTCk+H\nCigSx/UXnkWN14XJO1oI3SfJG8Q48Nm5WV3hACPpwde6rtK5bazBEl5FJ/Jpu4So\ns5JGaauRF4J34ln8N85tBQNz5XoLBeQh69kuH+nwlTo0excbINlKynlBAoGBAN1G\nZO/cByD9DuKpIMJsvFD472bTpWndI9L2kuN2UnHehkmykmt0K2i+bjWnYmBMdZvF\ntTp0W2tA+dz37a1AJ6l2q9o4X1NAcwCIRVulGtTHJCaGM1JoeZ8gx7w3XyPCnWop\nhuW93WfFntcuCgAbini3M2Lvyqxq6BwhcZYIlnihAoGBAI90PXHt+h9j4eHrFtkq\nMU+KVacMPp3w1NBqBg1S87ItmHAhXrMMvq/uZSCPtdgsfcMQfdxO4lHX1GhHNiMQ\n5BKgKQLz8byYT+YdPg8looblX0jYkhu27SlpokKE0yrHv/d19zp7veMmTGxCo5HO\nGhbjQCaeqMk63aWBShvZHW6b\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "1c3bMTs9d6HbpFJvukee3kWkfdJCMyR3bwBKBG4jW2UA";

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Variable to save current epoch time
unsigned long epochTime; 




bool isIdMode = false; // true for identification false for enrollment
bool isEnrolled = false;
bool modePrinted = false;
bool gotId = false;
int idToSave = 0;
byte readAttempt = 20;
bool isConnectedToWifi = false;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

uint8_t enrollFingerprint(int _idToEnroll) {
    int p = -1;
    Serial.print("Waiting for valid finger to enroll as #");
    Serial.println(_idToEnroll);

    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                Serial.print(".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                Serial.println("Imaging error");
                break;
            default:
                Serial.println("Unknown error");
                break;
        }
    }

    p = finger.image2Tz(1);
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("Image too messy");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return p;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return p;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return p;
        default:
            Serial.println("Unknown error");
            return p;
    }

    Serial.println("Remove finger");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
        p = finger.getImage();
    }

    Serial.print("ID "); Serial.println(_idToEnroll);
    p = -1;
    Serial.println("Place the same finger again");
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                Serial.print(".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                Serial.println("Imaging error");
                break;
            default:
                Serial.println("Unknown error");
                break;
        }
    }

    p = finger.image2Tz(2);
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("Image too messy");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return p;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return p;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return p;
        default:
            Serial.println("Unknown error");
            return p;
    }

    Serial.print("Creating model for #");
    Serial.println(_idToEnroll);
    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        Serial.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
        Serial.println("Fingerprints did not match");
        return p;
    } else {
        Serial.println("Unknown error");
        return p;
    }

    Serial.print("Storing ID ");
    Serial.println(_idToEnroll);
    p = finger.storeModel(_idToEnroll);
    if (p == FINGERPRINT_OK) {
        Serial.println("Stored!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
        Serial.println("Could not store in that location");
        return p;
    } else if (p == FINGERPRINT_FLASHERR) {
        Serial.println("Error writing to flash");
        return p;
    } else {
        Serial.println("Unknown error");
        return p;
    }

    return true;
}


// Token Callback function
void tokenStatusCallback(TokenInfo info){
  if (info.status == token_status_error){
      GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  } else {
      GSheet.printf("Token status: %s\n", GSheet.getTokenStatus(info).c_str());
  }
}

// Function to read a fingerprint and check if it matches an enrolled one
int getFingerprintID() {
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
        return -1; // No finger detected
    } else if (p != FINGERPRINT_OK) {
        return -2; // Error
    }

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        return -2; // Error
    }

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
        Serial.print("Fingerprint matched with ID: ");
        Serial.println(finger.fingerID);
        
        return finger.fingerID;
    } else {
        Serial.println("Fingerprint not recognized");
        return 0; // No match found
    }
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
      while (!Serial.available());
      num = Serial.parseInt();
  }
  return num;
}


void setupFingerprintSensor(){
      // Initialize hardware serial for the fingerprint sensor
      mySerial.begin(57600, SERIAL_8N1, 16, 17); // RX=16, TX=17

      if (finger.verifyPassword()) {
          Serial.println("Found fingerprint sensor!");
      } else {
          Serial.println("Did not find fingerprint sensor :(");
          while (1) { delay(1); }
      }
  
      Serial.println(F("Reading sensor parameters"));
      finger.getParameters();
      Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
      Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
      Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
      Serial.print(F("Security level: ")); Serial.println(finger.security_level);
      Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
      Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
      Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
}

void getMode (){
  if (Serial.available() > 0){

    char c = (char)Serial.read();  // Read one character and cast it to char
    Serial.println(c);

    if (c == 'E'){  // Compare with single quotes for char
      isIdMode = false;
      isEnrolled = false;
      modePrinted = false;
    } else if (c == 'I'){
      isIdMode = true;
      modePrinted = false;
    } else {
      Serial.println("Invalid command. Type E to enroll, type I to identify");
    }
  }
}


void enroll(){
  if (!isEnrolled){
    Serial.println("Give me a number:");

    while (Serial.available() == 0) {
      // Wait for user input
    }
    
    int _IDnum = Serial.parseInt(); // Read the number as an integer

    Serial.print("Enrolling ID: ");
    Serial.println(_IDnum);

    enrollFingerprint(_IDnum);  // Pass the ID number to the function
    isEnrolled = true;
  }
}

byte getFingerId(int _attempt) {
  int id;

  for (int i = 0; i < _attempt; i++) { // Fix condition here
    Serial.printf("ID attempt: %d\n", i);
    id = getFingerprintID();
    if (id != 0) {
      return id; // Return valid ID immediately
    }
  }
  return 0; // Return 0 if no valid fingerprint found
}

void setupWiFi() {
  Serial.print("Connecting to WiFi...");
  
  WiFi.begin(ssid, password); // Start connecting to WiFi
  
  while (WiFi.status() != WL_CONNECTED) { 
      delay(500);
      Serial.print(".");
  }
  isConnectedToWifi = true;
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Print the assigned IP address
}

void checkWifi(){
  if (WiFi.status() == WL_CONNECTED) {
    if(!isConnectedToWifi){
      Serial.printf("\n\n----------------\n\nWiFi Connected\n\n----------------\n");
    }
    isConnectedToWifi = true;
  } else{
    if(isConnectedToWifi) {
      Serial.printf("\n\n----------------\n\nWiFi Detatched\n\n----------------\n");
    }
    isConnectedToWifi = false;
  }
}

void saveDataToSheets(){
      bool ready = GSheet.ready();
      Serial.println("\nAppend spreadsheet values...");
      Serial.println("----------------------------");

      unsigned long epochTime = 1708451234;  // Example timestamp (Unix time)
      float temp = 26.5;                     // Temperature in Celsius
      float hum = 58.3;                       // Humidity in percentage
      float pres = 1013.25;                   // Pressure in hPa

      valueRange.add("majorDimension", "COLUMNS");
      valueRange.set("values/[0]/[0]", epochTime);
      valueRange.set("values/[1]/[0]", temp);
      valueRange.set("values/[2]/[0]", hum);
      valueRange.set("values/[3]/[0]", pres);

      // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
      // Append values to the spreadsheet
      bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
      if (success){
          response.toString(Serial, true);
          valueRange.clear();
      }
      else{
          Serial.println(GSheet.errorReason());
      }
      Serial.println();
      Serial.println(ESP.getFreeHeap());
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);  // Ensure serial is ready
    setupWiFi();           // Call WiFi setup function
    delay(100);
    setupFingerprintSensor();
    Serial.println("\n\nAdafruit Fingerprint Sensor Enrollment");
    //Configure time
    configTime(0, 0, ntpServer);        

    GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

    
    GSheet.setTokenCallback(tokenStatusCallback);                             // Set the callback for Google API access token generation status (for debug only)
    GSheet.setPrerefreshSeconds(10 * 60);                                     // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);                      // Begin the access token generation for Google API authentication

}


void loop() {
  checkWifi(); //checks wifi status and prints it 

  bool ready = GSheet.ready();

  getMode();
  if (!modePrinted){
    Serial.printf("Mode: %s\n", isIdMode ? "Identify" : "Enroll");
    modePrinted = true;
  }

  if (!isIdMode){
    enroll();
  } else {
    if(!gotId){        
      getFingerId(readAttempt);
    }
  }
  saveDataToSheets();
  delay(5000);
}
