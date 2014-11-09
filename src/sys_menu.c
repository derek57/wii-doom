#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <ogc/pad.h>
#include <wiilight.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "sys_fat.h"
#include "sys_nand.h"
#include "sys_usbstorage.h"
#include "sys_globals.h"

#include "video.h"
#include "d2x-cios-installer.h"
#include "gui.h"

/* Macros */
#define NB_FAT_DEVICES	(sizeof(fdevList) / sizeof(fatDevice))
#define MAXPATH		0x108
#define MAX_WIIMOTES	4

CONFIG gConfig;

int	is_chex_2 = 0;
int	extra_wad_loaded = 0;
int	load_extra_wad = 0;
int	load_dehacked = 0;

bool	multiplayer = false;
bool	multiplayer_flag = false;

// Local prototypes: wiiNinja
void WaitPrompt (char *prompt);
void drawMain();
void D_DoomMain (void);
void W_CheckSize(int wad);
//void WiiLightControl (int state);
int PushCurrentDir(char *dirStr, s32 Selected, s32 Start);
char *PopCurrentDir(s32 *Selected, s32 *Start);
char *PeekCurrentDir (void);
bool IsListFull (void);
u32 WaitButtons(void);
u32 Pad_GetButtons(void);
u32 Wpad_HeldButtons(void);

bool Wpad_TimeButton(void)
{
    u32 buttons = 1;
	
    time_t start,end;
    time (&start);

    int dif;

    /* Wait for button pressing */
    while (buttons)
    {
	buttons = Wpad_HeldButtons();

	VIDEO_WaitVSync();

	time (&end);

	dif = difftime (end,start);

	if(dif>=2)
	    return true;
    }
    return false;
}

u32 Wpad_GetButtons(void)
{
    u32 buttons = 0, cnt;

    /* Scan pads */
    WPAD_ScanPads();

    /* Get pressed buttons */
    for (cnt = 0; cnt < MAX_WIIMOTES; cnt++)
	buttons |= WPAD_ButtonsDown(cnt);

    return buttons;
}

void Sys_LoadMenu(void)
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
	    HBC=1; // Exit to HBC

    /* Homebrew Channel stub */
    if (HBC == 1)
	exit(0);

    /* Return to the Wii system menu */
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

void Con_Clear(void)
{
    /* Clear console */
    printf("\x1b[2J");

    fflush(stdout);
}

void Restart(void)
{
    Con_Clear ();

    fflush(stdout);

    /* Load system menu */
    Sys_LoadMenu();
}

/* FAT device list  */
fatDevice fdevList[] = {
    { "sd",	"    SD-Card    ",		&__io_wiisd },
    { "usb",	"  USB-Storage  ",		&__io_usbstorage },
    { "usb2",	"USB 2.0 Storage",		&__io_wiiums },
    { "gcsda",	"  SD-Gecko (A) ",		&__io_gcsda },
    { "gcsdb",	"  SD-Gecko (B) ",		&__io_gcsdb },
};

// wiiNinja: Define a buffer holding the previous path names as user
// traverses the directory tree. Max of 10 levels is define at this point
static fatDevice  *fdev = NULL;
static u8 gDirLevel = 0;
static char gDirList [MAX_DIR_LEVELS][MAX_FILE_PATH_LEN];
static s32  gSeleted[MAX_DIR_LEVELS];
static s32  gStart[MAX_DIR_LEVELS];
char gFileName[MAX_FILE_PATH_LEN];
char gTmpFilePath[MAX_FILE_PATH_LEN];

s32 __Menu_IsGreater(const void *p1, const void *p2)
{
    u32 n1 = *(u32 *)p1;
    u32 n2 = *(u32 *)p2;

    /* Equal */
    if (n1 == n2)
	return 0;

    return (n1 > n2) ? 1 : -1;
}


