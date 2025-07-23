#include "RuleInputDevice.h"
#include "Device.h"
#include "RuleManager.h"
#include "Log.h"

RuleInputDevice::RuleInputDevice(Rule *rule, Device *device, Json::Value &data)
{
	this->rule = rule;
	this->device = device;
	this->data = data;
	device->CheckData(data, isAvailable);
	if (device)
	{
		device->RegisterTrigger(this);
	}
}

RuleInputDevice::~RuleInputDevice()
{
	LOGI("~RuleInputDevice");
	if (device)
	{
		device->UnregisterTrigger(this);
	}
}

Json::Value *RuleInputDevice::getData()
{
	return &data;
}

void RuleInputDevice::Trigger(bool value)
{
	LOGD("Trigger");
	isAvailable = value;
	if (isAvailable && rule)
	{
		RuleManager::GetInstance()->CheckRule(rule);
	}
}

void RuleInputDevice::UpdateStatus(bool status)
{
	this->isAvailable = status;
}