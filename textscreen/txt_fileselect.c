//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// Routines for selecting files.
//


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "doomkeys.h"
#include "txt_fileselect.h"
#include "txt_inputbox.h"
#include "txt_main.h"
#include "txt_widget.h"


#define ZENITY_BINARY "/usr/bin/zenity"


struct txt_fileselect_s
{
    txt_widget_t widget;
    txt_inputbox_t *inputbox;
    int size;
    char *prompt;
    char **extensions;
};


// Dummy value to select a directory.
char *TXT_DIRECTORY[] = { "__directory__", NULL };


static char *ExecReadOutput(char **argv)
{
    char *result;
    int completed;
    int pid, status, result_len;
    int pipefd[2];

    if (pipe(pipefd) != 0)
    {
        return NULL;
    }

    pid = fork();

    if (pid == 0)
    {
        dup2(pipefd[1], fileno(stdout));
        execv(argv[0], argv);
        exit(-1);
    }

    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);

    // Read program output into 'result' string.
    // Wait until the program has completed and (if it was successful)
    // a full line has been read.
    result = NULL;
    result_len = 0;
    completed = 0;

    while (!completed
        || (status == 0 && (result == NULL || strchr(result, '\n') == NULL)))
    {
        char buf[64];
        int bytes;

        if (!completed && waitpid(pid, &status, WNOHANG) != 0)
        {
            completed = 1;
        }

        bytes = read(pipefd[0], buf, sizeof(buf));

        if (bytes < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                status = -1;
                break;
            }
        }
        else
        {
            result = realloc(result, result_len + bytes + 1);

            memcpy(result + result_len, buf, bytes);
            result_len += bytes;
            result[result_len] = '\0';
        }

        usleep(100 * 1000);
        TXT_Sleep(1);
        TXT_UpdateScreen();
    }

    close(pipefd[0]);
    close(pipefd[1]);

    // Must have a success exit code.
    if (WEXITSTATUS(status) != 0)
    {
        free(result);
        result = NULL;
    }

    // Strip off newline from the end.
    if (result != NULL && result[result_len - 1] == '\n')
    {
        result[result_len - 1] = '\0';
    }

    return result;
}

// Linux version: invoke the Zenity command line program to pop up a
// dialog box. This avoids adding Gtk+ as a compile dependency.
static unsigned int NumExtensions(char **extensions)
{
    unsigned int result = 0;

    if (extensions != NULL)
    {
        for (result = 0; extensions[result] != NULL; ++result);
    }

    return result;
}

static int ZenityAvailable(void)
{
    return system(ZENITY_BINARY " --help >/dev/null 2>&1") == 0;
}

int TXT_CanSelectFiles(void)
{
    return ZenityAvailable();
}

char *TXT_SelectFile(char *window_title, char **extensions)
{
    unsigned int i;
    size_t len;
    char *result;
    char **argv;
    int argc;

    if (!ZenityAvailable())
    {
        return NULL;
    }

    argv = calloc(4 + NumExtensions(extensions), sizeof(char *));
    argv[0] = ZENITY_BINARY;
    argv[1] = "--file-selection";
    argc = 2;

    if (window_title != NULL)
    {
        len = 10 + strlen(window_title);
        argv[argc] = malloc(len);
        TXT_snprintf(argv[argc], len, "--title=%s", window_title);
        ++argc;
    }

    if (extensions == TXT_DIRECTORY)
    {
        argv[argc] = strdup("--directory");
        ++argc;
    }
    else if (extensions != NULL)
    {
        for (i = 0; extensions[i] != NULL; ++i)
        {
            len = 30 + strlen(extensions[i]) * 2;
            argv[argc] = malloc(len);

            TXT_snprintf(argv[argc], len, "--file-filter=.%s | *.%s",
                         extensions[i], extensions[i]);

            ++argc;
        }
    }

    argv[argc] = NULL;

    result = ExecReadOutput(argv);

    for (i = 2; i < argc; ++i)
    {
        free(argv[i]);
    }

    free(argv);

    return result;
}

