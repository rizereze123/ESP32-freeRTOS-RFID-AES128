#include <Arduino.h>
#include <Crypto.h>
#include <AES.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
 

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883


// You can replace this one
#define AIO_USERNAME    "..."
#define AIO_KEY         "..."

#define DOORLOCK_FEED   "..../feeds/..."
#define RESPONSE_FEED   "..../feeds/..."

#define RST_PIN .. 
#define SS_PIN ..

#define relayDOORLOCK ..
#define BUZZER ..
#define blueLED ..
#define redLED ..

#define BUTTON_CLOSE_DOOR1 ..
#define BUTTON_ENROLL ..

#define ROW_NUM     .. 
#define COLUMN_NUM  .. 



const char* ssid = "...";
const char* password = "...";

String device_token = ".....";

// 0x31 to ASCII is 1
// 0x32 to ASCII is 2, you can replace this key and iv AES-128
byte key[] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31}; 
byte iv[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32};

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte pin_rows[ROW_NUM] = {.., .., .., ..}; 
byte pin_column[COLUMN_NUM] = {.., .., ..}; 

WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish doorlock = Adafruit_MQTT_Publish(&mqtt, DOORLOCK_FEED, 0);
Adafruit_MQTT_Subscribe confirmation_sub = Adafruit_MQTT_Subscribe(&mqtt, RESPONSE_FEED);

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522 mfrc522(SS_PIN, RST_PIN); 
AES aes;

uint8_t id;
String inputId = "";

bool enrollmentMode = false;

enum StateEnrollment {
  STATE_ENROLL_RFID,
  STATE_ENROLL_FINGERPRINT
};
StateEnrollment currentStateEnrollment = STATE_ENROLL_RFID;

Adafruit_MQTT_Subscribe *subscription;

SemaphoreHandle_t xBotMutex;

void MQTT_connect();
void responseSubscribe();

void doorlockOpen(bool);

bool StatePintuTerbuka = false;
bool StatePintuTerbukaByButton = false;
bool StateLCDTapOUT = false;

void taskMQTT_connect(void *pvParameters);
void taskresponseSubscribe(void *pvParameters);

