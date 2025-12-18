#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "Screen.h"
#include "hmi_driver.h"
#include "cmd_queue.h"
#include "cmd_process.h"
#include "sys.h"


static int setFluDataIndex2Text(u8 index);
static void addFluData2Table(uchar id,u16 value);
static int addHistoryIndex2Table(u8 index);
static void addHistoryFluData2Table(uchar id,u16 value);
static void setInputMol2Text(float mol,int unit);
static int setEquationIndex2Text(u8 index);
static void setEquationFunc2Text(float a,float b);
static void addEquationData2Table(u8 id,float mol,int unit,u16 fluvalue);
static void getMolInputAndUnitFromText(int mode);
static int setMolDetectIndex2Text(u8 index);
static void setMolEquation2Text(float a,float b);
static void addMolData2Table(u8 id,float mol,int unit);
static int addMolEquationIndex2Table(u16 index);
static void showMolEquation2Text(float a,float b);
static void addMolEquationPoints2Graph(u16* a,u16 len);
static void addMolEquationOnePoint2Graph(u16 a);


/** 帧头 控件类型 页面ID 控件ID 数据 保留位  帧尾    共8byte
    0xAA   xx     xx     xx    xx   xx   0xEB90
    一个对外指令数据帧有8byte数据.
 * 控件类型:0x11:按钮-bnt;0x22:文本-text;0x33:表格-table;0x44:图表-graph
 * 页面ID:控件所在页面ID->page_ID
 * 控件ID:控件本体ID->CON_ID
 * 控件状态:设置开关按钮使用,0x00:按下,0x01:弹起 */
static QUEUE queueCustom = {0,0,0}; //指令队列
static uint16 tailStateCustom = 0;  //队列帧尾检测状态
static qsize posCustom = 0;         //当前指令指针位置

static QUEUE queueSystem = {0,0,0}; //指令队列
static uint32 tailStateSystem = 0;        //队列帧尾检测状态
static qsize posSystem = 0;           //当前指令指针位置

/*----------- screen data frame function --------------*/
/*! 
*  \brief  清空指令数据
*   mode:0->自定义参数
*   mdoe:1->系统参数
*/
static void queueClear(int mode)
{
    if(mode){
        queueSystem._head = queueSystem._tail = 0;
        posSystem = 0;
        tailStateSystem = 0;
    }else{
        queueCustom._head = queueCustom._tail = 0;
        posCustom = 0;
        tailStateCustom = 0;
    }
}
/*! 
* \brief  添加指令数据
* \detial 串口接收的数据，通过此函数放入指令队列 
* \param  _data 指令数据
*/
static void queuePush(qdata _data,int mode)
{
    if(mode){
        qsize pos = (queueSystem._head+1)%QUEUE_SIZE_NUM;
        if(pos!=queueSystem._tail)  //非满状态
        {
            queueSystem._data[queueSystem._head] = _data;
            queueSystem._head = pos;
        }
    }else{
        qsize pos = (queueCustom._head+1)%QUEUE_SIZE_NUM;
        if(pos!=queueCustom._tail)  //非满状态
        {
            queueCustom._data[queueCustom._head] = _data;
            queueCustom._head = pos;
        } 
    }   
}

//从队列中取一个数据
static void queuePop(qdata* _data,int mode)
{
    if(mode){
        if(queueSystem._tail != queueSystem._head)    //非空状态
        {
            *_data = queueSystem._data[queueSystem._tail];
            queueSystem._tail = (queueSystem._tail+1)%QUEUE_SIZE_NUM;
        }
    }else{
        if(queueCustom._tail != queueCustom._head)    //非空状态
        {
            *_data = queueCustom._data[queueCustom._tail];
            queueCustom._tail = (queueCustom._tail+1)%QUEUE_SIZE_NUM;
        }  
    }
}

