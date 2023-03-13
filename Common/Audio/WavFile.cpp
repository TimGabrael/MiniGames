#include "WavFile.h"
#include <fstream>
#include "logging.h"

enum WavAudioFormat
{
	PCM = 0x0001,
	IEEEFloat = 0x0003,
	ALaw = 0x0006,
	MULaw = 0x0007,
	Extensible = 0xFFFE
};


int32_t FourBytesToInt(const uint8_t* source, int startIdx, bool littleEndian = true)
{
	int result;
	if(littleEndian)
		result = (source[startIdx + 3] << 24) | (source[startIdx + 2] << 16) | (source[startIdx + 1] << 8) | source[startIdx];
	else
		result = (source[startIdx] << 24) | (source[startIdx + 1] << 16) | (source[startIdx + 2] << 8) | source[startIdx + 3];
	return result;
}
int16_t TwoBytesToInt(const uint8_t* source, int startIdx, bool littleEndian = true)
{
	int16_t result;
	if (littleEndian)
		result = (source[startIdx + 1] << 8) | source[startIdx];
	else
		result = (source[startIdx] << 8) | source[startIdx + 1];
	return result;
}

float LinearInterpolate(float samples[2], float sampleIdx)
{
	const int normalization = (int)sampleIdx;
	sampleIdx = (sampleIdx - (float)normalization);	// leaves behind a range between 0->1
	return sampleIdx * samples[1] + (1.0f - sampleIdx) * samples[0];
}


float ByteToSample(const int8_t data)
{
	static constexpr float dx = 1.0f / (float)INT8_MAX;
	return data * dx;
}
float ShortToSample(const int16_t data)
{
	static constexpr float dx = 1.0f / (float)INT16_MAX;
	return data * dx;
}
float IntToSample(const int32_t data)
{
	static constexpr float dx = 1.0f / (float)INT32_MAX;
	return data * dx;
}


int GetIndexOfChunk(const uint8_t* fileData, int dataSize, const char* cmpStr, int startIdx, bool littleEndian = true)
{
	constexpr int dataLen = 4;
	if (strlen(cmpStr) != dataLen)
	{
		return -1;
	}
	int i = startIdx;
	while (i < (dataSize - dataLen))
	{
		if (memcmp(&fileData[i], cmpStr, dataLen) == 0)
		{
			return i;
		}
		i += dataLen;
		int chunkSize = FourBytesToInt(fileData, i, littleEndian);
		i += (dataLen + chunkSize);
	}
	return -1;
}

void WavFile::SetDataMode(int numChannels, int sampleRate)
{
	this->numChannels = numChannels;
	this->sampleRate = sampleRate;
}

