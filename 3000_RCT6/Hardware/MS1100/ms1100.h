#ifndef __MS1100_H_
#define __MS1100_H_

#include "main.h"



// /* IIC所有操作函数 */
// void iic_init(void);            /* 初始化IIC的IO口 */
// void iic_start(void);           /* 发送IIC开始信号 */
// void iic_stop(void);            /* 发送IIC停止信号 */
// void iic_ack(void);             /* IIC发送ACK信号 */
// void iic_nack(void);            /* IIC不发送ACK信号 */
// uint8_t iic_wait_ack(void);     /* IIC等待ACK信号 */
// void iic_send_byte(uint8_t txd);/* IIC发送一个字节 */
// uint8_t iic_read_byte(unsigned char ack);/* IIC读取一个字节 */


// u16 MS1100_ReadOneByte(void);				//指定地址读取一个字节
// void MS1100_WriteOneByte(u8 DataToWrite);		//指定地址写入一个字节
// void MS1100_Init(void); //初始化IIC

#define u16 unsigned short
#define u8 unsigned char

typedef struct 
{
    void (*init)(void);
    unsigned short (*readValue)(void);
    unsigned short (*readValueTest)(void);
    void (*writeReg)(u8 data);

}MS1100_STU_TYPE;
int MS1100Register(MS1100_STU_TYPE* obj);




#endif
