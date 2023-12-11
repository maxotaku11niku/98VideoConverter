// 98VideoConverter: Converts videos into .98v format, suitable for use with 98VIDEOP.COM
// Maxim Hoxha 2022-2023
// This is the 'heart' of the encoder. Many settings are currently hardcoded for my ease
// This software uses code of FFmpeg (http://ffmpeg.org) licensed under the LGPLv2.1 (http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)

#pragma once

#include <cmath>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext.h>
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

constexpr float bayermatrix2[4] = { -0.5f,   0.0f,
                                     0.25f, -0.25f };

constexpr float bayermatrix4[16] = { -0.5f,     0.0f,    -0.375f,   0.125f,
                                      0.25f,   -0.25f,    0.375f,  -0.125f,
                                     -0.3125f,  0.1875f, -0.4375f,  0.0625f,
                                      0.4375f, -0.0625f,  0.3125f, -0.1875f };

constexpr float bayermatrix8[64] = { -0.5f,       0.0f,      -0.375f,     0.125f,    -0.46875f,   0.03125f,  -0.34375f,   0.15625f,
                                      0.25f,     -0.25f,      0.375f,    -0.125f,     0.28125f,  -0.21875f,   0.40625f,  -0.09375f,
                                     -0.3125f,    0.1875f,   -0.4375f,    0.0625f,   -0.28125f,   0.21875f,  -0.40625f,   0.09375f,
                                      0.4375f,   -0.0625f,    0.3125f,   -0.1875f,    0.46875f,  -0.03125f,   0.34375f,  -0.15625f,
                                     -0.453125f,  0.046875f, -0.328125f,  0.171875f, -0.484375f,  0.015625f, -0.359375f,  0.140625f,
                                      0.296875f, -0.203125f,  0.421875f, -0.078125f,  0.265625f, -0.234375f,  0.390625f, -0.109375f,
                                     -0.265625f,  0.234375f, -0.390625f,  0.109375f, -0.296875f,  0.203125f, -0.421875f,  0.078125f,
                                      0.484375f, -0.015625f,  0.359375f, -0.140625f,  0.453125f, -0.046875f,  0.328125f, -0.171875f };

//this should produce some amazing results
constexpr float vcdithermatrix16_1[256] = { -0.35546875f, -0.05859375f,  0.4765625f,   0.1796875f,  -0.1875f,     -0.046875f,   -0.421875f,    0.1171875f,  -0.16015625f,  0.46484375f, -0.09765625f, -0.40234375f,  0.12890625f,  0.28515625f, -0.27734375f, -0.4375f,
                                             0.0546875f,  -0.20703125f, -0.3046875f,   0.296875f,	 0.08203125f, -0.2734375f,   0.390625f,    0.30859375f, -0.23046875f,  0.03515625f,  0.18359375f,  0.3515625f,  -0.00390625f, -0.484375f,   -0.08984375f,  0.37109375f,
                                             0.26171875f,  0.41796875f,  0.00390625f, -0.40625f,	 0.2421875f,  -0.34765625f, -0.1328125f,   0.171875f,   -0.44140625f,  0.26953125f, -0.3671875f,  -0.1796875f,   0.07421875f,  0.484375f,   -0.15625f,     0.2109375f,
                                            -0.2421875f,  -0.125f,      -0.46484375f,  0.14453125f,  0.43359375f, -0.0859375f,   0.06640625f,  0.4921875f,  -0.3125f,     -0.0390625f,   0.4296875f,  -0.2578125f,   0.25f,       -0.3203125f,  -0.4140625f,   0.11328125f,
                                            -0.37890625f,  0.32421875f, -0.03515625f, -0.171875f,	 0.34375f,    -0.24609375f, -0.4921875f,  -0.0078125f,   0.328125f,	   0.09765625f, -0.109375f,   -0.46875f,     0.1640625f,   0.33203125f, -0.07421875f,  0.0234375f,
                                            -0.29296875f,  0.19140625f,  0.48046875f, -0.36328125f,  0.03125f,     0.203125f,    0.2578125f,  -0.21484375f, -0.38671875f, -0.1640625f,   0.2265625f,   0.39453125f, -0.0234375f,  -0.203125f,    0.4375f,      0.2734375f,
                                             0.3828125f,   0.05859375f, -0.2265625f,   0.109375f,   -0.30078125f, -0.42578125f,  0.40625f,    -0.0703125f,   0.125f,       0.45703125f, -0.28515625f, -0.4296875f,   0.046875f,   -0.34375f,     0.13671875f, -0.49609375f,
                                            -0.15234375f, -0.41796875f, -0.0546875f,   0.29296875f,  0.44140625f, -0.1484375f,   0.16015625f,  0.35546875f, -0.33984375f,  0.2890625f,   0.01171875f,  0.1875f,     -0.13671875f,  0.359375f,   -0.25390625f, -0.09375f,
                                             0.4609375f,   0.15234375f,  0.23046875f, -0.47265625f, -0.10546875f, -0.015625f,   -0.26953125f,  0.05078125f, -0.4609375f,  -0.1015625f,  -0.22265625f, -0.39453125f,  0.47265625f,  0.23828125f,  0.09375f,     0.0f,
                                            -0.375f,      -0.19921875f,  0.34765625f, -0.328125f,    0.0859375f,   0.3125f,     -0.3984375f,  -0.18359375f,  0.38671875f,  0.21484375f,  0.078125f,    0.31640625f, -0.0625f,     -0.44921875f, -0.32421875f,  0.30078125f,
                                             0.20703125f, -0.28125f,     0.02734375f,  0.40234375f, -0.234375f,    0.24609375f,  0.48828125f,  0.17578125f, -0.04296875f, -0.30859375f,  0.42578125f,  0.1328125f,  -0.265625f,   -0.17578125f,  0.4140625f,  -0.03125f,
                                             0.375f,      -0.12890625f,  0.12109375f, -0.4453125f,  -0.06640625f, -0.359375f,    0.01953125f, -0.5f,         0.27734375f, -0.14453125f, -0.43359375f, -0.3515625f,   0.0390625f,   0.265625f,    0.15625f,    -0.48046875f,
                                             0.0703125f,  -0.41015625f,  0.46875f,    -0.16796875f,  0.19921875f,  0.421875f,   -0.12109375f,  0.1015625f,  -0.26171875f,  0.453125f,   -0.01171875f,  0.36328125f, -0.1171875f,   0.49609375f, -0.23828125f, -0.078125f,
                                            -0.31640625f,  0.28125f,    -0.01953125f,  0.3203125f,   0.140625f,   -0.296875f,   -0.2109375f,   0.3359375f,  -0.3828125f,   0.1484375f,  -0.1953125f,   0.1953125f,  -0.45703125f,  0.08984375f, -0.390625f,    0.33984375f,
                                            -0.19140625f,  0.22265625f, -0.25f,       -0.37109375f,  0.04296875f, -0.453125f,    0.25390625f, -0.08203125f,  0.37890625f,  0.0625f,     -0.05078125f,  0.3046875f,  -0.3359375f,   0.234375f,   -0.140625f,    0.015625f,
                                             0.3984375f,   0.10546875f, -0.48828125f, -0.11328125f,  0.3671875f,   0.44921875f,  0.0078125f,  -0.33203125f,  0.21875f,    -0.4765625f,  -0.2890625f,   0.41015625f, -0.21875f,    -0.02734375f,  0.4453125f,   0.16796875f };

