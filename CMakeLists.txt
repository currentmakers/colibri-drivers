cmake_minimum_required(VERSION 3.10)
project(colibri_drivers C)

set(CMAKE_C_STANDARD 11)

include_directories(include/)

# 1. Clear default CMake debug flag overrides
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "")

# 2. Global architecture-blind flags (Enforced WITHOUT -g)
set(COMMON_ARM_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(PORTABLE_PIC_FLAGS "-fropi -frwpi")
    set(CROSS_COMPILE_FLAGS "--target=${CMAKE_C_COMPILER_TARGET} --sysroot=${CMAKE_SYSROOT}")
    # Full "-g" emits variable location info that references SB-relative (r9) addresses under
    # -frwpi; ld.lld then rejects the resulting R_ARM_SBREL32 relocations inside .debug_info.
    # Line-tables-only debug info avoids that while still letting objdump map addresses to source.
    set(DEBUG_LINE_FLAGS "-gline-tables-only")
else()
    set(PORTABLE_PIC_FLAGS "-fPIC -msingle-pic-base -mpic-register=r9 -mno-pic-data-is-text-relative")
    set(CROSS_COMPILE_FLAGS "")
    set(DEBUG_LINE_FLAGS "-g1")
endif()

# We strictly enforce no -g here
set(CMAKE_C_FLAGS "${COMMON_ARM_FLAGS} ${PORTABLE_PIC_FLAGS} -O2")

# ==========================================
# Centralized Build Macro
# ==========================================
macro(add_colibri_driver DRIVER_NAME SOURCE_FILE MODEL_NAME MODEL_ID MODEL_REVISION)
    add_executable(${DRIVER_NAME} ${SOURCE_FILE})

    # Emit DWARF line-table debug info so the final .lst listing can show the originating source
    # lines. This only adds non-allocated .debug_* sections to the ELF; objcopy -O binary (used
    # for the actual flashable .bin) strips them, so the shipped binary is unaffected.
    target_compile_options(${DRIVER_NAME} PRIVATE ${DEBUG_LINE_FLAGS})

    target_link_options(${DRIVER_NAME} PRIVATE
            "-nostdlib"
            "-static"
            "-T${CMAKE_SOURCE_DIR}/module.ld"
            "-Wl,--entry=driver_load"
            "-Wl,--gc-sections"
            "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.map"
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

    # Have the compiler itself emit the assembler listing (instead of disassembling the binary afterwards)
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND sh -c "${CMAKE_C_COMPILER} ${CROSS_COMPILE_FLAGS} ${CMAKE_C_FLAGS} -I${CMAKE_SOURCE_DIR}/include -S -o ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.asm ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
            VERBATIM
            COMMENT "Generating assembler listing using ${CMAKE_C_COMPILER}: ${DRIVER_NAME}.asm"
    )

    # Force search for the ARM/LLVM-specific objdump utility instead of host's /usr/bin/objdump
    find_program(ARM_OBJDUMP
            NAMES llvm-objdump arm-none-eabi-objdump
            DOC "Path to cross-compilation objdump tool"
    )

    if(NOT ARM_OBJDUMP)
        # Fallback to a hardcoded string if find_program fails for some reason
        set(ARM_OBJDUMP "arm-none-eabi-objdump")
    endif()

    # Disassemble the actual linked binary, showing per-instruction memory address + raw bytes + mnemonic,
    # with the originating source lines interleaved and marked as comments ("; ") via --source-comment
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND sh -c "${ARM_OBJDUMP} -d -S -l --source-comment='; ' --wide $<TARGET_FILE:${DRIVER_NAME}> > ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.lst"
            VERBATIM
            COMMENT "Generating address/byte/instruction disassembly using ${ARM_OBJDUMP}: ${DRIVER_NAME}.lst"
    )

    # Convert ELF output to .bin using the cross-tool
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND ${ARM_OBJCOPY} -O binary $<TARGET_FILE:${DRIVER_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.bin
            COMMENT "Generating raw binary using ${ARM_OBJCOPY}: ${DRIVER_NAME}.bin"
    )

    # Convert .elf to .c file using xxd
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND sh -c "printf '#include <stdint.h>\\n\\n' >${DRIVER_NAME}.c && xxd -i -n ${DRIVER_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${DRIVER_NAME}.bin | sed 's/unsigned char/const uint8_t/g' >>${DRIVER_NAME}.c"
            VERBATIM
            COMMENT "Generating C array file using xxd: ${DRIVER_NAME}.c"
    )

    # Create eeprom images
    add_custom_command(TARGET ${DRIVER_NAME} POST_BUILD
            COMMAND sh -c "python3 ../manufacturing.py --model='${MODEL_NAME}' --vendor-model-id=${MODEL_ID} --vendor-revision=${MODEL_REVISION} --pic ${DRIVER_NAME}.bin"
            VERBATIM
            COMMENT "Generating EEPROM image file ${SOURCE_FILE}.eeprom"
    )
endmacro()

# ==========================================
# Register your Drivers here
# ==========================================

add_colibri_driver(carrier carrier/carrier.c "Colibri 8" 1025 A)
add_colibri_driver(io_aic io_aic/aic.c "Colibri AIC" 2 A)
add_colibri_driver(io_aiv io_aiv/aiv.c "Colibri AIV" 1 A)
add_colibri_driver(io_dio1 io_dio1/dio1.c "Colibri DIO1" 5 A)
add_colibri_driver(io_pid1 io_pid1/pid1.c "Colibri PID1" 9 A)
add_colibri_driver(io_ssr io_ssr/ssr.c "Colibri SSR" 13 A)
add_colibri_driver(io_aqv io_aqv/aqv.c "Colibri AQV" 3 C)
add_colibri_driver(io_broken io_broken/broken.c "<empty>" 0 ZZZZ)
add_colibri_driver(io_empty io_empty/empty.c "<empty>" 0 ZZZZ)
