// NFC Poker Game V1 — Rule-Corrected Texas Hold'em with 4x4 Keypad Input

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_PN532.h>
#include <Keypad.h>
#include <SD.h>


#define PN532_IRQ   (2)
#define PN532_RESET (3)
#define DEBOUNCE_MS 200



LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {26, 27, 28, 29};
byte colPins[COLS] = {22, 23, 24, 25};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String raiseBuffer = "";
bool collectingRaise = false;

const int chipSelect = 53;
File myFile;


const int MAX_PLAYERS = 6;
const int CARDS_PER_PLAYER = 2;
const char BASE_NAME[] = "NFCINF";    // 6 chars so you can do NFCINF0…NFCINF9
const uint8_t MAX_INDEX = 99;         // up to two-digit suffixes
const String RECORDS_DIR = "Records";
String filePath;      // e.g. "Records/NFCINF3.txt"
uint8_t fileIndex = 0;
String  fileName;
int startingChips = 1000;
int smallBlind = 10;
int bigBlind = 20;
int winnings = 0;
int inGameTot = 0;



String inputBuffer = "";


int numPlayers = 0;
String playerHands[MAX_PLAYERS][CARDS_PER_PLAYER];
int playerChips[MAX_PLAYERS];
bool hasFolded[MAX_PLAYERS];
int playerBets[MAX_PLAYERS];
bool inGame[MAX_PLAYERS];
bool hasActed[MAX_PLAYERS];
bool isAllIn[MAX_PLAYERS];  // track players who have gone all‑in (and can’t act further)
String scannedUIDs[52];
int totalScanned = 0;
int pot = 0;
String masterKeyStr[2] = {
 "1EC30E02",
 "21F41605"
};

// Mapping of UIDs to playing cards
String knownUIDs[52] = {
  "0484BA5A061F90", // Ace of Hearts
  "0484B95A061F90", // 2 of Hearts
  "0484B85A061F90", // 3 of Hearts
  "0484B65A061F90", // 4 of Hearts
  "0484B45A061F90", // 5 of Hearts
  "0484B55A061F90", // 6 of Hearts
  "0484B75A061F90", // 7 of Hearts
  "0484B35A061F90", // 8 of Hearts
  "0484AE5A061F90", // 9 of Hearts
  "0484B15A061F90", // 10 of Hearts
  "0484B05A061F90", // Jack of Hearts
  "0484B25A061F90", // Queen of Hearts
  "0484AF5A061F90", // King of Hearts
  "0484A05A061F90", // Ace of Clubs
  "0484AD5A061F90", // 2 of Clubs
  "0484AC5A061F90", // 3 of Clubs
  "0484A15A061F90", // 4 of Clubs
  "0484AB5A061F90", // 5 of Clubs
  "0484AA5A061F90", // 6 of Clubs
  "0484A95A061F90", // 7 of Clubs
  "0484A85A061F90", // 8 of Clubs
  "0484A75A061F90", // 9 of Clubs
  "0484A45A061F90", // 10 of Clubs
  "0484A65A061F90", // Jack of Clubs
  "0484A55A061F90", // Queen of Clubs
  "0484A35A061F90", // King of Clubs
  "0484A25A061F90", // Ace of Diamonds
  "04849C5A061F90", // 2 of Diamonds
  "0484975A061F90", // 3 of Diamonds
  "0484985A061F90", // 4 of Diamonds
  "04849E5A061F90", // 5 of Diamonds
  "04849F5A061F90", // 6 of Diamonds
  "04849D5A061F90", // 7 of Diamonds
  "04849A5A061F90", // 8 of Diamonds
  "0484945A061F90", // 9 of Diamonds
  "0484955A061F90", // 10 of Diamonds
  "04849B5A061F90", // Jack of Diamonds
  "0484995A061F90", // Queen of Diamonds
  "0484965A061F90", // King of Diamonds
  "04848B5A061F90", // Ace of Spades ----------
  "0484885A061F90", // 2 of Spades
  "0484935A061F90", // 3 of Spades
  "0484905A061F90", // 4 of Spades
  "04848D5A061F90", // 5 of Spades
  "0484925A061F90", // 6 of Spades
  "0484875A061F90", // 7 of Spades
  "04848E5A061F90", // 8 of Spades
  "04848F5A061F90", // 9 of Spades
  "04848A5A061F90", // 10 of Spades
  "04848C5A061F90", // Jack of Spades
  "0484915A061F90", // Queen of Spades
  "0484895A061F90" // King of Spades
};


