#include "stm32f0xx_hal.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"


#define ILI9341_TFTWIDTH  240
#define ILI9341_TFTHEIGHT 320

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0A
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR   0x30
#define ILI9341_MADCTL  0x36
#define ILI9341_PIXFMT  0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1
/*
#define ILI9341_PWCTR6  0xFC
*/

// Color definitions
#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
#define ILI9341_RED         0xF800      /* 255,   0,   0 */
#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
#define ILI9341_WHITE       0xFFFF      /* 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_PINK        0xF81F

void SystemClock_Config(void);
void Error_Handler(void);

void stled_on(void) {
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_SET);
}

void stled_off(void) {
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
}

void lcd_reset(void) {
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
}

void lcd_command(uint8_t n) {
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &n, 1, 0xFF);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void lcd_data(uint8_t n) {
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &n, 1, 0xFF);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void lcd_data_word(uint16_t n) {
    uint8_t *p = (uint8_t *) &n;
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, p+1, 1, 0xFF);
    HAL_SPI_Transmit(&hspi1, p, 1, 0xFF);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  uint8_t x, y;

  lcd_command(ILI9341_CASET); // Column addr set

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
  x = x0 >> 8;
  HAL_SPI_Transmit(&hspi1, &x, 1, 0xFF);
  x = x0;
  HAL_SPI_Transmit(&hspi1, &x, 1, 0xFF);
  x = x1 >> 8;
  HAL_SPI_Transmit(&hspi1, &x, 1, 0xFF);
  x = x1;
  HAL_SPI_Transmit(&hspi1, &x, 1, 0xFF);

  lcd_command(ILI9341_PASET); // Row addr set

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
  y = y0 >> 8;
  HAL_SPI_Transmit(&hspi1, &y, 1, 0xFF);
  y = y0;
  HAL_SPI_Transmit(&hspi1, &y, 1, 0xFF);
  y = y1 >> 8;
  HAL_SPI_Transmit(&hspi1, &y, 1, 0xFF);
  y = y1;
  HAL_SPI_Transmit(&hspi1, &y, 1, 0xFF);

  lcd_command(ILI9341_RAMWR); // write to RAM
}

void draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if ((x < 0) || (x >= ILI9341_TFTWIDTH) || (y < 0) || (y >= ILI9341_TFTHEIGHT)) return;

    set_addr_window(x, y, x + 1, y + 1);
    lcd_data_word(color);
}

void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if ((x >= ILI9341_TFTWIDTH) || (y >= ILI9341_TFTHEIGHT || h < 1 || w < 1)) return;
    if ((x + w - 1) >= ILI9341_TFTWIDTH)  w = ILI9341_TFTWIDTH  - x;
    if ((y + h - 1) >= ILI9341_TFTHEIGHT) h = ILI9341_TFTHEIGHT - y;
    if (w == 1 && h == 1) {
        draw_pixel(x, y, color);
        return;
    }

    set_addr_window(x, y, x + w - 1, y + h - 1);

    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    uint8_t hi = color >> 8, lo = color;
    for(y=h; y>0; y--) {
        for(x=w; x>0; x--) {
            HAL_SPI_Transmit(&hspi1, &hi, 1, 0xFF);
            HAL_SPI_Transmit(&hspi1, &lo, 1, 0xFF);
        }
    }
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}


void draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if ((x >= ILI9341_TFTWIDTH) || (y >= ILI9341_TFTHEIGHT || h < 1)) return;

    if ((y + h - 1) >= ILI9341_TFTHEIGHT)
        h = ILI9341_TFTHEIGHT - y;
    if (h < 2 ) {
        draw_pixel(x, y, color);
        return;
    }

    set_addr_window(x, y, x, y + h - 1);

    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

    uint8_t hi = color >> 8, lo = color;
    while (h--) {
        HAL_SPI_Transmit(&hspi1, &hi, 1, 0xFF);
        HAL_SPI_Transmit(&hspi1, &lo, 1, 0xFF);
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color) {

    if ((x >= ILI9341_TFTWIDTH) || (y >= ILI9341_TFTHEIGHT || w < 1)) return;
    if ((x + w - 1) >= ILI9341_TFTWIDTH)  w = ILI9341_TFTWIDTH - x;
    if (w < 2 ) {
        draw_pixel(x, y, color);
        return;
    }

    set_addr_window(x, y, x + w - 1, y);

    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

    uint8_t hi = color >> 8, lo = color;
    while (w--) {
        HAL_SPI_Transmit(&hspi1, &hi, 1, 0xFF);
        HAL_SPI_Transmit(&hspi1, &lo, 1, 0xFF);
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

int main(void) {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();

    lcd_reset();

    lcd_command(0xEF);
    lcd_data(0x03);
    lcd_data(0x80);
    lcd_data(0x02);

    lcd_command(0xCF);
    lcd_data(0x00);
    lcd_data(0XC1);
    lcd_data(0X30);

    lcd_command(0xED);
    lcd_data(0x64);
    lcd_data(0x03);
    lcd_data(0X12);
    lcd_data(0X81);

    lcd_command(0xE8);
    lcd_data(0x85);
    lcd_data(0x00);
    lcd_data(0x78);

    lcd_command(0xCB);
    lcd_data(0x39);
    lcd_data(0x2C);
    lcd_data(0x00);
    lcd_data(0x34);
    lcd_data(0x02);

    lcd_command(0xF7);
    lcd_data(0x20);

    lcd_command(0xEA);
    lcd_data(0x00);
    lcd_data(0x00);

    lcd_command(ILI9341_PWCTR1); // Power control
    lcd_data(0x23);   //VRH[5:0]

    // Power control
    lcd_command(ILI9341_PWCTR2);
    lcd_data(0x10); //SAP[2:0];BT[3:0]

    lcd_command(ILI9341_VMCTR1); // VCM control

    lcd_data(0x3e); //�Աȶȵ���
    lcd_data(0x28);

    lcd_command(ILI9341_VMCTR2); // VCM control2
    lcd_data(0x86);  //--

    lcd_command(ILI9341_MADCTL); // Memory Access Control
    lcd_data(0x48);

    lcd_command(ILI9341_PIXFMT);
    lcd_data(0x55);

    lcd_command(ILI9341_FRMCTR1);
    lcd_data(0x00);
    lcd_data(0x18);

    lcd_command(ILI9341_DFUNCTR); // Display Function Control
    lcd_data(0x08);
    lcd_data(0x82);
    lcd_data(0x27);

    lcd_command(0xF2); // Gamma Function Disable
    lcd_data(0x00);

    lcd_command(ILI9341_GAMMASET); // Gamma curve selected
    lcd_data(0x01);

    lcd_command(ILI9341_GMCTRP1); // Set Gamma
    lcd_data(0x0F);
    lcd_data(0x31);
    lcd_data(0x2B);
    lcd_data(0x0C);
    lcd_data(0x0E);
    lcd_data(0x08);
    lcd_data(0x4E);
    lcd_data(0xF1);
    lcd_data(0x37);
    lcd_data(0x07);
    lcd_data(0x10);
    lcd_data(0x03);
    lcd_data(0x0E);
    lcd_data(0x09);
    lcd_data(0x00);

    lcd_command(ILI9341_GMCTRN1); // Set Gamma
    lcd_data(0x00);
    lcd_data(0x0E);
    lcd_data(0x14);
    lcd_data(0x03);
    lcd_data(0x11);
    lcd_data(0x07);
    lcd_data(0x31);
    lcd_data(0xC1);
    lcd_data(0x48);
    lcd_data(0x08);
    lcd_data(0x0F);
    lcd_data(0x0C);
    lcd_data(0x31);
    lcd_data(0x36);
    lcd_data(0x0F);

    lcd_command(ILI9341_SLPOUT); // Exit Sleep
    /* HAL_Delay(120); */

    lcd_command(ILI9341_DISPON); // Display on
    HAL_Delay(100);

    fill_rect(0, 0, ILI9341_TFTWIDTH, ILI9341_TFTHEIGHT, 0);
    draw_hline(64, 100, ILI9341_TFTWIDTH-128, ILI9341_MAGENTA);
    draw_hline(64, 200, ILI9341_TFTWIDTH-128, ILI9341_MAGENTA);
    draw_vline(64, 100, 100, ILI9341_MAGENTA);
    draw_vline(ILI9341_TFTWIDTH - 64, 100, 100, ILI9341_MAGENTA);

    while (1) {
        stled_on();
        HAL_Delay(120);
        stled_off();
        HAL_Delay(120);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /* Initialize CPU, AHB and APB bus clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initialize CPU, AHB and APB bus clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }

    /* Configure the Systick interrupt time */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /* Configure the Systick */
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        HAL_IncTick();
    }
}

void Error_Handler(void) {
    while(1) {
    }
}
