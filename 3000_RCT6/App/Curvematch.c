#include "Curvematch.h"

/*********数据fifo长度*****/
#define LEN 100

/********数据单元***********/
struct _Data_Unit {
  float x;
  float y;
};

/*******数据fifo*****/
struct _Data {
  struct _Data_Unit data[LEN];
  int len;
};
struct _Data Data = {.len = 0};

/****
 * 函数名:void push(float x, float y)
 * 作用:压入数据
 *参数:x:测量数据x
 *     y:测量数据y
 */

void push(float x, float y) {
  int i = 0;

  if (Data.len < LEN) {
    Data.data[Data.len].x = x;
    Data.data[Data.len++].y = y;
    return;
  }

  // 数据超出范围,去掉第一个数据
  for (i = 0; i < LEN - 1; i++) {
    Data.data[i].x = Data.data[i + 1].x;
    Data.data[i].y = Data.data[i + 1].y;
  }
  Data.data[LEN - 1].x = x;
  Data.data[LEN - 1].y = y;
  Data.len = LEN;
}

/*********************************************************************
 *                           计算估值
 *拟合曲线y = a * x + b
 *参数:x:需要估值的数的x值
 *返回:估值y
 **********************************************************************/
#if 0
float calc(float x)
{
    int i = 0;
    float mean_x = 0;
    float mean_y = 0;
    float num1 = 0;
    float num2 = 0;
    float a = 0;
    float b = 0;
 
    //求x,y的均值
    for (i = 0; i < Data.len; i++)
    {
        mean_x += Data.data[i].x;
        mean_y += Data.data[i].y;
    }
    mean_x /= Data.len;
    mean_y /= Data.len;
 
    //printf("mean_x = %f,mean_y = %f\n", mean_x, mean_y);
 
    for (i = 0; i < Data.len; i++)
    {
        num1 += (Data.data[i].x - mean_x) * (Data.data[i].y - mean_y);
        num2 += (Data.data[i].x - mean_x) * (Data.data[i].x - mean_x);
    }
 
    b = num1 / num2;
    a = mean_y - b * mean_x;
 
    //printf("a = %f,b = %f\n", a, b);
    return (a + b * x);
}
#endif

/*********************************************************************
 *                           计算估值
 *拟合曲线y = a * x + b
 *参数:x:需要估值的数的x值
 *参数:y:需要估值的数的y值
 *参数:len:数据点个数
 **********************************************************************/
void curveMatching(float *a, float *b, float *x, float *y, unsigned char len) {
  int i = 0;
  float mean_x = 0;
  float mean_y = 0;
  float num1 = 0;
  float num2 = 0;

  for (i = 0; i < 10; i++) {
    push(x[i], y[i]);
  }
  for (i = 0; i < Data.len; i++) {
    mean_x += Data.data[i].x;
    mean_y += Data.data[i].y;
  }
  mean_x /= Data.len;
  mean_y /= Data.len;
  for (i = 0; i < Data.len; i++) {
    num1 += (Data.data[i].x - mean_x) * (Data.data[i].y - mean_y);
    num2 += (Data.data[i].x - mean_x) * (Data.data[i].x - mean_x);
  }
  *b = num1 / num2;
  *a = mean_y - (*b) * mean_x;
}
