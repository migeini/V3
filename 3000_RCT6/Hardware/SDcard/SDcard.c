#include "SDcard.h"

#define SPI1_SCK_GPIO_PORT              GPIOA
#define SPI1_SCK_GPIO_PIN               GPIO_PIN_5
#define SPI1_SCK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define SPI1_MISO_GPIO_PORT             GPIOA
#define SPI1_MISO_GPIO_PIN              GPIO_PIN_6
#define SPI1_MISO_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define SPI1_MOSI_GPIO_PORT             GPIOA
#define SPI1_MOSI_GPIO_PIN              GPIO_PIN_7
#define SPI1_MOSI_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define SPI1_SPI                        SPI1
#define SPI1_SPI_CLK_ENABLE()           do{ __HAL_RCC_SPI1_CLK_ENABLE(); }while(0) 
#define SPI_SPEED_2         0
#define SPI_SPEED_4         1
#define SPI_SPEED_8         2
#define SPI_SPEED_16        3
#define SPI_SPEED_32        4
#define SPI_SPEED_64        5
#define SPI_SPEED_128       6
#define SPI_SPEED_256       7


static void spi1_init(void);
static void spi1_set_speed(uint8_t speed);
static uint8_t spi1_read_write_byte(uint8_t txdata);

SPI_HandleTypeDef g_spi1_handler; /* SPI1句柄 */

/**
 * @brief       SPI初始化代码
 *   @note      主机模式,8位数据,禁止硬件片选
 * @param       无
 * @retval      无
 */
void spi1_init(void)
{
    SPI1_SPI_CLK_ENABLE(); /* SPI1时钟使能 */

    g_spi1_handler.Instance = SPI1_SPI;                                /* SPI1 */
    g_spi1_handler.Init.Mode = SPI_MODE_MASTER;                        /* 设置SPI工作模式，设置为主模式 */
    g_spi1_handler.Init.Direction = SPI_DIRECTION_2LINES;              /* 设置SPI单向或者双向的数据模式:SPI设置为双线模式 */
    g_spi1_handler.Init.DataSize = SPI_DATASIZE_8BIT;                  /* 设置SPI的数据大小:SPI发送接收8位帧结构 */
    g_spi1_handler.Init.CLKPolarity = SPI_POLARITY_HIGH;               /* 串行同步时钟的空闲状态为高电平 */
    g_spi1_handler.Init.CLKPhase = SPI_PHASE_2EDGE;                    /* 串行同步时钟的第二个跳变沿（上升或下降）数据被采样 */
    g_spi1_handler.Init.NSS = SPI_NSS_SOFT;                            /* NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制 */
    g_spi1_handler.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; /* 定义波特率预分频的值:波特率预分频值为256 */
    g_spi1_handler.Init.FirstBit = SPI_FIRSTBIT_MSB;                   /* 指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始 */
    g_spi1_handler.Init.TIMode = SPI_TIMODE_DISABLE;                   /* 关闭TI模式 */
    g_spi1_handler.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;   /* 关闭硬件CRC校验 */
    g_spi1_handler.Init.CRCPolynomial = 7;                             /* CRC值计算的多项式 */
    HAL_SPI_Init(&g_spi1_handler);                                     /* 初始化 */

    __HAL_SPI_ENABLE(&g_spi1_handler); /* 使能SPI1 */

    spi1_read_write_byte(0Xff); /* 启动传输, 实际上就是产生8个时钟脉冲, 达到清空DR的作用, 非必需 */
}

