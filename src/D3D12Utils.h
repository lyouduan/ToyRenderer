#pragma once
#include <stdint.h>

// Aligns a value to the nearest higher multiple of 'Alignment'.
inline uint32_t AlignArbitrary(uint32_t Val, uint32_t Alignment)
{
	return ((Val + Alignment - 1) / Alignment) * Alignment;
}

