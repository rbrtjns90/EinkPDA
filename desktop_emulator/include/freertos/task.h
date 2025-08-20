#ifndef FREERTOS_TASK_H
#define FREERTOS_TASK_H

#include "FreeRTOS.h"

// Task creation functions
BaseType_t xTaskCreate(void (*pvTaskCode)(void*), const char* const pcName, 
                      const uint16_t usStackDepth, void* const pvParameters,
                      UBaseType_t uxPriority, TaskHandle_t* const pxCreatedTask);

BaseType_t xTaskCreatePinnedToCore(void (*pvTaskCode)(void*), const char* const pcName,
                                  const uint32_t ulStackDepth, void* const pvParameters,
                                  UBaseType_t uxPriority, TaskHandle_t* const pvCreatedTask,
                                  const BaseType_t xCoreID);

// Task control functions
void vTaskDelete(TaskHandle_t xTaskToDelete);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t* const pxPreviousWakeTime, const TickType_t xTimeIncrement);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);

// Task utilities
TaskHandle_t xTaskGetCurrentTaskHandle();
char* pcTaskGetTaskName(TaskHandle_t xTaskToQuery);
UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask);
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);

// Task scheduling
void taskYIELD();
void portYIELD();

#endif