s32 __Menu_EntryCmp(const void *p1, const void *p2)
{
    fatFile *f1 = (fatFile *)p1;
    fatFile *f2 = (fatFile *)p2;

    /* Compare entries */ // wiiNinja: Include directory
    if ((f1->entry.d_type==DT_DIR) && !(f2->entry.d_type==DT_DIR))
        return (-1);
    else if (!(f1->entry.d_type==DT_DIR) && (f2->entry.d_type==DT_DIR))
        return (1);
    else
        return strcmp(f1->filename, f2->filename);
}

s32 __Menu_RetrieveList(char *inPath, fatFile **outbuf, u32 *outlen)
{
    fatFile  *buffer = NULL;

    DIR *dir = NULL;
	
    struct dirent *entry;

    //char dirpath[256], filename[768];
    u32  cnt = 0;

    /* Open directory */
    dir = opendir(inPath);

    if (!dir)
	return -1;

    while ((entry = readdir(dir)))
	cnt++;

    if (cnt > 0)
    {
	/* Allocate memory */
	buffer = malloc(sizeof(fatFile) * cnt);

	if (!buffer)
	{
	    closedir(dir);

	    return -2;
	}

	/* Reset directory */
	closedir(dir);

	dir = opendir(inPath);

	/* Get entries */
	for (cnt = 0; (entry = readdir(dir));)
	{
	    bool addFlag = false;

	    if (entry->d_type==DT_DIR) 
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
		if(strlen(entry->d_name)>4)
		{
		    if (!stricmp(entry->d_name+strlen(entry->d_name)-4, ".wad") ||
			!stricmp(entry->d_name+strlen(entry->d_name)-4, ".deh"))
			    addFlag = true;
		}
	    }

            if (addFlag == true)
            {
		fatFile *file = &buffer[cnt++];

		/* File name */
		strcpy(file->filename, entry->d_name);

		/* File stats */
		file->entry = *entry;
	    }
	}
	/* Sort list */
	qsort(buffer, cnt, sizeof(fatFile), __Menu_EntryCmp);
    }

    /* Close directory */
    closedir(dir);

    /* Set values */
    *outbuf = buffer;
    *outlen = cnt;

    return 0;
}