/**
 * @brief       SPI底层驱动，时钟使能，引脚配置
 *   @note      此函数会被HAL_SPI_Init()调用
 * @param       hspi:SPI句柄
 * @retval      无
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    if (hspi->Instance == SPI1_SPI)
    {
        SPI1_SCK_GPIO_CLK_ENABLE();  /* SPI1_SCK脚时钟使能 */
        SPI1_MISO_GPIO_CLK_ENABLE(); /* SPI1_MISO脚时钟使能 */
        SPI1_MOSI_GPIO_CLK_ENABLE(); /* SPI1_MOSI脚时钟使能 */

        /* SCK引脚模式设置(复用输出) */
        gpio_init_struct.Pin = SPI1_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI1_SCK_GPIO_PORT, &gpio_init_struct);

        /* MISO引脚模式设置(复用输出) */
        gpio_init_struct.Pin = SPI1_MISO_GPIO_PIN;
        HAL_GPIO_Init(SPI1_MISO_GPIO_PORT, &gpio_init_struct);

        /* MOSI引脚模式设置(复用输出) */
        gpio_init_struct.Pin = SPI1_MOSI_GPIO_PIN;
        HAL_GPIO_Init(SPI1_MOSI_GPIO_PORT, &gpio_init_struct);
    }
}

/**
 * @brief       SPI1速度设置函数
 *   @note      SPI1时钟选择来自APB2, 即PCLK2, 为72Mhz
 *              SPI速度 = PCLK2 / 2^(speed + 1)
 * @param       speed   : SPI1时钟分频系数
                        取值为SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_2 256
 * @retval      无
 */
void spi1_set_speed(uint8_t speed)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed)); /* 判断有效性 */
    __HAL_SPI_DISABLE(&g_spi1_handler);             /* 关闭SPI */
    g_spi1_handler.Instance->CR1 &= 0XFFC7;         /* 位3-5清零，用来设置波特率 */
    g_spi1_handler.Instance->CR1 |= speed << 3;     /* 设置SPI速度 */
    __HAL_SPI_ENABLE(&g_spi1_handler);              /* 使能SPI */
}

/**
 * @brief       SPI1读写一个字节数据
 * @param       txdata  : 要发送的数据(1字节)
 * @retval      接收到的数据(1字节)
 */
uint8_t spi1_read_write_byte(uint8_t txdata)
{
    uint8_t rxdata;
    HAL_SPI_TransmitReceive(&g_spi1_handler, &txdata, &rxdata, 1, 1000);
    return rxdata; /* 返回收到的数据 */
}

uint8_t  sd_type = 0; /* SD卡的类型 */

static void sd_spi_init(void);                                      /* SD SPI 硬件初始化 */
static void sd_deselect(void);                                      /* SD卡取消选中 */
static uint8_t sd_select(void);                                     /* SD卡 选中 */
static uint8_t sd_wait_ready(void);                                 /* 等待SD卡准备好 */
static uint8_t sd_get_response(uint8_t response);                   /* 等待SD卡回应 */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg);              /* SD卡发送命令 */
static uint8_t sd_send_block(uint8_t *buf, uint8_t cmd);            /* SD卡发送一个数据块 */
static uint8_t sd_receive_data(uint8_t *buf, uint16_t len);         /* SD卡接收一次数据 */
static uint8_t sd_init(void);                                              /* SD卡初始化 */
static uint32_t sd_get_sector_count(void);                                 /* 获取SD卡的总扇区数(扇区数) */
static uint8_t sd_get_status(void);                                        /* 获取SD卡状态 */
static uint8_t sd_get_cid(uint8_t *cid_data);                              /* 获取SD卡的CID信息 */
static uint8_t sd_get_csd(uint8_t *csd_data);                              /* 获取SD卡的CSD信息 */
static uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);  /* 读SD卡(fatfs/usb调用) */
static uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt); /* 写SD卡(fatfs/usb调用) */


/**
 * @brief       SD卡 SPI硬件层初始化
 *   @note      该函数会禁止SPI1上面的其他外设, 通过将其他外设的
 *              CS脚拉高, 防止其他外设影响SD卡的正常通信
 * @param       无
 * @retval      无
 */
