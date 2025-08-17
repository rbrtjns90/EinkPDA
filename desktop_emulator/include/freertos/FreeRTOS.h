#ifndef FREERTOS_H
#define FREERTOS_H

#include "../pocketmage_compat.h"

// FreeRTOS types
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;
typedef uint32_t TickType_t;
typedef uint32_t UBaseType_t;
typedef int32_t BaseType_t;

// FreeRTOS constants
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS pdTRUE
#define pdFAIL pdFALSE
#define portMAX_DELAY 0xFFFFFFFF
#define configTICK_RATE_HZ 1000

// FreeRTOS functions
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskDelete(TaskHandle_t xTaskToDelete);
TaskHandle_t xTaskGetCurrentTaskHandle();
TickType_t xTaskGetTickCount();
void taskYIELD();

#endif
