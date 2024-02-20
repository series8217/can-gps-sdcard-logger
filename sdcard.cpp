#include "sdcard.h"
#include "common.h"

#include "SdFat.h"
#include "sdios.h"

const int chipSelect = SDCARD_CS;

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif // SD_FAT_TYPE

/************************************/
/**    SD Card Helper Functions     */
/************************************/
void sd_card_message_speed()
{
    cout << F("Try another SD card or reduce the SPI bus speed.\n");
    cout << F("Edit SPI_SPEED in this program to change it.\n");
}

void sd_card_msg_reformat()
{
    cout << F("Try reformatting the card.  For best results use\n");
    cout << F("the SdFormatter program in SdFat/examples or download\n");
    cout << F("and use SDFormatter from www.sdcard.org/downloads.\n");
}

int setup_sd_card()
{
    cout << F("\nSPI pins:\n");
    cout << F("MISO: ") << int(MISO) << endl;
    cout << F("MOSI: ") << int(MOSI) << endl;
    cout << F("SCK:  ") << int(SCK) << endl;
    cout << F("SS:   ") << int(SS) << endl;
#ifdef SDCARD_SS_PIN
    cout << F("SDCARD_SS_PIN:   ") << int(SDCARD_SS_PIN) << endl;
#endif // SDCARD_SS_PIN

    static bool firstTry = true;

    if (!firstTry)
    {
        cout << F("\nRestarting\n");
    }
    firstTry = false;

    if (!sd.begin(SDCARD_SS_PIN, SPI_SPEED))
    {
        if (sd.card()->errorCode())
        {
            cout << F(
                "\nSD initialization failed.\n"
                "Do not reformat the card!\n"
                "Is the card correctly inserted?\n"
                "Is chipSelect set to the correct value?\n"
                "Does another SPI device need to be disabled?\n"
                "Is there a wiring/soldering problem?\n");
            cout << F("\nerrorCode: ") << hex << showbase;
            cout << int(sd.card()->errorCode());
            cout << F(", errorData: ") << int(sd.card()->errorData());
            cout << dec << noshowbase << endl;
            return 1;
        }
        cout << F("\nCard successfully initialized.\n");
        if (sd.vol()->fatType() == 0)
        {
            cout << F("Can't find a valid FAT16/FAT32 partition.\n");
            cout << F("Try reformatting the card.\n");
            sd_card_msg_reformat();
            return 1;
        }
    }

    uint32_t size = sd.card()->sectorCount();
    if (size == 0)
    {
        cout << F("Can't determine the card size.\n");
        sd_card_message_speed();
        return 1;
    }
    uint32_t sizeMB = 0.000512 * size + 0.5;
    cout << F("Card size: ") << sizeMB;
    cout << F(" MB (MB = 1,000,000 bytes)\n");
    cout << endl;
    cout << F("Volume is FAT") << int(sd.vol()->fatType());
    cout << F(", Cluster size (bytes): ") << sd.vol()->bytesPerCluster();
    cout << endl
         << endl;

    cout << F("Files found (date time size name):\n");
    sd.ls(LS_R | LS_DATE | LS_SIZE);

    if ((sizeMB > 1100 && sd.vol()->sectorsPerCluster() < 64) ||
        (sizeMB < 2200 && sd.vol()->fatType() == 32))
    {
        cout << F("\nThis card should be reformatted for best performance.\n");
        cout << F("Use a cluster size of 32 KB for cards larger than 1 GB.\n");
        cout << F("Only cards larger than 2 GB should be formatted FAT32.\n");
        sd_card_msg_reformat();
        return 1;
    }

    return 0;
}