static void sd_spi_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    SD_CS_GPIO_CLK_ENABLE();                            /* SD CS脚时钟使能 */
    gpio_init_struct.Pin = SD_CS_GPIO_PIN;              /* SD CS脚 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;        /* 推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
    HAL_GPIO_Init(SD_CS_GPIO_PORT, &gpio_init_struct);  /* SD CS引脚模式设置(推挽输出) */

    /* 设置硬件上与SD卡相关联的控制引脚输出 */
    /* 禁止其他外设(NRF/25Q64)对SD卡产生影响 */
    __HAL_RCC_GPIOA_CLK_ENABLE();                       /* PA时钟使能 */
    gpio_init_struct.Pin = GPIO_PIN_2 | GPIO_PIN_4;     /* PA2和PA4引脚 */
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;        /* 推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);            /* PA2和PA4引脚模式设置(推挽输出) */
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); /* PA2 输出1 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); /* PA4 输出1 */

    spi1_init();    /* 初始化SPI1 */
    SD_CS(1);       /* SD卡不选中 */
}

/**
 * @brief       SD卡 取消选择, 释放 SPI总线
 * @param       无
 * @retval      无
 */
static void sd_deselect(void)
{
    SD_CS(1);                       /* 取消SD卡片选 */
    sd_spi_read_write_byte(0xff);   /* 提供额外的8个时钟 */
}

/**
 * @brief       SD卡 选中, 并等待卡准备OK
 * @param       无
 * @retval      选中结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
static uint8_t sd_select(void)
{
    SD_CS(0);

    if (sd_wait_ready() == 0)
    {
        return SD_OK;   /* 等待成功 */
    }

    sd_deselect();
    return SD_ERROR;    /* 等待失败 */
}

/**
 * @brief       等待卡准备好
 * @param       无
 * @retval      等待结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
static uint8_t sd_wait_ready(void)
{
    uint32_t t = 0;

    do
    {
        if (sd_spi_read_write_byte(0XFF) == 0XFF)
        {
            return SD_OK;   /* OK */
        }

        t++;
    } while (t < 0XFFFFFF); /* 等待 */

    return SD_ERROR;
}

/**
 * @brief       等待SD卡回应
 * @param       response : 期待得到的回应值
 * @retval      等待结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
static uint8_t sd_get_response(uint8_t response)
{
    uint16_t count = 0xFFFF;    /* 等待次数 */

    while ((sd_spi_read_write_byte(0XFF) != response) && count)
    {
        count--;    /* 等待得到准确的回应 */
    }

    if (count == 0) /* 等待超时 */
    {
        return SD_ERROR;
    }

    return SD_OK;   /* 正确回应 */
}

/**
 * @brief       从SD卡读取一次数据
 *   @note      读取长度不限, 由len指定
 *              不过一般为512字节
 * @param       buf      : 数据缓存区
 * @param       len      : 要读取的数据长度
 * @retval      读取结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
static uint8_t sd_receive_data(uint8_t *buf, uint16_t len)
{
    if (sd_get_response(0xFE))   /* 等待SD卡发回数据起始令牌0xFE */
    {
        return SD_ERROR;
    }

    while (len--)   /* 开始接收数据 */
    {
        *buf = sd_spi_read_write_byte(0xFF);
        buf++;
    }

    /* 下面是2个伪CRC（dummy CRC） */
    sd_spi_read_write_byte(0xFF);
    sd_spi_read_write_byte(0xFF);

    return SD_OK;   /* 读取成功 */
}

/**
 * @brief       向SD卡写入一个数据包
 *   @note      写入长度固定为512字节!!
 * @param       buf      : 数据缓存区
 * @param       cmd      : 指令
* @retval      发送结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
static uint8_t sd_send_block(uint8_t *buf, uint8_t cmd)
{
    uint16_t t;

    if (sd_wait_ready())            /* 等待ready */
    {
        return SD_ERROR;
    }

    sd_spi_read_write_byte(cmd);    /* 发送 CMD */

    if (cmd != 0XFD)                /* 不是结束指令 */
    {
        for (t = 0; t < 512; t++)
        {
            sd_spi_read_write_byte(buf[t]); /* 发送数据 */
        }

        sd_spi_read_write_byte(0xFF);       /* 忽略crc */
        sd_spi_read_write_byte(0xFF);

        t = sd_spi_read_write_byte(0xFF);   /* 接收响应 */

        if ((t & 0x1F) != 0x05)             /* 数据包没有被接收 */
        {
            return SD_ERROR;
        }
    }

    return SD_OK;   /* 写入成功 */
}

