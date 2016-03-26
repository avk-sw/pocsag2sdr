#define MAXERRORLEN	1024

enum {
	ERR_ERRNO=0
#ifdef WIN32
	, ERR_WIN32
#endif // WIN32
	, ERR_MAX
};

void set_error(int err_type,char *format, ...);
char *my_strerror(void);