constexpr float vcdithermatrix16_2[256] = { -0.0546875f,  -0.17578125f,  0.1328125f,   0.25f,       -0.453125f,    0.41015625f,  0.21484375f, -0.42578125f,  0.28515625f, -0.22265625f, -0.44140625f,  0.01953125f, -0.1875f,      0.38671875f, -0.35546875f,  0.08984375f,
                                            -0.41015625f, -0.25390625f,  0.375f,      -0.3359375f,  -0.109375f,    0.11328125f, -0.0625f,     -0.31640625f,  0.04296875f, -0.1171875f,   0.3671875f,   0.125f,      -0.25f,        0.203125f,   -0.00390625f,  0.46484375f,
                                             0.29296875f,  0.03515625f, -0.48046875f, -0.01171875f,  0.4609375f,  -0.2421875f,   0.33984375f,  0.1484375f,   0.453125f,   -0.37109375f,  0.2265625f,   0.4921875f,  -0.328125f,   -0.12890625f, -0.44921875f,  0.15234375f,
                                            -0.10546875f,  0.40234375f,  0.234375f,   -0.14453125f,  0.30078125f, -0.39453125f, -0.19140625f, -0.48828125f, -0.0234375f,  -0.1640625f,   0.08203125f, -0.47265625f, -0.03515625f,  0.265625f,    0.34375f,    -0.2890625f,
                                             0.09375f,    -0.21484375f,  0.1640625f,  -0.30859375f,  0.0703125f,   0.1875f,      0.015625f,    0.390625f,    0.26953125f, -0.28125f,     0.3203125f,  -0.078125f,    0.41796875f,  0.06640625f, -0.171875f,   -0.390625f,
                                            -0.04296875f, -0.36328125f,  0.48828125f, -0.43359375f, -0.0703125f,   0.43359375f, -0.34765625f, -0.09765625f,  0.20703125f, -0.23046875f, -0.421875f,    0.17578125f, -0.3515625f,  -0.26171875f,  0.22265625f,  0.4453125f,
                                             0.12890625f,  0.0078125f,   0.28125f,    -0.15625f,     0.359375f,   -0.2734375f,   0.13671875f, -0.45703125f,  0.05078125f,  0.46875f,     0.109375f,    0.37890625f, -0.203125f,    0.02734375f,  0.35546875f, -0.46484375f,
                                            -0.33203125f,  0.328125f,   -0.234375f,   -0.49609375f,  0.24609375f, -0.02734375f, -0.20703125f,  0.33203125f, -0.140625f,   -0.3203125f,  -0.0078125f,  -0.12109375f,  0.25390625f, -0.41796875f,  0.15625f,    -0.08203125f,
                                             0.421875f,   -0.125f,       0.19921875f,  0.09765625f,  0.03125f,     0.47265625f, -0.40234375f,  0.23828125f,  0.4140625f,  -0.3828125f,   0.3046875f,  -0.4921875f,  -0.046875f,    0.484375f,   -0.15234375f, -0.27734375f,
                                             0.0546875f,  -0.40625f,     0.37109375f, -0.30078125f, -0.3671875f,  -0.11328125f,  0.16796875f,  0.078125f,   -0.05859375f, -0.1796875f,   0.140625f,    0.44140625f, -0.3046875f,   0.0859375f,   0.3125f,      0.18359375f,
                                             0.2734375f,  -0.01953125f, -0.1953125f,  -0.05078125f,  0.4375f,      0.296875f,   -0.24609375f, -0.4453125f,  -0.29296875f,  0.27734375f,  0.19140625f, -0.23828125f,  0.01171875f, -0.375f,      -0.2109375f,  -0.4765625f,
                                            -0.33984375f,  0.45703125f,  0.14453125f,  0.21875f,    -0.46875f,    -0.16015625f,  0.34765625f,  0.0f,         0.3984375f,   0.0390625f,  -0.4375f,     -0.1015625f,   0.3359375f,   0.2109375f,   0.40625f,    -0.07421875f,
                                             0.10546875f, -0.2578125f,  -0.4296875f,   0.39453125f,  0.05859375f, -0.32421875f,  0.12109375f, -0.0859375f,   0.4765625f,  -0.19921875f, -0.34375f,     0.3828125f,   0.1171875f,  -0.4140625f,  -0.16796875f,  0.00390625f,
                                             0.3515625f,  -0.13671875f,  0.2890625f,  -0.09375f,    -0.2265625f,   0.1796875f,   0.2578125f,  -0.359375f,   -0.484375f,    0.1015625f,   0.23046875f, -0.1484375f,  -0.0390625f,  -0.26953125f,  0.48046875f,  0.2421875f,
                                            -0.4609375f,   0.046875f,   -0.37890625f, -0.03125f,     0.49609375f, -0.3984375f,   0.36328125f, -0.1328125f,  -0.015625f,    0.31640625f, -0.296875f,    0.44921875f,  0.0625f,     -0.5f,         0.16015625f, -0.3125f,
                                             0.42578125f,  0.1953125f,   0.32421875f, -0.28515625f,  0.0234375f,  -0.18359375f,  0.07421875f, -0.265625f,    0.4296875f,   0.171875f,   -0.06640625f, -0.38671875f,  0.26171875f,  0.30859375f, -0.08984375f, -0.21875f };

