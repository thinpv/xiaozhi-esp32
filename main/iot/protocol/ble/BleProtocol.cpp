#include "BleProtocol.h"
#include <stdlib.h>
#include <thread>
#include <functional>
#include <byteswap.h>
#include "Log.h"
#include "Util.h"
#include <string.h>
#include <algorithm>
#include "Database.h"
#include "BleMeshDefine.h"
#include "DeviceBle.h"
#include "DeviceBleSwitchScene6ACRgb.h"
#include "DeviceBleSeftPowerRemote.h"
#include "AES.h"
#ifdef ESP_PLATFORM
#include "Led.h"
#endif

#include "DeviceManager.h"

#include "app_ble_mesh_provi.h"
#include "esp_ble_mesh_generic_model_api.h"

static uint8_t keyAes[] = {0x44, 0x69, 0x67, 0x69, 0x74, 0x61, 0x6c, 0x40, 0x32, 0x38, 0x31, 0x31, 0x32, 0x38, 0x30, 0x34};
static uint8_t plaintext[] = {0x24, 0x02, 0x28, 0x04, 0x28, 0x11, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t appKeyDefault[] = {0x60, 0x96, 0x47, 0x71, 0x73, 0x4f, 0xbd, 0x76, 0xe3, 0xb4, 0x05, 0x19, 0xd1, 0xd9, 0x4a, 0x48};

extern esp_ble_mesh_client_t config_client;
extern esp_ble_mesh_client_t onoff_client;
extern esp_ble_mesh_client_t level_client;
extern esp_ble_mesh_client_t ctl_client;
extern esp_ble_mesh_client_t hsl_client;

extern esp_ble_mesh_client_t time_client;
extern esp_ble_mesh_client_t scene_client;
extern esp_ble_mesh_client_t sched_client;

extern esp_ble_mesh_client_t sensor_client;
extern esp_ble_mesh_client_t battery_client;

extern bool isProvisioning;

static BleProtocol *bleProtocol = NULL;

BleProtocol *BleProtocol::GetInstance()
{
	if (!bleProtocol)
	{
		bleProtocol = new BleProtocol();
	}
	return bleProtocol;
}

BleProtocol::BleProtocol()
{
	haveNewMac = false;
	haveGetMacRsp = true;
	isInitKey = false;
	tid = 0;
}

BleProtocol::~BleProtocol()
{
}

void BleProtocol::init()
{
	app_ble_mesh_provi_init();
}

void BleProtocol::initKey(string netKeyStr, string appKeyStr)
{
	LOGI("netKey: %s, appKey: %s", netKeyStr.c_str(), appKeyStr.c_str());
	esp_err_t err;
	uint8_t netKey[16];
	uint8_t appKey[16];

	if (netKeyStr.size() == 32)
	{
		for (int i = 0; i < netKeyStr.length(); i += 2)
		{
			std::string hexByte = netKeyStr.substr(i, 2);
			netKey[i / 2] = std::stoi(hexByte, nullptr, 16);
		}
	}

	if (appKeyStr.size() == 32)
	{
		for (int i = 0; i < appKeyStr.length(); i += 2)
		{
			std::string hexByte = appKeyStr.substr(i, 2);
			appKey[i / 2] = std::stoi(hexByte, nullptr, 16);
		}
	}

	app_ble_mesh_provi_init_key(ESP_BLE_MESH_KEY_PRIMARY, netKey, APP_KEY_IDX, appKey);
}

int BleProtocol::SendMessage(uint16_t opReq, uint8_t *dataReq, int lenReq, uint8_t opRsp, uint8_t *dataRsp, int *lenRsp, uint32_t timeout, uint8_t *compare_data, int compare_position, int compare_len)
{
	int rs = CODE_OK;
	return rs;
}

int BleProtocol::StartScan()
{
	LOGD("StartScan BLE");
	isProvisioning = true;
	return CODE_OK;
}

int BleProtocol::StopScan()
{
	LOGD("StopScan");
	isProvisioning = false;
	return CODE_OK;
}

bool BleProtocol::IsProvision()
{
	return isProvisioning;
}

void BleProtocol::SetProvisioning(bool isProvision)
{
	// isProvisioning = isProvision;
}

int BleProtocol::ResetBle()
{
	LOGD("ResetBle");
	uint8_t d = HCI_GATEWAY_CMD_RESET;
	int rs = SendMessage(SYSTEM_REQ, &d, 1, 0, 0, 0, 8000);
	if (rs)
	{
		LOGE("Send reset factory error, rs: %d", rs);
	}
	return rs;
}

int BleProtocol::ResetFactory()
{
	LOGD("ResetFactory");
	initKey(Gateway::GetInstance()->getBleNetKey(), Gateway::GetInstance()->getBleAppKey());
	Database::GetInstance()->GatewayRead();
	return CODE_OK;
}

/**
 * @brief Gen Ble security key (6 bytes)
 *
 * @param mac mac of destination device
 * @param devAddr unicast addr of destination device
 * @param out out buffer to write key
 */
extern "C" void genSecurityKey(uint8_t *mac, uint16_t devAddr, uint8_t *out)
{
	AES aes(AESKeyLength::AES_128);
	memcpy(plaintext + 8, mac, 6);
	memcpy(plaintext + 14, (uint8_t *)&devAddr, 2);
	unsigned char *outAes = aes.EncryptECB(plaintext, 32, keyAes);
	for (int i = 0; i < 6; i++)
	{
		out[i] = outAes[i + 10];
	}
	free(outAes);
}

int BleProtocol::ResetDev(uint16_t devAddr)
{
	LOGD("Reset dev addr: 0x%04X", devAddr);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint16_t opcode;
	} reset_message_t;
	reset_message_t reset_message = {0};
	memset(&reset_message, 0x00, sizeof(reset_message));
	uint8_t resetHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0x80, 0x4a};
	reset_message.ble_message_header.devAddr = devAddr;
	reset_message.opcode = NODE_RESET;
	int rs = SendMessage(APP_REQ, (uint8_t *)&reset_message, sizeof(reset_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, resetHeader, 0, 6);
	if (rs == CODE_OK)
	{
		return CODE_OK;
	}
	return CODE_ERROR;
}

