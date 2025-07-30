#include "Database.h"
#include "TimerSchedule.h"
#include "BleProtocol.h"
#include "Gateway.h"
#include "Log.h"
#include "esp_netif.h"
#include "system_info.h"

void iot_thread(void *arg)
{
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    esp_log_level_set("*", ESP_LOG_INFO);
    log_set_level(XLOG_DEBUG);
    LOGI("Start IOT Thread");
    int isCreateDb = Database::GetInstance()->init();
    // TimerSchedule::GetInstance()->init();
    BleProtocol::GetInstance()->init();
    Gateway::GetInstance()->init();
    if (isCreateDb)
    {
#ifdef CONFIG_ENABLE_BLE
        BleProtocol::GetInstance()->ResetDelAll();
        BleProtocol::GetInstance()->ResetFactory();
#endif

#ifdef CONFIG_ENABLE_ZIGBEE
        ZigbeeProtocol::GetInstance()->ResetFactory();
        ZigbeeProtocol::GetInstance()->CommissionFormation();
#endif
    }
    BleProtocol::GetInstance()->initKey(Gateway::GetInstance()->getBleNetKey(), Gateway::GetInstance()->getBleAppKey());

    SystemInfo::PrintTaskCpuUsage(pdMS_TO_TICKS(1000));
    SystemInfo::PrintTaskList();
    LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

    // vTaskDelete(NULL); // Delete the task after execution
}

void iot_main()
{
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

    iot_thread(NULL);

    // if (!xTaskCreatePinnedToCoreWithCaps(iot_thread, // Task function
    //                                      "iot_main", // Task name
    //                                      20480,      // Stack size (words, not bytes)
    //                                      NULL,       // Param
    //                                      5,          // Priority
    //                                      NULL,       // Task handle
    //                                      1,          // Core 1 (APP_CPU)
    //                                      MALLOC_CAP_SPIRAM))
    // {
    //     LOGE("Failed to create pushTelemetryThread task");
    // }
}