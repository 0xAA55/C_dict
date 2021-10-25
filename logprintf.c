#include"logprintf.h"

#include<time.h>
#include<stdarg.h>
#include<stdlib.h>

void log_printf(FILE *fp, const char *Format, ...)
{
	char Buffer[64] = {0};
	time_t t = time(NULL);
	struct tm *hms = localtime(&t);
	va_list ap;
	va_start(ap, Format);
	strftime(Buffer, sizeof Buffer, "[%H:%M:%S] ", hms);
	fputs(Buffer, fp);
	vfprintf(fp, Format, ap);
	va_end(ap);
}
