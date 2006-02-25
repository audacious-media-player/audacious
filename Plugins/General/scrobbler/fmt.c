#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include "fmt.h"
#include <curl/curl.h>

char *fmt_escape(char *str)
{
	return curl_escape(str, 0);
}

char *fmt_unescape(char *str)
{
	return curl_unescape(str, 0);
}

char *fmt_timestr(time_t t, int gmt)
{
	struct tm *tm;
	static char buf[30];

	tm = gmt ? gmtime(&t) : localtime(&t);
	snprintf(buf, sizeof(buf), "%d-%.2d-%.2d %.2d:%.2d:%.2d",
			tm->tm_year + 1900,
			tm->tm_mon + 1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);
	return buf;
}

char *fmt_vastr(char *fmt, ...)
{
	va_list ap;
	static char buf[4096];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	return buf;
}

void fmt_debug(char *file, const char *fun, char *str)
{
	fprintf(stderr, "%s [%s] %s: %s\n", fmt_timestr(time(NULL), 0), 
			file, fun, str);
}

char *fmt_string_pack(char *string, char *fmt, ...)
{
	int buflen = 0, stringlen = 0;
	char buf[4096];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if(string != NULL) stringlen = strlen(string);
	buflen = strlen(buf);

	string = realloc(string, stringlen + buflen + 1);
	memcpy(string + stringlen, buf, buflen);
	*(string + stringlen + buflen) = 0;
	return string;
}

int fmt_strcasecmp(const char *s1, const char *s2)
{
	while (toupper(*s1) == toupper(*s2++))
		if (!*s1++)
			return 0;
	return toupper(s1[0]) - toupper(s2[-1]);
}

int fmt_strncasecmp(const char *s1, const char *s2, size_t n)
{
	while (toupper(*s1) == toupper(*s2++) && --n)
		if(!*s1++)
			return 0;
	return n ? toupper(s1[0]) - toupper(s2[-1]) : 0;
}
