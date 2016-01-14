// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005 Simon Howard
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//      Miscellaneous.
//
//-----------------------------------------------------------------------------


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "c_io.h"
#include "d_deh.h"
#include "doomtype.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


//
// Create a directory
//

void M_MakeDirectory(char *path)
{
    mkdir(path, 0755);
}

// Check if a file exists

dboolean M_FileExists(char *filename)
{
    FILE *fstream;

    fstream = fopen(filename, "r");

    if (fstream != NULL)
    {
        fclose(fstream);
        return true;
    }
    else
    {
        // If we can't open because the file is a directory, the 
        // "file" exists at least!

        return errno == EISDIR;
    }
}

//
// Determine the length of an open file.
//

long M_FileLength(FILE *handle)
{ 
    long savedpos;
    long length;

    // save the current position in the file
    savedpos = ftell(handle);
    
    // jump to the end and find the length
    fseek(handle, 0, SEEK_END);
    length = ftell(handle);

    // go back to the old location
    fseek(handle, savedpos, SEEK_SET);

    return length;
}

//
// M_WriteFile
//

dboolean M_WriteFile(char *name, void *source, int length)
{
    FILE *handle;
    int count;
        
    handle = fopen(name, "wb");

    if (handle == NULL)
    {
        C_Error("M_WriteFile: name handle is NULL");
        return false;
    }

    count = fwrite(source, 1, length, handle);
    fclose(handle);
        
    if (count < length)
    {
        C_Error("M_WriteFile: count < length for file %c", name);
        return false;
    }
                
    return true;
}


//
// M_ReadFile
//

int M_ReadFile(char *name, byte **buffer)
{
    FILE *handle;
    int  count, length;
    byte *buf;
        
    handle = fopen(name, "rb");
    if (handle == NULL)
    {
         I_Error("Couldn't read file %s", name);
        return 0;
    }

    // find the size of the file by seeking to the end and
    // reading the current position

    length = M_FileLength(handle);
    
    buf = Z_Malloc (length, PU_STATIC, NULL);
    count = fread(buf, 1, length, handle);
    fclose (handle);
        
    if (count < length)
        I_Error ("Couldn't read file %s", name);
                
    *buffer = buf;
    return length;
}

// Returns the path to a temporary file of the given name, stored
// inside the system temporary directory.
//
// The returned value must be freed with Z_Free after use.

char *M_TempFile(char *s)
{
    char *tempdir;

    tempdir = "";

    return M_StringJoin(tempdir, DIR_SEPARATOR_S, s, NULL);
}

dboolean M_StrToInt(const char *str, int *result)
{
    return sscanf(str, " 0x%2x", result) == 1
        || sscanf(str, " 0X%2x", result) == 1
        || sscanf(str, " 0%3o", result) == 1
        || sscanf(str, " %10d", result) == 1;
}

void M_ExtractFileBase(char *path, char *dest)
{
    char *src;
    char *filename;
    int length;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path && *(src - 1) != DIR_SEPARATOR)
    {
        src--;
    }

    filename = src;

    // Copy up to eight characters
    // Note: Vanilla Doom exits with an error if a filename is specified
    // with a base of more than eight characters.  To remove the 8.3
    // filename limit, instead we simply truncate the name.

    length = 0;
    memset(dest, 0, 8);

    while (*src != '\0' && *src != '.')
    {
        if (length >= 8)
        {
            C_Warning("Warning: Truncated '%s' lump name to '%.8s'.",
                   filename, dest);
            break;
        }

        dest[length++] = toupper((int)*src++);
    }
}

//---------------------------------------------------------------------------
//
// PROC M_ForceUppercase
//
// Change string to uppercase.
//
//---------------------------------------------------------------------------

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

void M_ForceUppercase(char *text)
{
    char *p;

    for (p = text; *p != '\0'; ++p)
    {
        *p = toupper(*p);
    }
}

//
// M_StrCaseStr
//
// Case-insensitive version of strstr()
//

char *M_StrCaseStr(char *haystack, char *needle)
{
    unsigned int haystack_len;
    unsigned int needle_len;
    unsigned int len;
    unsigned int i;

    haystack_len = strlen(haystack);
    needle_len = strlen(needle);

    if (haystack_len < needle_len)
    {
        return NULL;
    }

    len = haystack_len - needle_len;

    for (i = 0; i <= len; ++i)
    {
        if (!strncasecmp(haystack + i, needle, needle_len))
        {
            return haystack + i;
        }
    }

    return NULL;
}

