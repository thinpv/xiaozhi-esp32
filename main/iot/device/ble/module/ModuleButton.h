#pragma once
#include "Module.h"
#include "Database.h"

using namespace std;

class ModuleButton : public Module
{
protected:
	uint8_t bt;
	string key;

public:
	ModuleButton(Device *device, uint16_t addr, uint32_t index = 0);
	~ModuleButton();

#ifdef CONFIG_SAVE_ATTRIBUTE
	/**
	 * @brief Init parameter value from database after system start
	 *
	 * @param attribute id of attribute
	 * @param value value of attribute
	 */
	void InitAttribute(string attribute, double value);

	/**
	 * @brief Save parameter value to database
	 *
	 */
	void SaveAttribute();
#endif

	int InputData(Json::Value &dataValue, Json::Value &jsonValue);

	/**
	 * @brief Parse raw data to element parameter value
	 *
	 * @param data data from device driver (uart)
	 * @param len length of data
	 * @param jsonValue json value to put parameter after parsing
	 * @return true if data include this element opcode
	 * @return false
	 */
	int InputData(uint8_t *data, int len, Json::Value &jsonValue);

	/**
	 * @brief Check rule input
	 *
	 * @param dataValue json rule data input
	 * @param rs result of checking
	 * @return true if dataValue uses this element paramter
	 * @return false if dataValue don't use this element paramter
	 */
	bool CheckData(Json::Value &dataValue, bool &rs);

	/**
	 * @brief Build telemetry message with this element
	 *
	 * @param jsonValue
	 */
	void BuildTelemetryValue(Json::Value &jsonValue);
};