constexpr float vcdithermatrix16_3[256] = {  0.48828125f,  0.37890625f,  0.015625f,   -0.3125f,      0.35546875f, -0.48828125f,  0.078125f,   -0.23046875f, -0.30859375f,  0.41015625f, -0.4375f,      0.0f,        -0.1328125f,   0.43359375f,  0.33984375f, -0.41015625f,
                                            -0.2734375f,  -0.15234375f, -0.0703125f,  -0.21484375f,  0.2109375f,  -0.3515625f,   0.30859375f,  0.14453125f, -0.39453125f, -0.1015625f,   0.1640625f,   0.36328125f, -0.29296875f,  0.2734375f,  -0.19140625f,  0.0859375f,
                                             0.2265625f,  -0.37109375f,  0.2890625f,  -0.43359375f,  0.4140625f,  -0.0078125f,  -0.16796875f,  0.44140625f,  0.03125f,     0.2578125f,   0.0703125f,  -0.375f,      -0.0625f,      0.12109375f, -0.328125f,   -0.015625f,
                                             0.33203125f,  0.05859375f,  0.45703125f,  0.16015625f,  0.09375f,    -0.11328125f, -0.27734375f, -0.45703125f,  0.3828125f,  -0.203125f,   -0.26171875f,  0.4765625f,  -0.48046875f,  0.20703125f,  0.39453125f, -0.44921875f,
                                             0.1328125f,  -0.12890625f, -0.25390625f, -0.046875f,   -0.38671875f,  0.23828125f,  0.3359375f,  -0.07421875f,  0.1953125f,  -0.33984375f, -0.0234375f,   0.3203125f,  -0.16015625f,  0.02734375f, -0.2265625f,  -0.08984375f,
                                            -0.3046875f,  -0.5f,         0.37109375f, -0.1875f,     -0.32421875f,  0.4921875f,   0.125f,       0.01171875f, -0.41796875f,  0.29296875f, -0.1171875f,   0.10546875f,  0.41796875f, -0.421875f,    0.27734375f,  0.46484375f,
                                             0.0078125f,   0.25390625f,  0.19140625f,  0.0390625f,   0.30078125f, -0.46875f,    -0.2109375f,  -0.30078125f,  0.4609375f,   0.15234375f, -0.4609375f,   0.234375f,   -0.2890625f,  -0.05078125f,  0.171875f,   -0.35546875f,
                                            -0.20703125f,  0.40625f,    -0.4140625f,  -0.09375f,     0.421875f,   -0.03125f,     0.08203125f, -0.14453125f,  0.3984375f,   0.046875f,   -0.2421875f,   0.3671875f,  -0.3671875f,  -0.18359375f,  0.07421875f,  0.34375f,
                                            -0.02734375f,  0.1015625f,  -0.15625f,    -0.28125f,     0.13671875f,  0.21875f,    -0.40625f,     0.26953125f, -0.359375f,   -0.17578125f, -0.08203125f, -0.00390625f,  0.203125f,    0.49609375f, -0.125f,      -0.3984375f,
                                             0.4453125f,   0.16796875f, -0.4765625f,   0.32421875f, -0.34765625f, -0.23828125f,  0.359375f,   -0.05859375f,  0.18359375f,  0.328125f,   -0.44140625f,  0.4296875f,   0.11328125f, -0.484375f,    0.3046875f,  -0.265625f,
                                            -0.33203125f,  0.265625f,   -0.0546875f,   0.48046875f,  0.01953125f, -0.12109375f,  0.4375f,     -0.49609375f, -0.26953125f,  0.08984375f,  0.25f,       -0.3203125f,  -0.234375f,    0.04296875f,  0.22265625f, -0.0859375f,
                                             0.375f,      -0.21875f,     0.0546875f,  -0.3828125f,   0.23046875f, -0.1796875f,   0.0625f,      0.15625f,    -0.01171875f,  0.47265625f, -0.390625f,   -0.13671875f, -0.03515625f,  0.390625f,   -0.4296875f,   0.12890625f,
                                            -0.1484375f,  -0.453125f,    0.19921875f, -0.28515625f,  0.40234375f, -0.4453125f,   0.28515625f, -0.31640625f, -0.19921875f, -0.09765625f,  0.28125f,     0.17578125f,  0.3515625f,  -0.171875f,   -0.296875f,    0.00390625f,
                                             0.42578125f,  0.09765625f,  0.31640625f, -0.10546875f, -0.01953125f,  0.1171875f,   0.34765625f, -0.36328125f,  0.38671875f,  0.0234375f,  -0.2578125f,  -0.42578125f,  0.06640625f,  0.46875f,    -0.37890625f,  0.296875f,
                                            -0.04296875f, -0.34375f,    -0.1953125f,   0.453125f,   -0.40234375f, -0.25f,       -0.078125f,    0.1875f,     -0.46484375f,  0.109375f,    0.44921875f, -0.3359375f,  -0.06640625f,  0.1484375f,   0.2421875f,  -0.24609375f,
                                             0.1796875f,  -0.47265625f,  0.140625f,    0.26171875f,  0.05078125f, -0.140625f,    0.484375f,    0.24609375f, -0.0390625f,  -0.1640625f,   0.3125f,      0.21484375f, -0.22265625f, -0.4921875f,   0.03515625f, -0.109375f };

