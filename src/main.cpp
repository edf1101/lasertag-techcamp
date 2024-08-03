// File | Preferences -> then add this url into additional board managers: https://dl.espressif.com/dl/package_esp32_index.json
// Tools | Board | Board Manager -> search for ESP32 and intstall
// Tools | Board | ESP32 Dev Module
// Also need libraries: XT_DAC_Audio, FASTCRC-master, u8g2, MD_REncoder

#include "src/Infrared/Infrared.h"
#include "src/Radio/Radio.h"
#include "src/StudentHidden/StartupScreen.h"
#include "Images.h"

#include "SoundData.h"
#include "XT_DAC_Audio.h"
#include "Weapons.h"

#define mainFont u8g2_font_logisoso16_tf

#include "src/Menus/Menu.h"
#include <Preferences.h>

Preferences preferences;

const uint32_t FLASH_LOG_START = 0x200000;
const uint32_t FLASH_LOG_END = 0x3FFFFF;

// Pin definitions
const gpio_num_t irPinOut = GPIO_NUM_26;
const gpio_num_t irPinIn = GPIO_NUM_35;
const int RED = 32;
const int GREEN = 33;
const int YELLOW = 27;
const int TRIGGER = 12;
const int IR_OUT = 26;
const int IR_IN = 35;
const int DIGITAL_SPKR = 25;
const int BTN_LEFT = 0;  // Only on old guns
const int MASTER_OUT = 14;
const int MASTER_IN = 5;

// Time that a gun stays dead when hit
const int TEMP_DEAD_MILLIS = 10000;

// Default startup params
int wifiChannel = 1;

int secsSinceStartup = 0;  // increments once a second
int secsGameStarted = 0;   // secs stamp when game started
int lastSecsLoop = 0;      // used to track when need to do tasks every second
float health = 1;          // health of gun
uint32_t nextAliveMillis = 0;   // used to track when next alllowed to be alive (it is set 10 secs into the future to make the gun temporarily 'dead' if shot but not lost all lives
uint32_t nextPickupMillis = 0;  // used to track when to next pickup health pack or ammo
uint32_t lastStrikeMillis = 0;  // millis that last 'strike' was made
uint32_t stopJoiningMillis = 0;
uint32_t nextTransmitMillis = 0;
uint32_t loggedPackets = 0;

int whoILastHit = -1;
char lastTeamWon = -1;
char mineTriggered = 0;
char lastIR[3] = {0, 0, 0};
char page = 0;
char redrawScreen = 0;

char lastManStanding = -1;
unsigned char unitsAlive = 0;
unsigned char greenAlive, redAlive = 0;
unsigned char greenOn, redOn = 0;

unsigned char iAmGameController = 0;
unsigned char blueDominated, redDominated = 0;
int assignedUnitNumber = -1;
int8_t infraredCMDExpected = -1;
uint8_t logging = 0;
unsigned char masterMode = 0;
unsigned char testMode = 0;
int encoderPosition = 0;

int yellowFlashMillis = millis();
bool notChecking;
Infrared laserTagIR;


XT_DAC_Audio_Class DacAudio(DIGITAL_SPKR, 0);  // Create the main player class object.
// Use GPIO for SPKR and timer 0
MenuItem *mainMenu;

// Useful icon editor: https://xbm.jazzychad.net/

void sendIRControl(unsigned char controlCode) {
  laserTagIR.setPower(3);
  digitalWrite(YELLOW, HIGH);               // muzzle flash
  laserTagIR.sendIR(1, 0, controlCode, 0);  // Control mode, team 0 (irrelevent), base cmd, no extra bits
  // Now ignore any IR received when sent
  delay(30);  // 30ms should be enough to wait until IR sent
  laserTagIR.receiveIR();
  laserTagIR.infraredReceived = 0;
  delay(100);
  digitalWrite(YELLOW, LOW);
  laserTagIR.setPower(laserConfig.irPower);
  delay(400);
  infraredCMDExpected = controlCode;
}

void controlGame(unsigned char gameNum, GAME_TYPE thisGameType, unsigned char isRunning, unsigned char canJoin,
                 char winner) {
  if (isRunning) {
    if (!iAmGameController) {
      iAmGameController = 1;
      clearMonitorTable();
    }
  } else
    iAmGameController = 0;
  memcpy(&masterController, &laserConfig, sizeof(datapacketstruct));
  masterController.controllerNum = laserConfig.controllerNum;
  masterController.sequenceNumber++;
  masterController.gameType = thisGameType;
  masterController.isRunning = isRunning;
  masterController.canJoin = canJoin;
  masterController.winner = winner;
  masterController.friendlyFire = mainMenu->getParam("Frndly Fire");
  masterController.initLives = mainMenu->getParam("Lives");
  masterController.initAmmo = mainMenu->getParam("Ammo");
  masterController.irPower = mainMenu->getParam("IR Power");
  sendPacket(&masterController);
  currentMenuItem = topMenu;
  if (isRunning)
    stopJoiningMillis = millis() + 10000;
}

