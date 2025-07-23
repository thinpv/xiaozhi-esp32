
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"

#include "BleMeshDefine.h"

#include "app_ble_mesh_provi.h"
#include "Log.h"

#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"

extern esp_ble_mesh_client_t vendor_client;
extern prov_key_t prov_key;

void genSecurityKey(uint8_t *mac, uint16_t devAddr, uint8_t *out);

int esp_ble_send_vendor(uint32_t opcode, uint16_t addr, uint16_t len, uint8_t *data, bool ack)
{
	esp_ble_mesh_msg_ctx_t ctx = {
			.net_idx = prov_key.net_idx,
			.app_idx = prov_key.app_idx,
			.addr = addr,
			.send_rel = false,
			.send_ttl = 3,
	};
	return esp_ble_mesh_client_model_send_msg(vendor_client.model,
																						&ctx,
																						opcode,
																						len,
																						data,
																						MSG_TIMEOUT,
																						ack,
																						MSG_ROLE);
}

int esp_ble_set_gw_addr(uint16_t devAddr)
{
	LOGD("esp_ble_set_gw_addr: 0x%04X", devAddr);
	struct __attribute__((packed))
	{
		uint16_t header;
		uint16_t gwAddr;
		// uint8_t data[4];
	} set_gw_addr_message = {
			.header = RD_HEADER_PROVISION_SET_GW_ADDR,
			.gwAddr = PROV_OWN_ADDR,
	};
	int err = esp_ble_send_vendor(ESP_BLE_MESH_VND_MODEL_OP_SEND_E0,
																devAddr,
																sizeof(set_gw_addr_message),
																(uint8_t *)&set_gw_addr_message,
																true);
	if (err != ESP_OK)
	{
		LOGE("esp_ble_set_gw_addr failed - %d", err);
	}
	return err;
}

int esp_ble_get_device_type(uint8_t *mac, uint16_t devAddr)
{
	LOGD("esp_ble_get_device_type: 0x%04X", devAddr);
	struct __attribute__((packed))
	{
		uint16_t header;
		uint8_t data[6];
	} check_type_message = {
			.header = RD_HEADER_PROVISION_GET_DEV_TYPE,
	};
	genSecurityKey(mac, devAddr, &check_type_message.data[0]);
	int err = esp_ble_send_vendor(ESP_BLE_MESH_VND_MODEL_OP_SEND_E0,
																devAddr,
																sizeof(check_type_message),
																(uint8_t *)&check_type_message,
																true);
	if (err != ESP_OK)
	{
		LOGE("esp_ble_get_device_type failed - %d", err);
	}
	return err;
}

void example_ble_mesh_send_vendor_message(bool resend)
{
	// esp_ble_mesh_msg_ctx_t ctx = {0};
	// uint32_t opcode;
	// esp_err_t err;

	// ctx.net_idx = prov_key.net_idx;
	// ctx.app_idx = prov_key.app_idx;
	// ctx.addr = store.server_addr;
	// ctx.send_ttl = MSG_SEND_TTL;
	// opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND_E0;

	// if (resend == false)
	// {
	// 	store.tid++;
	// }

	// err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
	// 																				 sizeof(store.tid), (uint8_t *)&store.tid, MSG_TIMEOUT, true, MSG_ROLE);
	// if (err != ESP_OK)
	// {
	// 	LOGE("Failed to send vendor message 0x%06" PRIx32, opcode);
	// 	return;
	// }

	// mesh_example_info_store(); /* Store proper mesh example info */
}