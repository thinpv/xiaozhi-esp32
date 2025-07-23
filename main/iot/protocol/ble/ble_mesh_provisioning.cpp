#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"

#include "Log.h"
#include "esp_timer.h"
#include "DeviceManager.h"

extern esp_ble_mesh_client_t config_client;
extern scan_device_t scan_device;
extern prov_key_t prov_key;
bool isProvisioning = false;

static void recv_unprov_adv_pkt(uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN], uint8_t addr[BD_ADDR_LEN],
																esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
																uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{
	esp_ble_mesh_unprov_dev_add_t add_dev = {0};
	esp_err_t err;

	/* Due to the API esp_ble_mesh_provisioner_set_dev_uuid_match, Provisioner will only
	 * use this callback to report the devices, whose device UUID starts with 0xdd & 0xdd,
	 * to the application layer.
	 */

	// ESP_LOG_BUFFER_HEX("Device address", addr, BD_ADDR_LEN);
	// LOGI("Address type 0x%02x, adv type 0x%02x", addr_type, adv_type);
	// ESP_LOG_BUFFER_HEX("Device UUID", dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
	// LOGI("oob info 0x%04x, bearer %s", oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

	if ((scan_device.mac[0] +
			 scan_device.mac[1] +
			 scan_device.mac[2] +
			 scan_device.mac[3] +
			 scan_device.mac[4] +
			 scan_device.mac[5]) &&
			(time(NULL) - scan_device.lastUpdateTime) < 10)
	{
		return;
	}
	ESP_LOG_BUFFER_HEX("Add Device address", addr, BD_ADDR_LEN);

	memset(&scan_device, 0, sizeof(scan_device));
	for (int i = 0; i < BD_ADDR_LEN; i++)
	{
		scan_device.mac[i] = addr[BD_ADDR_LEN - 1 - i];
	}
	scan_device.lastUpdateTime = time(NULL);

	memcpy(add_dev.addr, addr, BD_ADDR_LEN);
	add_dev.addr_type = (esp_ble_mesh_addr_type_t)addr_type;
	memcpy(add_dev.uuid, dev_uuid, ESP_BLE_MESH_OCTET16_LEN);
	add_dev.oob_info = oob_info;
	add_dev.bearer = (esp_ble_mesh_prov_bearer_t)bearer;
	/* Note: If unprovisioned device adv packets have not been received, we should not add
					 device with ADD_DEV_START_PROV_NOW_FLAG set. */
	err = esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
																								ADD_DEV_RM_AFTER_PROV_FLAG |
																										ADD_DEV_START_PROV_NOW_FLAG |
																										ADD_DEV_FLUSHABLE_DEV_FLAG);
	if (err != ESP_OK)
	{
		LOGE("Failed to start provisioning device");
	}
}

static esp_err_t prov_complete(uint16_t node_index, const esp_ble_mesh_octet16_t uuid,
															 uint16_t primary_addr, uint8_t element_num, uint16_t net_idx)
{
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_cfg_client_get_state_t get = {0};
	esp_ble_mesh_node_t *node = NULL;
	char name[10] = {'\0'};
	esp_err_t err;

	LOGI("node_index %u, primary_addr 0x%04x, element_num %u, net_idx 0x%03x",
			 node_index, primary_addr, element_num, net_idx);
	ESP_LOG_BUFFER_HEX("uuid", uuid, ESP_BLE_MESH_OCTET16_LEN);

	sprintf(name, "%s%02x", "NODE-", node_index);
	err = esp_ble_mesh_provisioner_set_node_name(node_index, name);
	if (err != ESP_OK)
	{
		LOGE("Failed to set node name");
		return ESP_FAIL;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(primary_addr);
	if (node == NULL)
	{
		LOGE("Failed to get node 0x%04x info", primary_addr);
		return ESP_FAIL;
	}

	memcpy(scan_device.uuid, uuid, 16);
	scan_device.primary_addr = primary_addr;
	scan_device.element_num = element_num;

	example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
	get.comp_data_get.page = COMP_DATA_PAGE_0;
	err = esp_ble_mesh_config_client_get_state(&common, &get);
	if (err != ESP_OK)
	{
		LOGE("Failed to send Config Composition Data Get");
		return ESP_FAIL;
	}

	return ESP_OK;
}

extern "C" void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
																								 esp_ble_mesh_prov_cb_param_t *param)
{
	if (!isProvisioning)
		return;

	switch (event)
	{
	case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
		// LOGI("ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
		recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr,
												param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info,
												param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT, bearer %s",
				 param->provisioner_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT, bearer %s, reason 0x%02x",
				 param->provisioner_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", param->provisioner_prov_link_close.reason);
		break;
	case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
		prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
									param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
									param->provisioner_prov_complete.netkey_idx);
		break;
	case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
		if (param->provisioner_set_node_name_comp.err_code == 0)
		{
			const char *name = esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
			if (name)
			{
				LOGI("Node %d name %s", param->provisioner_set_node_name_comp.node_index, name);
			}
		}
		break;
	case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
		if (param->provisioner_add_app_key_comp.err_code == ESP_OK)
		{
			esp_err_t err = 0;
			prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_HSL_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_SCHEDULER_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_SCENE_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_TIME_CLI,
					ESP_BLE_MESH_CID_NVAL);
			// Sensor
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_SENSOR_CLI,
					ESP_BLE_MESH_CID_NVAL);
			// Battery
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_BATTERY_CLI,
					ESP_BLE_MESH_CID_NVAL);
			err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, prov_key.app_idx,
					ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
					RD_VENDOR_ID);
			if (err != ESP_OK)
			{
				LOGE("Provisioner bind local model appkey failed");
				return;
			}
		}
		break;
	case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
		break;
	case ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT:
		LOGI("ESP_BLE_MESH_PROVISIONER_STORE_NODE_COMP_DATA_COMP_EVT, err_code %d", param->provisioner_store_node_comp_data_comp.err_code);
		break;
	default:
		break;
	}
}
