#include "kernel.h"

char *strcpy(char *dest, const char *src) {
   char *save = dest;
   while(*dest++ = *src++);
   return save;	
}

int strcmp(const char *s1, const char *s2)
{
    for ( ; *s1 == *s2; s1++, s2++)
	if (*s1 == '\0')
	    return 0;
    return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
}

int strlen(const char *s) {
	const char *p = s;
    while (*s) ++s;
    return s - p;
}

void *memset(void *dest, char val, size_t count)
{
    char *temp = (char *)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}