int BleProtocol::ResetDelAll()
{
	LOGD("Reset all dev addr");
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint8_t data[6];
	} reset_message_t;
	reset_message_t reset_message = {0};
	memset(&reset_message, 0x00, sizeof(reset_message));
	reset_message.ble_message_header.devAddr = 0xFFFF;
	reset_message.opcodeVendor = RD_OPCODE_PROVISION;
	reset_message.vendorId = RD_VENDOR_ID;
	reset_message.opcodeRsp = RD_OPCODE_PROVISION_RSP;
	reset_message.header = 0xFFFF;
	for (int i = 0; i < 6; i++)
	{
		reset_message.data[i] = i + 1;
	}

	int rs = SendMessage(APP_REQ, (uint8_t *)&reset_message, sizeof(reset_message_t), 0, 0, 0, 1000);
	if (rs == CODE_OK)
	{
		return CODE_OK;
	}
	return CODE_ERROR;
}

int BleProtocol::GetTTL(uint16_t devAddr)
{
	LOGD("GetTTL");
	uint8_t dataRsp[100];
	int lenRsp = 0;
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint16_t opcode;
	} ttl_message_t;
	ttl_message_t ttl_message = {0};
	memset(&ttl_message, 0x00, sizeof(ttl_message));
	uint8_t getOnOffHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0x80, 0x0e};
	ttl_message.ble_message_header.devAddr = devAddr;
	ttl_message.opcode = CFG_DEFAULT_TTL_GET;
	int rs = SendMessage(APP_REQ, (uint8_t *)&ttl_message, sizeof(ttl_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 800, getOnOffHeader, 0, 6);
	if (rs == CODE_OK)
	{
		typedef struct __attribute__((packed))
		{
			uint16_t devAddr;
			uint16_t gwAddr;
			uint16_t opcode;
			uint8_t data[1];
		} ttl_rsp_message_t;
		if (lenRsp >= (int)sizeof(ttl_rsp_message_t))
		{
			ttl_rsp_message_t *ttl_rsp_message = (ttl_rsp_message_t *)dataRsp;
			if (ttl_rsp_message->opcode == CFG_DEFAULT_TTL_STATUS)
			{
				return CODE_OK;
			}
		}
		else
		{
			LOGW("GetTTL: response too short (%d bytes)", lenRsp);
		}
	}
	LOGW("Get TTL error");
	return CODE_ERROR;
}

