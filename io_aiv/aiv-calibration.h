#ifndef COLIBRI_DRIVERS_AQV_CALIBRATION_H
#define COLIBRI_DRIVERS_AQV_CALIBRATION_H

typedef union
{
    uint8_t data[128];
    struct
    {
        uint32_t device;
        float k1;   // AIN0 = 3.3V
        float m1;
        float k2;   // AIN1 = GND
        float m2;
        float k3;   // AIN2 = Input2
        float m3;
        float k4;   // AIN3 = Input1
        float m4;
    };
} calibration_t;


#endif //COLIBRI_DRIVERS_AQV_CALIBRATION_H