String cardLabels[52] = {
  "A-H",  // Ace of Hearts
  "2-H",  // 2 of Hearts
  "3-H",   // 3 of Hearts
  "4-H",
  "5-H",
  "6-H",
  "7-H",
  "8-H",
  "9-H",
  "10-H",
  "J-H",
  "Q-H",
  "K-H",
  "A-C",
  "2-C",
  "3-C",
  "4-C",
  "5-C",
  "6-C",
  "7-C",
  "8-C",
  "9-C",
  "10-C",
  "J-C",
  "Q-C",
  "K-C",
  "A-D",
  "2-D",
  "3-D",
  "4-D",
  "5-D",
  "6-D",
  "7-D",
  "8-D",
  "9-D",
  "10-D",
  "J-D",
  "Q-D",
  "K-D",
  "A-S",
  "2-S",
  "3-S",
  "4-S",
  "5-S",
  "6-S",
  "7-S",
  "8-S",
  "9-S",
  "10-S",
  "J-S",
  "Q-S",
  "K-S"
};
String communityCards[5];
int comCards = 0;
int dealerPos = 0;
int currentBet = 0;
int totalHands = 1;
bool preFlop = false;
bool showCards = false;
int pass = 0;
int key = 2653;
int equalChips;

enum Phase { SETUP, DEAL, PREFLOP, FLOP, TURN, RIVER, SHOWDOWN, WINNER };
Phase gamePhase = SETUP;

char getDebouncedKey() {
  char k = keypad.getKey();
  if (!k) return NO_KEY;
  delay(DEBOUNCE_MS);
  // consume any repeats of the same key until it's released
  while ( keypad.getKey() == k ) { }
  return k;
}

void(* resetFunc)(void) = 0; // RESET FUNCTION

void checkForReset(char k) { // RESET FUNCTION HELPER
  if (k == 'D') {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Resetting...        ");
    delay(500);
    resetFunc();  // will reboot the micro
  }
}

int getNumberFromKeypad(String prompt, String prompt2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(prompt);
  if (prompt2 != "") { 
    lcd.setCursor(0, 3);
    lcd.print(prompt2);
  }
  inputBuffer = "";
  while (true) {
    char key = getDebouncedKey();
    if (key) {
      if (key == '#') {
        return inputBuffer.toInt();
      } else if (key == '*') {
        if (inputBuffer.length() > 0) {
          inputBuffer.remove(inputBuffer.length() - 1);
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          lcd.print(inputBuffer);
        }
      } else if (isdigit(key)) {
        inputBuffer += key;
        lcd.setCursor(0, 1);
        lcd.print(inputBuffer);
      }
    }
  }
}

int getNumberFromKeypad(String prompt) {
  return getNumberFromKeypad(prompt, "");
}

void waitForRaise(int &raiseAmount) {
  lcd.clear();
  lcd.print("Enter Raise Amt:");
  inputBuffer = "";
  while (true) {
    char key = getDebouncedKey();
    if (key) {
      if (key == '#') {
        raiseAmount = inputBuffer.toInt();
        return;
      } else if (key == '*') {
        if (inputBuffer.length() > 0) {
          inputBuffer.remove(inputBuffer.length() - 1);
          lcd.setCursor(0, 1);
          lcd.print("                ");
          lcd.setCursor(0, 1);
          lcd.print(inputBuffer);
        }
      } else if (isdigit(key)) {
        inputBuffer += key;
        lcd.setCursor(0, 1);
        lcd.print(inputBuffer);
      }
    }
  }
}


