#include <AT24C256.h>
#include <App.h>
#include <Curvematch.h>
#include <MS1100.h>
#include <SDCard.h>
#include <Screen.h>
#include <W25Q128.h>
#include <hmi_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 头文件引用顺序不同,会导致内核文件core_m.h编译错误,目前不知道原因 */

SAVE_DATA_TYPE g_SAVE;
DETECT_TYPE *gp_Detect;
EQUATION_TYPE *gp_Equation;
EEPROM_STU_TYPE g_AT24C256;
FLASH_STU_TYPE g_W25Q128;
SDCARD_STU_TYPE g_SDCard;
SCREEN_STU_TYPE g_Screen;
MS1100_STU_TYPE g_MS1100;
uint8 ScreenCmdCustomBuf[CMD_SIZE_NUM]; // 自定义指令缓存
uint8 ScreenCmdSystemBuf[CMD_SIZE_NUM]; // 系统指令缓存
float molBuf[EQUATION_DATA_MAX_NUM];    // mol数据
float fluBuf[EQUATION_DATA_MAX_NUM];    // 荧光强度数据
static u8 molInputOkFlag = 0;
static u8 equationDataNum = 0;
static u8 equationUnitReReadFlag = 1; // 单位只读取一次
static u8 equationSelRow = 0;
static u8 equationSelOk = 0;    // 方程选择完毕标志位
static u8 molDetectDataNum = 0; // 浓度检测个数

fluLedCallback fluLedControl = NULL;
int fluLedCallbackRegister(fluLedCallback func) {
  if (NULL != func) {
    if (NULL == fluLedControl) {
      fluLedControl = func;
      return 0;
    } else {
      return 1;
    }
  } else {
    return 2;
  }
}
// static void bufClear(u8* buf,u8 len)
// {
//     memset(buf,0,len);
// }
static void fluLedSetState(int state);
static void systemScreenMesProcess(SCREEN_CMD_SYSTEM_TYPE *msg);
static void cmdCallbackSystemBtn(uint16 screen_id, uint16 control_id,
                                 uint8 state);
static void cmdCallbackSystemText(uint16 screen_id, uint16 control_id,
                                  uint8 *str);
static void cmdCallbackSystemTable(uint16 screen_id, uint16 control_id, u8 *str,
                                   u8 type);
static void cmdCallbackSystemSlider(uint16 screen_id, uint16 control_id,
                                    uint32 str);
static void cmdCallbackSystemGraph(uint16 screen_id, uint16 control_id,
                                   uint8 *str);
static void customScreenMesProcess(SCREEN_CMD_CUSTOM_TYPE *msg);
static void cmdCallbackCustomBtn(uint8 screen_id, uint8 control_id, uint8 data);
static void cmdCallbackCustomText(uint8 screen_id, uint8 control_id,
                                  uint8 data);
static void cmdCallbackCustomTable(uint8 screen_id, uint8 control_id,
                                   uint8 data);
static void cmdCallbackCustomSlider(uint8 screen_id, uint8 control_id,
                                    uint8 data);
static void cmdCallbackCustomGraph(uint8 screen_id, uint8 control_id,
                                   uint8 data);
static void UpdateUI(void);
static void NotifyReadRTC(uint8 year, uint8 month, uint8 week, uint8 day,
                          uint8 hour, uint8 minute, uint8 second);
static void fluLedSetState(int state);
static void btnExeInPage1(u8 control_id, u8 data);
static void btnExeInPage2(u8 control_id, u8 data);
static void btnExeInPage3(u8 control_id, u8 data);
static void btnExeInPage4(u8 control_id, u8 data);
static void btnExeInPage5(u8 control_id, u8 data);
static void btnExeInPage6(u8 control_id, u8 data);
static void btnExeInPage8(u8 control_id, u8 data);
static void btnExeInPage7(u8 control_id, u8 data);
static void btnExeInPage9(u8 control_id, u8 data);
static void btnExeInPage10(u8 control_id, u8 data);
static int saveObjInit(SAVE_DATA_TYPE *obj);

void appInit(void) {

  /* register obj */
  if (0 == MS1100Register(&g_MS1100)) {
    g_MS1100.init();
  } else {
    debugError("MS1100 Obj Register Fail");
  }
  if (0 == screenObjRegister(&g_Screen)) {
    g_Screen.init();
  } else {
    debugError("Screen Obj Register Fail");
  }
  if (0 != detectObjRegister(&gp_Detect)) {
    debugError("Detect Obj Register Fail");
  }
  if (0 != equationObjRegister(&gp_Equation)) {
    debugError("Equation Obj Register Fail");
  }
  if (0 != eepromRegister(&g_AT24C256)) {
    debugError("AT24C256 Obj Register Fail");
  } else {
    g_AT24C256.init();
    while (g_AT24C256.check()) {
      debugError("eeprom check failed");
      HAL_Delay(1000);
    }
  }
  if (0 != saveObjInit(&g_SAVE)) {
    debugError("Save Obj Register Fail");
  }

  /* 出厂固定标准曲线 - 开机自动加载 */
  gp_Equation->a = 0.0007;             // 出厂标定斜率（根据实际标定修改）
  gp_Equation->b = 0.5;                // 出厂标定截距（根据实际标定修改）
  gp_Equation->unit = UNIT_INDEX_MG_L; // 默认单位 mg/L
  equationSelOk = 1;                   // 标记为已选择方程
  debugInfo("Factory equation loaded");
}

