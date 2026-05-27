#include "colibri.h" // Assumed to contain your colors: COLIBRI_COLOR_OK, etc.
#include "colibri-io.h" // Assumed to contain your colors: COLIBRI_COLOR_OK, etc.
static const Host_API_t* host = NULL;

static uint8_t output = 0;
static uint8_t buf[2];

static void update(void)
{
    buf[0] = 0x01; // output register
    buf[1] = output; // all pins off
    host->i2c_write(0x41, buf, sizeof(buf));
}

static int set(int channel, bool value)
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

static void driver_init(void)
{
}

static void driver_loaded(void)
{
    output = 0;
    update();
    buf[0] = 0x03; // config register
    buf[1] = 0x0; // all pins as outputs
    host->i2c_write(0x41, buf, sizeof(buf));
    host->subscribe(create_io_event(0, COLIBRI_EVENT_TYPE_OUTPUT, 0));
    host->subscribe(create_io_event(0, COLIBRI_EVENT_TYPE_OUTPUT, 1));
}

static void driver_unloading(void)
{
}

static void config(int32_t id, uint64_t value)
{
    // TODO: Maybe modes for "slow" PWM or "pulse" mode
}

/*
 * Events supported;
 *
 *
   */
static void driver_event(int32_t event, uint64_t value)
{
    switch (event_type(event))
    {
    case COLIBRI_EVENT_TYPE_TIME_PERIOD:
        update();
        break;
    case COLIBRI_EVENT_TYPE_OUTPUT:
        set(event_param(event), value & 1);
        break;
    case COLIBRI_EVENT_TYPE_CONFIG:
        config(event_param(event), value);
        break;
    }
}

// The entry point. The linker script places this at the exact start of the binary.
__attribute__((section(".module_entry")))
const Driver_Interface_t* module_register(const Host_API_t* host_api)
{
    // Save the API pointer so our functions (like tick) can call the host
    host = host_api;

    // Define the VMT mapping our functions
    static const Driver_Interface_t driver_vmt = {
        .init = driver_init,
        .loaded = driver_loaded,
        .event = driver_event,
        .unloading = driver_unloading
    };

    // Return the driver's interface back to the host
    return &driver_vmt;
}