void setup() {
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  SPI.begin();               
  mfrc522.PCD_Init();       
  lcd.init();                   
  lcd.backlight();

  while (!Serial); 
  vTaskDelay(100 / portTICK_PERIOD_MS);

  pinMode(BUTTON_CLOSE_DOOR1, INPUT);
  pinMode(BUTTON_ENROLL, INPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(relayDOORLOCK, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  finger.begin(57600);
  vTaskDelay(5 / portTICK_PERIOD_MS);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { vTaskDelay(1 / portTICK_PERIOD_MS); }
  }

  mqtt.subscribe(&confirmation_sub);

  digitalWrite(relayDOORLOCK, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(BUZZER, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Kartu Anda!");
  lcd.setCursor(0, 1);
  lcd.print("Atau/ Jari Anda!");

  xBotMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    taskMQTT_connect,
    "taskMQTT_connect",
    4096,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    taskresponseSubscribe,
    "taskresponseSubscribe",
    8192,
    NULL,
    1,
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    taskEnrollment,
    "taskEnrollment",
    8192,
    NULL,
    1,
    NULL,
    1
  );
}



void loop() {
// Nothing, cuz using FreeRTOS
}



void taskEnrollment(void *pvParameters) {
  for (;;)
  {
    if (enrollmentMode) {
      modeEnrollment();
    } else {
      modeLogHistory();
    }

    if (digitalRead(BUTTON_ENROLL) == HIGH) {
      enrollmentMode = true;
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }
  }
  
}

void modeLogHistory() {
  if (xSemaphoreTake(xBotMutex = xSemaphoreCreateMutex(), portMAX_DELAY) == pdTRUE){
    waitRFIDScan();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  
  getFingerprintID();

  if (digitalRead(BUTTON_CLOSE_DOOR1) == HIGH) {
    if(StatePintuTerbuka == true){
      // Ketika buzzermenyala
      vTaskDelay(200 / portTICK_PERIOD_MS);
      resetESP32();
    } else {
      // Ketika state pintu idle
      StateLCDTapOUT = true;
      doorlockOpen(true);
    }
  }     
}

void modeEnrollment() {
  switch (currentStateEnrollment) {
    case STATE_ENROLL_RFID:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode Enrollment");
      lcd.setCursor(0, 1);
      lcd.print("Scan Kartu Anda!");
      if (xSemaphoreTake(xBotMutex = xSemaphoreCreateMutex(), portMAX_DELAY) == pdTRUE){
        waitRFIDScan();
        vTaskDelay(200 / portTICK_PERIOD_MS);
      }
      break;
    case STATE_ENROLL_FINGERPRINT:
    if (xSemaphoreTake(xBotMutex = xSemaphoreCreateMutex(), portMAX_DELAY) == pdTRUE){
      enrollFinger();
      }
      break;
    default:
      break;
  }    
}



void responseSubscribe(){
  if (xSemaphoreTake(xBotMutex = xSemaphoreCreateMutex(), portMAX_DELAY) == pdTRUE) {
    while ((subscription = mqtt.readSubscription(1000))) {
      if (subscription == &confirmation_sub) {
        Serial.println((char *)confirmation_sub.lastread);
        if (strcmp((char *)confirmation_sub.lastread, "Pintu Terbuka-IN") == 0) {
          doorlockOpen(false);
        }
        else if (strcmp((char *)confirmation_sub.lastread, "Pintu TerbukaOUT") == 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Tap Out Waktu");
          lcd.setCursor(0, 1);
          lcd.print("Keluar Berhasil");
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Jngan Lupa Tutup");
          lcd.setCursor(0, 1);
          lcd.print("Pintu Kembali");
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          resetESP32();
        } 
        else if (strcmp((char *)confirmation_sub.lastread, "New Card Added!!") == 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Success, Kartu");
          lcd.setCursor(0, 1);
          lcd.print("Telah terdaftar");
          digitalWrite(blueLED, HIGH);
          digitalWrite(BUZZER, HIGH);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          digitalWrite(blueLED, LOW);
          digitalWrite(BUZZER, LOW);
          currentStateEnrollment = STATE_ENROLL_FINGERPRINT;
        }
        else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("[Warning Error!]");
          lcd.setCursor(0, 1);
          lcd.print((char *)confirmation_sub.lastread);
          digitalWrite(BUZZER, HIGH);
          digitalWrite(redLED, HIGH);
          vTaskDelay(6000 / portTICK_PERIOD_MS);
          digitalWrite(BUZZER, LOW);
          digitalWrite(redLED, LOW);
          resetESP32();
        }
      }
    }
    xSemaphoreGive(xBotMutex = xSemaphoreCreateMutex());
  }
}

void taskresponseSubscribe(void *pvParameters) {
  for (;;) {
    responseSubscribe();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


String encrypt(String data) {
  byte plain[data.length()+1];
  data.getBytes(plain, data.length()+1);
  int plainLength = data.length()+1;
  int paddedLength = plainLength + N_BLOCK - plainLength % N_BLOCK;
  byte cipher[paddedLength];

  unsigned long ms = micros();
  aes.do_aes_encrypt(plain, plainLength, cipher, key, 128, iv);
  Serial.print("Encryption took: ");
  Serial.println(micros() - ms);

  // For HEX Decode
  // String encryptedMessage = "";
  // for (int i = 0; i < paddedLength; i++) {
  //   if (cipher[i] < 0x10) {
  //     encryptedMessage += "0";
  //   }
  //   encryptedMessage += String(cipher[i], HEX);
  // }

  // For Binary Decode
  String encryptedMessage = "";
  for (int i = 0; i < paddedLength; i++) {
    String binary = String(cipher[i], BIN);
    while (binary.length() < 8) {
      binary = "0" + binary;
    }
    encryptedMessage += binary;
  }
  return encryptedMessage;
}



void waitRFIDScan() {
    if (mfrc522.PICC_IsNewCardPresent()) {
      if (mfrc522.PICC_ReadCardSerial()) {
        String cardID = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          cardID += mfrc522.uid.uidByte[i];
        }
        String encryptedMessage = encrypt(cardID);
        String payload = "{\"card_uid\":\"" + encryptedMessage + "\", \"device_token\":\"" + device_token + "\"}"; // Data dummy
        doorlock.publish(payload.c_str());
        Serial.println("Publishing to MQTT Broker Adafruit IO...");
        mfrc522.PICC_HaltA();
      }
    }
}



uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!
  p = finger.image2Tz();
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
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[Warning Error!]");
    lcd.setCursor(0, 1);
    lcd.print("Jari Tidak Valid");
    digitalWrite(BUZZER, HIGH);
    digitalWrite(redLED, HIGH);
    vTaskDelay(6000 / portTICK_PERIOD_MS);
    digitalWrite(BUZZER, LOW);
    digitalWrite(redLED, LOW);
    resetESP32();
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  if(StatePintuTerbukaByButton == true)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tap Out Waktu");
    lcd.setCursor(0, 1);
    lcd.print("Keluar Berhasil");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Jngan Lupa Tutup");
    lcd.setCursor(0, 1);
    lcd.print("Pintu Kembali");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    resetESP32();
  } else {
    doorlockOpen(false);
  }

  return finger.fingerID;
}



void enrollFinger() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please Enter ID:");
  
  while(true) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      if (key == '#') {
        id = inputId.toInt();
        if (id == 0 || id > 127) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("[Warning Error!]");
          lcd.setCursor(0, 1);
          lcd.print("ID Tidak Valid!!");
          Serial.println("Invalid ID! Try again");
          digitalWrite(BUZZER, HIGH);
          digitalWrite(redLED, HIGH);
          vTaskDelay(2000 / portTICK_PERIOD_MS);
          digitalWrite(BUZZER, LOW);
          digitalWrite(redLED, LOW);
          lcd.clear();
          clearInputId();
        } else {
          Serial.println("Enrolling ID#" + String(id));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("[Success]");
          lcd.setCursor(0, 1);
          lcd.print("Enrolling ID" + String(id));
          digitalWrite(BUZZER, HIGH);
          digitalWrite(blueLED, HIGH);
          vTaskDelay(2000 / portTICK_PERIOD_MS);
          digitalWrite(BUZZER, LOW);
          digitalWrite(blueLED, LOW);
          lcd.clear();
          while (!getFingerprintEnroll());
          clearInputId();
          break; 
        }
      } else if (key == '*') {
        if (inputId.length() > 0) {
          //inputId.remove(inputId.length() - 1);
          clearInputId();
          lcd.clear();
        }
      } else {
        inputId += key;
      }
      lcd.setCursor(0, 0);
      lcd.print("Please Enter ID:");
      lcd.setCursor(0, 1);
      lcd.print(inputId); 
    }
  }
}