void menuChange(const char *itemName, int value) {
  if (strcmp(itemName, "I'm Playing") == 0) {
    preferences.begin("lt", false);
    preferences.putUChar("playing", value);
    laserConfig.canPlay = value;
    preferences.end();
  } else if (strcmp(itemName, "Tx:Red Base") == 0)
    sendIRControl(IR_CMD_RED_BASE);
  else if (strcmp(itemName, "Tx:Blue Base") == 0)
    sendIRControl(IR_CMD_BLUE_BASE);
  else if (strcmp(itemName, "Tx:Domination") == 0)
    sendIRControl(IR_CMD_DOMINATION);
  else if (strcmp(itemName, "Tx:Health") == 0)
    sendIRControl(IR_CMD_HEALTH);
  else if (strcmp(itemName, "Tx:Ammo") == 0)
    sendIRControl(IR_CMD_AMMO);
  else if (strcmp(itemName, "Tx:Red Mine") == 0)
    sendIRControl(IR_CMD_RED_MINE);
  else if (strcmp(itemName, "Tx:Blue Mine") == 0)
    sendIRControl(IR_CMD_BLUE_MINE);

  else if (strcmp(itemName, "Red") == 0 && !laserConfig.isRunning) {
    laserConfig.team = 1;
  } else if (strcmp(itemName, "Green") == 0) {
    laserConfig.team = 0;
  } else if (strcmp(itemName, "CTF") == 0)
    controlGame(laserConfig.unitNum, GAME_CTF, 1, 1, -1);
  else if (strcmp(itemName, "Death Match") == 0)
    controlGame(laserConfig.unitNum, GAME_DEATH_MATCH, 1, 1, -1);
  else if (strcmp(itemName, "Team Death") == 0)
    controlGame(laserConfig.unitNum, GAME_TEAM_DEATH, 1, 1, -1);
  else if (strcmp(itemName, "Domination") == 0) {
    controlGame(laserConfig.unitNum, GAME_DOMINATION, 1, 1, -1);
    blueDominated = 0;
    redDominated = 0;
  } else if (strcmp(itemName, "Trgt Practice") == 0)
    controlGame(laserConfig.unitNum, GAME_TARGET, 1, 1, -1);
  else if (strcmp(itemName, "Join for 10s") == 0)
    controlGame(laserConfig.unitNum, laserConfig.gameType, 1, 1, -1);
  else if (strcmp(itemName, "Stop Game") == 0)
    controlGame(laserConfig.unitNum, GAME_NONE, 0, 0, -1);
  else if (strcmp(itemName, "Unit Num") == 0) {
    preferences.begin("lt", false);
    preferences.putUChar("unitNum", value);
    laserConfig.unitNum = value;
    preferences.end();
  } else if (strcmp(itemName, "Game Num") == 0) {
    preferences.begin("lt", false);
    preferences.putUChar("gameNum", value);
    laserConfig.controllerNum = value;
    preferences.end();
  } else if (strcmp(itemName, "IR Power") == 0) {
    preferences.begin("lt", false);
    preferences.putUChar("irPower", value);
    laserConfig.irPower = value;
    preferences.end();
  } else if (strcmp(itemName, "Autonumber") == 0)
    controlGame(laserConfig.unitNum, GAME_AUTONUMBER, 0, 0, -1);
  else if (strcmp(itemName, "Delete Log") == 0) {
    int i;
    for (i = 0; i <= 511; i++) {
      u8g2.clearBuffer();
      u8g2.drawStr(0, 16, "Formatting");
      u8g2.drawStr(0, 36, "Sector");
      u8g2.setCursor(0, 56);
      u8g2.print(i + 1);
      u8g2.sendBuffer();
      ESP.flashEraseSector((FLASH_LOG_START / 4096) + i);
    }
    u8g2.clearBuffer();
    u8g2.drawStr(0, 16, "Done");
    u8g2.sendBuffer();
    while (digitalRead(ENC_SW) == HIGH);
    delay(250);
    loggedPackets = 0;
    currentMenuItem = topMenu;
  } else if (strcmp(itemName, "Logging") == 0) {
    char buffer[16];
    if (value) {
      int32_t i;
      uint32_t fourbytes;
      i = -1;
      do {
        i++;
        ESP.flashRead(FLASH_LOG_START + (i * 16), &fourbytes, 4);
      } while (fourbytes != 0xffffffff);
      loggedPackets = i;
      u8g2.clearBuffer();
      u8g2.drawStr(0, 16, "Prev Recs:");
      sprintf(buffer, "%d", loggedPackets);
      u8g2.setCursor(0, 36);
      u8g2.print(buffer);
      u8g2.drawStr(0, 56, "Logging On");
      u8g2.sendBuffer();
    } else {
      u8g2.clearBuffer();
      u8g2.drawStr(0, 16, "Logged:");
      sprintf(buffer, "%d", loggedPackets);
      u8g2.setCursor(0, 36);
      u8g2.print(buffer);
      u8g2.drawStr(0, 56, "Logging Off");
      u8g2.sendBuffer();
    }
    logging = value;
    delay(1000);
    currentMenuItem = topMenu;
  } else if (strcmp(itemName, "Dump Log") == 0) {
    uint32_t i;
    char buffer[100];
    datapacketstruct dp;
    i = 0;
    while (1) {
      u8g2.clearBuffer();
      u8g2.drawStr(0, 16, "0.5Mbps Dump ...");
      sprintf(buffer, "%d/%d", i + 1, loggedPackets);
      u8g2.setCursor(0, 36);
      u8g2.print(buffer);
      u8g2.sendBuffer();

      ESP.flashRead(FLASH_LOG_START + (i * 16), (uint32_t *) &dp, 16);
      if ((dp.crc == 0xffff) && (dp.unitNum == 0xff) && (dp.controllerNum == 0xff))
        break;
      sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
              dp.crc, dp.unitNum, dp.controllerNum, dp.sequenceNumber,
              dp.gameType, dp.isRunning, dp.canJoin, dp.winner,
              dp.friendlyFire, dp.initLives, dp.initAmmo, dp.irPower,
              dp.team, dp.lives, dp.ammo, dp.canPlay,
              dp.inGame, dp.hitBy, dp.strikes, dp.battery);
      i++;
    }
    while (digitalRead(ENC_SW) == HIGH);
    delay(250);
    currentMenuItem = topMenu;
  } else if (strcmp(itemName, "IR Test") == 0) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    delay(200);
    while (digitalRead(ENC_SW) == HIGH) {
      laserTagIR.receiveIR();
      if (laserTagIR.infraredReceived) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        delay(200);
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_7x13B_mr);  // menu system may use different fonts to us
        u8g2.drawStr(0, 16, "CRC:");
        u8g2.setCursor(28, 16);
        u8g2.print(laserTagIR.crcValid);
        u8g2.drawStr(38, 16, "Ctrl:");
        u8g2.setCursor(74, 16);
        u8g2.print(laserTagIR.irPacketIn.control);
        u8g2.drawStr(84, 16, "Team:");
        u8g2.setCursor(120, 16);
        u8g2.print(laserTagIR.irPacketIn.team);
        u8g2.setFont(mainFont);
        u8g2.drawStr(0, 36, "Data:");
        u8g2.setCursor(45, 36);
        if (laserTagIR.irPacketIn.control) {
          if (laserTagIR.irPacketIn.data == IR_CMD_BLUE_BASE)
            u8g2.print("Blue");
          else if (laserTagIR.irPacketIn.data == IR_CMD_RED_BASE)
            u8g2.print("Red");
          else if (laserTagIR.irPacketIn.data == IR_CMD_DOMINATION)
            u8g2.print("Dominate");
          else if (laserTagIR.irPacketIn.data == IR_CMD_HEALTH)
            u8g2.print("Health");
          else if (laserTagIR.irPacketIn.data == IR_CMD_AMMO)
            u8g2.print("Ammo");
          else if (laserTagIR.irPacketIn.data == IR_CMD_BLUE_MINE)
            u8g2.print("B Mine");
          else if (laserTagIR.irPacketIn.data == IR_CMD_RED_MINE)
            u8g2.print("R Mine");
        } else
          u8g2.print(laserTagIR.irPacketIn.data);
        u8g2.drawStr(0, 56, "Xtra:");
        u8g2.setCursor(45, 56);
        u8g2.print(laserTagIR.irPacketIn.extra);
        u8g2.sendBuffer();
        laserTagIR.infraredReceived = 0;
        while (laserTagIR.receiveIR() && laserTagIR.infraredReceived)  // Purge any buffered IR signals
          laserTagIR.infraredReceived = 0;
      }
    }
    currentMenuItem = topMenu;
    delay(200);
  }

  // check all gun types to see if we have a match
  for (auto &allGun: Weapons::allGuns) {
    if (strcmp(itemName, allGun->getName().c_str()) == 0) {
      Weapons::currentGun = allGun;
    }
  }
  for (auto &allGun: Weapons::allGodGuns) {
    if (strcmp(itemName, allGun->getName().c_str()) == 0) {
      Weapons::currentGun = allGun;
    }
  }
}

