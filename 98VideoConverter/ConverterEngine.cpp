// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022
// This is the 'heart' of the encoder. Many settings are currently hardcoded for my ease
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#include "ConverterEngine.h"
#include <string>
#include <locale>
#include <omp.h>
extern "C"
{
	#include "include/libavutil/imgutils.h"
	#include "include/libavutil/opt.h"
}

enum AVPixelFormat VideoConverterEngine::staticPixFmt; //bad kludge

const int outWidth = PC_98_WIDTH; //Fix output to 640x400, the total screen resolution of the PC-98
const int outHeight = PC_98_HEIGHT; //   AARRGGBB
const unsigned int basepalette[16] = { 0xFF111111, //This palette I believe has good colour coverage
								       0xFF771111,
                                       0xFF227733,
                                       0xFFDD4444, 
                                       0xFF3333BB,
									   0xFF33AAFF,
									   0xFF55DD55,
									   0xFF88FF55, 
									   0xFF777777,
									   0xFFBB33BB,
									   0xFFFF77FF,
									   0xFFFFBB77, 
									   0xFFCCBB33,
									   0xFF99FFFF,
									   0xFFFFFF66,
									   0xFFFFFFFF };

const unsigned int rgbpalette[16] =      {  0xFF000000, //This palette is a simplistic RGB palette
											0xFFFF0000,
											0xFF00FF00,
											0xFFFFFF00,
											0xFF0000FF,
											0xFFFF00FF,
											0xFF00FFFF,
											0xFFFFFFFF,
                                            0xFF000000,
											0xFF000000, 
											0xFF000000, 
											0xFF000000,
											0xFF000000, 
											0xFF000000, 
											0xFF000000,
											0xFF000000 };

const unsigned int octreepalette[16] = { 0xFF111111, //This palette is an untweaked palette found by octree search. It may not be any good
									     0xFF992222,
									     0xFF666622,
									     0xFFBB4444,
									     0xFF4444BB,
									     0xFFDD2266,
									     0xFF33BB44,
									     0xFF66DD22,
									     0xFFBB33CC,
									     0xFFDD99DD,
									     0xFFBBBB44,
									     0xFFDD6699,
									     0xFF6699DD,
									     0xFF44BBBB,
									     0xFF22DD99,
									     0xFFFFFFFF };
									   
const unsigned int greyscalepalette[16] = { 0xFF000000, //This palette is optimised such that a near-pure black and white video compresses well (i.e. the Bad Apple!! video)
											0xFFFFFFFF,
											0xFF111111,
											0xFFEEEEEE,
											0xFF222222,
											0xFFDDDDDD,
											0xFF333333,
											0xFFCCCCCC,
											0xFF444444,
											0xFFBBBBBB,
											0xFF555555,
											0xFFAAAAAA,
											0xFF666666,
											0xFF999999,
											0xFF777777,
											0xFF888888 };

const unsigned int bwpalette[16] = { 0xFF000000, //This palette is just pure black and white
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF,
									 0xFF000000,
									 0xFFFFFFFF };

const unsigned int lumchrompalette[16] = { 0xFF000000, //This palette cleanly separates luma and chroma
                                           0xFF333333,
										   0xFF999999, 
										   0xFFFFFFFF,
										   0xFF220000, 
										   0xFF770000, 
										   0xFFCC0000, 
										   0xFFFF0000,
										   0xFF002200,
										   0xFF007700,
										   0xFF00CC00,
										   0xFF00FF00, 
										   0xFF000022,
										   0xFF000077,
										   0xFF0000CC,
										   0xFF0000FF };

VideoConverterEngine::VideoConverterEngine()
{
	alreadyOpen = false;
	for (int i = 0; i < 16; i++)
	{
		palette[i] = basepalette[i];
		floatpal[i][0] = ((float)((palette[i] & 0xFF000000) >> 24)) / 255.0f; //A
		floatpal[i][1] = ((float)((palette[i] & 0x00FF0000) >> 16)) / 255.0f; //R
		floatpal[i][2] = ((float)((palette[i] & 0x0000FF00) >> 8)) / 255.0f; //G
		floatpal[i][3] = ((float)(palette[i] & 0x000000FF)) / 255.0f; //B
		float l = SRGBtoLMS[0] * SRGBGammaTransform(floatpal[i][1]) + SRGBtoLMS[1] * SRGBGammaTransform(floatpal[i][2]) + SRGBtoLMS[2] * SRGBGammaTransform(floatpal[i][3]);
		float m = SRGBtoLMS[3] * SRGBGammaTransform(floatpal[i][1]) + SRGBtoLMS[4] * SRGBGammaTransform(floatpal[i][2]) + SRGBtoLMS[5] * SRGBGammaTransform(floatpal[i][3]);
		float s = SRGBtoLMS[6] * SRGBGammaTransform(floatpal[i][1]) + SRGBtoLMS[7] * SRGBGammaTransform(floatpal[i][2]) + SRGBtoLMS[8] * SRGBGammaTransform(floatpal[i][3]);
		l = cbrtf(l); m = cbrtf(m); s = cbrtf(s);
		OKLabpal[i][0] = CRLMStoOKLab[0] * l + CRLMStoOKLab[1] * m + CRLMStoOKLab[2] * s;
		OKLabpal[i][1] = CRLMStoOKLab[3] * l + CRLMStoOKLab[4] * m + CRLMStoOKLab[5] * s;
		OKLabpal[i][2] = CRLMStoOKLab[6] * l + CRLMStoOKLab[7] * m + CRLMStoOKLab[8] * s;
		OKLabpalvec[0][i] = OKLabpal[i][0];
		OKLabpalvec[1][i] = OKLabpal[i][1];
		OKLabpalvec[2][i] = OKLabpal[i][2];
	}
	uvbias = 0.7f;
	ditherfactor = 0.5f;
	satditherfactor = 0.5f;
	hueditherfactor = 1.0f;
	sampleratespec = GetSampleRateSpec(22050.0f);
	planedata = new unsigned char*[4];
	prevplanedata = new unsigned char*[4];
	processedPlaneData = new unsigned short*[4];
	actualdisplayplanes = new unsigned char*[4];
	matchesoffset = new unsigned int*[4];
	matcheslength = new unsigned int*[4];
	matchesimpact = new int*[4];
	ismatchafill = new bool*[4];
	for (int i = 0; i < 4; i++)
	{
		planedata[i] = new unsigned char[PC_98_ONEPLANE_BYTE];
		prevplanedata[i] = new unsigned char[PC_98_ONEPLANE_BYTE];
		actualdisplayplanes[i] = new unsigned char[PC_98_ONEPLANE_BYTE];
		processedPlaneData[i] = new unsigned short[65536];
		matchesoffset[i] = new unsigned int[8192];
		matcheslength[i] = new unsigned int[8192];
		matchesimpact[i] = new int[8192];
		ismatchafill[i] = new bool[8192];
		memset(planedata[i], 0x00000000, PC_98_ONEPLANE_BYTE); //The screen will be clear when we start up the player
		memset(prevplanedata[i], 0x00000000, PC_98_ONEPLANE_BYTE);
		memset(actualdisplayplanes[i], 0x00000000, PC_98_ONEPLANE_BYTE);
	}
	minKeepLength = 2; //Legacy parameters
	minimpactperword = 1;
	minimpactperrun = 0;
	maxwordsperframe = 10000; //This quality parameter is preferred to the other ones
	frameskip = 0;
	outsampformat = AVSampleFormat::AV_SAMPLE_FMT_S32;
	outlayout = { AVChannelOrder::AV_CHANNEL_ORDER_NATIVE, 2, 1 << AVChannel::AV_CHAN_FRONT_CENTER }; //Use stereo as input to the ADPCM encoder
	hwtype = AVHWDeviceType::AV_HWDEVICE_TYPE_DXVA2;
	isForcedBuzzerAudio = false;
	isHalfVerticalResolution = false;
}

