#ifndef Radio_h
#define Radio_h

#include "FastCRC.h"
#include <limits.h>

enum GAME_TYPE {GAME_NONE,GAME_DEATH_MATCH,GAME_CTF,GAME_DOMINATION,GAME_TEAM_DEATH,GAME_TARGET,GAME_AUTONUMBER};

struct datapacketstruct {
  uint16_t crc:16;
  uint8_t unitNum=0; // My ID 
  uint8_t controllerNum=0; // ID of game controller
  int16_t sequenceNumber=0; // Incremented by one each time something changes in the game
  enum GAME_TYPE gameType:6;
  uint8_t isRunning:1;
  uint8_t canJoin:1;
  int8_t winner=-1; // -1 if no team won, 0=blue team, 1=red team, or unit number
  // following fields are used to setup params for game_start
  uint8_t friendlyFire:1;
  uint8_t initLives:3;
  uint16_t initAmmo:10;
  uint8_t irPower:2; // infrared power control
  // used to return params from players:
  uint8_t team:1;
  uint8_t lives:3;
  uint16_t ammo:10;
  uint8_t canPlay:1; // true if this unit CAN play the game (might be turned off if controller)
  uint8_t inGame:1; // true if this unit IS in the game
  int8_t hitBy:8; // unit number that has just hit me (reset after 5 secs)
  uint8_t strikes=0; // number of strikes I've made
  uint8_t battery:3; // battery level from 0-4
} __attribute__ ((packed));

struct unitmonitorstruct {
  uint8_t team:1;
  uint8_t lives:3;
  uint16_t ammo:10;
  uint8_t inGame:1;
  uint8_t battery:3;
  uint16_t secsLastSeen:14;
} __attribute__ ((packed));

void setupRadio(int channel,void (*thisCallBack)(void));
void sendPacket(datapacketstruct *dps);
void clearMonitorTable();

extern datapacketstruct incomingRadioPacket,laserConfig,masterController;
extern char gameStageChanged;
extern unitmonitorstruct unitMonitor[128];

#endif
