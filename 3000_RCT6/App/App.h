#ifndef _APP_H_
#define _APP_H_

#include "Detect.h"

#define UI_UPDATE_TIME 100
typedef void (*fluLedCallback)(int state);
typedef struct 
{
    u8 mark;
    u8 id;
    u8 index;
    u8 datanum;
    u16 fluValue[FLU_DETECT_MAX_NUM];
}SAVE_HISTORY_TYPE;
#define HISTORY_SIZE    sizeof(SAVE_HISTORY_TYPE)
typedef struct 
{
    u8 mark;
    u8 id;
    u8 index;
    u8 unit;
    u8 datanum;
    //u16 fluValue[EQUATION_DATA_MAX_NUM];//数据点荧光强度
    //float mol[EQUATION_DATA_MAX_NUM];//数据点物质浓度,画浓度曲线使用,当前不添加
    float a;
    float b;//方程系数,y=ax+b
}SAVE_EQUATION_TYPE;
#define EQUATION_SIZE    sizeof(SAVE_EQUATION_TYPE)


typedef struct 
{
    //数据记录相关函数
    void (*historyErpInit)(void);
    void (*historyFactoryReset)(void);
    void (*historyDelData)(u8 index);
    //获取数据记录数量
    unsigned char (*historygetNum)(void);
    //历史记录添加,返回ID,返回0xFF则添加失败
    unsigned char (*historyAddStu)(SAVE_HISTORY_TYPE *Para);
    //数据记录匹配，返回0xFF匹配失败，记录不存在,匹配成功返回记录ID
    unsigned char (*historyMatching)(unsigned char index);
    //检查数据是否存在，0不存在，1存在,i为数组下标,不是ID
    unsigned char (*historyParaCheck)(unsigned char i);
    //获取指定数据的ID,idx-结构体数组下标
    unsigned char (*historyGetStuId)(unsigned char idx);
    //获取指定数据记录的结构体数据,*pdDevPara-外部结构体指针，idx-要获取的数据结构体数组下标
    void (*historyGetStuData)(SAVE_HISTORY_TYPE *Para, unsigned char idx);
    //修改记录属性,id->指定记录idx psDevPara->记录属性结构体
    void (*historySetStuData)(unsigned char id,SAVE_HISTORY_TYPE *psDevPara);

    //方程相关函数
    void (*equationErpInit)(void);
    void (*equationFactoryReset)(void);
    //删除指定的方程数据,
    void (*equationDelData)(u8 index);
    //获取方程录数量
    unsigned char (*equationGetNum)(void);
    //添加方程,返回ID,返回0xFF则添加失败
    unsigned char (*equationAddStu)(SAVE_EQUATION_TYPE *Para);
    //数据记录匹配，返回0xFF匹配失败，记录不存在,匹配成功返回记录ID
    unsigned char (*equationMatching)(unsigned char index);
    //检查数据是否存在，0不存在，1存在,i为数组下标,不是ID
    unsigned char (*equationCheckExist)(unsigned char i);
    //获取指定数据的ID,idx-结构体数组下标
    unsigned char (*equationGetStuId)(unsigned char idx);
    //获取指定方程的结构体数据,*pdDevPara-外部结构体指针，idx-要获取的数据结构体数组下标
    void (*equationGetStuDat)(SAVE_EQUATION_TYPE *Para, unsigned char idx);
    //修改方程属性,id->指定方程idx psDevPara->方程属性结构体
    void (*equationSetStuData)(unsigned char id,SAVE_EQUATION_TYPE *psDevPara);
}SAVE_DATA_TYPE;


void appInit(void);

void appTask(void);


int fluLedCallbackRegister(fluLedCallback func);










#endif // !_APP_H_
