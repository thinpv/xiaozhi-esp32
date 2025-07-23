#pragma once

#include <string>
#include <stdint.h>
#include <vector>
#include <atomic>
#include <functional>
#include <mutex>
#include "ErrorCode.h"

#include "BleMeshDefine.h"
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
#include "app_ble_mesh_provi.h"
#include "ble_mesh_protocol.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#endif

using namespace std;

typedef struct __attribute__((packed))
{
	uint8_t mac[6];
	uint8_t len;
	uint8_t header_type;
	uint8_t beacon_type;
	uint8_t uuid[16];
	uint32_t uri_hash;
	uint16_t obb_info;
	int8_t rssi;
	uint16_t dc;
} scan_device_message_t;

typedef struct __attribute__((packed))
{
	uint16_t parentAddr;
	uint16_t gwAddr;
	uint8_t opcodeRsp;
	uint16_t vendorId;
	uint16_t header;
	uint8_t mac[4];
	uint8_t type;
	uint8_t rssi;
} scan_device_pair_message_t;

typedef struct __attribute__((packed))
{
	uint16_t len;
	uint8_t magic;
	uint8_t opcode;
	uint8_t data[];
} message_rsp_st;

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

class BleProtocol
{
private:
	typedef struct __attribute__((packed))
	{
		uint16_t opcode;
		uint8_t data[100];
	} message_req_st;

	typedef struct __attribute__((packed))
	{
		bool status;
		uint8_t opcode;
		int *len;
		uint8_t *data;
		uint8_t *compare_data;
		int compare_position;
		int compare_len;
	} message_rsp_list_st;

	typedef struct __attribute__((packed))
	{
		uint8_t uuid[8];
		uint8_t deviceType[4];
		uint16_t fwVersion;
		uint16_t magic;
	} uuid_t;

	typedef struct __attribute__((packed))
	{
		uint8_t netKey[16];
		uint16_t key_index;
		uint8_t flag;
		uint32_t iv_index;
		uint16_t unicast_address;
	} pro_net_info_t;

	typedef struct __attribute__((packed))
	{
		uint16_t nkIdx;
		uint16_t akIdx;
		uint8_t retryCnt;
		uint8_t rspMax;
		uint16_t devAddr;
	} ble_message_header_t;

	mutex messageRespListMtx;
	vector<message_rsp_list_st *> messageRespList;
	mutex mtxWaitSendUart;

#define BLE_CHECK_OPCODE_BUFFER_MAX_SIZE 500
	mutex vectorCheckOpcodeMtx;
	vector<message_rsp_st *> messageCheckOpcodeList;

	// TODO: Add init state
	pro_net_info_t pro_net_info;
	uint8_t netKey[16];
	uint8_t appKey[16];
	uint8_t gwKey[16];
	uint8_t deviceKey[16];
	uint16_t nextAddr;

	uint16_t tid;

	BleProtocol();
	virtual ~BleProtocol();

	int OnMessage(unsigned char *data, int len);
	int SendMessage(uint16_t opReq, uint8_t *dataReq, int lenReq, uint8_t opRsp, uint8_t *dataRsp, int *lenRsp, uint32_t timeout, uint8_t *compare_data = 0, int compare_position = 0, int compare_len = 0);

public:
	atomic<bool> haveNewMac;
	atomic<bool> haveGetMacRsp;
	atomic<bool> isInitKey;
	scan_device_message_t scanDeviceMessage;
	scan_device_pair_message_t scanDevicePairMessage;

	static BleProtocol *GetInstance();

	void init();

	int GetOpcodeExceptionMessage(message_rsp_st **data);
	void CheckOpcodeException(message_rsp_st *message);

	void initKey(string netKeyStr, string appKeyStr);
	int StartScan();
	int StopScan();
	bool IsProvision();
	void SetProvisioning(bool isProvision);

	int ResetBle();
	int ResetFactory();
	int ResetDev(uint16_t devAddr);
	int ResetDelAll();

	int SendOnlineCheck(uint16_t devAddr, uint32_t typeDev, uint16_t version);
	int GetTTL(uint16_t devAddr);

	// Generic
	int SetOnOffLight(uint16_t devAddr, uint8_t onoff, uint16_t transition, bool ack);
	int GetOnOffLight(uint16_t devAddr);

	// Light
	int SetDimmingLight(uint16_t devAddr, uint16_t dim, uint16_t transition, bool ack);
	// int GetDimming(uint16_t devAddr);
	int SetLevelDim(uint16_t devAddr, uint8_t dimMax, uint8_t dimMin, bool ack = true);
	int SetCctLight(uint16_t devAddr, uint16_t cct, uint16_t transition, bool ack);
	// int GetCct(uint16_t devAddr);
	int SetHSLLight(uint16_t devAddr, uint16_t H, uint16_t S, uint16_t L, uint16_t transition, bool ack);
	// int GetHSL(uint16_t devAddr);
	int SetCctDimLight(uint16_t devAddr, uint16_t cct, uint16_t dim, uint16_t transition, bool ack);
	// int GetCctDimLight(uint16_t devAddr);
	int UpdateLights(uint16_t devAddr);