constexpr float OkLabK1 = 0.206f;
constexpr float OkLabK2 = 0.03f;
constexpr float OkLabK3 = 1.17087378640776f;

constexpr float SRGBtoLMS[9] = { 0.4122214708f, 0.5363325363f, 0.0514459929f,
                                 0.2119034982f, 0.6806995451f, 0.1073969566f,
                                 0.0883024619f, 0.2817188376f, 0.6299787005f };

constexpr float CRLMStoOKLab[9] = { 0.2104542553f,  0.7936177850f, -0.0040720468f,
                                    1.9779984951f, -2.4285922050f,  0.4505937099f,
                                    0.0259040371f,  0.7827717662f, -0.8086757660f };

constexpr float SRGBtoLinearLUT[256] = { 0.0f,               0.000303526983549f, 0.000607053967098f, 0.000910580950647f, 0.001214107934195f, 0.001517634917744f, 0.001821161901293f, 0.002124688884842f,
                                         0.002428215868391f, 0.002731742851940f, 0.003035269835488f, 0.003346535763899f, 0.003676507324047f, 0.004024717018496f, 0.004391442037410f, 0.004776953480694f,
                                         0.005181516702338f, 0.005605391624203f, 0.006048833022857f, 0.006512090792594f, 0.006995410187265f, 0.007499032043226f, 0.008023192985385f, 0.008568125618069f,
                                         0.009134058702221f, 0.009721217320238f, 0.010329823029627f, 0.010960094006488f, 0.011612245179744f, 0.012286488356916f, 0.012983032342173f, 0.013702083047290f,
                                         0.014443843596093f, 0.015208514422913f, 0.015996293365510f, 0.016807375752887f, 0.017641954488384f, 0.018500220128380f, 0.019382360956936f, 0.020288563056652f,
                                         0.021219010376004f, 0.022173884793387f, 0.023153366178110f, 0.024157632448505f, 0.025186859627362f, 0.026241221894850f, 0.027320891639075f, 0.028426039504421f,
                                         0.029556834437809f, 0.030713443732994f, 0.031896033073012f, 0.033104766570885f, 0.034339806808682f, 0.035601314875020f, 0.036889450401100f, 0.038204371595347f,
                                         0.039546235276733f, 0.040915196906853f, 0.042311410620810f, 0.043735029256973f, 0.045186204385676f, 0.046665086336880f, 0.048171824226889f, 0.049706565984127f,
                                         0.051269458374043f, 0.052860647023180f, 0.054480276442442f, 0.056128490049600f, 0.057805430191067f, 0.059511238162981f, 0.061246054231618f, 0.063010017653168f,
                                         0.064803266692906f, 0.066625938643773f, 0.068478169844400f, 0.070360095696596f, 0.072271850682317f, 0.074213568380150f, 0.076185381481308f, 0.078187421805186f,
                                         0.080219820314468f, 0.082282707129815f, 0.084376211544149f, 0.086500462036550f, 0.088655586285773f, 0.090841711183408f, 0.093058962846687f, 0.095307466630965f,
                                         0.097587347141862f, 0.099898728247114f, 0.102241733088101f, 0.104616484091104f, 0.107023102978268f, 0.109461710778299f, 0.111932427836906f, 0.114435373826974f,
                                         0.116970667758511f, 0.119538427988346f, 0.122138772229602f, 0.124771817560950f, 0.127437680435647f, 0.130136476690364f, 0.132868321553818f, 0.135633329655206f,
                                         0.138431615032452f, 0.141263291140272f, 0.144128470858058f, 0.147027266497595f, 0.149959789810609f, 0.152926151996150f, 0.155926463707827f, 0.158960835060880f,
                                         0.162029375639111f, 0.165132194501668f, 0.168269400189691f, 0.171441100732823f, 0.174647403655585f, 0.177888415983629f, 0.181164244249860f, 0.184474994500441f,
                                         0.187820772300678f, 0.191201682740791f, 0.194617830441576f, 0.198069319559949f, 0.201556253794397f, 0.205078736390317f, 0.208636870145256f, 0.212230757414055f,
                                         0.215860500113899f, 0.219526199729269f, 0.223227957316809f, 0.226965873510098f, 0.230740048524349f, 0.234550582161005f, 0.238397573812271f, 0.242281122465555f,
                                         0.246201326707835f, 0.250158284729953f, 0.254152094330827f, 0.258182852921596f, 0.262250657529696f, 0.266355604802862f, 0.270497791013066f, 0.274677312060385f,
                                         0.278894263476810f, 0.283148740429992f, 0.287440837726917f, 0.291770649817536f, 0.296138270798321f, 0.300543794415776f, 0.304987314069886f, 0.309468922817509f,
                                         0.313988713375718f, 0.318546778125092f, 0.323143209112951f, 0.327778098056542f, 0.332451536346179f, 0.337163615048330f, 0.341914424908661f, 0.346704056355030f,
                                         0.351532599500439f, 0.356400144145944f, 0.361306779783510f, 0.366252595598839f, 0.371237680474149f, 0.376262122990907f, 0.381326011432530f, 0.386429433787049f,
                                         0.391572477749723f, 0.396755230725627f, 0.401977779832196f, 0.407240211901737f, 0.412542613483904f, 0.417885070848137f, 0.423267669986072f, 0.428690496613907f,
                                         0.434153636174749f, 0.439657173840919f, 0.445201194516228f, 0.450785782838223f, 0.456411023180405f, 0.462076999654407f, 0.467783796112159f, 0.473531496148010f,
                                         0.479320183100827f, 0.485149940056070f, 0.491020849847836f, 0.496932995060870f, 0.502886458032569f, 0.508881320854934f, 0.514917665376521f, 0.520995573204354f,
                                         0.527115125705813f, 0.533276404010505f, 0.539479489012107f, 0.545724461370187f, 0.552011401512000f, 0.558340389634268f, 0.564711505704929f, 0.571124829464873f,
                                         0.577580440429651f, 0.584078417891164f, 0.590618840919337f, 0.597201788363763f, 0.603827338855338f, 0.610495570807865f, 0.617206562419651f, 0.623960391675076f,
                                         0.630757136346147f, 0.637596873994033f, 0.644479681970582f, 0.651405637419824f, 0.658374817279448f, 0.665387298282272f, 0.672443156957688f, 0.679542469633094f,
                                         0.686685312435313f, 0.693871761291990f, 0.701101891932973f, 0.708375779891687f, 0.715693500506481f, 0.723055128921969f, 0.730460740090354f, 0.737910408772731f,
                                         0.745404209540387f, 0.752942216776078f, 0.760524504675292f, 0.768151147247507f, 0.775822218317424f, 0.783537791526194f, 0.791297940332630f, 0.799102738014409f,
                                         0.806952257669252f, 0.814846572216101f, 0.822785754396284f, 0.830769876774655f, 0.838799011740740f, 0.846873231509858f, 0.854992608124234f, 0.863157213454102f,
                                         0.871367119198797f, 0.879622396887832f, 0.887923117881966f, 0.896269353374266f, 0.904661174391150f, 0.913098651793419f, 0.921581856277295f, 0.930110858375424f,
                                         0.938685728457888f, 0.947306536733200f, 0.955973353249286f, 0.964686247894465f, 0.973445290398413f, 0.982250550333117f, 0.991102097113830f, 1.0f };

