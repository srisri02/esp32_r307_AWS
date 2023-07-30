#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include<SoftwareSerial.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <esp_system.h>
#include <nvs_flash.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#define MQTT_MAX_PACKET_SIZE 4096 
int id;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
HardwareSerial serialPort(2); // use UART2
SoftwareSerial mySerial(16,17);
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define BUFFER_LEN 512
char msg[BUFFER_LEN];
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
uint8_t fingerTemplate[512];  
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&serialPort);
uint8_t p;
#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};
byte pin_rows[ROW_NUM]      = {19, 18, 5, 17}; 
byte pin_column[COLUMN_NUM] = {16, 4, 2, 15};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin("OnePlus Nord","0987654321");
 
  Serial.println("Connecting to Wi-Fi");
  lcd.clear();
  lcd.print("Conn to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
  lcd.clear();
  lcd.print("Conn to AWS IoT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
  lcd.clear();
  lcd.print("Connected to ");
  lcd.setCursor(0, 1);
  lcd.print("the cloud!");
  delay(3000);
  lcd.clear();
}
int generateUserID() {
  // Initialize the non-volatile memory (NVM) module
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    // Handle error
    return -1;
  }

  // Open the NVM storage partition
  nvs_handle nvm_handle;
  err = nvs_open("storage", NVS_READWRITE, &nvm_handle);
  if (err != ESP_OK) {
    // Handle error
    return -1;
  }

  // Generate a random number using the hardware random number generator
  int rand_num = esp_random() % 1000 + 1;

  // Check if the random number was previously generated
  int prev_num;
  err = nvs_get_i32(nvm_handle, "prev_num", &prev_num);
  if (err == ESP_OK && prev_num == rand_num) {
    // If the random number was previously generated, generate a new one
    rand_num = esp_random() % 1000 + 1;
  }

  // Store the current random number in the NVM partition
  err = nvs_set_i32(nvm_handle, "prev_num", rand_num);
  if (err != ESP_OK) {
    // Handle error
    return -1;
  }

  // Close the NVM partition
  nvs_close(nvm_handle);

  return rand_num;
}
int getFingerprintIDez()
 {
     uint8_t p = finger.getImage();
     if (p != FINGERPRINT_OK)
         return -1;

     p = finger.image2Tz();
     if (p != FINGERPRINT_OK)
         return -1;

     p = finger.fingerFastSearch();
     if (p != FINGERPRINT_OK)
         return -1;

      //found a match!
     lcd.clear();
     //Serial.print("Found ID #");
     //lcd.print("Found ID #");
     Serial.print(id);
     lcd.print(id);
     delay(2000);
     lcd.clear();
     Serial.print(" with confidence of ");
     
     Serial.println(finger.confidence);
     return id;
 }

uint8_t getFingerprintID()
 {
  lcd.clear();
  lcd.print("Place Finger");
  int p = -1;
  
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
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
  switch (p)
  {
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
  lcd.print("Remove Finger");
  delay(2000);
  lcd.clear();
 
      //OK converted!
     p = finger.fingerSearch();
     if (p == FINGERPRINT_OK)
     {
         Serial.println("Found a print match!");
         lcd.clear();
         lcd.print("Match Found!");
         
         delay(2000);
         lcd.clear();
     }
     else if (p == FINGERPRINT_PACKETRECIEVEERR)
     {
         Serial.println("Communication error");
         return p;
     }
     else if (p == FINGERPRINT_NOTFOUND)
     {
         Serial.println("Did not find a match");
         lcd.clear();
         lcd.print("No match");
         delay(2000);
         lcd.clear();
         return p;
     }
     else
     {
         Serial.println("Unknown error");
         return p;
     }

      //found a match!
     lcd.clear();
     //Serial.print("Found ID #");
     //lcd.print("Found ID #");
     Serial.print(id);
     //lcd.print(id);
     //lcd.print(finger.fingerID);
     delay(2000);
     lcd.clear();
     Serial.print(" with confidence of ");
     
     Serial.println(finger.confidence);
     return id;
 }

uint8_t readnumber(void)
{
  uint8_t num = 0;

  while (num == 0)
  {
    while (!Serial.available())
      ;
    num = Serial.parseInt();
  }
  return num;
}


int getFingerprintIDez();
void printHex(int num, int precision);
uint8_t downloadFingerprintTemplate(uint16_t id);

uint8_t getFingerprintEnroll();

void setup()
{ 
 
  Serial.begin(9600);
  while (!Serial)
    ; 
  delay(100);
  lcd.init();                      // initialize the LCD display
  lcd.backlight(); 
  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
    {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);

  connectAWS();
  

}
 void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);

  Serial.print("payload: ");
  for (int i=0; i<length; i++) {
    Serial.print(char(payload[i]));
  }
  Serial.println();

  StaticJsonDocument<1536> doc;
  deserializeJson(doc, payload);
  String message = doc["message"];
  Serial.println(message);
}

