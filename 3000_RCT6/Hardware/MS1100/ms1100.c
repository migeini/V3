#include <ms1100.h>

/******************************************************************************************/
/* 引脚 定义 */

#define IIC_SCL_GPIO_PORT MS1100_SCL_GPIO_Port
#define IIC_SCL_GPIO_PIN MS1100_SCL_Pin
#define IIC_SCL_GPIO_CLK_ENABLE()                                              \
  do {                                                                         \
    __HAL_RCC_GPIOB_CLK_ENABLE();                                              \
  } while (0)

#define IIC_SDA_GPIO_PORT MS1100_SDA_GPIO_Port
#define IIC_SDA_GPIO_PIN MS1100_SDA_Pin
#define IIC_SDA_GPIO_CLK_ENABLE()                                              \
  do {                                                                         \
    __HAL_RCC_GPIOB_CLK_ENABLE();                                              \
  } while (0)

/******************************************************************************************/

/* IO操作 */
#define IIC_SCL(x)                                                             \
  do {                                                                         \
    x ? HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET)   \
      : HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN,                 \
                          GPIO_PIN_RESET);                                     \
  } while (0) /* SCL */

#define IIC_SDA(x)                                                             \
  do {                                                                         \
    x ? HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET)   \
      : HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN,                 \
                          GPIO_PIN_RESET);                                     \
  } while (0) /* SDA */

#define IIC_READ_SDA                                                           \
  HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN) /* 读取SDA */

static void iic_init(void);
static void iic_delay(void);
static void iic_start(void);
static void iic_stop(void);
static uint8_t iic_wait_ack(void);
static void iic_ack(void);
static void iic_nack(void);
static void iic_send_byte(uint8_t data);
static uint8_t iic_read_byte(uint8_t ack);
static void MS1100_Init(void);
static u16 MS1100_ReadOneByte(void);
static void MS1100_WriteOneByte(u8 DataToWrite);

static void for_delay_us(uint32_t nus) {
  uint32_t Delay = nus * 72 / 4;
  do {
    __NOP();
  } while (Delay--);
}

/**
 * @brief       初始化IIC
 * @param       无
 * @retval      无
 */
static void iic_init(void) {
  GPIO_InitTypeDef gpio_init_struct;

  IIC_SCL_GPIO_CLK_ENABLE(); /* SCL引脚时钟使能 */
  IIC_SDA_GPIO_CLK_ENABLE(); /* SDA引脚时钟使能 */

  gpio_init_struct.Pin = MS1100_SCL_Pin;
  gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* 推挽输出 */
  gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
  HAL_GPIO_Init(MS1100_SCL_GPIO_Port, &gpio_init_struct); /* SCL */

  gpio_init_struct.Pin = MS1100_SDA_Pin;
  gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;            /* 开漏输出 */
  HAL_GPIO_Init(MS1100_SDA_GPIO_Port, &gpio_init_struct); /* SDA */
  /* SDA引脚模式设置,开漏输出,上拉, 这样就不用再设置IO方向了,
   * 开漏输出的时候(=1), 也可以读取外部信号的高低电平 */

  iic_stop(); /* 停止总线上所有设备 */
}

/**
 * @brief       IIC延时函数,用于控制IIC读写速度
 * @param       无
 * @retval      无
 */
static void iic_delay(void) {
  for_delay_us(2); /* 2us的延时, 读写速度在250Khz以内 */
}

/**
 * @brief       产生IIC起始信号
 * @param       无
 * @retval      无
 */
static void iic_start(void) {
  IIC_SDA(1);
  IIC_SCL(1);
  iic_delay();
  IIC_SDA(0); /* START信号: 当SCL为高时, SDA从高变成低, 表示起始信号 */
  iic_delay();
  IIC_SCL(0); /* 钳住I2C总线，准备发送或接收数据 */
  iic_delay();
}

/**
 * @brief       产生IIC停止信号
 * @param       无
 * @retval      无
 */
static void iic_stop(void) {
  IIC_SDA(0); /* STOP信号: 当SCL为高时, SDA从低变成高, 表示停止信号 */
  iic_delay();
  IIC_SCL(1);
  iic_delay();
  IIC_SDA(1); /* 发送I2C总线结束信号 */
  iic_delay();
}

