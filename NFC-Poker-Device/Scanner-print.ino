#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)

LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);


String cards[52] = {
  "Ace of Hearts",
  "2 of Hearts",
  "3 of Hearts",
  "4 of Hearts",
  "5 of Hearts",
  "6 of Hearts",
  "7 of Hearts",
  "8 of Hearts",
  "9 of Hearts",
  "10 of Hearts",
  "Jack of Hearts",
  "Queen of Hearts",
  "King of Hearts",
  "Ace of Clubs",
  "2 of Clubs",
  "3 of Clubs",
  "4 of Clubs",
  "5 of Clubs",
  "6 of Clubs",
  "7 of Clubs",
  "8 of Clubs",
  "9 of Clubs",
  "10 of Clubs",
  "Jack of Clubs",
  "Queen of Clubs",
  "King of Clubs",
  "Ace of Diamonds",
  "2 of Diamonds",
  "3 of Diamonds",
  "4 of Diamonds",
  "5 of Diamonds",
  "6 of Diamonds",
  "7 of Diamonds",
  "8 of Diamonds",
  "9 of Diamonds",
  "10 of Diamonds",
  "Jack of Diamonds",
  "Queen of Diamonds",
  "King of Diamonds",
  "Ace of Spades",
  "2 of Spades",
  "3 of Spades",
  "4 of Spades",
  "5 of Spades",
  "6 of Spades",
  "7 of Spades",
  "8 of Spades",
  "9 of Spades",
  "10 of Spades",
  "Jack of Spades",
  "Queen of Spades",
  "King of Spades"
};


// 💡 CHANGE THIS TO SET HOW MANY UNIQUE UID'S TO SCAN
const int NUM_TO_SCAN = 52;

String scannedUIDs[NUM_TO_SCAN];
int uidCount = 0;

String uidToString(uint8_t *uid, uint8_t len) {
  String result = "";
  for (uint8_t i = 0; i < len; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

int uidAlreadyScanned(String uidStr) {
  for (int i = 0; i < uidCount; i++) {
    if (scannedUIDs[i] == uidStr) return i;
  }
  return -1;
}

void printAllUIDs() {
  Serial.println("\nUnique UID Code Format:");
  Serial.println("String knownUIDs[52] = {");
  for (int i = 0; i < uidCount; i++) {
    if (i < uidCount - 1) Serial.println("  \"" + scannedUIDs[i] + "\"," + "// " + cards[i]);
    else Serial.println("  \"" + scannedUIDs[i] + "\"");
  }
  Serial.println("};");
  Serial.println("-------------------------");
}

void setup() {
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for tag...");
  lcd.setCursor(0, 1);
  lcd.print(cards[uidCount]);
  nfc.begin();

  if (!nfc.getFirmwareVersion()) {
    lcd.clear();
    lcd.print("PN532 not found");
    while (1);
  }

  nfc.SAMConfig();
}

void loop() {
  if (uidCount >= NUM_TO_SCAN) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan limit hit");
    printAllUIDs();
    delay(5000);
    lcd.clear();
    return;  // stop scanning
  }

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String uidStr = uidToString(uid, uidLength);
    int result = uidAlreadyScanned(uidStr);
    if (result == -1) {
      scannedUIDs[uidCount] = uidStr;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("New Tag Saved");
      lcd.setCursor(0, 1);
      lcd.print(uidStr);

      Serial.println();
      Serial.print("✅ ");
      Serial.print(cards[uidCount]);
      Serial.print(" scanned: ");
      Serial.println(uidStr);
      uidCount++;
      Serial.println("-------------------------");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Already Scanned");
      lcd.setCursor(0, 1);
      lcd.print(cards[result]);
    }

    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Waiting for tag: ");
    lcd.setCursor(0, 1);
    lcd.print(cards[uidCount]);
  }
}