void VideoConverterEngine::ResetPlaneData()
{
	for (int i = 0; i < 4; i++)
	{
		memset(planedata[i], 0x00000000, PC_98_ONEPLANE_BYTE); //The screen will be clear when we start up the player
		memset(prevplanedata[i], 0x00000000, PC_98_ONEPLANE_BYTE);
	}
}

void VideoConverterEngine::EncodeVideo(wchar_t* outFileName, bool (*progressCallback)(unsigned int))
{
	ResetPlaneData();
	curtotalByteStreamSize = 0;
	actualFramerate = PC_98_FRAMERATE / ((double)(frameskip + 1));
	actualFrametime = ((double)(frameskip + 1)) / PC_98_FRAMERATE;
	convnumFrames = (int)(totalTime/actualFrametime); //Get the actual number of frames we need to write
	realoutsamplerate = GetRealSampleRateFromSpec(sampleratespec); //Sample rate must be one of the options that the PC-9801-86 supports
	CreateFullAudioStream();
	av_seek_frame(fmtcontext, vidstreamIndex, 0, 0);
	FILE* writefile;
	std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>* strconv = new std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>();
	std::string bytefname = strconv->to_bytes(outFileName);
	unsigned int writebuf[16];
	fopen_s(&writefile, bytefname.c_str(), "wb");
	fwrite("98V", 1, 3, writefile); //Write signature
	writebuf[0] = frameskip;
	fwrite(writebuf, 1, 1, writefile); //Write frameskip
	fwrite(&convnumFrames, 4, 1, writefile); //Write number of frames
	writebuf[0] = 0x0000 | sampleratespec;
	writebuf[0] |= isForcedBuzzerAudio ? 0x0008 : 0x0000;
	writebuf[0] |= isHalfVerticalResolution ? 0x0010 : 0x0000;
	writebuf[0] |= isStereo ? 0x0020 : 0x0000;
	fwrite(writebuf, 2, 1, writefile); //Write sample rate specifier
	unsigned char* bufbytes = (unsigned char*)writebuf;
	for (int i = 0; i < 16; i++)
	{
		*bufbytes = (((palette[i] & 0x00FF0000) >> 16) / 0x11) << 4;
		*bufbytes |= ((palette[i] & 0x0000FF00) >> 8) / 0x11;
		bufbytes++;
		*bufbytes = ((palette[i] & 0x000000FF) / 0x11) << 4;
		i++;
		*bufbytes |= ((palette[i] & 0x00FF0000) >> 16) / 0x11;
		bufbytes++;
		*bufbytes = (((palette[i] & 0x0000FF00) >> 8) / 0x11) << 4;
		*bufbytes |= (palette[i] & 0x000000FF) / 0x11;
		bufbytes++;
	}
	fwrite(writebuf, 1, 24, writefile); //Write palette

	unsigned short* curAudiodata;
	unsigned int curAudioLength;
	unsigned int curFrameLength[4];
	unsigned int planesToWrite;
	double curTime = 0.0;
	double refTime = 0.0;
	av_read_frame(fmtcontext, curPacket);
	avcodec_send_packet(vidcodcontext, curPacket);
	avcodec_receive_frame(vidcodcontext, curFrame);
	/*/ //Hardware acceleration doesn't seem to work. Is this because of my system (at the time of writing, it has an IGPU) or is there an error in my code?
	if (hardwareaccel)
	{
		av_hwframe_transfer_data(curFrame, curHWFrame, 0);
	}
	else
	{
		curFrame = curHWFrame;
	}
	//*/
	int tly = topLeftY;
	int outh = outHeight;
	av_image_copy(vidorigData, vidorigLineSize, (const unsigned char**)curFrame->data, curFrame->linesize, inPixFormat, inWidth, inHeight);
	if (isHalfVerticalResolution) //640x200 resolution makes the image look twice as tall as it logically is
	{
		sws_scale(scalercontexthalfheight, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
		tly /= 2;
		outh /= 2;
	}
	else
	{
		sws_scale(scalercontext, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
	}
	for (int i = 0; i < convnumFrames; i++)
	{
		memset(convertedFrame, palette[0], PC_98_RESOLUTION * 4);
		memset(inputrescaledframe, 0, PC_98_RESOLUTION * 4);

		unsigned int* inframeCol = (unsigned int*)vidscaleData[0];
		unsigned int* outframeCol = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * tly); //Cheeky reinterpretation
		unsigned int* outframeColLineDouble = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + 1));
		if (isHalfVerticalResolution) //640x200 resolution makes the image look twice as tall as it logically is
		{
			for (int i = 0; i < outh; i++)
			{
				for (int j = 0; j < outWidth; j++)
				{
					*outframeCol = *inframeCol++;
					*outframeColLineDouble++ = *outframeCol++;
				}
				outframeCol = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + i * 2 + 2));
				outframeColLineDouble = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + i * 2 + 3));
			}
		}
		else
		{
			for (int i = 0; i < outh; i++)
			{
				for (int j = 0; j < outWidth; j++)
				{
					*outframeCol++ = *inframeCol++;
				}
				outframeCol = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + i + 1));
			}
		}

		planesToWrite = 0;
		curFrameLength[0] = 0;
		curFrameLength[1] = 0;
		curFrameLength[2] = 0;
		curFrameLength[3] = 0;

		//Convert it for display
		inframeCol = (unsigned int*)vidscaleData[0];
		outframeCol = ((unsigned int*)convertedFrame) + (topLeftX + PC_98_WIDTH * tly); //Cheeky reinterpretation
		omp_set_num_threads(omp_get_max_threads());
		#pragma omp parallel for shared(outframeCol, inframeCol) schedule(static)
		for (int i = 0; i < outh; i++)
		{
			for (int j = 0; j < outWidth; j++)
			{
				outframeCol[j + i * PC_98_WIDTH] = VoidClusterDither16x16(inframeCol[j + i * outWidth], j, i);
			}
		}

		CreatePlaneData();

		curTime = actualFrametime * (i + 1);
		curAudiodata = ProcessAudio(&curAudioLength, curTime);
		fwrite(&curAudioLength, 2, 1, writefile); //Write audio length for this frame
		if (isStereo) curAudioLength *= 2; //two channels = two times the data
		fwrite(curAudiodata, 2, curAudioLength, writefile); //Write audio data
		ProcessFrame(curFrameLength, &planesToWrite);
		fwrite(&planesToWrite, 2, 1, writefile);
		for (int j = 0; j < 4; j++)
		{
			if (planesToWrite & (0x1 << j))
			{
				fwrite(curFrameLength + j, 2, 1, writefile);
				fwrite(processedPlaneData[j], 2, curFrameLength[j], writefile);
			}
		}
		SimulateRealResult(planesToWrite, curFrameLength); //Reads data back
		if (progressCallback != NULL)
		{
			if (progressCallback(i)) //Callback as a sort of progress indicator
			{
				return; //If the quit message has been emitted, return immediately.
			}
		}
		
		while (refTime < curTime) //Skip frames that are too close to each other
		{
			av_packet_unref(curPacket);
			av_read_frame(fmtcontext, curPacket);
			if (curPacket->data == nullptr)
			{
				break;
			}
			if (curPacket->stream_index != vidstreamIndex) continue;
			avcodec_send_packet(vidcodcontext, curPacket);
			avcodec_receive_frame(vidcodcontext, curFrame);
			/*/
			if (hardwareaccel)
			{
				av_hwframe_transfer_data(curFrame, curHWFrame, 0);
			}
			else
			{
				curFrame = curHWFrame;
			}
			//*/
			refTime = (((double)curPacket->dts) * ((double)vidstream->time_base.num)) / ((double)vidstream->time_base.den);
			if (curFrame->data[0] != nullptr)
			{
				av_image_copy(vidorigData, vidorigLineSize, (const unsigned char**)curFrame->data, curFrame->linesize, inPixFormat, inWidth, inHeight);
				if (isHalfVerticalResolution) //640x200 resolution makes the image look twice as tall as it logically is
				{
					sws_scale(scalercontexthalfheight, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
				}
				else
				{
					sws_scale(scalercontext, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
				}
			}
		}
	}

	fclose(writefile); //we are done
	delete strconv;
}

