
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_sensor_model_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"

#include "Log.h"
#include "esp_timer.h"

#include "DeviceManager.h"

extern "C" void example_ble_mesh_sensor_client_cb(esp_ble_mesh_sensor_client_cb_event_t event,
																									esp_ble_mesh_sensor_client_cb_param_t *param)
{
	uint32_t opcode = param->params->opcode;
	uint16_t addr = param->params->ctx.addr;

	LOGI("%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%08x",
			 __func__, param->error_code, event, addr, opcode);

	if (param->error_code)
	{
		LOGE("Send generic client message failed, opcode 0x%04x", opcode);
		return;
	}

	DeviceBle *deviceBle = DeviceManager::GetInstance()->GetDeviceBleFromAddr(addr);
	if (!deviceBle)
	{
		LOGE("%s: Get deviceBle failed", __func__);
		return;
	}

	LOGI("Sensor client, event %u, addr 0x%04x", event, param->params->ctx.addr);
	switch (event)
	{
	case ESP_BLE_MESH_SENSOR_CLIENT_GET_STATE_EVT:
		switch (param->params->opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_SENSOR_DESCRIPTOR_GET:
			LOGI("Sensor Descriptor Status, opcode 0x%04" PRIx32, param->params->ctx.recv_op);
			if (param->status_cb.descriptor_status.descriptor->len != ESP_BLE_MESH_SENSOR_SETTING_PROPERTY_ID_LEN &&
					param->status_cb.descriptor_status.descriptor->len % ESP_BLE_MESH_SENSOR_DESCRIPTOR_LEN)
			{
				LOGE("Invalid Sensor Descriptor Status length %d", param->status_cb.descriptor_status.descriptor->len);
				return;
			}
			if (param->status_cb.descriptor_status.descriptor->len)
			{
				ESP_LOG_BUFFER_HEX("Sensor Descriptor", param->status_cb.descriptor_status.descriptor->data,
													 param->status_cb.descriptor_status.descriptor->len);
				/* If running with sensor server example, sensor client can get two Sensor Property IDs.
				 * Currently we use the first Sensor Property ID for the following demonstration.
				 */
				// sensor_prop_id = param->status_cb.descriptor_status.descriptor->data[1] << 8 |
				// 								 param->status_cb.descriptor_status.descriptor->data[0];
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_GET:
			LOGI("Sensor Cadence Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.cadence_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Cadence", param->status_cb.cadence_status.sensor_cadence_value->data,
												 param->status_cb.cadence_status.sensor_cadence_value->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTINGS_GET:
			LOGI("Sensor Settings Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.settings_status.sensor_property_id);
			ESP_LOG_BUFFER_HEX("Sensor Settings", param->status_cb.settings_status.sensor_setting_property_ids->data,
												 param->status_cb.settings_status.sensor_setting_property_ids->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_GET:
			LOGI("Sensor Setting Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x, Sensor Setting Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.setting_status.sensor_property_id,
					 param->status_cb.setting_status.sensor_setting_property_id);
			if (param->status_cb.setting_status.op_en)
			{
				LOGI("Sensor Setting Access 0x%02x", param->status_cb.setting_status.sensor_setting_access);
				ESP_LOG_BUFFER_HEX("Sensor Setting Raw", param->status_cb.setting_status.sensor_setting_raw->data,
													 param->status_cb.setting_status.sensor_setting_raw->len);
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_GET:
			LOGI("Sensor Status, opcode 0x%04" PRIx32, param->params->ctx.recv_op);
			if (param->status_cb.sensor_status.marshalled_sensor_data->len)
			{
				ESP_LOG_BUFFER_HEX("Sensor Data", param->status_cb.sensor_status.marshalled_sensor_data->data,
													 param->status_cb.sensor_status.marshalled_sensor_data->len);
				uint8_t *data = param->status_cb.sensor_status.marshalled_sensor_data->data;
				uint16_t length = 0;
				for (; length < param->status_cb.sensor_status.marshalled_sensor_data->len;)
				{
					uint8_t fmt = ESP_BLE_MESH_GET_SENSOR_DATA_FORMAT(data);
					uint8_t data_len = ESP_BLE_MESH_GET_SENSOR_DATA_LENGTH(data, fmt);
					uint16_t prop_id = ESP_BLE_MESH_GET_SENSOR_DATA_PROPERTY_ID(data, fmt);
					uint8_t mpid_len = (fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? ESP_BLE_MESH_SENSOR_DATA_FORMAT_A_MPID_LEN : ESP_BLE_MESH_SENSOR_DATA_FORMAT_B_MPID_LEN);
					LOGI("Format %s, length 0x%02x, Sensor Property ID 0x%04x",
							 fmt == ESP_BLE_MESH_SENSOR_DATA_FORMAT_A ? "A" : "B", data_len, prop_id);
					if (data_len != ESP_BLE_MESH_SENSOR_DATA_ZERO_LEN)
					{
						ESP_LOG_BUFFER_HEX("Sensor Data", data + mpid_len, data_len + 1);
						length += mpid_len + data_len + 1;
						data += mpid_len + data_len + 1;
					}
					else
					{
						length += mpid_len;
						data += mpid_len;
					}
				}
			}
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_COLUMN_GET:
			LOGI("Sensor Column Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.column_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Column", param->status_cb.column_status.sensor_column_value->data,
												 param->status_cb.column_status.sensor_column_value->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SERIES_GET:
			LOGI("Sensor Series Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.series_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Series", param->status_cb.series_status.sensor_series_value->data,
												 param->status_cb.series_status.sensor_series_value->len);
			break;
		default:
			LOGE("Unknown Sensor Get opcode 0x%04" PRIx32, param->params->ctx.recv_op);
			break;
		}
		break;
	case ESP_BLE_MESH_SENSOR_CLIENT_SET_STATE_EVT:
		switch (param->params->opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_SENSOR_CADENCE_SET:
			LOGI("Sensor Cadence Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.cadence_status.property_id);
			ESP_LOG_BUFFER_HEX("Sensor Cadence", param->status_cb.cadence_status.sensor_cadence_value->data,
												 param->status_cb.cadence_status.sensor_cadence_value->len);
			break;
		case ESP_BLE_MESH_MODEL_OP_SENSOR_SETTING_SET:
			LOGI("Sensor Setting Status, opcode 0x%04" PRIx32 ", Sensor Property ID 0x%04x, Sensor Setting Property ID 0x%04x",
					 param->params->ctx.recv_op, param->status_cb.setting_status.sensor_property_id,
					 param->status_cb.setting_status.sensor_setting_property_id);
			if (param->status_cb.setting_status.op_en)
			{
				LOGI("Sensor Setting Access 0x%02x", param->status_cb.setting_status.sensor_setting_access);
				ESP_LOG_BUFFER_HEX("Sensor Setting Raw", param->status_cb.setting_status.sensor_setting_raw->data,
													 param->status_cb.setting_status.sensor_setting_raw->len);
			}
			break;
		default:
			LOGE("Unknown Sensor Set opcode 0x%04" PRIx32, param->params->ctx.recv_op);
			break;
		}
		break;
	case ESP_BLE_MESH_SENSOR_CLIENT_PUBLISH_EVT:
		switch (opcode)
		{
		case ESP_BLE_MESH_MODEL_OP_SENSOR_STATUS:
		{
			uint8_t *data = param->status_cb.sensor_status.marshalled_sensor_data->data;
			uint16_t len = param->status_cb.sensor_status.marshalled_sensor_data->len;
			if (len < 31)
			{
				struct __attribute__((packed))
				{
					uint8_t vendor_opcode;
					uint8_t data[31];
				} data_message = {
						.vendor_opcode = (uint8_t)opcode};
				memcpy(data_message.data, data, len);
				deviceBle->InputData((uint8_t *)&data_message, len + 1, addr);
			}
		}
		break;
		default:
			break;
		}

		break;
	case ESP_BLE_MESH_SENSOR_CLIENT_TIMEOUT_EVT:
		// example_ble_mesh_sensor_timeout(param->params->opcode);
	default:
		break;
	}
}