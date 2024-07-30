#ifndef Updater_h
#define Updater_h

int updaterSetup(void);
void updaterLoop(void);

extern IPAddress myIPAddress;
extern int uploadStatus;

#endif