//Function was intended to be used for parallelisation, is currently not even implemented
void VideoConverterEngine::DitherScanline(unsigned int* startInCol, unsigned int* startOutCol)
{

}

//Convert our entire audio stream
void VideoConverterEngine::CreateFullAudioStream()
{
	int numsamplesres = (int)(totalTime * realoutsamplerate);
	audfullstreamShort = new short[numsamplesres + 65536]; //I'm not taking any chances
	audfullstreamByte = new unsigned char[numsamplesres + 65536];
	audfullstreamShortDiff = new short[numsamplesres + 65536];
	audfullstreamByteDiff = new unsigned char[numsamplesres + 65536];
	if (audstreamIndex == AVERROR_STREAM_NOT_FOUND)
	{
		memset(audfullstreamByte, 0, numsamplesres + 32768); //Set to silence if no audio stream was found
		memset(audfullstreamByteDiff, 0, numsamplesres + 32768);
		return;
	}

	
	short* fullstreamptr = audfullstreamShort;
	short* fulldiffstreamptr = audfullstreamShortDiff;
	int* Linputptr;
	int* Rinputptr;
	double refTime = 0.0;
	double oldrefTime = -1.0;
	int transferredSamples = 2048;
	int sampleaddr = 0;
	int endaddr = 1;
	//Setup resampler
	swr_alloc_set_opts2(&resamplercontext, &outlayout, outsampformat, realoutsamplerate, &inChLayout, inAudFormat, inAudioRate, 0, NULL);
	swr_init(resamplercontext);
	av_samples_alloc_array_and_samples(&audorigData, &audorigLineSize, inChLayout.nb_channels, 65536, inAudFormat, 0);
	av_samples_alloc_array_and_samples(&audresData, &audresLineSize, outlayout.nb_channels, 65536, outsampformat, 0);
	av_seek_frame(fmtcontext, audstreamIndex, 0, 0);

	//Get the entire audio stream as a bunch of signed 16-bit integers
	while (refTime < totalTime)
	{
		av_read_frame(fmtcontext, curPacket);
		if (curPacket->data == nullptr)
		{
			break;
		}
		if (curPacket->stream_index != audstreamIndex)
		{
			av_packet_unref(curPacket);
			continue;
		}
		avcodec_send_packet(audcodcontext, curPacket);
		avcodec_receive_frame(audcodcontext, curFrame);
		refTime = (((double)curFrame->pts) * ((double)audstream->time_base.num)) / ((double)audstream->time_base.den);
		if (curFrame->data[0] != nullptr)
		{
			sampleaddr = (int)(refTime * (double)realoutsamplerate);
			if (sampleaddr < 0) sampleaddr = 0;
			av_samples_copy(audorigData, curFrame->data, 0, 0, curFrame->nb_samples, inChLayout.nb_channels, inAudFormat);
			swr_convert(resamplercontext, audresData, transferredSamples, (const unsigned char**)audorigData, curFrame->nb_samples);
			fullstreamptr = audfullstreamShort + sampleaddr;
			fulldiffstreamptr = audfullstreamShortDiff + sampleaddr;
			Linputptr = (int*)(audresData[0]);
			Rinputptr = Linputptr + 1;
			for (int i = 0; i < transferredSamples; i++)
			{
				const int Lsig = Linputptr[i * 2];
				const int Rsig = Rinputptr[i * 2];
				const int meansig = (Lsig >> 1) + (Rsig >> 1);
				const int diffsig = Lsig - meansig;
				fullstreamptr[i] = meansig >> 16;
				fulldiffstreamptr[i] = diffsig >> 16;
			}
			oldrefTime = refTime;
		}
		av_packet_unref(curPacket);
	}
	
	short lastsample = 0;
	int curshift = 0;
	short cursample = 0;
	short curdiff = 0;
	bool isnegative = false;
	short curmax = 0x007F;
	short curmin = 0xFF80;
	for (int i = 0; i < numsamplesres; i++) //ADPCM encode mean signal
	{
		cursample = audfullstreamShort[i];
		curdiff = cursample - lastsample;
		isnegative = (curdiff & 0x8000) != 0;
		if (isnegative)
		{
			if (curdiff < curmin)
			{
				curdiff = curmin;
			}
		}
		else
		{
			if (curdiff > curmax)
			{
				curdiff = curmax;
			}
		}
		curdiff >>= curshift;
		audfullstreamByte[i] = (signed char)curdiff;
		curdiff <<= curshift;
		lastsample += curdiff;
		curdiff >>= curshift;
		if (isnegative) curdiff ^= 0xFFFF;
		if (curdiff <= 0x0018) curshift--;
		else if (curdiff >= 0x0068) curshift++;
		if (curshift < 0) curshift = 0;
		else if (curshift > 8) curshift = 8;
		curmax = 0x7FFF >> (8 - curshift);
		curmin = 0xFFFF8000 >> (8 - curshift);
	}
	lastsample = 0;
	curshift = 0;
	cursample = 0;
	curdiff = 0;
	isnegative = false;
	curmax = 0x007F;
	curmin = 0xFF80;
	for (int i = 0; i < numsamplesres; i++) //ADPCM encode difference signal
	{
		cursample = audfullstreamShortDiff[i];
		curdiff = cursample - lastsample;
		isnegative = (curdiff & 0x8000) != 0;
		if (isnegative)
		{
			if (curdiff < curmin)
			{
				curdiff = curmin;
			}
		}
		else
		{
			if (curdiff > curmax)
			{
				curdiff = curmax;
			}
		}
		curdiff >>= curshift;
		audfullstreamByteDiff[i] = (signed char)curdiff;
		curdiff <<= curshift;
		lastsample += curdiff;
		curdiff >>= curshift;
		if (isnegative) curdiff ^= 0xFFFF;
		if (curdiff <= 0x0018) curshift--;
		else if (curdiff >= 0x0068) curshift++;
		if (curshift < 0) curshift = 0;
		else if (curshift > 8) curshift = 8;
		curmax = 0x7FFF >> (8 - curshift);
		curmin = 0xFFFF8000 >> (8 - curshift);
	}

	swr_free(&resamplercontext);
}

