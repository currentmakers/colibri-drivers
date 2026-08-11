#include "colibri-sdk/colibri-io.h"
#include "colibri-sdk/colibri.h"

static const Host_API_t* host = NULL;

static uint64_t next = 0;
static int state = 0;
static uint16_t slot;

static void set_rgb(int color)
{
    event_t event = (event_t){.type = COLIBRI_EVENT_TYPE_RGB_INDICATOR};
    host->publish(event, color);
}

static void tick(uint64_t time)
{
    if (next < time)
    {
        if (state)
        {
            next = time + 100;
            set_rgb(COLIBRI_COLOR_ERROR);
        }
        else
        {
            next = time + 900;
            set_rgb(COLIBRI_COLOR_OFF);
        }
        state = !state;
    }
}

static void driver_init(uint16_t slot_number, uint8_t *calibration_data)
{
    slot = slot_number;
}

static void driver_event(event_t event, int64_t value)
{
    tick(value);
}

// The entry point. The linker script places this at the exact start of the binary.
__attribute__((section(".module_entry")))
const Driver_Interface_t* module_register(const Host_API_t* host_api)
{
    // Save the API pointer so our functions (like tick) can call the host
    host = host_api;

    // Define the VMT mapping our functions
    static const Driver_Interface_t driver_vmt = {
        .initialize = driver_init,
        .event      = driver_event,
    };

    // Return the driver's interface back to the host
    return &driver_vmt;
}