void clearInputId() {
  inputId = "";
}


uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fingers Waiting");
  lcd.setCursor(0, 1);
  lcd.print("To Enroll As #");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
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

  // OK success!
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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please...");
  lcd.setCursor(0, 1);
  lcd.print("Lepas Jari Anda!");
  digitalWrite(BUZZER, HIGH);
  digitalWrite(blueLED, HIGH);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  digitalWrite(BUZZER, LOW);
  digitalWrite(blueLED, HIGH);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ID#" + String(id) + " Please Scan");
  lcd.setCursor(0, 1);
  lcd.print("SameFinger Again");
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

  // OK success!
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

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[Warning Error!]");
    lcd.setCursor(0, 1);
    lcd.print("Jari Tidak Sama!");
    digitalWrite(BUZZER, HIGH);
    digitalWrite(redLED, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    digitalWrite(BUZZER, LOW);
    digitalWrite(redLED, LOW);
    lcd.clear();
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[Success]");
    lcd.setCursor(0, 1);
    lcd.print("Pendaftaran Jari");
    digitalWrite(BUZZER, HIGH);
    digitalWrite(blueLED, HIGH);
    vTaskDelay(6000 / portTICK_PERIOD_MS);
    digitalWrite(BUZZER, LOW);
    digitalWrite(blueLED, LOW);
    lcd.clear();
    resetESP32();
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



void doorlockOpen(bool openByButton = false) {
  if(StateLCDTapOUT == true)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Silahkan Untuk");
    lcd.setCursor(0, 1);
    lcd.print("Tap OUT"); 
    Serial.println("Silahkan Masuk");
    }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[Final Success]");
    lcd.setCursor(0, 1);
    lcd.print("Silahkan Masuk!"); 
    Serial.println("Silahkan Masuk");
  }
  digitalWrite(relayDOORLOCK, HIGH);
  digitalWrite(blueLED, HIGH);
  digitalWrite(BUZZER, HIGH);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  digitalWrite(blueLED, LOW);
  digitalWrite(BUZZER, LOW);  
  StatePintuTerbuka =  true;
  StatePintuTerbukaByButton = openByButton;
}


void resetESP32() {
  ESP.restart();
}


void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { 
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       vTaskDelay(5000 / portTICK_PERIOD_MS);  
       retries--;
       if (retries == 0) {
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void taskMQTT_connect(void *pvParameters) {
  for (;;) {
    MQTT_connect();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}