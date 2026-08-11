#include "colibri-sdk/colibri-io.h"
#include "colibri-sdk/colibri.h"
#include "aic-calibration.h"

static const Host_API_t* host = NULL;

static int error;
static uint64_t next;
static int color;
static calibration_t *calibration;
static uint16_t slot;

static void set_rgb(int color)
{
    event_t event = (event_t){.type = COLIBRI_EVENT_TYPE_RGB_INDICATOR};
    host->publish(event, color);
}

static void read_adc(void)
{

}


static void tick(int64_t time)
{
    read_adc();
    if ( error )
    {
        if (time > next )
        {
            if ( color == COLIBRI_COLOR_OFF)
                color = COLIBRI_COLOR_ERROR;
            else
                color = COLIBRI_COLOR_OFF;
            set_rgb(color);
            next = time + 200;
        }
    }
}

static void driver_init(uint16_t slot_number, uint8_t *calibration_data)
{
    slot = slot_number;
    calibration = (calibration_t *) calibration_data;
}


static void driver_event(event_t event, int64_t value)
{
    if ( event.type == COLIBRI_EVENT_TYPE_TIME_PERIOD )
    {
        // Automatically gets an TIME_PERIOD event every 80ms or so.
        tick(value);
    }
}

// The entry point. The linker script places this at the exact start of the binary.
__attribute__((section(".module_entry")))
const Driver_Interface_t* driver_load(const Host_API_t* host_api)
{
    // Save the API pointer so our functions (like tick) can call the host
    host = host_api;

    // Define the VMT mapping our functions
    static const Driver_Interface_t driver_vmt = {
        .initialize = driver_init,
        .event = driver_event,
    };

    // Return the driver's interface back to the host
    return &driver_vmt;
}
