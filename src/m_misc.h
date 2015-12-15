// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
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


#ifndef __M_MISC__
#define __M_MISC__

#include <stdio.h>
#include <stdarg.h>

#include "doomtype.h"

dboolean M_WriteFile(char *name, void *source, int length);
dboolean M_FileExists(char *file);
dboolean M_StrToInt(const char *str, int *result);
dboolean M_StringCopy(char *dest, const char *src, size_t dest_size);
dboolean M_StringConcat(char *dest, const char *src, size_t dest_size);
dboolean M_StringStartsWith(const char *s, const char *prefix);
dboolean M_StringEndsWith(const char *s, const char *suffix);

int M_ReadFile(char *name, byte **buffer);
int M_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args);
int M_snprintf(char *buf, size_t buf_len, const char *s, ...);

void M_MakeDirectory(char *dir);
void M_ExtractFileBase(char *path, char *dest);
void M_ForceUppercase(char *text);

long M_FileLength(FILE *handle);

char *titlecase(const char *str);
char *uppercase(char *str);
char *commify(int value);
char *M_StringDuplicate(const char *orig);
char *M_TempFile(char *s);
char *M_StrCaseStr(char *haystack, char *needle);
char *M_StringJoin(const char *s, ...);
char *M_OEMToUTF8(const char *ansi);
char *M_DirName(char *path);
char *M_StringReplace(const char *haystack, const char *needle, const char *replacement);
char *M_ExtractFilename(char *path);
char *M_BaseName(char *path);
dboolean M_StringCompare(const char *str1, const char *str2);
dboolean isvowel(const char ch);
char *removeext(const char *file);

#endif