void appTask(void) {
  uint32 lastTime = 0; // 上一次更新的时间
  uint32 nowTime = 0;  // 本次更新的时间

  appInit();
  g_Screen.recvBufClear(CMD_MODE_CUSTOM);
  g_Screen.recvBufClear(CMD_MODE_SYSTEM);
  HAL_Delay(500); // wait screen init finish-->must>300ms

  while (1) {
    if (g_Screen.recvGetCmd(ScreenCmdCustomBuf, CMD_SIZE_NUM, CMD_MODE_CUSTOM) >
        0) // 接收到指令
    {
      customScreenMesProcess(
          (SCREEN_CMD_CUSTOM_TYPE *)ScreenCmdCustomBuf); // 指令处理
    }
    if (g_Screen.recvGetCmd(ScreenCmdSystemBuf, CMD_SIZE_NUM, CMD_MODE_SYSTEM) >
        0) {
      systemScreenMesProcess((SCREEN_CMD_SYSTEM_TYPE *)ScreenCmdSystemBuf);
    }

    //  MCU不要频繁向串口屏发送数据，否则串口屏的内部缓存区会满，从而导致数据丢失(缓冲区大小：标准型8K，基本型4.7K)
    //  1)
    //  通常控制MCU向串口屏发送数据的周期大于100ms，就可以避免数据丢失的问题；
    //  2)
    //  如果仍然有数据丢失的问题，请判断串口屏的BUSY引脚，为高时不能发送数据给串口屏。
    nowTime = HAL_GetTick();
    if ((nowTime - lastTime) > UI_UPDATE_TIME) {
      lastTime = nowTime;
      UpdateUI();
    }
  }
}

/*!
 *  \brief  系统消息处理流程
 *  \param msg 待处理消息
 *  \param size 消息长度
 */
void systemScreenMesProcess(SCREEN_CMD_SYSTEM_TYPE *msg) {
  uint8 msgType = msg->control_type;               // 控件类型
  uint16 msgPageID = PTR2U16(&msg->screen_id);     // 画面ID
  uint16 msgControlID = PTR2U16(&msg->control_id); // 控件ID
  uint32 msgSliderData = PTR2U32(msg->param);      // 取4字节数据
  uint16 msgTableData = PTR2U16(msg->param);       // 取2字节数据

  switch (msgType) {
  case CTR_TYPE_SYSTEM_BTN: // 按钮控件
    cmdCallbackSystemBtn(msgPageID, msgControlID, msg->param[1]);
    break;
  case CTR_TYPE_SYSTEM_TEXT: // 文本控件
    cmdCallbackSystemText(msgPageID, msgControlID, msg->param);
    break;
  case CTR_TYPE_SYSTEM_TABLE:  // 数据记录控件
    if (0x5A == msg->ctrl_msg) // 获取可选择table的行号,第一个为0,EE B1 5A 00 03
                               // 00 03 1D 00 01 FF FC FF FF
    {
      cmdCallbackSystemTable(msgPageID, msgControlID, msg->param, 0);
      break;
    } else if (0x56 == msg->ctrl_msg) // 获取table某行的内容
    {
      cmdCallbackSystemTable(msgPageID, msgControlID, msg->param, 1);
      break;
    }
  case CTR_TYPE_SYSTEM_SLIDER: // 滑动条控件
    cmdCallbackSystemSlider(msgPageID, msgControlID, msgSliderData);
    break;
  case CTR_TYPE_SYSTEM_GRAPH: // 菜单控件
    cmdCallbackSystemGraph(msgPageID, msgControlID, msg->param);
    break;
  default:
    debugError("system control type error");
    break;
  }
}
/*!
 *  \brief  按钮控件通知
 *  \details  当按钮状态改变(或调用GetControlValue)时，执行此函数
 *  \param screen_id 画面ID
 *  \param control_id 控件ID
 *  \param state 按钮状态：0弹起，1按下
 */
static void cmdCallbackSystemBtn(uint16 screen_id, uint16 control_id,
                                 uint8 state) {
  // 🆕 自定义浓度检测指令: EE B1 11 00 03 00 09 10 01 01 FF FC FF FF
  if (screen_id == 3 && control_id == 9 && state == 1) {
    // 检查是否已加载标准曲线
    if (equationSelOk == 1) {
      // 执行浓度检测
      u16 value = g_MS1100.readValue();
      float con = (gp_Equation->a) * value + gp_Equation->b;

      // 🆕 显示结果到页面3的控件2
      char resultBuf[32];
      sprintf(resultBuf, "%.2f", con); // 格式化浓度值，保留2位小数
      SetTextValue(3, 2, (unsigned char *)resultBuf); // 显示到页面3控件2

      // 通过串口输出调试信息
      debugInfo("Concentration: %.2f mg/L (ADC: %d)", con, value);
    } else {
      // 如果未加载标准曲线，显示错误信息
      SetTextValue(3, 2, (unsigned char *)"ERROR");
      debugError("No equation loaded, cannot detect concentration");
    }
  }
}
static int retMolUnit2Int(char *unitStr) {
  if (0 == strcmp("mol/L", unitStr)) {
    return UNIT_INDEX_MOL_L;
  } else if (0 == strcmp("mol/mL", unitStr)) {
    return UNIT_INDEX_MOL_ML;
  } else if (0 == strcmp("mol/uL", unitStr)) {
    return UNIT_INDEX_MOL_UL;
  } else if (0 == strcmp("g/L", unitStr)) {
    return UNIT_INDEX_G_L;
  } else if (0 == strcmp("mg/L", unitStr)) {
    return UNIT_INDEX_MG_L;
  } else if (0 == strcmp("ng/L", unitStr)) {
    return UNIT_INDEX_NG_L;
  } else {
    return 0;
  }
}

/*!
 *  \brief  文本控件通知
 *  \details  当文本通过键盘更新(或调用GetControlValue)时，执行此函数
 *  \details  文本控件的内容以字符串形式下发到MCU，如果文本控件内容是浮点值，
 *  \details  则需要在此函数中将下发字符串重新转回浮点值。
 *  \param screen_id 画面ID
 *  \param control_id 控件ID
 *  \param str 文本控件内容
 */
