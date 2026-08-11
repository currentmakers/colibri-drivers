#include "colibri-sdk/colibri-io.h"
#include "colibri-sdk/colibri.h"
#include "aqv-calibration.h"

static const Host_API_t* host = NULL;
static calibration_t* calibration;
static uint8_t buffer[4];
static uint16_t slot;
static uint16_t dac_value;

static void driver_init(uint16_t slot_number, uint8_t* calibration_data)
{
    slot = slot_number;
    calibration = (calibration_t*)calibration_data;

    // Subscribe to OUTPUT events for this slot.
    event_t to = (event_t){.io = true, .type = COLIBRI_EVENT_TYPE_OUTPUT, .parameter = 1, .slot = slot_number};
    host->subscribe(to);
    to.parameter = 2;
    host->subscribe(to);
}

static void driver_event(event_t event, int64_t value)
{
    switch (event.type)
    {
    case COLIBRI_EVENT_TYPE_TIME_PERIOD:
        // do nothing?
        break;
    case COLIBRI_EVENT_TYPE_OUTPUT:
        {
            // argument `value` is a floating point value to be set: the
            // event bus only carries an int64_t, so the caller packs the
            // wanted voltage's raw 32-bit float bit pattern into its low
            // 4 bytes and we reinterpret them back into a float here
            // (memcpy instead of a `(float*)&value` cast to avoid a
            // strict-aliasing violation -- same effect).
            uint8_t device;
            float out0_10V;
            __builtin_memcpy_inline(&out0_10V, &value, sizeof(out0_10V));
            // k is calibrated directly in DAC counts per volt (no offset
            // term: the DAC can't go negative, so there's no way to
            // compensate an opamp bias by shifting output down -- only the
            // gain is corrected).
            switch (event.parameter)
            {
            case 1: // for Output 1
                dac_value = (uint16_t)(out0_10V * calibration->k1) & 0xFFF;
                device = calibration->device1;
                break;
            case 2: // for Output 1
                dac_value = (uint16_t)(out0_10V * calibration->k2) & 0xFFF;
                device = calibration->device2;
                break;
            default:
                return;
            }
            // Fast Mode sends 2 bytes;
            // byte0 = 0b0000dddd, where top 4 bits are "fast mode, power down mode=normal" and dddd is data bits 8-11
            // byte1 = data bits 0-7
            buffer[0] = dac_value >> 8;
            buffer[1] = dac_value & 0xFF;
            host->i2c_write(device, buffer, 2);
            break;
        }
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
