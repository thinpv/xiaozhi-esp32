/* main.c - Application main entry point */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_timer.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_sensor_model_api.h"
#include "esp_ble_mesh_lighting_model_api.h"
#include "esp_ble_mesh_time_scene_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"

#include "ble_mesh_example_init.h"
#include "ble_mesh_example_nvs.h"
#include "app_ble_mesh_provi.h"
#include "ble_mesh_protocol.h"

#include "BleMeshDefine.h"
#include "Log.h"

static uint8_t dev_uuid[ESP_BLE_MESH_OCTET16_LEN];

prov_key_t prov_key = {
		.net_idx = 0,
		.app_idx = 0,
};

// ==== 1. PUBLICATIONS ====
ESP_BLE_MESH_MODEL_PUB_DEFINE(time_cli_pub, 2 + 5, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(scene_cli_pub, 2 + 5, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(sched_cli_pub, 2 + 5, ROLE_NODE);

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(level_cli_pub, 2 + 5, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(ctl_cli_pub, 2 + 5, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(hsl_cli_pub, 2 + 5, ROLE_NODE);

ESP_BLE_MESH_MODEL_PUB_DEFINE(sensor_cli_pub, 2 + 5, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(battery_cli_pub, 2 + 1, ROLE_NODE);

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_srv_pub, 2 + 3, ROLE_NODE);
// ==== 2. CLIENT MODEL STRUCTS ====
esp_ble_mesh_client_t config_client;
esp_ble_mesh_client_t onoff_client;
esp_ble_mesh_client_t level_client;
esp_ble_mesh_client_t ctl_client;
esp_ble_mesh_client_t hsl_client;

esp_ble_mesh_client_t time_client;
esp_ble_mesh_client_t scene_client;
esp_ble_mesh_client_t sched_client;

esp_ble_mesh_client_t sensor_client;
esp_ble_mesh_client_t battery_client;

// ==== 3. CONFIG SERVER ====
static esp_ble_mesh_cfg_srv_t config_server = {
		.net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
		.relay = ESP_BLE_MESH_RELAY_DISABLED,
		.relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
		.beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
		.gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
		.gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_FRIEND)
		.friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
		.friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
		.default_ttl = 7,
};

// ==== 4. VENDOR MODEL ====
static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = {
		{ESP_BLE_MESH_VND_MODEL_OP_SEND_E0, ESP_BLE_MESH_VND_MODEL_OP_STATUS_E0},
		{ESP_BLE_MESH_VND_MODEL_OP_SEND_E2, ESP_BLE_MESH_VND_MODEL_OP_STATUS_E2},
};

esp_ble_mesh_client_t vendor_client = {
		.op_pair_size = ARRAY_SIZE(vnd_op_pair),
		.op_pair = vnd_op_pair,
};

static esp_ble_mesh_model_op_t vnd_op[] = {
		ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND_E0, 0),
		ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS_E0, 0),
		ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND_E2, 0),
		ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS_E2, 0),
		ESP_BLE_MESH_MODEL_OP_END,
};

// ==== 5. ELEMENTS ====
static esp_ble_mesh_gen_onoff_srv_t onoff_server = {
		.rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
		.rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
};

static esp_ble_mesh_model_t root_models[] = {
		ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
		ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),
		ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_srv_pub, &onoff_server),
		ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client),
		ESP_BLE_MESH_MODEL_LIGHT_LIGHTNESS_CLI(&level_cli_pub, &level_client),
		ESP_BLE_MESH_MODEL_LIGHT_CTL_CLI(&ctl_cli_pub, &ctl_client),
		ESP_BLE_MESH_MODEL_LIGHT_HSL_CLI(&hsl_cli_pub, &hsl_client),

		ESP_BLE_MESH_MODEL_TIME_CLI(&time_cli_pub, &time_client),
		ESP_BLE_MESH_MODEL_SCENE_CLI(&scene_cli_pub, &scene_client),
		ESP_BLE_MESH_MODEL_SCHEDULER_CLI(&sched_cli_pub, &sched_client),

		ESP_BLE_MESH_MODEL_SENSOR_CLI(&sensor_cli_pub, &sensor_client),
		ESP_BLE_MESH_MODEL_GEN_BATTERY_CLI(&battery_cli_pub, &battery_client),
};