//获取队列中有效数据个数
static qsize queueGetSize(int mode)
{
    if(mode){
        return ((queueSystem._head+QUEUE_SIZE_NUM-queueSystem._tail)%QUEUE_SIZE_NUM);
    }else{
        return ((queueCustom._head+QUEUE_SIZE_NUM-queueCustom._tail)%QUEUE_SIZE_NUM);
    }
}
/*! 
*  \brief  从指令队列中取出一条完整的指令(包含帧头,指令,帧尾)
*  \param  cmd 指令接收缓存区
*  \param  buf_len 指令接收缓存区大小
*  \return  指令长度，0表示队列中无完整指令
*/
static qsize queueTakeCmd(qdata *buffer,qsize buf_len,int mode)
{
    qsize cmd_size = 0;
    qdata _data = 0;

    if(mode){
        while(queueGetSize(CMD_MODE_SYSTEM)>0)
        {
            //取一个数据
            queuePop(&_data,CMD_MODE_SYSTEM);
            //指令第一个字节必须是帧头，否则跳过
            if(posSystem==0&&_data != CMD_SYSTEM_HEAD){continue;}
            if(posSystem<buf_len)//防止缓冲区溢出
            {
                buffer[posSystem++] = _data;
            }
            tailStateSystem = ((tailStateSystem<<8)|_data); //拼接最后2个字节，组成一个16位整数
            if(tailStateSystem==CMD_SYSTEM_TAIL) //最后4个字节与帧尾匹配，得到完整帧
            {
                cmd_size = posSystem;   //指令字节长度
                tailStateSystem = 0;    //重新检测帧尾巴
                posSystem = 0;          //复位指令指针
                return cmd_size;
            }
        }
        return 0;   //没有形成完整的一帧
    }else{
        while(queueGetSize(CMD_MODE_CUSTOM)>0)
        {
            //取一个数据
            queuePop(&_data,CMD_MODE_CUSTOM);
            //指令第一个字节必须是帧头，否则跳过
            if(posCustom==0&&_data != CMD_CUSTOM_HEAD){continue;}
            if(posCustom<buf_len)//防止缓冲区溢出
            {
                buffer[posCustom++] = _data;
            }
            tailStateCustom = ((tailStateCustom<<8)|_data); //拼接最后2个字节，组成一个16位整数
            if(tailStateCustom==CMD_CUSTOM_TAIL) //最后4个字节与帧尾匹配，得到完整帧
            {
                cmd_size = posCustom;   //指令字节长度
                tailStateCustom = 0;    //重新检测帧尾巴
                posCustom = 0;          //复位指令指针
                return cmd_size;
            }
        }
        return 0;   //没有形成完整的一帧
    }
}








/*----------- screen func --------------*/
static void idU82C(u8 id,char* bai,char* shi,char* ge)
{
    *bai = id/100+0x30;
    *shi = id%100/10+0x30;
    *ge = id%100%10+0x30;
}

