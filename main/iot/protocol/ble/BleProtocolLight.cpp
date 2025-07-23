#include "BleProtocol.h"
#include "Log.h"

int BleProtocol::SetDimmingLight(uint16_t devAddr, uint16_t dim, uint16_t transition, bool ack)
{
	LOGD("Dimming addr: 0x%04X value %d", devAddr, dim);
	esp_ble_mesh_light_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													level_client.model,
													ack ? ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET : ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK);
	// dim = ((uint32_t)dim * 65535) / 100;
	set_state.lightness_set.lightness = dim;
	set_state.lightness_set.op_en = false;
	set_state.lightness_set.tid = tid++;
	set_state.lightness_set.delay = 0;
	set_state.lightness_set.trans_time = transition;
	esp_err_t err = esp_ble_mesh_light_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Set Dimming addr: 0x%04X value %d, ack: %d failed", devAddr, dim, ack);
	}
	return CODE_ERROR;
}

int BleProtocol::SetLevelDim(uint16_t devAddr, uint8_t dimMax, uint8_t dimMin, bool ack)
{
	LOGW("Set Level Dim addr: 0x%04X, dimMax: %d, dimMin: %d", devAddr, dimMax, dimMin);
	esp_ble_mesh_light_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													level_client.model,
													ack ? ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET : ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK);
	set_state.lightness_range_set.range_min = dimMin;
	set_state.lightness_range_set.range_max = dimMax;
	esp_err_t err = esp_ble_mesh_light_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Set Dimming addr: 0x%04X dimMax: %d, dimMin: %d, ack: %d failed", devAddr, dimMax, dimMin, ack);
	}
	return CODE_ERROR;
}

int BleProtocol::SetCctLight(uint16_t devAddr, uint16_t cct, uint16_t transition, bool ack)
{
	LOGD("Set Cct addr: 0x%04X value %d", devAddr, cct);
	esp_ble_mesh_light_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													ctl_client.model,
													ack ? ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_TEMPERATURE_SET : ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_TEMPERATURE_SET_UNACK);
	// if (devAddr < 0xC000)
	// 	devAddr = devAddr + 1;
	// cct = (cct * 192 + 800);
	set_state.ctl_temperature_set.ctl_temperature = cct; // 800 - 20000
	set_state.ctl_temperature_set.tid = tid++;
	set_state.ctl_temperature_set.delay = 0;
	set_state.ctl_temperature_set.trans_time = transition;
	esp_err_t err = esp_ble_mesh_light_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Set CCT addr: 0x%04X value %d, ack: %d failed", devAddr, cct, ack);
	}
	return CODE_ERROR;
}

int BleProtocol::SetHSLLight(uint16_t devAddr, uint16_t H, uint16_t S, uint16_t L, uint16_t transition, bool ack)
{
	LOGD("HSL addr: 0x%04X value HSL: %d-%d-%d", devAddr, H, S, L);
	esp_ble_mesh_light_client_set_state_t set_state = {0};
	esp_ble_mesh_client_common_param_t common = {0};
	ble_mesh_set_msg_common(devAddr,
													&common,
													hsl_client.model,
													ack ? ESP_BLE_MESH_MODEL_OP_LIGHT_HSL_SET : ESP_BLE_MESH_MODEL_OP_LIGHT_HSL_SET_UNACK);
	set_state.hsl_set.op_en = false;
	set_state.hsl_set.hsl_hue = H;
	set_state.hsl_set.hsl_saturation = S;
	set_state.hsl_set.hsl_lightness = L;
	set_state.hsl_set.tid = tid++;
	set_state.hsl_set.delay = 0;
	set_state.hsl_set.trans_time = transition;
	esp_err_t err = esp_ble_mesh_light_client_set_state(&common, &set_state);
	if (err)
	{
		LOGW("Set HSL addr: 0x%04X h: %d, s: %d, l: %d, ack: %d failed", devAddr, H, S, L, ack);
	}
	return CODE_ERROR;
}

int BleProtocol::SetCctDimLight(uint16_t devAddr, uint16_t cct, uint16_t dim, uint16_t transition, bool ack)
{
	LOGW("Set Dim cct addr: 0x%04X", devAddr);
	return CODE_ERROR;
}

int BleProtocol::UpdateLights(uint16_t devAddr)
{
	LOGW("Update lights addr: 0x%04X ", devAddr);
	return CODE_ERROR;
}