float batteryVoltage()  // return battery voltage
{
  int reading;
  reading = analogRead(34);
  float voltage;
  voltage = -0.000000000000016 * pow(reading, 4) + 0.000000000118171 * pow(reading, 3) -
            0.000000301211691 * pow(reading, 2) + 0.001109019271794 * reading + 0.034143524634089;
  return (voltage * 2) +
         0.33;  // multiply it by 2, as we have a divide-by-two voltage divider on battery voltage before it goes to ADC
  // The 0.33V compensates for the schottky diode drop
}

void packetRx()
// Called when radio packet received
{
  if (logging && ((FLASH_LOG_START + (loggedPackets * 16) + 16) < FLASH_LOG_END)) {
    datapacketstruct dp;
    memcpy(&dp, &incomingRadioPacket, sizeof(dp));
    dp.crc = millis() /
             1000;  // We store the secs since startup in the crc field just for the purpose of logging - crc must be valid to trigger packetRx
    ESP.flashWrite(FLASH_LOG_START + (loggedPackets * 16), (uint32_t *) &dp, 16);
    loggedPackets++;
  }

  if (gameStageChanged)  // set in Radio.cpp when new game stage received
  {
    randomSeed(millis());
    gameStageChanged = 0;
    if (laserConfig.isRunning) {
      secsGameStarted = millis() / 1000;
      lastTeamWon = -1;
      mineTriggered = 0;
      Weapons::currentGun->resetGun();
      health = 1;
    }
    if (laserConfig.inGame && laserConfig.isRunning)  // just moved to game running
    {
      health = 1;
      if (!countDownSound.Playing)
        DacAudio.Play(&countDownSound, false);
      laserTagIR.setPower(laserConfig.irPower);
    }
    Weapons::currentGun->setClips(laserConfig.ammo);
    if (!laserConfig.isRunning)  // just moved to game stopped
    {
      if (!gameOverSound.Playing) DacAudio.Play(&gameOverSound, false);
    }
  }
  if ((laserConfig.gameType == GAME_DOMINATION) && laserConfig.inGame && laserConfig.isRunning &&
      (laserConfig.winner != lastTeamWon)) {
    if ((laserConfig.winner == 1) && !redSound.Playing)
      DacAudio.Play(&redSound, false);
    else if ((laserConfig.winner == 0) && !greenSound.Playing)
      DacAudio.Play(&greenSound, false);
    lastTeamWon = laserConfig.winner;
  }
  if ((incomingRadioPacket.hitBy == laserConfig.unitNum) &&
      ((incomingRadioPacket.unitNum != whoILastHit) || (millis() > (lastStrikeMillis + 1000)))) {
    whoILastHit = incomingRadioPacket.unitNum;
    lastStrikeMillis = millis();
    laserConfig.strikes++;

    if (Weapons::currentGun->getName() == "LifeSteal") {
      laserConfig.lives++;
    }
    if (!hitSound.Playing) DacAudio.Play(&hitSound, false);

    whoILastHit = incomingRadioPacket.unitNum;

  }
  if ((laserConfig.gameType == GAME_AUTONUMBER) && (assignedUnitNumber == -1) && laserConfig.canPlay) {
    assignedUnitNumber = laserConfig.sequenceNumber;
    preferences.begin("lt", false);
    preferences.putUChar("unitNum", assignedUnitNumber);
    preferences.end();
    laserConfig.unitNum = assignedUnitNumber;
    laserConfig.sequenceNumber++;
  }
}

