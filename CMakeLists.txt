cmake_minimum_required(VERSION 3.10)
project(colibri_drivers C)

set(CMAKE_C_STANDARD 11)

# Ensure the compiler can always find colibri.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/colibri-sdk)

# 1. Clear default CMake debug flag overrides
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "")

# 2. Global architecture-blind flags (Enforced WITHOUT -g)
set(COMMON_ARM_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(PORTABLE_PIC_FLAGS "-fropi -frwpi")
else()
    set(PORTABLE_PIC_FLAGS "-fPIC -msingle-pic-base -mpic-register=r9 -mno-pic-data-is-text-relative")
endif()

# We strictly enforce no -g here
set(CMAKE_C_FLAGS "${COMMON_ARM_FLAGS} ${PORTABLE_PIC_FLAGS} -O2")

# ==========================================
# Centralized Build Macro
# ==========================================
macro(add_colibri_driver DRIVER_NAME SOURCE_FILE)
    add_executable(${DRIVER_NAME} ${SOURCE_FILE})

    target_link_options(${DRIVER_NAME} PRIVATE
            "-nostdlib"
            "-static"
            "-T${CMAKE_SOURCE_DIR}/module.ld"
            "-Wl,--entry=module_register"
            "-Wl,--gc-sections"
    )

    # Force search for the ARM/LLVM-specific objcopy utility instead of host's /usr/bin/objcopy
    find_program(ARM_OBJCOPY
            NAMES llvm-objcopy arm-none-eabi-objcopy
            DOC "Path to cross-compilation objcopy tool"
    )

    if(NOT ARM_OBJCOPY)
        # Fallback to a hardcoded string if find_program fails for some reason
        set(ARM_OBJCOPY "arm-none-eabi-objcopy")
    endif()

    # Convert ELF output to .bin using the cross-tool
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND ${ARM_OBJCOPY} -O binary $<TARGET_FILE:${DRIVER_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.bin
            COMMENT "Generating raw binary using ${ARM_OBJCOPY}: ${DRIVER_NAME}.bin"
    )

    # Convert .elf to .c file using xxd
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND sh -c "printf '#include <stdint.h>\\n\\n' >${CMAKE_CURRENT_BINARY_DIR}/../bin_output/${DRIVER_NAME}.c && xxd -i -n ${DRIVER_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.bin | sed 's/unsigned char/const uint8_t/g' >>${CMAKE_CURRENT_BINARY_DIR}/../bin_output/${DRIVER_NAME}.c"
            VERBATIM
            COMMENT "Generating C array file using xxd: ${DRIVER_NAME}.c"
    )

endmacro()

# ==========================================
# Register your Drivers here
# ==========================================

add_colibri_driver(carrier carrier/carrier.c)
add_colibri_driver(io_aic io_aic/aic.c)
add_colibri_driver(io_aiv io_aiv/aiv.c)
add_colibri_driver(io_aqv io_aqv/aqv.c)
add_colibri_driver(io_dio1 io_dio1/dio1.c)
add_colibri_driver(io_pid1 io_pid1/pid1.c)
add_colibri_driver(io_ssr io_ssr/ssr.c)
add_colibri_driver(rgb_blink rgb_blink/blink.c)
add_colibri_driver(io_empty io_empty/empty.c)
