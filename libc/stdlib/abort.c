#include <stdio.h>
#include <stdlib.h>
 
#ifdef __cplusplus
[[ noreturn ]]
#else
__attribute__((__noreturn__))
#endif
void abort(void) {
#if defined(__is_libk)
	// TODO: Add proper kernel panic.
	printf("kernel: panic: abort()\n");
#else
	// TODO: Abnormally terminate the process as if by SIGABRT.
	printf("abort()\n");
#endif
	while (1) { }
	__builtin_unreachable();
}