#include "colibri-sdk/colibri-io.h"
#include "colibri-sdk/colibri.h"

static const Host_API_t* host = NULL;

static void driver_init(uint16_t slot_number, uint8_t *calibration_data)
{
}

static void driver_event(event_t event, int64_t value)
{
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
