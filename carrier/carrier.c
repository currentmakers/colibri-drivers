#include "colibri-io.h"

static const Host_API_t* host = NULL;

static int wd_state = 0;

static void tick(uint64_t time)
{
    wd_state ^= 1;
    // TODO: Kick External Hardware Watchdog
    // TODO: Read and publish board temperature and humidity at reasonable intervals (1 second?)
}

static void driver_init(void)
{
}

static void driver_loaded(void)
{
    // TODO: Enable Watchdog
}

static void driver_unloading(void)
{
    // TODO: Disable Watchdog
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