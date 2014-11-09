/*
#include <malloc.h>
//#include <pspdebug.h>

#include <stdio.h>
#include <stdlib.h>
*/
#include <sys/stat.h>

#include "doomdef.h"
/*
//#include "h2_main.h"
#include "i_sound.h"
#include "m_argv.h"

#include "xmn_main.h"
#include "xmn_psp.h"
#include "xmn_PSPm.h"

#include <gctypes.h>

#define MAXDEPTH	16
#define MAXDIRNUM	1024
#define PATHLIST_H	3
#define REPEAT_TIME	0x40000
#define BUFSIZE		65536
*/
#define WADRoot		"WADS/"
#define PWADPath	"WADS/PWAD/"
#define IWADPath	"WADS/IWAD/"
/*
//static 			SceCtrlData ctl;

dirent_t		dlist[MAXDIRNUM];
dirent_t		dirlist[MAXDIRNUM];
dirent_t		dlist2[MAXDIRNUM];

char			check[MAXPATH];
char			now_path[MAXPATH];
char			buf[BUFSIZE];
char			doom_wad_dir[256];

int			txtLoaded = 0;
int			txtPos = 0;
int			WADLoaded = 0;
int			dlist_num;
int			dlist_num2;
int			dlist_start;
int			dlist_curpos;
int			dlist_drawstart;
int			cbuf_start[MAXDEPTH];
int			cbuf_curpos[MAXDEPTH];
int			now_depth;
int			extra_wad_loaded = 0;

void Get_DirList(char *path)
{
    int ret, fd;

    // Directory read
    fd = opendir(path);

    dlist_num = 0;
    ret = 1;

    while((ret > 0) && (dlist_num < MAXDIRNUM))
    {
	ret = readdir(fd, &dlist[dlist_num]);

	if (dlist[dlist_num].name[0] == '.' && dlist[dlist_num].name[1] != '.')
	    continue;

	if (ret > 0)
	    dlist_num++;
    }
    closedir(fd);

    if (dlist_start >= dlist_num)
	dlist_start = dlist_num - 1;

    if (dlist_start < 0)         
	dlist_start = 0;

    if (dlist_curpos >= dlist_num)
	dlist_curpos = dlist_num - 1;

    if (dlist_curpos < 0)
	dlist_curpos = 0;
}

void GetDirList(char *root)
{
    int dfd;
    int current = 0;

    dfd = opendir(root);

    if (dfd > 0)
    {
	while (readdir(dfd, &dirlist[current]) > 0)
	    current++;

	closedir(dfd);
    }
}

void Read_Key2()
{
    static int n = 0;

    sceCtrlReadBufferPositive(&ctl, 1);

    now_pad = ctl.Buttons;
    new_pad = now_pad & ~old_pad;

    if (old_pad == now_pad)
    {
	n++;

	if (n >= 25)
	{
	    new_pad = now_pad;
	    n = 20;
	}
    }
    else
    {
	n = 0;
	old_pad = now_pad;
    }
}

int Control_DirList(void)
{
    int i;

    char *temp;
    char line[BUFSIZ];
    char iwad_term[] = "IWAD";
    char pwad_term[] = "PWAD";

    FILE *file;

    Read_Key2();

    pgWaitV();

    if (new_pad & PSP_CTRL_UP)
    {
	if (dlist_curpos > 0)
	{
	    dlist_curpos--;

	    if (dlist_curpos < dlist_start) 
		dlist_start = dlist_curpos;
	}
	txtLoaded = 0;
	WADLoaded = 0;
    }

    if (new_pad & PSP_CTRL_DOWN)
    {
	if (dlist_curpos < (dlist_num - 1))
	{
	    dlist_curpos++;

	    if (dlist_curpos >= (dlist_start + PATHLIST_H))
		dlist_start++;
	}
	txtLoaded = 0;
	WADLoaded = 0;
    }

    if (new_pad & PSP_CTRL_CROSS)
    {
	txtLoaded = 0;
	WADLoaded = 0;

	if (dlist[dlist_curpos].type & TYPE_DIR) 
	{
	    if (dlist[dlist_curpos].name[0] == '.')
	    {
		if (now_depth > 0)
		{
		    for (i = 0; i < MAXPATH; i++)
		    {
			if (doom_wad_dir[i] == 0)
			    break;
		    }
		    i--;

		    while (i > 4)
		    {
			if (doom_wad_dir[i - 1] == '/')
			{
			    doom_wad_dir[i] = 0;

			    break;
			}
			i--;
		    }
		    now_depth--;

		    dlist_start  = cbuf_start[now_depth];
		    dlist_curpos = cbuf_curpos[now_depth];

		    return 1;
		}
		return 3;
	    }
	    if (now_depth < MAXDEPTH) 
	    {
		strcat(doom_wad_dir, dlist[dlist_curpos].name);
		strcat(doom_wad_dir, "/");

		cbuf_start[now_depth] = dlist_start;
		cbuf_curpos[now_depth] = dlist_curpos;
		dlist_start = 0;
		dlist_curpos = 0;

		now_depth++;

		return 1;
	    }
	}
	else
	{
	    strcpy(check, doom_wad_dir);
	    strcat(check, dlist[dlist_curpos].name);

	    file = fopen(check, "r");

	    if (file != NULL)
	    {
		while (fgets(line, sizeof(line), file))
		{
		    temp = line;

		    if (strncmp(iwad_term, temp, 4) == 0 && fsize != 19801320 && fsize != 27625596 &&
							    fsize != 27704188 && fsize != 28592816 &&
							    fsize != 28144744 && fsize != 19362644)
		    {
			strcpy(target, check);

			W_CheckSize(0);

			return 3;
		    }
		    else if (strncmp(pwad_term, temp, 4) == 0 && fsize == 12361532) 
		    {
			strcpy(target, check);

			extra_wad_loaded = 1;

			return 3;
		    }
		}
	    }

	    fclose(file);

	    return -1;
	}
    }

    if (new_pad & PSP_CTRL_SQUARE)
    {
	txtLoaded = 0;

	if (dlist[dlist_curpos].type & TYPE_FILE)
	{
	    load_dehacked = 1;

	    strcpy(dehacked_file, doom_wad_dir);
	    strcat(dehacked_file, dlist[dlist_curpos].name);
	}
	return 3;
    }

    if (new_pad & PSP_CTRL_CIRCLE)
    {
	txtLoaded = 0;

	load_dehacked = 0;
	load_extra_wad = 0;

	strcpy(dehacked_file, "");
	strcpy(extra_wad, "");
	strcpy(target, "");

	return 3;
    }

    if (new_pad & PSP_CTRL_START)
	return 2;

//    if (new_pad & PSP_CTRL_SELECT)
//	return 4;

    if (new_pad & PSP_CTRL_TRIANGLE)
    {
	txtLoaded = 0;
	WADLoaded = 0;

	if (dlist[dlist_curpos].type & TYPE_FILE)
	{
	    load_extra_wad = 1;

	    strcpy(check, doom_wad_dir);
	    strcat(check, dlist[dlist_curpos].name);

	    file = fopen(check, "r");

	    if (file != NULL)
	    {
		while (fgets(line, sizeof(line), file))
		{
		    temp = line;

		    if (strncmp(pwad_term, temp, 4) == 0) 
		    {
			strcpy(extra_wad, check);

			W_CheckSize(3);

			if(fsizecq == 7585664)
			   is_chex_2 = true;

			extra_wad_loaded = 1;

			return 3;
		    }
		    else
			load_extra_wad = 0;
		}
	    }

	    fclose(file);

	    return -1;
	}
	return 3;
    }

    if (new_pad & PSP_CTRL_LTRIGGER)
    {
	if (txtPos >= 0)
	    txtPos--;

	return 1;
    }

    if (new_pad & PSP_CTRL_RTRIGGER)
    {
	if (txtPos < 600)
	    txtPos++;

	return 1;
    }

    return 0;
}

void displayFile()
{
    int foundWAD = 0;
    int foundTxt = 0;
    int i = 0;

    if (dlist[dlist_curpos].type & TYPE_FILE)
    {
	for (i = 0; i < MAXPATH; i++) 
	{
	    if (dlist[dlist_curpos].name[i] == 0) 
		break;
	}
	i--;

	while (i > 2)
	{
	    if (dlist[dlist_curpos].name[i-1] == '.')
	    {
		if (dlist[dlist_curpos].name[i] == 't' || dlist[dlist_curpos].name[i] == 'T')
		    foundTxt = 1;
		else if (dlist[dlist_curpos].name[i] == 'w' || dlist[dlist_curpos].name[i] == 'W')
		    foundWAD = 1;

		break;
	    }
	    i--;
	}
    }

    if (foundTxt == 1)
    {
	if (txtLoaded == 0)
	{
	    txtPos = 0;

	    strcpy(path_tmp, doom_wad_dir);
	    strcat(path_tmp, dlist[dlist_curpos].name);

	    FILE* fp = fopen(path_tmp, "r");

	    fseek(fp, SEEK_END, 0);
	    fseek(fp, SEEK_SET, 0);

	    i = 0;

	    while (!feof(fp) && i < 600)
	    {
		fgets(buf + 100 * i, 100, fp);
		i++;
	    }
	    fclose(fp);

	    txtLoaded = 1;
	}
	int j = 0;

	for (i = txtPos; i < txtPos + 12; i++)
	{
	    mh_print(30, 152 + j * 10, buf + 100 * i, rgb2col(50, 255, 50), 0, 0);

	    j++;
	}
    }
    else if (foundWAD == 1)
    {
	if (WADLoaded == 0)
	{
	    strcpy(path_tmp, doom_wad_dir);
	    strcat(path_tmp, dlist[dlist_curpos].name);

	    W_CheckSize(1);

	    if(fsize == 4261144)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 BETA v1.4 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4271324)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 BETA v1.5 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4211660)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 BETA v1.6 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 10396254)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.1 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 10399316)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.2 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 10401760)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.6 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 11159840)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.8 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 12408292)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 REGISTERED v1.9UD DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 4207819)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.0 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4274218)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.1 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4225504)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.2 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4225460)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 SW. v1.25 Sybex DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4234124)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.666 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 4196020)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 1 SHAREWARE v1.8 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(47, 125, "YOU CANNOT -FILE WITH THE SHAREWARE & BETA VERSIONS OF DOOM. PLEASE REGISTER!",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 14943400)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.666 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 14824716)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.666G DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 14612688)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.7 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 14607420)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.8F DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 14604584)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: DOOM 2 REGISTERED v1.9 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 18195736)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: FINAL DOOM TNT",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(217, 111, "(OLD)",
			rgb2col(255, 255, 50), 0, 0);

		mh_print(247, 111, "DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 18654796)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: FINAL DOOM TNT",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(217, 111, "(NEW)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(247, 111, "DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 18240172)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: FINAL DOOM PLUTONIA",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(242, 111, "(NEW)",
			rgb2col(0, 255, 0), 0, 0);

		mh_print(272, 111, "DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 17420824)
	    {
		mh_print(47, 111, "DOOM IWAD VERSION: FINAL DOOM PLUTONIA",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(242, 111, "(OLD)",
			rgb2col(255, 255, 50), 0, 0);

		mh_print(272, 111, "DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 19801320)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.6.4 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 27704188)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.7 RC 1 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 27625596)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.7 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 28144744)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.8 BETA 1 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 28592816)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.8 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 19362644)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.8 PHASE 1 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 28422764)
	    {
		mh_print(47, 111, "FREEDOOM IWAD VERSION: FREEDOOM v0.8 PHASE 2 DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 12361532)
	    {
		mh_print(47, 111, "CHEX QUEST IWAD VERSION: CHEX QUEST DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize == 9745831)
	    {
		mh_print(47, 111, "HACX IWAD VERSION: HACX v1.0 SHAREWARE DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 21951805)
	    {
		mh_print(47, 111, "HACX IWAD VERSION: HACX v1.0 REGISTERED DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 22102300)
	    {
		mh_print(47, 111, "HACX IWAD VERSION: HACX v1.1 REGISTERED DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(298, 111, "(THIS WAD IS NOT SUPPORTED)",
			rgb2col(255, 0, 0), 0, 0);
	    }
	    else if(fsize == 19321722)
	    {
		mh_print(47, 111, "HACX IWAD VERSION: HACX v1.2 REGISTERED DETECTED",
			rgb2col(255, 0, 0), 0, 0);

		mh_print(318, 111, "(THIS WAD IS SUPPORTED)",
			rgb2col(0, 255, 0), 0, 0);
	    }
	    else if(fsize != 4261144  && fsize != 4271324  && fsize != 4211660  && fsize != 10396254 &&
		    fsize != 10399316 && fsize != 10401760 && fsize != 11159840 && fsize != 12408292 &&
		    fsize != 4207819  && fsize != 4274218  && fsize != 4225504  && fsize != 4225460  &&
		    fsize != 4234124  && fsize != 4196020  && fsize != 14943400 && fsize != 14824716 &&
		    fsize != 14612688 && fsize != 14607420 && fsize != 14604584 && fsize != 18195736 &&
		    fsize != 18654796 && fsize != 18240172 && fsize != 17420824 && fsize != 28422764 &&
		    fsize != 12361532 && fsize != 9745831  && fsize != 21951805 && fsize != 22102300 &&
		    fsize != 19321722 && fsize != 27625596 && fsize != 27704188 && fsize != 28592816 &&
		    fsize != 28144744 && fsize != 19801320 && fsize != 19362644)
	    {
		mh_print(47, 111, "!!! WARNING: THIS WAD CANNOT BE USED AS A MAIN GAME IWAD  -  UNKNOWN FILE !!!",
			rgb2col(255, 0, 0), 0, 0);
	    }
//	    WADLoaded = 1;
	}
    }
}
*/
#include "video.h"
#include "d2x-cios-installer.h"
#include "gui.h"
struct stConsoleCursorLocation stTexteLocation;
u8 getTextBoxRow(u8 chRow) {
    return (chRow==AUTOSIZE)?getConsoleRow():chRow;
}
u8 getTextBoxColumn(u8 chColumn) {
    return (chColumn==AUTOSIZE)?getConsoleColumn():chColumn;
}
struct stTable stTableSettings;

