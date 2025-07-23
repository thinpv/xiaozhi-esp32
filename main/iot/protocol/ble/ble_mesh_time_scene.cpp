#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_time_scene_model_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"
#include "ble_mesh_protocol.h"

#include "Log.h"
#include "esp_timer.h"
#include "DeviceManager.h"

extern "C" void example_ble_mesh_time_scene_client_cb(esp_ble_mesh_time_scene_client_cb_event_t event,
																											esp_ble_mesh_time_scene_client_cb_param_t *param)
{
	esp_ble_mesh_client_common_param_t common = {0};
	uint32_t opcode;
	uint16_t addr;
	int err;

	opcode = param->params->opcode;
	addr = param->params->ctx.addr;

	LOGI("%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
			 __func__, param->error_code, event, addr, opcode);
	LOGW("example_ble_mesh_time_scene_client_cb: %d", event);
}