class VideoConverterEngine
{
public:
    VideoConverterEngine();

    void EncodeVideo(char* outFileName, bool (*progressCallback)(unsigned int) = NULL);
    void OpenForDecodeVideo(char* inFileName);
    unsigned char* GrabFrame(int framenumber);
    unsigned char* ReprocessGrabbedFrame();
    void CloseDecoder();
    inline int GetOrigFrameNumber() { return innumFrames; };
    inline unsigned char* GetConvertedImageData() { return convertedFrame; };
    inline unsigned char* GetSimulatedOutput() { return actualdisplaybuffer; };
    inline unsigned char* GetRescaledInputFrame() { return inputrescaledframe; };
    inline int GetBitrate() { return maxwordsperframe * 2; };
    inline float GetDitherFactor() { return ditherfactor; };
    inline float GetSaturationDitherFactor() { return satditherfactor; };
    inline float GetHueDitherFactor() { return hueditherfactor; };
    inline float GetUVBias() { return uvbias; };
    inline int GetSampleRateSpec() { return sampleratespec; };
    inline bool GetIsHalfVerticalResolution() { return isHalfVerticalResolution; };
    inline int GetFrameSkip() { return frameskip; };
    inline bool GetIsStereo() { return isStereo; };
    inline void SetBitrate(int bpf) { maxwordsperframe = bpf / 2; };
    inline void SetDitherFactor(float ditfac) { ditherfactor = ditfac; };
    inline void SetSaturationDitherFactor(float ditfac) { satditherfactor = ditfac; };
    inline void SetHueDitherFactor(float ditfac) { hueditherfactor = ditfac; };
    inline void SetUVBias(float uvb) { uvbias = uvb; };
    inline void SetSampleRateSpec(int spec) { sampleratespec = spec; };
    inline void SetIsHalfVerticalResolution(bool hvr) { isHalfVerticalResolution = hvr; };
    inline void SetFrameSkip(int fskip) { frameskip = fskip; };
    inline void SetIsStereo(bool st) { isStereo = st; };
private:
    inline float SRGBGammaTransform(float val)
    {
        return val > 0.04045f ? powf((val + 0.055f) / 1.055f, 2.4f) : (val / 12.92f);
    }

