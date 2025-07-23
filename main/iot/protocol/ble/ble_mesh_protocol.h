#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

	int esp_ble_send_vendor(uint32_t opcode, uint16_t addr, uint16_t len, uint8_t *data, bool ack);
	int esp_ble_set_gw_addr(uint16_t devAddr);
	int esp_ble_get_device_type(uint8_t *mac, uint16_t devAddr);
	void example_ble_mesh_send_vendor_message(bool resend);

#ifdef __cplusplus
}
#endif