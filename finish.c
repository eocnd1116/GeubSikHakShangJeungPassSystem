// LIBRARY MANAG //
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h> //외부
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h> //외부 <- 이제 추가적으로 zip 하고 업데이트
#include <swRTC.h> //외부 

// ============================================================ //

// Bluetooth Reset //
#define BT_RXD 0
#define BT_TXD 1
SoftwareSerial bluetooth(BT_RXD, BT_TXD);

// Speaker Reset //
#define SP_PIN 2
float spTone[] = {32.7032,34.6478,36.7081,38.8909,41.2034,41.2034,43.6535,46.2493,48.9994,51.9130,55.0000,58.2705,61.7354};
String spMusic = "12345";

// Display Reset //
LiquidCrystal_I2C lcd(0x27,16,2);
byte ct_star[8] = {B00000,B00100,B00100,B11111,B01110,B11011,B10001,B00000};
byte ct_human[8] = {B01110,B11111,B11111,B01110,B00000,B01110,B11111,B11111};
byte ct_time[8] = {B00000,B00100,B01110,B10001,B10111,B10001,B01110,B00000};
int dpType = -1;
bool dpChange = true;
int cooltime = 0;

// RFID Reset //
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN); 
byte nuidPICC[4];
byte nuidPICC_back[4];
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i]);
  }
}

// Time(RTC) Reset //
swRTC rtc;
unsigned long previousMillis = 0;
const long interval = 1000;

// ============================================================ //

// MAIN RESET //
void setup() {
  // Default Setting //
  Serial.begin(9600);
  bluetooth.begin(9600);
  SPI.begin(); 

  // Display Setting //
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.createChar(0, ct_star);
  lcd.createChar(1, ct_human);
  lcd.createChar(2, ct_time);
  for ( int i=0; i<4; i++) {
    byte ct_timer [] = {
      B00000,
      B01110,
      (i==0?B10101:B10001),
      (i==1?B10111:(i==3?B11101:B10101)),
      (i==2?B10101:B10001),
      B01110,
      B00000,
      B00000
    };
    lcd.createChar(3+i, ct_timer);
  }

  // RFID Setting //
  rfid.PCD_Init(); 
  Serial.println(F("RFID scan start"));

  // RTC Setting //
  rtc.startRTC();
}

// ============================================================ //

// MAIN  //
void loop() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    if(cooltime>0){
      cooltime -= 1;
      lcd.setCursor(13, 1);
      lcd.write(3+cooltime%4);
      lcd.print(String(cooltime));
      if(cooltime==0){
        dpType = -1;
        dpChange = true;
      }
    }
  }

  // Bluetooth //
  if(bluetooth.available()){
    Serial.write(bluetooth.read());
  }
  if(Serial.available()) {
    bluetooth.write(Serial.read());
  }

  // Speaker //
  int spOctav = 3;
  int nextShap = 0;
  for(int i=0; i<spMusic.length(); i++){
    char temp = spMusic.charAt(i);
    if(temp=='?'){spOctav -= 1;}
    else if(temp=='!'){spOctav += 1;}
    else if(temp=='0'){nextShap = 0;}
    else if(temp=='/'){delay(100);}
    else{
      int temp2 = temp - '0';
      float tones = spTone[temp2*2-2+nextShap];
      for(int j=0; j<spOctav; j++){
        tones *= 2;
      }
      //tone(SP_PIN, tones);
      Serial.println(temp2*2-2);
      nextShap = 1;
    }
    delay(500);
  }
  noTone(SP_PIN);

  // Display //
  if(dpChange){
    lcd.clear();
    lcd.init();
    if(dpType == -1){
      lcd.setCursor(2, 0);
      lcd.write(byte(0));
      lcd.print(" Welcome! ");
      lcd.write(byte(0));
      lcd.setCursor(0, 1);
      lcd.print("NFC Scaned:Start");
    }
    dpChange = false;
  }

  // RFID //
  if ( ! rfid.PICC_IsNewCardPresent() || ! rfid.PICC_ReadCardSerial())
    return;

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
      rfid.uid.uidByte[1] != nuidPICC[1] || 
      rfid.uid.uidByte[2] != nuidPICC[2] || 
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("card detected."));
    
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    lcd.init();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.write(1);
    for (byte i = 0; i < 4; i++) {
      lcd.print(nuidPICC[i]);
      lcd.print(" ");
    }
    cooltime = 5;
    lcd.setCursor(0,1);
    lcd.write(2);
    lcd.print("23:59");
    Serial.println(nuidPICC[0]);
  
    for (byte i = 0; i < 4; i++) {
        nuidPICC_back[i] = nuidPICC[i]; //nuidPICC가 찍힌 값
    }

    for (byte i = 0; i < 4; i++) {
        nuidPICC[i] = NULL;
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  tone(SP_PIN, tones);
  delay(100);
  noTone(SP_PIN);
}
