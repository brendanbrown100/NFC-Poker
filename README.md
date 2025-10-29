# NFC-Poker
Texas Holdem Poker, using an arduino and rfid technolagy you can analize and revisit past hands and games.

------------------------- NFC-POKER-V1 -------------------------

------------------------- FOLDERS -------------------------

 - Records: Stores All Game Info
   - Files labeled NFC_INF (higher the number after the newer the game)

 - NFC-Game-Reader: Open dist to find the executable file to run
   - Upon running give a .txt from the Records folder

 - NFC-Poker-Script: Can be ignored, just a backup of the code installed on the Arduino

------------------------- HOW-TO -------------------------

 - KeyPad Actions
   - 'A' : All-in
   - 'B' : Fold
   - 'C' : Call
   - 'D' : reset (Only when reset option appears occasions)
   - During the betting round to raise a bet press 'D' followed by the raise then '#' again
     - The raise amount is the total amount the player has bet in the current round after the raise
   - When typing any numbers press '*' to delete/backspace

------------------------- PARTS -------------------------

  - Arduino Mega 2560/Elegoo Mega R3 Board ATmega 2560 : $49.90 / $22.99

  - IIC I2C TWI Serial 2004 20x4 LCD Module Shield : $12.99

  - HiLetgo PN532 NFC NXP RFID Module V3 Kit Near Field Communication : $8.99

  - NFC Stickers Ntag213 NFC Tags 25mm White Blank NFC Circular Sticker Writable and Programmable : $29.99 (100 count)

  - Micro SD TF Card Adapter Reader Module : $2.00

  - SanDisk microSDHC 8gb Memory Card : $9.99

  - Project Box (7.75in x 4.5in x 2.5in) : $7.00

------------------------- Pin-Out Diagram -------------------------

  - LCD Module Shield 
    - GND --> GND
    - VCC --> 5V
    - SDA --> PIN20
    - SCL --> PIN21

  - PN532 
    - GND --> GND
    - VCC --> 5V
    - SDA --> PIN20
    - SCL --> PIN21

  - KeyPad 
    - COLLUMN 1 --> PIN22
    - COLLUMN 2 --> PIN23
    - COLLUMN 3 --> PIN24
    - COLLUMN 4 --> PIN25
    - ROW 1 --> PIN26
    - ROW 2 --> PIN27
    - ROW 3 --> PIN28
    - ROW 4 --> PIN29

  - Micro SD Module
    - GND --> GND
    - VCC --> 5V
    - MISO --> PIN50
    - MOSI --> PIN51
    - SCK --> PIN52
    - CS --> PIN53



