#ifndef _DETECT_H_
#define _DETECT_H_

#include "sys.h"


#define FLU_DETECT_MAX_NUM      50
#define FLU_INDEX_MAX_NUM       20

#define EQUATION_DATA_MAX_NUM   50      //建立方程数据点最大个数
#define EQUATION_INDEX_MAX_NUM  20

#define SAMPLE_TYPE_NULL        0
#define SAMPLE_TYPE_BLANK       1
#define SAMPLE_TYPE_NOBLANK     2




typedef struct  
{
    //void (* clearBuf)(void);
    float a;
    float b;//方程系数,y=ax+b
    u16 fluDataBuf[FLU_DETECT_MAX_NUM];//fluDataBuf[0]为空白对照组荧光值
    u8 sampleType;
    u8 index;
    u8 indexRow;
		u8 dataNum;
}DETECT_TYPE;


typedef struct  
{
    u8 index;
    u8 unit;
    u16 fluValue[EQUATION_DATA_MAX_NUM];
    float a;
    float b;//方程系数,y=ax+b
}EQUATION_TYPE;





int detectObjRegister(DETECT_TYPE** detect);
int equationObjRegister(EQUATION_TYPE** obj);

u8 getFluDataIndexNum(void);
u8 allotFluDataIndex(void);
u8 deleteFluDataIndex(void);//未添加功能

u8 getEquationIndexNum(void);
u8 allotEquationIndex(void);
u8 deleteEquationIndex(void);//未添加功能


#endif // !_DETECT_H_