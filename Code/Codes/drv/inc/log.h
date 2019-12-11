#ifndef __LOG_H
#define __LOG_H

#include "SEGGER_RTT.h"

#define LOG_ENABLE 1

#define LOG_LEVEL		LOG_LEVEL_DGB

#define LOG_LEVEL_NOEN		0
#define LOG_LEVEL_ERR			1
#define LOG_LEVEL_WAR			2
#define LOG_LEVEL_INF			2
#define LOG_LEVEL_DGB			3


#if	LOG_ENABLE
	#define LOG_PRINT(__format,...) 	SEGGER_RTT_printf(0,__format,##__VA_ARGS__)
#else
	#define LOG_PRINT(__format,...)
#endif

#if	LOG_ENABLE	&&  (LOG_LEVEL >= LOG_LEVEL_ERR)
	#define	LOG_ERR(__format,...)			SEGGER_RTT_printf(0,"ERR:%s@%d:"__format"\r\n",__FUNCTION__,__LINE__,##__VA_ARGS__)
#else
	#define	LOG_ERR(__format,...)
#endif

#if	LOG_ENABLE	&&  (LOG_LEVEL >= LOG_LEVEL_WAR)
	#define	LOG_WAR(__format,...)			SEGGER_RTT_printf(0,"WAR:%s@%d:"__format"\r\n",__FUNCTION__,__LINE__,##__VA_ARGS__)
#else
	#define	LOG_WAR(__format,...)
#endif

#if	LOG_ENABLE	&&  (LOG_LEVEL >= LOG_LEVEL_INF)
	#define	LOG_INF(__format,...)			SEGGER_RTT_printf(0,"INF:%s@%d:"__format"\r\n",__FUNCTION__,__LINE__,##__VA_ARGS__)
#else
	#define	LOG_INF(__format,...)
#endif

#if	LOG_ENABLE	&&  (LOG_LEVEL >= LOG_LEVEL_DGB)
	#define	LOG_DGB(__format,...)			SEGGER_RTT_printf(0,"DGB:%s@%d:"__format"\r\n",__FUNCTION__,__LINE__,##__VA_ARGS__)
#else
	#define	LOG_DGB(__format,...)
#endif


#endif