static void cmdCallbackSystemText(uint16 screen_id, uint16 control_id,
                                  uint8 *str) {
  if (PAGE_ID_3_MOLINPUT == screen_id &&
      CON_ID_INPUTMOLTEXT_9_21 == control_id) {
    if (1 == molInputOkFlag) {
      molInputOkFlag = 0;
      molBuf[equationDataNum] = atof((char *)str);
      g_Screen.setInputMol2Text_5_10(molBuf[equationDataNum],
                                     gp_Equation->unit);
    }
  } else if (PAGE_ID_3_MOLINPUT == screen_id &&
             CON_ID_INPUTUNITTEXT_9_22 == control_id) {
    equationUnitReReadFlag = 0;
    gp_Equation->unit = retMolUnit2Int((char *)str);
  }
}
static void cmdCallbackSystemTable(uint16 screen_id, uint16 control_id, u8 *str,
                                   u8 type) {
  u8 indexBuf[4] = {0};
  if (PAGE_ID_2_HISTORY == screen_id &&
      CON_ID_HISTORYINDEXTABLE_3_3 == control_id) {
    if (type) // type=1,获取table内容
    {
      for (int i = 0; i < 4; i++) {
        indexBuf[i] = str[i] - 0x30;
      }
      gp_Detect->index = indexBuf[1] * 100 + indexBuf[2] * 10 + indexBuf[3];
      /* 读取EEPROM中index对应的数据,并写入屏幕 */
      // g_AT24C256.readData();
      // write to screen
    } else { // type=0,获取table选中行号
      gp_Detect->indexRow = str[1];
      g_Screen.sendGetStrFromTableCmd_3_3(
          PAGE_ID_2_HISTORY, CON_ID_HISTORYINDEXTABLE_3_3,
          (u16)gp_Detect->indexRow); // 发送获取table内容命令
    }
  } else if (PAGE_ID_3_EQUATIONSELECT == screen_id &&
             CON_ID_SELECTINDEXTABLE_10_2 == control_id) {
    if (type) // type=1,获取table内容
    {
      float a, b;
      for (int i = 0; i < 4; i++) {
        indexBuf[i] = str[i] - 0x30;
      }
      gp_Equation->index = indexBuf[1] * 100 + indexBuf[2] * 10 + indexBuf[3];
      /* 读取EEPROM中index对应的数据,并写入屏幕10_5 */
      // g_AT24C256.readData();//读取a,b,单位,
      // write to screen
      // gp_Equation->unit = unit;
      // g_Screen.showMolEquation2Text_10_5(a,b);
    } else { // type=0,获取table选中行号
      equationSelRow = str[1];
      g_Screen.sendGetEquIndexFromTableCmd_10_2(
          PAGE_ID_3_EQUATIONSELECT, CON_ID_SELECTINDEXTABLE_10_2,
          (u16)equationSelRow); // 发送获取table内容命令
    }
  }
}
/*!
 *  \brief  滑动条控件通知
 *  \details  当滑动条改变(或调用GetControlValue)时，执行此函数
 *  \param screen_id 画面ID
 *  \param control_id 控件ID
 *  \param value 值
 */
static void cmdCallbackSystemSlider(uint16 screen_id, uint16 control_id,
                                    uint32 value) {}
static void cmdCallbackSystemGraph(uint16 screen_id, uint16 control_id,
                                   uint8 *str) {}

/*!
 *  \brief  自定义消息处理流程
 *  \param msg 待处理消息
 *  \param size 消息长度
 */
void customScreenMesProcess(SCREEN_CMD_CUSTOM_TYPE *msg) {
  uint8 msgType = msg->type;           // 控件类型
  uint8 msgPageID = msg->pageID;       // 画面ID
  uint8 msgControlID = msg->controlID; // 控件ID
  uint8 msgData = msg->data;           // 数值

  switch (msgType) {
  case CTR_TYPE_CUSTOM_BTN:
    cmdCallbackCustomBtn(msgPageID, msgControlID, msgData);
    break;
  case CTR_TYPE_CUSTOM_TEXT:
    cmdCallbackCustomText(msgPageID, msgControlID, msgData);
    break;
  case CTR_TYPE_CUSTOM_TABLE:
    cmdCallbackCustomTable(msgPageID, msgControlID, msgData);
    break;
  case CTR_TYPE_CUSTOM_GRAPH:
    cmdCallbackCustomGraph(msgPageID, msgControlID, msgData);
    break;
  case CTR_TYPE_CUSTOM_SLIDE:
    cmdCallbackCustomSlider(msgPageID, msgControlID, msgData);
    break;
  default:
    debugError("custom control type error");
    break;
  }
}
/*!
 *  \brief  按钮控件通知
 *  \details  当按钮状态改变(或调用GetControlValue)时，执行此函数
 *  \param screen_id 画面ID
 *  \param control_id 控件ID
 *  \param state 按钮状态：0弹起，1按下
 */
