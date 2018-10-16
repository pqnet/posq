#ifndef _STDLIB_H
#define _STDLIB_H 1
 
#include <sys/cdefs.h>
 
#ifdef __cplusplus
extern "C" {
[[ noreturn ]]
#endif
#ifndef __cplusplus
__attribute__((__noreturn__))
#endif
 
void abort(void);
 
#ifdef __cplusplus
}
#endif
 
#endif