static void TXT_FileSelectSizeCalc(TXT_UNCAST_ARG(fileselect))
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    // Calculate widget size, but override the width to always
    // be the configured size.
    TXT_CalcWidgetSize(fileselect->inputbox);
    fileselect->widget.w = fileselect->size;
    fileselect->widget.h = fileselect->inputbox->widget.h;
}

static void TXT_FileSelectDrawer(TXT_UNCAST_ARG(fileselect))
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    // Input box widget inherits all the properties of the
    // file selector.
    fileselect->inputbox->widget.x = fileselect->widget.x;
    fileselect->inputbox->widget.y = fileselect->widget.y;
    fileselect->inputbox->widget.w = fileselect->widget.w;
    fileselect->inputbox->widget.h = fileselect->widget.h;

    TXT_DrawWidget(fileselect->inputbox);
}

static void TXT_FileSelectDestructor(TXT_UNCAST_ARG(fileselect))
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    TXT_DestroyWidget(fileselect->inputbox);
}

static int DoSelectFile(txt_fileselect_t *fileselect)
{
    char *path;

    if (TXT_CanSelectFiles())
    {
        char **var;

        path = TXT_SelectFile(fileselect->prompt, fileselect->extensions);

        // Update inputbox variable.
        // If cancel was pressed (ie. NULL was returned by TXT_SelectFile)
        // then reset to empty string, not NULL).
        if (path == NULL)
        {
            path = strdup("");
        }

        var = fileselect->inputbox->value;
        free(*var);
        *var = path;
        return 1;
    }

    return 0;
}

static int TXT_FileSelectKeyPress(TXT_UNCAST_ARG(fileselect), int key)
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    // When the enter key is pressed, pop up a file selection dialog,
    // if file selectors work. Allow holding down 'alt' to override
    // use of the native file selector, so the user can just type a path.
    if (!fileselect->inputbox->editing && !TXT_GetModifierState(TXT_MOD_ALT)
        && key == KEY_ENTER)
    {
        if (DoSelectFile(fileselect))
        {
            return 1;
        }
    }

    return TXT_WidgetKeyPress(fileselect->inputbox, key);
}

static void TXT_FileSelectMousePress(TXT_UNCAST_ARG(fileselect),
                                     int x, int y, int b)
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    if (!fileselect->inputbox->editing && !TXT_GetModifierState(TXT_MOD_ALT)
        && b == TXT_MOUSE_LEFT)
    {
        if (DoSelectFile(fileselect))
        {
            return;
        }
    }

    TXT_WidgetMousePress(fileselect->inputbox, x, y, b);
}

static void TXT_FileSelectFocused(TXT_UNCAST_ARG(fileselect), int focused)
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    TXT_SetWidgetFocus(fileselect->inputbox, focused);
}

txt_widget_class_t txt_fileselect_class =
{
    TXT_AlwaysSelectable,
    TXT_FileSelectSizeCalc,
    TXT_FileSelectDrawer,
    TXT_FileSelectKeyPress,
    TXT_FileSelectDestructor,
    TXT_FileSelectMousePress,
    NULL,
    TXT_FileSelectFocused
};

// If the (inner) inputbox widget is changed, emit a change to the
// outer (fileselect) widget.
static void InputBoxChanged(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(fileselect))
{
    TXT_CAST_ARG(txt_fileselect_t, fileselect);

    TXT_EmitSignal(&fileselect->widget, "changed");
}

txt_fileselect_t *TXT_NewFileSelector(char **variable, int size,
                                      char *prompt, char **extensions)
{
    txt_fileselect_t *fileselect;

    fileselect = malloc(sizeof(txt_fileselect_t));
    TXT_InitWidget(fileselect, &txt_fileselect_class);
    fileselect->inputbox = TXT_NewInputBox(variable, 1024);
    fileselect->inputbox->widget.parent = &fileselect->widget;
    fileselect->size = size;
    fileselect->prompt = prompt;
    fileselect->extensions = extensions;

    TXT_SignalConnect(fileselect->inputbox, "changed",
                      InputBoxChanged, fileselect);

    return fileselect;
}

