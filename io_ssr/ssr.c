#include "colibri-sdk/colibri.h"
#include "colibri-sdk/colibri-io.h"

static const Host_API_t* host = NULL;

static uint8_t output = 0;
static uint8_t buf[2];
static uint16_t slot;

static void update(void)
{
    buf[0] = 0x01;
    buf[1] = output;
    host->i2c_write(0x41, buf, sizeof(buf));
}

static void set(int channel, bool value)
{
    if (value)
    {
        output = output | 1 << channel;
    }
    else
    {
        output = output & ~(1 << channel);
    }
    update();
}

static void driver_init(uint16_t slot_number, uint8_t *calibration_data)
{
    slot = slot_number;
    output = 0;
    update();
    buf[0] = 0x03; // config register
    buf[1] = 0x0; // all pins as outputs
    host->i2c_write(0x41, buf, sizeof(buf));
    event_t event = {.type= COLIBRI_EVENT_TYPE_OUTPUT, .parameter = 1};
    host->subscribe(event);
    event.parameter = 2;
    host->subscribe(event);
}

static void config(int32_t id, uint64_t value)
{
    // TODO: Maybe modes for "slow" PWM or "pulse" mode
}

static void driver_event(event_t event, int64_t value)
{
    switch (event.type)
    {
    case COLIBRI_EVENT_TYPE_TIME_PERIOD:
        update();
        break;
    case COLIBRI_EVENT_TYPE_OUTPUT:
        set(event.parameter, value & 1);
        break;
    case COLIBRI_EVENT_TYPE_CONFIG:
        config(event.parameter, value);
        break;
    default:
        break;
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
        .event = driver_event
    };

    // Return the driver's interface back to the host
    return &driver_vmt;
}