//
// Safe version of strdup() that checks the string was successfully
// allocated.
//

char *M_StringDuplicate(const char *orig)
{
    char *result;

    result = strdup(orig);

    if (result == NULL)
    {
        I_Error("Failed to duplicate string (length %i)\n",
                strlen(orig));
    }

    return result;
}

//
// String replace function.
//

char *M_StringReplace(const char *haystack, const char *needle,
                      const char *replacement)
{
    char *result, *dst;
    const char *p;
    size_t needle_len = strlen(needle);
    size_t result_len, dst_len;

    // Iterate through occurrences of 'needle' and calculate the size of
    // the new string.
    result_len = strlen(haystack) + 1;
    p = haystack;

    for (;;)
    {
        p = strstr(p, needle);
        if (p == NULL)
        {
            break;
        }

        p += needle_len;
        result_len += strlen(replacement) - needle_len;
    }

    // Construct new string.

    result = malloc(result_len);
    if (result == NULL)
    {
        I_Error("M_StringReplace: Failed to allocate new string");
        return NULL;
    }

    dst = result; dst_len = result_len;
    p = haystack;

    while (*p != '\0')
    {
        if (!strncmp(p, needle, needle_len))
        {
            M_StringCopy(dst, replacement, dst_len);
            p += needle_len;
            dst += strlen(replacement);
            dst_len -= strlen(replacement);
        }
        else
        {
            *dst = *p;
            ++dst; --dst_len;
            ++p;
        }
    }

    return result;
}

// Safe string copy function that works like OpenBSD's strlcpy().
// Returns true if the string was not truncated.

dboolean M_StringCopy(char *dest, const char *src, size_t dest_size)
{
    size_t len;

    if (dest_size >= 1)
    {
        dest[dest_size - 1] = '\0';
        strncpy(dest, src, dest_size - 1);
    }
    else
    {
        return false;
    }

    len = strlen(dest);
    return src[len] == '\0';
}

// Safe string concat function that works like OpenBSD's strlcat().
// Returns true if string not truncated.

dboolean M_StringConcat(char *dest, const char *src, size_t dest_size)
{
    size_t offset;

    offset = strlen(dest);
    if (offset > dest_size)
    {
        offset = dest_size;
    }

    return M_StringCopy(dest + offset, src, dest_size - offset);
}

// Returns true if 's' begins with the specified prefix.

dboolean M_StringStartsWith(const char *s, const char *prefix)
{
    return strlen(s) > strlen(prefix)
        && strncmp(s, prefix, strlen(prefix)) == 0;
}

// Returns true if 's' ends with the specified suffix.

dboolean M_StringEndsWith(const char *s, const char *suffix)
{
    return strlen(s) >= strlen(suffix)
        && strcmp(s + strlen(s) - strlen(suffix), suffix) == 0;
}

// Return a newly-malloced string with all the strings given as arguments
// concatenated together.

char *M_StringJoin(const char *s, ...)
{
    char *result;
    const char *v;
    va_list args;
    size_t result_len;

    result_len = strlen(s) + 1;

    va_start(args, s);
    for (;;)
    {
        v = va_arg(args, const char *);
        if (v == NULL)
        {
            break;
        }

        result_len += strlen(v);
    }
    va_end(args);

    result = malloc(result_len);

    if (result == NULL)
    {
        I_Error("M_StringJoin: Failed to allocate new string.");
        return NULL;
    }

    M_StringCopy(result, s, result_len);

    va_start(args, s);
    for (;;)
    {
        v = va_arg(args, const char *);
        if (v == NULL)
        {
            break;
        }

        M_StringConcat(result, v, result_len);
    }
    va_end(args);

    return result;
}

// Safe, portable vsnprintf().
int M_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args)
{
    int result;

    if (buf_len < 1)
    {
        return 0;
    }

    // Windows (and other OSes?) has a vsnprintf() that doesn't always
    // append a trailing \0. So we must do it, and write into a buffer
    // that is one byte shorter; otherwise this function is unsafe.
    result = vsnprintf(buf, buf_len, s, args);

    // If truncated, change the final char in the buffer to a \0.
    // A negative result indicates a truncated buffer on Windows.
    if (result < 0 || result >= buf_len)
    {
        buf[buf_len - 1] = '\0';
        result = buf_len - 1;
    }

    return result;
}