//Get the samples from the full stream that are needed for this frame
unsigned short* VideoConverterEngine::ProcessAudio(unsigned int* returnLength, double cutoffTime)
{
	unsigned short* streamptr = (unsigned short*)(audfullstreamByte) + curtotalByteStreamSize;
	unsigned int thisChunkLength = 0;
	double refTime = (((double)curtotalByteStreamSize) * 2.0) / (realoutsamplerate); //There are 2 samples in every 16-bit chunk
	while (refTime < cutoffTime)
	{
		processedAudioData[thisChunkLength] = *streamptr++;
		thisChunkLength++;
		refTime = ((double)(curtotalByteStreamSize + thisChunkLength) * 2.0) / (realoutsamplerate);
	}
	if (isStereo)
	{
		streamptr = (unsigned short*)(audfullstreamByteDiff)+curtotalByteStreamSize;
		for (int i = 0; i < thisChunkLength; i++)
		{
			processedAudioData[thisChunkLength + i] = *streamptr++;
		}
	}
	curtotalByteStreamSize += thisChunkLength;
	*returnLength = thisChunkLength;
	return processedAudioData;
}

//Get the correct frame data
void VideoConverterEngine::ProcessFrame(unsigned int* returnLength, unsigned int* returnuPlanes)
{
	unsigned int nummatches[4];
	unsigned int numfills = 0;
	unsigned int numcopies = 0;
	unsigned int curmatchoffset = 0;
	unsigned int curmatchlength = 0;
	unsigned int curmatchimpact = 0;
	unsigned int planedatalength = 0;
	unsigned int fullmatchesimpact[4];
	int maxwordsfixed[4];
	unsigned int planeorder[4];
	unsigned short* framedatptr;
	unsigned short* prevframedatptr;
	unsigned short* datawriteptr;
	unsigned short fillcomp;
	bool isfill;
	*returnuPlanes = 0;
	for (int i = 0; i < 4; i++) //Find matches and how much of an impact them all combined make
	{
		framedatptr = (unsigned short*)planedata[i];
		prevframedatptr = (unsigned short*)prevplanedata[i];
		nummatches[i] = 0;

		for (int j = 0; j < PC_98_ONEPLANE_WORD; j++) //Form impact number array
		{
			impactarray[j] = GetNumChangedBits(*framedatptr++, *prevframedatptr++);
			if (impactarray[j] < minimpactperword) //Prune low-impact words
			{
				impactarray[j] = -1; //-1 -> no impact, do not include
			}
			isalreadydesignatedfill[j] = false;
		}

		for (int j = 1; j < PC_98_ONEPLANE_WORD - 1; j++) //Detect deltas that are close to each other
		{
			if ((impactarray[j] < 0) && (impactarray[j - 1] > 0))
			{
				if (impactarray[j + 1] > 0) //distance is 1 word -> saves memory and processing time
				{
					impactarray[j] = 0; // 0 -> no impact, but it is so close to other potential deltas that it would be better to include anyway
				}
			}
		}

		framedatptr = (unsigned short*)planedata[i];
		prevframedatptr = (unsigned short*)prevplanedata[i];
		for (int j = 0; j < PC_98_ONEPLANE_WORD; j++) //Find matches changing: fills first
		{
			if (*framedatptr != *prevframedatptr)
			{
				fillcomp = *framedatptr;
				isfill = true;
				curmatchoffset = j;
				curmatchlength = 0;
				curmatchimpact = 0;
				while (impactarray[j] >= 0 && *framedatptr == fillcomp)
				{
					if (impactarray[j] == 0 && *(framedatptr + 1) != fillcomp) //if we hit a zero impact word that's close to another delta, check if it's compatible
					{
						break;
					}
					curmatchimpact += impactarray[j];
					curmatchlength++;
					framedatptr++;
					prevframedatptr++;
					j++;
				}
				if (curmatchlength >= 2) //Short fills are pretty useless
				{
					matchesoffset[i][nummatches[i]] = curmatchoffset << 1;
					matcheslength[i][nummatches[i]] = curmatchlength;
					matchesimpact[i][nummatches[i]] = curmatchimpact;
					ismatchafill[i][nummatches[i]] = true;
					for (int k = 0; k < curmatchlength; k++)
					{
						isalreadydesignatedfill[k + curmatchoffset] = true;
					}
					nummatches[i]++;
				}
			}
			framedatptr++;
			prevframedatptr++;
		}

		framedatptr = (unsigned short*)planedata[i];
		prevframedatptr = (unsigned short*)prevplanedata[i];
		for (int j = 0; j < PC_98_ONEPLANE_WORD; j++) //Find matches changing
		{
			if (*framedatptr != *prevframedatptr)
			{
				fillcomp = *framedatptr;
				isfill = false;
				curmatchoffset = j;
				curmatchlength = 0;
				curmatchimpact = 0;
				while (impactarray[j] >= 0 && !isalreadydesignatedfill[j])
				{
					if (impactarray[j] == 0 && isalreadydesignatedfill[j + 1]) //if we hit a zero impact word that's close to another delta, check if it's compatible
					{
						break;
					}
					curmatchimpact += impactarray[j];
					curmatchlength++;
					framedatptr++;
					prevframedatptr++;
					j++;
				}
				if (curmatchimpact >= minimpactperrun && curmatchlength > 0) //Don't keep a match that has too small of an impact. Therefore this is a lossy codec
				{
					matchesoffset[i][nummatches[i]] = curmatchoffset << 1;
					matcheslength[i][nummatches[i]] = curmatchlength;
					matchesimpact[i][nummatches[i]] = curmatchimpact;
					ismatchafill[i][nummatches[i]] = false;
					nummatches[i]++;
				}
			}
			framedatptr++;
			prevframedatptr++;
		}

		fullmatchesimpact[i] = 0;
		if (nummatches[i] <= 0)
		{
			continue;
		}

		//Sort the matches so that the most impactful runs come first
		int stackdepth = 0;
		int partindstack[256];
		int startindstack[256];
		int endindstack[256];
		int sectionstack[256];
		bool goingdownstack[256];
		unsigned int pivotnum;
		unsigned int swapnum1;
		unsigned int swapnum2;
		bool swapbool1;
		bool swapbool2;
		bool issorted = false;
		startindstack[0] = 0;
		endindstack[0] = nummatches[i] - 1;
		goingdownstack[0] = true;
		while (!issorted)
		{
			if ((endindstack[stackdepth] - startindstack[stackdepth]) <= 0)
			{
				goingdownstack[stackdepth] = false;
				goto endofcheck;
			}
			else if ((endindstack[stackdepth] - startindstack[stackdepth]) <= 1)
			{
				pivotnum = matchesimpact[i][endindstack[stackdepth]];
				if (matchesimpact[i][startindstack[stackdepth]] < pivotnum)
				{
					swapnum1 = matchesimpact[i][startindstack[stackdepth]];
					swapnum2 = matchesimpact[i][endindstack[stackdepth]];
					matchesimpact[i][startindstack[stackdepth]] = swapnum2;
					matchesimpact[i][endindstack[stackdepth]] = swapnum1;
					swapnum1 = matcheslength[i][startindstack[stackdepth]];
					swapnum2 = matcheslength[i][endindstack[stackdepth]];
					matcheslength[i][startindstack[stackdepth]] = swapnum2;
					matcheslength[i][endindstack[stackdepth]] = swapnum1;
					swapnum1 = matchesoffset[i][startindstack[stackdepth]];
					swapnum2 = matchesoffset[i][endindstack[stackdepth]];
					matchesoffset[i][startindstack[stackdepth]] = swapnum2;
					matchesoffset[i][endindstack[stackdepth]] = swapnum1;
					swapbool1 = ismatchafill[i][startindstack[stackdepth]];
					swapbool2 = ismatchafill[i][endindstack[stackdepth]];
					ismatchafill[i][startindstack[stackdepth]] = swapbool2;
					ismatchafill[i][endindstack[stackdepth]] = swapbool1;
				}
				goingdownstack[stackdepth] = false;
				goto endofcheck;
			}
			pivotnum = matchesimpact[i][endindstack[stackdepth]];
			partindstack[stackdepth] = startindstack[stackdepth];
			for (int j = startindstack[stackdepth]; j < endindstack[stackdepth]; j++)
			{
				if (matchesimpact[i][j] > pivotnum)
				{
					swapnum1 = matchesimpact[i][j];
					swapnum2 = matchesimpact[i][partindstack[stackdepth]];
					matchesimpact[i][j] = swapnum2;
					matchesimpact[i][partindstack[stackdepth]] = swapnum1;
					swapnum1 = matcheslength[i][j];
					swapnum2 = matcheslength[i][partindstack[stackdepth]];
					matcheslength[i][j] = swapnum2;
					matcheslength[i][partindstack[stackdepth]] = swapnum1;
					swapnum1 = matchesoffset[i][j];
					swapnum2 = matchesoffset[i][partindstack[stackdepth]];
					matchesoffset[i][j] = swapnum2;
					matchesoffset[i][partindstack[stackdepth]] = swapnum1;
					swapbool1 = ismatchafill[i][j];
					swapbool2 = ismatchafill[i][partindstack[stackdepth]];
					ismatchafill[i][j] = swapbool2;
					ismatchafill[i][partindstack[stackdepth]] = swapbool1;
					partindstack[stackdepth]++;
				}
			}
			swapnum1 = matchesimpact[i][endindstack[stackdepth]];
			swapnum2 = matchesimpact[i][partindstack[stackdepth]];
			matchesimpact[i][endindstack[stackdepth]] = swapnum2;
			matchesimpact[i][partindstack[stackdepth]] = swapnum1;
			swapnum1 = matcheslength[i][endindstack[stackdepth]];
			swapnum2 = matcheslength[i][partindstack[stackdepth]];
			matcheslength[i][endindstack[stackdepth]] = swapnum2;
			matcheslength[i][partindstack[stackdepth]] = swapnum1;
			swapnum1 = matchesoffset[i][endindstack[stackdepth]];
			swapnum2 = matchesoffset[i][partindstack[stackdepth]];
			matchesoffset[i][endindstack[stackdepth]] = swapnum2;
			matchesoffset[i][partindstack[stackdepth]] = swapnum1;
			swapbool1 = ismatchafill[i][endindstack[stackdepth]];
			swapbool2 = ismatchafill[i][partindstack[stackdepth]];
			ismatchafill[i][endindstack[stackdepth]] = swapbool2;
			ismatchafill[i][partindstack[stackdepth]] = swapbool1;
		endofcheck:
			if (goingdownstack[stackdepth])
			{
				sectionstack[stackdepth] = 0;
				goingdownstack[stackdepth] = false;
				stackdepth++;
				startindstack[stackdepth] = startindstack[stackdepth - 1];
				endindstack[stackdepth] = partindstack[stackdepth - 1] - 1;
				goingdownstack[stackdepth] = true;
			}
			else if (sectionstack[stackdepth - 1] == 0 && stackdepth > 0)
			{
				sectionstack[stackdepth - 1] = 1;
				startindstack[stackdepth] = partindstack[stackdepth - 1] + 1;
				endindstack[stackdepth] = endindstack[stackdepth - 1];
			}
			else
			{
				stackdepth--;
				if (stackdepth < 0)
				{
					issorted = true;
				}
			}
		}

		for (int j = 0; j < nummatches[i]; j++)
		{
			fullmatchesimpact[i] += matchesimpact[i][j];
		}
	}

	unsigned int totalmaximpact = 0; //Reallocate plane sizes based on their impact
	for (int i = 0; i < 4; i++)
	{
		totalmaximpact += fullmatchesimpact[i];
	}
	if (totalmaximpact <= 0)
	{
		for (int i = 0; i < 4; i++)
		{
			maxwordsfixed[i] = maxwordsperframe/4;
			planeorder[i] = i;
		}
	}
	else
	{
		unsigned int totalframedata = maxwordsperframe;
		double wordsperimpact = ((double)totalframedata) / ((double)totalmaximpact);
		for (int i = 0; i < 4; i++)
		{
			maxwordsfixed[i] = (unsigned int)(wordsperimpact * ((double)fullmatchesimpact[i]));
			planeorder[i] = 0xFF;
		}
		int curmax = 0;
		int curmaxplaneind = 0;
		bool alreadyinorder;
		for (int i = 0; i < 4; i++)
		{
			curmax = -9999;
			curmaxplaneind = 0;
			for (int j = 0; j < 4; j++)
			{
				alreadyinorder = false;
				for (int k = 0; k < 4; k++)
				{
					if (j == planeorder[k])
					{
						alreadyinorder = true;
						break;
					}
				}
				if (alreadyinorder) continue;
				if (maxwordsfixed[j] > curmax)
				{
					curmaxplaneind = j;
					curmax = maxwordsfixed[j];
				}
			}
			planeorder[i] = curmaxplaneind;
		}
	}

	for (int n = 0; n < 4; n++) //Commit data with fixed plane size allocations
	{
		int i = planeorder[n];
		datawriteptr = (unsigned short*)processedPlaneData[i];
		numfills = 0;
		numcopies = 0;

		planedatalength = 2;
		for (int j = 0; j < nummatches[i]; j++) //Organise fills and copies
		{
			if (ismatchafill[i][j])
			{
				foundfills[numfills] = j;
				numfills++;
				planedatalength += 3;
			}
			else
			{
				foundcopies[numcopies] = j;
				numcopies++;
				planedatalength += matcheslength[i][j] + 2;
			}
			if (planedatalength >= maxwordsfixed[i])
			{
				break; //break out if we're going to write too much data
			}
		}

		if (planedatalength < maxwordsfixed[i] && n < 3) //Push excess onto next plane
		{
			maxwordsfixed[planeorder[n + 1]] += maxwordsfixed[i] - planedatalength;
		}
		
		//Commit fills
		*datawriteptr = numfills;
		datawriteptr++;
		for (int j = 0; j < numfills; j++)
		{
			*datawriteptr = matchesoffset[i][foundfills[j]];
			datawriteptr++;
			*datawriteptr = matcheslength[i][foundfills[j]];
			datawriteptr++;
			*datawriteptr = *((unsigned short*)(planedata[i] + matchesoffset[i][foundfills[j]]));
			datawriteptr++;
		}

		//Commit copies
		*datawriteptr = numcopies;
		datawriteptr++;
		for (int j = 0; j < numcopies; j++)
		{
			*datawriteptr = matchesoffset[i][foundcopies[j]];
			datawriteptr++;
			*datawriteptr = matcheslength[i][foundcopies[j]];
			datawriteptr++;
			memcpy(datawriteptr, (unsigned short*)(planedata[i] + matchesoffset[i][foundcopies[j]]), matcheslength[i][foundcopies[j]] << 1);
			datawriteptr += matcheslength[i][foundcopies[j]];
		}

		returnLength[i] = planedatalength;
		*returnuPlanes |= 0x1 << i;

		unsigned short* datareadptr = (unsigned short*)processedPlaneData[i]; //Make previous comparison plane what it should be
		unsigned int writeoffset;
		unsigned int writelength;

		datareadptr++;
		for (int j = 0; j < numfills; j++)
		{
			writeoffset = *datareadptr;
			datareadptr++;
			writelength = *datareadptr;
			datareadptr++;
			datawriteptr = (unsigned short*)(prevplanedata[i] + writeoffset);
			for (int k = 0; k < writelength; k++)
			{
				*datawriteptr++ = *datareadptr;
			}
			datareadptr++;
		}
		datareadptr++;
		for (int j = 0; j < numcopies; j++)
		{
			writeoffset = *datareadptr;
			datareadptr++;
			writelength = *datareadptr;
			datareadptr++;
			datawriteptr = (unsigned short*)(prevplanedata[i] + writeoffset);
			for (int k = 0; k < writelength; k++)
			{
				*datawriteptr++ = *datareadptr++;
			}
		}
	}
}

