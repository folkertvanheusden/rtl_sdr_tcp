// (C) 2023 by Folkert van Heusden, released under MIT license

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void error_exit(const bool se, const char *const format, ...)
{
	int e = errno;
	va_list ap;

	char *buffer = nullptr;

	va_start(ap, format);
	vasprintf(&buffer, format, ap);
	va_end(ap);

	fprintf(stderr, "%s\n", buffer);

	if (e)
		fprintf(stderr, "%s\n", strerror(e));

	exit(1);
}
