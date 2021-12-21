#ifndef air_h
#define air_h


#include "Arduino.h"

class air
{
    public:
        air(int pin);
        void launch(const char* host, const char* ssid, const char* password, bool debug);
        
    private:
        int _pin;
        const char* _host;
        const char* _ssid; 
        const char* _password;
        bool _debug;
        
       

};

#endif


