#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim;
DMA_HandleTypeDef hdma;

#define RC5_TIMER TIM3
#define RC5_DMA_CHANNEL DMA1_Stream4
#define RC5_GPIO_PIN GPIO_PIN_6
#define RC5_GPIO_PORT GPIOB

void RC5_Generate(uint16_t command);
void RC5_Receive();
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma);

void RC5_Generate(uint16_t command) {
    // Konfiguracja timera do generowania sygnału
    htim.Instance = RC5_TIMER;
    htim.Init.Prescaler = 90 - 1; // Dostosowane do NUCLEO-F446RE (90MHz APB1 timer clock)
    htim.Init.Period = 888; // Okres dla 36kHz
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim);

    // Konfiguracja DMA do przesyłu danych do timera
    hdma.Instance = RC5_DMA_CHANNEL;
    hdma.Init.Channel = DMA_CHANNEL_5;
    hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma.Init.MemInc = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma.Init.Mode = DMA_CIRCULAR;
    hdma.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma);
    __HAL_LINKDMA(&htim, hdma[TIM_DMA_ID_CC1], hdma);
    
    // Uruchomienie sygnału
    HAL_TIM_PWM_Start(&htim, TIM_CHANNEL_1);
}

void RC5_Receive() {
    // Konfiguracja wejścia timera do przechwytywania sygnału
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RC5_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RC5_GPIO_PORT, &GPIO_InitStruct);
    
    // Konfiguracja timera
    htim.Instance = RC5_TIMER;
    htim.Init.Prescaler = 90 - 1;
    htim.Init.Period = 65535;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&htim);
    
    // Konfiguracja DMA do przechwytywania danych
    hdma.Instance = RC5_DMA_CHANNEL;
    hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma.Init.MemInc = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma.Init.Mode = DMA_CIRCULAR;
    hdma.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma);
    __HAL_LINKDMA(&htim, hdma[TIM_DMA_ID_CC1], hdma);
    
    // Uruchomienie nasłuchu
    HAL_TIM_IC_Start_DMA(&htim, TIM_CHANNEL_1, (uint32_t*)buffer, BUFFER_SIZE);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == RC5_TIMER) {
        // Obsługa zakończenia generowania RC5
    }
}

void HAL_DMA_IRQHandler(DMA_HandleTypeDef *hdma) {
    HAL_DMA_IRQHandler(hdma);
}
