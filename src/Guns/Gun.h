#ifndef LASERTAG_TECHCAMP_GUN_H
#define LASERTAG_TECHCAMP_GUN_H

#include "string"
#include "Arduino.h"

extern const int TRIGGER;

class Gun { // This class represents a gun
public:
    Gun(std::string _gunName, int _fireSpeed, int _burstSize, int _clipSize, int _reloadTime); // Constructor

    void fireGun(); // Fires the gun

    bool canFire(); // Returns whether the gun can fire

    void poll(); // Polls the gun for input

    int getShotsInClip(){return bullets;}

    int getClips(){return clipsRemaining;}

    void resetGun();

    std::string getName(){return gunName;}

private:
    std::string gunName; // The name of the gun
    int fireSpeed; // The speed at which the gun can fire
    int burstSize; // The number of shots in a burst (-1 for automatic)
    int clipSize; // The number of shots in a clip
    int reloadTime; // The time it takes to reload the gun

    int bullets; // The number of shots left in the clip
    int shotsLeftInBurst; // The number of shots left in the burst
    int clipsRemaining;
    bool reloading = false;

    unsigned long reloadStartMillis;
    unsigned long lastFireMillis;
};


#endif //LASERTAG_TECHCAMP_GUN_H