void Menu_FatDevice(void)
{
    s32 ret, selected = 0;

    /* Select source device */
    if (gConfig.fatDeviceIndex < 0)
    {
	for (;;)
	{
	    /* Clear console */
	    Con_Clear();

	    /* Draw main title */
	    drawMain();

	    /* Selected device */
	    fdev = &fdevList[selected];

//	    printf(">>Source: < %.30s >"/*, 132*/, fdev->name);
	    printf(">>Source: < %s >"/*, 132*/, fdev->name);
	    printf("    |");
	    printf("\n                                 |");
	    printf("\n                                 |");
	    printStyledText(7, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"IWAD: ");
	    printf("\n  L / R: Change device.          |"/*, 132*/);
	    printStyledText(8, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"PWAD: ");
	    printf("\n                                 |\n");
	    printStyledText(9, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,".DEH: ");
	    printf("\n  A: continue.                   |");
	    printf("\n  Home: Quit.                    |\n");
	    printStyledText(11, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");

	    u32 buttons = WaitButtons();

	    /* LEFT/RIGHT buttons */
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

	    /* HOME button */
	    if (buttons & WPAD_CLASSIC_BUTTON_HOME)
		Restart();

	    /* A button */
	    if (buttons & WPAD_CLASSIC_BUTTON_B)
		break;
	}
    }
    else
    {
	sleep(3/*5*/);

	fdev = &fdevList[gConfig.fatDeviceIndex];
    }

    /* Clear console */
    Con_Clear();

    /* Draw main title */
    drawMain();

//    printf("  Loading...: %.30s", fdev->name );
    printf("  Loading...: %s", fdev->name );
    printf("    |");

    fflush(stdout);

    /* Mount FAT device */
    ret = Fat_Mount(fdev);

    if (ret < 0)
    {
	printf("\n                                 |");
	printStyledText(7, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"IWAD: ");
	printf("\n");
	printStyledText(7, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"  Error! (ret = %d)", ret);
	printf("              |");
	printf("\n                                 |");
	printStyledText(8, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"PWAD: ");
	printf("\n                                 |");
	printStyledText(9, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,".DEH: ");

	goto err;
    }
    else
	printf("  OK!                              |");

    return;

    err:

    if(gConfig.fatDeviceIndex >= 0) gConfig.fatDeviceIndex = -1;

//	WiiLightControl (WII_LIGHT_OFF);

	printf("\n  Press any key...               |\n"/*, 129*/);
	printStyledText(11, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");

	WaitButtons();

	/* Prompt menu again */
	Menu_FatDevice();
}

int MD5_Check(char *final);

void Menu_WadList(void)
{
    bool	md5_check = false;

    char	line[BUFSIZ];
    char	check[MAXPATH];
    char	iwad_term[] = "IWAD";
    char	pwad_term[] = "PWAD";
    char	deh_term[] = "Patch File for DeHackEd";
    char	str [100];
    char	stripped_target[MAXPATH] = "";
    char	stripped_extra_wad[256] = "";
    char	stripped_dehacked_file[256] = "";
    char	*temp;
    char	*tmpPath = malloc (MAX_FILE_PATH_LEN);

    const char	*iwad_ver = NULL;
    const char	*shareware_warn = NULL;

    extern char	path_tmp[MAXPATH];
    extern char	target[MAXPATH];
    extern char	extra_wad[256];
    extern char	dehacked_file[256];
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
    extern char known_md5_string_doom2_1_666_iwad[33];
    extern char known_md5_string_doom2_1_666g_iwad[33];
    extern char known_md5_string_doom2_1_7_iwad[33];
    extern char known_md5_string_doom2_1_7a_iwad[33];
    extern char known_md5_string_doom2_1_8_iwad[33];
    extern char known_md5_string_doom2_1_8f_iwad[33];
    extern char known_md5_string_doom2_1_9_iwad[33];
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
/*
    int		installCnt = 0;
    int		uninstallCnt = 0;
    int		WADLoaded = 0;
    int		txtLoaded = 0;
*/
    extern int	fsize;
    extern int	fsizecq;
//    extern int	fsizerw;

    FILE	*file;

    fatFile	*fileList = NULL;

    u32		fileCnt;

    s32		ret;
    s32		selected = 0;
    s32		start = 0;

    // wiiNinja: check for malloc error
    if (tmpPath == NULL)
    {
        ret = -997; // What am I gonna use here?

	printf("  Error! Out of memory (ret = %d)\n", ret);

        return;
    }

    printf("  Opening file list..."/*, 153*/);

    fflush(stdout);

    gDirLevel = 0;

    // wiiNinja: The root is always the primary folder
    // But if the user has a /wad directory, just go there. This makes
    // both sides of the argument win
    sprintf(tmpPath, "%s:" WAD_DIRECTORY, fdev->mount);

    PushCurrentDir(tmpPath,0,0);

    if (strcmp (WAD_DIRECTORY, gConfig.startupPath) != 0)
    {
	sprintf(tmpPath, "%s:/%s", fdev->mount, gConfig.startupPath);

	PushCurrentDir(tmpPath,0,0); // wiiNinja
    }

    /* Retrieve filelist */

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

    /* No files */
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

    for (;;)
    {
	u32 cnt;

	s32 index;

	/* Clear console */
	Con_Clear();

	/* Draw main title */
	drawMain();

	/** Print entries **/
	cnt = strlen(tmpPath);

	if(cnt>30)
	    index = cnt-30;
	else
	    index = 0;

	/* Print entries */
	for (cnt = start; cnt < fileCnt; cnt++)
	{
	    fatFile *file     = &fileList[cnt];

	    /* Entries per page limit */
	    if ((cnt - start) >= ENTRIES_PER_PAGE)
		break;

	    strncpy(str, file->filename, 40); //Only 40 chars to fit the screen

	    str[40]=0;

            if (file->entry.d_type==DT_DIR) // wiiNinja
	    {
		printf("%2s[%.27s]\n", (cnt == selected) ? ">>" : "  ", str);
/*
		if(cnt == selected)
		    fsize = 0;
*/
	    }
	    else
	    {
		if(cnt == selected && file->entry.d_type != DT_DIR)	// REQUIRED FOR WAD SUPPORT MSG
		{
		    strcpy(path_tmp, tmpPath);
		    strcat(path_tmp, file->filename);

//		    W_CheckSize(1);
		}
		printf("%2s%.27s\n", (cnt == selected) ? ">>" : "  ", str);
//		printf("%2s%s%.27s\n", (cnt == selected) ? ">>" : "  ", (file->install == 1) ? "+" : ((file->install == 2) ? "-" : ((file->install == 3) ? "p" : " ")), str);
	    }
	}

	printStyledText(5, 33,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"|");

	printStyledText(5, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"Loc.:");
	printStyledText(5, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,tmpPath);

	printStyledText(6, 33,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"|");

	if(multiplayer)	// MAIN FLAG
	{
	    printStyledText(6, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"NET.:");

	    if(multiplayer_flag)
		printStyledText(6, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"YES");
	    else
		printStyledText(6, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"NO");
	}

	printStyledText(7, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"IWAD: ");
	printStyledText(7, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,stripped_target);
	printStyledText(7, 33,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"|");
	printStyledText(8, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"PWAD: ");
	printStyledText(8, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,stripped_extra_wad);
	printStyledText(8, 33,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"|");
	printStyledText(9, 35,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,".DEH: ");
	printStyledText(9, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,stripped_dehacked_file);
	printStyledText(9, 33,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"|");

	printf("\n");

	printStyledText(10, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  A: Select WAD");
	printStyledText(10, 16,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"/");

	if(gDirLevel>1)
	    printStyledText(10, 18,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"B: Prev. dir.");
	else
	    printStyledText(10, 18,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"B: Sel. dev.");

	printStyledText(10, 33,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"|");
	printStyledText(11, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");
	printStyledText(12, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"  WARNING: LOADING IWAD'S AS PWAD, VICE VERSA OR EVEN NON-DOOM WADS MAY CRASH!");
	printStyledText(13, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");

	if(fsize == 4261144)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 BETA v1.4 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4271324)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 BETA v1.5 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4211660)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 BETA v1.6 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 10396254)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 REGISTERED v1.1 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 10399316)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 REGISTERED v1.2 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 10401760)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 REGISTERED v1.6 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 11159840)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 REGISTERED v1.8 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 12408292)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 REGISTERED v1.9UD DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 4207819)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.0 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4274218)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.1 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4225504)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.2 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4225460)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 SW. v1.25 Sybex DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4234124)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.666 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 4196020)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.8 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	    printStyledText(16, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSION OF DOOM. PLEASE REGISTER!");
	}
	else if(fsize == 14943400)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 2 REGISTERED v1.666 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 14824716)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 2 REGISTERED v1.666G DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 14612688)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 2 REGISTERED v1.7 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 14607420)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 2 REGISTERED v1.8F DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 14604584)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: DOOM 2 REGISTERED v1.9 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 18195736)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: FINAL DOOM TNT");
	    printStyledText(14, 36,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"(OLD)");
	    printStyledText(14, 42,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 18654796)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: FINAL DOOM TNT");
	    printStyledText(14, 36,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(NEW)");
	    printStyledText(14, 42,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 18240172)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: FINAL DOOM PLUTONIA");
	    printStyledText(14, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(NEW)");
	    printStyledText(14, 47,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 17420824)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DOOM IWAD VERSION: FINAL DOOM PLUTONIA");
	    printStyledText(14, 41,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"(OLD)");
	    printStyledText(14, 47,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 19801320)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.6.4 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 27704188)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.7 RC 1 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 27625596)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.7 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 28144744)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.8 BETA 1 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 28592816)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.8 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 19362644)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.8 PHASE 1 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 28422764)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"FREEDOOM IWAD VERSION: FREEDOOM v0.8 PHASE 2 DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 12361532)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"CHEX QUEST IWAD VERSION: CHEX QUEST DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
	else if(fsize == 9745831)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"HACX IWAD VERSION: HACX v1.0 SHAREWARE DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 21951805)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"HACX IWAD VERSION: HACX v1.0 REGISTERED DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 22102300)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"HACX IWAD VERSION: HACX v1.1 REGISTERED DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS NOT SUPPORTED)");
	}
	else if(fsize == 19321722)
	{
	    printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"HACX IWAD VERSION: HACX v1.2 REGISTERED DETECTED");
	    printStyledText(14, 55,CONSOLE_FONT_BLACK,CONSOLE_FONT_GREEN,CONSOLE_FONT_BOLD,&stTexteLocation,"(THIS WAD IS SUPPORTED)");
	}
