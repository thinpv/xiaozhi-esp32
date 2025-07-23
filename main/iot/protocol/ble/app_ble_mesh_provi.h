#ifndef BLE_MESH_H_
#define BLE_MESH_H_
#include <stdio.h>
#include <string.h>

#include "esp_ble_mesh_defs.h"

#define DEVICE_SIG_MODEL_MAX 32
#define DEVICE_VENDOR_MODEL_MAX 16

typedef struct
{
	uint16_t company_id;
	uint16_t model_id;
	uint8_t is_binded;
} model_t;

typedef struct
{
	uint16_t loc;

	// reserve space for up to 20 SIG models
	model_t sig_models[DEVICE_SIG_MODEL_MAX];
	uint8_t nums;

	// reserve space for up to 4 vendor models
	model_t vendor_models[DEVICE_VENDOR_MODEL_MAX];
	uint8_t numv;
} elenment_t;

typedef struct
{
	uint8_t mac[BD_ADDR_LEN];
	uint8_t uuid[ESP_BLE_MESH_OCTET16_LEN];
	uint16_t primary_addr;
	uint8_t element_num;

	uint16_t cid;
	uint16_t pid;
	uint16_t vid;
	uint16_t crpl;
	uint16_t feat;
	elenment_t elements[6];

	time_t lastUpdateTime;
	uint32_t deviceType;
	uint16_t deviceVersion;
	bool isSetGWAddr;
} scan_device_t;

typedef struct esp_ble_mesh_key
{
	uint16_t net_idx;
	uint16_t app_idx;

	uint8_t net_key[ESP_BLE_MESH_OCTET16_LEN];
	uint8_t app_key[ESP_BLE_MESH_OCTET16_LEN];
} prov_key_t;

#ifdef __cplusplus
extern "C"
{
#endif

	/**************************************************************************/
	/**
	 * @brief This function initializes BLE Mesh Provisioning.
	 ******************************************************************************/
	void app_ble_mesh_provi_init(void);
	void app_ble_mesh_provi_init_key(uint16_t net_idx, uint8_t *net_key, uint16_t app_idx, uint8_t *app_key);

	void example_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
																			 esp_ble_mesh_node_t *node,
																			 esp_ble_mesh_model_t *model, uint32_t opcode);

	void ble_mesh_set_msg_common(uint16_t addr,
															 esp_ble_mesh_client_common_param_t *common,
															 esp_ble_mesh_model_t *model, uint32_t opcode);

#ifdef __cplusplus
}
#endif

#endif /* BLE_MESH_H_ */