/**
 * @brief       向SD卡发送一个命令
 *   @note      不同命令的CRC值在该函数内部自动确认
 * @param       cmd      : 要发送的命令
 *   @note                 最高位为1 表示ACMD(应用命令)
 *                         最高位为0 表示CMD(普通命令)
 * @param       arg      : 命令的参数
 * @retval      SD卡返回的命令响应
 */
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t res;
    uint8_t retry = 0;
    uint8_t crc = 0X01; /* 默认 CRC = 忽略CRC + 停止 */

    if (cmd & 0x80)     /* ACMD发送前, 需要先发送一个 CMD55 命令 */
    {
        cmd &= 0x7F;                    /* 清除最高位, 获取ACMD命令 */
        res = sd_send_cmd(CMD55, 0);    /* 发送CMD55 */

        if (res > 1)
        {
            return res;
        }
    }

    if (cmd != CMD12)   /* 当 cmd 不等于 多块读结束命令时(CMD12), 等待卡选中成功 */
    {
        sd_deselect();  /* 取消上次片选 */

        if (sd_select())
        {
            return 0xFF;/* 选中失败 */
        }
    }

    /* 发送命令包 */
    sd_spi_read_write_byte(cmd | 0x40); /* 起始 + 命令索引号 */
    sd_spi_read_write_byte(arg >> 24);  /* 参数[31 : 24] */
    sd_spi_read_write_byte(arg >> 16);  /* 参数[23 : 16] */
    sd_spi_read_write_byte(arg >> 8);   /* 参数[15 : 8] */
    sd_spi_read_write_byte(arg);        /* 参数[7 : 0] */

    if (cmd == CMD0) crc = 0X95;        /* CMD0 的CRC值固定为 0X95 */

    if (cmd == CMD8) crc = 0X87;        /* CMD8 的CRC值固定为 0X87 */

    sd_spi_read_write_byte(crc);

    if (cmd == CMD12)                   /* cmd 等于 多块读结束命令(CMD12)时 */
    {
        sd_spi_read_write_byte(0xff);   /* CMD12 跳过一个字节 */
    }


    retry = 10; /* 重试次数 */

    do          /* 等待响应，或超时退出 */
    {
        res = sd_spi_read_write_byte(0xFF);
    } while ((res & 0X80) && retry--);

    return res; /* 返回状态值 */
}

/**
 * @brief       获取SD卡的状态
 * @param       无
 * @retval      读取结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
uint8_t sd_get_status(void)
{
    uint8_t res;
    uint8_t retry = 20; /* 发送ACMD经常失败, 多尝试几次 */

    do
    {
        res = sd_send_cmd(ACMD13, 0);   /* 发ACMD13命令，获取状态 */
    }while(res && retry--);

    sd_deselect();      /* 取消片选 */

    return res;
}

/**
 * @brief       获取SD卡的CID信息, 包括制造商信息等
 * @param       cid_data : 存放CID的内存缓冲区(至少16字节)
 * @retval      读取结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
uint8_t sd_get_cid(uint8_t *cid_data)
{
    uint8_t res;

    res = sd_send_cmd(CMD10, 0);            /* 发CMD10命令，读CID */

    if (res == 0x00)
    {
        res = sd_receive_data(cid_data, 16);/* 接收16个字节的数据 */
    }

    sd_deselect();  /* 取消片选 */

    return res;
}