/*
	else if(fsize != 4261144  && fsize != 4271324  && fsize != 4211660  && fsize != 10396254 &&
		fsize != 10399316 && fsize != 10401760 && fsize != 11159840 && fsize != 12408292 &&
		fsize != 4207819  && fsize != 4274218  && fsize != 4225504  && fsize != 4225460  &&
		fsize != 4234124  && fsize != 4196020  && fsize != 14943400 && fsize != 14824716 &&
		fsize != 14612688 && fsize != 14607420 && fsize != 14604584 && fsize != 18195736 &&
		fsize != 18654796 && fsize != 18240172 && fsize != 17420824 && fsize != 28422764 &&
		fsize != 12361532 && fsize != 9745831  && fsize != 21951805 && fsize != 22102300 &&
		fsize != 19321722 && fsize != 27625596 && fsize != 27704188 && fsize != 28592816 &&
		fsize != 28144744 && fsize != 19801320 && fsize != 19362644 && fsize != 0)
	{
	    fatFile *file     = &fileList[cnt];

	    if(file->entry.d_type != DT_DIR)
		printStyledText(14, 2,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"!!! WARNING: THIS WAD CAN'T BE USED AS A MAIN GAME IWAD  -  UNKNOWN FILE !!!");
	}
*/
	printStyledText(14, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,iwad_ver);
	printStyledText(15, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");
	printStyledText(16, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,shareware_warn);
	printStyledText(17, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");
/*
	// ONLY FOR DEBUGGING STUFF
	if	(strncmp(calculated_md5_string, known_md5_string_chex_quest_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_beta_1_4_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_beta_1_5_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_beta_1_6_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_0_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_1_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_2_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_25s_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_666_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_8_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_share_1_9_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_1_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_2_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_6_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_6b_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_666_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_8_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_9_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom_reg_1_9ud_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_666_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_666g_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_7_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_7a_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_8_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_8f_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_doom2_1_9_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_final_doom_tnt_old_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_final_doom_tnt_new_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_final_doom_plutonia_old_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_final_doom_plutonia_new_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_6_4_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_7_rc_1_beta_1_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_7_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_beta_1_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_phase_1_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_phase_2_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_hacx_share_1_0_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_hacx_reg_1_0_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_hacx_reg_1_1_iwad, 32) == 0 ||
		 strncmp(calculated_md5_string, known_md5_string_hacx_reg_1_2_iwad, 32) == 0)
	    printStyledText(19, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"MD5 MATCH!");
	else
	    printStyledText(19, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"MD5 FAIL!");

	if(fsize == 12361532)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_chex_quest_iwad);
	else if(fsize == 4261144)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_beta_1_4_iwad);
	else if(fsize == 4271324)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_beta_1_5_iwad);
	else if(fsize == 4211660)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_beta_1_6_iwad);
	else if(fsize == 4207819)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_share_1_0_iwad);
	else if(fsize == 4274218)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_share_1_1_iwad);
	else if(fsize == 4225504)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_share_1_2_iwad);
	else if(fsize == 4225460)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_share_1_25s_iwad);
	else if(fsize == 4234124)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_share_1_666_iwad);
	else if(fsize == 4196020)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_share_1_8_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_doom_share_1_9_iwad);
	else if(fsize == 10396254)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_reg_1_1_iwad);
	else if(fsize == 10399316)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_reg_1_2_iwad);
	else if(fsize == 10401760)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_reg_1_6_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_doom_reg_1_6b_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_doom_reg_1_666_iwad);
	else if(fsize == 11159840)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_reg_1_8_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_doom_reg_1_9_iwad);
	else if(fsize == 12408292)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom_reg_1_9ud_iwad);
	else if(fsize == 14943400)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom2_1_666_iwad);
	else if(fsize == 14824716)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom2_1_666g_iwad);
	else if(fsize == 14612688)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom2_1_7_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_doom2_1_7a_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_doom2_1_8_iwad);
	else if(fsize == 14607420)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom2_1_8f_iwad);
	else if(fsize == 14604584)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_doom2_1_9_iwad);
	else if(fsize == 18195736)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_final_doom_tnt_old_iwad);
	else if(fsize == 18654796)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_final_doom_tnt_new_iwad);
	else if(fsize == 17420824)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_final_doom_plutonia_old_iwad);
	else if(fsize == 18240172)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_final_doom_plutonia_new_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_freedoom_0_6_4_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_freedoom_0_7_rc_1_beta_1_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_freedoom_0_7_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_freedoom_0_8_beta_1_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_freedoom_0_8_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_freedoom_0_8_phase_1_iwad);
	else if(fsize == 28422764)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_freedoom_0_8_phase_2_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_hacx_share_1_0_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_hacx_reg_1_0_iwad);
//	else if(fsize == )
//	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
//		    known_md5_string_hacx_reg_1_1_iwad);
	else if(fsize == 19321722)
	    printStyledText(21, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"known MD5 string: %s\n",
		    known_md5_string_hacx_reg_1_2_iwad);

	printStyledText(22, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"calculated MD5 string: %s\n", calculated_md5_string);
*/
	if (selected < fileCnt - 1)
	    printStyledText(8, 32,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"+");

	if(selected > 0)
	    printStyledText(5, 32,CONSOLE_FONT_BLACK,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"-");

	/** Controls **/
	u32 buttons = WaitButtons();

	/* DPAD buttons */
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

	/* HOME button */
	if (buttons & WPAD_CLASSIC_BUTTON_HOME)
	    Restart();
/*
	// Plus Button - Leathl
	if (buttons & WPAD_CLASSIC_BUTTON_PLUS)
	{
	    multiplayer_flag = true;
	}

	// Minus Button - Leathl
	if (buttons & WPAD_CLASSIC_BUTTON_MINUS)
	{
	    multiplayer_flag = false;
	}
*/
	// 1 Button - Leathl
	if (buttons & WPAD_CLASSIC_BUTTON_PLUS)
	{
	    /* Clear console */
	    Con_Clear();

	    /* START DOOM */
	    D_DoomMain();
	}

	// A button
	if (buttons & WPAD_CLASSIC_BUTTON_B)
	{
//	    WADLoaded = 0;

	    fatFile *tmpFile = &fileList[selected];

	    char *tmpCurPath/* = NULL*/;	// DO NOT ACTIVATE: CRASHES THE WII IF SELECTING A WAD

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
/*
		else if (IsListFull () == true)
		{
		    WaitPrompt ("reached maximum count of sub folders.\n");
		}
*/
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
/*
		    installCnt = 0;

		    uninstallCnt = 0;
*/
		    goto getList;
		}
	    }
	    else
	    {
		strcpy(check, tmpPath);
//		strcpy(check, tmpCurPath);
		strcat(check, tmpFile->filename);

		MD5_Check(check);

		if (strncmp(calculated_md5_string, known_md5_string_chex_quest_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 12361532;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_beta_1_4_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4261144;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_beta_1_5_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4271324;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_beta_1_6_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4211660;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_0_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4207819;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_1_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4274218;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_2_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4225504;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_25s_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4225460;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_666_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4234124;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_8_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 4196020;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_share_1_9_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 20428208;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_1_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 10396254;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_2_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 10399316;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_6_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 10401760;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_6b_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 20428208;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_666_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 20428208;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_8_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 11159840;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_9_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 20428208;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom_reg_1_9ud_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 12408292;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_666_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 14943400;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_666g_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 14824716;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_7_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 14612688;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_7a_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 20428208;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_8_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 20428208;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_8f_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 14607420;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_doom2_1_9_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 14604584;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_final_doom_tnt_old_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 18195736;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_final_doom_tnt_new_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 18654796;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_final_doom_plutonia_old_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 17420824;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_final_doom_plutonia_new_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 18240172;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_6_4_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 19801320;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_7_rc_1_beta_1_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 27704188;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_7_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 27625596;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_beta_1_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 28144744;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 28592816;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_phase_1_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 19362644;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_freedoom_0_8_phase_2_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 28422764;

		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_hacx_share_1_0_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 9745831;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_hacx_reg_1_0_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 21951805;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_hacx_reg_1_1_iwad, 32) == 0)
		{
/*
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 21951805;
*/
		    md5_check = true;
		}
		else if (strncmp(calculated_md5_string, known_md5_string_hacx_reg_1_2_iwad, 32) == 0)
		{
		    strcpy(target, check);
		    strcpy(stripped_target, tmpFile->filename);

		    fsize = 19321722;

		    md5_check = true;
		}
		else
		    md5_check = false;

		file = fopen(check, "r");

		if (file != NULL && !md5_check)
		{
		    while (fgets(line, sizeof(line), file))
		    {
			temp = line;

			if (strncmp(iwad_term, temp, 4) == 0)
			{
			    strcpy(extra_wad, check);
			    strcpy(stripped_extra_wad, tmpFile->filename);

//			    W_CheckSize(0);
			}
			else if (strncmp(pwad_term, temp, 4) == 0)
			{
			    load_extra_wad = 1;

			    strcpy(extra_wad, check);
			    strcpy(stripped_extra_wad, tmpFile->filename);

			    W_CheckSize(4);

			    if(fsizecq == 7585664)
				is_chex_2 = true;

			    extra_wad_loaded = 1;
			}
			else if (strncmp(deh_term, temp, 23) == 0) 
			{
			    load_dehacked = 1;

			    strcpy(dehacked_file, check);
			    strcpy(stripped_dehacked_file, tmpFile->filename);
			}
		    }
		}
		fclose(file);
	    }
	}

	/* B button */
	if (buttons & WPAD_CLASSIC_BUTTON_B)
	{
	    if(gDirLevel<=1)
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

	if (buttons & WPAD_CLASSIC_BUTTON_A)
	{
	    load_dehacked = 0;
	    load_extra_wad = 0;

	    strcpy(stripped_dehacked_file, "");
	    strcpy(stripped_extra_wad, "");
	    strcpy(stripped_target, "");

	    strcpy(dehacked_file, "");
	    strcpy(extra_wad, "");
	    strcpy(target, "");
	}

	/* List scrolling */
	index = (selected - start);

	if (index >= ENTRIES_PER_PAGE)
	    start += index - (ENTRIES_PER_PAGE - 1);

	if (index <= -1)
	    start += index;
    }

    err:

    printf("\n");
    printf("\n  Press any key...               |\n"/*, 129*/);

    free (tmpPath);

    /* Wait for button */
    WaitButtons();
}

