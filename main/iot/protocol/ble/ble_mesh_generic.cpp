
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"

#include "Log.h"
#include "esp_timer.h"
#include "DeviceManager.h"

extern "C" void example_ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
																									 esp_ble_mesh_generic_client_cb_param_t *param)
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
	case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
		switch (opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
		{
			// esp_ble_mesh_generic_client_set_state_t set_state = {0};
			// node->onoff = param->status_cb.onoff_status.present_onoff;
			// LOGI("ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET onoff: 0x%02x", node->onoff);
			// /* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
			// example_ble_mesh_set_msg_common(&common, node->unicast, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
			// set_state.onoff_set.op_en = false;
			// set_state.onoff_set.onoff = !node->onoff;
			// set_state.onoff_set.tid = 0;
			// err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
			// if (err)
			// {
			// 	LOGE("%s: Generic OnOff Set failed", __func__);
			// 	return;
			// }
			break;
		}
		default:
			break;
		}
		break;
	case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
		switch (opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
			// node->onoff = param->status_cb.onoff_status.present_onoff;
			// LOGI("ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET onoff: 0x%02x", node->onoff);
			break;
		default:
			break;
		}
		break;
	case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
		switch (opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS:
		{
			// node->onoff = param->status_cb.onoff_status.present_onoff;
			LOGI("ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS onoff: 0x%02x", param->status_cb.onoff_status.present_onoff);
			LOGD("Device: %s", deviceBle->getMac().c_str());
			struct __attribute__((packed))
			{
				uint16_t opcode;
				uint8_t state;
			} data_message = {
					.opcode = bswap_16(opcode),
					.state = param->status_cb.onoff_status.present_onoff};
			deviceBle->InputData((uint8_t *)&data_message, sizeof(data_message), addr);
		}
		break;
		default:
			break;
		}
		break;
	case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
		/* If failed to receive the responses, these messages will be resend */
		switch (opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
		{
			// esp_ble_mesh_generic_client_get_state_t get_state = {0};
			// example_ble_mesh_set_msg_common(&common, node->unicast, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
			// err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
			// if (err)
			// {
			// 	LOGE("%s: Generic OnOff Get failed", __func__);
			// 	return;
			// }
			break;
		}
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
		{
			// esp_ble_mesh_generic_client_set_state_t set_state = {0};
			// node->onoff = param->status_cb.onoff_status.present_onoff;
			// LOGI("ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET onoff: 0x%02x", node->onoff);
			// example_ble_mesh_set_msg_common(&common, node->unicast, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET);
			// set_state.onoff_set.op_en = false;
			// set_state.onoff_set.onoff = !node->onoff;
			// set_state.onoff_set.tid = 0;
			// err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
			// if (err)
			// {
			// 	LOGE("%s: Generic OnOff Set failed", __func__);
			// 	return;
			// }
			break;
		}
		default:
			break;
		}
		break;
	default:
		LOGE("Not a generic client status message event");
		break;
	}
}