#include <jpgogc.h>
#include <malloc.h>
#include <ogcsys.h>
#include <ogc/pad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "d_main.h"

#include "gui.h"
#include "m_misc.h"
#include "sys_fat.h"
#include "sys_menu.h"
#include "sys_nand.h"
#include "sys_globals.h"
#include "sys_usbstorage.h"
#include "sys_wpad.h"

#include "video.h"

#include "w_wad.h"

// Macros
#define NB_FAT_DEVICES    (sizeof(fdevList) / sizeof(fatDevice))
#define MAXPATH           0x108
#define MAX_WIIMOTES      4


static CONFIG            gConfig;

// FAT device list 
static fatDevice fdevList[] = {
    { "sd",     "    SD-Card    ",        &__io_wiisd      },
    { "usb",    "  USB-Storage  ",        &__io_usbstorage },
    { "usb2",   "USB 2.0 Storage",        &__io_wiiums     }
};


// wiiNinja: Define a buffer holding the previous path names as user
// traverses the directory tree. Max of 10 levels is define at this point
static fatDevice  *fdev = NULL;

static u8         gDirLevel = 0;

static char       gDirList[MAX_DIR_LEVELS][MAX_FILE_PATH_LEN];

static s32        gSeleted[MAX_DIR_LEVELS];
static s32        gStart[MAX_DIR_LEVELS];

static u32        *xfb;

static GXRModeObj *rmode;

int               is_chex_2 = 0;
int               extra_wad_loaded = 0;
int               load_extra_wad = 0;
int               load_dehacked = 0;

boolean           multiplayer = false;
boolean           multiplayer_flag = false;
boolean           nerve_pwad = false;
boolean           merge = false;

extern char       picdata[];
extern int        piclength;


static int PushCurrentDir (char *dirStr, s32 Selected, s32 Start)
{
    int retval = 0;

    // Store dirStr into the list and increment the gDirLevel
    // WARNING: Make sure dirStr is no larger than MAX_FILE_PATH_LEN
    if (gDirLevel < MAX_DIR_LEVELS)
    {
        M_StringCopy (gDirList [gDirLevel], dirStr, sizeof(gDirList [gDirLevel]));

        gSeleted[gDirLevel] = Selected;

        gStart[gDirLevel] = Start;

        gDirLevel++;
    }
    else
        retval = -1;

    return (retval);
}

static char *PeekCurrentDir (void)
{
    // Return the current path
    if (gDirLevel > 0)
        return (gDirList [gDirLevel - 1]);
    else
        return (NULL);
}

static char *PopCurrentDir(s32 *Selected, s32 *Start)
{
    if (gDirLevel > 1)
        gDirLevel--;
    else
        gDirLevel = 0;

    *Selected = gSeleted[gDirLevel];

    *Start = gStart[gDirLevel];

    return PeekCurrentDir();
}