void Menu_Loop(void)
{
    for (;;)
    {
	/* FAT device menu */
	Menu_FatDevice();

	/* WAD list menu */
	Menu_WadList();
    }
}

// Start of wiiNinja's added routines

int PushCurrentDir (char *dirStr, s32 Selected, s32 Start)
{
    int retval = 0;

    // Store dirStr into the list and increment the gDirLevel
    // WARNING: Make sure dirStr is no larger than MAX_FILE_PATH_LEN
    if (gDirLevel < MAX_DIR_LEVELS)
    {
        strcpy (gDirList [gDirLevel], dirStr);

	gSeleted[gDirLevel]=Selected;

	gStart[gDirLevel]=Start;

        gDirLevel++;
    }
    else
        retval = -1;

    return (retval);
}

char *PopCurrentDir(s32 *Selected, s32 *Start)
{
    if (gDirLevel > 1)
        gDirLevel--;
    else
        gDirLevel = 0;

    *Selected = gSeleted[gDirLevel];

    *Start = gStart[gDirLevel];

	return PeekCurrentDir();
}

bool IsListFull (void)
{
    if (gDirLevel < MAX_DIR_LEVELS)
        return (false);
    else
        return (true);
}

char *PeekCurrentDir (void)
{
    // Return the current path
    if (gDirLevel > 0)
        return (gDirList [gDirLevel-1]);
    else
        return (NULL);
}