//For use in the preview
void VideoConverterEngine::SimulateRealResult(unsigned int updateplanes, unsigned int* planelengths)
{
	unsigned short* datareadptr;
	unsigned short* datawriteptr;
	unsigned char* midptr;
	unsigned int* finalwriteptr;
	unsigned int totalLength;
	unsigned int numfills;
	unsigned int numcopies;
	unsigned int writeoffset;
	unsigned int writelength;

	for (int i = 0; i < 4; i++) //Write to plane
	{
		if (!(updateplanes & (0x1 << i))) continue;
		datareadptr = (unsigned short*)processedPlaneData[i];
		totalLength = planelengths[i];
		numfills = *datareadptr;
		datareadptr++;
		for (int j = 0; j < numfills; j++)
		{
			writeoffset = *datareadptr;
			datareadptr++;
			writelength = *datareadptr;
			datareadptr++;
			datawriteptr = (unsigned short*)(actualdisplayplanes[i] + writeoffset);
			for (int k = 0; k < writelength; k++)
			{
				*datawriteptr++ = *datareadptr;
			}
			datareadptr++;
		}
		numcopies = *datareadptr;
		datareadptr++;
		for (int j = 0; j < numcopies; j++)
		{
			writeoffset = *datareadptr;
			datareadptr++;
			writelength = *datareadptr;
			datareadptr++;
			datawriteptr = (unsigned short*)(actualdisplayplanes[i] + writeoffset);
			for (int k = 0; k < writelength; k++)
			{
				*datawriteptr++ = *datareadptr++;
			}
		}
	}

	int planeind = 0;
	int planeshift = 0;
	int indcomp = 0;
	int index = 0;
	finalwriteptr = (unsigned int*)actualdisplaybuffer;
	if (isHalfVerticalResolution)
	{
		for (int i = 0; i < PC_98_RESOLUTION; i++) //Convert from planar data to colour
		{
			planeind = i / 8;
			planeind %= PC_98_BLOCKWIDTH;
			planeind += PC_98_BLOCKWIDTH * (i / 1280);
			planeshift = 7 - (i & 0x7);
			index = actualdisplayplanes[0][planeind] & (0x1 << planeshift);
			index <<= 8;
			index >>= planeshift + 8;
			indcomp = actualdisplayplanes[1][planeind] & (0x1 << planeshift);
			indcomp <<= 8;
			indcomp >>= planeshift + 7;
			index |= indcomp;
			indcomp = actualdisplayplanes[2][planeind] & (0x1 << planeshift);
			indcomp <<= 8;
			indcomp >>= planeshift + 6;
			index |= indcomp;
			indcomp = actualdisplayplanes[3][planeind] & (0x1 << planeshift);
			indcomp <<= 8;
			indcomp >>= planeshift + 5;
			index |= indcomp;
			*finalwriteptr++ = palette[index];
		}
	}
	else
	{
		for (int i = 0; i < PC_98_RESOLUTION; i++) //Convert from planar data to colour
		{
			planeind = i / 8;
			planeshift = 7 - (i & 0x7);
			index = actualdisplayplanes[0][planeind] & (0x1 << planeshift);
			index <<= 8;
			index >>= planeshift + 8;
			indcomp = actualdisplayplanes[1][planeind] & (0x1 << planeshift);
			indcomp <<= 8;
			indcomp >>= planeshift + 7;
			index |= indcomp;
			indcomp = actualdisplayplanes[2][planeind] & (0x1 << planeshift);
			indcomp <<= 8;
			indcomp >>= planeshift + 6;
			index |= indcomp;
			indcomp = actualdisplayplanes[3][planeind] & (0x1 << planeshift);
			indcomp <<= 8;
			indcomp >>= planeshift + 5;
			index |= indcomp;
			*finalwriteptr++ = palette[index];
		}
	}
}

