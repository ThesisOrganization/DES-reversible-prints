/** \file wrappers.c
 * This file contains the wrappers for the printf and the fprintf, which will allow us to realize the reversible output in the simulation.
 * But GCC is a very funny compiler and instead of using printf and fprints it uses puts and fwrite -.-.
 */

#include <stdarg.h>
#include <asm-generic/errno-base.h>
#include <stdio.h>

#include "wrappers.h"

size_t __wrap_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream){
  	int done=EAGAIN;
  	//do things with the arguments
  		__real_puts("got into the fwrite wrapper! > ");
  		done=__real_fwrite(ptr,size,nmemb,stream);
  	return done;
  }

int __wrap_puts(const char *s){
	int done=EAGAIN;
	__real_puts("got into the printf wrapper! > ");
	done = __real_puts(s);
	return done;
}

/// We need to wrap the fclose since the model cannot close the file in an event that could be idscrded.
extern int __wrap_fclose(FILE* stream){
	__real_puts("got into the fclose wrapper\n");
	return __real_fclose(stream);
}