int BleProtocol::SendOnlineCheck(uint16_t devAddr, uint32_t typeDev, uint16_t version)
{
	// LOGV("SendOnlineCheck addr: 0x%04X", devAddr);
	// #ifndef CONFIG_SAVE_ATTRIBUTE
	switch (typeDev)
	{
	case BLE_LED_CHIEU_TRANH:
	case BLE_LED_CHIEU_GUONG:
	case BLE_DEN_BAN:
	case BLE_DOWNLIGHT_SMT:
	case BLE_DOWNLIGHT_COB_GOC_HEP:
	case BLE_DOWNLIGHT_COB_GOC_RONG:
	case BLE_DOWNLIGHT_COB_TRANG_TRI:
	case BLE_LED_FLOOD:
	case BLE_LED_AT39:
	case BLE_LED_AT40:
	case BLE_LED_AT41:
	case BLE_LED_DAY_LINEAR:
	case BLE_LED_OP_TRAN:
	case BLE_LED_OP_TUONG:
	case BLE_LED_OP_TRAN_LOA:
	case BLE_PANEL_TRON:
	case BLE_PANEL_VUONG:
	case BLE_TRACKLIGHT:
	case BLE_LED_THA_TRAN:
	case BLE_LED_TUBE_M16:
	case BLE_LED_RLT03_06W:
	case BLE_LED_RLT02_10W:
	case BLE_LED_RLT02_20W:
	case BLE_LED_RLT01_10W:
	case BLE_LED_TRL08_20W:
	case BLE_LED_TRL08_10W:
	case BLE_LED_RLT03_12W:
	case BLE_DOWNLIGHT_RGBCW:
	case BLE_LED_DAY_RGBCW:
	case BLE_LED_BULB:
	case BLE_LED_DAY_RGB:
		if (version > 256)
			BleProtocol::UpdateLights(devAddr);
		else
			BleProtocol::GetOnOffLight(devAddr);
		break;
	case BLE_SWITCH_ONOFF:
	case BLE_SWITCH_ONOFF_V2:
		BleProtocol::GetOnOffLight(devAddr);
		break;
	case BLE_SWITCH_RGB_1:
	case BLE_SWITCH_RGB_1_SQUARE:
	case BLE_SWITCH_RGB_WATER_HEATER:
	case BLE_SWITCH_RGB_2:
	case BLE_SWITCH_RGB_2_SQUARE:
	case BLE_SWITCH_RGB_3:
	case BLE_SWITCH_RGB_3_SQUARE:
	case BLE_SWITCH_RGB_4:
	case BLE_SWITCH_RGB_4_SQUARE:
	case BLE_SWITCH_RGB_1_V2:
	case BLE_SWITCH_RGB_2_V2:
	case BLE_SWITCH_RGB_3_V2:
	case BLE_SWITCH_RGB_4_V2:
	case BLE_SWITCH_RGB_1_SQUARE_V2:
	case BLE_SWITCH_RGB_2_SQUARE_V2:
	case BLE_SWITCH_RGB_3_SQUARE_V2:
	case BLE_SWITCH_RGB_4_SQUARE_V2:
	case BLE_SWITCH_ELECTRICAL_1:
	case BLE_SWITCH_ELECTRICAL_2:
	case BLE_SWITCH_ELECTRICAL_3:
	case BLE_SWITCH_ELECTRICAL_4:
	case BLE_SWITCH_ELECTRICAL_WATER_HEATER:
	case BLE_SWITCH_ELECTRICAL_1_V2:
	case BLE_SWITCH_ELECTRICAL_2_V2:
	case BLE_SWITCH_ELECTRICAL_3_V2:
	case BLE_SWITCH_1:
	case BLE_SWITCH_WATER_HEATER:
	case BLE_SWITCH_2:
	case BLE_SWITCH_3:
	case BLE_SWITCH_4:
	case BLE_WIFI_SWITCH_1:
	case BLE_WIFI_SWITCH_2:
	case BLE_WIFI_SWITCH_3:
	case BLE_WIFI_SWITCH_4:
	case BLE_WIFI_SWITCH_1_SQUARE:
	case BLE_WIFI_SWITCH_2_SQUARE:
	case BLE_WIFI_SWITCH_3_SQUARE:
	case BLE_WIFI_SWITCH_4_SQUARE:
	case BLE_WIFI_SWITCH_ELECTRICAL_1:
	case BLE_WIFI_SWITCH_ELECTRICAL_2:
	case BLE_WIFI_SWITCH_ELECTRICAL_3:
	case BLE_SWITCH_KNOB:
		BleProtocol::UpdateStatusRelaySwitch(devAddr, typeDev);
		break;
	case BLE_SWITCH_CURTAIN:
	case BLE_SWITCH_RGB_CURTAIN:
	case BLE_SWITCH_RGB_CURTAIN_SQUARE:
	case BLE_SWITCH_RGB_CURTAIN_HCN:
	case BLE_SWITCH_RGB_CURTAIN_SQUARE_V2:
	case BLE_SWITCH_ROOLING_DOOR:
	case BLE_SWITCH_ROOLING_DOOR_V2:
	case BLE_SWITCH_ROOLING_DOOR_SQUARE:
	case BLE_WIFI_SWITCH_CURTAIN:
	case BLE_WIFI_SWITCH_CURTAIN_SQUARE:
	case BLE_WIFI_SWITCH_ROOLING_DOOR:
	case BLE_WIFI_SWITCH_ROOLING_DOOR_SQUARE:
		BleProtocol::UpdateStatusCurtain(devAddr);
		break;
	default:
		BleProtocol::GetTTL(devAddr);
		break;
	}
	return CODE_OK;
}

