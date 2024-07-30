#include "Menu.h"
#include <string.h>
#include <MD_REncoder.h>

#define USE_SSD1306
  #define fontName u8g2_font_helvB14_tr
  #define fontSmall u8g2_font_7x13B_mr
  //#define fontName u8g2_font_7x13_mf
  #define fontX 7
  #define fontY 16
  #define offsetX 0
  #define offsetY 3 
  #define U8_Width 128
  #define U8_Height 64
  #define USE_HWI2C
  
MD_REncoder R = MD_REncoder(ENC_A, ENC_B);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ CLOCK_PIN, /* data=*/ DATA_PIN);
MenuItem *currentMenuItem=NULL;
MenuItem *topMenu;
char changingParams=0;
void (* menuFunction)(const char *itemName, int value);
unsigned long lastPress;

char MenuItem::isActive() // Returns true if menu is visible
{
  return(!(currentMenuItem==topMenu));
}

MenuItem *MenuItem::getItem(const char*thisName)
{
  MenuItem *workingItem;

  workingItem = this;
  if(strcmp(itemName,thisName)) // This menu is NOT the item we're looking for, so look through it's children and subsequent peers
  {
    if(workingItem->childItem) // if this item has a child, go into this child to check if it is the one we're looking for or if it's one of it's descendents ...
    {
      workingItem = workingItem->childItem->getItem(thisName);
      if(workingItem)
        return workingItem;
    }
    // Now step through the subsequent items in the menu to see if they are the correct item we're looking for ...
    workingItem = nextItem;
    while(workingItem) // Now we're looking through the list of subsequent menu items to check
    {
      if(strcmp(workingItem->itemName,thisName)==0)
        return workingItem;
      if(workingItem->childItem) // there's another child menu, so will need to check it and it's descendents for the right one ...
      {
        workingItem = workingItem->childItem->getItem(thisName);
        if(workingItem)
          return workingItem;
      }
      workingItem = workingItem->nextItem; // Go to the next menu item in the list to check it
    }
    return NULL;
  }
  else
    return workingItem;
}

int MenuItem::getParam(const char*thisName)
{
  MenuItem *thisItem;
  thisItem = getItem(thisName);
  if(thisItem)
    return thisItem->defaultVar;
  else
    return -1;
}

void MenuItem::setParam(const char*thisName, int value)
{
  MenuItem *thisItem;
  thisItem = getItem(thisName);
  if(thisItem)
    thisItem->defaultVar=value;
}

MenuItem::MenuItem(const char*thisName, MenuItem *thisParent, MenuItem *thisChild, MenuItem *thisPrev, MenuItem *thisNext, int thisMin, int thisMax, int thisDefault, int thisInc)
{
  strncpy(itemName, thisName, 16);
  parentItem = thisParent;
  childItem = thisChild;
  prevItem = thisPrev;
  nextItem = thisNext;
  minVar = thisMin;
  maxVar = thisMax;
  defaultVar = thisDefault;
  incVar = thisInc;
}

void MenuItem::activate(void (* functionPointer)(const char *itemName, int value))
// This is called to activate the menu.  The callback function will be called whenever an item is selected or parameter editing has finished
{
  menuFunction = functionPointer;

  R.begin();
  
  u8g2.begin();
  u8g2.setContrast(255);  
  //setSSD1306VcomDeselect(7);
  //setSSD1306PreChargePeriod(1,15);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  topMenu = this;
  currentMenuItem = topMenu;
}

int MenuItem::addItem(const char*menuName, const char*thisName, int minVal, int maxVal, int defaultVal, int increment)
{
  MenuItem *previous;
  MenuItem *workingItem;

  workingItem = this;
  // This code is voodoo.  Do not change it unless you REALLY know what you are doing.  It recursively goes through the menu finding where to add the item
  // It could be simpler by processing the linked list of subsequent menu items as well as the 'child' items, but this would eat up the stack more quickly
  if(strcmp(itemName,menuName)) // This menu is NOT the parent menu we're looking to insert an item into, so try finding the right menu in its descendents or subsequent items ...
  {
    if(childItem) // if this item has a child, go into this child to check if it is the correct 'parent' and to add item to it or its descendents ...
      childItem->addItem(menuName,thisName,minVal,maxVal,defaultVal,increment);
    // Now step through the subsequent items in the menu to see if they are the correct 'parent' menu we're looking for ...
    workingItem = nextItem;
    while(workingItem) // Now we're looking through the list of subsequent menu items to check if these are the ones we need to add the item to:
    {
      if(strcmp(workingItem->itemName,menuName)==0)
        break; // we've found it - drop through and process adding the new item as don't need to check any more children or subsequent items ...
      if(workingItem->childItem) // there's another child menu, so will need to check it and it's descendents for the right parent menu ...
        workingItem->childItem->addItem(menuName,thisName,minVal,maxVal,defaultVal,increment);
      workingItem = workingItem->nextItem; // Go to the next menu item in the list to check it
    }
  }

  if(!workingItem) // got to the end of the list of menu items without a match, so can't find it here - let's exit
    return 0;
    
  // At this point we are in the correct menu so add it to this menu ... </end voodoo>
  if(workingItem->childItem==NULL) // no children yet, so there won't be a 'previous' one for this new child ...
    previous=NULL;
  else // at least one child, so step through to the last one ...
  {
    previous=workingItem->childItem;
    while(previous->nextItem!=NULL)
      previous=previous->nextItem;
  }
  MenuItem *newMenuItem = new MenuItem(thisName,workingItem,NULL,previous,NULL,minVal,maxVal,defaultVal,increment);
  if(workingItem->childItem==NULL) // First child, so just point to it ...
    workingItem->childItem=newMenuItem;
  else // Subsequent child, so point previous one to new one ...
    previous->nextItem = newMenuItem;
  return 1;
}

