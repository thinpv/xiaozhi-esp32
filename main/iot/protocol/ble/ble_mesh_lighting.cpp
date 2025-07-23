#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"
#include "ble_mesh_protocol.h"

#include "Log.h"
#include "esp_timer.h"
#include "DeviceManager.h"

extern "C" void example_ble_mesh_lighting_client_cb(esp_ble_mesh_light_client_cb_event_t event,
																										esp_ble_mesh_light_client_cb_param_t *param)
{
	esp_ble_mesh_client_common_param_t common = {0};
	uint32_t opcode;
	uint16_t addr;
	int err;

	opcode = param->params->opcode;
	addr = param->params->ctx.addr;

	LOGI("%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
			 __func__, param->error_code, event, addr, opcode);

	if (param->error_code)
	{
		LOGE("Send generic client message failed, opcode 0x%04x", opcode);
		return;
	}

	DeviceBle *deviceBle = DeviceManager::GetInstance()->GetDeviceBleFromAddr(addr);
	if (!deviceBle)
	{
		LOGE("%s: Get deviceBle failed", __func__);
		return;
	}

	switch (event)
	{
	case ESP_BLE_MESH_LIGHT_CLIENT_GET_STATE_EVT:
		break;
	case ESP_BLE_MESH_LIGHT_CLIENT_SET_STATE_EVT:
		break;
	case ESP_BLE_MESH_LIGHT_CLIENT_PUBLISH_EVT:
		switch (opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS:
		{
			struct __attribute__((packed))
			{
				uint16_t opcode;
				uint16_t dim;
			} data_message = {
					.opcode = bswap_16(opcode),
					.dim = param->status_cb.lightness_status.present_lightness};
			deviceBle->InputData((uint8_t *)&data_message, sizeof(data_message), addr);
		}
		break;
		case ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_TEMPERATURE_STATUS:
		{
			struct __attribute__((packed))
			{
				uint16_t opcode;
				uint16_t cct;
			} data_message = {
					.opcode = bswap_16(opcode),
					.cct = param->status_cb.ctl_temperature_status.present_ctl_temperature};
			deviceBle->InputData((uint8_t *)&data_message, sizeof(data_message), addr);
		}
		break;
		default:
			break;
		}
		break;
	case ESP_BLE_MESH_LIGHT_CLIENT_TIMEOUT_EVT:
		break;
	case ESP_BLE_MESH_LIGHT_CLIENT_EVT_MAX:
	default:
		LOGE("Not a generic client status message event");
		break;
	}
}