// int BleProtocol::CallModeRgb(uint16_t devAddr, uint8_t modeRgb)
// {
// 	LOGD("Call modeRgb: %d, addr: 0x%04X ", modeRgb, devAddr);
// 	uint8_t dataRsp[100];
// 	int lenRsp = 0;
// 	uint8_t modeRgbHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0x82, 0x52};
// 	typedef struct __attribute__((packed))
// 	{
// 		ble_message_header_t ble_message_header;
// 		uint16_t opcode;
// 		uint16_t header;
// 		uint8_t mode;
// 	} modergb_message_t;
// 	modergb_message_t modergb_message = {0};
// 	memset(&modergb_message, 0x00, sizeof(modergb_message));
// 	modergb_message.ble_message_header.devAddr = devAddr;
// 	modergb_message.opcode = LIGHTNESS_LINEAR_SET;
// 	modergb_message.header = HEADER_CALLMODE_RGB;
// 	modergb_message.mode = modeRgb;
// 	int rs = SendMessage(APP_REQ, (uint8_t *)&modergb_message, sizeof(modergb_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, modeRgbHeader, 0, 6);
// 	if (rs == CODE_OK)
// 	{
// 		typedef struct __attribute__((packed))
// 		{
// 			uint16_t devAddr;
// 			uint16_t gwAddr;
// 			uint16_t opcode;
// 			uint16_t header;
// 			uint8_t mode;
// 		} modergb_rsp_message_t;
// 		if (lenRsp >= (int)sizeof(modergb_rsp_message_t))
// 		{
// 			modergb_rsp_message_t *modergb_rsp_message = (modergb_rsp_message_t *)dataRsp;
// 			if (modergb_rsp_message->header == HEADER_CALLMODE_RGB)
// 			{
// 				if (modergb_rsp_message->mode == modeRgb)
// 				{
// 					return CODE_OK;
// 				}
// 				LOGW("call mode rgb resp state not match with input control");
// 			}
// 		}
// 		else
// 		{
// 			LOGW("call mode rgb response too short (%d bytes)", lenRsp);
// 		}
// 	}
// 	LOGW("call mode rgb err");
// 	return CODE_ERROR;
// }

int BleProtocol::UpdateStatusSensorsPm(uint16_t devAddr)
{
	LOGD("Update status sensor addr: 0x%04X ", devAddr);
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint8_t data[6];
	} update_message_t;
	update_message_t update_message = {0};
	memset(&update_message, 0x00, sizeof(update_message));
	update_message.ble_message_header.devAddr = devAddr;
	update_message.opcodeVendor = RD_OPCODE_CONFIG;
	update_message.vendorId = RD_VENDOR_ID;
	update_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	update_message.header = GET_STATUS_SENSOR_PM;
	int rs = SendMessage(APP_REQ, (uint8_t *)&update_message, sizeof(update_message_t), HCI_GATEWAY_RSP_OP_CODE, 0, 0, 1000);
	if (rs == CODE_OK)
	{
		return CODE_OK;
	}
	LOGW("update status sensor err");
	return CODE_ERROR;
}

