// FastBC6HEncoder (https://github.com/thennequin/UtilsCollection/FastBC6HEncoder)
// Single library for fast BC6H compression on CPU
// Adapted from GPU Realtime BC6H (https://github.com/knarkowicz/GPURealTimeBC6H/)

// Use '#define FASTBC6HENCODER_IMPLEMENTATION' before including to create the implementation

// Perfomances
// Encode a 4096 * 2048 texture under 5 seconds on a AMD 3900x (monothread)
// and under 0.5 second with OpenMP multithreading against 5 seconds for Compressonator

#ifndef __FASTBC6HENCODER_HEADER__
#define __FASTBC6HENCODER_HEADER__

// block : uint8_t[16]
// texels : float[16 * 3]
void EncodeBC6H_Fast(void* block, float& blockMSLE, const float* texels);
void EncodeBC6H_Quality(void* block, float& blockMSLE, const float* texels);

#endif // __FASTBC6HENCODER_HEADER__

////////////////////////////////////////////
////////////// End of header ///////////////
////////////////////////////////////////////

#ifdef FASTBC6HENCODER_IMPLEMENTATION
////////////////////////////////////////////

// Improve quality at small performance loss
#define INSET_COLOR_BBOX 1
#define OPTIMIZE_ENDPOINTS 1

// Whether to optimize for luminance error or for RGB error
#define LUMINANCE_WEIGHTS 1

////////////////////////////////////////////

#include <stdint.h>
#include <math.h>

#define floor floorf
#define sqrt sqrtf
#define abs fabs

struct float3
{
	float x, y, z;

	float3(float f)
	{
		x = f;
		y = f;
		z = f;
	}

	float3(float fX, float fY, float fZ)
	{
		x = fX;
		y = fY;
		z = fZ;
	}

