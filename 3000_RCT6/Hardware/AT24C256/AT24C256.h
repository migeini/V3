#ifndef _AT24C256_H_
#define _AT24C256_H_

#include "main.h"
#include "sys.h"



#define AT24C01     127
#define AT24C02     255
#define AT24C04     511
#define AT24C08     1023
#define AT24C16     2047
#define AT24C32     4095
#define AT24C64     8191
#define AT24C128    16383
#define AT24C256    32767

/* EEPROM类型定义 所以定义EE_TYPE为AT24C02 */
#define EE_TYPE                 AT24C256
#define ERP_PAGE_NUM            512
#define ERP_PAGE_BYTE_NUM       64
// void at24cxx_init(void);        /* 初始化IIC */
// uint8_t at24cxx_check(void);    /* 检查器件 */
// uint8_t at24cxx_read_one_byte(uint16_t addr);                       /* 指定地址读取一个字节 */
// void at24cxx_write_one_byte(uint16_t addr,uint8_t data);            /* 指定地址写入一个字节 */
// void at24cxx_write(uint16_t addr, uint8_t *pbuf, uint16_t datalen); /* 从指定地址开始写入指定长度的数据 */
// void at24cxx_read(uint16_t addr, uint8_t *pbuf, uint16_t datalen);  /* 从指定地址开始读出指定长度的数据 */

/*  AT24C256	256kBit	32kByte	512Page	 1页64字节 
*   前2k空间留给设备信息使用,固件号,版本号等
*   2-10k空间留给荧光数据历史记录保存,一条数据101byte,共20条.2k左右
*   10-15k空间留给方程参数保存,一条数据112byte,共20条,2k左右
*   剩余空间保留
*/
#define ERP_DEVICE_OFFSET       0 
#define ERP_DEVICE_SIZE         2048
#define ERP_HISTORY_OFFSET      2048
#define ERP_HISTORY_SIZE        8192
#define ERP_EQUATION_OFFSET     10240
#define ERP_EQUATION_SIZE       5120




typedef struct 
{
    void (*init)(void);    
    uint8_t(*check)(void); 
    uint8_t (*readOneByte)(uint16_t addr); 
    void (*writeOneByte)(uint16_t addr,uint8_t data);
    void (*writeData)(uint16_t addr, uint8_t *pbuf, uint16_t datalen);
    void (*readData)(uint16_t addr, uint8_t *pbuf, uint16_t datalen); 
}EEPROM_STU_TYPE;

int eepromRegister(EEPROM_STU_TYPE* obj);








#endif
