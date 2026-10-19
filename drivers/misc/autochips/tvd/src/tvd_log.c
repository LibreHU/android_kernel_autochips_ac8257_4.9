#include "tvd_log.h"


#if defined(__KERNEL__)
#else

int tvd_log_level = TVD_INFO;

const char *tvd_log_str[] = {\
		"emerg",\
		"alert",\
		"crit",\
		"err",\
		"warning",\
		"notice",\
		"info",\
		"debug",\
	};\
	
#endif
