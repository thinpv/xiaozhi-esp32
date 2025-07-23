#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_networking_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"
#include "ble_mesh_protocol.h"

#include "Log.h"
#include "esp_timer.h"
#include "DeviceManager.h"
#include "GroupManager.h"
#include "Database.h"

extern esp_ble_mesh_client_t config_client;
extern prov_key_t prov_key;
extern scan_device_t scan_device;
static model_t *model_binding = NULL;
extern bool isProvisioning;
static int element_addr_binding = 0;

static void print_device()
{
	LOGI("********************** Print Device Info Start **********************");
	LOGI("primary_addr address: 0x%02x, element num: %d", scan_device.primary_addr, scan_device.element_num);
	LOGI("device uuid: %s, mac: %s", bt_hex(scan_device.uuid, 16), bt_hex(scan_device.mac, BD_ADDR_LEN));
	LOGI("* CID 0x%04x, PID 0x%04x, VID 0x%04x, CRPL 0x%04x, Features 0x%04x *", scan_device.cid, scan_device.pid, scan_device.vid, scan_device.crpl, scan_device.feat);
	for (int i = 0; i < scan_device.element_num; i++)
	{
		LOGI("* Loc 0x%04x, NumS 0x%02x, NumV 0x%02x *", scan_device.elements[i].loc, scan_device.elements[i].nums, scan_device.elements[i].numv);
		for (int j = 0; j < scan_device.elements[i].nums; j++)
		{
			LOGI("* SIG Model ID 0x%04x *", scan_device.elements[i].sig_models[j].model_id);
		}
		for (int j = 0; j < scan_device.elements[i].numv; j++)
		{
			LOGI("* Vendor Model ID 0x%04x, Company ID 0x%04x *", scan_device.elements[i].vendor_models[j].model_id, scan_device.elements[i].vendor_models[j].company_id);
		}
	}
	LOGI("*********************** Print Device Info End ***********************");
}

static void example_ble_mesh_parse_node_comp_data(const uint8_t *data, uint16_t length)
{
	uint16_t offset;
	int i = 0, j;

	scan_device.cid = COMP_DATA_2_OCTET(data, 0);
	scan_device.pid = COMP_DATA_2_OCTET(data, 2);
	scan_device.vid = COMP_DATA_2_OCTET(data, 4);
	scan_device.crpl = COMP_DATA_2_OCTET(data, 6);
	scan_device.feat = COMP_DATA_2_OCTET(data, 8);
	offset = 10;

	for (; offset < length;)
	{
		scan_device.elements[i].loc = COMP_DATA_2_OCTET(data, offset);
		scan_device.elements[i].nums = COMP_DATA_1_OCTET(data, offset + 2);
		scan_device.elements[i].numv = COMP_DATA_1_OCTET(data, offset + 3);
		offset += 4;
		for (j = 0; j < scan_device.elements[i].nums; j++)
		{
			scan_device.elements[i].sig_models[j].company_id = ESP_BLE_MESH_CID_NVAL;
			scan_device.elements[i].sig_models[j].model_id = COMP_DATA_2_OCTET(data, offset);
			offset += 2;
		}
		for (j = 0; j < scan_device.elements[i].numv; j++)
		{
			scan_device.elements[i].vendor_models[j].company_id = COMP_DATA_2_OCTET(data, offset);
			scan_device.elements[i].vendor_models[j].model_id = COMP_DATA_2_OCTET(data, offset + 2);
			offset += 4;
		}
		i++;
	}
}

/**
 * @brief binding all
 *
 * @return true binding all done
 * @return false waiting binding callback
 */