void publishMessage(byte* fingerTemplate, size_t fingerTemplateSize) {
  // Create a JSON document with a buffer size of 2048 bytes
  StaticJsonDocument<2048> doc;

  // Create a nested object for the fingerprint
 

  // Convert the byte array to a hex string and add it to the nested object
  char hexString[1025]; // 2 hex digits per byte, so a 512-byte array needs 1024 characters in the hex string, plus one extra byte for the null terminator
  for (int i = 0; i < sizeof(fingerTemplate); i++) {
    sprintf(hexString + i * 2, "%02X", fingerTemplate[i]);
  }
  hexString[1024] = '\0'; // Add a null terminator to the end of the hex string
  
   Serial.print(hexString);
  // Add the CID to the main JSON object
  doc["cid"] = id;
  doc["fingerprint"] =hexString;
  // Serialize the JSON document
  size_t jsonBufferSize = measureJson(doc) + 1;
  char* jsonBuffer = new char[jsonBufferSize];
  serializeJson(doc, jsonBuffer, jsonBufferSize);

  // Publish the JSON document to the MQTT topic
  if (!client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
    Serial.println("Failed to publish message");
  } else {
    Serial.println("Message published");
  
  }

  delete[] jsonBuffer;
}



 uint8_t downloadFingerprintTemplate(uint16_t id)
 {
     Serial.println("------------------------------------");
     Serial.print("Attempting to load #");
     Serial.println(id);
     uint8_t p = finger.loadModel(id);
     switch (p)
     {
     case FINGERPRINT_OK:
         Serial.print("Template ");
         Serial.print(id);
         Serial.println(" loaded");
         break;
     case FINGERPRINT_PACKETRECIEVEERR:
         Serial.println("Communication error");
         return p;
     default:
         Serial.print("Unknown error ");
         Serial.println(p);
         return p;
     }


    Serial.print("Attempting to get #");
     Serial.println(id);
     p = finger.getModel();
    switch (p)
     {
    case FINGERPRINT_OK:
         Serial.print("Template ");
         Serial.print(id);
         Serial.println(" transferring:");
         break;
     default:
         Serial.print("Unknown error ");
         Serial.println(p);
        return p;
     }

    
     uint8_t bytesReceived[534];  
     memset(bytesReceived, 0xff, 534);

     uint32_t starttime = millis();
     int i = 0;
     while (i < 534 && (millis() - starttime) < 20000)
     {
         if (serialPort.available())
         {
             bytesReceived[i++] = serialPort.read();
         }
     }
     Serial.print(i);
     Serial.println(" bytes read.");
     Serial.println("Decoding packet...");
     memset(fingerTemplate, 0xff, 512);

     int uindx = 9, index = 0;
     memcpy(fingerTemplate + index, bytesReceived + uindx, 256); // first 256 bytes
     uindx += 256;                                                //skip data
     uindx += 2;                                                  //skip checksum
     uindx += 9;                                                  //skip next header
     index += 256;                                                //advance pointer
     memcpy(fingerTemplate + index, bytesReceived + uindx, 256);  //second 256 bytes

     for (int i = 0; i < 512; ++i)
     {
          Serial.print("0x");
         printHex(fingerTemplate[i], 2);
          Serial.print(", ");
     }
     Serial.println("\ndone.");

        for (int i = 0; i < 512; ++i)
        {Serial.print(fingerTemplate[i]);
         
        }
        publishMessage(fingerTemplate,512);
 }

 void printHex(int num, int precision)
 {
     char tmp[16];
     char format[128];

     sprintf(format, "%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
 }


void loop() // run over and over again
{
  lcd.setCursor(0, 0);             
  lcd.print("1.Enroll"); 
  lcd.setCursor(0, 1);             
  lcd.print("2.Authenticate"); 

char key = keypad.getKey();
if (key) {
     lcd.clear();
    lcd.print(key);


if(key=='1')
{
  Serial.println("Ready to enroll a fingerprint!");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready to enroll");
   id= generateUserID();
 

  while (!getFingerprintEnroll())
    ;
}
else if(key=='2')
{
getFingerprintID();
     delay(50); 
}
} 
}

uint8_t getFingerprintEnroll()
{
  
  int p = -1;
   lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place Finger");
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
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
lcd.clear();
lcd.print("Remove Finger");
  // OK success!

  p = finger.image2Tz(1);
  switch (p)
  {
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
  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger ");
  lcd.clear();
  lcd.print("Place same finger again");
  lcd.setCursor(0,1);
  lcd.print("again");
  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();
    switch (p)
    {
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
  switch (p)
  {
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
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Prints matched!");
    lcd.print("Prints matched!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_ENROLLMISMATCH)
  {
    Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.print("No Match.");
    lcd.setCursor(0, 1);
    lcd.print("Try Again");
    delay(3000);
    lcd.clear();
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  lcd.clear();
  lcd.print("Your ID is :");
  lcd.print(id);
  Serial.println(id);
  lcd.clear();
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Stored!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_BADLOCATION)
  {
    Serial.println("Could not store in that location");
    return p;
  }
  else if (p == FINGERPRINT_FLASHERR)
  {
    Serial.println("Error writing to flash");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }
   int finger =1 ;
   downloadFingerprintTemplate(finger);

   return true;
}

