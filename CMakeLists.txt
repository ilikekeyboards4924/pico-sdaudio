cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================


set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(sdaudio C CXX ASM)

pico_sdk_init()

add_executable(sdaudio
    sdaudio.c
)

pico_set_program_name(sdaudio "sdaudio")
pico_set_program_version(sdaudio "0.1")

pico_enable_stdio_uart(sdaudio 0)
pico_enable_stdio_usb(sdaudio 1)

pico_generate_pio_header(sdaudio ${CMAKE_CURRENT_LIST_DIR}/audio.pio)

add_subdirectory(pico-vfs)
pico_enable_filesystem(sdaudio FS_INIT ${CMAKE_CURRENT_LIST_DIR}/fs_init.c)

target_link_libraries(sdaudio
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_clocks
        hardware_dma
        blockdevice_sd
        filesystem_fat
        filesystem_vfs
)

target_include_directories(sdaudio PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(sdaudio)