static void inputMol2StrWithUnit(float mol,int unit,char* str)
{
    switch (unit)
    {
        case UNIT_INDEX_MOL_L:sprintf(str,"%.2f%s",mol,UNIT_STR_MOL_L);break;
        case UNIT_INDEX_MOL_ML:sprintf(str,"%.2f%s",mol,UNIT_STR_MOL_ML);break;
        case UNIT_INDEX_MOL_UL:sprintf(str,"%.2f%s",mol,UNIT_STR_MOL_UL);break;
        case UNIT_INDEX_G_L:sprintf(str,"%.2f%s",mol,UNIT_STR_G_L);break;
        case UNIT_INDEX_MG_L:sprintf(str,"%.2f%s",mol,UNIT_STR_MG_L);break;
        case UNIT_INDEX_NG_L:sprintf(str,"%.2f%s",mol,UNIT_STR_NG_L);break;
        default:sprintf(str,"%s","error");break;
    }
}
static void MolAndFluValue2StrWithUnit(float mol,int unit,u16 fluValue,char* str)
{
    switch (unit)
    {
        case UNIT_INDEX_MOL_L:sprintf(str,"%.2f%s;%u;",mol,UNIT_STR_MOL_L,fluValue);break;
        case UNIT_INDEX_MOL_ML:sprintf(str,"%.2f%s;%u;",mol,UNIT_STR_MOL_ML,fluValue);break;
        case UNIT_INDEX_MOL_UL:sprintf(str,"%.2f%s;%u;",mol,UNIT_STR_MOL_UL,fluValue);break;
        case UNIT_INDEX_G_L:sprintf(str,"%.2f%s;%u;",mol,UNIT_STR_G_L,fluValue);break;
        case UNIT_INDEX_MG_L:sprintf(str,"%.2f%s;%u;",mol,UNIT_STR_MG_L,fluValue);break;
        case UNIT_INDEX_NG_L:sprintf(str,"%.2f%s;%u;",mol,UNIT_STR_NG_L,fluValue);break;
        default:sprintf(str,"%s","error");break;
    }
}
static void MolDetectMol2StrWithUnit(u8 id,float mol,int unit,char* str)
{
    char bai,shi,ge;
    idU82C(id,&bai,&shi,&ge);
    switch (unit)
    {
        case UNIT_INDEX_MOL_L:sprintf(str,"%c%c%c;%.2f%s;",bai,shi,ge,mol,UNIT_STR_MOL_L);break;
        case UNIT_INDEX_MOL_ML:sprintf(str,"%c%c%c;%.2f%s;",bai,shi,ge,mol,UNIT_STR_MOL_ML);break;
        case UNIT_INDEX_MOL_UL:sprintf(str,"%c%c%c;%.2f%s;",bai,shi,ge,mol,UNIT_STR_MOL_UL);break;
        case UNIT_INDEX_G_L:sprintf(str,"%c%c%c;%.2f%s;",bai,shi,ge,mol,UNIT_STR_G_L);break;
        case UNIT_INDEX_MG_L:sprintf(str,"%c%c%c;%.2f%s;",bai,shi,ge,mol,UNIT_STR_MG_L);break;
        case UNIT_INDEX_NG_L:sprintf(str,"%c%c%c;%.2f%s;",bai,shi,ge,mol,UNIT_STR_NG_L);break;
        default:sprintf(str,"%s","unit error");break;
    }
}






/* ---- fluorescence detect function begin ---- */
static int setFluDataIndex2Text(u8 index)
{
    char dataBuf[8] = {0};
    if(index < FLUINDEXMAXNUM){
        //memset(dataBuf,0,sizeof(dataBuf));
        sprintf(dataBuf,"D%u",index);
        SetTextValue(PAGE_ID_2_FLUDETECT,CON_ID_FLUINDEXTEXT_2_7,(unsigned char*)dataBuf);
        return 0;
    }
    else{
        return 1;
    }
    
}
static void addFluData2Table(uchar id,u16 value)
{
    char dataBuf[20] = {0};
    char bai,shi,ge;

    idU82C(id,&bai,&shi,&ge);
    //memset(dataBuf,0,sizeof(dataBuf));
    sprintf(dataBuf,"%c%c%c;%d;",bai,shi,ge,value);
    Record_Add(PAGE_ID_2_FLUDETECT,CON_ID_FLUDATATABLE_2_6,(unsigned char*)dataBuf);
}
static void clearDluDataTable(void)
{
    Record_Clear(PAGE_ID_2_FLUDETECT,CON_ID_FLUDATATABLE_2_6);
}
/* ---- fluorescence detect function end ---- */


/* ---- history function begin ---- */
static int addHistoryIndex2Table(u8 index)
{
    char dataBuf[8] = {0};
    if(index < 999){
        //memset(dataBuf,0,sizeof(dataBuf));
        sprintf(dataBuf,"D%u;",index);
        Record_Add(PAGE_ID_2_HISTORY,CON_ID_HISTORYINDEXTABLE_3_3,(unsigned char*)dataBuf);
        return 0;
    }
    else{
        return 1;
    }
}
static void addHistoryFluData2Table(uchar id,u16 value)
{
    char dataBuf[20] = {0};
    char bai,shi,ge;

    idU82C(id,&bai,&shi,&ge);
    //memset(dataBuf,0,sizeof(dataBuf));
    sprintf(dataBuf,"%c%c%c;%d;",bai,shi,ge,value);
    Record_Add(PAGE_ID_2_HISTORY,CON_ID_HISTORYDATATABLE_3_2,(unsigned char*)dataBuf);
}
static void sendGetTableStrtCmd(uint16 screen_id,uint16 control_id,u16 row)
{
    Record_Get(screen_id,control_id,row);
}
/* ---- history function end ---- */


