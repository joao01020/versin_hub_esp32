#ifndef DISPLAY_MANAGER_H


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


class DisplayManager {

    // Funções privadas 
private:
    Adafruit_SSD1306 display;
    void checkHardware(); 

    


    //funções publicas
public:
    DisplayManager();
    
    void initDisplays(); 
    void updateSystemVisual(bool isWifiConnected, String operation);
};

#endif // DISPLAY_MANAGER_H