String uidToString(uint8_t *uid, uint8_t len) {
  String result = "";
  for (uint8_t i = 0; i < len; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

String getCardLabel(String uidStr) {
  for (int i = 0; i < 52; i++) {
    if (knownUIDs[i] == uidStr) return cardLabels[i];
  }
  return "Unknown";
}

bool uidScanned(String uidStr) {
  for (int i = 0; i < totalScanned; i++) {
    if (scannedUIDs[i] == uidStr) return true;
  }
  return false;
}

void waitForCardScan(String prompt, String &cardOut) {
  lcd.clear();
  lcd.print(prompt);
  while (true) {
    uint8_t uid[7]; uint8_t uidLen;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) {
      String uidStr = uidToString(uid, uidLen);
      if (!uidScanned(uidStr)) {
        cardOut = getCardLabel(uidStr);
        scannedUIDs[totalScanned++] = uidStr;
        lcd.setCursor(0, 1);
        showCards || prompt == "Community Card" ? lcd.print(cardOut + "                 ") : lcd.print("XX-X                ");
        delay(1000);
        return;
      } else {
        lcd.setCursor(0, 1);
        lcd.print("Card already used   ");
        delay(1000);
      }
    }
  }
}

void dealHoleCards() {
  myFile = SD.open(filePath, FILE_WRITE);
  for (int c = 0; c < 2; c++) {
    for (int p = 0; p < numPlayers; p++) {
      int player = (p + dealerPos + 1) % numPlayers;
      if (inGame[player] && playerChips[player] > 0) {
        String card;
        waitForCardScan("P" + String(player + 1) + " Card " + String(c + 1), card);
        playerHands[player][c] = card;
        if (myFile) {
          myFile.println("p" + String(player + 1) + ":" + card);
        } else {
          sdErrorScreen();
        }
      }
    }
  }
  myFile.close();
}


int countNotFolded() {
  int c = 0;
  for (int i = 0; i < numPlayers; ++i) {
    if (inGame[i] && !hasFolded[i]) ++c;
  }
  return c;
}

void resetAfterRaise(int raiser) {
  for (int i = 0; i < numPlayers; ++i) {
    if (!hasFolded[i] && !isAllIn[i]) {
      hasActed[i] = (i == raiser);  // raiser has acted; others now must respond
    }
  }
}


int countAbleToAct() {             // can still take betting actions
  int c = 0;
  for (int i = 0; i < numPlayers; ++i)
    if (inGame[i] && !hasFolded[i] && !isAllIn[i]) ++c;
  return c;
}

void sdErrorScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SD Card Error...    ");
  lcd.setCursor(0, 1);
  lcd.print("Insert SD Card      ");
  lcd.setCursor(0, 2);
  lcd.print("Press Reset         ");
  while (true) {
    char k = getDebouncedKey();
    checkForReset(k);
  }
}

void scanCommunity(int startIndex, int count) {
  myFile = SD.open(filePath, FILE_WRITE);
  if (myFile) {
    myFile.print("com:");
    for (int i = 0; i < count; i++) {
      String card;
      waitForCardScan("Community Card", card);
      communityCards[startIndex + i] = card;
      comCards++;
      if (i < count - 1) myFile.print(card + ",");
      else myFile.println(card);
    }
    myFile.close();
  } else {
    sdErrorScreen();
  }
}

bool isGameAlive() {
  int active = 0;
  for (int i = 0; i < numPlayers; i++) {
    if (!hasFolded[i] && !isAllIn[i]) active++;
  }
  return active > 1;
}


void printScreen(int current, int pot) {
  lcd.clear();
  lcd.print("P" + String(current + 1) + ": $" + String(playerChips[current]));
  lcd.setCursor(0, 1);
  lcd.print("Pot: $" + String(pot));
  lcd.setCursor(0, 2);
  if (showCards) {
    lcd.print("Cards: " + String(playerHands[current][0]) + " & " + String(playerHands[current][1]) + "    ");
  } else {
    lcd.print("Cards: XX-X & XX-X  ");
  }
}

