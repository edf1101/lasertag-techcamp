/*
 * Created by Ed Fillingham on 30/07/2024.
*/



#ifndef LASERTAG_TECHCAMP_STARTUPSCREEN_H
#define LASERTAG_TECHCAMP_STARTUPSCREEN_H

#include "Images.h"
#include <U8g2lib.h>
#include <string.h>
#include "Radio/Radio.h"
#include "SoundData.h"
#include "XT_DAC_Audio.h";

class StartupScreen {
public:
    static void startupScreen(U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2);
};


#endif //LASERTAG_TECHCAMP_STARTUPSCREEN_H
