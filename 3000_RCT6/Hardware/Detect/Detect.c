#include "Detect.h"
#include "sys.h"
#include "AT24C256.h"


static u8 fluDetectIndex = 0;
static u8 equationtIndex = 0;
DETECT_TYPE currentFluDetectDataStu = {0};
EQUATION_TYPE currentEquationStu = {0};
int setfluDetectIndex(u8 index)
{
    if(index < 200){
        fluDetectIndex = index; return 0;
    }else{
       debugError("fluIndex over"); return 1;
    }
}
u8 getfluDetectIndex(void)
{
    return fluDetectIndex;
}
int setEquationtIndex(u8 index)
{
    if(index < 200){
        equationtIndex = index; return 0;
    }else{
        debugError("equationIndex over"); return 1;
    }
}
int getEquationtIndex(u8 index)
{
    return equationtIndex;
}

u8 fluDataIndexNow = 0;//当前荧光数据序列号,
u8 getFluDataIndexNum(void)
{
    return fluDataIndexNow;
}
u8 allotFluDataIndex(void)
{
    if(fluDataIndexNow >= FLU_INDEX_MAX_NUM){return 1;}
    return ++fluDataIndexNow;
}
u8 deleteFluDataIndex(void)//未添加功能
{
    return 0;
}

u8 getEquationIndexNum(void)
{
    return equationtIndex;
}
u8 allotEquationIndex(void)
{
    if(equationtIndex >= EQUATION_INDEX_MAX_NUM){return 1;}
    return ++equationtIndex;
}
u8 deleteEquationIndex(void)//未添加功能
{
    return 0;
}



int detectObjRegister(DETECT_TYPE** detect)
{
    if(NULL != detect)
    {
        *detect = &currentFluDetectDataStu;
        return 0;
    }else{return 1;}
}
int equationObjRegister(EQUATION_TYPE** obj)
{
    if(NULL != obj)
    {
        *obj = &currentEquationStu;
        return 0;
    }else{return 1;}
}










