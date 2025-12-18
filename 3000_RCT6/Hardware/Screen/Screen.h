#ifndef __SCREEN_H_
#define __SCREEN_H_


#include "sys.h"

#define FLUINDEXMAXNUM 200



/* screen data frame buffer define */
#define CMD_CUSTOM_HEAD 0XAA            //帧头
#define CMD_CUSTOM_TAIL 0XEB90          //帧尾
#define QUEUE_SIZE_NUM 512              //队列接收缓冲区大小
#define CMD_SIZE_NUM 64                 //单条指令大小 
/* screen custom cmd  */
#define CTR_TYPE_CUSTOM_BTN    0X11     //按钮控件
#define CTR_TYPE_CUSTOM_TEXT   0X22     //文本控件
#define CTR_TYPE_CUSTOM_TABLE  0X33     //数据记录控件
#define CTR_TYPE_CUSTOM_GRAPH  0X44     //折线图控件
#define CTR_TYPE_CUSTOM_SLIDE  0X55     //滑块控件

/* screen system cmd  */
#define CMD_SYSTEM_HEAD         0XEE        //帧头
#define CMD_SYSTEM_TAIL         0XFFFCFFFF  //帧尾
#define CTR_TYPE_SYSTEM_BTN     0X10        //按钮控件
#define CTR_TYPE_SYSTEM_TEXT    0X11        //文本控件
#define CTR_TYPE_SYSTEM_SLIDER  0X13        //滑块控件
#define CTR_TYPE_SYSTEM_GRAPH   0X18        //折线图控件(官方demo中计算出的,可能是错的,手册上找不到具体数值)
#define CTR_TYPE_SYSTEM_TABLE   0X1D        //数据图控件

#define CMD_MODE_CUSTOM   0x00        //读取自定义命令
#define CMD_MODE_SYSTEM   0x01        //读取系统命令

typedef unsigned char qdata;
typedef unsigned short qsize;
typedef struct _QUEUE                                             
{                                                                 
    qsize _head;                    //队列头
    qsize _tail;                    //队列尾
    qdata _data[QUEUE_SIZE_NUM];    //队列数据缓存区
}QUEUE;



typedef struct
{
    u8 head;        //帧头:0xAA
    u8 type;        //控件类型:0x11:按钮-bnt;0x22:文本-text;0x33:表格-table;0x44:图表-graph
    u8 pageID;      //页面ID
    u8 controlID;   //控件ID
    u8 data;        //数据
    u8 reserve;     //预留
    u8 tail[2];     //帧尾:0xEB90
}SCREEN_CMD_CUSTOM_TYPE;

typedef struct
{
    uint8 cmd_head;      //帧头
    uint8 cmd_type;      //命令类型(UPDATE_CONTROL)    
    uint8 ctrl_msg;      //CtrlMsgType-指示消息的类型
    uint16 screen_id;    //产生消息的画面ID
    uint16 control_id;   //产生消息的控件ID
    uint8 control_type;  //控件类型
    uint8 param[256];    //可变长度参数，最多256个字节
    uint8 cmd_tail[4];   //帧尾
}SCREEN_CMD_SYSTEM_TYPE;


#define PTR2U16(PTR) ((((uint8 *)(PTR))[0]<<8)|((uint8 *)(PTR))[1])  //从缓冲区取16位数据
#define PTR2U32(PTR) ((((uint8 *)(PTR))[0]<<24)|(((uint8 *)(PTR))[1]<<16)|(((uint8 *)(PTR))[2]<<8)|((uint8 *)(PTR))[3])  //从缓冲区取32位数据


















/* interface page id define */
#define PAGE_ID_0_ENTER                 0
#define PAGE_ID_1_FUNCSELECT            1
#define PAGE_ID_2_FLUDETECT             2
#define PAGE_ID_2_HISTORY               3
#define PAGE_ID_2_SYSTEM                4
#define PAGE_ID_2_EQUATION              5
#define PAGE_ID_2_MOL                   6
#define PAGE_ID_2_WIFI                  8
#define PAGE_ID_3_INTRUDUCE             7
#define PAGE_ID_3_MOLINPUT              9
#define PAGE_ID_3_EQUATIONSELECT        10
#define PAGE_ID_ERROR_NOSELEUQ          11
#define PAGE_ID_ERROR_NOADDBLANK        12
#define PAGE_ID_ERROR_NOADDSAMPLE       13
#define PAGE_ID_ERROR_NOEQUMOL          14
#define PAGE_ID_ERROR_NOEQUDATANUM      15
#define PAGE_ID_ERROR_ALREADYBLANK      16
#define PAGE_ID_ERROR_NOBLANKDATA       17
#define PAGE_ID_ERROR_ADDHISTORYFAIL    18
#define PAGE_ID_ERROR_NODATATOSAVE      21

#define PAGE_ID_NOTIFY_ADDHISTORYOK     19
#define PAGE_ID_NOTIFY_HISSAVEING       20



