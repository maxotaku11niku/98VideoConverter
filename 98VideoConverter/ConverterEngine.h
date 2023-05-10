// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022
// This is the 'heart' of the encoder. Many settings are currently hardcoded for my ease
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#pragma once

#include <math.h>

extern "C"
{
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"
#include "include/libavutil/hwcontext.h"
}


#define PC_98_WIDTH 640 //These parameters strictly only apply to the standard-resolution PC-9801 models
#define PC_98_HEIGHT 400
#define PC_98_BLOCKWIDTH 80
#define PC_98_WORDWIDTH 40
#define PC_98_RESOLUTION 256000
#define PC_98_VRAMSIZE 128000
#define PC_98_ONEPLANE_BYTE 32000
#define PC_98_ONEPLANE_WORD 16000
#define PC_98_FRAMERATE 56.4 //Yes, not quite 60 FPS
#define PC_98_ASPECT 1.6 //Width/Height

#define CD_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define CD_CLAMP_HIGH(x, high)  (((x) > (high)) ? (high) : (x))
#define CD_CLAMP_LOW(x, low)  (((x) < (low)) ? (low) : (x))

constexpr float odithermatrix2[4] = { -0.5f,   0.0f,
								     0.25f, -0.25f };

constexpr float odithermatrix4[16] =  { -0.5f,     0.0f,  -0.375f,   0.125f,
									   0.25f,   -0.25f,   0.375f,  -0.125f,
									-0.3125f,  0.1875f, -0.4375f,  0.0625f,
									 0.4375f, -0.0625f,  0.3125f, -0.1875f };

constexpr float odithermatrix8[64] =   { -0.5f,       0.0f,    -0.375f,     0.125f,  -0.46875f,   0.03125f,  -0.34375f,   0.15625f,
										 0.25f,     -0.25f,     0.375f,    -0.125f,   0.28125f,  -0.21875f,   0.40625f,  -0.09375f,
									  -0.3125f,    0.1875f,   -0.4375f,    0.0625f,  -0.28125f,   0.21875f,  -0.40625f,   0.09375f,
									   0.4375f,   -0.0625f,    0.3125f,   -0.1875f,   0.46875f,  -0.03125f,   0.34375f,  -0.15625f,
									-0.453125f,  0.046875f, -0.328125f,  0.171875f, -0.484375f,  0.015625f, -0.359375f,  0.140625f,
									 0.296875f, -0.203125f,  0.421875f, -0.078125f,  0.265625f, -0.234375f,  0.390625f, -0.109375f,
									-0.265625f,  0.234375f, -0.390625f,  0.109375f, -0.296875f,  0.203125f, -0.421875f,  0.078125f,
									 0.484375f, -0.015625f,  0.359375f, -0.140625f,  0.453125f, -0.046875f,  0.328125f, -0.171875f };

constexpr float YUVtoRGB[9] = { 1.0f,      0.0f,  1.13983f,
								1.0f, -0.39465f, -0.58060f,
								1.0f,  2.03211f,      0.0f };

constexpr float RGBtoYUV[9] = {    0.299f,    0.587f,    0.114f,
								-0.14713f, -0.28886f,    0.436f,
								   0.615f, -0.51499f, -0.10001f };

constexpr float YIQtoRGB[9] = { 1.0f,  0.955942f,  0.620796f,
								1.0f, -0.271990f, -0.647199f,
								1.0f, -1.106766f,  1.704271f };

constexpr float RGBtoYIQ[9] = { 0.299f,     0.587f,      0.114f,
							    0.595915f, -0.274583f,  -0.321338f,
							    0.211559f, -0.522742f,   0.311191f };

class VideoConverterEngine
{
public:
	VideoConverterEngine();

