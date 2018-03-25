#pragma once

#include "Common.h"

namespace Sonar
{

namespace Wav
{

	#pragma pack(push, 1)
	struct Header
	{
		Sonar::U32 riffTag;
		Sonar::U32 fileSize;
		Sonar::U32 waveTag;
		Sonar::U32 formatTag;
		Sonar::U32 formatLength;
		Sonar::U16 formatType;
		Sonar::U16 channelCount;
		Sonar::U32 sampleRate;
		Sonar::U32 byteRate;
		Sonar::U16 blockAlign;
		Sonar::U16 bitsPerSample;
		Sonar::U16 extraParametersSize;
	};
	struct DataHeader
	{
		Sonar::U32 dataTag;
		Sonar::U32 dataSize;
	};
	#pragma pack(pop)

	// Expected header values
	constexpr Sonar::U32 RiffTag = 'RIFF';
	constexpr Sonar::U32 WaveTag = 'WAVE';
	constexpr Sonar::U32 FormatTag = 'fmt\0';
	constexpr Sonar::U32 DataTag = 'data';

}

}