void bettingRound() {
  bool roundComplete = false;
  int lastRaiser = -1;
  int current = 0;
  currentBet = 0;
  //bool hasActed[MAX_PLAYERS];  // Track if each player acted
  int minRaise = 0;
  int activePlayers = 0;



  for (int i = 0; i < numPlayers; i++) {
    if (inGame[i] && playerChips[i] > 0 && !isAllIn[i]) activePlayers++;
    isAllIn[i] = (playerChips[i] == 0);
    hasActed[i] = hasFolded[i];   // folded seats are “done”
    playerBets[i] = 0;
  }

  if (preFlop) {
    // find SB and BB positions
    int sb = (dealerPos + 1) % numPlayers;
    while (!inGame[sb]) sb = (sb + 1) % numPlayers;
    int bb = (sb + 1) % numPlayers;
    while (!inGame[bb]) bb = (bb + 1) % numPlayers;

    // heads-up: only BB posts
    if (activePlayers == 2) {
      // post BB only
      int toPost = min(playerChips[sb], bigBlind);
      playerBets[sb]   = toPost;
      playerChips[sb] -= toPost;
      pot              = toPost;

      // log just the big blind
      myFile = SD.open(filePath, FILE_WRITE);
      if (myFile) {
        myFile.println("p" + String(sb + 1) + ":bb");
        myFile.close();
      } else {
        sdErrorScreen();
      }

      currentBet   = toPost;
      lastRaiser   = sb;
      // first action is dealer (acts first heads-up)
      current      = dealerPos;
      minRaise = toPost;
    }
    // >2 players: normal SB & BB
    else {
      // small blind
      int sbPost = min(playerChips[sb], smallBlind);
      playerBets[sb]   = sbPost;
      playerChips[sb] -= sbPost;

      // big blind
      int bbPost = min(playerChips[bb], bigBlind);
      playerBets[bb]   = bbPost;
      playerChips[bb] -= bbPost;

      pot = sbPost + bbPost;

      myFile = SD.open(filePath, FILE_WRITE);
      if (myFile) {
        myFile.println("p" + String(sb + 1) + ":sb");
        myFile.println("p" + String(bb + 1) + ":bb");
        myFile.close();
      } else {
        sdErrorScreen();
      }

      currentBet = bbPost;
      lastRaiser = bb;
      current    = (bb + 1) % numPlayers;
      minRaise = bbPost;
    }

    preFlop = false;
  } else {
    current = (dealerPos + 1) % numPlayers;
  }
  myFile = SD.open(filePath, FILE_WRITE);
  if (!myFile) sdErrorScreen();
  while (!roundComplete) {
    // if only one player remains, end the round immediately
    if (countNotFolded() <= 1) {
      roundComplete = true;
      break;
    }

    if (hasFolded[current] || isAllIn[current]) { 
      current = (current + 1) % numPlayers; 
      continue; 
    }

    printScreen(current, pot);
    lcd.setCursor(0, 3);
    lcd.print("$" + String(currentBet - playerBets[current]) + " To Call");

    bool actionTaken = false;
    while (!actionTaken) {
      if (countNotFolded() == 1) {
        roundComplete = true;
        break;
      }
      char key = getDebouncedKey();
      if (key) {
        if (key == 'A') {
          int toCall = currentBet - playerBets[current];
          int stack  = playerChips[current];
          int contrib;

          if (stack > toCall) {
            // All‑in raise
            contrib          = stack;
            playerChips[current] = 0;
            playerBets[current]  += contrib;
            currentBet            = playerBets[current];
            minRaise              = contrib - toCall;
            lastRaiser            = current;
            isAllIn[current]      = true;
            resetAfterRaise(current);
            actionTaken           = true;
          } else {
            // All‑in call (not enough to cover full bet)
            contrib          = stack;
            playerChips[current] = 0;
            playerBets[current]  += contrib;
            isAllIn[current] = true;
            hasActed[current] = true;
            actionTaken = true;
          }

          pot              += contrib;

          if (myFile) myFile.println("p" + String(current+1) + ":a-" + String(contrib));
          printScreen(current, pot);
          lcd.setCursor(0,3);
          lcd.print("P" + String(current+1) + " ALL IN $" + String(contrib));
        } else if (key == 'B') {
          hasFolded[current] = true;
          actionTaken = true;
          hasActed[current] = true;
          printScreen(current, pot);
          lcd.setCursor(0, 3); lcd.print("Folded              ");
          if (myFile) {
            myFile.println("p" + String(current + 1) + ":F");
          } else {
            sdErrorScreen();
          }
          
        } else if (key == 'C') {
          int toCall = currentBet - playerBets[current];
          if (playerChips[current] > toCall) {
            playerChips[current] -= toCall;
            playerBets[current] += toCall;
            pot += toCall;
            actionTaken = true;
            hasActed[current] = true;
            if (myFile) {
              myFile.println("p" + String(current + 1) + ":c-" + String(toCall));
            } else {
              sdErrorScreen();
            }
            printScreen(current, pot);
            lcd.setCursor(0, 3); lcd.print("Called: $" + String(toCall));
          } else {
            printScreen(current, pot);
            lcd.setCursor(0, 3);
            lcd.print("Insufficient chips  ");
            delay(1000);
            lcd.print("                    ");
          }
        } else if (key == 'D') {
            if (!collectingRaise) {
              raiseBuffer = "";
              collectingRaise = true;
              printScreen(current, pot);
              lcd.setCursor(0, 3);
              lcd.print("Raise: $            ");
            } else if (raiseBuffer.length() > 0) {
              int raiseAmount = raiseBuffer.toInt();
              raiseAmount -= playerBets[current];
              int leftToCall = currentBet - playerBets[current];
              int raiseAmt = raiseAmount - leftToCall;

              if (raiseAmt >= minRaise && playerChips[current] > raiseAmt) {
                playerChips[current] -= raiseAmount;
                playerBets[current] += raiseAmount;

                pot += raiseAmount;
                minRaise = raiseAmt;
                currentBet += raiseAmt;
                lastRaiser = current;

                resetAfterRaise(current);
                hasActed[current] = true;
                actionTaken = true;

                printScreen(current, pot);
                lcd.setCursor(0, 3);
                lcd.print("Raised by: $" + String(raiseAmt));
                if (myFile) {
                  myFile.println("p" + String(current + 1) + ":r-" + String(raiseAmount));
                } else {
                  sdErrorScreen();
                }
              } else {
                printScreen(current, pot);
                lcd.setCursor(0, 3);
                lcd.print("Invalid Raise       ");
              }
              collectingRaise = false;
            }
          } else if (key == '*') {
            if (raiseBuffer.length() > 0) {
              raiseBuffer.remove(raiseBuffer.length() - 1);
              lcd.setCursor(8, 3);
              lcd.print(raiseBuffer + " ");
            }
          } else if (collectingRaise && isDigit(key)) {
            raiseBuffer += key;
            lcd.setCursor(8, 3);
            lcd.print(raiseBuffer);
          }
        }
    }
    delay(1000);
    current = (current + 1) % numPlayers;

    if (countNotFolded() <= 1) { break; }

    // Check if all active players have called or folded
    bool allMatched = true;
    for (int i = 0; i < numPlayers; i++) {
      if (!hasFolded[i] && !isAllIn[i]) {
        if (playerBets[i] != currentBet || !hasActed[i]) {
          allMatched = false; break;
        }
      }
    }

    if (allMatched) roundComplete = true;
  }
  if (myFile) myFile.close();
}