void drawMain()
{
    printStyledText(2, 0,CONSOLE_FONT_BLUE,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"              CHOCOLATE DOOM for NINTENDO WII by NITR8 (RELEASE 1)              ");
    printStyledText(3, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  SELECT THE GAME'S MAIN IWAD. YOU CAN ALSO SELECT A PWAD AND DEH FILE TO LOAD  ");
    printStyledText(4, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"  ----------------------------------------------------------------------------  ");
}

void CreateDirs(void)
{
    char createWADRoot[121];
    char createPWADDir[121];
    char createIWADDir[121];

    strcpy(createWADRoot, WADRoot);
    strcpy(createPWADDir, PWADPath);
    strcpy(createIWADDir, IWADPath);

    createWADRoot[120] = 0;
    createPWADDir[120] = 0;
    createIWADDir[120] = 0;

    mkdir(createWADRoot, 0755);
    mkdir(createPWADDir, 0755);
    mkdir(createIWADDir, 0755);
}

void Menu_Loop(void);

void drawDirectory()
{
    CreateDirs();

    Menu_Loop();

    drawMain();
}
/*
void enterMenu(char *path)
{


    strcat(doom_wad_dir, "WADS/");

//    Get_DirList(doom_wad_dir);

    dlist_start = 0;
    dlist_curpos = 0;
    now_depth = 0;
	
//    while (1)
    {
	drawDirectory();

	switch (Control_DirList())
	{
	    case 1:
		Get_DirList(doom_wad_dir);

		break;

	    case 2:
		D_DoomMain (); 

		break;

	    case 3:
		break;

	    case 4:
		D_SetupMain();

		break;
	}
    }    
}
*/

