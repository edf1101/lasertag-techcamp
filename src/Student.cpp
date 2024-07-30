#include "Student.h"
#include "SoundData.h"
#include <string.h>

#define mainFont u8g2_font_logisoso16_tf

const int DISPLAY_HEIGHT = 64;
const int DISPLAY_WIDTH = 128;
const int CENTER_X = DISPLAY_WIDTH/2;
const int CENTER_Y = DISPLAY_HEIGHT/2;

int center_w(int width) {
  return CENTER_X - width/2;
}
int center_h(int height) {
  return CENTER_Y - height/2;
}


// u8g2 documentation: https://github.com/olikraus/u8g2/wiki/u8g2reference
void startupScreen(){
  u8g2.begin();
  u8g2.setFont(mainFont);
  u8g2.clearBuffer();

  // Example code showing how to draw an xbm:
  // u8g2.drawXBM(center_w(64), center_h(32), 64, 32, flag);
  // u8g2.sendBuffer();
  // delay(2000);
  
}