int BleProtocol::TimeActionPirLightSensor(uint16_t devAddr, uint16_t time)
{
	LOGD("TimeActionPirLightSensor 0x%04X", devAddr);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	uint8_t timeActionHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0xe3, 0x11, 0x02};
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint16_t time;
	} time_action_message_t;
	time_action_message_t time_action_message = {0};
	memset(&time_action_message, 0x00, sizeof(time_action_message));
	time_action_message.ble_message_header.devAddr = devAddr;
	time_action_message.opcodeVendor = RD_OPCODE_CONFIG;
	time_action_message.vendorId = RD_VENDOR_ID;
	time_action_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	time_action_message.header = RD_HEADER_CONFIG_SET_TIME_ACTION_PIR_LIGHT_SENSOR;
	time_action_message.time = time;
	int rs = SendMessage(APP_REQ, (uint8_t *)&time_action_message, sizeof(time_action_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, timeActionHeader, 0, 7);
	if (rs == CODE_OK)
	{
		typedef struct __attribute__((packed))
		{
			uint16_t devAddr;
			uint16_t gwAddr;
			uint8_t opcodeRsp;
			uint16_t vendorId;
			uint16_t header;
			uint16_t time;
		} time_action_rsp_message_t;
		if (lenRsp >= (int)sizeof(time_action_rsp_message_t))
		{
			time_action_rsp_message_t *time_action_rsp_message = (time_action_rsp_message_t *)dataRsp;
			if (time_action_rsp_message->header == RD_HEADER_CONFIG_SET_TIME_ACTION_PIR_LIGHT_SENSOR && time_action_rsp_message->time == time)
			{
				return CODE_OK;
			}
			LOGW("time action pir light resp state not match with input control");
		}
		else
		{
			LOGW("time action pir light response too short (%d bytes)", lenRsp);
		}
	}
	LOGW("time action pir light err");
	return CODE_ERROR;
}

int BleProtocol::SetModeActionPirLightSensor(uint16_t devAddr, uint8_t mode)
{
	LOGD("ModeActionPirLightSensor 0x%04X", devAddr);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	uint8_t modeActionHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0xe3, 0x11, 0x02};
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint8_t mode;
		uint8_t future[5];
	} mode_action_message_t;
	mode_action_message_t mode_action_message = {0};
	memset(&mode_action_message, 0x00, sizeof(mode_action_message));
	mode_action_message.ble_message_header.devAddr = devAddr;
	mode_action_message.opcodeVendor = RD_OPCODE_CONFIG;
	mode_action_message.vendorId = RD_VENDOR_ID;
	mode_action_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	mode_action_message.header = RD_HEADER_CONFIG_SET_MODE_ACTION_PIR_LIGHT_SENSOR;
	mode_action_message.mode = mode;
	int rs = SendMessage(APP_REQ, (uint8_t *)&mode_action_message, sizeof(mode_action_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, modeActionHeader, 0, 7);
	if (rs == CODE_OK)
	{
		typedef struct __attribute__((packed))
		{
			uint16_t devAddr;
			uint16_t gwAddr;
			uint8_t opcodeRsp;
			uint16_t vendorId;
			uint16_t header;
			uint8_t mode;
		} mode_action_rsp_message_t;
		if (lenRsp >= (int)sizeof(mode_action_rsp_message_t))
		{
			mode_action_rsp_message_t *mode_action_rsp_message = (mode_action_rsp_message_t *)dataRsp;
			if (mode_action_rsp_message->header == RD_HEADER_CONFIG_SET_MODE_ACTION_PIR_LIGHT_SENSOR && mode_action_rsp_message->mode == mode)
			{
				return CODE_OK;
			}
			LOGW("mode action pir light resp state not match with input control");
		}
		else
		{
			LOGW("mode action pir light response too short (%d bytes)", lenRsp);
		}
	}
	LOGW("mode action pir light err");
	return CODE_ERROR;
}