static bool binding_all(esp_ble_mesh_node_t *node)
{
	int err;
	for (int i = 0; i < scan_device.element_num; i++)
	{
		for (int j = 0; j < scan_device.elements[i].nums; j++)
		{
			if (!scan_device.elements[i].sig_models[j].is_binded)
			{
				model_binding = &(scan_device.elements[i].sig_models[j]);
				element_addr_binding = scan_device.primary_addr + i;
				esp_ble_mesh_client_common_param_t common = {0};
				esp_ble_mesh_cfg_client_set_state_t set_state = {0};
				scan_device.elements[i].sig_models[j].is_binded = 1;
				LOGI("Binding Element %d SIG Model Id 0x%04x Model ID 0x%04x",
						 i,
						 scan_device.elements[i].sig_models[j].company_id,
						 scan_device.elements[i].sig_models[j].model_id);
				example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
				set_state.model_app_bind.element_addr = scan_device.primary_addr + i;
				set_state.model_app_bind.model_app_idx = prov_key.app_idx;
				set_state.model_app_bind.model_id = scan_device.elements[i].sig_models[j].model_id;
				set_state.model_app_bind.company_id = scan_device.elements[i].sig_models[j].company_id;
				err = esp_ble_mesh_config_client_set_state(&common, &set_state);
				if (err)
				{
					LOGE("%s: Config Model App Bind failed", __func__);
				}
				return false;
			}
		}
		for (int j = 0; j < scan_device.elements[i].numv; j++)
		{
			if (!scan_device.elements[i].vendor_models[j].is_binded)
			{
				model_binding = &(scan_device.elements[i].vendor_models[j]);
				element_addr_binding = scan_device.primary_addr + i;
				esp_ble_mesh_client_common_param_t common = {0};
				esp_ble_mesh_cfg_client_set_state_t set_state = {0};
				scan_device.elements[i].vendor_models[j].is_binded = 1;
				LOGI("Binding Element %d Vendor Model Id 0x%04x Model ID 0x%04x",
						 i,
						 scan_device.elements[i].vendor_models[j].company_id,
						 scan_device.elements[i].vendor_models[j].model_id);
				example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
				set_state.model_app_bind.element_addr = scan_device.primary_addr + i;
				set_state.model_app_bind.model_app_idx = prov_key.app_idx;
				set_state.model_app_bind.model_id = scan_device.elements[i].vendor_models[j].model_id;
				set_state.model_app_bind.company_id = scan_device.elements[i].vendor_models[j].company_id;
				err = esp_ble_mesh_config_client_set_state(&common, &set_state);
				if (err)
				{
					LOGE("%s: Config Model App Bind failed", __func__);
				}
				return false;
			}
		}
	}
	return true;
}

