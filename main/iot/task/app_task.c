#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "Log.h"

TaskHandle_t app_new_task(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t ulStackDepth, void *const pvParameters, UBaseType_t uxPriority)
{

  // Allocate stack memory from PSRAM
  StackType_t *psram_stack = (StackType_t *)heap_caps_malloc(ulStackDepth * sizeof(StackType_t), MALLOC_CAP_SPIRAM);
  if (!psram_stack)
  {
    LOGE("Failed to allocate stack in PSRAM");
    // return;
  }

  // Allocate static task control block
  StaticTask_t *task_buffer = (StaticTask_t *)heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL);
  if (!task_buffer)
  {
    LOGE("Failed to allocate task control block");
    // return;
  }

  TaskHandle_t handle = xTaskCreateStaticPinnedToCore(
      pvTaskCode,   // Task function
      pcName,       // Task name
      ulStackDepth, // Stack size (words, not bytes)
      pvParameters, // Param
      uxPriority,   // Priority
      psram_stack,  // Stack in PSRAM
      task_buffer,  // Task control block
      1             // Core 1 (APP_CPU)
  );

  if (!handle)
  {
    LOGE("Failed to create task");
  }

  return handle;
}