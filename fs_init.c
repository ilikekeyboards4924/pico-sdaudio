#include "pico/stdlib.h"
#include "blockdevice/sd.h"
#include "filesystem/fat.h"
#include "filesystem/vfs.h"

bool fs_init(void)
{
    // change as needed
    // spi, MOSI, MISO, SCK, CS, baud rate, ??? unknown bool that is false
    blockdevice_t *sd = blockdevice_sd_create(spi0, 19, 16, 18, 17, 12500 * 1000, false);
    filesystem_t *fat = filesystem_fat_create();
    return fs_mount("/", fat, sd) == 0;
}