/* ---- establish equation function begin ---- */
static void setInputMol2Text(float mol,int unit)
{
    char dataBuf[20] = {0};

    inputMol2StrWithUnit(mol,unit,dataBuf);
    SetTextValue(PAGE_ID_2_EQUATION,CON_ID_EQUATIONMOLTEXT_5_10,(unsigned char*)dataBuf); 
}
static int setEquationIndex2Text(u8 index)
{
    char dataBuf[8] = {0};
    if(index < 50){
        sprintf(dataBuf,"E%u;",index);
        SetTextValue(PAGE_ID_2_EQUATION,CON_ID_EQUATIONINDEXTEXT_5_7,(unsigned char*)dataBuf); 
        return 0;
    }
    else{
        return 1;
    }
}
static void setEquationFunc2Text(float a,float b)
{
    char dataBuf[20] = {0};
    sprintf(dataBuf,"y=%.2fx+%.2f;",a,b);
    SetTextValue(PAGE_ID_2_EQUATION,CON_ID_EQUATIONFUNCTEXT_5_5,(unsigned char*)dataBuf); 
}
static void addEquationData2Table(u8 id,float mol,int unit,u16 fluvalue)
{
    char dataBuf[40] = {0};
    char bai,shi,ge;
    idU82C(id,&bai,&shi,&ge);
    dataBuf[0] = bai;
    dataBuf[1] = shi;
    dataBuf[2] = ge;
    dataBuf[3] = ';';
    MolAndFluValue2StrWithUnit(mol,unit,fluvalue,&dataBuf[4]);
    Record_Add(PAGE_ID_2_EQUATION,CON_ID_EQUATIONDATATABLE_5_6,(unsigned char*)dataBuf); 
}
/*mode=0-->返回浓度与单位,
* mode=1-->返回浓度,
* mode=2-->返回单位,
*/
static void getMolInputAndUnitFromText(int mode)
{
    switch (mode)
    {
        case 0:
        GetControlValue(PAGE_ID_3_MOLINPUT,CON_ID_INPUTMOLTEXT_9_21);HAL_Delay(100); 
        GetControlValue(PAGE_ID_3_MOLINPUT,CON_ID_INPUTUNITTEXT_9_22);HAL_Delay(10);break;
        case 1:
        GetControlValue(PAGE_ID_3_MOLINPUT,CON_ID_INPUTMOLTEXT_9_21);HAL_Delay(10);break;
        case 2:
        GetControlValue(PAGE_ID_3_MOLINPUT,CON_ID_INPUTUNITTEXT_9_22);HAL_Delay(10);break;
        default:break;
    }
}

/* ---- establish equation function end ---- */