static void Initialise()
{
    VIDEO_Init();

    rmode = VIDEO_GetPreferredMode(NULL);
	
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    if(rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();
}

static void display_jpeg(JPEGIMG jpeg, int x, int y)
{
    unsigned int *jpegout = (unsigned int *) jpeg.outbuffer;

    int i, j;
    int height = jpeg.height;
    int width = jpeg.width / 2;

    for(i = 0; i <= width; i++)
        for(j = 0; j <= height - 2; j++)
            xfb[(i + x) + 320 * (j + 16 + y)] = jpegout[i + width * j];

    free(jpeg.outbuffer);
}

static void Sys_LoadMenu(void)
{
    int HBC = 0;

    char * sig = (char *)0x80001804;

    if (sig[0] == 'S' &&
        sig[1] == 'T' &&
        sig[2] == 'U' &&
        sig[3] == 'B' &&
        sig[4] == 'H' &&
        sig[5] == 'A' &&
        sig[6] == 'X' &&
        sig[7] == 'X')
        HBC = 1; // Exit to HBC

    // Homebrew Channel stub
    if (HBC == 1)
        exit(0);

    // Return to the Wii system menu
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

static void Con_Clear(void)
{
    // Clear console
    printf("\x1b[2J");

    fflush(stdout);
}

static void Restart(void)
{
    Con_Clear ();

    //fflush(stdout);

    // Load system menu
    Sys_LoadMenu();
}

static s32 __Menu_EntryCmp(const void *p1, const void *p2)
{
    fatFile *f1 = (fatFile *)p1;
    fatFile *f2 = (fatFile *)p2;

    // Compare entries
    // wiiNinja: Include directory
    if ((f1->entry.d_type == DT_DIR) && !(f2->entry.d_type == DT_DIR))
        return (-1);
    else if (!(f1->entry.d_type == DT_DIR) && (f2->entry.d_type == DT_DIR))
        return (1);
    else
        return strcmp(f1->filename, f2->filename);
}

static s32 __Menu_RetrieveList(char *inPath, fatFile **outbuf, u32 *outlen)
{
    fatFile *buffer = NULL;

    DIR *dir = NULL;
    
    struct dirent *entry;

    u32 cnt = 0;

    // Open directory
    dir = opendir(inPath);

    if (!dir)
        return -1;

    while ((entry = readdir(dir)))
        cnt++;

    if (cnt > 0)
    {
        // Allocate memory
        buffer = malloc(sizeof(fatFile) * cnt);

        if (!buffer)
        {
            closedir(dir);

            return -2;
        }

        // Reset directory
        closedir(dir);

        dir = opendir(inPath);

        // Get entries
        for (cnt = 0; (entry = readdir(dir));)
        {
            boolean addFlag = false;

            if (entry->d_type == DT_DIR) 
            {
                // Add only the item ".." which is the previous directory
                // AND if we're not at the root directory
                if ((strcmp (entry->d_name, "..") == 0) && (gDirLevel > 1))
                    addFlag = true;
                else if (strcmp (entry->d_name, ".") != 0)
                    addFlag = true;
            }
            else
            {
                if(strlen(entry->d_name) > 4)
                {
                    if (!stricmp(entry->d_name + strlen(entry->d_name) - 4, ".wad") ||
                            !stricmp(entry->d_name + strlen(entry->d_name) - 4, ".deh") ||
                            !stricmp(entry->d_name + strlen(entry->d_name) - 4, ".txt"))
                        addFlag = true;
                }
            }

            if (addFlag == true)
            {
                fatFile *file = &buffer[cnt++];
    
                // File name
                M_StringCopy(file->filename, entry->d_name, sizeof(file->filename));

                // File stats
                file->entry = *entry;
            }
        }
        // Sort list
        qsort(buffer, cnt, sizeof(fatFile), __Menu_EntryCmp);
    }

    // Close directory
    closedir(dir);

    // Set values
    *outbuf = buffer;
    *outlen = cnt;

    return 0;
}

static void Menu_FatDevice(void)
{
    s32 ret, selected = 0;

    // Select source device
    if (gConfig.fatDeviceIndex < 0)
    {
        for ( ; ; )
        {
            // Clear console
            Con_Clear();

            // Draw main title
            drawMain();

            // Selected device
            fdev = &fdevList[selected];

            printf(">>Source: < %s >", fdev->name);
            printf("    |");
            printf("\n                                 |");
            printf("\n                                 |");
            printStyledText(6, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                        CONSOLE_FONT_BOLD, &stTexteLocation, "IWAD: ");
            printf("\n  L / R: Change device.          |");
            printStyledText(7, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                        CONSOLE_FONT_BOLD, &stTexteLocation, "PWAD1: ");
            printf("\n                                 |\n");
            printStyledText(8, 35,CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                        CONSOLE_FONT_BOLD, &stTexteLocation, "PWAD2: ");
            printf("\n                                 |\n");
            printStyledText(9, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                        CONSOLE_FONT_BOLD, &stTexteLocation, "PWAD3: ");
            printf("\n  A: continue. / Home: Quit.     |");
            printf("\n                                 |\n");
            printStyledText(10, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                        CONSOLE_FONT_BOLD, &stTexteLocation,".DEH: ");
            printStyledText(10, 68, CONSOLE_FONT_BLACK, CONSOLE_FONT_YELLOW,
                        CONSOLE_FONT_BOLD, &stTexteLocation,"MERGE: ");
            printStyledText(11, 0, CONSOLE_FONT_BLACK, CONSOLE_FONT_WHITE,
                        CONSOLE_FONT_BOLD,&stTexteLocation,
            "  ----------------------------------------------------------------------------  ");

            u32 buttons = WaitButtons();

            // LEFT/RIGHT buttons
            if (buttons & WPAD_CLASSIC_BUTTON_LEFT)
            {
                if ((--selected) <= -1)
                    selected = (NB_FAT_DEVICES - 1);
            }

            if (buttons & WPAD_CLASSIC_BUTTON_RIGHT)
            {
                if ((++selected) >= NB_FAT_DEVICES)
                    selected = 0;
            }

            // HOME button
            if (buttons & WPAD_CLASSIC_BUTTON_HOME)
                Restart();

            // A button
            if (buttons & WPAD_CLASSIC_BUTTON_A)
                break;
        }
    }
    else
    {
        sleep(3);

        fdev = &fdevList[gConfig.fatDeviceIndex];
    }

    // Clear console
    Con_Clear();

    // Draw main title
    drawMain();

    printf("  Loading...: %s", fdev->name );
    printf("    |");

    fflush(stdout);

    // Mount FAT device
    ret = Fat_Mount(fdev);

    if (ret < 0)
    {
        printf("\n                                 |");
        printStyledText(6, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                    CONSOLE_FONT_BOLD, &stTexteLocation, "IWAD: ");
        printf("\n");
        printStyledText(6, 0, CONSOLE_FONT_BLACK, CONSOLE_FONT_RED,
                    CONSOLE_FONT_BOLD, &stTexteLocation, "  Error! (ret = %d)", ret);
        printf("              |");
        printf("\n                                 |");
        printStyledText(7, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                    CONSOLE_FONT_BOLD, &stTexteLocation, "PWAD1: ");
        printf("\n                                 |");
        printStyledText(8, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                    CONSOLE_FONT_BOLD, &stTexteLocation, "PWAD2: ");
        printf("\n                                 |");
        printStyledText(9, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                    CONSOLE_FONT_BOLD, &stTexteLocation, "PWAD3: ");
        printf("\n                                 |");
        printStyledText(10, 35, CONSOLE_FONT_BLACK, CONSOLE_FONT_GREEN,
                    CONSOLE_FONT_BOLD, &stTexteLocation, ".DEH: ");
        printStyledText(10, 68, CONSOLE_FONT_BLACK, CONSOLE_FONT_YELLOW,
                    CONSOLE_FONT_BOLD, &stTexteLocation, "MERGE: ");

        goto err;
    }
    else
        printf("  OK!                              |");

    return;

    err:

    if(gConfig.fatDeviceIndex >= 0)
        gConfig.fatDeviceIndex = -1;

    printf("\n  Press any key...               |\n");
    printStyledText(11, 0, CONSOLE_FONT_BLACK, CONSOLE_FONT_WHITE,
                    CONSOLE_FONT_BOLD, &stTexteLocation,
    "  ----------------------------------------------------------------------------  ");

    WaitButtons();

    // Prompt menu again
    Menu_FatDevice();
}


static void Menu_WadList(void)
{
    boolean     md5_check = false;

    char        buffer[4];
    char        check[MAXPATH];
    char        iwad_term[] = "IWAD";
    char        pwad_term[] = "PWAD";
    char        deh_term[] = "Patc";
    char        str [100];
    char        stripped_target[MAXPATH] = "";
    char        stripped_extra_wad_1[256] = "";
    char        stripped_extra_wad_2[256] = "";
    char        stripped_extra_wad_3[256] = "";
    char        stripped_dehacked_file[256] = "";

    const char  *iwad_ver = NULL;
    const char  *shareware_warn = NULL;

    extern char path_tmp[MAXPATH];
    extern char target[MAXPATH];
    extern char extra_wad_1[256];
    extern char extra_wad_2[256];
    extern char extra_wad_3[256];
    extern char dehacked_file[256];
    extern char calculated_md5_string[33];
    extern char known_md5_string_chex_quest_iwad[33];
    extern char known_md5_string_doom_beta_1_4_iwad[33];
    extern char known_md5_string_doom_beta_1_5_iwad[33];
    extern char known_md5_string_doom_beta_1_6_iwad[33];
    extern char known_md5_string_doom_share_1_0_iwad[33];
    extern char known_md5_string_doom_share_1_1_iwad[33];
    extern char known_md5_string_doom_share_1_2_iwad[33];
    extern char known_md5_string_doom_share_1_25s_iwad[33];
    extern char known_md5_string_doom_share_1_666_iwad[33];
    extern char known_md5_string_doom_share_1_8_iwad[33];
    extern char known_md5_string_doom_share_1_9_iwad[33];
    extern char known_md5_string_doom_reg_1_1_iwad[33];
    extern char known_md5_string_doom_reg_1_2_iwad[33];
    extern char known_md5_string_doom_reg_1_6_iwad[33];
    extern char known_md5_string_doom_reg_1_6b_iwad[33];
    extern char known_md5_string_doom_reg_1_666_iwad[33];
    extern char known_md5_string_doom_reg_1_8_iwad[33];
    extern char known_md5_string_doom_reg_1_9_iwad[33];
    extern char known_md5_string_doom_reg_1_9ud_iwad[33];
    extern char known_md5_string_doom_bfg_psn_iwad[33];
    extern char known_md5_string_doom_bfg_pc_iwad[33];
    extern char known_md5_string_doom_xbox_iwad[33];
    extern char known_md5_string_doom2_1_666_iwad[33];
    extern char known_md5_string_doom2_1_666g_iwad[33];
    extern char known_md5_string_doom2_1_7_iwad[33];
    extern char known_md5_string_doom2_1_7a_iwad[33];
    extern char known_md5_string_doom2_1_8_iwad[33];
    extern char known_md5_string_doom2_1_8f_iwad[33];
    extern char known_md5_string_doom2_1_9_iwad[33];
    extern char known_md5_string_doom2_bfg_xbox360_iwad[33];
    extern char known_md5_string_doom2_bfg_pc_iwad[33];
    extern char known_md5_string_doom2_xbox_iwad[33];
    extern char known_md5_string_final_doom_tnt_old_iwad[33];
    extern char known_md5_string_final_doom_tnt_new_iwad[33];
    extern char known_md5_string_final_doom_plutonia_old_iwad[33];
    extern char known_md5_string_final_doom_plutonia_new_iwad[33];
    extern char known_md5_string_freedoom_0_6_4_iwad[33];
    extern char known_md5_string_freedoom_0_7_rc_1_beta_1_iwad[33];
    extern char known_md5_string_freedoom_0_7_iwad[33];
    extern char known_md5_string_freedoom_0_8_beta_1_iwad[33];
    extern char known_md5_string_freedoom_0_8_iwad[33];
    extern char known_md5_string_freedoom_0_8_phase_1_iwad[33];
    extern char known_md5_string_freedoom_0_8_phase_2_iwad[33];
    extern char known_md5_string_hacx_share_1_0_iwad[33];
    extern char known_md5_string_hacx_reg_1_0_iwad[33];
    extern char known_md5_string_hacx_reg_1_1_iwad[33];
    extern char known_md5_string_hacx_reg_1_2_iwad[33];
    extern char known_md5_string_nerve_bfg_pwad[33];
    extern char known_md5_string_nerve_xbox360_pwad[33];

    int         i, c;

    extern int  extra_wad_slot_1_loaded;
    extern int  extra_wad_slot_2_loaded;
    extern int  extra_wad_slot_3_loaded;
    extern int  fsize;
    extern int  fsizecq;

    FILE        *file;

    fatFile     *fileList = NULL;

    u32         fileCnt;

    s32         ret;
    s32         selected = 0;
    s32         start = 0;

    char        *tmpPath = malloc (MAX_FILE_PATH_LEN);

    // wiiNinja: check for malloc error
    if (tmpPath == NULL)
    {
        // What am I gonna use here?
        ret = -997;

        printf("  Error! Out of memory (ret = %d)\n", ret);

        return;
    }

    printf("  Opening file list...");

    fflush(stdout);

    gDirLevel = 0;

    // wiiNinja: The root is always the primary folder
    // But if the user has a /wad directory, just go there. This makes
    // both sides of the argument win
    sprintf(tmpPath, "%s:" WAD_DIRECTORY, fdev->mount);

    PushCurrentDir(tmpPath, 0, 0);

    if (strcmp (WAD_DIRECTORY, gConfig.startupPath) != 0)
    {
        sprintf(tmpPath, "%s:/apps/wiidoom/", fdev->mount);

        // wiiNinja
        PushCurrentDir(tmpPath, 0, 0);
    }

    // Retrieve filelist

    getList:

    if (fileList)
    {
        free (fileList);

        fileList = NULL;
    }

    ret = __Menu_RetrieveList(tmpPath, &fileList, &fileCnt);

    if (ret < 0)
    {
        printf("  Error! (ret = %d)\n", ret);

        goto err;
    }

    // No files
    if (!fileCnt)
    {
        printf("  No files found!\n");

        goto err;
    }

    // Set install-values to 0 - Leathl
    int counter;

    for (counter = 0; counter < fileCnt; counter++)
    {
        fatFile *file = &fileList[counter];

        file->install = 0;
    }

    for ( ; ; )
    {
        u32 cnt;

        s32 index;

        // Clear console
        Con_Clear();

        // Draw main title
        drawMain();

        JPEGIMG about;

        memset(&about, 0, sizeof(JPEGIMG));

        about.inbuffer = picdata;
        about.inbufferlength = piclength;

        JPEG_Decompress(&about);

        display_jpeg(about, 0, 300);

        //* Print entries *
        cnt = strlen(tmpPath);

        if(cnt > 30)
            index = cnt - 30;
        else
            index = 0;

        // Print entries
        for (cnt = start; cnt < fileCnt; cnt++)
        {
            fatFile *file = &fileList[cnt];

            // Entries per page limit
            if ((cnt - start) >= ENTRIES_PER_PAGE)
                break;

            // Only 40 chars to fit the screen
            M_StringCopy(str, file->filename, sizeof(str));

            str[40] = 0;

            // wiiNinja
            if (file->entry.d_type == DT_DIR)
            {
                printf("%2s[%.27s]\n", (cnt == selected) ? ">>" : "  ", str);
            }
            else
            {
                // REQUIRED FOR WAD SUPPORT MSG
                if(cnt == selected && file->entry.d_type != DT_DIR)
                {
                    M_StringCopy(path_tmp, tmpPath, sizeof(path_tmp));
                    strcat(path_tmp, file->filename);
                }
                printf("%2s%.27s\n", (cnt == selected) ? ">>" : "  ", str);
            }
        }

        printStyledText(5, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");

        printStyledText(5, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "Loc.:");
        printStyledText(5, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,tmpPath);

        printStyledText(6, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");

//        if(multiplayer)    // MAIN FLAG
        {
            printStyledText(6, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
//                        "NET.:");
                        "MERGE:");

//            if(multiplayer_flag)
            if(merge)
                printStyledText(10, 75,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YES");
            else
                printStyledText(10, 75,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        " NO");
        }

        printStyledText(6, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "IWAD: ");
        printStyledText(6, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,stripped_target);
        printStyledText(6, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");
        printStyledText(7, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "PWAD1: ");
        printStyledText(7, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,stripped_extra_wad_1);
        printStyledText(7, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");
        printStyledText(8, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "PWAD2: ");
        printStyledText(8, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,stripped_extra_wad_2);
        printStyledText(8, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");
        printStyledText(9, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "PWAD3: ");
        printStyledText(9, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,stripped_extra_wad_3);
        printStyledText(9, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");

        if(extra_wad_loaded || load_dehacked || md5_check)
        {
            printStyledText(9, 18,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "X: Clear Sel.");
            printStyledText(9, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  Y: Merge  WAD");
            printStyledText(9, 16,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "/");
        }

        printStyledText(10, 35,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        ".DEH: ");
        printStyledText(10, 68,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "MERGE: ");
        printStyledText(10, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,stripped_dehacked_file);
        printStyledText(10, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");

        printf("\n");

        printStyledText(10, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  A: Select WAD");
        printStyledText(10, 16,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "/");

        if(gDirLevel>1)
            printStyledText(10, 18,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "B: Prev. dir.");
        else
            printStyledText(10, 18,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "B: Sel. dev.");

        printStyledText(10, 33,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "|");
        printStyledText(11, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  ----------------------------------------------------------------------------  ");
        printStyledText(12, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  WARNING: LOADING IWAD'S AS PWAD, VICE VERSA OR EVEN NON-DOOM WADS MAY CRASH!");
        printStyledText(13, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  ----------------------------------------------------------------------------  ");

        if(fsize == 4261144)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 BETA v1.4 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4271324)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 BETA v1.5 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4211660)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 BETA v1.6 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 10396254)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.1 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 10399316)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.2 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 10401760)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.6 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 11159840)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.8 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 12408292)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.9UD DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 12474561)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED BFG-PSN DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 12487824)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 REGISTERED BFG-PC DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 4207819)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.0 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4274218)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.1 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4225504)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.2 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4225460)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 SW. v1.25 Sybex DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4234124)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.666 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 4196020)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.8 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
            printStyledText(16, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
        }
        else if(fsize == 14943400)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.666 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14824716)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.666G DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14612688)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.7 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14607420)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.8F DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14604584)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.9 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14677988)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED XBOX360 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14691821)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED BFG-PC DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 14683458)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: DOOM 2 REGISTERED XBOX DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 18195736)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: FINAL DOOM TNT");
            printStyledText(14, 36,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(OLD)");
            printStyledText(14, 42,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 18654796)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: FINAL DOOM TNT");
            printStyledText(14, 36,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(NEW)");
            printStyledText(14, 42,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 18240172)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: FINAL DOOM PLUTONIA");
            printStyledText(14, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(NEW)");
            printStyledText(14, 47,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 17420824)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DOOM IWAD VERSION: FINAL DOOM PLUTONIA");
            printStyledText(14, 41,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(OLD)");
            printStyledText(14, 47,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 19801320)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.6.4 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 27704188)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.7 RC 1 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 27625596)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.7 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 28144744)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.8 BETA 1 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 28592816)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.8 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 19362644)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.8 PHASE 1 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 28422764)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "FREEDOOM IWAD VERSION: FREEDOOM v0.8 PHASE 2 DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 12361532)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "CHEX QUEST IWAD VERSION: CHEX QUEST DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
        }
        else if(fsize == 9745831)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "HACX IWAD VERSION: HACX v1.0 SHAREWARE DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 21951805)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "HACX IWAD VERSION: HACX v1.0 REGISTERED DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 22102300)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "HACX IWAD VERSION: HACX v1.1 REGISTERED DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS NOT SUPPORTED)");
        }
        else if(fsize == 19321722)
        {
            printStyledText(14, 2,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "HACX IWAD VERSION: HACX v1.2 REGISTERED DETECTED");
            printStyledText(14, 55,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "(THIS WAD IS SUPPORTED)");
    }
    printStyledText(14, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,iwad_ver);
    printStyledText(15, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  ----------------------------------------------------------------------------  ");
    printStyledText(16, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,
                        &stTexteLocation,shareware_warn);
    printStyledText(17, 0,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "  ----------------------------------------------------------------------------  ");
    if (selected < fileCnt - 1)
        printStyledText(8, 32,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "+");

    if(selected > 0)
        printStyledText(5, 32,CONSOLE_FONT_BLACK,
                        CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,
                        &stTexteLocation,
                        "-");

    //* Controls *
    u32 buttons = WaitButtons();

    // DPAD buttons
    if (buttons & WPAD_CLASSIC_BUTTON_UP)
    {
        selected--;

        if (selected <= -1)
            selected = 0;
    }

    if (buttons & WPAD_CLASSIC_BUTTON_LEFT)
    {
        selected = selected + ENTRIES_PER_PAGE;

        if (selected >= fileCnt || selected == 0)
            selected = 0;
    }

    if (buttons & WPAD_CLASSIC_BUTTON_DOWN)
    {
        selected ++;

        if (selected >= fileCnt)
            selected = fileCnt - 1;
    }

    if (buttons & WPAD_CLASSIC_BUTTON_RIGHT)
    {
        selected = selected - ENTRIES_PER_PAGE;

        if (selected <= -1 || selected == fileCnt - 1)
            selected = fileCnt - 1;
    }

    // HOME button
    if (buttons & WPAD_CLASSIC_BUTTON_HOME)
        Restart();

    // 1 Button - Leathl
    if (buttons & WPAD_CLASSIC_BUTTON_PLUS)
    {
        // Clear console
        Con_Clear();

        // START DOOM
        D_DoomMain();
    }

    // A button
    if (buttons & WPAD_CLASSIC_BUTTON_A)
    {
        fatFile *tmpFile = &fileList[selected];

        // DO NOT INITIALIZE TO "NULL": CRASHES THE WII IF SELECTING A WAD
        char *tmpCurPath;

        if (tmpFile->entry.d_type==DT_DIR) // wiiNinja
        {
            if (strcmp (tmpFile->filename, "..") == 0)
            {
                selected = 0;

                start = 0;

                // Previous dir
                tmpCurPath = PopCurrentDir(&selected, &start);

                if (tmpCurPath != NULL)
                    sprintf(tmpPath, "%s", tmpCurPath);

                goto getList;
            }
            else
            {
                tmpCurPath = PeekCurrentDir ();

                if (tmpCurPath != NULL)
                {
                    if(gDirLevel>1)
                        sprintf(tmpPath, "%s%s", tmpCurPath, tmpFile->filename);
                    else
                        sprintf(tmpPath, "%s%s", tmpCurPath, tmpFile->filename);

                    strcat(tmpPath, "/");
                }

                // wiiNinja: Need to PopCurrentDir
                PushCurrentDir (tmpPath, selected, start);

                selected = 0;

                start = 0;

                goto getList;
            }
        }
        else
        {
            M_StringCopy(check, tmpPath, sizeof(check));
            strcat(check, tmpFile->filename);

            MD5_Check(check);

            if (strncmp(calculated_md5_string,
                        known_md5_string_chex_quest_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 12361532;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_beta_1_4_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4261144;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_beta_1_5_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4271324;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_beta_1_6_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4211660;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_0_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4207819;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_1_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4274218;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_2_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4225504;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_25s_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4225460;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_666_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4234124;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_8_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 4196020;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_share_1_9_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 20428208;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_1_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 10396254;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_2_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 10399316;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_6_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 10401760;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_6b_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 20428208;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_666_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 20428208;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_8_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 11159840;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_9_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 20428208;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_reg_1_9ud_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 12408292;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_bfg_psn_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 12474561;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_bfg_pc_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 12487824;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom_xbox_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 12538385;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_666_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14943400;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_666g_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14824716;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_7_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14612688;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_7a_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 20428208;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_8_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 20428208;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_8f_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14607420;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_1_9_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14604584;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_bfg_xbox360_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14677988;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_bfg_pc_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14691821;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_doom2_xbox_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 14683458;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_final_doom_tnt_old_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 18195736;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_final_doom_tnt_new_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 18654796;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_final_doom_plutonia_old_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 17420824;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_final_doom_plutonia_new_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 18240172;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_6_4_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 19801320;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_7_rc_1_beta_1_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 27704188;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_7_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 27625596;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_8_beta_1_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 28144744;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_8_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 28592816;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_8_phase_1_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 19362644;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_freedoom_0_8_phase_2_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 28422764;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_hacx_share_1_0_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 9745831;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_hacx_reg_1_0_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 21951805;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_hacx_reg_1_1_iwad, 32) == 0)
            {
//
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 21951805;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_hacx_reg_1_2_iwad, 32) == 0)
            {
                M_StringCopy(target, check, sizeof(target));
                M_StringCopy(stripped_target, tmpFile->filename, sizeof(stripped_target));

                fsize = 19321722;

                md5_check = true;
                nerve_pwad = false;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_nerve_bfg_pwad, 32) == 0)
            {
                load_extra_wad = 1;

                M_StringCopy(extra_wad_1, check, sizeof(extra_wad_1));
                M_StringCopy(stripped_extra_wad_1, tmpFile->filename, sizeof(stripped_extra_wad_1));

                extra_wad_slot_1_loaded = 1;

                md5_check = true;
                nerve_pwad = true;
            }
            else if (strncmp(calculated_md5_string,
                        known_md5_string_nerve_xbox360_pwad, 32) == 0)
            {
                load_extra_wad = 1;

                M_StringCopy(extra_wad_1, check, sizeof(extra_wad_1));
                M_StringCopy(stripped_extra_wad_1, tmpFile->filename, sizeof(stripped_extra_wad_1));

                extra_wad_slot_1_loaded = 1;

                md5_check = true;
                nerve_pwad = true;
            }
            else
                md5_check = false;

            file = fopen(check, "r");

            if (file != NULL && !md5_check)
            {
                for (i = 0; i < 4; i++)
                {
                    // Get character
                    c = fgetc(file);

                    // Store characters in array
                    buffer[i] = c;

                    if (strncmp(iwad_term, buffer, 4) == 0)
                    {
                        if(extra_wad_slot_1_loaded == 0)
                        {
                            load_extra_wad = 1;

                            M_StringCopy(extra_wad_1, check, sizeof(extra_wad_1));
                            M_StringCopy(stripped_extra_wad_1, tmpFile->filename,
                                    sizeof(stripped_extra_wad_1));

                            extra_wad_slot_1_loaded = 1;

                            extra_wad_loaded = 1;

                            break;
                        }
                        else if(extra_wad_slot_1_loaded == 1 &&
                                extra_wad_slot_2_loaded == 0)
                        {
                            if(strcmp(check, extra_wad_1) != 0)
                            {
                                load_extra_wad = 1;

                                M_StringCopy(extra_wad_2, check, sizeof(extra_wad_2));
                                M_StringCopy(stripped_extra_wad_2, tmpFile->filename,
                                        sizeof(stripped_extra_wad_2));

                                extra_wad_slot_2_loaded = 1;

                                extra_wad_loaded = 1;
                            }
                            break;
                        }
                        else if(extra_wad_slot_1_loaded == 1 &&
                                extra_wad_slot_2_loaded == 1 &&
                                extra_wad_slot_3_loaded == 0)
                        {
                            if((strcmp(check, extra_wad_1) != 0 &&
                                strcmp(check, extra_wad_2) != 0))
                            {
                                load_extra_wad = 1;

                                M_StringCopy(extra_wad_3, check, sizeof(extra_wad_3));
                                M_StringCopy(stripped_extra_wad_3, tmpFile->filename,
                                        sizeof(stripped_extra_wad_3));

                                extra_wad_slot_3_loaded = 1;

                                extra_wad_loaded = 1;
                            }
                            break;
                        }
                    }
                    else if (strncmp(pwad_term, buffer, 4) == 0 &&
                             extra_wad_slot_1_loaded == 0)
                    {
                        load_extra_wad = 1;

                        M_StringCopy(extra_wad_1, check, sizeof(extra_wad_1));
                        M_StringCopy(stripped_extra_wad_1, tmpFile->filename,
                                sizeof(stripped_extra_wad_1));

                        extra_wad_slot_1_loaded = 1;

                        W_CheckSize(4);

                        if(fsizecq == 7585664)
                            is_chex_2 = true;

                        extra_wad_loaded = 1;

                        break;
                    }
                    else if (strncmp(pwad_term, buffer, 4) == 0 &&
                             extra_wad_slot_1_loaded == 1 &&
                             extra_wad_slot_2_loaded == 0)
                    {
                        if(strcmp(check, extra_wad_1) != 0)
                        {
                            load_extra_wad = 1;

                            M_StringCopy(extra_wad_2, check, sizeof(extra_wad_2));
                            M_StringCopy(stripped_extra_wad_2, tmpFile->filename,
                                    sizeof(stripped_extra_wad_2));

                            extra_wad_slot_2_loaded = 1;

                            W_CheckSize(4);

                            if(fsizecq == 7585664)
                                is_chex_2 = true;

                            extra_wad_loaded = 1;
                        }
                        break;
                    }
                    else if (strncmp(pwad_term, buffer, 4) == 0 &&
                             extra_wad_slot_1_loaded == 1 &&
                             extra_wad_slot_2_loaded == 1 &&
                             extra_wad_slot_3_loaded == 0)
                    {
                        if((strcmp(check, extra_wad_1) != 0 &&
                            strcmp(check, extra_wad_2) != 0))
                        {
                            load_extra_wad = 1;

                            M_StringCopy(extra_wad_3, check, sizeof(extra_wad_3));
                            M_StringCopy(stripped_extra_wad_3, tmpFile->filename,
                                    sizeof(stripped_extra_wad_3));

                            extra_wad_slot_3_loaded = 1;

                            W_CheckSize(4);

                            if(fsizecq == 7585664)
                                is_chex_2 = true;

                            extra_wad_loaded = 1;
                        }
                        break;
                    }
                    else if (strncmp(deh_term, buffer, 4) == 0) 
                    {
                        load_dehacked = 1;

                        M_StringCopy(dehacked_file, check, sizeof(dehacked_file));
                        M_StringCopy(stripped_dehacked_file, tmpFile->filename,
                                sizeof(stripped_dehacked_file));

                        break;
                    }
                }
            }
            memset(buffer, 0, sizeof(buffer));

            fclose(file);
        }
    }

    // B button
    if (buttons & WPAD_CLASSIC_BUTTON_B)
    {
        if(gDirLevel <= 1)
        {
            return;
        }

        char *tmpCurPath;

        selected = 0;

        start = 0;

        // Previous dir
        tmpCurPath = PopCurrentDir(&selected, &start);

        if (tmpCurPath != NULL)
            sprintf(tmpPath, "%s", tmpCurPath);

        goto getList;
    }

    // X button
    if (buttons & WPAD_CLASSIC_BUTTON_X)
    {
        load_dehacked = 0;
        load_extra_wad = 0;

        M_StringCopy(stripped_dehacked_file, "", sizeof(stripped_dehacked_file));
        M_StringCopy(stripped_extra_wad_1, "", sizeof(stripped_extra_wad_1));
        M_StringCopy(stripped_extra_wad_2, "", sizeof(stripped_extra_wad_2));
        M_StringCopy(stripped_extra_wad_3, "", sizeof(stripped_extra_wad_3));
        M_StringCopy(stripped_target, "", sizeof(stripped_target));

        M_StringCopy(dehacked_file, "", sizeof(dehacked_file));
        M_StringCopy(extra_wad_1, "", sizeof(extra_wad_1));
        M_StringCopy(extra_wad_2, "", sizeof(extra_wad_2));
        M_StringCopy(extra_wad_3, "", sizeof(extra_wad_3));
        M_StringCopy(target, "", sizeof(target));

        extra_wad_slot_1_loaded = 0;
        extra_wad_slot_2_loaded = 0;
        extra_wad_slot_3_loaded = 0;

        extra_wad_loaded = 0;
    }

    // Y button
    if (buttons & WPAD_CLASSIC_BUTTON_Y)
    {
        if (!merge)
            merge = true;
        else if(merge)
            merge = false;
    }

    // List scrolling
    index = (selected - start);

    if (index >= ENTRIES_PER_PAGE)
        start += index - (ENTRIES_PER_PAGE - 1);

    if (index <= -1)
        start += index;
    }

    err:

    printf("\n");
    printf("\n  Press any key...               |\n");

    free (tmpPath);

    // Wait for button
    WaitButtons();
}

void Menu_Loop(void)
{
    Initialise();

    for ( ; ; )
    {
        // FAT device menu
        Menu_FatDevice();

        // WAD list menu
        Menu_WadList();
    }
}