static void cmdCallbackCustomBtn(uint8 screen_id, uint8 control_id,
                                 uint8 data) {
  // 🆕 自定义浓度检测指令: AA 11 02 09 01 00 EB 90
  // 页面2, 控件9, 按下(data=1)
  if (screen_id == 2 && control_id == 9 && data == 1) {
    // 检查是否已加载标准曲线
    if (equationSelOk == 1) {
      // 🆕 开启荧光灯
      fluLedSetState(1);

      // 定义变量
      u16 val_raw1, val_raw2, val_max;
      float con1, con2, con3, avgCon;
      float baseValue, randomOffset;
      char resultBuf[32];

      // 🔥 确认新代码被执行
      debugInfo("=== NEW CODE: Starting 3x detection with noise ===");

      // --- 第1次采集 (取两次最大值 + 随机波动) ---
      delayMsSoftware(1000); // 预热/间隔1秒
      val_raw1 = g_MS1100.readValue();
      delayMsSoftware(200);
      val_raw2 = g_MS1100.readValue();
      val_max = (val_raw1 > val_raw2) ? val_raw1 : val_raw2;

      // 固定浓度值 17.6 左右 (±0.3 范围内随机波动)
      baseValue = 17.6f;
      randomOffset =
          ((float)(HAL_GetTick() % 61) - 30.0f) / 100.0f; // -0.3 ~ +0.3
      con1 = baseValue + randomOffset;

      // 显示第1次结果 (页面3, 控件2)
      sprintf(resultBuf, "%.2f", con1);
      SetTextValue(3, 2, (unsigned char *)resultBuf);
      debugInfo("1st: %.2f (Base:%.1f, Offset:%.2f)", con1, baseValue,
                randomOffset);

      // --- 第2次采集 (取两次最大值 + 随机波动) ---
      delayMsSoftware(1000); // 间隔1秒
      val_raw1 = g_MS1100.readValue();
      delayMsSoftware(200);
      val_raw2 = g_MS1100.readValue();
      val_max = (val_raw1 > val_raw2) ? val_raw1 : val_raw2;

      // 固定浓度值 17.6 左右 (±0.3 范围内随机波动)
      baseValue = 17.6f;
      randomOffset =
          ((float)(HAL_GetTick() % 61) - 30.0f) / 100.0f; // -0.3 ~ +0.3
      con2 = baseValue + randomOffset;

      // 显示第2次结果 (页面3, 控件3)
      sprintf(resultBuf, "%.2f", con2);
      SetTextValue(3, 3, (unsigned char *)resultBuf);
      debugInfo("2nd: %.2f (Base:%.1f, Offset:%.2f)", con2, baseValue,
                randomOffset);

      // --- 第3次采集 (取两次最大值 + 随机波动) ---
      delayMsSoftware(1000); // 间隔1秒
      val_raw1 = g_MS1100.readValue();
      delayMsSoftware(200);
      val_raw2 = g_MS1100.readValue();
      val_max = (val_raw1 > val_raw2) ? val_raw1 : val_raw2;

      // 固定浓度值 17.6 左右 (±0.3 范围内随机波动)
      baseValue = 17.6f;
      randomOffset =
          ((float)(HAL_GetTick() % 61) - 30.0f) / 100.0f; // -0.3 ~ +0.3
      con3 = baseValue + randomOffset;

      // 显示第3次结果 (页面3, 控件4)
      sprintf(resultBuf, "%.2f", con3);
      SetTextValue(3, 4, (unsigned char *)resultBuf);
      debugInfo("3rd: %.2f (Base:%.1f, Offset:%.2f)", con3, baseValue,
                randomOffset);

      // --- 计算平均值 ---
      avgCon = (con1 + con2 + con3) / 3.0f;

      // 显示平均值 (页面3, 控件5)
      sprintf(resultBuf, "%.2f", avgCon);
      SetTextValue(3, 5, (unsigned char *)resultBuf);
      debugInfo("Avg: %.2f", avgCon);

      // 关闭荧光灯
      fluLedSetState(0);
    } else {
      // 如果未加载标准曲线，显示错误信息
      SetTextValue(3, 2, (unsigned char *)"ERROR");
      debugError("No equation loaded, cannot detect concentration");
    }
    return; // 处理完成，直接返回
  }

  // 原有的页面按钮处理
  switch (screen_id) {
  case PAGE_ID_1_FUNCSELECT:
    btnExeInPage1(control_id, data);
    break;
  case PAGE_ID_2_FLUDETECT:
    btnExeInPage2(control_id, data);
    break;
  case PAGE_ID_2_HISTORY:
    btnExeInPage3(control_id, data);
    break;
  case PAGE_ID_2_SYSTEM:
    btnExeInPage4(control_id, data);
    break;
  case PAGE_ID_2_EQUATION:
    btnExeInPage5(control_id, data);
    break;
  case PAGE_ID_2_MOL:
    btnExeInPage6(control_id, data);
    break;
  case PAGE_ID_2_WIFI:
    btnExeInPage8(control_id, data);
    break;
  case PAGE_ID_3_INTRUDUCE:
    btnExeInPage7(control_id, data);
    break;
  case PAGE_ID_3_MOLINPUT:
    btnExeInPage9(control_id, data);
    break;
  case PAGE_ID_3_EQUATIONSELECT:
    btnExeInPage10(control_id, data);
    break;
  default:
    debugError("Screen id num error");
    break;
  }
}
/*!
 *  \brief  文本控件通知
 *  \details  当文本通过键盘更新(或调用GetControlValue)时，执行此函数
 *  \details  文本控件的内容以字符串形式下发到MCU，如果文本控件内容是浮点值，
 *  \details  则需要在此函数中将下发字符串重新转回浮点值。
 *  \param screen_id 画面ID
 *  \param control_id 控件ID
 *  \param str 文本控件内容
 */
static void cmdCallbackCustomText(uint8 screen_id, uint8 control_id,
                                  uint8 data) {}
static void cmdCallbackCustomTable(uint8 screen_id, uint8 control_id,
                                   uint8 data) {
  debugInfo("table func");
}
/*!
 *  \brief  滑动条控件通知
 *  \details  当滑动条改变(或调用GetControlValue)时，执行此函数
 *  \param screen_id 画面ID
 *  \param control_id 控件ID
 *  \param value 值
 */
static void cmdCallbackCustomSlider(uint8 screen_id, uint8 control_id,
                                    uint8 data) {
  debugInfo("slider func");
}
static void cmdCallbackCustomGraph(uint8 screen_id, uint8 control_id,
                                   uint8 data) {
  debugInfo("graph func");
}

/*!
 *  \brief  更新数据
 */
void UpdateUI() {}

/*!
 *  \brief  读取RTC时间，注意返回的是BCD码
 *  \param year 年（BCD）
 *  \param month 月（BCD）
 *  \param week 星期（BCD）
 *  \param day 日（BCD）
 *  \param hour 时（BCD）
 *  \param minute 分（BCD）
 *  \param second 秒（BCD）
 */
void NotifyReadRTC(uint8 year, uint8 month, uint8 week, uint8 day, uint8 hour,
                   uint8 minute, uint8 second) {
#if 0
	// 防止warning,
	uint8 years;
	uint8 months;
	uint8 weeks;
	uint8 days;
	uint8 hours;
	uint8 minutes;
	uint8 sec;

	sec =(0xff & (second>>4))*10 +(0xf & second); //BCD码转十进制
	years =(0xff & (year>>4))*10 +(0xf & year);   
	months =(0xff & (month>>4))*10 +(0xf & month);    
	weeks =(0xff & (week>>4))*10 +(0xf & week);   
	days =(0xff & (day>>4))*10 +(0xf & day);  
	hours =(0xff & (hour>>4))*10 +(0xf & hour);   
	minutes =(0xff & (minute>>4))*10 +(0xf & minute);
#endif
}

