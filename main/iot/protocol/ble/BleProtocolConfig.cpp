#include "BleProtocol.h"
#include "Log.h"

int BleProtocol::GroupAddDevice(uint16_t devAddr, uint16_t element, uint16_t group)
{
	// TODO: check model_id folow element
	LOGD("Add dev addr: 0x%04X  with element: 0x%04x to group: 0x%04X", devAddr, element, group);
	esp_ble_mesh_cfg_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr, &common, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD);
	set_state.model_sub_add.element_addr = element;
	set_state.model_sub_add.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
	set_state.model_sub_add.sub_addr = group;
	set_state.model_sub_add.company_id = ESP_BLE_MESH_CID_NVAL; // ESP_BLE_MESH_CID_NVAL 0xFFFF
	esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Add device 0x%04X to group 0x%04X: %d failed", devAddr, group);
	}
	return CODE_ERROR;
}

int BleProtocol::GroupDelDevice(uint16_t devAddr, uint16_t element, uint16_t group)
{
	// TODO: check model_id folow element
	LOGD("Del dev addr: 0x%04X  with element: 0x%04x from group: 0x%04X", devAddr, element, group);
	esp_ble_mesh_cfg_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr, &common, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE);
	set_state.model_sub_delete.element_addr = element;
	set_state.model_sub_delete.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_SRV;
	set_state.model_sub_delete.sub_addr = group;
	set_state.model_sub_delete.company_id = ESP_BLE_MESH_CID_NVAL; // ESP_BLE_MESH_CID_NVAL 0xFFFF
	esp_err_t err = esp_ble_mesh_config_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Del device 0x%04X from group 0x%04X: %d failed", devAddr, group);
	}
	return CODE_ERROR;
}