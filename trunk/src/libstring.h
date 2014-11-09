#ifndef _LIBSTRING_H_
#define _LIBSTRING_H_
#include <stdbool.h>
char *getCloneString(const char *strValue);
char **getSplitStrings(const char *strValue,const char *strSeparator,unsigned int *intItemsCount);
char *getBreakString(const char *strValue,const char chBreakChar,size_t intMaxStringSize);
char **getBreakStrings(const char *strValue,const char chBreakChar,size_t intMaxLineSize,unsigned int *intBreakStringsCount);
unsigned int getLinesCount(const char *strValue,unsigned int *intMaxLineSize);
bool isInStringArray(const char *strValue,const char **strArray,unsigned int intArraySize);
unsigned int uniqueStringArray(char **strArray,unsigned int intArraySize);
char *getSubString(const char *strValue,unsigned int intOffset,size_t intLength);
#endif