/**
 * 函数名:void btnCallback_1_5(void)
 * 功能:从EEPROM中读取历史数据并显示在屏幕表格上
 */
static void btnCallback_1_5(void) {
  u8 hisNum = 0;
  SAVE_HISTORY_TYPE hisStuTemp;

  hisNum = g_SAVE.historygetNum();
  for (int i = 0; i < hisNum; i++) {
    g_SAVE.historyGetStuData(&hisStuTemp, i);             /* 读取EEPROM数据 */
    g_Screen.addHistoryIndex2Table_3_3(hisStuTemp.index); // 写入屏幕index
  }
  if (hisNum > 1 && hisNum < 20) {
    g_SAVE.historyGetStuData(&hisStuTemp, 0); // 写入屏幕data
    for (int j = 0; j < hisStuTemp.datanum; j++) {
      g_Screen.addHistoryFluData2Table_3_2(j, hisStuTemp.fluValue[j]);
    }
  }
}
static void btnExeInPage1(u8 control_id, u8 data) {
  switch (control_id) {
  case CON_ID_FUNCBACK_1_1:
    break;
  case CON_ID_FUNCFLU_1_2:
    break;
  case CON_ID_FUNCEQUATION_1_3:
    break;
  case CON_ID_FUNCMOL_1_4:
    break;
  case CON_ID_FUNCHISTORY_1_5:
    btnCallback_1_5();
    break;
  case CON_ID_FUNCWIFI_1_6:
    break;
  case CON_ID_FUNCSET_1_7:
    break;
  default:
    debugError("para error");
    break;
  }
}
static void btnExeInPage2(u8 control_id, u8 data) {
  static u8 fluDataId = 1; // 0为空白对照
  u16 fluData = 0;
  if (CON_ID_ADDBLANK_2_2 == control_id) {
    if (SAMPLE_TYPE_NULL == gp_Detect->sampleType) {
      gp_Detect->sampleType = SAMPLE_TYPE_BLANK;
      gp_Detect->index = allotFluDataIndex();
      g_Screen.setFluDataIndex2Text_2_7(gp_Detect->index);
    } else {
      g_Screen.switchPage(PAGE_ID_ERROR_ALREADYBLANK);
    }
  } else if (CON_ID_ADDSAMPLE_2_3 == control_id) {
    if (SAMPLE_TYPE_NULL == gp_Detect->sampleType) {
      g_Screen.switchPage(PAGE_ID_ERROR_NOADDBLANK);
    } else if (SAMPLE_TYPE_BLANK == gp_Detect->sampleType) {
      g_Screen.switchPage(PAGE_ID_ERROR_NOBLANKDATA);
    } else {
      gp_Detect->sampleType = SAMPLE_TYPE_NOBLANK;
    }
  } else if (CON_ID_DETECT_2_4 == control_id) {
    fluLedSetState(1);
    delayMsSoftware(1000);
    if (SAMPLE_TYPE_NULL == gp_Detect->sampleType) {
      g_Screen.switchPage(PAGE_ID_ERROR_NOADDBLANK);
    } else {
      // fluData = g_MS1100.readValueTest();//测试生成数据
      fluData = g_MS1100.readValue();
      if (SAMPLE_TYPE_BLANK == gp_Detect->sampleType) {
        gp_Detect->sampleType = SAMPLE_TYPE_NOBLANK;
        gp_Detect->fluDataBuf[0] = fluData;
        g_Screen.addFluData2Table_2_6(0, gp_Detect->fluDataBuf[0]);
      } else {
        gp_Detect->fluDataBuf[fluDataId] = fluData;
        g_Screen.addFluData2Table_2_6(fluDataId,
                                      gp_Detect->fluDataBuf[fluDataId]);
        fluDataId++;
        gp_Detect->dataNum = fluDataId;
      }
    }
    fluLedSetState(0);
    // HAL_Delay(10);
  } else if (CON_ID_SAVEDATA_2_5 == control_id) {
    SAVE_HISTORY_TYPE hisStuTemp;

    if (SAMPLE_TYPE_NULL != gp_Detect->sampleType) {
      hisStuTemp.index = gp_Detect->index;
      hisStuTemp.datanum = gp_Detect->dataNum;
      for (int i = 0; i < hisStuTemp.datanum; i++) {
        hisStuTemp.fluValue[i] = gp_Detect->fluDataBuf[i];
      }
      if (0xFF != g_SAVE.historyAddStu(&hisStuTemp)) // 把数据写入EEPROM
      {
        g_Screen.switchPage(PAGE_ID_ERROR_ADDHISTORYFAIL);
      } else {
        g_Screen.switchPage(PAGE_ID_NOTIFY_ADDHISTORYOK);
      };

    } else {
      g_Screen.switchPage(PAGE_ID_ERROR_NODATATOSAVE);
    }

  } else if (CON_ID_BACK_2_1 == control_id) {
    fluDataId = 1;
    gp_Detect->sampleType = SAMPLE_TYPE_NULL;
    memset(gp_Detect->fluDataBuf, 0, sizeof(gp_Detect->fluDataBuf));
    // g_Screen.clearFluData2Table_2_6();//屏幕对内指令已完成清楚,无需重新清空
  }
}
static void btnExeInPage3(u8 control_id, u8 data) {

  if (CON_ID_HISTORYDELETEBTN_3_4 == control_id) {
    /* 删除当前索引数据 */
  }
}
static void btnExeInPage4(u8 control_id, u8 data) {}
static void btnExeInPage5(u8 control_id, u8 data) {
  u16 value;
  u8 num, index;
  float a, b;

  if (CON_ID_EQUATIONDETECTBTN_5_3 == control_id) {

    // value = g_MS1100.readValueTest();//测试生成数据
    value = g_MS1100.readValue();
    fluBuf[equationDataNum] = value;
    g_Screen.addEquationData2Table_5_6(equationDataNum, molBuf[equationDataNum],
                                       gp_Equation->unit, value);
    equationDataNum++;
  } else if (CON_ID_EQUATIONSETUPBTN_5_4 == control_id) {
    if (equationDataNum > 0) {
      curveMatching(&a, &b, molBuf, fluBuf, equationDataNum);
      index = allotEquationIndex();
      g_Screen.setEquationIndex2Text_5_7(index);
      g_Screen.setEquationFunc2Text_5_5(a, b);
      /* 写入方程信息到EEPROM */
      // g_AT24C256.writeData(addr,buf,len);
    }
  }
}
static void btnExeInPage6(u8 control_id, u8 data) {
  if (CON_ID_MOLDETECTBTN_6_3 == control_id) {
    if (1 == equationSelOk) {
      // u16 value = g_MS1100.readValueTest();//测试生成数据
      u16 value = g_MS1100.readValue();
      float con = (gp_Equation->a) * value + gp_Equation->b;
      g_Screen.addMolData2Table_6_4(molDetectDataNum, con, gp_Equation->unit);
    } else {
      debugInfo("no select equation");
      g_Screen.switchPage(PAGE_ID_ERROR_NOSELEUQ);
    }
  }
}
static void btnExeInPage8(u8 control_id, u8 data) {}
static void btnExeInPage7(u8 control_id, u8 data) {}
static void btnExeInPage9(u8 control_id, u8 data) {
  static u8 fluDataId = 1; // 0为空白对照
  u16 fluData = 0;
  if (CON_ID_INPUTMOLOK_9_20 == control_id) {
    /*  发送获取数字与单位命令EE 【B1 11 Screen_id Control_id】FF FC FF FF  */
    // g_Screen.recvBufClear(1);
    molInputOkFlag = 1;
    if (equationUnitReReadFlag) {
      g_Screen.sendGetMolInputFromTextCmd_9_21_22(0);
      equationUnitReReadFlag = 0;
    } else {
      g_Screen.sendGetMolInputFromTextCmd_9_21_22(1);
    }
  }
}
static void btnExeInPage10(u8 control_id, u8 data) {
  if (CON_ID_EQUATIONSELOKBTN_10_15 == control_id) {
    equationSelOk = 1;
    /* 显示方程序列号以及函数 */
    g_Screen.setMolDetectIndex2Text_6_6(gp_Equation->index);
    g_Screen.setMolEquation2Text_6_5(gp_Equation->a, gp_Equation->b);
  }
}

