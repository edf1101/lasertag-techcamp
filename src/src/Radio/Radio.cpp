#include "Radio.h"
#include <WiFi.h>
#include <esp_now.h> // esp-now is a connectionless protocol that uses wifi frequencies but doesn't need an access point
#include <limits.h>

esp_now_peer_info_t slave;
uint8_t mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; // this is broadcase mac address
datapacketstruct incomingRadioPacket,laserConfig,masterController;
unitmonitorstruct unitMonitor[128];

void (*callBack)(void);

char gameStageChanged=0;

void clearMonitorTable()
{
  int i;
  for(i=0;i<=127; i++)
    unitMonitor[i].secsLastSeen = 0;
}

void processIncomingPacket()
{
  if(incomingRadioPacket.controllerNum==laserConfig.controllerNum) // Only process packet if in the correct game ...
  {
    unitMonitor[incomingRadioPacket.unitNum].team = incomingRadioPacket.team;
    unitMonitor[incomingRadioPacket.unitNum].lives = incomingRadioPacket.lives;
    unitMonitor[incomingRadioPacket.unitNum].ammo = incomingRadioPacket.ammo;
    unitMonitor[incomingRadioPacket.unitNum].inGame = incomingRadioPacket.inGame;
    unitMonitor[incomingRadioPacket.unitNum].battery = incomingRadioPacket.battery;
    unitMonitor[incomingRadioPacket.unitNum].secsLastSeen = millis()/1000;
    if(incomingRadioPacket.sequenceNumber>laserConfig.sequenceNumber)
    {
      if(incomingRadioPacket.isRunning && !laserConfig.isRunning)
        gameStageChanged = 1;
      if(incomingRadioPacket.isRunning && laserConfig.canPlay && !laserConfig.inGame && incomingRadioPacket.canJoin) 
      { // if newly running, and still able to join, set my lives and ammunition and put me in the game ...
        laserConfig.inGame = 1;
        laserConfig.lives = incomingRadioPacket.initLives;
        laserConfig.ammo = incomingRadioPacket.initAmmo;
        if(!laserConfig.ammo)
          laserConfig.ammo=1023; // infinite ammo
        laserConfig.strikes = 0; // I haven't hit anyone yet
      }
      if(!incomingRadioPacket.isRunning && laserConfig.isRunning)
      { // if now not running any more, stop my game 
        gameStageChanged = 1;
        laserConfig.inGame = 0;
      }
      laserConfig.sequenceNumber = incomingRadioPacket.sequenceNumber;
      laserConfig.gameType = incomingRadioPacket.gameType;
      laserConfig.isRunning = incomingRadioPacket.isRunning;
      laserConfig.canJoin = incomingRadioPacket.canJoin;
      laserConfig.winner = incomingRadioPacket.winner;
      laserConfig.friendlyFire = incomingRadioPacket.friendlyFire;
      laserConfig.initLives = incomingRadioPacket.initLives;
      laserConfig.initAmmo = incomingRadioPacket.initAmmo;
      laserConfig.irPower = incomingRadioPacket.irPower;
    }
    callBack();
  }
}

void OnDataRecv(const uint8_t *macaddr, const uint8_t *data, int len) { // Called when data packet received via radio
  FastCRC16 CRC16;
  uint16_t calccrc;

  if(len<=sizeof(datapacketstruct))
  {
    memcpy(&incomingRadioPacket, data, len);
    calccrc = CRC16.ccitt(((const uint8_t*)&incomingRadioPacket)+2, len-2);
    if(calccrc==incomingRadioPacket.crc)
      processIncomingPacket();
  }
}

void sendPacket(datapacketstruct *dps) // Sends packet via radio
{
  FastCRC16 CRC16;
  int8_t bytestosend;
  
  bytestosend=sizeof(datapacketstruct);
  dps->crc = CRC16.ccitt(((const uint8_t*)dps)+2, bytestosend-2);
  esp_now_send (mac, (const uint8_t*)dps, bytestosend);
  if(dps==&masterController) // if I'm sending a controller packet out, force us to process it as an incoming packet ourselves in case we're playing the game as well as controlling it ...
  {
    memcpy(&incomingRadioPacket, &masterController, sizeof(datapacketstruct));
    processIncomingPacket();
  }
}

void setupRadio(int channel,void (*thisCallBack)(void)) // Setup routine for radio (esp-now, which is a simple, quick protocol that doesn't go through a wifi point)
{
  uint64_t chipid; 
  chipid=ESP.getEfuseMac();
  laserConfig.unitNum = (uint16_t)(chipid>>40); // unit num taken from high byte of 6-byte chip address

  callBack = thisCallBack;
  
  WiFi.mode(WIFI_STA); // Station mode for esp-now 
  WiFi.disconnect(); // Conventional wifi not needed for esp-now

  delay(1500);
  //Serial.println ("Initializing ...");

  if (esp_now_init () == ESP_OK) {
    //Serial.println ("direct link init ok");
  } else {
    //Serial.println ("dl init failed");
    ESP.restart ();
  }

  esp_now_register_recv_cb(OnDataRecv);
  slave.channel = channel;
  slave.encrypt = 0; // no encryption
  int i;
  for(i=0; i<6; i++)
    slave.peer_addr[i] = mac[i];
  bool exists = esp_now_is_peer_exist((const uint8_t *)&slave.peer_addr);
  if ( exists) {
    //Serial.println("Already Paired");
  } else {
    esp_err_t addStatus = esp_now_add_peer(&slave);
    /*if (addStatus == ESP_OK) 
      Serial.println("Pair success");
    else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT)
      Serial.println("ESPNOW Not Init");
    else if (addStatus == ESP_ERR_ESPNOW_ARG)
      Serial.println("Invalid Argument");
    else if (addStatus == ESP_ERR_ESPNOW_FULL)
      Serial.println("Peer list full");
    else if (addStatus == ESP_ERR_ESPNOW_NO_MEM)
      Serial.println("Out of memory");
    else if (addStatus == ESP_ERR_ESPNOW_EXIST)
      Serial.println("Peer Exists");
    else
      Serial.println("Not sure what happened");*/
  }
  laserConfig.winner = -1;
  laserConfig.hitBy = -1;
}