void WaitPrompt (char *prompt)
{
    printf("\n%s", prompt);
    printf("\n  Press any key...               |\n"/*, 129*/);

    /* Wait for button */
    WaitButtons();
}

u32 Pad_GetButtons(void)
{
    u32 buttons = 0, cnt;

    /* Scan pads */
    PAD_ScanPads();

    /* Get pressed buttons */
    for (cnt = 0; cnt < 4; cnt++)
	buttons |= PAD_ButtonsDown(cnt);

    return buttons;
}

// Routine to wait for a button from either the Wiimote or a gamecube
// controller. The return value will mimic the WPAD buttons to minimize
// the amount of changes to the original code, that is expecting only
// Wiimote button presses. Note that the "HOME" button on the Wiimote
// is mapped to the "SELECT" button on the Gamecube Ctrl. (wiiNinja 5/15/2009)
u32 WaitButtons(void)
{
    u32 buttons = 0;
    u32 buttonsGC = 0;

    /* Wait for button pressing */
    while (!buttons && !buttonsGC)
    {
        // GC buttons
        buttonsGC = Pad_GetButtons ();

        // Wii buttons
	buttons = Wpad_GetButtons();

	VIDEO_WaitVSync();
    }

    if (buttonsGC)
    {
        if(buttonsGC & PAD_BUTTON_A)
        {
            //printf ("Button A on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_A;
        }
        else if(buttonsGC & PAD_BUTTON_B)
        {
            //printf ("Button B on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_B;
        }
        else if(buttonsGC & PAD_BUTTON_LEFT)
        {
            //printf ("Button LEFT on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_LEFT;
        }
        else if(buttonsGC & PAD_BUTTON_RIGHT)
        {
            //printf ("Button RIGHT on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_RIGHT;
        }
        else if(buttonsGC & PAD_BUTTON_DOWN)
        {
            //printf ("Button DOWN on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_DOWN;
        }
        else if(buttonsGC & PAD_BUTTON_UP)
        {
            //printf ("Button UP on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_UP;
        }
        else if(buttonsGC & PAD_BUTTON_START)
        {
            //printf ("Button START on the GC controller\n");
            buttons |= WPAD_CLASSIC_BUTTON_HOME;
        }
    }
	return buttons;
}

/*
void WiiLightControl (int state)
{
	switch (state)
	{
		case WII_LIGHT_ON:
			// Turn on Wii Light
			WIILIGHT_SetLevel(255);
			WIILIGHT_TurnOn();
			break;

		case WII_LIGHT_OFF:
		default:
			// Turn off Wii Light
			WIILIGHT_SetLevel(0);
			WIILIGHT_TurnOn();
			WIILIGHT_Toggle();
			break;
	}
} // WiiLightControl
*/

