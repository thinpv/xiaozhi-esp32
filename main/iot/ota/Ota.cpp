#include "Ota.h"
#include "Log.h"
#include "ErrorCode.h"
#include <string.h>
#include <thread>

#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#define BUFFSIZE 1024

static esp_ota_handle_t update_handle = 0;
static const esp_partition_t *update_partition = NULL;

static int fwSize = 0;
static string fwChecksumAlgorithm;
static string fwChecksum;

void Ota::init()
{
}

bool Ota::CheckNeedUpdate(string fwVersion)
{
	LOGD("ota_need_update");
	esp_app_desc_t running_app_info;
	const esp_partition_t *running = esp_ota_get_running_partition();
	if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
	{
		LOGI("update version: %s, running version: %s", fwVersion.c_str(), running_app_info.version);
		if (fwVersion != string(running_app_info.version))
		{
			return true;
		}
	}
	return false;
}

static void http_cleanup(esp_http_client_handle_t client)
{
	esp_http_client_close(client);
	esp_http_client_cleanup(client);
}

int Ota::UpdateTask(string fwUrl)
{
	esp_err_t err;
	/* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
	esp_ota_handle_t update_handle = 0;
	const esp_partition_t *update_partition = NULL;

	LOGI("Starting OTA example: %s", fwUrl.c_str());
	char *ota_write_data = (char *)malloc(BUFFSIZE); // heap_caps_malloc_prefer((BUFFSIZE + 1) * 2, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);

	const esp_partition_t *configured = esp_ota_get_boot_partition();
	const esp_partition_t *running = esp_ota_get_running_partition();

	if (configured != running)
	{
		LOGW("Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
		LOGW("(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
	}
	LOGI("Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, running->address);

	esp_http_client_config_t config = {
			.url = fwUrl.c_str(),
			// .cert_pem = (char *)server_cert_pem_start,
			// .timeout_ms = 10000,
			.keep_alive_enable = true,
			// .skip_cert_common_name_check = true;
	};

	esp_https_ota_config_t ota_config = {
			.http_config = &config,
	};
	LOGI("Attempting to download update from %s", config.url);
	esp_err_t ret = esp_https_ota(&ota_config);
	if (ret == ESP_OK)
	{
		LOGI("OTA Succeed, Rebooting...");
		// TODO: OTA response to server
		esp_restart();
	}
	else
	{
		LOGE("Firmware upgrade failed");
		return CODE_ERROR;
	}
	return CODE_OK;
}

int Ota::Update(string fwUrl, string fwChecksumAlgorithm, string fwChecksum)
{
	thread updateThread(Ota::UpdateTask, fwUrl);
	updateThread.detach();
	return CODE_OK;
}

int Ota::Update(int _fwSize, string _fwChecksumAlgorithm, string _fwChecksum)
{
	fwSize = _fwSize;
	fwChecksumAlgorithm = _fwChecksumAlgorithm;
	fwChecksum = _fwChecksum;
	return CODE_OK;
}

int Ota::UpdateChunk(int chunkId, uint8_t *data, int dataLen)
{
	// LOGW("UpdateChunk size: %d", sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t));
	esp_err_t err;
	if (chunkId == 0)
	{
		esp_app_desc_t new_app_info;
		if (dataLen > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
		{
			// check current version with downloading
			memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
			LOGI("New firmware version: %s", new_app_info.version);

			esp_app_desc_t running_app_info;
			const esp_partition_t *running = esp_ota_get_running_partition();
			if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
			{
				LOGI("Running firmware version: %s", running_app_info.version);
			}

			if (strcmp(new_app_info.version, running_app_info.version) == 0)
			{
				LOGI("Firmware version is the same, no need to update");
				return CODE_OK;
			}

			update_partition = esp_ota_get_next_update_partition(NULL);
			assert(update_partition != NULL);
			LOGI("Writing to partition subtype %d at offset 0x%" PRIx32,
					 update_partition->subtype, update_partition->address);

			// image_header_was_checked = true;

			err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
			if (err != ESP_OK)
			{
				LOGE("esp_ota_begin failed (%s)", esp_err_to_name(err));
				esp_ota_abort(update_handle);
				return CODE_OTA_ERROR;
			}
			LOGI("esp_ota_begin succeeded");
		}
		else
		{
			LOGE("received package is not fit len");
			return CODE_OTA_DATA_ERROR;
		}
	}

	err = esp_ota_write(update_handle, (const void *)data, dataLen);
	if (err != ESP_OK)
	{
		esp_ota_abort(update_handle);
		return CODE_OTA_ERROR;
	}
	return CODE_OK;
}

int Ota::UpdateFinish()
{
	LOGI("UpdateFinish");
	esp_err_t err = esp_ota_end(update_handle);
	if (err != ESP_OK)
	{
		if (err == ESP_ERR_OTA_VALIDATE_FAILED)
		{
			LOGE("Image validation failed, image is corrupted");
			return CODE_OTA_DATA_ERROR;
		}
		else
		{
			LOGE("esp_ota_end failed (%s)!", esp_err_to_name(err));
		}
		return CODE_OTA_ERROR;
	}

	err = esp_ota_set_boot_partition(update_partition);
	if (err != ESP_OK)
	{
		LOGE("esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		return CODE_OTA_ERROR;
	}
	return CODE_OK;
}