/* ---- concentration detect function begin ---- */
static int setMolDetectIndex2Text(u8 index)
{
    char dataBuf[8] = {0};
    if(index < 50){
        sprintf(dataBuf,"E%u;",index);
        SetTextValue(PAGE_ID_2_MOL,CON_ID_MOLINDEXTEXT_6_6,(unsigned char*)dataBuf); 
        return 0;
    }else{
        return 1;
    }
}
static void setMolEquation2Text(float a,float b)
{
    char dataBuf[40] = {0};

    sprintf(dataBuf,"y=%.2fx+%.2f;",a,b);
    SetTextValue(PAGE_ID_2_MOL,CON_ID_MOLFUNCTEXT_6_5,(unsigned char*)dataBuf); 
}
static void addMolData2Table(u8 id,float mol,int unit)
{
    char dataBuf[40] = {0};

    MolDetectMol2StrWithUnit(id,mol,unit,dataBuf);
    Record_Add(PAGE_ID_2_MOL,CON_ID_MOLDATATABLE_6_4,(unsigned char*)dataBuf); 
}
static int addMolEquationIndex2Table(u16 index)
{
    char dataBuf[8] = {0};
    if(index < 999){
        sprintf(dataBuf,"E%u;",index);
        Record_Add(PAGE_ID_3_EQUATIONSELECT,CON_ID_SELECTINDEXTABLE_10_2,(unsigned char*)dataBuf); 
        return 0;
    }else{
        return 1;
    }
}
static void showMolEquation2Text(float a,float b)
{
    char dataBuf[40] = {0};

    sprintf(dataBuf,"y=%.2fx+%.2f",a,b);
    SetTextValue(PAGE_ID_3_EQUATIONSELECT,CON_ID_SELECTFUNCTEXT_10_5,(unsigned char*)dataBuf); 
}
static void addMolEquationPoints2Graph(u16* a,u16 len)
{
    unsigned char* data;
    int i = 0;
    int j = 0;

    data = (unsigned char*)malloc(2*len+1);
    for(i = 0;i < 2*len;i+=2,j++)
    {
        data[i] = (u8)(a[j]>>8);
        data[i+1] = (u8)(a[j]&0xff);
    }
    GraphChannelDataAdd(PAGE_ID_3_EQUATIONSELECT,CON_ID_SELECTDATAGRAPH_10_6,0,data,2*len);
}
static void addMolEquationOnePoint2Graph(u16 a)
{
    unsigned char dataBuf[2];

    dataBuf[0] = (u8)(a>>8);
    dataBuf[1] = (u8)(a&0xff);
    GraphChannelDataAdd(PAGE_ID_3_EQUATIONSELECT,CON_ID_SELECTDATAGRAPH_10_6,0,dataBuf,2);
}
/* ---- concentration detect function end ---- */

/* ---- screen other function begin ---- */
static void screenInit(void)
{
    /* ---- screen init function begin ---- */
}
static void screenSwitchPage(u16 pageId)
{
    SetScreen(pageId);
}


int screenObjRegister(SCREEN_STU_TYPE* obj)
{
    if(NULL != obj){
        obj->init = screenInit;
        obj->setFluDataIndex2Text_2_7 = setFluDataIndex2Text;
        obj->addFluData2Table_2_6 = addFluData2Table;
        obj->clearFluData2Table_2_6 = clearDluDataTable;
        obj->addHistoryIndex2Table_3_3 = addHistoryIndex2Table;
        obj->addHistoryFluData2Table_3_2 = addHistoryFluData2Table;
        obj->sendGetStrFromTableCmd_3_3 = sendGetTableStrtCmd;
        obj->setInputMol2Text_5_10 = setInputMol2Text;
        obj->setEquationIndex2Text_5_7 = setEquationIndex2Text;
        obj->setEquationFunc2Text_5_5 = setEquationFunc2Text;
        obj->addEquationData2Table_5_6 = addEquationData2Table;
        obj->sendGetMolInputFromTextCmd_9_21_22 = getMolInputAndUnitFromText;
        obj->setMolDetectIndex2Text_6_6 = setMolDetectIndex2Text;
        obj->setMolEquation2Text_6_5 = setMolEquation2Text;
        obj->addMolData2Table_6_4 = addMolData2Table;
        obj->addMolEquationIndex2Table_10_2 = addMolEquationIndex2Table;
        obj->showMolEquation2Text_10_5 = showMolEquation2Text;
        obj->sendGetEquIndexFromTableCmd_10_2 = sendGetTableStrtCmd;
        obj->addMolEquationPoints2Graph_10_6 = addMolEquationPoints2Graph;
        obj->addMolEquationOnePoint2Graph_10_6 = addMolEquationOnePoint2Graph;
        obj->recvBufClear=queueClear;
        obj->recvBufPush=queuePush;
        obj->recvBufPop=queuePop;
        obj->recvGetCmd=queueTakeCmd;
        obj->recvGetBufSize=queueGetSize;
        obj->switchPage = screenSwitchPage;
        return 0;
    }else{
        return 1;
    }
}

/* ---- screen init function begin ---- */




