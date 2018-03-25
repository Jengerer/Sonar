#include "Adpcm.h"
#include "Common.h"
#include "File.h"
#include "Wav.h"

#include <Audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <iostream>

using namespace Sonar;
using namespace std;

// Sine state
const float Pi = 3.14159265f;
const float TwoPi = 2.f * Pi;
const float ToneFrequency = 50.f;
const float AmplitudeFrequency = 0.5f;
float time = 0.f;
float sineTimeRate = TwoPi;

// Sample state
Sample* samples = nullptr;
Size sampleCount = 0;
Size sampleIndex = 0;

// Global device state
UINT32 bufferFrameCount;
WORD channelCount;

// Fill buffer
bool FillBuffer(IAudioClient* audioClient, IAudioRenderClient* renderClient)
{
	// Get current padding
	UINT32 paddingFrames;
	HRESULT result = audioClient->GetCurrentPadding(&paddingFrames);
	if(FAILED(result))
	{
		cout << "Failed to get padding frames.\n";
		return(false);
	}

	// Should always be one available
	const UINT32 availableFrames = bufferFrameCount - paddingFrames;
	if(availableFrames == 0)
	{
		cout << "No available frames to fill.\n";
		return(false);
	}

	BYTE* data;
	result = renderClient->GetBuffer(availableFrames, &data);
	if(FAILED(result))
	{
		cout << "Failed to get buffer.\n";
		return(false);
	}

	// Fill buffer
	const size_t floatsPerFrame = channelCount;
	const size_t frameEndIndex = availableFrames * floatsPerFrame;
	float* const bufferBegin = reinterpret_cast<float*>(data);
	float* const bufferEnd = bufferBegin + frameEndIndex;
	for (float* frameStart = bufferBegin; frameStart != bufferEnd; frameStart += floatsPerFrame)
	{
		const float amplitude = 1.f;
		const float frequency = ToneFrequency;
		const float sample = amplitude * sinf(time * frequency);
		time += sineTimeRate;
		if (time >= TwoPi)
		{
			time -= TwoPi;
		}

		const Sample adpcmSample = samples[sampleIndex];
		sampleIndex = (sampleIndex + 1) % sampleCount;

		// Copy sample to all frames
		float* const sampleEnd = frameStart + floatsPerFrame;
		for (float* channelIt = frameStart; channelIt != sampleEnd; ++channelIt)
		{
			*channelIt = adpcmSample;
		}
	}

	// Load buffer back
	DWORD flags = 0;
	result = renderClient->ReleaseBuffer(availableFrames, flags);
	if(FAILED(result))
	{
		cout << "Failed to release buffer.\n";
		return(false);
	}

	cout << "Filled " << availableFrames << " frames.\n";
	return(true);
}

bool LoadSamples()
{
	constexpr Size HeaderSamples = 2;
	constexpr Size SamplesPerByte = 2;

	// Read data
	U8* bytes = Read("C:\\Users\\Jengerer\\Desktop\\tone_adpcm.wav");
	const Wav::Header* waveHeader = reinterpret_cast<Wav::Header*>(bytes);
	const U8* extraParameters = reinterpret_cast<const U8*>(waveHeader + 1);
	const I16* extraCoefficients = reinterpret_cast<const I16*>(extraParameters);
	const Wav::DataHeader* dataHeader = reinterpret_cast<const Wav::DataHeader*>(extraParameters + waveHeader->extraParametersSize);
	const U8* data = reinterpret_cast<const U8*>(dataHeader + 1);

	// Get format values
	const Size dataSize = dataHeader->dataSize;
	const Size blockAlign = waveHeader->blockAlign;
	const Size remainder = dataSize % blockAlign;
	if(remainder != 0)
	{
		cout << "Invalid data size! Remainder bytes: " << remainder << "\n";
	}
	const Size nibbleBytes = waveHeader->blockAlign - sizeof(Adpcm::Preamble);
	const Size samplesPerBlock = (nibbleBytes * SamplesPerByte) + HeaderSamples;
	const Size nibbleSampleCount = samplesPerBlock - HeaderSamples;
	const Size blockCount = dataSize / blockAlign;
	sampleCount = samplesPerBlock * blockCount;
	samples = new Sample[sampleCount];

	// Go through each block
	Sample* blockSamples = samples;
	const U8* blockEnd = data + dataSize;
	for(const U8* blockStart = data; blockStart != blockEnd; blockStart += blockAlign)
	{
		const Adpcm::Preamble* preamble = reinterpret_cast<const Adpcm::Preamble*>(blockStart);
		const Adpcm::Nibble* nibbles = reinterpret_cast<const Adpcm::Nibble*>(preamble + 1);
		blockSamples = Adpcm::ParseBlock(*preamble, nibbles, extraCoefficients, nibbleSampleCount, blockSamples);
	}
	return(true);
}