int BleProtocol::SetSensiPirLightSensor(uint16_t devAddr, uint8_t sensi)
{
	LOGD("Set sensiPirLightSensor: 0x%04X, sensi: %d", devAddr, sensi);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	uint8_t sensiHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0xe3, 0x11, 0x02};
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint16_t sensi;
	} sensi_message_t;
	sensi_message_t sensi_message = {0};
	memset(&sensi_message, 0x00, sizeof(sensi_message));
	sensi_message.ble_message_header.devAddr = devAddr;
	sensi_message.opcodeVendor = RD_OPCODE_CONFIG;
	sensi_message.vendorId = RD_VENDOR_ID;
	sensi_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	sensi_message.header = RD_HEADER_CONFIG_SET_SENSI_PIR_LIGHT_SENSOR;
	sensi_message.sensi = sensi;
	int rs = SendMessage(APP_REQ, (uint8_t *)&sensi_message, sizeof(sensi_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, sensiHeader, 0, 7);
	if (rs == CODE_OK)
	{
		return CODE_OK;
	}
	else
		LOGW("Set sensi error");
	return CODE_ERROR;
}

int BleProtocol::SetDistanceSensor(uint16_t devAddr, uint8_t distance)
{
	LOGD("Set distance sensor: 0x%04X, distance: %d", devAddr, distance);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	uint8_t distanceHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0xe3, 0x11, 0x02};
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint16_t distance;
	} distance_message_t;
	distance_message_t distance_message = {0};
	memset(&distance_message, 0x00, sizeof(distance_message));
	distance_message.ble_message_header.devAddr = devAddr;
	distance_message.opcodeVendor = RD_OPCODE_CONFIG;
	distance_message.vendorId = RD_VENDOR_ID;
	distance_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	distance_message.header = RD_HEADER_CONFIG_SET_DISTANCE_RADA_SENSOR;
	distance_message.distance = distance;
	int rs = SendMessage(APP_REQ, (uint8_t *)&distance_message, sizeof(distance_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, distanceHeader, 0, 7);
	if (rs == CODE_OK)
	{
		return CODE_OK;
	}
	else
		LOGW("Set distance error");
	return CODE_ERROR;
}

int BleProtocol::SetTimeRspSensor(uint16_t devAddr, uint16_t time)
{
	LOGD("Set time rsp sensor: 0x%04X, time: %d", devAddr, time);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	uint8_t timeRspHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0xe3, 0x11, 0x02};
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint16_t time;
	} timeRsp_message_t;
	timeRsp_message_t timeRsp_message = {0};
	memset(&timeRsp_message, 0x00, sizeof(timeRsp_message));
	timeRsp_message.ble_message_header.devAddr = devAddr;
	timeRsp_message.opcodeVendor = RD_OPCODE_CONFIG;
	timeRsp_message.vendorId = RD_VENDOR_ID;
	timeRsp_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	timeRsp_message.header = RD_HEADER_CONFIG_TIME_RSP_SENSOR;
	timeRsp_message.time = time;
	int rs = SendMessage(APP_REQ, (uint8_t *)&timeRsp_message, sizeof(timeRsp_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, timeRspHeader, 0, 7);
	if (rs == CODE_OK)
	{
		return CODE_OK;
	}
	else
		LOGW("Set time rsp error");
	return CODE_ERROR;
}