extern "C" void example_ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
																									esp_ble_mesh_cfg_client_cb_param_t *param)
{
	esp_ble_mesh_client_common_param_t common = {0};
	esp_ble_mesh_cfg_client_set_state_t set = {0};
	esp_ble_mesh_node_t *node = NULL;
	esp_err_t err;

	LOGI("Config client, err_code %d, event %u, addr 0x%04x, opcode 0x%04" PRIx32,
			 param->error_code, event, param->params->ctx.addr, param->params->opcode);

	if (param->error_code)
	{
		LOGE("Send config client message failed, opcode 0x%04" PRIx32, param->params->opcode);
		return;
	}

	node = esp_ble_mesh_provisioner_get_node_with_addr(param->params->ctx.addr);
	if (!node)
	{
		LOGE("Failed to get node 0x%04x info", param->params->ctx.addr);
		return;
	}

	switch (event)
	{
	case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
		if (param->params->ctx.addr == scan_device.primary_addr)
		{
			if (!isProvisioning)
				return;

			scan_device.lastUpdateTime = time(NULL);
			if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET)
			{
				ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data,
													 param->status_cb.comp_data_status.composition_data->len);
				example_ble_mesh_parse_node_comp_data(param->status_cb.comp_data_status.composition_data->data,
																							param->status_cb.comp_data_status.composition_data->len);
				print_device();
				err = esp_ble_mesh_provisioner_store_node_comp_data(param->params->ctx.addr,
																														param->status_cb.comp_data_status.composition_data->data,
																														param->status_cb.comp_data_status.composition_data->len);
				if (err != ESP_OK)
				{
					LOGE("Failed to store node composition data");
					break;
				}

				example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
				set.app_key_add.net_idx = prov_key.net_idx;
				set.app_key_add.app_idx = prov_key.app_idx;
				memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
				err = esp_ble_mesh_config_client_set_state(&common, &set);
				if (err != ESP_OK)
				{
					LOGE("Failed to send Config AppKey Add");
				}
			}
		}
		break;
	case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
		if (param->params->ctx.addr == scan_device.primary_addr)
		{
			if (!isProvisioning)
				return;

			scan_device.lastUpdateTime = time(NULL);
			if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD)
			{
				LOGI("ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
				if (binding_all(node))
				{
					LOGI("Binding all done");
				}
				break;
			}
			else if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND)
			{
				LOGI("ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND: %d - %d", scan_device.primary_addr, scan_device.element_num);
				if (binding_all(node))
				{
					LOGI("Binding all done");
					esp_ble_set_gw_addr(scan_device.primary_addr);
				}
			}
		}
		else
		{
			if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD)
			{
				LOGI("Model Subscription Add done, model id: 0x%04x, element addr: 0x%04x, sub addr: 0x%04x",
						 param->status_cb.model_sub_status.model_id,
						 param->status_cb.model_sub_status.element_addr,
						 param->status_cb.model_sub_status.sub_addr);
				DeviceBle *device = DeviceManager::GetInstance()->GetDeviceBleFromAddr(param->status_cb.model_sub_status.element_addr);
				if (device)
				{
					device->OnAddToGroup(param->status_cb.model_sub_status.element_addr - device->getAddr(),
															 param->status_cb.model_sub_status.sub_addr - BLE_GROUP_OFFSET,
															 ESP_OK);
				}
			}
			else if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_MODEL_SUB_DELETE)
			{
				LOGI("Model Subscription Del done, model id: 0x%04x, element addr: 0x%04x, sub addr: 0x%04x",
						 param->status_cb.model_sub_status.model_id,
						 param->status_cb.model_sub_status.element_addr,
						 param->status_cb.model_sub_status.sub_addr);
				DeviceBle *device = DeviceManager::GetInstance()->GetDeviceBleFromAddr(param->status_cb.model_sub_status.element_addr);
				if (device)
				{
					device->OnRemoveFromGroup(param->status_cb.model_sub_status.element_addr - device->getAddr(),
																		param->status_cb.model_sub_status.sub_addr - BLE_GROUP_OFFSET,
																		ESP_OK);
				}
			}
		}
		break;
	case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
		if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS)
		{
			ESP_LOG_BUFFER_HEX("Composition data", param->status_cb.comp_data_status.composition_data->data,
												 param->status_cb.comp_data_status.composition_data->len);
		}
		break;
	case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
		switch (param->params->opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET:
		{
			esp_ble_mesh_cfg_client_get_state_t get = {0};
			example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
			get.comp_data_get.page = COMP_DATA_PAGE_0;
			err = esp_ble_mesh_config_client_get_state(&common, &get);
			if (err != ESP_OK)
			{
				LOGE("Failed to send Config Composition Data Get");
			}
			break;
		}
		case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
			example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
			set.app_key_add.net_idx = prov_key.net_idx;
			set.app_key_add.app_idx = prov_key.app_idx;
			memcpy(set.app_key_add.app_key, prov_key.app_key, ESP_BLE_MESH_OCTET16_LEN);
			err = esp_ble_mesh_config_client_set_state(&common, &set);
			if (err != ESP_OK)
			{
				LOGE("Failed to send Config AppKey Add");
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
			if (model_binding)
			{
				example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
				set.model_app_bind.element_addr = element_addr_binding;
				set.model_app_bind.model_app_idx = prov_key.app_idx;
				set.model_app_bind.model_id = model_binding->model_id;
				set.model_app_bind.company_id = model_binding->company_id;
				err = esp_ble_mesh_config_client_set_state(&common, &set);
				if (err != ESP_OK)
				{
					LOGE("Failed to send Config Model App Bind");
				}
			}
			break;
		default:
			break;
		}
		break;
	default:
		LOGE("Invalid config client event %u", event);
		break;
	}
}