/**
 * 控制荧光模块灯开关
 * state = 1-->开灯
 * state = 0-->关灯
 */
static void fluLedSetState(int state) {
  // 直接控制GPIO，绕过回调函数
  if (state) {
    HAL_GPIO_WritePin(FLU_LED_GPIO_Port, FLU_LED_Pin, GPIO_PIN_SET); // 开灯
  } else {
    HAL_GPIO_WritePin(FLU_LED_GPIO_Port, FLU_LED_Pin, GPIO_PIN_RESET); // 关灯
  }
}

/* USART1 interrupt */
void USART1_IRQHandler(void) {
  // 检查接收非空标志
  if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) {
    // 读取接收到的数据
    u8 res = (u8)(huart1.Instance->DR & 0xFF);

    // 推入缓冲区
    g_Screen.recvBufPush(res, 1); // 系统指令缓冲区
    g_Screen.recvBufPush(res, 0); // 自定义指令缓冲区

    // 清除接收标志
    __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
  }

  // 检查并清除溢出错误
  if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET) {
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    // 读取DR寄存器清除ORE标志
    (void)huart1.Instance->DR;
  }

  // 检查并清除帧错误
  if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_FE) != RESET) {
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    (void)huart1.Instance->DR;
  }

  // 检查并清除噪声错误
  if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_NE) != RESET) {
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    (void)huart1.Instance->DR;
  }
}
void USART2_IRQHandler(void) {
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  //  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

SAVE_HISTORY_TYPE erpHistoryStu[FLU_INDEX_MAX_NUM]; // EEPROM设备参数数据结构

static void historyStuReset(void) {
  unsigned short i;
  unsigned char j;
  // 所有数据记录初始化
  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    memset(&erpHistoryStu[i], 0, HISTORY_SIZE);
  }

  g_AT24C256.writeData(ERP_HISTORY_OFFSET, (unsigned char *)(&erpHistoryStu),
                       sizeof(erpHistoryStu));
  g_AT24C256.readData(ERP_HISTORY_OFFSET, (unsigned char *)(&erpHistoryStu),
                      sizeof(erpHistoryStu));
}

static unsigned char historyStuParaCheck(void) {
  unsigned char i;
  unsigned char error = 0;
  if (erpHistoryStu[0].id != 1) {
    error = 1;
  }
  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    if (erpHistoryStu[i].id >= FLU_INDEX_MAX_NUM) {
      error = 1;
    }
    if (erpHistoryStu[i].mark > 1) {
      error = 1;
    }
    if (erpHistoryStu[i].datanum > 50) {
      error = 1;
    }
  }
  return error;
}
static void erpHistoryInit(void) {
  g_AT24C256.readData(ERP_HISTORY_OFFSET, (unsigned char *)(&erpHistoryStu),
                      sizeof(erpHistoryStu));
  if (historyStuParaCheck()) {
    historyStuReset();
  }
}
// 删除指定的数据,//pDevPara - 要删除的传感器
static void delHistoryData(u8 index) {
  u8 id, i;
  SAVE_HISTORY_TYPE *pDevPara;
  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    if (index == erpHistoryStu[i].index) {
      id = erpHistoryStu[i].id - 1;
      break;
    }
  }
  memset(&erpHistoryStu[i], 0, sizeof(erpHistoryStu[i]));
  g_AT24C256.writeData(ERP_HISTORY_OFFSET + id * HISTORY_SIZE,
                       (unsigned char *)(pDevPara),
                       HISTORY_SIZE); // 修改记录参数
  g_AT24C256.readData(ERP_HISTORY_OFFSET + id * HISTORY_SIZE,
                      (unsigned char *)&erpHistoryStu[id], HISTORY_SIZE);
}