bool WavFile::Load(const char* filename)
{
	if (data) delete[] data;
	data = nullptr;

	std::ifstream file(filename, std::ios::binary);
	if (!file.good()) return false;

	file.unsetf(std::ios::skipws);
	file.seekg(0, std::ios::end);
	int len = (int)file.tellg();
	file.seekg(0, std::ios::beg);

	unsigned char* fileData = new unsigned char[len];
	file.read((char*)fileData, len);
	file.close();
	bool result = LoadFromMemory(fileData, len);
	delete[] fileData;
	return result;
}
bool WavFile::LoadFromMemory(const void* data, int size)
{
	const uint8_t* fileData = (const uint8_t*)data;
	if (this->data) delete[] this->data;
	this->data = nullptr;

	std::string header(fileData, fileData + 4);
	if (header != "RIFF") return false;	// file is not a wav file

	std::string format(fileData + 8, fileData + 12);

	int dataIdx = GetIndexOfChunk(fileData, size, "data", 12);
	int fmtIdx = GetIndexOfChunk(fileData, size, "fmt ", 12);
	

	if (dataIdx == -1 || fmtIdx == -1 || format != "WAVE")
		return false;

	
	int f = fmtIdx;
	std::string fmtChunk(fileData + f, fileData + f + 4);

	uint16_t audioFormat = TwoBytesToInt(fileData, f + 8);
	uint16_t fileNumChannels = TwoBytesToInt(fileData, f + 10);
	
	int fileSampleRate = FourBytesToInt(fileData, f + 12);
	uint32_t numBytesPerSecond = FourBytesToInt(fileData, f + 16);
	uint16_t numBytesPerBlock = TwoBytesToInt(fileData, f + 20);
	int fileBitDepth = (int)TwoBytesToInt(fileData, f + 22);
	
	uint16_t numBytesPerSample = (uint16_t)fileBitDepth / 8;

	if (audioFormat != WavAudioFormat::PCM && audioFormat != WavAudioFormat::IEEEFloat && audioFormat != WavAudioFormat::Extensible) return false; // format not supported
	if (fileNumChannels < 1 || fileNumChannels > 128) return false;	// num channels invalid
	if (numBytesPerSecond != (uint32_t)((fileNumChannels * fileSampleRate * fileBitDepth) / 8) || numBytesPerBlock != (fileNumChannels * numBytesPerSample)) return false; // inconsistent
	if (fileBitDepth != 8 && fileBitDepth != 16 && fileBitDepth != 24 && fileBitDepth != 32) return false;	// invalid bit Depth
	
	int d = dataIdx;
	std::string dataChunk(fileData + d, fileData + d + 4);
	int dataChunkSize = FourBytesToInt(fileData, d + 4);
	int fileNumSamples = dataChunkSize / (fileNumChannels * fileBitDepth / 8);
	int samplesStartIndex = d + 8;
	this->numSamples = fileNumSamples;

	bool needsResampling = false;
	float sampleDiff = 1.0f;
	if (this->sampleRate != fileSampleRate)
	{
		sampleDiff = (float)fileSampleRate / (float)sampleRate;
		needsResampling = true;
		this->numSamples = (int)((float)numSamples / sampleDiff);
	}
	
	this->data = new float[numSamples * numChannels];
	memset(this->data, 0, sizeof(float) * numSamples * numChannels);

	bool finished = false;
	for (int i = 0; i < numSamples && !finished; i++)
	{
		for (int channel = 0; channel < std::min(numChannels, (int)fileNumChannels); channel++)
		{
			int writeIdx = i * numChannels + channel;
			float accurateSampleIndex = i * sampleDiff;
			int sampleIndex = samplesStartIndex + (numBytesPerBlock * i) + channel * numBytesPerSample;

			if ((sampleIndex + (fileBitDepth / 8) - 1) >= size) {
				finished = true;
				break;
			}
			float curSample = 0.0f;
			if (fileBitDepth == 8)
			{
				curSample = ByteToSample(fileData[sampleIndex]);
			}
			else if (fileBitDepth == 16)
			{
				int16_t sampleAsShort = TwoBytesToInt(fileData, sampleIndex);
				curSample = ShortToSample(sampleAsShort);
			}
			else if (fileBitDepth == 24)
			{
				int32_t intLowHigh = (fileData[sampleIndex + 2] << 16) | (fileData[sampleIndex + 1] << 8) | fileData[sampleIndex];
				if (intLowHigh & 0x800000)
					intLowHigh = intLowHigh | ~0xFFFFFF;
				curSample = (float)intLowHigh / (float)0x800000;
			}
			else if (fileBitDepth == 32)
			{
				int intLowHigh = FourBytesToInt(fileData, sampleIndex);
				curSample = IntToSample(intLowHigh);
			}
			this->data[writeIdx] = curSample;

			if (fileNumChannels == 1 && numChannels > 1)
			{
				for (int k = 1; k < numChannels; k++)
				{
					this->data[writeIdx + k] = this->data[writeIdx];
				}
			}

		}
	}

	return true;
}

int WavFile::GetNumChannels() const
{
	return numChannels;
}
int WavFile::GetNumSamples() const
{
	return numSamples;
}
const float* WavFile::GetData() const
{
	return data;
}
