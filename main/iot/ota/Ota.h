#pragma once

#include <string>

using namespace std;

namespace Ota
{
	void init();
	bool CheckNeedUpdate(string fwVersion);
	int UpdateTask(string fwUrl);
	int Update(string fwUrl, string fwChecksumAlgorithm, string fwChecksum);
	int Update(int fwSize, string fwChecksumAlgorithm, string fwChecksum);
	int UpdateChunk(int chunkId, uint8_t *data, int dataLen);
	int UpdateFinish();
}
