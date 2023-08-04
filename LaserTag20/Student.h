#pragma once
#include <U8g2lib.h>
#include <string.h>
#include "Radio.h"
#include "SoundData.h"
#include "XT_DAC_Audio.h";
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern uint8_t damage;
extern datapacketstruct laserConfig;
extern const int TRIGGER;


// Useful icon editor: https://xbm.jazzychad.net/
static unsigned char bullet[] = {
  0x10, 0xfe, 0x28, 0xfe, 0x28, 0xfe, 0x44, 0xfe, 0x44, 0xfe, 0x44, 0xfe,
  0xfe, 0xfe, 0x82, 0xfe, 0x82, 0xfe, 0x82, 0xfe, 0x82, 0xfe, 0x82, 0xfe,
  0x82, 0xfe, 0x7c, 0xfe, 0x82, 0xfe, 0xfe, 0xfe
};
static unsigned char person[] = {
  0xC0, 0x03, 0xE0, 0x07, 0xE0, 0x07, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F,
  0xE0, 0x07, 0xE0, 0x07, 0xC0, 0x03, 0x00, 0x00, 0xE0, 0x07, 0xFC, 0x3F,
  0xFE, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static unsigned char infinityImage[] = {
  0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x1c, 0x9c, 0x22, 0xa2, 0x41, 0xc1,
  0x81, 0xc0, 0x41, 0xc1, 0x22, 0xa2, 0x1c, 0x9c, 0x00, 0x80, 0x00, 0x80,
  0x00, 0x80
};

static unsigned char batteryIcons[] = {  // Contains 6 icons from flat to 'charging'
  0xff, 0x7f, 0x01, 0x40, 0x01, 0xc0, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xc0, 0x01, 0x40, 0xff, 0x7f,
  0xff, 0x7f, 0x01, 0x40, 0x0d, 0xc0, 0x0d, 0x80, 0x0d, 0x80, 0x0d, 0x80, 0x0d, 0x80, 0x0d, 0xc0, 0x01, 0x40, 0xff, 0x7f,
  0xff, 0x7f, 0x01, 0x40, 0x6d, 0xc0, 0x6d, 0x80, 0x6d, 0x80, 0x6d, 0x80, 0x6d, 0x80, 0x6d, 0xc0, 0x01, 0x40, 0xff, 0x7f,
  0xff, 0x7f, 0x01, 0x40, 0x6d, 0xc3, 0x6d, 0x83, 0x6d, 0x83, 0x6d, 0x83, 0x6d, 0x83, 0x6d, 0xc3, 0x01, 0x40, 0xff, 0x7f,
  0xff, 0x7f, 0x01, 0x40, 0x6d, 0xdb, 0x6d, 0x9b, 0x6d, 0x9b, 0x6d, 0x9b, 0x6d, 0x9b, 0x6d, 0xdb, 0x01, 0x40, 0xff, 0x7f,
  0xff, 0x7f, 0x01, 0x40, 0x81, 0xc1, 0x81, 0x87, 0xfd, 0x9f, 0xf1, 0x80, 0xc1, 0x80, 0x01, 0xc0, 0x01, 0x40, 0xff, 0x7f
};


// Gun class
class Gun {
public:
  // Gun attributes
  String gunName;
  //unsigned char* fireSoundWav;
  int fireSpeed;
  bool automatic;
  uint8_t damage;
  int clipSize;
  int burstSize = 0;
  int burstDelay = 500;
  int reloadTime = 2000;

  // Gun counters
  int lastBurstMillis = millis();
  int reloadStartMillis = millis();
  int lastFireMillis = millis();
  int shotsLeftInClip;
  int shotsLeftInBurst = burstSize;
  bool triggerHeld = false;
  int clips=9;


  // Constructor 1 - just base params
  Gun(String incGunName, int incFireSpeed, int incClipSize, bool incAutomatic, uint8_t incDamage) {
    gunName = incGunName;
    fireSpeed = incFireSpeed;
    clipSize = incClipSize;
    shotsLeftInClip = clipSize;
    automatic = incAutomatic;
    damage = incDamage;
    burstSize=0;
  }

  // Constructor 2 - base params + burst
  Gun(String incGunName, int incFireSpeed, int incClipSize, bool incAutomatic, uint8_t incDamage, int incBurstSize, int incBurstDelay) {
    gunName = incGunName;
    fireSpeed = incFireSpeed;
    clipSize = incClipSize;
    shotsLeftInClip = clipSize;
    automatic = incAutomatic;
    damage = incDamage;
    burstSize = incBurstSize;
    shotsLeftInBurst = burstSize;
    burstDelay = incBurstDelay;
  }

  // Constructor 3 - base params + reload time
  Gun(String incGunName, int incFireSpeed, int incClipSize, bool incAutomatic, uint8_t incDamage, int incReloadTime) {
    gunName = incGunName;
    fireSpeed = incFireSpeed;
    clipSize = incClipSize;
    shotsLeftInClip = clipSize;
    automatic = incAutomatic;
    damage = incDamage;
    reloadTime = incReloadTime;
     burstSize=0;
  }

  // Constructor 4 - base params + burst + reload time
  Gun(String incGunName, int incFireSpeed, int incClipSize, bool incAutomatic, uint8_t incDamage, int incBurstSize, int incBurstDelay, int incReloadTime) {
    gunName = incGunName;
    fireSpeed = incFireSpeed;
    clipSize = incClipSize;
    shotsLeftInClip = clipSize;
    automatic = incAutomatic;
    damage = incDamage;
    burstSize = incBurstSize;
    shotsLeftInBurst = burstSize;
    burstDelay = incBurstDelay;
    reloadTime = incReloadTime;
  }

  void reload() {
    reloadStartMillis = millis();
    laserConfig.ammo -= clipSize;
    shotsLeftInClip = clipSize;
  }

  bool canFire() {
    if (shotsLeftInClip && (triggerHeld == false || automatic == true || burstSize != 0) && !(shotsLeftInBurst == burstSize && burstSize != 0 && triggerHeld == true) && (lastBurstMillis + burstDelay < millis() || burstSize==0) && (reloadStartMillis + reloadTime < millis()) && (lastFireMillis + fireSpeed < millis())) {
      return true;
    } else {
      return false;
    }
  }

  void fire() {

    shotsLeftInClip--;
    if (burstSize != 0) {
      shotsLeftInBurst--;
    }
    if (shotsLeftInBurst <= 0) {
      shotsLeftInBurst = burstSize;
      lastBurstMillis = millis();
    }

    if (shotsLeftInClip <= 0) {
      shotsLeftInClip = clipSize;
      clips--;
    }

    lastFireMillis = millis();
    triggerHeld = true;
  }

  void updateCounters() {
    if (digitalRead(TRIGGER) == HIGH)
      triggerHeld = false;
  }
};



// Guns!
static Gun P90("P-90", 100, 15, false, 1, 3, 500);
static Gun AWP("AWP", 1000, 7, false, 2);
static Gun AR("AR", 250, 16, true, 2);

// Gun properties
// Set currentGun to the gun you would like to be the default
static Gun currentGun = P90;

void startupScreen();
