#include <Arduino.h>
#include <U8g2lib.h>

#define ENC_A 22
#define ENC_B 21
#define ENC_SW 17
// stockguns=2, encoder V1=16, encoder V2=17

#define CLOCK_PIN 4
#define DATA_PIN 16
// stockguns=16, encoder V1=17, encoder V2=16

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

class MenuItem // Contains the structure of the any menu item which can be menus themselves
{
  public:
    char itemName[17];
    MenuItem *parentItem=NULL; // contains the parent of this menu item - NULL for the root
    MenuItem *childItem=NULL; // links to first child item for submenus
    MenuItem *prevItem=NULL; // links to the previous menu item in this menu - NULL if it's the first
    MenuItem *nextItem=NULL; // links to the next menu item in this menu - NULL if it's the last
    int minVar,maxVar,defaultVar,incVar; // used for parameters to set the range, default and increment
    MenuItem(const char*thisName, MenuItem *thisParent=NULL, MenuItem *thisChild=NULL, MenuItem *thisPrev=NULL, MenuItem *thisNext=NULL, int thisMin=0, int thisMax=0, int thisDefault=0, int thisInc=0);
    int addItem(const char*menuName, const char*thisName, int minVal=0, int maxVal=0, int defaultVal=0, int increment=1);
    MenuItem *getItem(const char*thisName);
    int getParam(const char*thisName);
    void setParam(const char*thisName, int value);
    void activate(void (* functionPointer)(const char *itemName, int value));
    char isActive();
};

extern MenuItem *currentMenuItem;
extern MenuItem *topMenu;

int menuService();
int readEncoder();