int BleProtocol::DelAllScene(uint16_t devAddr)
{
	LOGD("DelAllScene 0x%04x", devAddr);
	uint8_t dataRsp[100];
	int lenRsp = 0;
	uint8_t delAllSceneScreenTouchHeader[] = {(uint8_t)(devAddr & 0xFF), (uint8_t)((devAddr >> 8) & 0xFF), 1, 0, 0xe3, 0x11, 0x02};
	typedef struct __attribute__((packed))
	{
		ble_message_header_t ble_message_header;
		uint8_t opcodeVendor;
		uint16_t vendorId;
		uint8_t opcodeRsp;
		uint8_t tidPos;
		uint16_t header;
		uint8_t check[6];
	} del_allscene_screen_touch_message_t;
	del_allscene_screen_touch_message_t del_allscene_screen_touch_message = {0};
	memset(&del_allscene_screen_touch_message, 0x00, sizeof(del_allscene_screen_touch_message));
	del_allscene_screen_touch_message.ble_message_header.devAddr = devAddr;
	del_allscene_screen_touch_message.opcodeVendor = RD_OPCODE_CONFIG;
	del_allscene_screen_touch_message.vendorId = RD_VENDOR_ID;
	del_allscene_screen_touch_message.opcodeRsp = RD_OPCODE_CONFIG_RSP;
	del_allscene_screen_touch_message.header = RD_HEADER_CONFIG_DEL_ALL_SCENE;
	for (int i = 0; i < 6; i++)
	{
		del_allscene_screen_touch_message.check[i] = i;
	}
	int rs = SendMessage(APP_REQ, (uint8_t *)&del_allscene_screen_touch_message, sizeof(del_allscene_screen_touch_message_t), HCI_GATEWAY_RSP_OP_CODE, dataRsp, &lenRsp, 1000, delAllSceneScreenTouchHeader, 0, 7);
	if (rs == CODE_OK)
	{
		typedef struct __attribute__((packed))
		{
			uint16_t devAddr;
			uint16_t gwAddr;
			uint8_t opcodeRsp;
			uint16_t vendorId;
			uint16_t header;
		} scene_screen_touch_rsp_message_t;
		if (lenRsp >= (int)sizeof(scene_screen_touch_rsp_message_t))
		{
			scene_screen_touch_rsp_message_t *scene_screen_touch_rsp_message = (scene_screen_touch_rsp_message_t *)dataRsp;
			if (scene_screen_touch_rsp_message->header == RD_HEADER_CONFIG_DEL_ALL_SCENE)
			{
				return CODE_OK;
			}
			LOGW("del all scene screen touch resp state not match with input control");
		}
		else
		{
			LOGW("del all scene screen touch response too short (%d bytes)", lenRsp);
		}
	}
	LOGW("del all scene screen touch err");
	return CODE_ERROR;
}

int BleProtocol::SetGroup(uint16_t devAddr, uint16_t group)
{
	LOGW("SetGroup 0x%04x, group %d", devAddr, group);
	return CODE_ERROR;
}

int BleProtocol::ControlOpenClosePausePercent(uint16_t devAddr, uint8_t type, uint8_t percent)
{
	LOGW("ControlOpenClosePausePercent 0x%04x, type %d, percent %d", devAddr, type, percent);
	return CODE_ERROR;
}

int BleProtocol::ConfigMotor(uint16_t devAddr, uint8_t typeMotor)
{
	LOGW("ConfigMotor 0x%04x, typeMotor %d", devAddr, typeMotor);
	return CODE_ERROR;
}

int BleProtocol::CalibCurtain(uint16_t devAddr, uint8_t status)
{
	LOGW("CalibCurtain 0x%04x, status %d", devAddr, status);
	return CODE_ERROR;
}

int BleProtocol::CalibAuto(uint16_t devAddr, uint16_t time)
{
	LOGW("CalibAuto 0x%04x, time %d", devAddr, time);
	return CODE_ERROR;
}

int BleProtocol::LockDevice(uint16_t devAddr, uint8_t locked)
{
	LOGW("LockDevice 0x%04x, locked %d", devAddr, locked);
	return CODE_ERROR;
}

int BleProtocol::SetModeWifi(uint16_t devAddr, uint8_t mode)
{
	LOGW("SetModeWifi 0x%04x, mode %d", devAddr, mode);
	return CODE_ERROR;
}

int BleProtocol::UpdateStatusCurtain(uint16_t devAddr)
{
	LOGW("UpdateStatusCurtain 0x%04x", devAddr);
	return CODE_ERROR;
}

int BleProtocol::ControlRgbSwitch(uint16_t devAddr, uint8_t button, uint8_t b, uint8_t g, uint8_t r, uint8_t dimOn, uint8_t dimOff)
{
	LOGW("ControlRgbSwitch 0x%04X, button %d, r %d, g %d, b %d, dimon %d, dimOff %d", devAddr, button, r, g, b, dimOn, dimOff);
	return CODE_ERROR;
}

int BleProtocol::ControlRelayOfSwitch(uint16_t devAddr, uint32_t type, uint8_t relay, uint8_t value)
{
	LOGW("ControlRelayOfSwitch 0x%04X, relayid %d, value %d", devAddr, relay, value);
	return CODE_ERROR;
}

