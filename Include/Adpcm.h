#pragma once

#include "Common.h"

namespace Adpcm
{

	// Structure for indicating how a block of ADPCM is parsed
	#pragma pack(push, 1)
	struct Preamble
	{
		Sonar::U8 predictor;
		Sonar::I16 initialDelta;
		Sonar::I16 currentSample;
		Sonar::I16 previousSample;
	};
	#pragma pack(pop)
	constexpr Sonar::U8 DefaultPredictorsCount = 7;

	// Structure and constants representing data bits for expanding samples
	// High 4 bits are first, low 4 bits are second.
	using Nibble = Sonar::U8;
	constexpr Nibble NibbleBitSize = 4;
	constexpr Nibble SecondNibbleMask = 0xF;
	constexpr Nibble MaximumNibbleValue = 0xF;
	constexpr Nibble NibbleValueCount = MaximumNibbleValue + 1;

	// Tables used for parsing
	constexpr Sonar::I16 AdaptationTable[NibbleValueCount] =
	{
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};
	constexpr Sonar::I16 CoefficientsA[DefaultPredictorsCount] =
	{
		256, 512, 0, 192, 240, 460, 392
	};
	constexpr Sonar::I16 CoefficientsB[DefaultPredictorsCount] =
	{
		0, -256, 0, 64, 0, -208, -232
	};

	// Sign-extend a 4-bit twos-complement value
	constexpr Sonar::I16 Extend(Nibble nibble)
	{
		constexpr Sonar::I16 nibbleSignExtendShift = (16 - NibbleBitSize);
		const Sonar::I16 as16 = static_cast<Sonar::I16>(nibble);
		const Sonar::I16 shifted16 = as16 << nibbleSignExtendShift;
		const Sonar::I16 unshifted16 = shifted16 >> nibbleSignExtendShift;
		return unshifted16;
	}

	// Helper to compute I16 sample given the ADPCM parameters
	constexpr Sonar::I16 ComputeSample
	(
		Nibble nibble,
		Sonar::I16 currentSample,
		Sonar::I16 previousSample,
		Sonar::I16 coefficientA,
		Sonar::I16 coefficientB,
		Sonar::I16 delta
	)
	{
		const Sonar::I16 extendedNibble = Extend(nibble);
		const Sonar::I32 factorA = static_cast<Sonar::I32>(currentSample) * coefficientA;
		const Sonar::I32 factorB = static_cast<Sonar::I32>(previousSample) * coefficientB;
		const Sonar::I32 basePredictor = (factorA + factorB) / 256;
		const Sonar::I32 predictor = static_cast<Sonar::I32>(basePredictor) + (delta * extendedNibble);
		const Sonar::I16 result = Sonar::Saturate<Sonar::I32, Sonar::I16>(predictor);
		return result;
	}

	constexpr Sonar::I16 ComputeDelta(Nibble nibble, Sonar::I16 oldDelta)
	{
		const Sonar::I16 delta = (AdaptationTable[nibble] * oldDelta) / 256;
		constexpr Sonar::I16 minimumDelta = 16;
		return ((delta < minimumDelta) ? minimumDelta : delta);
	}

	// Parse a stream of nibbles from a single block and fill out array of samples as float
	Sonar::Sample* ParseBlock
	(
		const Preamble& preamble,
		const Nibble* nibblesPairs,
		const Sonar::I16* extraCoefficients,
		Sonar::Size sampleCount,
		Sonar::Sample* output
	);

}
