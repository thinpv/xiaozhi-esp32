#ifndef APP_TASK_H__
#define APP_TASK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C"
{
#endif

  TaskHandle_t app_new_task(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t ulStackDepth, void *const pvParameters, UBaseType_t uxPriority);

#ifdef __cplusplus
} /* end of the 'extern "C"' block */
#endif

#endif