    inline unsigned int GetClosestOKLabColour(const float L, const float a, const float b)
    {
        int dists[16]; //Integer comparisons are faster
        const float uvb = uvbias;
        const float* const Lptr = OKLabpalvec[0];
        const float* const aptr = OKLabpalvec[1];
        const float* const bptr = OKLabpalvec[2];
        for (int j = 0; j < 16; j++) //Organised like this to get the compiler to vectorise it
        {
            const float Ldif = (L - Lptr[j]) * uvb;
            const float adif = a - aptr[j];
            const float bdif = b - bptr[j];
            dists[j] = (int)((Ldif * Ldif + adif * adif + bdif * bdif) * 1000000.0f);
        }
        int lowestDistance = dists[0];
        int setIndex = 0;
        for (int j = 1; j < 16; j++) //Organised like this to get the compiler to generate optimal cmov-based assembly
        {
            const bool isNewLowest = dists[j] < lowestDistance;
            lowestDistance = isNewLowest ? dists[j] : lowestDistance;
            setIndex = isNewLowest ? j : setIndex;
        }
        return palette[setIndex];
    }

    inline unsigned int BayerDither2x2(unsigned int argbcolour, unsigned int x, unsigned int y)
    {
        const int R = (argbcolour & 0x00FF0000) >> 16;
        const int G = (argbcolour & 0x0000FF00) >> 8;
        const int B = argbcolour & 0x000000FF;
        float l = SRGBtoLMS[0] * SRGBtoLinearLUT[R] + SRGBtoLMS[1] * SRGBtoLinearLUT[G] + SRGBtoLMS[2] * SRGBtoLinearLUT[B];
        float m = SRGBtoLMS[3] * SRGBtoLinearLUT[R] + SRGBtoLMS[4] * SRGBtoLinearLUT[G] + SRGBtoLMS[5] * SRGBtoLinearLUT[B];
        float s = SRGBtoLMS[6] * SRGBtoLinearLUT[R] + SRGBtoLMS[7] * SRGBtoLinearLUT[G] + SRGBtoLMS[8] * SRGBtoLinearLUT[B];
        l = cbrtf(l); m = cbrtf(m); s = cbrtf(s);
        float L = CRLMStoOKLab[0] * l + CRLMStoOKLab[1] * m + CRLMStoOKLab[2] * s;
        float a = CRLMStoOKLab[3] * l + CRLMStoOKLab[4] * m + CRLMStoOKLab[5] * s;
        float b = CRLMStoOKLab[6] * l + CRLMStoOKLab[7] * m + CRLMStoOKLab[8] * s;
        L = (OkLabK3 * L - OkLabK1 + sqrtf((OkLabK3 * L - OkLabK1) * (OkLabK3 * L - OkLabK1) + 4.0f * OkLabK2 * OkLabK3 * L)) * 0.5f;
        float sat = sqrtf(a * a + b * b);
        float hue = atan2f(b, a);
        const unsigned int matind1 = (x & 0x01) + 2 * (y & 0x01);
        const unsigned int matind2 = ((x + 1) & 0x01) + 2 * (y & 0x01);
        const unsigned int matind3 = (x & 0x01) + 2 * ((y + 1) & 0x01);
        const float midsat = -satditherfactor * bayermatrix2[matind2];
        L += ditherfactor * bayermatrix2[matind1];
        sat *= 1.0f + midsat;
        sat += midsat * 0.5f;
        hue += hueditherfactor * bayermatrix2[matind3];
        a = sat * cosf(hue);
        b = sat * sinf(hue);
        return GetClosestOKLabColour(L, a, b);
    }