void setup() {
  int i;

  Serial.begin(9600);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(IR_OUT, OUTPUT);
  pinMode(IR_IN, INPUT);
  pinMode(TRIGGER, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(MASTER_IN, INPUT_PULLUP);
  pinMode(MASTER_OUT, OUTPUT);
  digitalWrite(MASTER_OUT, LOW);

  masterMode = 0;

  StartupScreen::startupScreen(&u8g2);

  preferences.begin("lt", false);
  //preferences.putUChar("unitNum",4);
  //preferences.putUChar("gameNum",0);
  unsigned char unitNum = preferences.getUChar("unitNum", 0);
  unsigned char gameNum = preferences.getUChar("gameNum", 0);
  unsigned char playing = preferences.getUChar("playing", 1);
  unsigned char irPower = preferences.getUChar("irPower", 3);
  preferences.end();

  mainMenu = new MenuItem("Main Menu");
  if (masterMode == 1) {
    mainMenu->addItem("Main Menu", "Game Control");
    mainMenu->addItem("Game Control", "CTF");
    mainMenu->addItem("Game Control", "Death Match");
    mainMenu->addItem("Game Control", "Team Death");
    mainMenu->addItem("Game Control", "Domination");
    mainMenu->addItem("Game Control", "Stop Game");
    mainMenu->addItem("Game Control", "Join for 10s");
    mainMenu->addItem("Game Control", "Trgt Practice");
    mainMenu->addItem("Game Control", "Back");
    mainMenu->addItem("Main Menu", "Game Setup");
    mainMenu->addItem("Game Setup", "Lives", 1, 5, 3);
    mainMenu->addItem("Game Setup", "Ammo", 0, 30, 9, 1);
    mainMenu->addItem("Game Setup", "Frndly Fire", 0, 1, 0);
    mainMenu->addItem("Game Setup", "IR Power", 0, 3, irPower);
    mainMenu->addItem("Game Setup", "I'm Playing", 0, 1, playing);
    mainMenu->addItem("Game Setup", "Back");
    mainMenu->addItem("Main Menu", "Beacon Setup");
    mainMenu->addItem("Beacon Setup", "Tx:Red Base");
    mainMenu->addItem("Beacon Setup", "Tx:Blue Base");
    mainMenu->addItem("Beacon Setup", "Tx:Domination");
    mainMenu->addItem("Beacon Setup", "Tx:Health");
    mainMenu->addItem("Beacon Setup", "Tx:Ammo");
    mainMenu->addItem("Beacon Setup", "Tx:Red Mine");
    mainMenu->addItem("Beacon Setup", "Tx:Blue Mine");
    mainMenu->addItem("Beacon Setup", "Back");
    mainMenu->addItem("Main Menu", "Tools");
    mainMenu->addItem("Tools", "Unit Num", 0, 127, unitNum);
    mainMenu->addItem("Tools", "Game Num", 0, 127, gameNum);
    mainMenu->addItem("Tools", "IR Test");
    mainMenu->addItem("Tools", "Autonumber");
    mainMenu->addItem("Tools", "Logging", 0, 1, 0);
    mainMenu->addItem("Tools", "Delete Log");
    mainMenu->addItem("Tools", "Dump Log");
    mainMenu->addItem("Tools", "Back");

  }
  mainMenu->addItem("Main Menu", "Gun Type");

  // iterate through all guns array
  for (auto &allGun: Weapons::allGuns) {
    mainMenu->addItem("Gun Type", allGun->getName().c_str());
  }
  if (masterMode) {
    for (auto &allGun: Weapons::allGodGuns) {
      mainMenu->addItem("Gun Type", allGun->getName().c_str());
    }
  }

  mainMenu->addItem("Gun Type", "Back");
  mainMenu->addItem("Main Menu", "Team");
  mainMenu->addItem("Team", "Red");
  mainMenu->addItem("Team", "Green");
  mainMenu->addItem("Team", "Back");
  mainMenu->addItem("Main Menu", "Back");


  mainMenu->activate(menuChange);

  DacAudio.DacVolume = 100;

  laserTagIR.init(irPinIn, irPinOut);

  // oled setup

  if (0 && (digitalRead(TRIGGER) == LOW))  // Go into test mode if trigger held when starting
  {
    for (i = 1; i <= 4; i++)  // Flash green/red/yellow LEDs 4 times in order
    {
      digitalWrite(GREEN, HIGH);
      delay(250);
      digitalWrite(GREEN, LOW);
      digitalWrite(RED, HIGH);
      delay(250);
      digitalWrite(RED, LOW);
      digitalWrite(YELLOW, HIGH);
      delay(250);
      digitalWrite(YELLOW, LOW);
    }
    u8g2.setFont(mainFont);
    u8g2.clearBuffer();  // clear the internal memory
    u8g2.drawStr(0, 16, "Test Mode");
    u8g2.sendBuffer();  // transfer internal memory to the display
    testMode = 1;
  } else {
    ledcSetup(0, 4, 16);  // use ledc channel 0 for green led, 4 hz, 16 bit
    ledcAttachPin(GREEN, 0);
    ledcSetup(1, 4, 16);
    ledcAttachPin(RED, 1);
    ledcWrite(0, 32000);  // 32000 is about half of 16-bit max value, so will be 50% duty cycle at 4hz
    ledcWrite(1, 32000);

    setupRadio(wifiChannel, packetRx);  // initialise the radio subsystem
    laserConfig.unitNum = unitNum;
    laserConfig.controllerNum = gameNum;
    laserConfig.canPlay = playing;
    laserConfig.irPower = irPower;

    if (batteryVoltage() < 3.75) {
      if (lowBatterySound.Playing == false) DacAudio.Play(&lowBatterySound, false);
    }
  }
}

void standardDisplay() {
  u8g2.setFont(mainFont);  // menu system may use different fonts to us
  u8g2.clearBuffer();      // clear the internal memory
  char buffer[20];
  if (!laserConfig.isRunning) {
    if ((laserConfig.gameType == GAME_CTF) || (laserConfig.gameType == GAME_TEAM_DEATH) ||
        (laserConfig.gameType == GAME_DOMINATION)) {
      if (laserConfig.winner == 1)
        strcpy(buffer, "Red Won");
      else if (laserConfig.winner == 0)
        strcpy(buffer, "Green Won");
      else
        strcpy(buffer, "Nobody Won");
    } else if (laserConfig.gameType == GAME_DEATH_MATCH) {
      if (laserConfig.winner >= 0)
        sprintf(buffer, "%d Won!", laserConfig.winner);
      else
        strcpy(buffer, "Nobody Won");
    } else if (laserConfig.gameType == GAME_AUTONUMBER)
      strcpy(buffer, "Autonumber");
    else
      strcpy(buffer, "Stopped");
  } else if (laserConfig.isRunning) {
    if (millis() < nextAliveMillis) {
      strcpy(buffer, "Respawning");
    } else if (laserConfig.gameType == GAME_DEATH_MATCH)
      strcpy(buffer, "Death Match");
    else if (laserConfig.gameType == GAME_TEAM_DEATH)
      strcpy(buffer, "Team Death");
    else if (laserConfig.gameType == GAME_CTF)
      strcpy(buffer, "CTF");
    else if (laserConfig.gameType == GAME_TARGET)
      strcpy(buffer, "TRGT PRACTICE");
    else if (laserConfig.gameType == GAME_DOMINATION) {
      if (laserConfig.winner == 1)
        strcpy(buffer, "Red");
      else if (laserConfig.winner == 0)
        strcpy(buffer, "Green");
      else
        strcpy(buffer, "Domination");
    }
  } else
    strcpy(buffer, "Joining");

  unsigned char textWidth;
  textWidth = u8g2.getStrWidth(buffer);
  u8g2.setCursor((128 - textWidth) >> 1, 16);
  u8g2.print(buffer);

//PRINT KILLS
  u8g2.drawXBM(0, 22, 16, 16, killIcon);
  u8g2.setCursor(18, 38);
  u8g2.print(laserConfig.strikes);

  u8g2.drawXBM(43, 22, 16, 16, person);
  strcpy(buffer, u8g2_u8toa((uint8_t) laserConfig.lives, 1));
  u8g2.drawStr(60, 38, buffer);
  u8g2.drawXBM(78, 22, 9, 16, bullet);
  u8g2.setCursor(88, 40);

  if (Weapons::currentGun->getShotsInClip() == 0 && Weapons::currentGun->getClips() > 0) {
    u8g2.print("RLD");
  } else {
    u8g2.print(Weapons::currentGun->getShotsInClip());
    u8g2.print("/");
    if (laserConfig.ammo == 0 || laserConfig.ammo == 1023) // infinite ammo
      u8g2.print("-");
    else
      u8g2.print(Weapons::currentGun->getClips());
  }

  u8g2.setFont(u8g2_font_7x13B_mr);
  unsigned char mins, secs, gamemin, gamesec;
  mins = secsSinceStartup / 60;
  secs = secsSinceStartup % 60;
  if (laserConfig.isRunning && (laserConfig.inGame || iAmGameController)) {
    gamemin = (secsSinceStartup - secsGameStarted) / 60;
    gamesec = (secsSinceStartup - secsGameStarted) % 60;
  } else {
    gamemin = 0;
    gamesec = 0;
  }
  sprintf(buffer, "%02d:%02d %02d:%02d", mins, secs, gamemin, gamesec);
  textWidth = u8g2.getStrWidth(buffer);
  u8g2.setCursor(0, 52);
  u8g2.print(Weapons::currentGun->getName().c_str());
  u8g2.print(" Unit:");
  u8g2.print(laserConfig.unitNum);

  u8g2.setCursor(0, 64);

  u8g2.print("Health: ");
  int healthAsInt = min(100, max(0, (int) round(health * 100)));
  u8g2.print(healthAsInt);
  u8g2.drawXBM(111, 54, 16, 10, batteryIcons + (20 * laserConfig.battery));
  u8g2.sendBuffer();  // transfer internal memory to the display
}

void debugDisplay(int page) {
  char buffer[20];
  char firstUnit = 0;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x13_t_symbols);
  char i;
  firstUnit = (page - 1) * 5;

  for (i = 0; i <= 4; i++) {
    if ((firstUnit + i) <= 127) {
      if (unitMonitor[firstUnit + i].inGame)
        u8g2.drawGlyph(0, 12 + (13 * i), 9632);
      else
        u8g2.drawGlyph(0, 12 + (13 * i), 9633);
      u8g2.setCursor(10, 12 + (13 * i));
      sprintf(buffer, "%03d %c %dL", firstUnit + i, unitMonitor[firstUnit + i].team ? 'R' : 'G',
              unitMonitor[firstUnit + i].lives);
      u8g2.print(buffer);
      if (unitMonitor[firstUnit + i].secsLastSeen > 0) {
        u8g2.setCursor(70, 12 + (13 * i));
        u8g2.print((millis() / 1000) - unitMonitor[firstUnit + i].secsLastSeen);
      }
      u8g2.drawXBM(111, 2 + (13 * i), 16, 10, batteryIcons + (20 * unitMonitor[firstUnit + i].battery));
    }
  }
  u8g2.sendBuffer();
}

