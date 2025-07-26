#include "Database.h"
#include "TimerSchedule.h"
#include "BleProtocol.h"
#include "Gateway.h"
#include "app_task.h"
#include "Log.h"
#include "esp_netif.h"

void iot_thread(void *arg)
{
	esp_log_level_set("*", ESP_LOG_INFO);
	log_set_level(XLOG_DEBUG);
	LOGI("Start IOT Thread");
	ESP_ERROR_CHECK(esp_netif_init());
	int isCreateDb = Database::GetInstance()->init();
	TimerSchedule::GetInstance()->init();
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
	// vTaskDelete(NULL); // Delete the task after execution
}

void iot_main()
{
	// vTaskDelay(10000 / portTICK_PERIOD_MS);
	LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

	iot_thread(NULL);

	// if (!app_new_task(iot_thread, // Task function
	// 									"iot_main", // Task name
	// 									20480,			// Stack size (words, not bytes)
	// 									NULL,				// Param
	// 									5						// Priority
	// 									))
	// {
	// 	LOGE("Failed to create pushTelemetryThread task");
	// }
}