int main(int argc, char** argv)
{
	// Initialize COM
	HRESULT result = CoInitialize(NULL);
	if(FAILED(result))
	{
		cout << "Failed to initialize COM.\n";
		return(-1);
	}

	IMMDeviceEnumerator* deviceEnumerator;
	const CLSID DeviceEnumeratorClassId = __uuidof(MMDeviceEnumerator);
	const IID DeviceEnumeratorInterfaceId = __uuidof(IMMDeviceEnumerator);
	result = CoCreateInstance(DeviceEnumeratorClassId, NULL, CLSCTX_ALL, DeviceEnumeratorInterfaceId, (void**)&deviceEnumerator);
	if(FAILED(result))
	{
		cout << "Failed to create enumerator!\n";
		return(-1);
	}

	IMMDevice* defaultEndpoint;
	result = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultEndpoint);
	if(FAILED(result))
	{
		cout << "Failed to get default endpoint!\n";
		return(-1);
	}

	IAudioClient* audioClient;
	const IID AudioClientInterfaceId = __uuidof(IAudioClient);
	result = defaultEndpoint->Activate(AudioClientInterfaceId, CLSCTX_ALL, NULL, (void**)&audioClient);
	if(FAILED(result))
	{
		cout << "Failed to initialize audio client!\n";
		return(-1);
	}

	WAVEFORMATEX* mixFormat;
	result = audioClient->GetMixFormat(&mixFormat);
	if(FAILED(result))
	{
		cout << "Failed to get mix format!\n";
		return(-1);
	}
	channelCount = mixFormat->nChannels;
	sineTimeRate /= mixFormat->nSamplesPerSec;

	// Get period
	REFERENCE_TIME defaultPeriod;
	REFERENCE_TIME minimumPeriod;
	result = audioClient->GetDevicePeriod(&defaultPeriod, &minimumPeriod);
	if(FAILED(result))
	{
		cout << "Failed to get period.\n";
		return(-1);
	}

	const REFERENCE_TIME PeriodMultiplier = 10;
	const REFERENCE_TIME bufferDuration = defaultPeriod * PeriodMultiplier;
	GUID sessionId = GUID_NULL;
	result = audioClient->Initialize
	(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		bufferDuration,
		0,
		mixFormat,
		&sessionId
	);
	if(FAILED(result))
	{
		cout << "Failed to initialize audio client.\n";
		return(-1);
	}

	// Create event and register it
	HANDLE bufferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(!bufferEvent)
	{
		cout << "Failed to create event.\n";
		return -1;
	}
	audioClient->SetEventHandle(bufferEvent);

	// Create render client
	IAudioRenderClient* renderClient;
	const IID RenderClientInterfaceId = __uuidof(IAudioRenderClient);
	result = audioClient->GetService(RenderClientInterfaceId, (void**)&renderClient);
	if(FAILED(result))
	{
		cout << "Failed to get render client\n";
		return(-1);
	}

	// Get buffer size
	result = audioClient->GetBufferSize(&bufferFrameCount);
	if(FAILED(result))
	{
		cout << "Failed to get buffer frame count!\n";
		return(-1);
	}

	// Load the file
	if(!LoadSamples())
	{
		cout << "Failed to load samples.\n";
		return(-1);
	}

	// Do first fill
	if(!FillBuffer(audioClient, renderClient))
	{
		cout << "Failed to perform first buffer fill.\n";
		return(-1);
	}

	// Start!
	result = audioClient->Start();
	if(FAILED(result))
	{
		cout << "Failed to start audio client!\n";
		return(-1);
	}

	// Loop to fill buffer
	size_t buffersFilled = 0;
	const size_t buffersRequired = 10000;
	const UINT32 bufferFrameThreshold = (bufferFrameCount * 3) / 4;
	while (buffersFilled < buffersRequired)
	{
		// Wait for event
		DWORD waitResult = WaitForSingleObject(bufferEvent, INFINITE);
		if(waitResult != WAIT_OBJECT_0)
		{
			cout << "Failed to wait!\n";
			return(-1);
		}

		// We did it!
		if(!FillBuffer(audioClient, renderClient))
		{
			cout << "Failed to fill buffer #" << buffersFilled << "\n";
			return(-1);
		}
		++buffersFilled;
	}

	cout << "Finished!\n";
	return(0);
}