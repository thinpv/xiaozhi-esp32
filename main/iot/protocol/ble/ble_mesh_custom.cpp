
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"

#include "BleMeshDefine.h"
#include "app_ble_mesh_provi.h"
#include "ble_mesh_protocol.h"

#include "Log.h"
#include "esp_timer.h"

#include "DeviceManager.h"
#include "Device.h"
#include "Gateway.h"
#include "Database.h"
#include "Util.h"

extern scan_device_t scan_device;

static void AddDeviceThread(void *data)
{
	scan_device_t *scan_device_ptr = (scan_device_t *)data;
	if (scan_device_ptr)
	{
		esp_ble_mesh_node_t *node = esp_ble_mesh_provisioner_get_node_with_addr(scan_device_ptr->primary_addr);
		if (node)
		{
			string mac = Util::ConvertU32ToHexString(scan_device_ptr->mac, sizeof(scan_device_ptr->mac));
			string dev_key = Util::ConvertU32ToHexString(node->dev_key, sizeof(node->dev_key));
			LOGI("AddDeviceThread mac: %s, dev_key: %s", mac.c_str(), dev_key.c_str());
			Json::Value dataJson;
			dataJson["devicekey"] = dev_key;
			Device *device = DeviceManager::GetInstance()->AddDevice(mac, scan_device_ptr->deviceType, scan_device_ptr->primary_addr, scan_device_ptr->deviceVersion, &dataJson);
			if (device)
			{
				Json::Value onlineValue;
				onlineValue["stt"] = 1;
				Database::GetInstance()->DeviceAdd(device);
				if (Gateway::GetInstance()->DeviceAddNew(device) == CODE_OK)
				{
					device->PushTelemetry(onlineValue);
				}
			}
		}
		free(scan_device_ptr);
	}
	vTaskDelete(NULL);
}

extern "C" void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
																								 esp_ble_mesh_model_cb_param_t *param)
{
	LOGW("example_ble_mesh_custom_model_cb: %d", event);
	// static int64_t start_time;

	switch (event)
	{
	case ESP_BLE_MESH_MODEL_OPERATION_EVT:
		LOGI("0x%04X - 0x%04X", param->model_operation.opcode, ESP_BLE_MESH_VND_MODEL_OP_STATUS_E0);
		if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_STATUS_E0)
		{
			// int64_t end_time = esp_timer_get_time();
			// LOGI("Recv 0x06%" PRIx32 ", tid 0x%04x, time %lldus",
			// 		 param->model_operation.opcode, store.tid, end_time - start_time);

			uint16_t addr = param->model_operation.ctx->addr;

			typedef struct __attribute__((packed))
			{
				uint16_t header;
			} vendor_provision_t;
			vendor_provision_t *vendor_provision = (vendor_provision_t *)param->model_operation.msg;
			LOGW("vendor_provision 0x%04x", vendor_provision->header);
			if (vendor_provision->header == RD_HEADER_PROVISION_SET_GW_ADDR)
			{
				if (scan_device.primary_addr == addr)
				{
					esp_ble_get_device_type(scan_device.mac, scan_device.primary_addr);
				}
				else
				{
					LOGW("Another device...");
				}
			}
			else if (vendor_provision->header == RD_HEADER_PROVISION_GET_DEV_TYPE)
			{
				if (scan_device.primary_addr == addr)
				{
					scan_device.isSetGWAddr = true;
					typedef struct __attribute__((packed))
					{
						uint8_t header[2];
						uint8_t deviceType[3];
						uint8_t magic;
						uint8_t version[2];
					} check_type_rsp_message_t;
					check_type_rsp_message_t *check_type_rsp_message = (check_type_rsp_message_t *)param->model_operation.msg;
					scan_device.deviceType = (check_type_rsp_message->deviceType[0] << 16) | (check_type_rsp_message->deviceType[1] << 8) | check_type_rsp_message->deviceType[2];
					scan_device.deviceVersion = (check_type_rsp_message->version[0] << 8) | (check_type_rsp_message->version[1]);
					LOGI("deviceType: 0x%04X, deviceVersion: 0x%04X", scan_device.deviceType, scan_device.deviceVersion);
					scan_device_t *scan_device_ptr = (scan_device_t *)malloc(sizeof(scan_device_t));
					memcpy(scan_device_ptr, &scan_device, sizeof(scan_device_t));
					memset(&scan_device, 0, sizeof(scan_device));
					if (xTaskCreateWithCaps(AddDeviceThread, "AddDeviceThread", 10240, scan_device_ptr, 10, NULL, MALLOC_CAP_SPIRAM) != pdPASS)
					{
						LOGE("Failed to create task");
					}
				}
				else
				{
					LOGW("Another device...");
				}
			}
		}
		break;
	case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
		if (param->model_send_comp.err_code)
		{
			LOGE("Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
			break;
		}
		// start_time = esp_timer_get_time();
		LOGI("Send 0x%06" PRIx32, param->model_send_comp.opcode);
		break;
	case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
	{
		LOGI("Receive publish message 0x%06" PRIx32, param->client_recv_publish_msg.opcode);
		LOGI("ESP_BLE_MESH_VND_MODEL_OP_STATUS_E2 0x%06" PRIx32, ESP_BLE_MESH_VND_MODEL_OP_STATUS_E2);
		if (param->client_recv_publish_msg.opcode == ESP_BLE_MESH_VND_MODEL_OP_STATUS_E0)
		{
			ESP_LOG_BUFFER_HEX("msg", param->client_recv_publish_msg.msg, param->client_recv_publish_msg.length);
		}
		else if (param->client_recv_publish_msg.opcode == ESP_BLE_MESH_VND_MODEL_OP_STATUS_E2)
		{
			ESP_LOG_BUFFER_HEX("msg", param->client_recv_publish_msg.msg, param->client_recv_publish_msg.length);
		}
	}
	break;
	case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT:
		LOGW("Client message 0x%06" PRIx32 " timeout", param->client_send_timeout.opcode);
		example_ble_mesh_send_vendor_message(true);
		break;
	default:
		break;
	}
}