static esp_ble_mesh_model_t vnd_models[] = {
		ESP_BLE_MESH_VENDOR_MODEL(RD_VENDOR_ID, ESP_BLE_MESH_VND_MODEL_ID_CLIENT, vnd_op, NULL, &vendor_client),
};

static esp_ble_mesh_elem_t elements[] = {
		ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
};

// ==== 6. COMPOSITION ====
static esp_ble_mesh_comp_t composition = {
		.cid = RD_VENDOR_ID,
		.element_count = ARRAY_SIZE(elements),
		.elements = elements,
};

// ==== 7. PROVISIONING ====
static esp_ble_mesh_prov_t provision = {
		.prov_uuid = dev_uuid,
		.prov_unicast_addr = PROV_OWN_ADDR,
		.prov_start_address = 0x0005,
		.prov_attention = 0x00,
		.prov_algorithm = 0x00,
		.prov_pub_key_oob = 0x00,
		.prov_static_oob_val = NULL,
		.prov_static_oob_len = 0x00,
		.flags = 0x00,
		.iv_index = 0x00,
};

scan_device_t scan_device = {0};

void example_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
																		 esp_ble_mesh_node_t *node,
																		 esp_ble_mesh_model_t *model,
																		 uint32_t opcode)
{
	common->opcode = opcode;
	common->model = model;
	common->ctx.net_idx = prov_key.net_idx;
	common->ctx.app_idx = prov_key.app_idx;
	common->ctx.addr = node->unicast_addr;
	common->ctx.send_ttl = MSG_SEND_TTL;
	common->msg_timeout = MSG_TIMEOUT;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 2, 0)
	common->msg_role = MSG_ROLE;
#endif
}

void ble_mesh_set_msg_common(uint16_t addr,
														 esp_ble_mesh_client_common_param_t *common,
														 esp_ble_mesh_model_t *model,
														 uint32_t opcode)
{
	common->opcode = opcode;
	common->model = model;
	common->ctx.net_idx = prov_key.net_idx;
	common->ctx.app_idx = prov_key.app_idx;
	common->ctx.addr = addr;
	common->ctx.send_ttl = MSG_SEND_TTL;
	common->msg_timeout = MSG_TIMEOUT;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 2, 0)
	common->msg_role = MSG_ROLE;
#endif
}

void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
																			esp_ble_mesh_prov_cb_param_t *param);
void example_ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
																			 esp_ble_mesh_cfg_client_cb_param_t *param);
void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
																			esp_ble_mesh_model_cb_param_t *param);
void example_ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
																				esp_ble_mesh_generic_client_cb_param_t *param);
void example_ble_mesh_lighting_client_cb(esp_ble_mesh_light_client_cb_event_t event,
																				 esp_ble_mesh_light_client_cb_param_t *param);
void example_ble_mesh_time_scene_client_cb(esp_ble_mesh_time_scene_client_cb_event_t event,
																					 esp_ble_mesh_time_scene_client_cb_param_t *param);
void example_ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
																			 esp_ble_mesh_sensor_client_cb_param_t *param);

static void example_ble_mesh_generic_server_cb(esp_ble_mesh_generic_server_cb_event_t event,
																							 esp_ble_mesh_generic_server_cb_param_t *param)
{
	esp_ble_mesh_gen_onoff_srv_t *srv;
	LOGW("example_ble_mesh_generic_server_cb event 0x%02x, opcode 0x%04" PRIx32 ", src 0x%04x, dst 0x%04x",
			 event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);
}

static esp_err_t ble_mesh_init(void)
{
	esp_err_t err = ESP_OK;

	esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
	esp_ble_mesh_register_config_client_callback(example_ble_mesh_config_client_cb);
	esp_ble_mesh_register_generic_client_callback(example_ble_mesh_generic_client_cb);
	esp_ble_mesh_register_light_client_callback(example_ble_mesh_lighting_client_cb);
	esp_ble_mesh_register_time_scene_client_callback(example_ble_mesh_time_scene_client_cb);
	esp_ble_mesh_register_sensor_client_callback(example_ble_mesh_sensor_client_cb);
	esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);
	esp_ble_mesh_register_generic_server_callback(example_ble_mesh_generic_server_cb);

	err = esp_ble_mesh_init(&provision, &composition);
	if (err != ESP_OK)
	{
		LOGE("Failed to initialize mesh stack (err %d)", err);
		return err;
	}

	err = esp_ble_mesh_client_model_init(&vnd_models[0]);
	if (err)
	{
		LOGE("Failed to initialize vendor client");
		return err;
	}

	err = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV);
	if (err != ESP_OK)
	{
		LOGE("Failed to enable mesh provisioner: %s", esp_err_to_name(err));
		return err;
	}

	LOGI("ESP BLE Mesh Provisioner initialized");

	return ESP_OK;
}

