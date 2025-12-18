#ifndef __SYS_H_
#define __SYS_H_ 

//#include <stdio.h>

#ifndef NULL
#define NULL (void*)0
#endif


#ifndef u8 
typedef unsigned char   u8;        
#endif

#ifndef uint8 
typedef unsigned char   uint8;     
#endif

#ifndef uint8_t 
typedef unsigned char   uint8_t;    
#endif

#ifndef uchar 
typedef unsigned char   uchar;      
#endif

#ifndef u16 
typedef unsigned short  u16;       
#endif

#ifndef uint16 
typedef unsigned short  uint16;     
#endif

#ifndef uint16_t 
typedef unsigned short  uint16_t;   
#endif

#ifndef int16 
typedef short           int16;   
#endif

#ifndef u32 
typedef  unsigned int   u32;    
#endif

#ifndef uint32 
typedef unsigned int    uint32;    
#endif

#ifndef uint32_t 
typedef unsigned int    uint32_t;  
#endif

#ifndef int32 
typedef int             int32; 
#endif




#define __DEBUG

#ifdef __DEBUG
#define debugInfo(format,...)      printf(""__FILE__"-Line:%d-%s-Info:"format"\n", __LINE__, __func__, ##__VA_ARGS__)
#define debugWarning(format,...)   printf(""__FILE__"-Line:%d-%s-Warning:"format"\n", __LINE__, __func__, ##__VA_ARGS__)
#define debugError(format,...)     printf(""__FILE__"-Line: %d-%s-Error:"format"\n", __LINE__, __func__, ##__VA_ARGS__)
#else

#endif

void delayUsSoftware(unsigned int nus);
void delayMsSoftware(unsigned int nms);




#endif