//PC-98 VRAM is planar, and this codec respects that
void VideoConverterEngine::CreatePlaneData()
{
	const unsigned int* const convframeptr = (unsigned int*)convertedFrame;

	for (int i = 0; i < PC_98_ONEPLANE_BYTE; i++)
	{
		unsigned int inds[8];
		for (int j = 0; j < 8; j++) //Get colour indices
		{
			const unsigned int compcol = convframeptr[i * 8 + j];
			unsigned int curind = 0;
			for (int k = 15; k >= 0; k--)
			{
				if (compcol == palette[k])
				{
					curind = k << 8; //Preshift index
				}
			}
			inds[j] = curind;
		}

		unsigned int p0c = (inds[0] & 0x100) >> 1;
		unsigned int p1c = (inds[0] & 0x200) >> 2;
		unsigned int p2c = (inds[0] & 0x400) >> 3;
		unsigned int p3c = (inds[0] & 0x800) >> 4;
		for (int j = 1; j < 8; j++) //Write to bitplanes
		{
			const unsigned int curind = inds[j];
			p0c |= (curind & 0x100) >> (j + 1);
			p1c |= (curind & 0x200) >> (j + 2);
			p2c |= (curind & 0x400) >> (j + 3);
			p3c |= (curind & 0x800) >> (j + 4);
		}
		planedata[0][i] = (unsigned char)p0c;
		planedata[1][i] = (unsigned char)p1c;
		planedata[2][i] = (unsigned char)p2c;
		planedata[3][i] = (unsigned char)p3c;
	}
}