int BleProtocol::SetIdCombine(uint16_t devAddr, uint16_t id)
{
	LOGW("SetIdCombine 0x%04x, id %d", devAddr, id);
	return CODE_ERROR;
}

int BleProtocol::CountDownSwitch(uint16_t devAddr, uint16_t timer, uint8_t status)
{
	LOGW("CountDownSwitch 0x%04x, timer %d, status %d", devAddr, timer, status);
	return CODE_ERROR;
}

int BleProtocol::UpdateStatusRelaySwitch(uint16_t devAddr, uint32_t type)
{
	LOGW("Update status Relay Switch 0x%04x", devAddr);
	return CODE_ERROR;
}

int BleProtocol::ConfigStatusStartupSwitch(uint16_t devAddr, uint8_t status, uint32_t type)
{
	LOGW("ConfigStatusStartup 0x%04x, status %d", devAddr, status);
	return CODE_ERROR;
}

int BleProtocol::ConfigModeInputSwitchOnoff(uint16_t devAddr, uint8_t mode)
{
	LOGW("ConfigModeInputStatusOnoff 0x%04x, mode %d", devAddr, mode);
	return CODE_ERROR;
}

int BleProtocol::ConfigModeInputModuleInOut(uint16_t devAddr, uint8_t index, uint8_t mode)
{
	LOGW("ConfigModeInputModuleInOut 0x%04x, index %d, mode %d", devAddr, index, mode);
	return CODE_ERROR;
}

int BleProtocol::ConfigCombinInOutModuleInOut(uint16_t devAddr, uint8_t indexIn, uint8_t indexOut)
{
	LOGW("ConfigCombinInOutModuleInOut 0x%04x, indexIn %d, indexOut %d", devAddr, indexIn, indexOut);
	return CODE_ERROR;
}

int BleProtocol::SetSceneModuleInOut(uint16_t devAddr, uint8_t type, uint8_t indexIn, uint8_t status, uint16_t sceneId)
{
	LOGW("SetSceneModuleInOut 0x%04x, type %d, indexIn %d, status %d, sceneId: %d", devAddr, type, indexIn, status, sceneId);
	return CODE_ERROR;
}

int BleProtocol::ConfigDeltaADC(uint16_t devAddr, uint8_t delta)
{
	LOGW("ConfigDeltaADC 0x%04x, delta %d", devAddr, delta);
	return CODE_ERROR;
}

int BleProtocol::ConfigStatusStartupRelay(uint16_t devAddr, uint8_t relayId, uint8_t status)
{
	LOGW("ConfigStatusStartupRelay 0x%04x, relay %d, status %d", devAddr, relayId, status);
	return CODE_ERROR;
}

int BleProtocol::GetInfogw()
{
	LOGW("GetInfogw");
	return CODE_ERROR;
}

int BleProtocol::GetInfoMesh()
{
	LOGW("GetInfoMesh");
	return CODE_ERROR;
}

int BleProtocol::UpdateDeviceKeyDev(uint16_t devAddr, string devKeyDev)
{
	LOGW("UpdateDeviceKeyDev: %d, devKey: %s", devAddr, devKeyDev.c_str());
	return CODE_ERROR;
}

int BleProtocol::UpdateDeviceKeyGateway(uint16_t gwAddr, string devKeyDev)
{
	LOGW("UpdateDeviceKeyGateway: %s", devKeyDev.c_str());
	return CODE_ERROR;
}

int BleProtocol::UpdateNetKey(uint16_t gwAddr, string netKey, uint32_t indexId)
{
	LOGW("UpdateNetKey: %s, indexId: %d", netKey.c_str(), indexId);
	return CODE_ERROR;
}

int BleProtocol::UpdateDevKey(uint16_t gwAddr, string devKey)
{
	LOGW("UpdateDevKey %s", devKey.c_str());
	return CODE_ERROR;
}

int BleProtocol::UpdateAppKey(string appKey)
{
	LOGW("UpdateAppKey %s", appKey.c_str());
	return CODE_ERROR;
}

int BleProtocol::UpdateMaxAddr(uint16_t addr)
{
	LOGW("UpdateMaxAddr: %d", addr);
	return CODE_ERROR;
}
