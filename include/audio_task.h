/*
 * Author: Yang
 * Audio DMA sampling and FFT task.
 */
#pragma once

#include <Arduino.h>

void startAudioTask();
uint32_t getAudioTaskStackWatermark();
