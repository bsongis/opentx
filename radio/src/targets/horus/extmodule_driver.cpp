/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

void extmoduleSendNextFrame();

void extmoduleStop()
{
  NVIC_DisableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_DisableIRQ(EXTMODULE_TIMER_IRQn);

  EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
  EXTMODULE_TIMER->DIER &= ~(TIM_DIER_CC2IE | TIM_DIER_UDE);
  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;

  if (!IS_TRAINER_EXTERNAL_MODULE()) {
    EXTERNAL_MODULE_OFF();
  }
}

void extmoduleNoneStart()
{
  if (!IS_TRAINER_EXTERNAL_MODULE()) {
    EXTERNAL_MODULE_OFF();
  }

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, 0);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);
  GPIO_SetBits(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PIN); // Set high

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  EXTMODULE_TIMER->ARR = 36000; // 18mS
  EXTMODULE_TIMER->CCR2 = 32000; // Update time
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;
  EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE; // Enable this interrupt
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 7);
}

void extmodulePpmStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, EXTMODULE_PPM_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);

  // PPM generation principle:
  //
  // Hardware timer in PWM mode is used for PPM generation
  // Output is OFF if CNT<CCR1(delay) and ON if bigger
  // CCR1 register defines duration of pulse length and is constant
  // AAR register defines duration of each pulse, it is
  // updated after every pulse in Update interrupt handler.
  // CCR2 register defines duration of no pulses (time between two pulse trains)
  // it is calculated every round to have PPM period constant.
  // CC2 interrupt is then used to setup new PPM values for the
  // next PPM pulses train.

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN; // Stop timer
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  EXTMODULE_TIMER->ARR = 41000;
  // CH1 PWM mode 1, CH1 out is OFF if CNT<CCR1(delay) and ON if bigger (up to AAR,which is pulse length)
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC2PE;        // PWM mode 1
  EXTMODULE_TIMER->EGR = 1; // Reloads register vales now
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE; // Update DMA request
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN; // Start timer

  setupPulsesPPM(EXTERNAL_MODULE);
  extmoduleSendNextFrame();

  NVIC_EnableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_SetPriority(EXTMODULE_DMA_IRQn, 7);
  NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 7);
}

void set_external_ppm_parameters(uint32_t idleTime, uint32_t delay, uint32_t positive)
{
  EXTMODULE_TIMER->CCR2 = idleTime;
  EXTMODULE_TIMER->CCR1 = delay;
  EXTMODULE_TIMER->CCER = TIM_CCER_CC1E | (positive ? 0 : TIM_CCER_CC1P);
}

void extmodulePxxStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, EXTMODULE_PPM_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  EXTMODULE_TIMER->ARR = 18000;
  EXTMODULE_TIMER->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NE | TIM_CCER_CC1NP; //  TIM_CCER_CC1E | TIM_CCER_CC1P;
  EXTMODULE_TIMER->BDTR = TIM_BDTR_MOE; // Enable outputs
  EXTMODULE_TIMER->CCR1 = 18;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0; // Force O/P high
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE; // Enable DMA on update
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  setupPulsesPXX(EXTERNAL_MODULE);
  extmoduleSendNextFrame();

  NVIC_EnableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_SetPriority(EXTMODULE_DMA_IRQn, 7);
  NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 7);
}

#if defined(DSM2)
void extmoduleDsm2Start()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, EXTMODULE_PPM_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  EXTMODULE_TIMER->ARR = 44000; // 22mS
  EXTMODULE_TIMER->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P;
  EXTMODULE_TIMER->BDTR = TIM_BDTR_MOE; // Enable outputs
  EXTMODULE_TIMER->CCR1 = 0;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0; // Force O/P high
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE; // Enable DMA on update
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  setupPulsesDSM2(EXTERNAL_MODULE);
  extmoduleSendNextFrame();

  NVIC_EnableIRQ(EXTMODULE_DMA_IRQn);
  NVIC_SetPriority(EXTMODULE_DMA_IRQn, 7);
  NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 7);
}
#endif

void extmoduleCrossfireStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PinSource, 0);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_PPM_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_PPM_GPIO, &GPIO_InitStructure);
  GPIO_SetBits(EXTMODULE_PPM_GPIO, EXTMODULE_PPM_GPIO_PIN); // Set high

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  EXTMODULE_TIMER->ARR = (2000 * CROSSFIRE_FRAME_PERIOD);
  EXTMODULE_TIMER->CCR2 = (2000 * CROSSFIRE_FRAME_PERIOD) - 1000;
  EXTMODULE_TIMER->EGR = 1; // Restart
  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;
  EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE; // Enable this interrupt
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 7);
}

void extmoduleSendNextFrame()
{
  if (s_current_protocol[EXTERNAL_MODULE] == PROTO_PPM) {
    EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
    EXTMODULE_DMA_STREAM->CR |= EXTMODULE_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_1 | DMA_SxCR_MSIZE_1 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    EXTMODULE_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
    EXTMODULE_DMA_STREAM->M0AR = CONVERT_PTR_UINT(modulePulsesData[EXTERNAL_MODULE].ppm.pulses);
    EXTMODULE_DMA_STREAM->NDTR = modulePulsesData[EXTERNAL_MODULE].ppm.ptr - modulePulsesData[EXTERNAL_MODULE].ppm.pulses;
    EXTMODULE_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
  }
  else if (s_current_protocol[EXTERNAL_MODULE] == PROTO_PXX) {
    EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
    EXTMODULE_DMA_STREAM->CR |= EXTMODULE_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_1 | DMA_SxCR_MSIZE_1 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    EXTMODULE_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
    EXTMODULE_DMA_STREAM->M0AR = CONVERT_PTR_UINT(modulePulsesData[EXTERNAL_MODULE].pxx.pulses);
    EXTMODULE_DMA_STREAM->NDTR = modulePulsesData[EXTERNAL_MODULE].pxx.ptr - modulePulsesData[EXTERNAL_MODULE].pxx.pulses;
    EXTMODULE_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
  }
  else if ((s_current_protocol[EXTERNAL_MODULE] >= PROTO_DSM2_LP45 && s_current_protocol[EXTERNAL_MODULE] <= PROTO_DSM2_DSMX)
#if defined(MULTIMODULE)
    || s_current_protocol[EXTERNAL_MODULE] == PROTO_MULTIMODULE
#endif
    ) {
    EXTMODULE_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
    EXTMODULE_DMA_STREAM->CR |= EXTMODULE_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_1 | DMA_SxCR_MSIZE_1 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
    EXTMODULE_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
    EXTMODULE_DMA_STREAM->M0AR = CONVERT_PTR_UINT(modulePulsesData[EXTERNAL_MODULE].dsm2.pulses);
    EXTMODULE_DMA_STREAM->NDTR = modulePulsesData[EXTERNAL_MODULE].dsm2.ptr - modulePulsesData[EXTERNAL_MODULE].dsm2.pulses;
    EXTMODULE_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
  }
  else {
    EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE;
  }
}

extern "C" void EXTMODULE_DMA_IRQHandler()
{
  if (!DMA_GetITStatus(EXTMODULE_DMA_STREAM, EXTMODULE_DMA_FLAG_TC))
    return;

  DMA_ClearITPendingBit(EXTMODULE_DMA_STREAM, EXTMODULE_DMA_FLAG_TC);

  uint32_t arr = EXTMODULE_TIMER->ARR;
  if (arr > 4000) {
    EXTMODULE_TIMER->CCR2 = arr - 4000; // 2mS in advance
    EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF; // Clear flag
    EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE; // Enable this interrupt
  }
  else {
    setupPulses(EXTERNAL_MODULE);
    extmoduleSendNextFrame();
  }
}

extern "C" void EXTMODULE_TIMER_IRQHandler()
{
  EXTMODULE_TIMER->DIER &= ~TIM_DIER_CC2IE; // Stop this interrupt
  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;
  setupPulses(EXTERNAL_MODULE);
  extmoduleSendNextFrame();
}