//Used only before total conversion since it's inefficient
unsigned char* VideoConverterEngine::GrabFrame(int framenumber)
{
	//Get our frame in
	if(convertedFrame == nullptr) convertedFrame = new unsigned char[PC_98_RESOLUTION * 4];
	double doubletime = ((double)framenumber * (double)vidstream->time_base.den * (double)vidstream->avg_frame_rate.den) / ((double)vidstream->avg_frame_rate.num * (double)vidstream->time_base.num);
	unsigned long long actualTime = (unsigned long long) doubletime;
	av_seek_frame(fmtcontext, vidstreamIndex, actualTime, 0);
	av_read_frame(fmtcontext, curPacket);
	avcodec_send_packet(vidcodcontext, curPacket);
	avcodec_receive_frame(vidcodcontext, curFrame);
	/*/
	if (hardwareaccel)
	{
		av_hwframe_transfer_data(curFrame, curHWFrame, 0);
	}
	else
	{
		curFrame = curHWFrame;
	}
	//*/
	int tly = topLeftY;
	int outh = outHeight;
	av_image_copy(vidorigData, vidorigLineSize, (const unsigned char**)curFrame->data, curFrame->linesize, inPixFormat, inWidth, inHeight);
	if (isHalfVerticalResolution) //640x200 resolution makes the image look twice as tall as it logically is
	{
		sws_scale(scalercontexthalfheight, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
		outh /= 2;
	}
	else
	{
		sws_scale(scalercontext, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
	}
	memset(convertedFrame, palette[0], PC_98_RESOLUTION * 4);
	memset(inputrescaledframe, 0, PC_98_RESOLUTION * 4);

	//Convert it for display
	unsigned int* inframeCol = (unsigned int*)vidscaleData[0];
	unsigned int* outframeCol = ((unsigned int*)convertedFrame) + (topLeftX + PC_98_WIDTH * tly); //Cheeky reinterpretation
	unsigned int* outframeColLineDouble = ((unsigned int*)convertedFrame) + (topLeftX + PC_98_WIDTH * (tly + 1));
	if (isHalfVerticalResolution) //640x200 resolution makes the image look twice as tall as it logically is
	{
		for (int i = 0; i < outh; i++)
		{
			for (int j = 0; j < outWidth; j++)
			{
				*outframeCol = VoidClusterDither16x16(*inframeCol++, j, i);
				*outframeColLineDouble++ = *outframeCol++;
			}
			outframeCol = ((unsigned int*)convertedFrame) + (topLeftX + PC_98_WIDTH * (tly + i * 2 + 2));
			outframeColLineDouble = ((unsigned int*)convertedFrame) + (topLeftX + PC_98_WIDTH * (tly + i * 2 + 3));
		}
	}
	else
	{
		for (int i = 0; i < outh; i++)
		{
			for (int j = 0; j < outWidth; j++)
			{
				*outframeCol++ = VoidClusterDither16x16(*inframeCol++, j, i);
			}
			outframeCol = ((unsigned int*)convertedFrame) + (topLeftX + PC_98_WIDTH * (tly + i + 1));
		}
	}

	//Get the rescaled input frame out for comparison
	inframeCol = (unsigned int*)vidscaleData[0];
	outframeCol = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * tly); //Cheeky reinterpretation
	outframeColLineDouble = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + 1));
	if (isHalfVerticalResolution) //640x200 resolution makes the image look twice as tall as it logically is
	{
		for (int i = 0; i < outh; i++)
		{
			for (int j = 0; j < outWidth; j++)
			{
				*outframeCol = *inframeCol++;
				*outframeColLineDouble++ = *outframeCol++;
			}
			outframeCol = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + i * 2 + 2));
			outframeColLineDouble = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + i * 2 + 3));
		}
	}
	else
	{
		for (int i = 0; i < outh; i++)
		{
			for (int j = 0; j < outWidth; j++)
			{
				*outframeCol++ = *inframeCol++;
			}
			outframeCol = ((unsigned int*)inputrescaledframe) + (topLeftX + PC_98_WIDTH * (tly + i + 1));
		}
	}
	
	av_packet_unref(curPacket);

	return convertedFrame;
}

