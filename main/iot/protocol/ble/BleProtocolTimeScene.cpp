#include "BleProtocol.h"
#include "Log.h"

int BleProtocol::SceneAddDevice(uint16_t devAddr, uint16_t scene, uint8_t modeRgb)
{
	LOGD("Set scene addr: 0x%04X to scene: 0x%04X, modergb: %d", devAddr, scene, modeRgb);
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_time_scene_client_set_state_t set_state = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													scene_client.model,
													ESP_BLE_MESH_MODEL_OP_SCENE_STORE);
	set_state.scene_store.scene_number = scene;
	esp_err_t err = esp_ble_mesh_time_scene_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Set scene device 0x%04X to scene 0x%04X: %d failed", devAddr, scene);
	}
	return err;
}

int BleProtocol::SceneDelDevice(uint16_t devAddr, uint16_t scene)
{
	LOGD("Del scene addr: 0x%04X to scene: 0x%04X", devAddr, scene);
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_time_scene_client_set_state_t set_state = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													scene_client.model,
													ESP_BLE_MESH_MODEL_OP_SCENE_DELETE);
	set_state.scene_delete.scene_number = scene;
	esp_err_t err = esp_ble_mesh_time_scene_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Del scene device 0x%04X to scene 0x%04X: %d failed", devAddr, scene);
	}
	return err;
}

int BleProtocol::SceneRecall(uint16_t devAddr, uint16_t scene, uint16_t transition, bool ack)
{
	LOGD("Call scene: 0x%04X", scene);
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_time_scene_client_set_state_t set_state = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													scene_client.model,
													ack ? ESP_BLE_MESH_MODEL_OP_SCENE_RECALL : ESP_BLE_MESH_MODEL_OP_SCENE_RECALL_UNACK);
	set_state.scene_recall.tid = tid++;
	set_state.scene_recall.op_en = true;
	set_state.scene_recall.scene_number = scene;
	set_state.scene_recall.trans_time = transition;
	set_state.scene_recall.delay = 0;
	esp_err_t err = esp_ble_mesh_time_scene_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Call scene device 0x%04X with scene 0x%04X: %d failed", devAddr, scene);
	}
	return err;
}