    inline unsigned int BayerDither4x4(unsigned int argbcolour, unsigned int x, unsigned int y)
    {
        const int R = (argbcolour & 0x00FF0000) >> 16;
        const int G = (argbcolour & 0x0000FF00) >> 8;
        const int B = argbcolour & 0x000000FF;
        float l = SRGBtoLMS[0] * SRGBtoLinearLUT[R] + SRGBtoLMS[1] * SRGBtoLinearLUT[G] + SRGBtoLMS[2] * SRGBtoLinearLUT[B];
        float m = SRGBtoLMS[3] * SRGBtoLinearLUT[R] + SRGBtoLMS[4] * SRGBtoLinearLUT[G] + SRGBtoLMS[5] * SRGBtoLinearLUT[B];
        float s = SRGBtoLMS[6] * SRGBtoLinearLUT[R] + SRGBtoLMS[7] * SRGBtoLinearLUT[G] + SRGBtoLMS[8] * SRGBtoLinearLUT[B];
        l = cbrtf(l); m = cbrtf(m); s = cbrtf(s);
        float L = CRLMStoOKLab[0] * l + CRLMStoOKLab[1] * m + CRLMStoOKLab[2] * s;
        float a = CRLMStoOKLab[3] * l + CRLMStoOKLab[4] * m + CRLMStoOKLab[5] * s;
        float b = CRLMStoOKLab[6] * l + CRLMStoOKLab[7] * m + CRLMStoOKLab[8] * s;
        L = (OkLabK3 * L - OkLabK1 + sqrtf((OkLabK3 * L - OkLabK1) * (OkLabK3 * L - OkLabK1) + 4.0f * OkLabK2 * OkLabK3 * L)) * 0.5f;
        float sat = sqrtf(a * a + b * b);
        float hue = atan2f(b, a);
        const unsigned int matind1 = (x & 0x03) + 4 * (y & 0x03);
        const unsigned int matind2 = ((x + 1) & 0x03) + 4 * ((y + 3) & 0x03);
        const unsigned int matind3 = ((x + 2) & 0x03) + 4 * ((y + 1) & 0x03);
        const float midsat = -satditherfactor * bayermatrix4[matind2];
        L += ditherfactor * bayermatrix4[matind1];
        sat *= 1.0f + midsat;
        sat += midsat * 0.5f;
        hue += hueditherfactor * bayermatrix4[matind3];
        a = sat * cosf(hue);
        b = sat * sinf(hue);
        return GetClosestOKLabColour(L, a, b);
    }

