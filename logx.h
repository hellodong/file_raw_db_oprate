

#ifndef _LOGX_H_
#define _LOGX_H_



#define LOG(fmt,arg...)		do{printf("%s() #%d "fmt, __FUNCTION__,__LINE__,##arg);}while(0)
#define LOG_ERR(fmt,arg...)		do{printf("%s() #%d [Error] "fmt, __FUNCTION__,__LINE__,##arg);}while(0)


#endif