/**
 * @brief       等待应答信号到来
 * @param       无
 * @retval      1，接收应答失败
 *              0，接收应答成功
 */
static uint8_t iic_wait_ack(void) {
  uint8_t waittime = 0;
  uint8_t rack = 0;

  IIC_SDA(1); /* 主机释放SDA线(此时外部器件可以拉低SDA线) */
  iic_delay();
  IIC_SCL(1); /* SCL=1, 此时从机可以返回ACK */
  iic_delay();

  while (IIC_READ_SDA) /* 等待应答 */
  {
    waittime++;

    if (waittime > 250) {
      iic_stop();
      rack = 1;
      break;
    }
  }

  IIC_SCL(0); /* SCL=0, 结束ACK检查 */
  iic_delay();
  return rack;
}

/**
 * @brief       产生ACK应答
 * @param       无
 * @retval      无
 */
static void iic_ack(void) {
  IIC_SDA(0); /* SCL 0 -> 1  时 SDA = 0,表示应答 */
  iic_delay();
  IIC_SCL(1); /* 产生一个时钟 */
  iic_delay();
  IIC_SCL(0);
  iic_delay();
  IIC_SDA(1); /* 主机释放SDA线 */
  iic_delay();
}

/**
 * @brief       不产生ACK应答
 * @param       无
 * @retval      无
 */
static void iic_nack(void) {
  IIC_SDA(1); /* SCL 0 -> 1  时 SDA = 1,表示不应答 */
  iic_delay();
  IIC_SCL(1); /* 产生一个时钟 */
  iic_delay();
  IIC_SCL(0);
  iic_delay();
}

/**
 * @brief       IIC发送一个字节
 * @param       data: 要发送的数据
 * @retval      无
 */
static void iic_send_byte(uint8_t data) {
  uint8_t t;

  for (t = 0; t < 8; t++) {
    IIC_SDA((data & 0x80) >> 7); /* 高位先发送 */
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SCL(0);
    data <<= 1; /* 左移1位,用于下一次发送 */
  }
  IIC_SDA(1); /* 发送完成, 主机释放SDA线 */
}

/**
 * @brief       IIC读取一个字节
 * @param       ack:  ack=1时，发送ack; ack=0时，发送nack
 * @retval      接收到的数据
 */
static uint8_t iic_read_byte(uint8_t ack) {
  uint8_t i, receive = 0;

  for (i = 0; i < 8; i++) /* 接收1个字节数据 */
  {
    receive <<= 1; /* 高位先输出,所以先收到的数据位要左移 */
    IIC_SCL(1);
    iic_delay();

    if (IIC_READ_SDA) {
      receive++;
    }

    IIC_SCL(0);
    iic_delay();
  }

  if (!ack) {
    iic_nack(); /* 发送nACK */
  } else {
    iic_ack(); /* 发送ACK */
  }

  return receive;
}

// 初始化IIC接口
static void MS1100_Init(void) {
  iic_init(); // IIC初始化
}

// 在MS1100中读出一个数据
// ReadAddr:开始读数的地址
// 返回值temp:读到的数据
static u16 MS1100_ReadOneByte(void) {
  u8 data_H, data_L;
  u16 temp = 0;
  iic_start();
  iic_send_byte(0X91); // 发送IIC设备地址
  iic_wait_ack();
  data_H = iic_read_byte(1); // 读取高8位
  data_L = iic_read_byte(1); // 读取低8位
  iic_stop();                // 产生一个停止条件
  temp = data_H << 8 | data_L;
  return temp;
}

// 在MS1100指定地址写入一个数据
// DataToWrite:要写入的数据
static void MS1100_WriteOneByte(u8 DataToWrite) {
  iic_start();
  iic_send_byte(0X90); // 发送IIC设备地址
  iic_wait_ack();
  iic_send_byte(DataToWrite); // 发送字节
  iic_wait_ack();
  iic_stop(); // 产生一个停止条件
}
static u16 MS1100_ReadTest(void) {
  static u16 data = 9;
  return ++data;
}

int MS1100Register(MS1100_STU_TYPE *obj) {
  if (NULL != obj) {
    obj->init = MS1100_Init;
    obj->readValue = MS1100_ReadOneByte;
    obj->readValueTest = MS1100_ReadTest;
    obj->writeReg = MS1100_WriteOneByte;
    return 0;
  } else {
    return 1;
  }
}
