#ifndef LASERTAG_TECHCAMP_WEAPONS_H
#define LASERTAG_TECHCAMP_WEAPONS_H

#include "src/Guns/Gun.h"
#include "vector"
#include "SoundData.h"

class Weapons {

public:
    // DO NOT MODIFY THE LZR pistol weapon !!!
    static inline Gun LaserPistol = Gun("LZR Gun", 100, -1, 30, 2000,&phaserSound);

    // define your guns here
    static inline Gun M4 = Gun("M4", 250, -1, 20, 2000,&phaserSound);
    static inline Gun P90 = Gun("P90", 50, 3, 20, 2000,&M16Sound);


    // Add all guns into this list
    static inline std::vector<Gun *> allGuns = {&LaserPistol, &M4, &P90};


    static inline Gun *currentGun = &LaserPistol;
};


#endif //LASERTAG_TECHCAMP_WEAPONS_H