void setup() {
  lcd.begin();
  lcd.backlight();
  Serial.begin(115200);
  nfc.begin();
  nfc.SAMConfig();

  lcd.setCursor(0,0);
  lcd.print("Initializing SD card");
  delay(500);

  if (!SD.begin(chipSelect)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initialization fail ");
    lcd.setCursor(0, 1);
    lcd.print("Insert SD card      ");
    lcd.setCursor(0, 2);
    lcd.print("Reset to try again  ");
    while (true) {
      char k = getDebouncedKey();
      checkForReset(k);
    }
  }

  if (!SD.exists(RECORDS_DIR)) {
    SD.mkdir(RECORDS_DIR);
  }




  do {
    if (fileIndex == 0) {
      fileName = String(BASE_NAME);
    } else {
      fileName = String(BASE_NAME) + String(fileIndex);
    }
    fileName += ".txt";
    filePath = RECORDS_DIR + "/" + fileName;
    if (++fileIndex > MAX_INDEX) sdErrorScreen();
  } while ( SD.exists(filePath) );

  // feedback
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initialized         ");
  lcd.setCursor(0, 1);
  lcd.print("All systems ready   ");
  lcd.setCursor(0,2);
  lcd.print("Ready to log to     ");
  lcd.setCursor(0,3);
  lcd.print(fileName);
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Master Card    ");
  lcd.setCursor(0, 1);
  lcd.print("Or Press D          ");
  while (true) {
    // 1) Check keypad first
    char key = getDebouncedKey();
    if (key == 'D') {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cards Will Be Masked");
      delay(1500);
      break;
    }

    // 2) Then do a NON-BLOCKING NFC poll (small timeout)
    uint8_t uid[7]; 
    uint8_t uidLen;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 50)) {
      String uidStr = uidToString(uid, uidLen);
      if (uidStr == masterKeyStr[0] || uidStr == masterKeyStr[1]) {
        showCards = true;
        lcd.clear();
        lcd.print("Showing Cards       ");
        delay(1500);
        break;
      }
    }

    // small yield to avoid tight spin
    delay(5);
  }

  numPlayers = getNumberFromKeypad("Players 2-6:        ");
  if (numPlayers < 2) numPlayers = 2;
  if (numPlayers > 6) numPlayers = 6;


  bool equalFlag = ( getNumberFromKeypad("Equal stacks?       ","1=Yes 0=No          ") == 1 );

  if (equalFlag) {
    startingChips = getNumberFromKeypad("Starting chip size? ");
    if (startingChips) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Start Chips: $1000");
      delay(1500);
    }
    for (int i = 0; i < numPlayers; i++) {
      playerChips[i] = startingChips;
    }
  } else {
    for (int i = 0; i < numPlayers; i++) {
      playerChips[i] = getNumberFromKeypad("Chips for P" + String(i+1) + "        ");
    }
  }

  bigBlind = getNumberFromKeypad("Big-Blinds size?    ", "Must be 2|x         ");
  if (bigBlind < 2 || bigBlind % 2 != 0) {
    bigBlind = 10;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BIG BLIND : $10     ");
    delay(2000);
  }
  smallBlind = bigBlind / 2;


  for (int i = 0; i < numPlayers; i++) {
    hasFolded[i] = false;
    inGame[i]    = true;
    isAllIn[i]   = false;    // reset your all-in flag too
  }

  gamePhase = DEAL;
  preFlop = true;

  myFile = SD.open(filePath, FILE_WRITE);

  if (myFile) {
    myFile.println("players:" + String(numPlayers));
    myFile.println("pot:" + String(startingChips));
    myFile.println("sb:" + String(smallBlind));
    myFile.println("bb:" + String(bigBlind));
    myFile.println("Game Start");
    myFile.close();
  } else {
    sdErrorScreen();
  }
}