	float3(const float3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	bool operator == (const float3& b) const
	{
		return x == b.x
			&& y == b.y
			&& z == b.z;
	}

	float3 operator *(float f) const
	{
		return { x * f, y * f, z * f };
	}

	float3 operator *(const float3& f) const
	{
		return { x * f.x, y * f.y, z * f.z };
	}

	float3 operator /(float f) const
	{
		return { x / f, y / f, z / f };
	}

	float3 operator /(const float3& f) const
	{
		return { x / f.x, y / f.y, z / f.z };
	}

	float3 operator +(float f) const
	{
		return { x + f, y + f, z + f };
	}

	float3 operator +(const float3& f) const
	{
		return { x + f.x, y + f.y, z + f.z };
	}

	float3 operator -(float f) const
	{
		return { x - f, y - f, z - f };
	}

	float3 operator -(const float3& f) const
	{
		return { x - f.x, y - f.y, z - f.z };
	}

	float3& operator *=(const float3& f)
	{
		*this = *this + f;
		return *this;
	}

	float3& operator /=(const float3& f)
	{
		*this = *this / f;
		return *this;
	}

	float3& operator +=(const float3& f)
	{
		*this = *this + f;
		return *this;
	}

	float3& operator -=(const float3& f)
	{
		*this = *this + f;
		return *this;
	}
};

float3 operator +(float left, const float3& right)
{
	return float3(left + right.x, left + right.y, left + right.z);
}

float3 operator -(float left, const float3& right)
{
	return float3(left - right.x, left - right.y, left - right.z);
}

float3 operator *(float left, const float3& right)
{
	return float3(left * right.x, left * right.y, left * right.z);
}

float3 operator /(float left, const float3& right)
{
	return float3(left / right.x, left / right.y, left / right.z);
}

float dot(const float3& a, const float3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float3 normalize(const float3& v)
{
	return v / sqrt(dot(v, v));
}

float f16tof32(uint16_t iHalfValue)
{
	uint32_t iMantissa = (uint32_t)(iHalfValue & 0x03FF);

	uint32_t iExponent = (iHalfValue & 0x7C00);
	if (iExponent == 0x7C00) // INF/NAN
	{
		iExponent = (uint32_t)0x8f;
	}
	else if (iExponent != 0)  // The value is normalized
	{
		iExponent = (uint32_t)((iHalfValue >> 10) & 0x1F);
	}
	else if (iMantissa != 0)     // The value is denormalized
	{
		// Normalize the value in the resulting float
		iExponent = 1;

		do
		{
			iExponent--;
			iMantissa <<= 1;
		} while ((iMantissa & 0x0400) == 0);

		iMantissa &= 0x03FF;
	}
	else                        // The value is zero
	{
		iExponent = (uint32_t)-112;
	}

	uint32_t iResult = ((iHalfValue & 0x8000) << 16) | // Sign
		((iExponent + 112) << 23) | // Exponent
		(iMantissa << 13);          // Mantissa

	return reinterpret_cast<float*>(&iResult)[0];
}

uint16_t f32tof16(float fValue)
{
	uint32_t iResult;

	uint32_t iValue = reinterpret_cast<uint32_t*>(&fValue)[0];
	uint32_t Sign = (iValue & 0x80000000U) >> 16U;
	iValue = iValue & 0x7FFFFFFFU;      // Hack off the sign

	if (iValue > 0x477FE000U)
	{
		// The number is too large to be represented as a half.  Saturate to infinity.
		if (((iValue & 0x7F800000) == 0x7F800000) && ((iValue & 0x7FFFFF) != 0))
		{
			iResult = 0x7FFF; // NAN
		}
		else
		{
			iResult = 0x7C00U; // INF
		}
	}
	else
	{
		if (iValue < 0x38800000U)
		{
			// The number is too small to be represented as a normalized half.
			// Convert it to a denormalized value.
			uint32_t iShift = 113U - (iValue >> 23U);
			iValue = (0x800000U | (iValue & 0x7FFFFFU)) >> iShift;
		}
		else
		{
			// Rebias the exponent to represent the value as a normalized half.
			iValue += 0xC8000000U;
		}

		iResult = ((iValue + 0x0FFFU + ((iValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
	}
	return (uint16_t)(iResult | Sign);
}

float3 log2(const float3& v)
{
	return { log2f(v.x), log2f(v.y) , log2f(v.z) };
}

float3 exp2(const float3& v)
{
	return { exp2f(v.x), exp2f(v.y) , exp2f(v.z) };
}

float rcp(float f)
{
	return 1.f / f;
}

float clamp(float x, float fMin, float fMax)
{
	return (x < fMin) ? fMin : (x > fMax) ? fMax : x;
}

float saturate(float x)
{
	return clamp(x, 0.f, 1.f);
}

float3 saturate(const float3& v)
{
	return {
		clamp(v.x, 0.f, 1.f),
		clamp(v.y, 0.f, 1.f),
		clamp(v.z, 0.f, 1.f)
	};
}

float3 clamp(const float3& v, float fMin, float fMax)
{
	return {
		clamp(v.x, fMin, fMax),
		clamp(v.y, fMin, fMax),
		clamp(v.z, fMin, fMax)
	};
}

float3 clamp(const float3& v, const float3& fMin, const float3& fMax)
{
	return {
		clamp(v.x, fMin.x, fMax.x),
		clamp(v.y, fMin.y, fMax.y),
		clamp(v.z, fMin.z, fMax.z)
	};
}

float3 floor(const float3& v)
{
	return {
		floor(v.x),
		floor(v.y),
		floor(v.z)
	};
}

float min(float a, float b)
{
	return a < b ? a : b;
}


float max(float a, float b)
{
	return a > b ? a : b;
}


float3 min(const float3& a, const float3& b)
{
	return {
		min(a.x, b.x),
		min(a.y, b.y),
		min(a.z, b.z)
	};
}

float3 max(const float3& a, const float3& b)
{
	return {
		max(a.x, b.x),
		max(a.y, b.y),
		max(a.z, b.z)
	};
}

float3 f32tof16(float3 x)
{
	return { (float)f32tof16(x.x), (float)f32tof16(x.y), (float)f32tof16(x.z) };
}

float3 f16tof32(float3 x)
{
	return { f16tof32((uint16_t)x.x), f16tof32((uint16_t)x.y), f16tof32((uint16_t)x.z) };
}

////////////////////////////////////////////

static const float HALF_MAX = 65504.0f;
static const uint32_t PATTERN_NUM = 32;

float CalcMSLE(const float3& a, const float3& b)
{
	float3 delta = log2((b + 1.0f) / (a + 1.0f));
	float3 deltaSq = delta * delta;

#if LUMINANCE_WEIGHTS
	float3 luminanceWeights = float3(0.299f, 0.587f, 0.114f);
	deltaSq *= luminanceWeights;
#endif

	return deltaSq.x + deltaSq.y + deltaSq.z;
}

uint32_t PatternFixupID(uint32_t i)
{
	uint32_t ret = 15;
	ret = ((3441033216 >> i) & 0x1) ? 2 : ret;
	ret = ((845414400 >> i) & 0x1) ? 8 : ret;
	return ret;
}

uint32_t Pattern(uint32_t p, uint32_t i)
{
	uint32_t p2 = p / 2;
	uint32_t p3 = p - p2 * 2;

	uint32_t enc = 0;
	enc = p2 == 0 ? 2290666700 : enc;
	enc = p2 == 1 ? 3972591342 : enc;
	enc = p2 == 2 ? 4276930688 : enc;
	enc = p2 == 3 ? 3967876808 : enc;
	enc = p2 == 4 ? 4293707776 : enc;
	enc = p2 == 5 ? 3892379264 : enc;
	enc = p2 == 6 ? 4278255592 : enc;
	enc = p2 == 7 ? 4026597360 : enc;
	enc = p2 == 8 ? 9369360 : enc;
	enc = p2 == 9 ? 147747072 : enc;
	enc = p2 == 10 ? 1930428556 : enc;
	enc = p2 == 11 ? 2362323200 : enc;
	enc = p2 == 12 ? 823134348 : enc;
	enc = p2 == 13 ? 913073766 : enc;
	enc = p2 == 14 ? 267393000 : enc;
	enc = p2 == 15 ? 966553998 : enc;

	enc = p3 ? enc >> 16 : enc;
	uint32_t ret = (enc >> i) & 0x1;
	return ret;
}

float3 Quantize7(float3 x)
{
	return (f32tof16(x) * 128.0f) / (0x7bff + 1.0f);
}

float3 Quantize9(float3 x)
{
	return (f32tof16(x) * 512.0f) / (0x7bff + 1.0f);
}

float3 Quantize10(float3 x)
{
	return (f32tof16(x) * 1024.0f) / (0x7bff + 1.0f);
}

float3 Unquantize7(float3 x)
{
	return (x * 65536.0f + 0x8000) / 128.0f;
}

float3 Unquantize9(float3 x)
{
	return (x * 65536.0f + 0x8000) / 512.0f;
}

float3 Unquantize10(float3 x)
{
	return (x * 65536.0f + 0x8000) / 1024.0f;
}

float3 FinishUnquantize(float3 endpoint0Unq, float3 endpoint1Unq, float weight)
{
	float3 comp = (endpoint0Unq * (64.0f - weight) + endpoint1Unq * weight + 32.0f) * (31.0f / 4096.0f);
	return f16tof32(comp);
}

void Swap(float3& a, float3& b)
{
	float3 tmp = a;
	a = b;
	b = tmp;
}

void Swap(float& a, float& b)
{
	float tmp = a;
	a = b;
	b = tmp;
}

uint32_t ComputeIndex3(float texelPos, float endPoint0Pos, float endPoint1Pos)
{
	float r = (texelPos - endPoint0Pos) / (endPoint1Pos - endPoint0Pos);
	return (uint32_t) clamp(r * 6.98182f + 0.00909f + 0.5f, 0.0f, 7.0f);
}

uint32_t ComputeIndex4(float texelPos, float endPoint0Pos, float endPoint1Pos)
{
	float r = (texelPos - endPoint0Pos) / (endPoint1Pos - endPoint0Pos);
	return (uint32_t)clamp(r * 14.93333f + 0.03333f + 0.5f, 0.0f, 15.0f);
}

void SignExtend(float3& v1, uint32_t mask, uint32_t signFlag)
{
	int v[3] = { (int)v1.x, (int)v1.y, (int)v1.z };
	v[0] = (v[0] & mask) | (v[0] < 0 ? signFlag : 0);
	v[1] = (v[1] & mask) | (v[1] < 0 ? signFlag : 0);
	v[2] = (v[2] & mask) | (v[2] < 0 ? signFlag : 0);
	v1 = { (float)v[0], (float)v[1], (float)v[2] };
}

// Refine endpoints by insetting bounding box in log2 RGB space
void InsetColorBBoxP1(const float3 texels[16], float3& blockMin, float3& blockMax)
{
	float3 refinedBlockMin = blockMax;
	float3 refinedBlockMax = blockMin;

	for (uint32_t i = 0; i < 16; ++i)
	{
		refinedBlockMin = min(refinedBlockMin, texels[i] == blockMin ? refinedBlockMin : texels[i]);
		refinedBlockMax = max(refinedBlockMax, texels[i] == blockMax ? refinedBlockMax : texels[i]);
	}

	float3 logRefinedBlockMax = log2(refinedBlockMax + 1.0f);
	float3 logRefinedBlockMin = log2(refinedBlockMin + 1.0f);

	float3 logBlockMax = log2(blockMax + 1.0f);
	float3 logBlockMin = log2(blockMin + 1.0f);
	float3 logBlockMaxExt = (logBlockMax - logBlockMin) * (1.0f / 32.0f);

	logBlockMin += min(logRefinedBlockMin - logBlockMin, logBlockMaxExt);
	logBlockMax -= min(logBlockMax - logRefinedBlockMax, logBlockMaxExt);

	blockMin = exp2(logBlockMin) - 1.0f;
	blockMax = exp2(logBlockMax) - 1.0f;
}

// Refine endpoints by insetting bounding box in log2 RGB space
void InsetColorBBoxP2(const float3 texels[16], uint32_t pattern, uint32_t patternSelector, float3& blockMin, float3& blockMax)
{
	float3 refinedBlockMin = blockMax;
	float3 refinedBlockMax = blockMin;

	for (uint32_t i = 0; i < 16; ++i)
	{
		uint32_t paletteID = Pattern(pattern, i);
		if (paletteID == patternSelector)
		{
			refinedBlockMin = min(refinedBlockMin, texels[i] == blockMin ? refinedBlockMin : texels[i]);
			refinedBlockMax = max(refinedBlockMax, texels[i] == blockMax ? refinedBlockMax : texels[i]);
		}
	}

	float3 logRefinedBlockMax = log2(refinedBlockMax + 1.0f);
	float3 logRefinedBlockMin = log2(refinedBlockMin + 1.0f);

	float3 logBlockMax = log2(blockMax + 1.0f);
	float3 logBlockMin = log2(blockMin + 1.0f);
	float3 logBlockMaxExt = (logBlockMax - logBlockMin) * (1.0f / 32.0f);

	logBlockMin += min(logRefinedBlockMin - logBlockMin, logBlockMaxExt);
	logBlockMax -= min(logBlockMax - logRefinedBlockMax, logBlockMaxExt);

	blockMin = exp2(logBlockMin) - 1.0f;
	blockMax = exp2(logBlockMax) - 1.0f;
}

// Least squares optimization to find best endpoints for the selected block indices
void OptimizeEndpointsP1(const float3 texels[16], float3& blockMin, float3& blockMax, const float3& blockMinNonInset, const float3& blockMaxNonInset)
{
	float3 blockDir = blockMax - blockMin;
	blockDir = blockDir / (blockDir.x + blockDir.y + blockDir.z);

	float endPoint0Pos = f32tof16(dot(blockMin, blockDir));
	float endPoint1Pos = f32tof16(dot(blockMax, blockDir));

	float3 alphaTexelSum = 0.0f;
	float3 betaTexelSum = 0.0f;
	float alphaBetaSum = 0.0f;
	float alphaSqSum = 0.0f;
	float betaSqSum = 0.0f;

	for (int i = 0; i < 16; i++)
	{
		float texelPos = f32tof16(dot(texels[i], blockDir));
		uint32_t texelIndex = ComputeIndex4(texelPos, endPoint0Pos, endPoint1Pos);

		float beta = saturate(texelIndex / 15.0f);
		float alpha = 1.0f - beta;

		float3 texelF16 = f32tof16(texels[i]);
		alphaTexelSum += alpha * texelF16;
		betaTexelSum += beta * texelF16;

		alphaBetaSum += alpha * beta;

		alphaSqSum += alpha * alpha;
		betaSqSum += beta * beta;
	}

	float det = alphaSqSum * betaSqSum - alphaBetaSum * alphaBetaSum;

	if (abs(det) > 0.00001f)
	{
		float detRcp = rcp(det);
		blockMin = clamp(f16tof32(clamp(detRcp * (alphaTexelSum * betaSqSum - betaTexelSum * alphaBetaSum), 0.0f, HALF_MAX)), blockMinNonInset, blockMaxNonInset);
		blockMax = clamp(f16tof32(clamp(detRcp * (betaTexelSum * alphaSqSum - alphaTexelSum * alphaBetaSum), 0.0f, HALF_MAX)), blockMinNonInset, blockMaxNonInset);
	}
}

// Least squares optimization to find best endpoints for the selected block indices
void OptimizeEndpointsP2(const float3 texels[16], uint32_t pattern, uint32_t patternSelector, float3& blockMin, float3& blockMax)
{
	float3 blockDir = blockMax - blockMin;
	blockDir = blockDir / (blockDir.x + blockDir.y + blockDir.z);

	float endPoint0Pos = f32tof16(dot(blockMin, blockDir));
	float endPoint1Pos = f32tof16(dot(blockMax, blockDir));

	float3 alphaTexelSum = 0.0f;
	float3 betaTexelSum = 0.0f;
	float alphaBetaSum = 0.0f;
	float alphaSqSum = 0.0f;
	float betaSqSum = 0.0f;

	for (int i = 0; i < 16; i++)
	{
		uint32_t paletteID = Pattern(pattern, i);
		if (paletteID == patternSelector)
		{
			float texelPos = f32tof16(dot(texels[i], blockDir));
			uint32_t texelIndex = ComputeIndex3(texelPos, endPoint0Pos, endPoint1Pos);

			float beta = saturate(texelIndex / 7.0f);
			float alpha = 1.0f - beta;

			float3 texelF16 = f32tof16(texels[i]);
			alphaTexelSum += alpha * texelF16;
			betaTexelSum += beta * texelF16;

			alphaBetaSum += alpha * beta;

			alphaSqSum += alpha * alpha;
			betaSqSum += beta * beta;
		}
	}

	float det = alphaSqSum * betaSqSum - alphaBetaSum * alphaBetaSum;

	if (abs(det) > 0.00001f)
	{
		float detRcp = rcp(det);
		blockMin = f16tof32(clamp(detRcp * (alphaTexelSum * betaSqSum - betaTexelSum * alphaBetaSum), 0.0f, HALF_MAX));
		blockMax = f16tof32(clamp(detRcp * (betaTexelSum * alphaSqSum - alphaTexelSum * alphaBetaSum), 0.0f, HALF_MAX));
	}
}

void EncodeBC6H_Fast(uint32_t block[4], float& blockMSLE, const float3 texels[16])
{
	// compute endpoints (min/max RGB bbox)
	float3 blockMin = texels[0];
	float3 blockMax = texels[0];
	for (uint32_t i = 1; i < 16; ++i)
	{
		blockMin = min(blockMin, texels[i]);
		blockMax = max(blockMax, texels[i]);
	}

	float3 blockMinNonInset = blockMin;
	float3 blockMaxNonInset = blockMax;
#if INSET_COLOR_BBOX
	InsetColorBBoxP1(texels, blockMin, blockMax);
#endif

#if OPTIMIZE_ENDPOINTS
	OptimizeEndpointsP1(texels, blockMin, blockMax, blockMinNonInset, blockMaxNonInset);
#endif


	float3 blockDir = blockMax - blockMin;
	blockDir = blockDir / (blockDir.x + blockDir.y + blockDir.z);

	float3 endpoint0 = Quantize10(blockMin);
	float3 endpoint1 = Quantize10(blockMax);
	float endPoint0Pos = f32tof16(dot(blockMin, blockDir));
	float endPoint1Pos = f32tof16(dot(blockMax, blockDir));

	// check if endpoint swap is required
	float fixupTexelPos = f32tof16(dot(texels[0], blockDir));
	uint32_t fixupIndex = ComputeIndex4(fixupTexelPos, endPoint0Pos, endPoint1Pos);
	if (fixupIndex > 7)
	{
		Swap(endPoint0Pos, endPoint1Pos);
		Swap(endpoint0, endpoint1);
	}

	// compute indices
	uint32_t indices[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (uint32_t i = 0; i < 16; ++i)
	{
		float texelPos = f32tof16(dot(texels[i], blockDir));
		indices[i] = ComputeIndex4(texelPos, endPoint0Pos, endPoint1Pos);
	}

	// compute compression error (MSLE)
	float3 endpoint0Unq = Unquantize10(endpoint0);
	float3 endpoint1Unq = Unquantize10(endpoint1);
	float msle = 0.0f;
	for (uint32_t i = 0; i < 16; ++i)
	{
		float weight = floor((indices[i] * 64.0f) / 15.0f + 0.5f);
		float3 texelUnc = FinishUnquantize(endpoint0Unq, endpoint1Unq, weight);

		msle += CalcMSLE(texels[i], texelUnc);
	}


	// encode block for mode 11
	blockMSLE = msle;
	block[0] = 0x03;
	block[1] = 0;
	block[2] = 0;
	block[3] = 0;

	// endpoints
	block[0] |= (uint32_t)endpoint0.x << 5;
	block[0] |= (uint32_t)endpoint0.y << 15;
	block[0] |= (uint32_t)endpoint0.z << 25;
	block[1] |= (uint32_t)endpoint0.z >> 7;
	block[1] |= (uint32_t)endpoint1.x << 3;
	block[1] |= (uint32_t)endpoint1.y << 13;
	block[1] |= (uint32_t)endpoint1.z << 23;
	block[2] |= (uint32_t)endpoint1.z >> 9;

	// indices
	block[2] |= indices[0] << 1;
	block[2] |= indices[1] << 4;
	block[2] |= indices[2] << 8;
	block[2] |= indices[3] << 12;
	block[2] |= indices[4] << 16;
	block[2] |= indices[5] << 20;
	block[2] |= indices[6] << 24;
	block[2] |= indices[7] << 28;
	block[3] |= indices[8] << 0;
	block[3] |= indices[9] << 4;
	block[3] |= indices[10] << 8;
	block[3] |= indices[11] << 12;
	block[3] |= indices[12] << 16;
	block[3] |= indices[13] << 20;
	block[3] |= indices[14] << 24;
	block[3] |= indices[15] << 28;
}

float DistToLineSq(float3 PointOnLine, float3 LineDirection, float3 Point)
{
	float3 w = Point - PointOnLine;
	float3 x = w - dot(w, LineDirection) * LineDirection;
	return dot(x, x);
}

// Evaluate how good is given P2 pattern for encoding current block
float EvaluateP2Pattern(int pattern, const float3 texels[16])
{
	float3 p0BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 p0BlockMax = float3(0.0f, 0.0f, 0.0f);
	float3 p1BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 p1BlockMax = float3(0.0f, 0.0f, 0.0f);

	for (uint32_t i = 0; i < 16; ++i)
	{
		uint32_t paletteID = Pattern(pattern, i);
		if (paletteID == 0)
		{
			p0BlockMin = min(p0BlockMin, texels[i]);
			p0BlockMax = max(p0BlockMax, texels[i]);
		}
		else
		{
			p1BlockMin = min(p1BlockMin, texels[i]);
			p1BlockMax = max(p1BlockMax, texels[i]);
		}
	}

	float3 p0BlockDir = normalize(p0BlockMax - p0BlockMin);
	float3 p1BlockDir = normalize(p1BlockMax - p1BlockMin);

	float sqDistanceFromLine = 0.0f;

	for (uint32_t i = 0; i < 16; ++i)
	{
		uint32_t paletteID = Pattern(pattern, i);
		if (paletteID == 0)
		{
			sqDistanceFromLine += DistToLineSq(p0BlockMin, p0BlockDir, texels[i]);
		}
		else
		{
			sqDistanceFromLine += DistToLineSq(p1BlockMin, p1BlockDir, texels[i]);
		}
	}

	return sqDistanceFromLine;
}

void EncodeP2Pattern(uint32_t* block, float& blockMSLE, int pattern, const float3 texels[16])
{
	float3 p0BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 p0BlockMax = float3(0.0f, 0.0f, 0.0f);
	float3 p1BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 p1BlockMax = float3(0.0f, 0.0f, 0.0f);

	for (uint32_t i = 0; i < 16; ++i)
	{
		uint32_t paletteID = Pattern(pattern, i);
		if (paletteID == 0)
		{
			p0BlockMin = min(p0BlockMin, texels[i]);
			p0BlockMax = max(p0BlockMax, texels[i]);
		}
		else
		{
			p1BlockMin = min(p1BlockMin, texels[i]);
			p1BlockMax = max(p1BlockMax, texels[i]);
		}
	}

#if INSET_COLOR_BBOX
	// Disabled because it was a negligible quality increase
	//InsetColorBBoxP2(texels, pattern, 0, p0BlockMin, p0BlockMax);
	//InsetColorBBoxP2(texels, pattern, 1, p1BlockMin, p1BlockMax);
#endif

#if OPTIMIZE_ENDPOINTS
	OptimizeEndpointsP2(texels, pattern, 0, p0BlockMin, p0BlockMax);
	OptimizeEndpointsP2(texels, pattern, 1, p1BlockMin, p1BlockMax);
#endif

	float3 p0BlockDir = p0BlockMax - p0BlockMin;
	float3 p1BlockDir = p1BlockMax - p1BlockMin;
	p0BlockDir = p0BlockDir / (p0BlockDir.x + p0BlockDir.y + p0BlockDir.z);
	p1BlockDir = p1BlockDir / (p1BlockDir.x + p1BlockDir.y + p1BlockDir.z);


	float p0Endpoint0Pos = f32tof16(dot(p0BlockMin, p0BlockDir));
	float p0Endpoint1Pos = f32tof16(dot(p0BlockMax, p0BlockDir));
	float p1Endpoint0Pos = f32tof16(dot(p1BlockMin, p1BlockDir));
	float p1Endpoint1Pos = f32tof16(dot(p1BlockMax, p1BlockDir));


	uint32_t fixupID = PatternFixupID(pattern);
	float p0FixupTexelPos = f32tof16(dot(texels[0], p0BlockDir));
	float p1FixupTexelPos = f32tof16(dot(texels[fixupID], p1BlockDir));
	uint32_t p0FixupIndex = ComputeIndex3(p0FixupTexelPos, p0Endpoint0Pos, p0Endpoint1Pos);
	uint32_t p1FixupIndex = ComputeIndex3(p1FixupTexelPos, p1Endpoint0Pos, p1Endpoint1Pos);
	if (p0FixupIndex > 3)
	{
		Swap(p0Endpoint0Pos, p0Endpoint1Pos);
		Swap(p0BlockMin, p0BlockMax);
	}
	if (p1FixupIndex > 3)
	{
		Swap(p1Endpoint0Pos, p1Endpoint1Pos);
		Swap(p1BlockMin, p1BlockMax);
	}

	uint32_t indices[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (uint32_t i = 0; i < 16; ++i)
	{
		float p0TexelPos = f32tof16(dot(texels[i], p0BlockDir));
		float p1TexelPos = f32tof16(dot(texels[i], p1BlockDir));
		uint32_t p0Index = ComputeIndex3(p0TexelPos, p0Endpoint0Pos, p0Endpoint1Pos);
		uint32_t p1Index = ComputeIndex3(p1TexelPos, p1Endpoint0Pos, p1Endpoint1Pos);

		uint32_t paletteID = Pattern(pattern, i);
		indices[i] = paletteID == 0 ? p0Index : p1Index;
	}

	float3 endpoint760 = floor(Quantize7(p0BlockMin));
	float3 endpoint761 = floor(Quantize7(p0BlockMax));
	float3 endpoint762 = floor(Quantize7(p1BlockMin));
	float3 endpoint763 = floor(Quantize7(p1BlockMax));

	float3 endpoint950 = floor(Quantize9(p0BlockMin));
	float3 endpoint951 = floor(Quantize9(p0BlockMax));
	float3 endpoint952 = floor(Quantize9(p1BlockMin));
	float3 endpoint953 = floor(Quantize9(p1BlockMax));

	endpoint761 = endpoint761 - endpoint760;
	endpoint762 = endpoint762 - endpoint760;
	endpoint763 = endpoint763 - endpoint760;

	endpoint951 = endpoint951 - endpoint950;
	endpoint952 = endpoint952 - endpoint950;
	endpoint953 = endpoint953 - endpoint950;

	const float maxVal76 = 0x1F;
	endpoint761 = clamp(endpoint761, -maxVal76, maxVal76);
	endpoint762 = clamp(endpoint762, -maxVal76, maxVal76);
	endpoint763 = clamp(endpoint763, -maxVal76, maxVal76);

	const float  maxVal95 = 0xF;
	endpoint951 = clamp(endpoint951, -maxVal95, maxVal95);
	endpoint952 = clamp(endpoint952, -maxVal95, maxVal95);
	endpoint953 = clamp(endpoint953, -maxVal95, maxVal95);

	float3 endpoint760Unq = Unquantize7(endpoint760);
	float3 endpoint761Unq = Unquantize7(endpoint760 + endpoint761);
	float3 endpoint762Unq = Unquantize7(endpoint760 + endpoint762);
	float3 endpoint763Unq = Unquantize7(endpoint760 + endpoint763);
	float3 endpoint950Unq = Unquantize9(endpoint950);
	float3 endpoint951Unq = Unquantize9(endpoint950 + endpoint951);
	float3 endpoint952Unq = Unquantize9(endpoint950 + endpoint952);
	float3 endpoint953Unq = Unquantize9(endpoint950 + endpoint953);

	float msle76 = 0.0f;
	float msle95 = 0.0f;
	for (uint32_t i = 0; i < 16; ++i)
	{
		uint32_t paletteID = Pattern(pattern, i);

		float3 tmp760Unq = paletteID == 0 ? endpoint760Unq : endpoint762Unq;
		float3 tmp761Unq = paletteID == 0 ? endpoint761Unq : endpoint763Unq;
		float3 tmp950Unq = paletteID == 0 ? endpoint950Unq : endpoint952Unq;
		float3 tmp951Unq = paletteID == 0 ? endpoint951Unq : endpoint953Unq;

		float weight = floor((indices[i] * 64.0f) / 7.0f + 0.5f);
		float3 texelUnc76 = FinishUnquantize(tmp760Unq, tmp761Unq, weight);
		float3 texelUnc95 = FinishUnquantize(tmp950Unq, tmp951Unq, weight);

		msle76 += CalcMSLE(texels[i], texelUnc76);
		msle95 += CalcMSLE(texels[i], texelUnc95);
	}

	SignExtend(endpoint761, 0x1F, 0x20);
	SignExtend(endpoint762, 0x1F, 0x20);
	SignExtend(endpoint763, 0x1F, 0x20);

	SignExtend(endpoint951, 0xF, 0x10);
	SignExtend(endpoint952, 0xF, 0x10);
	SignExtend(endpoint953, 0xF, 0x10);

	// encode block
	float p2MSLE = min(msle76, msle95);
	if (p2MSLE < blockMSLE)
	{
		blockMSLE = p2MSLE;
		block[0] = 0;
		block[1] = 0;
		block[2] = 0;
		block[3] = 0;

		if (p2MSLE == msle76)
		{
			// 7.6
			block[0] = 0x1;
			block[0] |= ((uint32_t) endpoint762.y & 0x20) >> 3;
			block[0] |= ((uint32_t) endpoint763.y & 0x10) >> 1;
			block[0] |= ((uint32_t) endpoint763.y & 0x20) >> 1;
			block[0] |= (uint32_t) endpoint760.x << 5;
			block[0] |= ((uint32_t) endpoint763.z & 0x01) << 12;
			block[0] |= ((uint32_t) endpoint763.z & 0x02) << 12;
			block[0] |= ((uint32_t) endpoint762.z & 0x10) << 10;
			block[0] |= (uint32_t) endpoint760.y << 15;
			block[0] |= ((uint32_t) endpoint762.z & 0x20) << 17;
			block[0] |= ((uint32_t) endpoint763.z & 0x04) << 21;
			block[0] |= ((uint32_t) endpoint762.y & 0x10) << 20;
			block[0] |= (uint32_t) endpoint760.z << 25;
			block[1] |= ((uint32_t) endpoint763.z & 0x08) >> 3;
			block[1] |= ((uint32_t) endpoint763.z & 0x20) >> 4;
			block[1] |= ((uint32_t) endpoint763.z & 0x10) >> 2;
			block[1] |= (uint32_t) endpoint761.x << 3;
			block[1] |= ((uint32_t) endpoint762.y & 0x0F) << 9;
			block[1] |= (uint32_t) endpoint761.y << 13;
			block[1] |= ((uint32_t) endpoint763.y & 0x0F) << 19;
			block[1] |= (uint32_t) endpoint761.z << 23;
			block[1] |= ((uint32_t) endpoint762.z & 0x07) << 29;
			block[2] |= ((uint32_t) endpoint762.z & 0x08) >> 3;
			block[2] |= (uint32_t) endpoint762.x << 1;
			block[2] |= (uint32_t) endpoint763.x << 7;
		}
		else
		{
			// 9.5
			block[0] = 0xE;
			block[0] |= (uint32_t) endpoint950.x << 5;
			block[0] |= ((uint32_t) endpoint952.z & 0x10) << 10;
			block[0] |= (uint32_t) endpoint950.y << 15;
			block[0] |= ((uint32_t) endpoint952.y & 0x10) << 20;
			block[0] |= (uint32_t) endpoint950.z << 25;
			block[1] |= (uint32_t) endpoint950.z >> 7;
			block[1] |= ((uint32_t) endpoint953.z & 0x10) >> 2;
			block[1] |= (uint32_t) endpoint951.x << 3;
			block[1] |= ((uint32_t) endpoint953.y & 0x10) << 4;
			block[1] |= ((uint32_t) endpoint952.y & 0x0F) << 9;
			block[1] |= (uint32_t) endpoint951.y << 13;
			block[1] |= ((uint32_t) endpoint953.z & 0x01) << 18;
			block[1] |= ((uint32_t) endpoint953.y & 0x0F) << 19;
			block[1] |= (uint32_t) endpoint951.z << 23;
			block[1] |= ((uint32_t) endpoint953.z & 0x02) << 27;
			block[1] |= (uint32_t) endpoint952.z << 29;
			block[2] |= ((uint32_t) endpoint952.z & 0x08) >> 3;
			block[2] |= (uint32_t) endpoint952.x << 1;
			block[2] |= ((uint32_t) endpoint953.z & 0x04) << 4;
			block[2] |= (uint32_t) endpoint953.x << 7;
			block[2] |= ((uint32_t) endpoint953.z & 0x08) << 9;
		}

		block[2] |= pattern << 13;
		uint32_t blockFixupID = PatternFixupID(pattern);
		if (blockFixupID == 15)
		{
			block[2] |= indices[0] << 18;
			block[2] |= indices[1] << 20;
			block[2] |= indices[2] << 23;
			block[2] |= indices[3] << 26;
			block[2] |= indices[4] << 29;
			block[3] |= indices[5] << 0;
			block[3] |= indices[6] << 3;
			block[3] |= indices[7] << 6;
			block[3] |= indices[8] << 9;
			block[3] |= indices[9] << 12;
			block[3] |= indices[10] << 15;
			block[3] |= indices[11] << 18;
			block[3] |= indices[12] << 21;
			block[3] |= indices[13] << 24;
			block[3] |= indices[14] << 27;
			block[3] |= indices[15] << 30;
		}
		else if (blockFixupID == 2)
		{
			block[2] |= indices[0] << 18;
			block[2] |= indices[1] << 20;
			block[2] |= indices[2] << 23;
			block[2] |= indices[3] << 25;
			block[2] |= indices[4] << 28;
			block[2] |= indices[5] << 31;
			block[3] |= indices[5] >> 1;
			block[3] |= indices[6] << 2;
			block[3] |= indices[7] << 5;
			block[3] |= indices[8] << 8;
			block[3] |= indices[9] << 11;
			block[3] |= indices[10] << 14;
			block[3] |= indices[11] << 17;
			block[3] |= indices[12] << 20;
			block[3] |= indices[13] << 23;
			block[3] |= indices[14] << 26;
			block[3] |= indices[15] << 29;
		}
		else
		{
			block[2] |= indices[0] << 18;
			block[2] |= indices[1] << 20;
			block[2] |= indices[2] << 23;
			block[2] |= indices[3] << 26;
			block[2] |= indices[4] << 29;
			block[3] |= indices[5] << 0;
			block[3] |= indices[6] << 3;
			block[3] |= indices[7] << 6;
			block[3] |= indices[8] << 9;
			block[3] |= indices[9] << 11;
			block[3] |= indices[10] << 14;
			block[3] |= indices[11] << 17;
			block[3] |= indices[12] << 20;
			block[3] |= indices[13] << 23;
			block[3] |= indices[14] << 26;
			block[3] |= indices[15] << 29;
		}
	}
}

void EncodeBC6H_Quality(uint32_t* block, float& blockMSLE, const float3 texels[16])
{
	EncodeBC6H_Fast(block, blockMSLE, texels);

	// First find pattern which is a best fit for a current block
	float bestScore = EvaluateP2Pattern(0, texels);
	uint32_t bestPattern = 0;

	for (uint32_t patternIndex = 1; patternIndex < 32; ++patternIndex)
	{
		float score = EvaluateP2Pattern(patternIndex, texels);
		if (score < bestScore)
		{
			bestPattern = patternIndex;
			bestScore = score;
		}
	}

	// Then encode it
	EncodeP2Pattern(block, blockMSLE, bestPattern, texels);
}

void EncodeBC6H_Fast(void* block, float& blockMSLE, const float* texels)
{
	EncodeBC6H_Fast((uint32_t*)block, blockMSLE, (const float3*)texels);
}

void EncodeBC6H_Quality(void* block, float& blockMSLE, const float* texels)
{
	EncodeBC6H_Quality((uint32_t*)block, blockMSLE, (const float3*)texels);
}

#endif //FASTBC6HENCODER_IMPLEMENTATION