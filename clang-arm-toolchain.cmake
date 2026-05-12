# Target Operating System
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prevent CMake from failing the compiler test due to missing bare-metal runtime libraries
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# Set compilers to Clang
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)

# Specify the Target Triple to force cross-compilation
set(ARM_TARGET_TRIPLE "arm-none-eabi")
set(CMAKE_C_COMPILER_TARGET ${ARM_TARGET_TRIPLE})
set(CMAKE_ASM_COMPILER_TARGET ${ARM_TARGET_TRIPLE})

# Direct path to system headers (usually located via GNU Arm Embedded Toolchain)
set(CMAKE_SYSROOT "/usr/share/arm-none-eabi")