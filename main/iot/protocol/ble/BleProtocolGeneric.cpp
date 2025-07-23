#include "BleProtocol.h"
#include "Log.h"

int BleProtocol::SetOnOffLight(uint16_t devAddr, uint8_t onoff, uint16_t transition, bool ack)
{
	LOGD("Set OnOff addr: 0x%04X value %d, ack: %d", devAddr, onoff, ack);
	esp_ble_mesh_generic_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													onoff_client.model,
													ack ? ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET : ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
	set_state.onoff_set.op_en = false;
	set_state.onoff_set.onoff = onoff;
	set_state.onoff_set.delay = 0;
	set_state.onoff_set.trans_time = transition;
	set_state.onoff_set.tid = tid++;
	esp_err_t err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Set OnOff addr: 0x%04X value %d, ack: %d failed", devAddr, onoff, ack);
	}
	return err;
}

int BleProtocol::GetOnOffLight(uint16_t devAddr)
{
	LOGD("Get OnOff addr: 0x%04X", devAddr);
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_generic_client_get_state_t get_state = {0};
	ble_mesh_set_msg_common(devAddr, &common, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET);
	esp_err_t err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
	if (err)
	{
		LOGW("Generic OnOff Get failed with error %s", esp_err_to_name(err));
	}
	return err;
}
