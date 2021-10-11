#ifndef Thermistor_h
#define Thermistor_h

#include "Arduino.h"
#include "math.h"


class Thermistor {
    public:
        Thermistor(int pin);
        double getTemp();

    private:
        int _pin;
};

#endif