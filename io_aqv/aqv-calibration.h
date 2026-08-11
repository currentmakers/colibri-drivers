#ifndef COLIBRI_DRIVERS_AQV_CALIBRATION_H
#define COLIBRI_DRIVERS_AQV_CALIBRATION_H

typedef union
{
    uint8_t data[128];
    struct
    {
        uint32_t device1;
        float k1;
        float m1;
        uint32_t device2;
        float k2;
        float m2;
    };
} calibration_t;


#endif //COLIBRI_DRIVERS_AQV_CALIBRATION_H