/**************************************************************************/
/**
 * Initializes BLE Mesh Provisioning.
 ******************************************************************************/
void app_ble_mesh_provi_init(void)
{
	esp_err_t err;
	LOGI("Initializing...");
	err = bluetooth_init();
	if (err != ESP_OK)
	{
		LOGE("esp32_bluetooth_init failed (err %d)", err);
		return;
	}

	ble_mesh_get_dev_uuid(dev_uuid);

	/* Initialize the Bluetooth Mesh Subsystem */
	err = ble_mesh_init();
	if (err != ESP_OK)
	{
		LOGE("Bluetooth mesh init failed (err %d)", err);
	}
}

void app_ble_mesh_provi_init_key(uint16_t net_idx, uint8_t *net_key, uint16_t app_idx, uint8_t *app_key)
{
	esp_err_t err;
	if (!net_key || !app_key)
		return;

	prov_key.net_idx = ESP_BLE_MESH_KEY_PRIMARY;
	prov_key.app_idx = APP_KEY_IDX;
	memcpy(prov_key.net_key, net_key, ESP_BLE_MESH_OCTET16_LEN);
	memcpy(prov_key.app_key, app_key, ESP_BLE_MESH_OCTET16_LEN);

	const uint8_t *net_key_tem = esp_ble_mesh_provisioner_get_local_net_key(net_idx);
	if (net_key_tem)
	{
		if (memcmp(net_key, net_key_tem, ESP_BLE_MESH_OCTET16_LEN))
		{
			LOGI("Update old Net Key %s", bt_hex(net_key_tem, ESP_BLE_MESH_OCTET16_LEN));
			err = esp_ble_mesh_provisioner_update_local_net_key(net_key, net_idx);
			if (err != ESP_OK)
			{
				LOGE("Failed to update local NetKey: %s", esp_err_to_name(err));
			}
		}
	}

	// LOGI("Add App Key %s", bt_hex(app_key, ESP_BLE_MESH_OCTET16_LEN));
	// err = esp_ble_mesh_provisioner_add_local_app_key(app_key, net_idx, app_idx);
	// if (err != ESP_OK)
	// {
	// 	LOGE("Failed to add local AppKey: %s", esp_err_to_name(err));
	// }

	const uint8_t *app_key_tem = esp_ble_mesh_provisioner_get_local_app_key(net_idx, app_idx);
	if (app_key_tem)
	{
		bool isUpdate = false;
		for (int i = 0; i < ESP_BLE_MESH_OCTET16_LEN; i++)
		{
			if (app_key[i] != app_key_tem[i])
			{
				LOGI("Update App Key %s to %s", bt_hex(app_key_tem, ESP_BLE_MESH_OCTET16_LEN), bt_hex(app_key, ESP_BLE_MESH_OCTET16_LEN));
				isUpdate = true;
				err = esp_ble_mesh_provisioner_update_local_app_key(app_key, net_idx, app_idx);
				if (err != ESP_OK)
				{
					LOGE("Failed to update local AppKey: %s", esp_err_to_name(err));
				}
				break;
			}
		}

		if (!isUpdate)
		{
			LOGI("App Key not changed, skip update");
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_HSL_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_SCHEDULER_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_SCENE_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_TIME_CLI,
					ESP_BLE_MESH_CID_NVAL);
			// Sensor
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_SENSOR_CLI,
					ESP_BLE_MESH_CID_NVAL);
			// Battery
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_MODEL_ID_GEN_BATTERY_CLI,
					ESP_BLE_MESH_CID_NVAL);
			esp_ble_mesh_provisioner_bind_app_key_to_local_model(
					PROV_OWN_ADDR, app_idx,
					ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
					RD_VENDOR_ID);
		}
	}
	else
	{
		LOGI("Add App Key %s", bt_hex(app_key, ESP_BLE_MESH_OCTET16_LEN));
		err = esp_ble_mesh_provisioner_add_local_app_key(app_key, net_idx, app_idx);
		if (err != ESP_OK)
		{
			LOGE("Failed to add local AppKey: %s", esp_err_to_name(err));
		}
	}
}