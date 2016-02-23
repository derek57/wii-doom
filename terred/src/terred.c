/*    
    TerrainEd
    
    Copyright 2001 James Haley
    
    Commandline utility for creating a TERTYPES lump for the
    Eternity Engine
    
    See COPYING for license information.    
*/


#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "terred.h"


#define MAXLINELENGTH 80
#define FLATNAMELENGTH 9
#define TYPENAMELENGTH 21


#ifdef DEBUG
#define DEBUG_PRINTF(s) printf("%s%s", s, "\n")
#else
#define DEBUG_PRINTF(s)
#endif


queueitem_t    *queue;

short          numterraintypes;


int main(int argc, char *argv[])
{
    FILE *in, *out;
    
    if (argc != 3)
    {
        printf("TerrainEd -- by Quasar\n");
        printf("  Creates Eternity TERTYPES lump\n");
        printf("  Usage:\n");
        printf("     terred scriptname outfile\n");

        exit(0);
    }
    
    if (!(in = fopen(argv[1], "r")))
    {
        printf("Script file %s not found.\n", argv[1]);

        exit(1);
    }
        
    if (!(out = fopen(argv[2], "wb")))
    {
        printf("Couldn't open file %s for output.\n", argv[2]);

        exit(1);
    }
    
    queue = NULL;
    numterraintypes = 0;
    
    ReadScript(in);
    WriteLump(out);
    
    return 0;
}

void ReadScript(FILE *file)
{
    int i;

    char ch = 0;
    char temp[MAXLINELENGTH];    
    char name[FLATNAMELENGTH];
    char type[TYPENAMELENGTH];
    
    while (!feof(file) && ch != EOF)
    {
        terraintype_t *tt;
        
        memset(temp, '\0', sizeof(char)*MAXLINELENGTH);
        memset(name, '\0', sizeof(char)*MAXLINELENGTH);
        memset(type, '\0', sizeof(char)*MAXLINELENGTH);
        
        i = 0;

        while (i < MAXLINELENGTH - 1)
        {
            ch = getc(file);

            if (ch == '\n' || ch == EOF)
                break;
            else
                temp[i] = ch;

            i++;
        }
        StripSpaces(temp);
            
        // blank line?
        if (strlen(temp) == 1 || temp[0] == '\n' || !(temp[0]))
            continue;
            
        strcpy(name, strtok(temp, " \t="));
        strcpy(type, strtok(NULL, " \t="));
        
        DEBUG_PRINTF(name);
        DEBUG_PRINTF(type);
        
        tt = malloc(sizeof(terraintype_t));

        if (!tt)
        {
            printf("Out of memory.\n");

            exit(1);
        }

        memset(tt, 0, sizeof(terraintype_t));
        strcpy(tt->name, name);
        tt->type = TerrainTypeForName(type);
        
        // throw it on the queue
        enqueue(tt);
        
        // increment the count
        numterraintypes++;
    }
}

void WriteLump(FILE *file)
{
    queueitem_t *temp;
        
    fwrite(&numterraintypes, sizeof(short), 1, file);

    while ((temp = dequeue()))
    {
        terraintype_t *tt = temp->terraintype;

        fwrite(tt->name, sizeof(char), FLATNAMELENGTH, file);
        fwrite(&(tt->type), sizeof(short), 1, file);
    }        
}

void StripSpaces(char *str)
{
    StripLSpace(str);
    StripRSpace(str);
}

void StripLSpace(char *str)
{
    int i = 0;

    char *temp = str;
    
    while (i < MAXLINELENGTH && isspace(str[i]))
    {
        temp++;

        i++;
    }
    
    strcpy(str, temp);
}

void StripRSpace(char *str)
{
    int i = MAXLINELENGTH - 1;

    while (i >= 0 && !isalpha(str[i]))
    {
        str[i] = '\0';

        i--;
    }
}

short TerrainTypeForName(char *str)
{
    if (!strcmp(str, "FLOOR_WATER"))
    {
        return FLOOR_WATER;
    }
    else if (!strcmp(str, "FLOOR_LAVA"))
    {
        return FLOOR_LAVA;
    }
    else if (!strcmp(str, "FLOOR_NUKAGE"))
    {
        return FLOOR_NUKAGE;
    }
    else if (!strcmp(str, "FLOOR_BLOOD"))
    {
        return FLOOR_BLOOD;
    }
    else if (!strcmp(str, "FLOOR_SLIME"))
    {
        return FLOOR_SLIME;
    }
    else
    {
        return FLOOR_SOLID;
    }
}

// Queue functions

void enqueue(terraintype_t *tt)
{
    queueitem_t *newitem, *temp;
    
    newitem = malloc(sizeof(queueitem_t));

    if (!newitem)
    {
        printf("Out of memory.\n");

        exit(1);
    }
    
    newitem->terraintype = tt;
    
    if (!queue)
    {
        queue = newitem;

        newitem->next = NULL;
    }
    else
    {
        temp = queue;

        while (temp->next)
            temp = temp->next;
          
        temp->next = newitem;

        newitem->next = NULL;
    }
}

queueitem_t *dequeue(void)
{
    queueitem_t *temp;
    
    if (!queue)
      return NULL;
    
    temp = queue;
    
    queue = queue->next;
    
    temp->next = NULL;
    
    return temp;
}