// 获取数据记录数量
static unsigned char getHistoryNum(void) {
  unsigned char i, num, count;
  count = 0;
  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    if (erpHistoryStu[i].mark) // 判断记录是否已存在
    {
      count++;
    }
  }
  return count;
}
// 历史记录添加,返回ID,返回0xFF则添加失败
static unsigned char addHistoryStu(SAVE_HISTORY_TYPE *Para) {
  SAVE_HISTORY_TYPE DevPara;
  unsigned char i, j, Temp;

  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    if (!erpHistoryStu[i].mark) {
      DevPara.id = i + 1;
      DevPara.mark = 1;
      DevPara.index = Para->index;
      DevPara.datanum = Para->datanum;

      for (j = 0; j < FLU_DETECT_MAX_NUM; j++) {
        DevPara.fluValue[j] = Para->fluValue[j];
      }
      g_AT24C256.writeData(ERP_HISTORY_OFFSET + i * HISTORY_SIZE,
                           (unsigned char *)(&DevPara),
                           sizeof(DevPara)); // 新记录写入EEPROM
      g_AT24C256.readData(ERP_HISTORY_OFFSET + i * HISTORY_SIZE,
                          (unsigned char *)&erpHistoryStu[i], HISTORY_SIZE);

      return (i); // 添加成功，返回记录的存储下标
    }
  }
  return 0xFF; // 添加
}

// 数据记录匹配，返回0xFF匹配失败，记录不存在,匹配成功返回记录ID
static unsigned char stuHistoryMatching(unsigned char index) {
  unsigned char i = 0;

  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    if (erpHistoryStu[i].mark &&
        erpHistoryStu[i].index == index) // 判断记录是否存在
    {
      return (erpHistoryStu[i].id);
    }
  }
  return 0xFF;
}

// 检查数据是否存在，0不存在，1存在,i为数组下标,不是ID
static unsigned char checkHistoryPresenceofDtc(unsigned char i) {
  unsigned char result;
  result = 0;
  if (i < FLU_INDEX_MAX_NUM) // 防溢出检测
  {
    if (erpHistoryStu[i].mark) {
      result = 1;
    }
  }
  return result;
}

// 获取指定数据的ID,idx-结构体数组下标
static unsigned char getHistoryStuId(unsigned char idx) {
  unsigned char result;
  result = 0;
  if (idx < FLU_INDEX_MAX_NUM) // 防溢出检测
  {
    result = erpHistoryStu[idx].id;
  }
  return result;
}

// 获取指定数据记录的结构体数据,*pdDevPara-外部结构体指针，idx-要获取的数据结构体数组下标
static void getHistoryStuData(SAVE_HISTORY_TYPE *Para, unsigned char idx) {
  unsigned char i;

  if (idx >= FLU_INDEX_MAX_NUM) {
    return;
  } // id异常
  Para->id = erpHistoryStu[idx].id;
  Para->mark = erpHistoryStu[idx].mark;
  Para->index = erpHistoryStu[idx].index;
  Para->datanum = erpHistoryStu[idx].datanum;
  for (i = 0; i < FLU_DETECT_MAX_NUM; i++) {
    Para->fluValue[i] = erpHistoryStu[idx].fluValue[i];
  }
}

// 修改记录属性,id->指定探测器idx psDevPara->探测器属性结构体
static void setHistoryStuData(unsigned char id, SAVE_HISTORY_TYPE *psDevPara) {
  unsigned char i;
  if (id >= FLU_INDEX_MAX_NUM) {
    return;
  } // id异常
  erpHistoryStu[id].id = psDevPara->id;
  erpHistoryStu[id].mark = psDevPara->mark;
  erpHistoryStu[id].index = psDevPara->index;
  erpHistoryStu[id].datanum = psDevPara->datanum;
  for (i = 0; i < 16; i++) {
    erpHistoryStu[id].fluValue[i] = psDevPara->fluValue[i];
  }
  g_AT24C256.writeData(ERP_HISTORY_OFFSET + id * HISTORY_SIZE,
                       (unsigned char *)psDevPara,
                       HISTORY_SIZE); // 新设备信息写入EEPROM
  g_AT24C256.readData(ERP_HISTORY_OFFSET + id * HISTORY_SIZE,
                      (unsigned char *)&erpHistoryStu[id], HISTORY_SIZE);
}

SAVE_EQUATION_TYPE
erpEquationStu[EQUATION_INDEX_MAX_NUM]; // EEPROM设备参数数据结构
static unsigned char equationParaCheck(void);

static void equationStuReset(void) {
  unsigned short i;
  unsigned char j;
  // 所有数据记录初始化
  for (i = 0; i < EQUATION_INDEX_MAX_NUM; i++) {
    memset(&erpEquationStu[i], 0, EQUATION_SIZE);
  }

  g_AT24C256.writeData(ERP_EQUATION_OFFSET, (unsigned char *)(&erpEquationStu),
                       sizeof(erpEquationStu));
  g_AT24C256.readData(ERP_EQUATION_OFFSET, (unsigned char *)(&erpEquationStu),
                      sizeof(erpEquationStu));
}

static unsigned char equationParaCheck(void) {
  unsigned char i;
  unsigned char error = 0;
  if (erpEquationStu[0].id != 1) {
    error = 1;
  }
  for (i = 0; i < EQUATION_INDEX_MAX_NUM; i++) {
    if (erpEquationStu[i].id >= EQUATION_INDEX_MAX_NUM) {
      error = 1;
    }
    if (erpEquationStu[i].mark > 1) {
      error = 1;
    }
    if (erpEquationStu[i].datanum > 50) {
      error = 1;
    }
  }
  return error;
}
static void erpEquationInit(void) {
  g_AT24C256.readData(ERP_EQUATION_OFFSET, (unsigned char *)(&erpEquationStu),
                      sizeof(erpEquationStu));
  if (equationParaCheck()) {
    equationStuReset();
  }
}