// Safe, portable snprintf().
int M_snprintf(char *buf, size_t buf_len, const char *s, ...)
{
    va_list args;
    int result;
    va_start(args, s);
    result = M_vsnprintf(buf, buf_len, s, args);
    va_end(args);
    return result;
}

// [crispy] portable pendant to libgen.h's dirname()
// does not modify its argument
//
// [nitr8] UNUSED
//
/*
char *M_DirName(char *path)
{
    char *src, *res;

    res = M_StringDuplicate(path);
    src = res + strlen(res) - 1;

    while (src != res)
    {
        if (*src == DIR_SEPARATOR)
        {
            *src = '\0';
            return res;
        }

        src--;
    }

    // path string does not contain a directory separator
    free(res);
    return M_StringDuplicate(".");
}
*/

char *uppercase(char *str)
{
    char        *newstr;
    char        *p;

    p = newstr = strdup(str);
    while ((*p = toupper(*p)))
        p++;

    return newstr;
}

char *commify(int value)
{
    char result[64];

    M_snprintf(result, sizeof(result), "%i", value);
    if (ABS(value) >= 1000)
    {
        char        *pt;
        int         n;

        for (pt = result; *pt && *pt != '.'; pt++);
        n = result + sizeof(result) - pt;
        do
        {
            pt -= 3;
            if (pt > result)
            {
                memmove(pt + 1, pt, n);
                *pt = ',';
                n += 4;
            }
            else
                break;
        } while (1);
    }
    return strdup(result);
}

char *M_ExtractFilename(char *path)
{
    size_t      len;
    char        *pdest;
    char        *inpfile = NULL;

    pdest = strrchr(path, '\\');

    if (!pdest)
        pdest = strrchr(path, '/');
    if (!pdest)
        pdest = path;
    else
        pdest++;

    len = strlen(pdest);
    inpfile = malloc(len + 1);
    strncpy(inpfile, pdest, len + 1);
    return inpfile;
}

// [crispy] portable pendant to libgen.h's basename()
char *M_BaseName(char *path)
{
    char *src;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path && *(src - 1) != DIR_SEPARATOR)
    {
        src--;
    }

    return src;
}

char *titlecase(const char *str)
{
    char        *newstr = strdup(str);
    size_t      len = strlen(newstr);

    if (len > 1)
    {
        size_t  i;

        newstr[0] = toupper(newstr[0]);
        for (i = 1; i < len; ++i)
            if (!isalnum((unsigned char)newstr[i - 1]) && isalnum((unsigned char)newstr[i]))
                newstr[i] = toupper(newstr[i]);
    }

    return newstr;
}

//
// [nitr8] UNUSED
//
/*
int stricmp(const char *string1, const char *string2)
{
    char src[4096];
    char dest[4096];
    int i;

    for (i=0; i<strlen(string1); i++)
        if (string1[i] >= 'A' && string1[i] <= 'Z')
            src[i] = string1[i] + 32;
        else
            src[i] = string1[i];
    src[i] = 0;

    for (i=0; i<strlen(string2); i++)
        if (string2[i] >= 'A' && string2[i] <= 'Z')
            dest[i] = string2[i] + 32;
        else
            dest[i] = string2[i];
    dest[i] = 0;

    return strcmp(src, dest);
}
*/

// Returns true if 'str1' and 'str2' are the same.
// (Case-insensitive, return value reverse of stricmp() to avoid confusion.
dboolean M_StringCompare(const char *str1, const char *str2)
{
//    return !stricmp(str1, str2);    // FIXME: stricmp() is missing in standard C headers
    return !strcasecmp(str1, str2);
}

dboolean isvowel(const char ch)
{
    return (!!strchr("aeiou", ch));
}

char *removeext(const char *file)
{
    char        *newstr = strdup(file);
    char        *lastdot = strrchr(newstr, '.');

    *lastdot = '\0';

    return newstr;
}

int search_string(char src[], char str[])
{
    int i = 0;
    int j = 0;

    while (src[i] != '\0')
    {
        int firstOcc;

        while (src[i] != str[0] && src[i] != '\0')
            i++;
 
        if (src[i] == '\0')
            return (-1);
 
        firstOcc = i;
 
        while (src[i] == str[j] && src[i] != '\0' && str[j] != '\0')
        {
            i++;
            j++;
        }
 
        if (str[j] == '\0')
            return (firstOcc);

        if (src[i] == '\0')
            return (-1);
 
        i = firstOcc + 1;
        j = 0;
    }
    return 0;
}

