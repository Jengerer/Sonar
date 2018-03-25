#include "Adpcm.h"

using namespace Adpcm;
using namespace Sonar;

Sample* Adpcm::ParseBlock
(
	const Preamble& preamble,
	const Nibble* nibblePairs,
	const I16* extraCoefficients,
	Size sampleCount,
	Sample* output
)
{
	I16 delta = preamble.initialDelta;
	I16 currentSample = preamble.currentSample;
	I16 previousSample = preamble.previousSample;
	
	// Output samples
	*output++ = ToSample(previousSample);
	*output++ = ToSample(currentSample);

	// Do the rest of the samples from the pairs
	const U8 predictor = preamble.predictor;
	const bool extraCoefficient = (predictor >= DefaultPredictorsCount);
	const U8 extraPredictorIndex = (predictor - DefaultPredictorsCount);
	const I16 coefficientA = (extraCoefficient ? extraCoefficients[extraPredictorIndex] : CoefficientsA[predictor]);
	const I16 coefficientB = (extraCoefficient ? extraCoefficients[extraPredictorIndex] : CoefficientsB[predictor]);
	const Size nibblePairCount = sampleCount / 2;
	for(Size i = 0; i < nibblePairCount; ++i)
	{
		const Nibble pair = nibblePairs[i];
		const Nibble nibbleA = pair >> NibbleBitSize;
		const I16 nibbleSampleA = ComputeSample(nibbleA, currentSample, previousSample, coefficientA, coefficientB, delta);
		const I16 deltaA = ComputeDelta(nibbleA, delta);
		*output++ = ToSample(nibbleSampleA);

		// Now compute next sample
		const Nibble nibbleB = pair & SecondNibbleMask;
		const I16 nibbleSampleB = ComputeSample(nibbleB, nibbleSampleA, currentSample, coefficientA, coefficientB, deltaA);
		delta = ComputeDelta(nibbleB, deltaA);
		*output++ = ToSample(nibbleSampleB);

		// Replace samples used for next nibble pair
		currentSample = nibbleSampleB;
		previousSample = nibbleSampleA;
	}

	return(output);
}
