#ifndef LASERTAG_TECHCAMP_WEAPONS_H
#define LASERTAG_TECHCAMP_WEAPONS_H

#include "Guns/Gun.h"
#include "vector"

class Weapons {

public:
    // DO NOT MODIFY THE AK47 !!!
    static inline Gun AK47 = Gun("AK47", 100, -1, 30, 2000);

    // define your guns here
    static inline Gun M4 = Gun("M4", 250, -1, 20, 2000);
    static inline Gun P90 = Gun("P90", 50, 3, 20, 2000);


    // Add all guns into this list
    static inline std::vector<Gun *> allGuns = {&AK47, &M4, &P90};


    static inline Gun *currentGun = &AK47;
};


#endif //LASERTAG_TECHCAMP_WEAPONS_H