	void EncodeVideo(wchar_t* outFileName, bool (*progressCallback)(unsigned int) = NULL);
	void OpenForDecodeVideo(wchar_t* inFileName);
	unsigned char* GrabFrame(int framenumber);
	void CloseDecoder();
	inline int GetOrigFrameNumber() { return innumFrames; };
	inline unsigned char* GetConvertedImageData() { return convertedFrame; };
	inline unsigned char* GetSimulatedOutput() { return actualdisplaybuffer; };
	inline unsigned char* GetRescaledInputFrame() { return inputrescaledframe; };
	inline int GetBitrate() { return maxwordsperframe; };
	inline float GetDitherFactor() { return ditherfactor; };
	inline float GetSaturationDitherFactor() { return satditherfactor; };
	inline float GetHueDitherFactor() { return hueditherfactor; };
	inline float GetUVBias() { return uvbias; };
	inline float GetIBias() { return ibias; };
	inline int GetSampleRateSpec() { return sampleratespec; };
	inline bool GetIsHalfVerticalResolution() { return isHalfVerticalResolution; };
	inline int GetFrameSkip() { return frameskip; };
	inline void SetBitrate(int wpf) { maxwordsperframe = wpf; };
	inline void SetDitherFactor(float ditfac) { ditherfactor = ditfac; };
	inline void SetSaturationDitherFactor(float ditfac) { satditherfactor = ditfac; };
	inline void SetHueDitherFactor(float ditfac) { hueditherfactor = ditfac; };
	inline void SetUVBias(float uvb) { uvbias = uvb; };
	inline void SetIBias(float ib) { ibias = ib; };
	inline void SetSampleRateSpec(int spec) { sampleratespec = spec; };
	inline void SetIsHalfVerticalResolution(bool hvr) { isHalfVerticalResolution = hvr; };
	inline void SetFrameSkip(int fskip) { frameskip = fskip; };
private:
	inline unsigned int GetClosestColour(unsigned int argbcolour)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		float y = RGBtoYUV[0] * floatcol[1] + RGBtoYUV[1] * floatcol[2] + RGBtoYUV[2] * floatcol[3]; //Y
		float u = RGBtoYUV[3] * floatcol[1] + RGBtoYUV[4] * floatcol[2] + RGBtoYUV[5] * floatcol[3]; //U
		float v = RGBtoYUV[6] * floatcol[1] + RGBtoYUV[7] * floatcol[2] + RGBtoYUV[8] * floatcol[3]; //V
		float lowestDistance = 999999.0f; //No way it would be higher anyway
		float curDistance = 0.0f; //Actually distance squared but it doesn't matter
		int setIndex = 0;
		float ydif = 0.0f;
		float udif = 0.0f;
		float vdif = 0.0f;
		for (int i = 0; i < 16; i++)
		{
			ydif = (y - yuvpal[i][0]) * uvbias;
			udif = u - yuvpal[i][1];
			vdif = v - yuvpal[i][2];
			curDistance = ydif * ydif + udif * udif + vdif * vdif;//Square of the Euclidean norm
			if (curDistance < lowestDistance)
			{
				lowestDistance = curDistance;
				setIndex = i;
			}
		}
		return palette[setIndex];
	}

	inline unsigned int GetClosestColour(float r, float g, float b)
	{
		float lowestDistance = 999999.0f; //No way it would be higher anyway
		float curDistance = 0.0f; //Actually distance squared but it doesn't matter
		int setIndex = 0;
		float y = RGBtoYUV[0] * r + RGBtoYUV[1] * g + RGBtoYUV[2] * b; //Y
		float u = RGBtoYUV[3] * r + RGBtoYUV[4] * g + RGBtoYUV[5] * b; //U
		float v = RGBtoYUV[6] * r + RGBtoYUV[7] * g + RGBtoYUV[8] * b; //V
		float ydif = 0.0f;
		float udif = 0.0f;
		float vdif = 0.0f;
		for (int i = 0; i < 16; i++)
		{
			ydif = (y - yuvpal[i][0]) * uvbias;
			udif = u - yuvpal[i][1];
			vdif = v - yuvpal[i][2];
			curDistance = ydif * ydif + udif * udif + vdif * vdif;
			if (curDistance < lowestDistance)
			{
				lowestDistance = curDistance;
				setIndex = i;
			}
		}
		return palette[setIndex];
	}

	inline unsigned int GetClosestYUVColour(float y, float u, float v)
	{
		float lowestDistance = 999999.0f; //No way it would be higher anyway
		float curDistance = 0.0f; //Actually some sort of weird metric
		int setIndex = 0;
		float ydif = 0.0f;
		float udif = 0.0f;
		float vdif = 0.0f;
		for (int i = 0; i < 16; i++)
		{
			ydif = fabsf((y - yuvpal[i][0]) * uvbias);
			udif = u - yuvpal[i][1];
			vdif = v - yuvpal[i][2];
			curDistance = ydif + sqrtf(udif * udif + vdif * vdif);
			if (curDistance < lowestDistance)
			{
				lowestDistance = curDistance;
				setIndex = i;
			}
		}
		return palette[setIndex];
	}

	inline unsigned int GetClosestYIQColour(float y, float i, float q)
	{
		float lowestDistance = 999999.0f; //No way it would be higher anyway
		float curDistance = 0.0f; //Actually some sort of weird metric
		int setIndex = 0;
		float ydif = 0.0f;
		float idif = 0.0f;
		float qdif = 0.0f;
		for (int j = 0; j < 16; j++)
		{
			ydif = fabsf((y - yiqpal[j][0]) * uvbias);
			idif = i - yiqpal[j][1];
			qdif = (q - yiqpal[j][2]) * ibias;
			curDistance = ydif + sqrtf(idif * idif + qdif * qdif);
			if (curDistance < lowestDistance)
			{
				lowestDistance = curDistance;
				setIndex = j;
			}
		}
		return palette[setIndex];
	}

	inline unsigned int GetClosestColourAccel(unsigned int argbcolour)
	{
		argbcolour &= 0x00FFFFFF;
		return nearestcolours[argbcolour];
	}

	inline unsigned int GetClosestColourAccel(float r, float g, float b)
	{
		unsigned int argbcolour = (unsigned int)(b * 255.0f);
		argbcolour += ((unsigned int)(g * 255.0f)) << 8;
		argbcolour += ((unsigned int)(r * 255.0f)) << 16;
		argbcolour &= 0x00FFFFFF;
		return nearestcolours[argbcolour];
	}

	inline unsigned int GetClosestYUVColourAccel(float y, float u, float v)
	{
		float r = YUVtoRGB[0] * y + YUVtoRGB[1] * u + YUVtoRGB[2] * v;
		float g = YUVtoRGB[3] * y + YUVtoRGB[4] * u + YUVtoRGB[5] * v;
		float b = YUVtoRGB[6] * y + YUVtoRGB[7] * u + YUVtoRGB[8] * v;
		int temp = (int)(b * 255.0f);
		unsigned int argbcolour = (temp > 0xFF) ? 0xFF : ((temp < 0x00) ? 0x00 : temp);
		temp = (int)(g * 255.0f);
		argbcolour += (temp > 0xFF) ? 0xFF00 : ((temp < 0x00) ? 0x0000 : (temp << 8));
		temp = (int)(r * 255.0f);
		argbcolour += (temp > 0xFF) ? 0xFF0000 : ((temp < 0x00) ? 0x000000 : (temp << 16));
		return nearestcolours[argbcolour];
	}

	inline unsigned int OrderedDither2x2(unsigned int argbcolour, unsigned int x, unsigned int y)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		floatcol[1] += ditherfactor * odithermatrix2[(x & 0x01) + 2 * (y & 0x01)];
		floatcol[2] += ditherfactor * odithermatrix2[(x & 0x01) + 2 * (y & 0x01)];
		floatcol[3] += ditherfactor * odithermatrix2[(x & 0x01) + 2 * (y & 0x01)];
		return GetClosestColourAccel(floatcol[1], floatcol[2], floatcol[3]);
	}

	inline unsigned int OrderedDither4x4(unsigned int argbcolour, unsigned int x, unsigned int y)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		floatcol[1] += ditherfactor * odithermatrix4[(x & 0x03) + 4 * (y & 0x03)];
		floatcol[2] += ditherfactor * odithermatrix4[(x & 0x03) + 4 * (y & 0x03)];
		floatcol[3] += ditherfactor * odithermatrix4[(x & 0x03) + 4 * (y & 0x03)];
		return GetClosestColourAccel(floatcol[1], floatcol[2], floatcol[3]);
	}

	inline unsigned int OrderedDither8x8(unsigned int argbcolour, unsigned int x, unsigned int y)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		floatcol[1] += ditherfactor * odithermatrix8[(x & 0x07) + 8 * (y & 0x07)];
		floatcol[2] += ditherfactor * odithermatrix8[(x & 0x07) + 8 * (y & 0x07)];
		floatcol[3] += ditherfactor * odithermatrix8[(x & 0x07) + 8 * (y & 0x07)];
		return GetClosestColourAccel(floatcol[1], floatcol[2], floatcol[3]);
	}

	inline unsigned int OrderedDither8x8WithYUV(unsigned int argbcolour, unsigned int x, unsigned int y)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		float cy = RGBtoYUV[0] * floatcol[1] + RGBtoYUV[1] * floatcol[2] + RGBtoYUV[2] * floatcol[3]; //Y
		float u = RGBtoYUV[3] * floatcol[1] + RGBtoYUV[4] * floatcol[2] + RGBtoYUV[5] * floatcol[3]; //U
		float v = RGBtoYUV[6] * floatcol[1] + RGBtoYUV[7] * floatcol[2] + RGBtoYUV[8] * floatcol[3]; //V
		float sat = sqrtf(u * u + v * v);
		float hue = atan2f(v, u);
		cy += ditherfactor * odithermatrix8[(x & 0x07) + 8 * (y & 0x07)];
		sat -= satditherfactor * odithermatrix8[((x - 3) & 0x07) + 8 * ((y - 7) & 0x07)];
		hue += hueditherfactor * odithermatrix8[((x + 2) & 0x07) + 8 * ((y - 5) & 0x07)];
		u = sat * cosf(hue);
		v = sat * sinf(hue);
		return GetClosestYUVColour(cy, u, v);
	}

	inline unsigned int OrderedDither8x8WithYUVAccel(unsigned int argbcolour, unsigned int x, unsigned int y)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		float cy = RGBtoYUV[0] * floatcol[1] + RGBtoYUV[1] * floatcol[2] + RGBtoYUV[2] * floatcol[3]; //Y
		float u = RGBtoYUV[3] * floatcol[1] + RGBtoYUV[4] * floatcol[2] + RGBtoYUV[5] * floatcol[3]; //U
		float v = RGBtoYUV[6] * floatcol[1] + RGBtoYUV[7] * floatcol[2] + RGBtoYUV[8] * floatcol[3]; //V
		float sat = sqrtf(u * u + v * v);
		float hue = atan2f(v, u);
		cy += ditherfactor * odithermatrix8[(x & 0x07) + 8 * (y & 0x07)];
		sat -= satditherfactor * odithermatrix8[((x - 3) & 0x07) + 8 * ((y - 7) & 0x07)];
		hue += hueditherfactor * odithermatrix8[((x + 2) & 0x07) + 8 * ((y - 5) & 0x07)];
		u = sat * cosf(hue);
		v = sat * sinf(hue);
		return GetClosestYUVColourAccel(cy, u, v);
	}

	inline unsigned int OrderedDither8x8WithYIQ(unsigned int argbcolour, unsigned int x, unsigned int y)
	{
		float floatcol[4];
		floatcol[0] = ((float)((argbcolour & 0xFF000000) >> 24)) / 255.0f; //A
		floatcol[1] = ((float)((argbcolour & 0x00FF0000) >> 16)) / 255.0f; //R
		floatcol[2] = ((float)((argbcolour & 0x0000FF00) >> 8)) / 255.0f; //G
		floatcol[3] = ((float)(argbcolour & 0x000000FF)) / 255.0f; //B
		float cy = RGBtoYIQ[0] * floatcol[1] + RGBtoYIQ[1] * floatcol[2] + RGBtoYIQ[2] * floatcol[3]; //Y
		float i = RGBtoYIQ[3] * floatcol[1] + RGBtoYIQ[4] * floatcol[2] + RGBtoYIQ[5] * floatcol[3]; //I
		float q = RGBtoYIQ[6] * floatcol[1] + RGBtoYIQ[7] * floatcol[2] + RGBtoYIQ[8] * floatcol[3]; //Q
		float sat = sqrtf(i * i + q * q);
		float hue = atan2f(i, q);
		cy += ditherfactor * odithermatrix8[(x & 0x07) + 8 * (y & 0x07)];
		sat -= satditherfactor * odithermatrix8[((x - 3) & 0x07) + 8 * ((y - 7) & 0x07)];
		hue += hueditherfactor * odithermatrix8[((x + 2) & 0x07) + 8 * ((y - 5) & 0x07)];
		q = sat * cosf(hue);
		i = sat * sinf(hue);
		return GetClosestYIQColour(cy, i, q);
	}

	inline void RegenerateColourMap()
	{
		for (unsigned int i = 0; i < 0x01000000; i++)
		{
			nearestcolours[i] = GetClosestColour(i);
		}
	}

	inline int GetSampleRateSpec(float samplerate)
	{
		if (samplerate > 38587.5f)
		{
			return 0x00; //44100 Hz
		}
		else if (samplerate > 27562.5f)
		{
			return 0x01; //33075 Hz
		}
		else if (samplerate > 19293.75f)
		{
			return 0x02; //22050 Hz
		}
		else if (samplerate > 13781.25f)
		{
			return 0x03; //16537.5 Hz
		}
		else if (samplerate > 9647.5f)
		{
			return 0x04; //11025 Hz
		}
		else if (samplerate > 6895.0f)
		{
			return 0x05; //8270 Hz
		}
		else if (samplerate > 4825.0f)
		{
			return 0x06; //5520 Hz
		}
		else
		{
			return 0x07; //4130 Hz
		}
	}

	inline float GetRealSampleRateFromSpec(int spec)
	{
		switch (spec)
		{
		case 0x00: return 44100.0f;
		case 0x01: return 33075.0f;
		default:
		case 0x02: return 22050.0f;
		case 0x03: return 16537.5f;
		case 0x04: return 11025.0f;
		case 0x05: return 8270.0f;
		case 0x06: return 5520.0f;
		case 0x07: return 4130.0f;
		}
	}

	inline unsigned int GetNumChangedBits(unsigned short l, unsigned short r)
	{
		unsigned short xoredshort = l ^ r;
		unsigned int bitnum = 0;
		for (int i = 0; i < 16; i++)
		{
			bitnum += (xoredshort >> i) & 0x0001;
		}
		return bitnum;
	}

	void DitherScanline(unsigned int* startInCol, unsigned int* startOutCol);
	void CreatePlaneData();
	void ResetPlaneData();
	void CreateFullAudioStream();
	unsigned short* ProcessAudio(unsigned int* returnLength, double cutoffTime);
	void ProcessFrame(unsigned int* returnLength, unsigned int* returnuPlanes);
	void SimulateRealResult(unsigned int updateplanes, unsigned int* planesLength);
	static enum AVPixelFormat GetHWFormat(AVCodecContext* ctx, const enum AVPixelFormat* pixfmts);

	static enum AVPixelFormat staticPixFmt; //Part of a horrible kludge that only works because this is a singleton class
	AVFormatContext* fmtcontext;
	AVCodecContext* vidcodcontext;
	AVCodecContext* audcodcontext;
	AVBufferRef* hwcontext;
	enum AVHWDeviceType hwtype;
	bool hardwareaccel;
	SwsContext* scalercontext;
	SwsContext* scalercontexthalfheight;
	SwrContext* resamplercontext;
	AVStream* vidstream;
	AVStream* audstream;
	AVFrame* curFrame;
	AVFrame* curHWFrame;
	AVPacket* curPacket;
	AVPacket* nextPacket;
	int vidstreamIndex;
	int audstreamIndex;
	int inWidth;
	int inHeight;
	double inAspect;
	enum AVPixelFormat inPixFormat;
	int inAudioRate;
	int innumFrames;
	AVChannelLayout inChLayout;
	enum AVSampleFormat inAudFormat;

	float realoutsamplerate;
	int sampleratespec;
	AVSampleFormat outsampformat;
	AVChannelLayout outlayout;
	unsigned char** audorigData;
	unsigned char** audresData;
	int audorigLineSize;
	int audresLineSize;
	short* audfullstreamShort;
	unsigned char* audfullstreamByte;
	int curtotalByteStreamSize;
	bool isForcedBuzzerAudio;
	bool isHalfVerticalResolution;
	unsigned char inputrescaledframe[PC_98_RESOLUTION * 4];

	unsigned char* vidorigData[4];
	int vidorigLineSize[4];
	int convnumFrames;
	int frameskip;
	int outWidth;
	int outHeight;
	int topLeftX;
	int topLeftY;
	double actualFramerate;
	double actualFrametime;
	double totalTime;
	unsigned char* vidscaleData[4];
	int vidscaleLineSize[4];
	int vidorigBufsize;
	int vidscaleBufsize;
	unsigned char** planedata;
	unsigned char** prevplanedata;
	unsigned char* convertedFrame;
	unsigned short processedAudioData[65536];
	unsigned short** processedPlaneData;
	unsigned char actualdisplaybuffer[PC_98_RESOLUTION * 4];
	unsigned char actualdisplaycolind[PC_98_RESOLUTION];
	unsigned char** actualdisplayplanes;
	bool alreadyOpen;
	unsigned int palette[16];
	unsigned int nearestcolours[0x01000000];
	float floatpal[16][4];
	float yuvpal[16][3];
	float yiqpal[16][3];
	float uvbias;
	float ibias;
	unsigned int colind[640 * 400];
	unsigned int** matchesoffset;
	unsigned int** matcheslength;
	int** matchesimpact;
	bool** ismatchafill;
	unsigned int foundfills[8192];
	unsigned int foundcopies[8192];
	int impactarray[PC_98_ONEPLANE_WORD];
	bool isalreadydesignatedfill[PC_98_ONEPLANE_WORD];
	int minKeepLength;
	int minimpactperword;
	int minimpactperrun;
	int maxwordsperframe;
	float ditherfactor;
	float satditherfactor;
	float hueditherfactor;
};