//I thought this was necessary to set up hardware acceleration
enum AVPixelFormat VideoConverterEngine::GetHWFormat(AVCodecContext* ctx, const enum AVPixelFormat* pixfmts)
{
	const enum AVPixelFormat* p;

	for (p = pixfmts; *p != -1; p++) {
		if (*p == staticPixFmt)
			return *p;
	}
}

//Sets up our converter upon loading a video file
void VideoConverterEngine::OpenForDecodeVideo(wchar_t* inFileName)
{
	//Open video file
	if (alreadyOpen) CloseDecoder();
	std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>* strconv = new std::wstring_convert<std::codecvt<wchar_t, char, mbstate_t>, wchar_t>();
	std::string bytefname = strconv->to_bytes(inFileName);
	avformat_open_input(&fmtcontext, bytefname.c_str(), NULL, NULL);
	avformat_find_stream_info(fmtcontext, NULL);

	//Locate video stream
	vidstreamIndex = av_find_best_stream(fmtcontext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	vidstream = fmtcontext->streams[vidstreamIndex];
	const AVCodec* ac = avcodec_find_decoder(vidstream->codecpar->codec_id);
	const AVCodecHWConfig* hwconfig;
	int n = 0;
	while (true)
	{
		hwconfig = avcodec_get_hw_config(ac, n);
		if (!hwconfig)
		{
			hardwareaccel = false;
			break;
		}
		else if ((hwconfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) && (hwconfig->device_type == hwtype))
		{
			hardwareaccel = true;
			break;
		}
		n++;
	}
	vidcodcontext = avcodec_alloc_context3(ac);
	vidcodcontext->get_format = GetHWFormat;
	int errcod = 0;
	if (hardwareaccel)
	{
		errcod = av_hwdevice_ctx_create(&hwcontext, hwtype, NULL, NULL, 0);
		if (errcod < 0)
		{
			hardwareaccel = false;
		}
		else
		{
			vidcodcontext->hw_device_ctx = av_buffer_ref(hwcontext);
		}
	}
	avcodec_parameters_to_context(vidcodcontext, vidstream->codecpar);
	avcodec_open2(vidcodcontext, ac, NULL);
	inWidth = vidcodcontext->width;
	inHeight = vidcodcontext->height;
	inPixFormat = vidcodcontext->pix_fmt;
	staticPixFmt = inPixFormat;
	vidorigBufsize = av_image_alloc(vidorigData, vidorigLineSize, inWidth, inHeight, inPixFormat, 1);
	curFrame = av_frame_alloc();
	curHWFrame = av_frame_alloc();
	curPacket = av_packet_alloc();
	nextPacket = av_packet_alloc();
	innumFrames = vidstream->nb_frames;

	//Locate audio stream
	audstreamIndex = av_find_best_stream(fmtcontext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (audstreamIndex != AVERROR_STREAM_NOT_FOUND)
	{
		audstream = fmtcontext->streams[audstreamIndex];
		ac = avcodec_find_decoder(audstream->codecpar->codec_id);
		audcodcontext = avcodec_alloc_context3(ac);
		avcodec_parameters_to_context(audcodcontext, audstream->codecpar);
		avcodec_open2(audcodcontext, ac, NULL);
		inAudioRate = audcodcontext->sample_rate;
		inAudFormat = audcodcontext->sample_fmt;
		inChLayout = audcodcontext->ch_layout;
	}

	//Setup rescaler
	inAspect = ((double)inWidth) / ((double)inHeight);
	if (inAspect > PC_98_ASPECT)
	{
		outWidth = PC_98_WIDTH;
		outHeight = (int)(((double)PC_98_WIDTH) / inAspect);
		topLeftX = 0;
		topLeftY = (PC_98_HEIGHT - outHeight) / 2;
	}
	else if (inAspect < PC_98_ASPECT)
	{
		outHeight = PC_98_HEIGHT;
		outWidth = (int)((double)PC_98_HEIGHT * inAspect);
		topLeftY = 0;
		topLeftX = (PC_98_WIDTH - outWidth) / 2;
	}
	else
	{
		outWidth = PC_98_WIDTH;
		outHeight = PC_98_HEIGHT;
		topLeftX = 0;
		topLeftY = 0;
	}
	scalercontext = sws_getContext(inWidth, inHeight, inPixFormat, outWidth, outHeight, AVPixelFormat::AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
	scalercontexthalfheight = sws_getContext(inWidth, inHeight, inPixFormat, outWidth, outHeight/2, AVPixelFormat::AV_PIX_FMT_BGRA, SWS_BILINEAR, NULL, NULL, NULL);
	vidscaleBufsize = av_image_alloc(vidscaleData, vidscaleLineSize, outWidth, outHeight, AVPixelFormat::AV_PIX_FMT_BGRA, 1);
	sws_scale(scalercontext, vidorigData, vidorigLineSize, 0, inHeight, vidscaleData, vidscaleLineSize);
	alreadyOpen = true;
	totalTime = (((double)(vidstream->duration)) * ((double)vidstream->time_base.num)) / ((double)vidstream->time_base.den);

	delete strconv;
}

void VideoConverterEngine::CloseDecoder()
{
	sws_freeContext(scalercontext);
	sws_freeContext(scalercontexthalfheight);
	avcodec_free_context(&vidcodcontext);
	avcodec_free_context(&audcodcontext);
	avformat_close_input(&fmtcontext);
}