void updateMenu() // This draws the menu
{
  int textWidth;
  char buffer[17];
  
  u8g2.clearBuffer();
  if(currentMenuItem!=topMenu) // if current item points to main menu, the menu system isn't active 
  {
    if(currentMenuItem->prevItem!=NULL) // if there's a previous menu item, draw it in small font above the selected one
    {
      u8g2.setFont(fontSmall);
      textWidth = u8g2.getStrWidth(currentMenuItem->prevItem->itemName);
      u8g2.setCursor((128-textWidth)>>1, 15);
      u8g2.print(currentMenuItem->prevItem->itemName);
    }
    u8g2.setFont(fontName); // now draw the selected menu item in bigger font ...
    if(currentMenuItem->maxVar>0)
    {
      if(changingParams)
        sprintf(buffer,"%s:%i",currentMenuItem->itemName,currentMenuItem->defaultVar);
      else
        sprintf(buffer,"%s=%i",currentMenuItem->itemName,currentMenuItem->defaultVar);
    }
    else
      strcpy(buffer,currentMenuItem->itemName);
    textWidth = u8g2.getStrWidth(buffer);
    u8g2.setCursor((128-textWidth)>>1, 35);
    u8g2.print(buffer);
    if(currentMenuItem->nextItem!=NULL) // if there's a subsequent menu item, draw it as well in smaller font below the selected one ...
    {
      u8g2.setFont(fontSmall);
      textWidth = u8g2.getStrWidth(currentMenuItem->nextItem->itemName);
      u8g2.setCursor((128-textWidth)>>1, 55);
      u8g2.print(currentMenuItem->nextItem->itemName);
    }
  }
  u8g2.sendBuffer();
}

int readEncoder()
{
  int encoderValue = R.read();
  if(encoderValue==DIR_CW)
    return 1;
  else if(encoderValue==DIR_CCW)
    return -1;
  else
    return 0;
}

int menuService() // must be called regularly to check the encoder, process the menu system, and update the LCD
{
  int encoderPulses = readEncoder();

  if(encoderPulses!=0)
  {
    if(currentMenuItem==topMenu) // If menu not showing ...
      return encoderPulses;
    if(currentMenuItem!=topMenu) // If menu showing ...
    {
      if(changingParams) // Allow parameter editing ...
      {
        unsigned int increment;;
        increment = currentMenuItem->incVar;
        if(R.speed()>=10)
          increment*=10;
        currentMenuItem->defaultVar+=(increment*encoderPulses);
        if(currentMenuItem->defaultVar>currentMenuItem->maxVar)
          currentMenuItem->defaultVar = currentMenuItem->maxVar;
        if(currentMenuItem->defaultVar<currentMenuItem->minVar)
          currentMenuItem->defaultVar = currentMenuItem->minVar;
      }
      else if((encoderPulses>0) && (currentMenuItem->nextItem!=NULL)) // ... or allow menu selection
        currentMenuItem=currentMenuItem->nextItem;
      else if((encoderPulses<0) && (currentMenuItem->prevItem!=NULL))
        currentMenuItem=currentMenuItem->prevItem;
    }
    updateMenu();
  }
  // Now process encoder button press but don't allow pressing unless at least 300ms have elapsed since last press
  if((digitalRead(ENC_SW)==LOW) && (millis()>=(lastPress+300))) 
  {
    lastPress=millis();
    if(changingParams) // clicked when finished editing a parameter - execute the callback
    {
      changingParams=0;
      menuFunction(currentMenuItem->itemName,currentMenuItem->defaultVar);
    }
    else if(currentMenuItem==topMenu) // Menu not displayed yet, so set it to first item on main menu ...
      currentMenuItem=topMenu->childItem;
    else
    {
      if(strcmp(currentMenuItem->itemName,"Back")==0) // clicked on a 'back' option so go back!
        currentMenuItem=currentMenuItem->parentItem;
      else if(currentMenuItem->childItem!=NULL) // clicked on a submenu, so go into it ...
        currentMenuItem=currentMenuItem->childItem;
      else if(currentMenuItem->maxVar>0) // clicked on a parameter, so start editing ...
        changingParams=1;
      else // must have clicked on a 'final' menu item, so execute the callback ...
        menuFunction(currentMenuItem->itemName,currentMenuItem->defaultVar); 
    }
    updateMenu(); 
  }
  return 0;
}
