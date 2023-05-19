/* cprintf.h */
#include <stdarg.h>

int cprintf(const char *fmt, ...);
typedef void(*putchar_func_t)(char);
int dprintf(putchar_func_t func, const char *fmt, ...);
int vdprintf(putchar_func_t func, const char *fmt, va_list ap);

#define printf cprintf


