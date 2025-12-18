/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include <main.h>
#include <usart.h>
#include <gpio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <sys.h>
#include <App.h>
/* --diag_suppress=1,177,550 消除newline/函数定义未使用/变量定义未使用*/




/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void fluLedConrtolCallback(int state)               //控制流光LED状态的回调函数
{
    if(state){
        HAL_GPIO_WritePin(FLU_LED_GPIO_Port,FLU_LED_Pin,GPIO_PIN_SET);          //当参数state为真时，通过调用HAL_GPIO_WritePin函数将流光LED引脚设置为高电平（GPIO_PIN_SET）
    }else{
        HAL_GPIO_WritePin(FLU_LED_GPIO_Port,FLU_LED_Pin,GPIO_PIN_RESET);        //当参数state为假时，将流光LED引脚设置为低电平（GPIO_PIN_RESET）
    }
}
void usartRecvInterruptEnable(int i)	              //使能USART接收中断
{
	if(0 == i)                                        //根据参数 `i` 的不同取值，分别对USART1、USART2、USART3的接收中断进行使能
		                                                //当 `i` 为0时，同时使能USART1、USART2和USART3的接收中断
	{
        __HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);
        __HAL_UART_ENABLE_IT(&huart2,UART_IT_RXNE);
        __HAL_UART_ENABLE_IT(&huart3,UART_IT_RXNE);
	}
	else if(i == 1)                                   //当 `i` 为1、2、3时，分别使能对应USART的接收中断   
	{
		__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);
	}
	else if(i == 2)
	{
		__HAL_UART_ENABLE_IT(&huart2,UART_IT_RXNE);
	}
	else if(i == 3)
	{
		__HAL_UART_ENABLE_IT(&huart3,UART_IT_RXNE);
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();                //初始化硬件抽象层（HAL），对所有外设进行复位，初始化闪存接口和系统滴答定时器

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();       //配置系统时钟

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();                //分别初始化配置了的GPIO端口、USART1和USART2串口
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  //MX_USART3_UART_Init();//wifi 预留
  /* USER CODE BEGIN 2 */
    if(0 == fluLedCallbackRegister(fluLedConrtolCallback)){                   //函数注册了流光LED控制回调函数
			                                                                        //如果注册成功，输出调试信息；如果失败，输出错误信息。
        debugInfo("flu led control func callback resigter success");
    }else{
        debugError("flu led control func callback resigter failed");
    }
		
    usartRecvInterruptEnable(1);//开启串口1接收中断,-->屏幕
    usartRecvInterruptEnable(2);//开启串口2接收中断,-->type-c
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
              
		
  while (1)
  {
    /* USER CODE END WHILE */

    appTask();
		
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
