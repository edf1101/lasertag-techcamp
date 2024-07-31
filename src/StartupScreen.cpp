/*
 * Created by Ed Fillingham on 30/07/2024.
*/

#include "src/StudentHidden/StartupScreen.h"


// u8g2 documentation: https://github.com/olikraus/u8g2/wiki/u8g2reference
void StartupScreen::startupScreen(U8G2_SSD1306_128X64_NONAME_F_HW_I2C* u8g2) {
  u8g2->begin();
  u8g2->setFont(u8g2_font_logisoso16_tf);
  u8g2->clearBuffer();

//   Example code showing how to draw an xbm:
   u8g2->drawXBM(30, 16, 64, 32, bullet);
   u8g2->sendBuffer();
   delay(2000);

}