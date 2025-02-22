#include <Arduino.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

// Use Hardware Serial on ESP32
HardwareSerial mySerial(2); // Use UART2 (RX2, TX2)

// Connect sensor's RX to ESP32 TX2 (GPIO17)
// Connect sensor's TX to ESP32 RX2 (GPIO16)

bool isIdMode = true; // true for identification false for enrollment
bool isEnrolled = false;
bool modePrinted = false;
bool gotId = false;
int idToSave = 0;
byte readAttempt = 20;

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


void setup() {
    Serial.begin(115200);
    while (!Serial);  // Ensure serial is ready
    delay(100);
    setupFingerprintSensor();
    Serial.println("\n\nAdafruit Fingerprint Sensor Enrollment");
}


void loop() {
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
}
