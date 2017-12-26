#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "my_strerror.h"

static int type_of_error=0;
static char *opt_error = NULL;
static char my_error[MAXERRORLEN + 1];

#ifdef WIN32
#include <Windows.h>
static char *strerror_win32(void)
{
	static char ret_err[MAXERRORLEN + 1];
	DWORD dwErr;
	int l, rc;

	dwErr = GetLastError();
	sprintf(ret_err, "[0x%08lX] ", dwErr);
	l = strlen(ret_err);
	rc = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwErr, 0, (LPTSTR)(ret_err + l), MAXERRORLEN - l, NULL);
	if (rc == 0) {
		ret_err[l - 1] = 0;
	}
	else {
		if (ret_err[l + rc - 1] == '\n') ret_err[l + rc - 1] = 0;
	}
	return ret_err;
}
#endif // WIN32

void set_error(int err_type,char *format, ...) {
	va_list ap;
	int l;
	char *err_msg;

	my_error[0] = 0; l = 0;
	if (format != NULL) {
		va_start(ap, format);
		vsnprintf(my_error, MAXERRORLEN, format, ap);
		l = strlen(my_error);
	}
	switch (err_type) {
	case ERR_ERRNO: err_msg = strerror(errno); break;
#ifdef WIN32
	case ERR_WIN32: err_msg = strerror_win32(); break;
#endif // WIN32
	default: err_msg=NULL;
	}
	if (err_msg != NULL) {
		if (l != 0) {
			snprintf(my_error + l, MAXERRORLEN - l, ": %s", err_msg);
		} else {
			strncpy(my_error, err_msg, MAXERRORLEN);
		}
	}
}

char *my_strerror(void) {
	return my_error;
}
