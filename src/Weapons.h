#ifndef LASERTAG_TECHCAMP_WEAPONS_H
#define LASERTAG_TECHCAMP_WEAPONS_H

#include "src/Guns/Gun.h"
#include "vector"
#include "SoundData.h"

class Weapons {

public:
    // DO NOT MODIFY THE LZR pistol weapon !!!
    static inline Gun LaserPistol = Gun("LZR Gun", 100, -1, 30, 2000,&phaserSound,1);

    // define your guns here
    static inline Gun M4 = Gun("M4", 500, -1, 20, 2000,&phaserSound,0.6);
    static inline Gun P90 = Gun("P90", 50, 3, 20, 2000,&M16Sound,0.2);
    static inline Gun M16 = Gun("M16", 37, -1, 10, 1000,&M16Sound,0.2);
    static inline Gun AR15 = Gun("AR15", 100, -1, 30, 2000,&M16Sound,0.3);
    static inline Gun Healer = Gun("Healer", 100, -1, 30, 2000,&pickupSound,1);
    static inline Gun GodMode = Gun("GodMode", 25, -1, 30, 500,&godShot,1);
    static inline Gun LifeSteal = Gun("LifeSteal", 1000, 1, 1, 2500,&pickupSound,1);


    // Add all normal guns into this list
    static inline std::vector<Gun *> allGuns = {&AR15, &LaserPistol, &M4, &P90, &M16};
    // Add all god guns into this list
    static inline std::vector<Gun *> allGodGuns = { &Healer, &GodMode, &LifeSteal};



    static inline Gun *currentGun = &AR15;
};


#endif //LASERTAG_TECHCAMP_WEAPONS_H
