#pragma once

#include <cstdint>
#include <limits>

namespace Sonar
{
	// Generic integer types
	using I8 = std::int8_t;
	using U8 = std::uint8_t;
	using I16 = std::int16_t;
	using U16 = std::uint16_t;
	using I32 = std::int32_t;
	using U32 = std::uint32_t;
	using I64 = std::int64_t;
	using U64 = std::uint64_t;
	using Size = size_t;
	using F32 = float;
	using F64 = double;
	using Char = char;

	// Sound specific types
	using Sample = F32;

	// Converting between types
	// TODO: move to .cpp in debug
	constexpr Sample ToSample(I16 integerSample)
	{
		constexpr Sample PositiveLimit = static_cast<Sample>(std::numeric_limits<I16>::max());
		constexpr Sample NegativeLimit = -static_cast<Sample>(std::numeric_limits<I16>::min());
		const Sample asFloat = static_cast<Sample>(integerSample);
		const Sample signMaximum = ((asFloat < 0.f) ? NegativeLimit : PositiveLimit);
		const Sample normalized = asFloat / signMaximum;
		return(normalized);
	}

	// Clamp value to another range
	template<typename Source, typename Target>
	constexpr Target Saturate(Source value)
	{
		constexpr Target Maximum = std::numeric_limits<Target>::max();
		constexpr Target Minimum = std::numeric_limits<Target>::min();
		return ((value > Maximum) ? Maximum : ((value < Minimum) ? Minimum : value));
	}

	// Assert method
	inline void Assert(bool condition)
	{
		if(!condition)
		{
			__debugbreak();
		}
	}

}
