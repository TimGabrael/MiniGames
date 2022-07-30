#pragma once

struct Data
{
	float channel1; float channel2;
};

template<int BufferSize>
struct AudioBuffer
{
	AudioBuffer() { memset(ringBuf, 0, sizeof(Data) * BufferSize); }

	Data GetNext();

	// returns number of floats written to buffer
	int AddToBuffer(const float* audio, int numSamples, int numChannels);
	int AddToBuffer(float* audioC1, float* audioC2, int size);

private:

	Data ringBuf[BufferSize];

	int writeIdx = 0;
	int readIdx = 0;
};









template<int BufferSize>
Data AudioBuffer<BufferSize>::GetNext()
{
	Data output{ 0.0f, 0.0f };
	int diff = writeIdx - readIdx;
	if (diff == 0) return output;
	if (writeIdx < readIdx)
	{
		diff = (diff + BufferSize) % BufferSize;
	}
	// float attenuation = std::max(1.0f / 4.0f * diff ;

	output = ringBuf[readIdx];
	readIdx = (readIdx + 1) % BufferSize;
	return output;
}

template<int BufferSize>
int AudioBuffer<BufferSize>::AddToBuffer(const float* audio, int numSamples, int numChannels)
{
	int endIdx = readIdx - 1;
	if (readIdx == 0) endIdx = BufferSize - 2;

	int numWritten = 0;
	int curIdx = writeIdx;
	for (numWritten = 0; numWritten < numSamples && curIdx != endIdx; numWritten++)
	{
		curIdx = (writeIdx + numWritten) % BufferSize;
		if (numChannels < 2)
		{
			ringBuf[curIdx].channel1 = audio[numWritten];
			ringBuf[curIdx].channel2 = audio[numWritten];
		}
		else
		{
			ringBuf[curIdx].channel1 = audio[numChannels * numWritten];
			ringBuf[curIdx].channel2 = audio[numChannels * numWritten + 1];
		}
	}
	writeIdx = (writeIdx + numWritten) % BufferSize;

	return numWritten;
}
template<int BufferSize>
int AudioBuffer<BufferSize>::AddToBuffer(float* audioC1, float* audioC2, int size)
{
	int endIdx = readIdx - 1;
	if (readIdx == 0) endIdx = BufferSize - 2;

	int numWritten = 0;
	int curIdx = writeIdx;
	for (numWritten = 0; numWritten < size && curIdx != endIdx; numWritten++)
	{
		curIdx = (writeIdx + numWritten) % BufferSize;

		ringBuf[curIdx].channel1 = audioC1[numWritten];
		ringBuf[curIdx].channel2 = audioC2[numWritten];
	}
	writeIdx = (writeIdx + numWritten) % BufferSize;

	return numWritten;
}