/**
 * @brief       获取SD卡的CSD信息，包括容量和速度信息
 * @param       csd_data : 存放CSD的内存缓冲区(至少16字节)
 * @retval      读取结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
uint8_t sd_get_csd(uint8_t *csd_data)
{
    uint8_t res;
    res = sd_send_cmd(CMD9, 0);             /* 发CMD9命令，读CSD */

    if (res == 0)
    {
        res = sd_receive_data(csd_data, 16);/* 接收16个字节的数据 */
    }

    sd_deselect();  /* 取消片选 */

    return res;
}

/**
 * @brief       获取SD卡的总扇区数(扇区数)
 *   @note      每扇区的字节数必为512, 如果不是512, 则初始化不能通过
 * @param       无
 * @retval      SD卡容量(扇区数), 换成字节数要 * 512
 */
uint32_t sd_get_sector_count(void)
{
    uint8_t csd[16];
    uint32_t capacity;
    uint8_t n;
    uint16_t csize;

    if (sd_get_csd(csd) != 0)       /* 取CSD信息，如果期间出错，返回0 */
    {
        return 0;                   /* 返回0表示获取容量失败 */
    }

    /* 如果为SDHC卡，按照下面方式计算 */
    if ((csd[0] & 0xC0) == 0x40)    /* V2.00的卡 */
    {
        csize = csd[9] + ((uint16_t)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1;
        capacity = (uint32_t)csize << 10;       /* 得到扇区数 */
    }
    else    /* V1.XX的卡 / MMC V3卡 */
    {
        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
        csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
        capacity = (uint32_t)csize << (n - 9);  /* 得到扇区数 */
    }

    return capacity;    /* 注意这里返回的是扇区数量, 不是实际容量的字节数, 换成字节数 得 * 512 */
}

/**
 * @brief       初始化SD卡
 * @param       无
 * @retval      初始化结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
uint8_t sd_init(void)
{
    uint8_t res;        /*  存放SD卡的返回值 */
    uint16_t retry;     /*  用来进行超时计数 */
    uint8_t ocr[4];
    uint16_t i;
    uint8_t cmd;

    sd_spi_init();      /* 初始化IO */
    sd_spi_speed_low(); /* 设置到低速模式 */

    for (i = 0; i < 10; i++)
    {
        sd_spi_read_write_byte(0XFF);       /* 发送最少74个脉冲 */
    }

    retry = 20;

    do
    {
        res = sd_send_cmd(CMD0, 0);         /* 进入IDLE状态 */
    } while ((res != 0X01) && retry--);

    sd_type = 0;        /* 默认无卡 */

    if (res == 0X01)
    {
        if (sd_send_cmd(CMD8, 0x1AA) == 1)   /* SD V2.0 */
        {
            for (i = 0; i < 4; i++)
            {
                ocr[i] = sd_spi_read_write_byte(0XFF);  /* 得到R7的32位响应 */
            }

            if (ocr[2] == 0X01 && ocr[3] == 0XAA)       /* 卡是否支持2.7~3.6V */
            {
                retry = 1000;

                do
                {
                    res = sd_send_cmd(ACMD41, 1UL << 30);   /* 发送ACMD41 */
                } while (res && retry--);

                if (retry && sd_send_cmd(CMD58, 0) == 0)    /* 鉴别SD2.0卡版本开始 */
                {
                    for (i = 0; i < 4; i++)
                    {
                        ocr[i] = sd_spi_read_write_byte(0XFF);  /* 得到OCR值 */
                    }

                    if (ocr[0] & 0x40)          /* 检查CCS */
                    {
                        sd_type = SD_TYPE_V2HC; /* V2.0 SDHC卡 */
                    }
                    else
                    {
                        sd_type = SD_TYPE_V2;   /* V2.0 卡 */
                    }
                }
            }
        }
        else     /* SD V1.x / MMC V3 */
        {
            res = sd_send_cmd(ACMD41, 0);   /* 发送ACMD41 */
            retry = 1000;

            if (res <= 1)
            {
                sd_type = SD_TYPE_V1;   /* SD V1卡 */
                cmd = ACMD41;           /* 命令等于 ACMD41 */
            }
            else     /* MMC卡不支持 ACMD41 识别 */
            {
                sd_type = SD_TYPE_MMC;  /* MMC V3 */
                cmd = CMD1;             /* 命令等于 CMD1 */
            }

            do
            {
                res = sd_send_cmd(cmd, 0);  /* 发送 ACMD41 / CMD1 */
            } while (res && retry--);       /* 等待退出IDLE模式 */

            if (retry == 0 || sd_send_cmd(CMD16, 512) != 0)
            {
                sd_type = SD_TYPE_ERR;      /* 错误的卡 */
            }
        }
    }

    sd_deselect();          /* 取消片选 */
    sd_spi_speed_high();    /* 高速模式 */

    if (sd_type)            /* 卡类型有效, 初始化完成 */
    {
        return SD_OK;
    }

    return res;
}