    inline unsigned int BayerDither8x8(const unsigned int argbcolour, const unsigned int x, const unsigned int y)
    {
        const int R = (argbcolour & 0x00FF0000) >> 16;
        const int G = (argbcolour & 0x0000FF00) >> 8;
        const int B = argbcolour & 0x000000FF;
        float l = SRGBtoLMS[0] * SRGBtoLinearLUT[R] + SRGBtoLMS[1] * SRGBtoLinearLUT[G] + SRGBtoLMS[2] * SRGBtoLinearLUT[B];
        float m = SRGBtoLMS[3] * SRGBtoLinearLUT[R] + SRGBtoLMS[4] * SRGBtoLinearLUT[G] + SRGBtoLMS[5] * SRGBtoLinearLUT[B];
        float s = SRGBtoLMS[6] * SRGBtoLinearLUT[R] + SRGBtoLMS[7] * SRGBtoLinearLUT[G] + SRGBtoLMS[8] * SRGBtoLinearLUT[B];
        l = cbrtf(l); m = cbrtf(m); s = cbrtf(s);
        float L = CRLMStoOKLab[0] * l + CRLMStoOKLab[1] * m + CRLMStoOKLab[2] * s;
        float a = CRLMStoOKLab[3] * l + CRLMStoOKLab[4] * m + CRLMStoOKLab[5] * s;
        float b = CRLMStoOKLab[6] * l + CRLMStoOKLab[7] * m + CRLMStoOKLab[8] * s;
        L = (OkLabK3 * L - OkLabK1 + sqrtf((OkLabK3 * L - OkLabK1) * (OkLabK3 * L - OkLabK1) + 4.0f * OkLabK2 * OkLabK3 * L)) * 0.5f;
        float sat = sqrtf(a * a + b * b);
        float hue = atan2f(b, a);
        const unsigned int matind1 = (x & 0x07) + 8 * (y & 0x07);
        const unsigned int matind2 = ((x + 1) & 0x07) + 8 * ((y + 6) & 0x07);
        const unsigned int matind3 = ((x + 4) & 0x07) + 8 * ((y + 3) & 0x07);
        const float midsat = -satditherfactor * bayermatrix8[matind2];
        L += ditherfactor * bayermatrix8[matind1];
        sat *= 1.0f + midsat;
        sat += midsat * 0.5f;
        hue += hueditherfactor * bayermatrix8[matind3];
        a = sat * cosf(hue);
        b = sat * sinf(hue);
        return GetClosestOKLabColour(L, a, b);
    }

    inline unsigned int VoidClusterDither16x16(const unsigned int argbcolour, const unsigned int x, const unsigned int y)
    {
        const int R = (argbcolour & 0x00FF0000) >> 16;
        const int G = (argbcolour & 0x0000FF00) >> 8;
        const int B = argbcolour & 0x000000FF;
        float l = SRGBtoLMS[0] * SRGBtoLinearLUT[R] + SRGBtoLMS[1] * SRGBtoLinearLUT[G] + SRGBtoLMS[2] * SRGBtoLinearLUT[B];
        float m = SRGBtoLMS[3] * SRGBtoLinearLUT[R] + SRGBtoLMS[4] * SRGBtoLinearLUT[G] + SRGBtoLMS[5] * SRGBtoLinearLUT[B];
        float s = SRGBtoLMS[6] * SRGBtoLinearLUT[R] + SRGBtoLMS[7] * SRGBtoLinearLUT[G] + SRGBtoLMS[8] * SRGBtoLinearLUT[B];
        l = cbrtf(l); m = cbrtf(m); s = cbrtf(s);
        float L = CRLMStoOKLab[0] * l + CRLMStoOKLab[1] * m + CRLMStoOKLab[2] * s;
        float a = CRLMStoOKLab[3] * l + CRLMStoOKLab[4] * m + CRLMStoOKLab[5] * s;
        float b = CRLMStoOKLab[6] * l + CRLMStoOKLab[7] * m + CRLMStoOKLab[8] * s;
        L = (OkLabK3 * L - OkLabK1 + sqrtf((OkLabK3 * L - OkLabK1) * (OkLabK3 * L - OkLabK1) + 4.0f * OkLabK2 * OkLabK3 * L)) * 0.5f;
        float sat = sqrtf(a * a + b * b);
        float hue = atan2f(b, a);
        //This extra index messing around is to reduce the appearance of patterning
        const unsigned int matind = (x & 0x0F) + 16 * (y & 0x0F);
        const unsigned int matind1 = (matind + (y / 16) * (7 + 12 * (y / 32))) & 0xFF;
        const unsigned int matind2 = (matind + (y / 16) * (9 - 13 * (y / 32))) & 0xFF;
        const unsigned int matind3 = (matind + (y / 16) * (19 + 24 * (y / 32))) & 0xFF;
        const float midsat = satditherfactor * vcdithermatrix16_2[matind2];
        L += ditherfactor * vcdithermatrix16_1[matind1];
        sat *= 1.0f + midsat;
        sat += midsat * 0.5f;
        hue += hueditherfactor * vcdithermatrix16_3[matind3];
        a = sat * cosf(hue);
        b = sat * sinf(hue);
        return GetClosestOKLabColour(L, a, b);
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
    short* audfullstreamShortDiff;
    unsigned char* audfullstreamByteDiff;
    int curtotalByteStreamSize;
    bool isForcedBuzzerAudio;
    bool isStereo;
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
    float floatpal[16][4];
    float OKLabpal[16][3];
    float OKLabpalvec[3][16]; //to aid vectorisation
    float uvbias;
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