/* interface control id define */\
#define CON_ID_FUNCBACK_1_1             1
#define CON_ID_FUNCFLU_1_2              2
#define CON_ID_FUNCEQUATION_1_3         3
#define CON_ID_FUNCMOL_1_4              4
#define CON_ID_FUNCHISTORY_1_5          5
#define CON_ID_FUNCWIFI_1_6             6
#define CON_ID_FUNCSET_1_7              7

#define CON_ID_BACK_2_1                 1
#define CON_ID_ADDBLANK_2_2             2
#define CON_ID_ADDSAMPLE_2_3            3
#define CON_ID_DETECT_2_4               4
#define CON_ID_SAVEDATA_2_5             5
#define CON_ID_FLUINDEXTEXT_2_7         7
#define CON_ID_FLUDATATABLE_2_6         6

#define CON_ID_HISTORYDELETEBTN_3_4     4
#define CON_ID_HISTORYINDEXTABLE_3_3    3
#define CON_ID_HISTORYDATATABLE_3_2     2
#define CON_ID_EQUATIONMOLTEXT_5_10     10
#define CON_ID_EQUATIONMOUNITTEXT_5_8   8
#define CON_ID_EQUATIONINDEXTEXT_5_7    7
#define CON_ID_EQUATIONFUNCTEXT_5_5     5
#define CON_ID_EQUATIONDATATABLE_5_6    6
#define CON_ID_EQUATIONDETECTBTN_5_3    3
#define CON_ID_EQUATIONSETUPBTN_5_4     4
#define CON_ID_MOLINDEXTEXT_6_6         6
#define CON_ID_MOLFUNCTEXT_6_5          5
#define CON_ID_MOLDATATABLE_6_4         4
#define CON_ID_MOLDETECTBTN_6_3         3
#define CON_ID_INPUTMOLOK_9_20          20
#define CON_ID_INPUTMOLTEXT_9_21        21
#define CON_ID_INPUTUNITTEXT_9_22       22
#define CON_ID_SELECTINDEXTABLE_10_2    2
#define CON_ID_SELECTFUNCTEXT_10_5      5
#define CON_ID_SELECTDATAGRAPH_10_6     6
#define CON_ID_EQUATIONSELOKBTN_10_15   15
#define CON_ID_EQUATIONSELBACKBTN_10_14 14
#define CON_ID_EQUATIONSETUPBTN_10_13   13

/* concentration unit define */
#define UNIT_STR_MOL_L                  "mol/L"
#define UNIT_STR_MOL_ML                 "mol/mL"
#define UNIT_STR_MOL_UL                 "mol/uL"
#define UNIT_STR_G_L                    "g/L"
#define UNIT_STR_MG_L                   "mg/L"
#define UNIT_STR_NG_L                   "ng/L"
#define UNIT_INDEX_MOL_L                 1
#define UNIT_INDEX_MOL_ML                2
#define UNIT_INDEX_MOL_UL                3
#define UNIT_INDEX_G_L                   4
#define UNIT_INDEX_MG_L                  5
#define UNIT_INDEX_NG_L                  6


typedef struct 
{
     /**********Screen init function*************/
    void (*init)(void);
    /**********flu Detect function*************/
    int (*setFluDataIndex2Text_2_7)(u8 index);
    void (*addFluData2Table_2_6)(u8 id,u16 value);
    void (*clearFluData2Table_2_6)(void);//清除表格在已在屏幕内部指令中完成
    /**********History function*************/
    int (*addHistoryIndex2Table_3_3)(u8 index);
    void (*sendGetStrFromTableCmd_3_3)(u16 pageId,u16 conId,u16 row);
    void (*addHistoryFluData2Table_3_2)(u8 id,u16 value);
    /**********system set function*************/

    /**********establish equation function*************/
    void (*setInputMol2Text_5_10)(float mol,int unit);
    int (*setEquationIndex2Text_5_7)(u8 index);
    void (*setEquationFunc2Text_5_5)(float a,float b);
    void (*addEquationData2Table_5_6)(u8 id,float mol,int unit,u16 fluvalue);
    void (*sendGetMolInputFromTextCmd_9_21_22)(int mode);
    /**********concentration detection function*************/
    int (*setMolDetectIndex2Text_6_6)(u8 index);
    void (*setMolEquation2Text_6_5)(float a,float b);
    void (*addMolData2Table_6_4)(u8 id,float mol,int unit);
    int (*addMolEquationIndex2Table_10_2)(u16 index);
    void (*showMolEquation2Text_10_5)(float a,float b);
    void (*addMolEquationPoints2Graph_10_6)(u16* a,u16 len);
    void (*addMolEquationOnePoint2Graph_10_6)(u16 a);
    void (*sendGetEquIndexFromTableCmd_10_2)(u16 pageId,u16 conId,u16 row);
    /**********wifi function*************/

    /**********recv data frame parse function*************/
    void (*recvBufClear)(int mode);
    void (*recvBufPush)(u8 data,int mode);
    void (*recvBufPop)(u8* data,int mode);
    u16 (*recvGetCmd)(u8* buf,u16 len,int mode);
    u16 (*recvGetBufSize)(int mode);
    /**********other function*************/
    void (*switchPage)(u16 pageId);

}SCREEN_STU_TYPE;

int screenObjRegister(SCREEN_STU_TYPE* obj);

#endif
