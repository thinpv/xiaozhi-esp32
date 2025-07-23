#include "BleProtocol.h"
#include "Log.h"

int BleProtocol::CallModeRgb(uint16_t devAddr, uint8_t modeRgb)
{
	typedef struct __attribute__((packed))
	{
		uint16_t header;
		uint8_t mode;
	} mode_rgb_message_t;
	mode_rgb_message_t mode_rgb_message = {
			.header = HEADER_CALLMODE_RGB,
			.mode = modeRgb,
	};
	int err = esp_ble_send_vendor(ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET,
																devAddr,
																sizeof(mode_rgb_message_t),
																(uint8_t *)&mode_rgb_message,
																true);
	if (err != ESP_OK)
	{
		LOGE("esp_ble_get_device_type failed - %d", err);
	}
	return err;
}