/**
 * @brief       读SD卡(fatfs/usb调用)
 * @param       pbuf  : 数据缓存区
 * @param       saddr : 扇区地址
 * @param       cnt   : 扇区个数
 * @retval      读取结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t res;
    long long lsaddr = saddr;

    if (sd_type != SD_TYPE_V2HC)
    {
        lsaddr <<= 9;   /* 转换为字节地址 */
    }

    if (cnt == 1)
    {
        res = sd_send_cmd(CMD17, lsaddr);       /* 读命令 */

        if (res == 0)   /* 指令发送成功 */
        {
            res = sd_receive_data(pbuf, 512);   /* 接收512个字节 */
        }
    }
    else
    {
        res = sd_send_cmd(CMD18, lsaddr);       /* 连续读命令 */

        do
        {
            res = sd_receive_data(pbuf, 512);   /* 接收512个字节 */
            pbuf += 512;
        } while (--cnt && res == 0);

        sd_send_cmd(CMD12, 0);  /* 发送停止命令 */
    }

    sd_deselect();  /* 取消片选 */
    return res;
}

/**
 * @brief       写SD卡(fatfs/usb调用)
 * @param       pbuf  : 数据缓存区
 * @param       saddr : 扇区地址
 * @param       cnt   : 扇区个数
 * @retval      写入结果
 *              SD_OK,      成功
 *              SD_ERROR,   失败
 */
uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t retry = 20; /* 发送ACMD经常失败, 多尝试几次 */
    uint8_t res;
    long long lsaddr = saddr;

    if (sd_type != SD_TYPE_V2HC)
    {
        lsaddr <<= 9;   /* 转换为字节地址 */
    }

    if (cnt == 1)
    {
        res = sd_send_cmd(CMD24, lsaddr);   /* 读命令 */

        if (res == 0)   /* 指令发送成功 */
        {
            res = sd_send_block(pbuf, 0xFE);/* 写512个字节 */
        }
    }
    else
    {
        if (sd_type != SD_TYPE_MMC)
        {
            do
            {
                sd_send_cmd(ACMD23, cnt);   /* 发送 ACMD23 指令 */
            }while(res && retry--);
        }

        res = sd_send_cmd(CMD25, lsaddr);   /* 连续读命令 */

        if (res == 0)
        {
            do
            {
                res = sd_send_block(pbuf, 0xFC);    /* 写入一个block, 512个字节 */
                pbuf += 512;
            } while (--cnt && res == 0);

            res = sd_send_block(0, 0xFD);   /* 写入一个block, 512个字节 */
        }
    }

    sd_deselect();/* 取消片选 */
    return res;
}




int sdcardRegister(SDCARD_STU_TYPE* obj)
{
    if(NULL != obj)
    {
        obj->init = sd_init;
        obj->getSectorCount = sd_get_sector_count;
        obj->getStatus = sd_get_status;
        obj->getCid = sd_get_cid;
        obj->getCsd = sd_get_csd;
        obj->readDisk = sd_read_disk;
        obj->writeDisk = sd_write_disk;
        return 0;
    }else{return 1;}
}

















