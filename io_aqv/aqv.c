#include "colibri.h" // Assumed to contain your colors: COLIBRI_COLOR_OK, etc.

static const Host_API_t* host = NULL;

static uint64_t next = 0;
static int state = 0;

static int tick(uint64_t time)
{
    if (next < time)
    {
        next = time + 1000;
        if (state)
            host->set_rgb_color(COLIBRI_COLOR_ERROR);
        else
            host->set_rgb_color(COLIBRI_COLOR_OFF);
        state = !state;
    }
    return state;
}

// 1. Define the internal implementations of your driver's lifecycle
static void driver_init(void)
{
}

static void driver_loaded(void)
{
    host->set_rgb_color(COLIBRI_COLOR_OFF);
}

static void driver_unloading(void)
{
    host->set_rgb_color(COLIBRI_COLOR_OFF);
}

static void driver_event(int32_t id, uint64_t value)
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
        .init      = driver_init,
        .loaded    = driver_loaded,
        .event     = driver_event,
        .unloading = driver_unloading
    };

    // Return the driver's interface back to the host
    return &driver_vmt;
}