void determineWhosAlive() {
  int i;
  lastManStanding = -1;
  unitsAlive = 0;
  greenAlive = 0;
  redAlive = 0;
  redOn = 0;
  greenOn = 0;
  for (i = 0; i <= 127; i++)
    if (unitMonitor[i].secsLastSeen && ((secsSinceStartup - unitMonitor[i].secsLastSeen) < 60)) {
      if (unitMonitor[i].team)
        redOn++;
      else
        greenOn++;
      if ((unitMonitor[i].lives > 0) && unitMonitor[i].inGame) {
        unitsAlive++;
        lastManStanding = i;
        if (unitMonitor[i].team)
          redAlive++;
        else
          greenAlive++;
      }
    }
}

void loop() {
  char secTickerTriggered = 0;
  unsigned char alive;
  int encoderPulses;

  if (testMode) {
    DacAudio.FillBuffer();    // Fill the sound buffer with data (just needed to call every time through the loop if you want to generate sounds using the sound library)
    digitalWrite(YELLOW, 0);  // Yellow LED off
    int i = readEncoder();
    if (i) {
      encoderPosition = encoderPosition + i;
      u8g2.clearBuffer();  // clear the internal memory
      u8g2.drawStr(0, 16, "Test Mode");
      u8g2.setCursor(96, 16);
      u8g2.print(encoderPosition);
      u8g2.sendBuffer();  // transfer internal memory to the display
    }

    if ((digitalRead(TRIGGER) == LOW))  // TRIGGER -> show 'T', beep, flash green leds and send IR command
    {
      u8g2.sendBuffer();  // transfer internal memory to the display

      // Set different sounds for different guns
      DacAudio.Play(Weapons::currentGun->getFireSound(), false);
      digitalWrite(GREEN, HIGH);
      laserTagIR.sendIR(1, 0, IR_CMD_TEST, 0);
      digitalWrite(GREEN, LOW);
    }
    if ((digitalRead(ENC_SW) == LOW)) {
      u8g2.drawStr(20, 40, "Pressed");
      u8g2.sendBuffer();  // transfer internal memory to the display
      delay(250);
      u8g2.clearBuffer();
      u8g2.drawStr(0, 16, "Test Mode");
      u8g2.sendBuffer();  // transfer internal memory to the display
    }
    // Now play the 'check' sound if we've received our own transmision ...
    laserTagIR.receiveIR();
    if (laserTagIR.infraredReceived) {
      if (laserTagIR.crcValid && laserTagIR.irPacketIn.control && laserTagIR.irPacketIn.data == IR_CMD_TEST)
        if (checkSound.Playing == false) DacAudio.Play(&checkSound, false);
      laserTagIR.infraredReceived = 0;
    }
    return;
  }


  secsSinceStartup = millis() / 1000;

  if ((digitalRead(TRIGGER) == HIGH))  //masterMode &&
    encoderPulses = menuService();

  if (!laserConfig.canPlay && (encoderPulses != 0)) {
    if ((encoderPulses < 0) & (page > 0))
      page--;
    else if ((encoderPulses > 0) & (page < 26))
      page++;
    redrawScreen = 1;
  }


  // alive is a flag set if note temporarily dead (nextAliveMillis set in the future) and we are not permanently dead (we have some lives) and we are running
  alive = (millis() > nextAliveMillis) && laserConfig.lives && laserConfig.inGame && laserConfig.isRunning;

  determineWhosAlive();

  if (iAmGameController && (laserConfig.gameType != GAME_TARGET) && laserConfig.canJoin &&
      (millis() >= stopJoiningMillis))
    controlGame(laserConfig.unitNum, laserConfig.gameType, 1, 0,
                laserConfig.winner);  // prevent joining game 10 seconds in

  if (iAmGameController && ((laserConfig.gameType == GAME_DEATH_MATCH) || (laserConfig.gameType == GAME_TEAM_DEATH))
      && laserConfig.isRunning && (secsSinceStartup > (secsGameStarted + 10))) {  // Check for winner in death matches
    if ((laserConfig.gameType == GAME_DEATH_MATCH) && unitsAlive == 1)
      controlGame(laserConfig.unitNum, laserConfig.gameType, 0, 0, lastManStanding);  // stopped, we have a winner!
    else if (laserConfig.gameType == GAME_TEAM_DEATH) {
      if (greenAlive && !redAlive)
        controlGame(laserConfig.unitNum, laserConfig.gameType, 0, 0, 0);  // Blue won!
      else if (redAlive && !greenAlive)
        controlGame(laserConfig.unitNum, laserConfig.gameType, 0, 0, 1);  // Red won!
    }
  }
  if (iAmGameController && (laserConfig.gameType == GAME_DOMINATION) && laserConfig.isRunning &&
      (secsSinceStartup > (secsGameStarted + 180))) {
    if (redDominated > blueDominated)
      controlGame(laserConfig.unitNum, laserConfig.gameType, 0, 0, 1);  // Red won!
    else
      controlGame(laserConfig.unitNum, laserConfig.gameType, 0, 0, 0);  // Blue won!
  }

  // this block handles red and green leds
  if (!laserConfig.canPlay) {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  } else if (laserConfig.team == 0) {
    if (alive) {
      ledcWrite(0, 65535);  // full duty cycle so should be full on
    } else if (!laserConfig.isRunning || (laserConfig.lives == 0)) {
      // Flash quickly if not running, or running and now zero lives left
      if (round(ledcReadFreq(0)) != 4)
        ledcSetup(0, 4, 16);  // set frequency to 4hz
      ledcWrite(0, 32000);    // 32000 is about half of 16-bit max so will be 50% duty cycle at 4hz
    } else {
      // Flash slowly as only temporarily dead
      if (round(ledcReadFreq(0)) != 2)
        ledcSetup(0, 2, 16);
      ledcWrite(0, 32000);  // 50% duty cycle at 2hz will be a slow flash
    }
    ledcWrite(1, 0);  // red off
  } else {
    if (alive) {
      ledcWrite(1, 65535);  // full duty cycle so should be full on
    } else if (!laserConfig.isRunning || (laserConfig.lives == 0)) {
      // Flash quickly if not running, or running and now zero lives left
      if (round(ledcReadFreq(1)) != 4)
        ledcSetup(1, 4, 16);
      ledcWrite(1, 32000);
    } else {
      // Flash slowly as only temporarily dead
      if (round(ledcReadFreq(1)) != 2)
        ledcSetup(1, 2, 16);
      ledcWrite(1, 32000);
    }
    ledcWrite(0, 0);  // blue off
  }

  if (millis() > nextTransmitMillis) {
    // Send regular status update over the radio
    laserConfig.hitBy = -1;
    sendPacket(&laserConfig);
    nextTransmitMillis = millis() + 4900 + random(0,
                                                  201);  // Transmit roughly every 5 seconds, but add some random variation to avoid regular collisions
  }

  // Some housekeeping to do every second - e.g. checking current domination winner, reading battery voltage
  if (secsSinceStartup > lastSecsLoop) {
    lastSecsLoop = secsSinceStartup;
    secTickerTriggered = 1;  // flag used to do some other things regularly below
    if (laserConfig.winner)
      redDominated++;
    else
      blueDominated++;

    float voltage = batteryVoltage();
    if (voltage > 4.4)
      laserConfig.battery = 5;
    else if (voltage > 4.02)
      laserConfig.battery = 4;
    else if (voltage > 3.87)
      laserConfig.battery = 3;
    else if (voltage > 3.80)
      laserConfig.battery = 2;
    else if (voltage > 3.73)
      laserConfig.battery = 1;
    else
      laserConfig.battery = 0;
  }

  // Now update display once a second or when a redraw is needed
  if ((secTickerTriggered || redrawScreen) && !mainMenu->isActive()) {
    if (!laserConfig.canPlay && (page > 0))
      debugDisplay(page);
    else
      standardDisplay();
    redrawScreen = 0;
  }

  DacAudio.FillBuffer();  // Fill the sound buffer with data (just needed to call every time through the loop if you want to generate sounds using the sound library)

  // Alternate team ...
  if (digitalRead(TRIGGER) == HIGH && !notChecking) {
    notChecking = true;
  }
  if (digitalRead(TRIGGER) == LOW) {
    notChecking = false;
  }



  // Handle firing:
  if (Weapons::currentGun->canFire() && (digitalRead(TRIGGER) == LOW) && laserConfig.isRunning && laserConfig.inGame &&
      laserConfig.lives && (millis() > nextAliveMillis)) {
    // if been at least 500ms since last fired, am holding trigger, game running, we have ammo and lives and not temporarily dead ...
    digitalWrite(YELLOW, HIGH);  // muzzle flash
    Weapons::currentGun->fireGun();

    redrawScreen = 1;
    yellowFlashMillis = millis();
    DacAudio.Play(Weapons::currentGun->getFireSound(), false);
    if (Weapons::currentGun->getName() == "Healer") {
      sendIRControl(IR_CMD_HEALTH);
    } else {
      laserTagIR.sendIR(0, laserConfig.team, laserConfig.unitNum,
                        Weapons::currentGun->getDamageAsExtra());  // No control mode+team+unitNum +no extra bits
    }

    // Now ignore any IR received when sent    delay(30); // 30ms should be enough to wait until IR sent
    laserTagIR.receiveIR();
    laserTagIR.infraredReceived = 0;
  }

  // Turns yellow LED off
  if (yellowFlashMillis + 100 < millis())
    digitalWrite(YELLOW, LOW);

  Weapons::currentGun->poll();



  // Process any IR received signals ...
  laserTagIR.receiveIR();
  if (laserTagIR.infraredReceived) {
    if (laserTagIR.crcValid && laserTagIR.irPacketIn.control && laserTagIR.irPacketIn.data == infraredCMDExpected) {
      if (!checkSound.Playing) DacAudio.Play(&checkSound, false);
      infraredCMDExpected = -1;
    }
    if (laserTagIR.irPacketIn.control == 0) {
      lastIR[0] = (laserTagIR.irPacketIn.team == 0) ? 'B' : 'R';
      lastIR[1] = 'F';
    } else {
      switch (laserTagIR.irPacketIn.data) {
        case IR_CMD_BLUE_BASE:
          lastIR[0] = 'B';
          lastIR[1] = 'B';
          break;
        case IR_CMD_RED_BASE:
          lastIR[0] = 'R';
          lastIR[1] = 'B';
          break;
        case IR_CMD_DOMINATION:
          lastIR[0] = 'D';
          lastIR[1] = 'O';
          break;
        case IR_CMD_HEALTH:
          lastIR[0] = 'H';
          lastIR[1] = 'L';
          break;
        case IR_CMD_AMMO:
          lastIR[0] = 'A';
          lastIR[1] = 'M';
          break;
        case IR_CMD_BLUE_MINE:
          lastIR[0] = 'B';
          lastIR[1] = 'M';
          break;
        case IR_CMD_RED_MINE:
          lastIR[0] = 'R';
          lastIR[1] = 'M';
          break;
      }
    }
    if (!laserTagIR.crcValid) {  // Change to lowercase if bad crc
      lastIR[0] += 32;
      lastIR[1] += 32;
    }
    if (laserTagIR.crcValid && laserConfig.isRunning && laserConfig.inGame)  // if valid reception and we're running ...
    {
      // Check if we're hit by a gun:
      if ((laserTagIR.irPacketIn.control == 0) && laserConfig.lives &&
          (millis() > nextAliveMillis))  // coming from a gun and we've got lives and we're not temporarily dead ...
      {
        if (((laserTagIR.irPacketIn.team != laserConfig.team) || laserConfig.friendlyFire) &&
            laserTagIR.irPacketIn.data !=
            laserConfig.unitNum)  // shot has come from opposite team or friendly fire mode is on
        {
          if (laserConfig.gameType != GAME_TARGET && Weapons::currentGun->getName() != "GodMode") {
            int damageAsExtra = laserTagIR.irPacketIn.extra;
            float damageAsFloat = (float) (7.0 - (float) damageAsExtra) / (float) 7.0;
            health -= damageAsFloat;


            laserConfig.hitBy = laserTagIR.irPacketIn.data;
            sendPacket(&laserConfig);
            delay(random(1, 4));
            sendPacket(&laserConfig);
            delay(random(1, 4));
            sendPacket(&laserConfig);
            laserConfig.hitBy = -1;
            if (!ughSound.Playing) DacAudio.Play(&ughSound, false);
            if (health <= 0) {
              health = 1;
              laserConfig.lives--;
              nextAliveMillis = millis() + TEMP_DEAD_MILLIS;  // stop firing/being fired at for 10 seconds
              if (!flatlineSound.Playing) DacAudio.Play(&flatlineSound, false);
              laserConfig.hitBy = laserTagIR.irPacketIn.data;
              sendPacket(&laserConfig);
              delay(random(1, 4));
              sendPacket(&laserConfig);
              delay(random(1, 4));
              sendPacket(&laserConfig);
              laserConfig.hitBy = -1;
            }
            standardDisplay();
          }

        }
      } else if ((laserTagIR.irPacketIn.control == 1)
                 && laserConfig.isRunning && laserConfig.inGame)  // if received a control message and am running
      {
        if (laserTagIR.irPacketIn.data == IR_CMD_BLUE_BASE) {
          if ((laserConfig.team == 0) && (!laserConfig.lives))  // If I'm blue and no lives, resurrect me:
          {
            laserConfig.lives++;
            nextAliveMillis = millis() + TEMP_DEAD_MILLIS;  // don't resurrect for 10 secs
            if (!powerUpSound.Playing) DacAudio.Play(&powerUpSound, false);
          } else if ((laserConfig.team == 1) && (laserConfig.lives) &&
                     (laserConfig.gameType == GAME_CTF))  // If I'm red and alive, I've captured the flag!
            controlGame(laserConfig.controllerNum, laserConfig.gameType, 0, 0, 1);                          // red won
        } else if (laserTagIR.irPacketIn.data == IR_CMD_RED_BASE) {
          if ((laserConfig.team == 1) && (!laserConfig.lives))  // If I'm red and no lives, resurrect me:
          {
            laserConfig.lives++;
            nextAliveMillis = millis() +
                              TEMP_DEAD_MILLIS;                                                  // don't resurrect for 10 secs
          } else if ((laserConfig.team == 0) && (laserConfig.lives) &&
                     (laserConfig.gameType == GAME_CTF))  // If I'm blue and alive, I've captured the flag!
            controlGame(laserConfig.controllerNum, laserConfig.gameType, 0, 0, 0);                          // blue won
        } else if ((laserTagIR.irPacketIn.data == IR_CMD_HEALTH) && (laserConfig.lives < laserConfig.initLives) &&
                   (millis() > nextPickupMillis)) {
          laserConfig.lives++;
          if (!pickupSound.Playing) DacAudio.Play(&pickupSound, false);
          nextPickupMillis = millis() + 3000;  // don't pickup again for 10 secs
        } else if ((laserTagIR.irPacketIn.data == IR_CMD_DOMINATION) && laserConfig.lives)
          controlGame(laserConfig.controllerNum, laserConfig.gameType, 1, 0, laserConfig.team);
        else if ((laserTagIR.irPacketIn.data == IR_CMD_AMMO) && (laserConfig.ammo < laserConfig.initAmmo) &&
                 (millis() > nextPickupMillis)) {
          laserConfig.ammo = ((laserConfig.ammo + 50) > laserConfig.initAmmo ? laserConfig.initAmmo : laserConfig.ammo +
                                                                                                      50);
          if (!pickupSound.Playing) DacAudio.Play(&pickupSound, false);
          nextPickupMillis = millis() + 3000;  // don't pickup again for 10 secs
        } else if ((laserTagIR.irPacketIn.data == IR_CMD_BLUE_MINE) && laserConfig.team && laserConfig.lives &&
                   (millis() > nextAliveMillis) && !mineTriggered) {
          laserConfig.lives--;
          nextAliveMillis = millis() + TEMP_DEAD_MILLIS;  // stop firing/being fired at for 10 seconds
          if (!explosionSound.Playing) DacAudio.Play(&explosionSound, false);
          mineTriggered = 1;
        } else if ((laserTagIR.irPacketIn.data == IR_CMD_RED_MINE) && !laserConfig.team && laserConfig.lives &&
                   (millis() > nextAliveMillis) && !mineTriggered) {
          laserConfig.lives--;
          nextAliveMillis = millis() + TEMP_DEAD_MILLIS;  // stop firing/being fired at for 10 seconds
          if (!explosionSound.Playing) DacAudio.Play(&explosionSound, false);
          mineTriggered = 1;
        }
      }
    }
    laserTagIR.infraredReceived = 0;
  }
}