void loop() {
  switch (gamePhase) {
    case DEAL:
      myFile = SD.open(filePath, FILE_WRITE);
      if (myFile) {
        myFile.println("hand:" + String(totalHands));
        myFile.println("dealer:" + String(dealerPos));
        myFile.print("Stacks:[");
        for (int i = 0; i < numPlayers; i++) {
          if (i < numPlayers - 1) myFile.print(String(playerChips[i]) + ",");
          else myFile.print(String(playerChips[i]));
        }
        myFile.println("]");
        myFile.close();
      } else {
        sdErrorScreen();
      }
      
      dealHoleCards();
      gamePhase = PREFLOP;
      break;
    case PREFLOP:
      bettingRound();
      gamePhase = (countNotFolded() <= 1 || countAbleToAct() <= 1) ? SHOWDOWN : FLOP;
      break;
    case FLOP:
      scanCommunity(0, 3);
      bettingRound();
      gamePhase = (countNotFolded() <= 1 || countAbleToAct() <= 1) ? SHOWDOWN : TURN;
      break;
    case TURN:
      scanCommunity(3, 1);
      bettingRound();
      gamePhase = (countNotFolded() <= 1 || countAbleToAct() <= 1) ? SHOWDOWN : RIVER;
      break;
    case RIVER:
      scanCommunity(4, 1);
      bettingRound();
      gamePhase = SHOWDOWN;
      break;
    case SHOWDOWN:
      if (countNotFolded() == 1) {
        int winner = -1;
        for (int i = 0; i < numPlayers; ++i) {
          if (inGame[i] && !hasFolded[i]) { winner = i; break; }
        }
        if (winner >= 0) {
          playerChips[winner] += pot;
          myFile = SD.open(filePath, FILE_WRITE);
          if (myFile) { myFile.println("W-p" + String(winner + 1) + ":" + String(pot)); myFile.close(); }
          lcd.clear();
          lcd.setCursor(0,0); lcd.print("Winner: P" + String(winner+1));
          lcd.setCursor(0,1); lcd.print("Collected $" + String(pot));
          delay(2000);
          pot = 0;
        }
        gamePhase = WINNER;   // jump straight to overall winner check
        break;
      }
      lcd.clear();
      lcd.print("Showdown");
      delay(1500);
      if (comCards != 5) scanCommunity(comCards, 5 - comCards);
      
      myFile = SD.open(filePath, FILE_WRITE);
      if (!myFile) sdErrorScreen();

      for (int i = 0; i < numPlayers; i++) {
        if (hasFolded[i] && inGame[i]) continue;
        
        winnings = getNumberFromKeypad("P" + String(i + 1) + " winnings?        ", "Pot: $" + String(pot));
        if (winnings > 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Player " + String(i + 1) + "wins        ");
          lcd.setCursor(0, 1);
          lcd.print("$" + String(winnings));
          playerChips[i] += winnings;
          pot -= winnings;
          myFile.println("W-p" + String(i + 1) + ":" + String(winnings));
          delay(2500);
        }
      }
      myFile.close();
      lcd.clear();


      totalHands++;
      totalScanned = 0;
      comCards = 0;
      preFlop = true;
      pot = 0;
      inGameTot = 0;
      gamePhase = DEAL;
      dealerPos = (dealerPos + 1) % numPlayers;  // wrap the dealer button
      Serial.println(String(playerChips[0]) + ", " + String(playerChips[1]) + ", " + String(playerChips[2]));
      for (int i = 0; i < numPlayers; i++) {
        if (playerChips[i] > 0) {
          inGame[i]    = true;
          inGameTot++;
        } else {
          inGame[i]    = false;   // turn off busted seats
        }
        hasFolded[i] = false;
        isAllIn[i]   = false;
      }

      if (inGameTot == 1) {
        gamePhase = WINNER; 
        break;
      }

      lcd.setCursor(0, 0);
      lcd.print("     NFC POKER      ");
      lcd.setCursor(0, 1);
      lcd.print("Hand: " + String(totalHands)); 
      lcd.setCursor(0, 2);
      lcd.print("Dealer: p" + String(dealerPos + 1));
      lcd.setCursor(0, 3);
      lcd.print("'D' To Continue     ");
      while (true) {
        char k = getDebouncedKey();
        if (k == 'D') {
          break;
        }
        delay(5);
      }
      break;
    case WINNER:
      for (int i = 0; i < numPlayers; i++) {
        if (playerChips[i] > 0) {
          myFile = SD.open(filePath, FILE_WRITE);
          myFile.println("Winner:p" + String(i + 1) + "-" + playerChips[i]);
          myFile.close();
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("     NFC POKER      ");
          lcd.setCursor(0, 1);
          lcd.print("Winner: p" + String(i + 1));
          lcd.setCursor(0, 2);
          lcd.print("Total winnings:     ");
          lcd.setCursor(0, 3);
          lcd.print("$" + String(playerChips[i]));
          delay(5000);
          while(true) {
            char k = getDebouncedKey();
            checkForReset(k);
          }
        }
      }
  }
}
