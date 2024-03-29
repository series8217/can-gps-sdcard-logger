#ifndef SDCARD_H
#define SDCARD_H

#include "common.h"
#include "SdFat.h"

#if SD_FAT_TYPE == 0
extern SdFat sd;
extern File file;
#elif SD_FAT_TYPE == 1
extern SdFat32 sd;
extern File32 file;
#elif SD_FAT_TYPE == 2
extern SdExFat sd;
extern ExFile file;
#elif SD_FAT_TYPE == 3
extern SdFs sd;
extern FsFile file;
#else // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif // SD_FAT_TYPE

void sd_card_message_speed();
void sd_card_msg_reformat();
int setup_sd_card();

#endif // SDCARD_H