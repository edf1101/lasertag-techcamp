/*
 * Created by Ed Fillingham on 30/07/2024.
*/

#include "Gun.h"

#include <utility>

Gun::Gun(std::string _gunName, int _fireSpeed, int _burstSize, int _clipSize, int _reloadTime,
         XT_Wav_Class *_fireSound) {
  gunName = std::move(_gunName);
  fireSpeed = _fireSpeed;
  burstSize = _burstSize;
  clipSize = _clipSize;
  reloadTime = _reloadTime;
  myFireSound = _fireSound;

  bullets = clipSize;
  shotsLeftInBurst = burstSize;
  clipsRemaining = 9;
}

void Gun::fireGun() {
  if (!canFire())
    return;

  if (burstSize != -1) {
    shotsLeftInBurst--;
  }
  lastFireMillis = millis();
  bullets--;
  if (bullets == 0 && clipsRemaining > 0) { // need to reload
    clipsRemaining--;
    reloading = true;
    reloadStartMillis = millis();

  }


}

void Gun::poll() {

  if (digitalRead(TRIGGER) == HIGH) { // pull up resistor so this means trigger is unpressed
    if (burstSize != -1) {
      shotsLeftInBurst = burstSize;
    }
  }

  if (reloading && millis() - reloadStartMillis > reloadTime) { // reload
    reloading = false;
    bullets = clipSize;
  }
}

bool Gun::canFire() {
  // return if the gun can fire

  if (bullets <= 0)
    return false;
  if (reloading)
    return false;
  if (millis() - lastFireMillis < fireSpeed)
    return false;
  if (burstSize != -1 && shotsLeftInBurst <= 0)
    return false;

  return true;
}

void Gun::resetGun() {
  // reset stats when we enter a new game

  shotsLeftInBurst = burstSize;
  bullets = clipSize;
  clipsRemaining = 9;

}