	// Config
	int GroupAddDevice(uint16_t devAddr, uint16_t element, uint16_t group);
	int GroupDelDevice(uint16_t devAddr, uint16_t element, uint16_t group);

	// Scene light
	int SceneAddDevice(uint16_t devAddr, uint16_t scene, uint8_t modeRgb);
	int SceneDelDevice(uint16_t devAddr, uint16_t scene);
	int SceneRecall(uint16_t devAddr, uint16_t scene, uint16_t transition, bool ack);
	int CallModeRgb(uint16_t devAddr, uint8_t modeRgb);

	// update status lights
	int UpdateStatusSensorsPm(uint16_t devAddr);

	// remote scene
	int SetSceneSwitchSceneDC(uint16_t devAddr, uint8_t button, uint8_t mode, uint16_t sceneId, uint8_t type);
	int SetSceneSwitchSceneAC(uint16_t devAddr, uint8_t button, uint8_t mode, uint16_t sceneId, uint8_t type);
	int DelSceneSwitchSceneDC(uint16_t devAddr, uint8_t button, uint8_t mode);
	int DelSceneSwitchSceneAC(uint16_t devAddr, uint8_t button, uint8_t mode);

	// PirLightSensor
	int SetScenePirLightSensor(uint16_t devAddr, uint8_t condition, uint8_t pir, uint16_t lowLux, uint16_t highLux, uint16_t scene, uint8_t type);
	int DelScenePirLightSensor(uint16_t devAddr, uint16_t scene);
	int TimeActionPirLightSensor(uint16_t devAddr, uint16_t time);
	int SetModeActionPirLightSensor(uint16_t devAddr, uint8_t mode);
	int SetSensiPirLightSensor(uint16_t devAddr, uint8_t sensi);
	int SetDistanceSensor(uint16_t devAddr, uint8_t distance);
	int SetTimeRspSensor(uint16_t devAddr, uint16_t time);

	// switch
	int ControlRgbSwitch(uint16_t devAddr, uint8_t button, uint8_t b, uint8_t g, uint8_t r, uint8_t dimOn, uint8_t dimOff);
	int ControlRelayOfSwitch(uint16_t devAddr, uint32_t type, uint8_t relay, uint8_t value);
	int SetIdCombine(uint16_t devAddr, uint16_t id);
	int CountDownSwitch(uint16_t devAddr, uint16_t timer, uint8_t status);
	int UpdateStatusRelaySwitch(uint16_t devAddr, uint32_t type = 0);
	int ConfigStatusStartupSwitch(uint16_t devAddr, uint8_t status, uint32_t type = 0);
	int ConfigModeInputSwitchOnoff(uint16_t devAddr, uint8_t mode);

	// screen touch
	int SceneForScreenTouch(uint16_t devAddr, uint16_t scene, uint8_t icon, uint8_t type);
	int EditIconScreenTouch(uint16_t devAddr, uint16_t scene, uint8_t icon);
	int DelSceneScreenTouch(uint16_t devAddr, uint16_t scene);
	int DelAllScene(uint16_t devAddr);
	int SendWeatherOutdoor(uint16_t devAddr, uint8_t status, uint16_t temp);
	int SendWeatherIndoor(uint16_t devAddr, uint16_t temp, uint16_t hum, uint16_t pm25);
	int SendDate(uint16_t devAddr, uint16_t years, uint8_t month, uint8_t date, uint8_t day);
	int SendTime(uint16_t devAddr, uint8_t hours, uint8_t minute, uint8_t second);
	int SetGroup(uint16_t devAddr, uint16_t group);

	// Rooling door, curtain
	int ControlOpenClosePausePercent(uint16_t devAddr, uint8_t type, uint8_t percent = 0);
	int ConfigMotor(uint16_t devAddr, uint8_t typeMotor);
	int CalibCurtain(uint16_t devAddr, uint8_t status);
	int UpdateStatusCurtain(uint16_t devAddr);
	int CalibAuto(uint16_t devAddr, uint16_t time);
	int LockDevice(uint16_t devAddr, uint8_t locked);
	int SetModeWifi(uint16_t devAddr, uint8_t mode);

	// ModuleInOut
	int ConfigModeInputModuleInOut(uint16_t devAddr, uint8_t index, uint8_t mode);
	int ConfigCombinInOutModuleInOut(uint16_t devAddr, uint8_t indexIn, uint8_t indexOut);
	int SetSceneModuleInOut(uint16_t devAddr, uint8_t type, uint8_t indexIn, uint8_t status, uint16_t sceneId);
	int ConfigDeltaADC(uint16_t devAddr, uint8_t delta);
	int ConfigStatusStartupRelay(uint16_t devAddr, uint8_t relayId, uint8_t status);

	// Backup
	int GetInfogw();
	int GetInfoMesh();
	int UpdateDeviceKeyDev(uint16_t devAddr, string devKeyDev);
	int UpdateDeviceKeyGateway(uint16_t gwAddr, string devKeyDev);
	int UpdateNetKey(uint16_t gwAddr, string netKey, uint32_t indexId);
	int UpdateDevKey(uint16_t gwAddr, string devKey);
	int UpdateAppKey(string appKey);
	int UpdateMaxAddr(uint16_t addr);
};