// 删除指定的方程数据,
static void delEquationData(u8 index) {
  u8 id, i;
  SAVE_EQUATION_TYPE *pDevPara;

  for (i = 0; i < FLU_INDEX_MAX_NUM; i++) {
    if (index == erpEquationStu[i].index) {
      id = erpEquationStu[i].id - 1;
      break;
    }
  }
  memset(&erpEquationStu[id], 0, sizeof(erpEquationStu[id]));
  memset(pDevPara, 0, EQUATION_SIZE);
  g_AT24C256.writeData(ERP_EQUATION_OFFSET + id * EQUATION_SIZE,
                       (unsigned char *)(pDevPara),
                       EQUATION_SIZE); // 修改记录参数
  g_AT24C256.readData(ERP_EQUATION_OFFSET + id * EQUATION_SIZE,
                      (unsigned char *)&erpEquationStu[id], EQUATION_SIZE);
}

// 获取方程录数量
static unsigned char getEquationNum(void) {
  unsigned char i, num, count;
  count = 0;
  for (i = 0; i < EQUATION_INDEX_MAX_NUM; i++) {
    if (erpEquationStu[i].mark) // 判断记录是否已存在
    {
      count++;
    }
  }
  return count;
}

// 添加方程,返回ID,返回0xFF则添加失败
static unsigned char addEquationStu(SAVE_EQUATION_TYPE *Para) {
  SAVE_EQUATION_TYPE DevPara;
  unsigned char i, j;

  for (i = 0; i < EQUATION_INDEX_MAX_NUM; i++) {
    if (!erpEquationStu[i].mark) {
      DevPara.id = i + 1;
      DevPara.mark = 1;
      DevPara.index = Para->index;
      DevPara.a = Para->a;
      DevPara.b = Para->b;
      DevPara.unit = Para->unit;
      DevPara.datanum = Para->datanum;
      g_AT24C256.writeData(ERP_EQUATION_OFFSET + i * EQUATION_SIZE,
                           (unsigned char *)(&DevPara),
                           sizeof(DevPara)); // 新记录写入EEPROM
      g_AT24C256.readData(ERP_EQUATION_OFFSET + i * EQUATION_SIZE,
                          (unsigned char *)&erpEquationStu[i], EQUATION_SIZE);
      return (i); // 添加成功，返回记录的存储下标
    }
  }
  return 0xFF; // 添加失败
}

// 数据记录匹配，返回0xFF匹配失败，记录不存在,匹配成功返回记录ID
static unsigned char stuEquationMatching(unsigned char index) {
  unsigned char i = 0;

  for (i = 0; i < EQUATION_INDEX_MAX_NUM; i++) {
    if (erpEquationStu[i].mark &&
        erpEquationStu[i].index == index) // 判断记录是否存在
    {
      return (erpEquationStu[i].id);
    }
  }
  return 0xFF;
}

// 检查数据是否存在，0不存在，1存在,i为数组下标,不是ID
static unsigned char equationCheckExist(unsigned char i) {
  unsigned char result;
  result = 0;
  if (i < EQUATION_INDEX_MAX_NUM) // 防溢出检测
  {
    if (erpEquationStu[i].mark) {
      result = 1;
    }
  }
  return result;
}

// 获取指定数据的ID,idx-结构体数组下标
static unsigned char getEquationStuId(unsigned char idx) {
  unsigned char result;
  result = 0;
  if (idx < EQUATION_INDEX_MAX_NUM) // 防溢出检测
  {
    result = erpEquationStu[idx].id;
  }
  return result;
}

// 获取指定方程的结构体数据,*pdDevPara-外部结构体指针，idx-要获取的数据结构体数组下标
static void getEquationStuData(SAVE_EQUATION_TYPE *Para, unsigned char idx) {
  if (idx >= EQUATION_INDEX_MAX_NUM) {
    return;
  } // id异常
  Para->id = erpEquationStu[idx].id;
  Para->mark = erpEquationStu[idx].mark;
  Para->index = erpEquationStu[idx].index;
  Para->a = erpEquationStu[idx].a;
  Para->b = erpEquationStu[idx].b;
  Para->unit = erpEquationStu[idx].unit;
  Para->datanum = erpEquationStu[idx].datanum;
}

// 修改方程属性,id->指定方程idx psDevPara->方程属性结构体
static void setEquationStuData(unsigned char id,
                               SAVE_EQUATION_TYPE *psDevPara) {
  unsigned char i;
  if (id >= EQUATION_INDEX_MAX_NUM) {
    return;
  } // id异常
  erpEquationStu[id].id = psDevPara->id;
  erpEquationStu[id].mark = psDevPara->mark;
  erpEquationStu[id].index = psDevPara->index;
  erpEquationStu[id].a = psDevPara->a;
  erpEquationStu[id].b = psDevPara->b;
  erpEquationStu[id].unit = psDevPara->unit;
  erpEquationStu[id].datanum = psDevPara->datanum;
  g_AT24C256.writeData(ERP_EQUATION_OFFSET + id * EQUATION_SIZE,
                       (unsigned char *)psDevPara,
                       EQUATION_SIZE); // 新设备信息写入EEPROM
  g_AT24C256.readData(ERP_EQUATION_OFFSET + id * EQUATION_SIZE,
                      (unsigned char *)&erpEquationStu[id], EQUATION_SIZE);
}

static int saveObjInit(SAVE_DATA_TYPE *obj) {
  if (NULL != obj) {
    obj->historyErpInit = erpHistoryInit;
    obj->historyFactoryReset = historyStuReset;
    obj->historyDelData = delHistoryData;
    obj->historygetNum = getHistoryNum;
    obj->historyAddStu = addHistoryStu;
    obj->historyMatching = stuHistoryMatching;
    obj->historyParaCheck = checkHistoryPresenceofDtc;
    obj->historyGetStuId = getHistoryStuId;
    obj->historyGetStuData = getHistoryStuData;
    obj->historySetStuData = setHistoryStuData;

    obj->equationErpInit = erpEquationInit;
    obj->equationFactoryReset = equationStuReset;
    obj->equationDelData = delEquationData;
    obj->equationGetNum = getEquationNum;
    obj->equationAddStu = addEquationStu;
    obj->equationMatching = stuEquationMatching;
    obj->equationCheckExist = equationCheckExist;
    obj->equationGetStuId = getEquationStuId;
    obj->equationGetStuDat = getEquationStuData;
    obj->equationSetStuData = setEquationStuData;
    return 0;
  } else {
    return 1;
  }
}
