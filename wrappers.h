/** \file wrappers.h
 * Definition for the usage of the REAL functions that have been wrapped.
 */
#ifndef WRAPPERS_H_INCLUDED
#define WRAPPERS_H_INCLUDED

int __real_puts(const char* s);
size_t __real_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int __real_fclose(FILE* stream);

#endif // WRAPPERS_H_INCLUDED
