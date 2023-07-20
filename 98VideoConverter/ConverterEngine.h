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

#define D65_X 0.950489f
#define D65_Y 1.0f
#define D65_Z 1.08884f

#define CD_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define CD_CLAMP_HIGH(x, high)  (((x) > (high)) ? (high) : (x))
#define CD_CLAMP_LOW(x, low)  (((x) < (low)) ? (low) : (x))

constexpr float odithermatrix2[4] = { -0.5f,   0.0f,
                                       0.25f, -0.25f };

constexpr float odithermatrix4[16] =  { -0.5f,     0.0f,    -0.375f,   0.125f,
                                         0.25f,   -0.25f,    0.375f,  -0.125f,
                                        -0.3125f,  0.1875f, -0.4375f,  0.0625f,
                                         0.4375f, -0.0625f,  0.3125f, -0.1875f };

constexpr float odithermatrix8[64] =   { -0.5f,       0.0f,      -0.375f,     0.125f,    -0.46875f,   0.03125f,  -0.34375f,   0.15625f,
                                          0.25f,     -0.25f,      0.375f,    -0.125f,     0.28125f,  -0.21875f,   0.40625f,  -0.09375f,
                                         -0.3125f,    0.1875f,   -0.4375f,    0.0625f,   -0.28125f,   0.21875f,  -0.40625f,   0.09375f,
                                          0.4375f,   -0.0625f,    0.3125f,   -0.1875f,    0.46875f,  -0.03125f,   0.34375f,  -0.15625f,
                                         -0.453125f,  0.046875f, -0.328125f,  0.171875f, -0.484375f,  0.015625f, -0.359375f,  0.140625f,
                                          0.296875f, -0.203125f,  0.421875f, -0.078125f,  0.265625f, -0.234375f,  0.390625f, -0.109375f,
                                         -0.265625f,  0.234375f, -0.390625f,  0.109375f, -0.296875f,  0.203125f, -0.421875f,  0.078125f,
                                          0.484375f, -0.015625f,  0.359375f, -0.140625f,  0.453125f, -0.046875f,  0.328125f, -0.171875f };

//this should produce some amazing results
constexpr float vcdithermatrix16[256] = { -0.35546875, -0.05859375,	 0.4765625,	  0.1796875,  -0.1875,     -0.046875,   -0.421875,    0.1171875,  -0.16015625,  0.46484375, -0.09765625, -0.40234375,  0.12890625,  0.28515625, -0.27734375, -0.4375,
                                           0.0546875,  -0.20703125, -0.3046875,	  0.296875,	   0.08203125, -0.2734375,	 0.390625,    0.30859375, -0.23046875,	0.03515625,  0.18359375,  0.3515625,  -0.00390625, -0.484375,   -0.08984375,  0.37109375,
                                           0.26171875,	0.41796875,	 0.00390625, -0.40625,	   0.2421875,  -0.34765625, -0.1328125,   0.171875,   -0.44140625,  0.26953125, -0.3671875,  -0.1796875,   0.07421875,  0.484375,   -0.15625,     0.2109375,
                                          -0.2421875,  -0.125,      -0.46484375,  0.14453125,  0.43359375, -0.0859375,	 0.06640625,  0.4921875,  -0.3125,     -0.0390625,   0.4296875,  -0.2578125,   0.25,       -0.3203125,  -0.4140625,   0.11328125,
                                          -0.37890625,	0.32421875, -0.03515625, -0.171875,	   0.34375,    -0.24609375, -0.4921875,  -0.0078125,   0.328125,	0.09765625, -0.109375,   -0.46875,     0.1640625,   0.33203125, -0.07421875,  0.0234375,
                                          -0.29296875,	0.19140625,  0.48046875, -0.36328125,  0.03125,     0.203125,    0.2578125,  -0.21484375, -0.38671875, -0.1640625,   0.2265625,   0.39453125, -0.0234375,  -0.203125,    0.4375,      0.2734375,
                                           0.3828125,   0.05859375, -0.2265625,   0.109375,   -0.30078125, -0.42578125,  0.40625,    -0.0703125,   0.125,       0.45703125, -0.28515625, -0.4296875,   0.046875,   -0.34375,     0.13671875, -0.49609375,
                                          -0.15234375, -0.41796875, -0.0546875,	  0.29296875,  0.44140625, -0.1484375,   0.16015625,  0.35546875, -0.33984375,  0.2890625,   0.01171875,  0.1875,     -0.13671875,  0.359375,   -0.25390625, -0.09375,
                                           0.4609375,   0.15234375,	 0.23046875, -0.47265625, -0.10546875, -0.015625,   -0.26953125,  0.05078125, -0.4609375,  -0.1015625,  -0.22265625, -0.39453125,  0.47265625,  0.23828125,  0.09375,     0.0,
                                          -0.375,      -0.19921875,	 0.34765625, -0.328125,    0.0859375,   0.3125,     -0.3984375,  -0.18359375,  0.38671875,  0.21484375,  0.078125,    0.31640625, -0.0625,     -0.44921875, -0.32421875,  0.30078125,
                                           0.20703125, -0.28125,     0.02734375,  0.40234375, -0.234375,    0.24609375,  0.48828125,  0.17578125, -0.04296875, -0.30859375,  0.42578125,  0.1328125,  -0.265625,   -0.17578125,  0.4140625,  -0.03125,
                                           0.375,      -0.12890625,  0.12109375, -0.4453125,  -0.06640625, -0.359375,    0.01953125, -0.5,         0.27734375, -0.14453125, -0.43359375, -0.3515625,   0.0390625,   0.265625,    0.15625,    -0.48046875,
                                           0.0703125,  -0.41015625,  0.46875,    -0.16796875,  0.19921875,  0.421875,   -0.12109375,  0.1015625,  -0.26171875,  0.453125,   -0.01171875,  0.36328125, -0.1171875,   0.49609375, -0.23828125, -0.078125,
                                          -0.31640625,  0.28125,    -0.01953125,  0.3203125,   0.140625,   -0.296875,   -0.2109375,   0.3359375,  -0.3828125,   0.1484375,  -0.1953125,   0.1953125,  -0.45703125,  0.08984375, -0.390625,    0.33984375,
                                          -0.19140625,  0.22265625, -0.25,       -0.37109375,  0.04296875, -0.453125,    0.25390625, -0.08203125,  0.37890625,  0.0625,     -0.05078125,  0.3046875,  -0.3359375,   0.234375,   -0.140625,    0.015625,
                                           0.3984375,   0.10546875, -0.48828125, -0.11328125,  0.3671875,   0.44921875,  0.0078125,  -0.33203125,  0.21875,    -0.4765625,  -0.2890625,   0.41015625, -0.21875,    -0.02734375,  0.4453125,   0.16796875 };

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

constexpr float SRGBtoXYZ[9] = { 0.4124f, 0.3576f, 0.1805f,
                                 0.2126f, 0.7152f, 0.0722f,
                                 0.0193f, 0.1192f, 0.9505f };

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

constexpr float LabTransferLUT[512] = { 0.137931034482759f, 0.153169854516099f, 0.168408674549440f, 0.183647494582780f, 0.198886314616121f, 0.213886333004646f, 0.227288144598433f, 0.239272275594637f,
                                        0.250162972666756f, 0.260179976285348f, 0.269479893237379f, 0.278178735304251f, 0.286365117771116f, 0.294108437607342f, 0.301464176677931f, 0.308477471870579f,
                                        0.315185595167122f, 0.321619723141185f, 0.327806228883058f, 0.333767644356577f, 0.339523390013196f, 0.345090336662841f, 0.350483244242960f, 0.355715108774249f,
                                        0.360797439835453f, 0.365740484760828f, 0.370553411493183f, 0.375244459000134f, 0.379821061985752f, 0.384289955043832f, 0.388657260228096f, 0.392928561140032f,
                                        0.397108965974701f, 0.401203161461214f, 0.405215459246940f, 0.409149835973427f, 0.413009968056422f, 0.416799261996437f, 0.420520880898678f, 0.424177767762972f,
                                        0.427772666009293f, 0.431308137627451f, 0.434786579276822f, 0.438210236610645f, 0.441581217057151f, 0.444901501254858f, 0.448172953310320f, 0.451397330022406f,
                                        0.454576289196866f, 0.457711397157891f, 0.460804135548922f, 0.463855907502761f, 0.466868043250618f, 0.469841805230866f, 0.472778392750683f, 0.475678946247212f,
                                        0.478544551189275f, 0.481376241655790f, 0.484175003622878f, 0.486941777987941f, 0.489677463355847f, 0.492382918609576f, 0.495058965285231f, 0.497706389769222f,
                                        0.500325945333512f, 0.502918354023218f, 0.505484308409355f, 0.508024473218254f, 0.510539486848038f, 0.513029962781513f, 0.515496490903956f, 0.517939638733444f,
                                        0.520359952570695f, 0.522757958574710f, 0.525134163769959f, 0.527489056990327f, 0.529823109764579f, 0.532136777147688f, 0.534430498501987f, 0.536704698231785f,
                                        0.538959786474757f, 0.541196159753179f, 0.543414201587781f, 0.545614283076807f, 0.547796763442655f, 0.549961990548244f, 0.552110301385165f, 0.554242022535420f,
                                        0.556357470608501f, 0.558456952655374f, 0.560540766560826f, 0.562609201415551f, 0.564662537869225f, 0.566701048465720f, 0.568724997961573f, 0.570734643628690f,
                                        0.572730235542231f, 0.574712016854555f, 0.576680224056019f, 0.578635087223410f, 0.580576830256697f, 0.582505671104777f, 0.584421821980818f, 0.586325489567785f,
                                        0.588216875214683f, 0.590096175124022f, 0.591963580530975f, 0.593819277874672f, 0.595663448962051f, 0.597496271124644f, 0.599317917368675f, 0.601128556518806f,
                                        0.602928353355862f, 0.604717468748826f, 0.606496059781411f, 0.608264279873450f, 0.610022278897391f, 0.611770203290102f, 0.613508196160243f, 0.615236397391395f,
                                        0.616954943741157f, 0.618663968936405f, 0.620363603764878f, 0.622053976163280f, 0.623735211302038f, 0.625407431666890f, 0.627070757137425f, 0.628725305062727f,
                                        0.630371190334244f, 0.632008525456011f, 0.633637420612335f, 0.635257983733057f, 0.636870320556498f, 0.638474534690182f, 0.640070727669432f, 0.641658999013939f,
                                        0.643239446282369f, 0.644812165125108f, 0.646377249335212f, 0.647934790897640f, 0.649484880036835f, 0.651027605262727f, 0.652563053415212f, 0.654091309707176f,
                                        0.655612457766116f, 0.657126579674421f, 0.658633756008350f, 0.660134065875771f, 0.661627586952713f, 0.663114395518753f, 0.664594566491309f, 0.666068173458861f,
                                        0.667535288713155f, 0.668995983280410f, 0.670450326951589f, 0.671898388311739f, 0.673340234768464f, 0.674775932579537f, 0.676205546879701f, 0.677629141706673f,
                                        0.679046780026393f, 0.680458523757533f, 0.681864433795299f, 0.683264570034552f, 0.684658991392261f, 0.686047755829323f, 0.687430920371770f, 0.688808541131374f,
                                        0.690180673325683f, 0.691547371297499f, 0.692908688533818f, 0.694264677684258f, 0.695615390578972f, 0.696960878246090f, 0.698301190928684f, 0.699636378101277f,
                                        0.700966488485919f, 0.702291570067833f, 0.703611670110650f, 0.704926835171246f, 0.706237111114192f, 0.707542543125833f, 0.708843175727998f, 0.710139052791368f,
                                        0.711430217548497f, 0.712716712606506f, 0.713998579959455f, 0.715275861000410f, 0.716548596533204f, 0.717816826783912f, 0.719080591412035f, 0.720339929521420f,
                                        0.721594879670906f, 0.722845479884715f, 0.724091767662591f, 0.725333779989696f, 0.726571553346270f, 0.727805123717060f, 0.729034526600530f, 0.730259797017844f,
                                        0.731480969521655f, 0.732698078204674f, 0.733911156708048f, 0.735120238229545f, 0.736325355531547f, 0.737526540948864f, 0.738723826396369f, 0.739917243376463f,
                                        0.741106822986365f, 0.742292595925248f, 0.743474592501207f, 0.744652842638080f, 0.745827375882114f, 0.746998221408484f, 0.748165408027672f, 0.749328964191707f,
                                        0.750488918000268f, 0.751645297206655f, 0.752798129223633f, 0.753947441129154f, 0.755093259671950f, 0.756235611277014f, 0.757374522050964f, 0.758510017787292f,
                                        0.759642123971504f, 0.760770865786159f, 0.761896268115791f, 0.763018355551745f, 0.764137152396899f, 0.765252682670301f, 0.766364970111706f, 0.767474038186022f,
                                        0.768579910087663f, 0.769682608744826f, 0.770782156823665f, 0.771878576732398f, 0.772971890625323f, 0.774062120406758f, 0.775149287734905f, 0.776233414025633f,
                                        0.777314520456191f, 0.778392627968854f, 0.779467757274485f, 0.780539928856043f, 0.781609162972013f, 0.782675479659777f, 0.783738898738920f, 0.784799439814466f,
                                        0.785857122280064f, 0.786911965321110f, 0.787963987917804f, 0.789013208848161f, 0.790059646690958f, 0.791103319828627f, 0.792144246450105f, 0.793182444553612f,
                                        0.794217931949401f, 0.795250726262438f, 0.796280844935048f, 0.797308305229503f, 0.798333124230572f, 0.799355318848018f, 0.800374905819053f, 0.801391901710755f,
                                        0.802406322922428f, 0.803418185687938f, 0.804427506077994f, 0.805434300002398f, 0.806438583212246f, 0.807440371302105f, 0.808439679712136f, 0.809436523730193f,
                                        0.810430918493879f, 0.811422878992573f, 0.812412420069412f, 0.813399556423251f, 0.814384302610580f, 0.815366673047419f, 0.816346682011169f, 0.817324343642442f,
                                        0.818299671946855f, 0.819272680796796f, 0.820243383933160f, 0.821211794967059f, 0.822177927381498f, 0.823141794533029f, 0.824103409653377f, 0.825062785851036f,
                                        0.826019936112844f, 0.826974873305526f, 0.827927610177224f, 0.828878159358986f, 0.829826533366243f, 0.830772744600264f, 0.831716805349573f, 0.832658727791363f,
                                        0.833598523992874f, 0.834536205912752f, 0.835471785402392f, 0.836405274207253f, 0.837336683968158f, 0.838266026222570f, 0.839193312405849f, 0.840118553852493f,
                                        0.841041761797355f, 0.841962947376844f, 0.842882121630107f, 0.843799295500195f, 0.844714479835207f, 0.845627685389419f, 0.846538922824399f, 0.847448202710102f,
                                        0.848355535525943f, 0.849260931661870f, 0.850164401419403f, 0.851065955012666f, 0.851965602569410f, 0.852863354132005f, 0.853759219658433f, 0.854653209023259f,
                                        0.855545332018586f, 0.856435598355001f, 0.857324017662506f, 0.858210599491433f, 0.859095353313347f, 0.859978288521939f, 0.860859414433902f, 0.861738740289796f,
                                        0.862616275254902f, 0.863492028420060f, 0.864366008802502f, 0.865238225346665f, 0.866108686924999f, 0.866977402338761f, 0.867844380318796f, 0.868709629526314f,
                                        0.869573158553645f, 0.870434975924995f, 0.871295090097184f, 0.872153509460378f, 0.873010242338808f, 0.873865296991480f, 0.874718681612876f, 0.875570404333646f,
                                        0.876420473221290f, 0.877268896280829f, 0.878115681455467f, 0.878960836627250f, 0.879804369617708f, 0.880646288188492f, 0.881486600042007f, 0.882325312822025f,
                                        0.883162434114302f, 0.883997971447180f, 0.884831932292183f, 0.885664324064607f, 0.886495154124096f, 0.887324429775217f, 0.888152158268027f, 0.888978346798627f,
                                        0.889803002509715f, 0.890626132491131f, 0.891447743780388f, 0.892267843363206f, 0.893086438174035f, 0.893903535096568f, 0.894719140964251f, 0.895533262560790f,
                                        0.896345906620641f, 0.897157079829505f, 0.897966788824812f, 0.898775040196194f, 0.899581840485965f, 0.900387196189579f, 0.901191113756096f, 0.901993599588632f,
                                        0.902794660044813f, 0.903594301437212f, 0.904392530033792f, 0.905189352058337f, 0.905984773690877f, 0.906778801068112f, 0.907571440283826f, 0.908362697389304f,
                                        0.909152578393732f, 0.909941089264604f, 0.910728235928113f, 0.911514024269550f, 0.912298460133686f, 0.913081549325155f, 0.913863297608834f, 0.914643710710217f,
                                        0.915422794315781f, 0.916200554073352f, 0.916976995592467f, 0.917752124444727f, 0.918525946164151f, 0.919298466247524f, 0.920069690154739f, 0.920839623309137f,
                                        0.921608271097843f, 0.922375638872099f, 0.923141731947589f, 0.923906555604764f, 0.924670115089165f, 0.925432415611736f, 0.926193462349138f, 0.926953260444060f,
                                        0.927711815005522f, 0.928469131109181f, 0.929225213797626f, 0.929980068080677f, 0.930733698935671f, 0.931486111307762f, 0.932237310110194f, 0.932987300224591f,
                                        0.933736086501235f, 0.934483673759341f, 0.935230066787329f, 0.935975270343097f, 0.936719289154284f, 0.937462127918537f, 0.938203791303772f, 0.938944283948429f,
                                        0.939683610461731f, 0.940421775423935f, 0.941158783386580f, 0.941894638872737f, 0.942629346377251f, 0.943362910366984f, 0.944095335281054f, 0.944826625531069f,
                                        0.945556785501365f, 0.946285819549234f, 0.947013732005152f, 0.947740527173011f, 0.948466209330336f, 0.949190782728511f, 0.949914251592996f, 0.950636620123547f,
                                        0.951357892494425f, 0.952078072854613f, 0.952797165328026f, 0.953515174013716f, 0.954232102986078f, 0.954947956295056f, 0.955662737966343f, 0.956376452001579f,
                                        0.957089102378549f, 0.957800693051379f, 0.958511227950729f, 0.959220710983981f, 0.959929146035434f, 0.960636536966483f, 0.961342887615814f, 0.962048201799578f,
                                        0.962752483311579f, 0.963455735923451f, 0.964157963384834f, 0.964859169423554f, 0.965559357745794f, 0.966258532036268f, 0.966956695958390f, 0.967653853154446f,
                                        0.968350007245756f, 0.969045161832846f, 0.969739320495606f, 0.970432486793456f, 0.971124664265507f, 0.971815856430714f, 0.972506066788043f, 0.973195298816618f,
                                        0.973883555975882f, 0.974570841705744f, 0.975257159426735f, 0.975942512540156f, 0.976626904428225f, 0.977310338454227f, 0.977992817962657f, 0.978674346279364f,
                                        0.979354926711694f, 0.980034562548634f, 0.980713257060947f, 0.981391013501315f, 0.982067835104471f, 0.982743725087341f, 0.983418686649175f, 0.984092722971679f,
                                        0.984765837219151f, 0.985438032538609f, 0.986109312059920f, 0.986779678895931f, 0.987449136142592f, 0.988117686879085f, 0.988785334167950f, 0.989452081055203f,
                                        0.990117930570462f, 0.990782885727071f, 0.991446949522211f, 0.992110124937030f, 0.992772414936751f, 0.993433822470797f, 0.994094350472898f, 0.994754001861215f,
                                        0.995412779538444f, 0.996070686391936f, 0.996727725293804f, 0.997383899101035f, 0.998039210655599f, 0.998693662784557f, 0.999347258300169f, 1.0f };

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
    inline float SRGBGammaTransform(float val)
    {
        return val > 0.04045f ? powf((val + 0.055f) / 1.055f, 2.4f) : (val / 12.92f);
    }

    inline float LabTransferFunction(float val)
    {
        return val > (0.008856451679f) ? powf(val, 0.333333333333333333f) : ((val * 7.787037037f) + 0.1379310345f);
    }

    inline float LabTransferFunctionLerp(float val)
    {
        if (val <= 0.0f) return LabTransferLUT[0];
        else if (val >= 1.0f) return LabTransferLUT[511];
        const float rescval = val * 511.0f;
        const int lbound = (int)rescval;
        const float lerpfac = rescval - (float)lbound;
        const int ubound = lbound + 1;
        return (1.0f - lerpfac) * LabTransferLUT[lbound] + lerpfac * LabTransferLUT[ubound];
    }

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

    inline unsigned int GetClosestLabColour(const float L, const float a, const float b)
    {
        int dists[16]; //Integer comparisons are faster
        const float uvb = uvbias;
        const float ib = ibias;
        const float* const Lptr = Labpalvec[0];
        const float* const aptr = Labpalvec[1];
        const float* const bptr = Labpalvec[2];
        const float* const sptr = Labpalvec[3];
        const float sat = sqrtf(a * a + b * b);
        const float huefac = 1.0f + 0.015f * sat;
        const float satfac = 1.0f + 0.045f * sat;
        const float sfacu = 1.0f / (satfac * satfac);
        const float hfacu = 1.0f / (huefac * huefac);
        for (int j = 0; j < 16; j++) //Organised like this to get the compiler to vectorise it
        {
            const float Ldif = (L - Lptr[j]) * uvb;
            const float adif = (a - aptr[j]) * ib;
            const float bdif = b - bptr[j];
            const float sdif = sat - sptr[j];
            const float sdifsq = sdif * sdif;
            const float hdif = adif * adif + bdif * bdif - sdifsq;
            dists[j] = (int)((Ldif * Ldif + sdifsq * sfacu + hdif * hfacu) * 100000000.0f);
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
    
    inline unsigned int VoidClusterDither16x16WithYIQ(unsigned int argbcolour, unsigned int x, unsigned int y)
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
        cy += ditherfactor * vcdithermatrix16[(x & 0x0F) + 16 * (y & 0x0F)];
        sat -= satditherfactor * vcdithermatrix16[((x - 7) & 0x0F) + 16 * ((y - 13) & 0x0F)];
        hue += hueditherfactor * vcdithermatrix16[((x + 4) & 0x0F) + 16 * ((y - 6) & 0x0F)];
        q = sat * cosf(hue);
        i = sat * sinf(hue);
        return GetClosestYIQColour(cy, i, q);
    }

    inline unsigned int VoidClusterDither16x16WithLab(const unsigned int argbcolour, const unsigned int x, const unsigned int y)
    {
        const int R = (argbcolour & 0x00FF0000) >> 16; //R
        const int G = (argbcolour & 0x0000FF00) >> 8; //G
        const int B = argbcolour & 0x000000FF; //B
        const float ciex = (SRGBtoXYZ[0] * SRGBtoLinearLUT[R] + SRGBtoXYZ[1] * SRGBtoLinearLUT[G] + SRGBtoXYZ[2] * SRGBtoLinearLUT[B]) / D65_X; //X
        const float ciey = (SRGBtoXYZ[3] * SRGBtoLinearLUT[R] + SRGBtoXYZ[4] * SRGBtoLinearLUT[G] + SRGBtoXYZ[5] * SRGBtoLinearLUT[B]) / D65_Y; //Y
        const float ciez = (SRGBtoXYZ[6] * SRGBtoLinearLUT[R] + SRGBtoXYZ[7] * SRGBtoLinearLUT[G] + SRGBtoXYZ[8] * SRGBtoLinearLUT[B]) / D65_Z; //Z
        float L = 1.16f * LabTransferFunctionLerp(ciey) - 0.16f; //L*
        float a = 5.0f * (LabTransferFunctionLerp(ciex) - LabTransferFunctionLerp(ciey)); //a*
        float b = 2.0f * (LabTransferFunctionLerp(ciey) - LabTransferFunctionLerp(ciez)); //b*
        float sat = sqrtf(a * a + b * b);
        float hue = atan2f(b, a);
        L += ditherfactor * vcdithermatrix16[(x & 0x0F) + 16 * (y & 0x0F)];
        sat -= satditherfactor * vcdithermatrix16[((x - 7) & 0x0F) + 16 * ((y - 13) & 0x0F)];
        hue += hueditherfactor * vcdithermatrix16[((x + 4) & 0x0F) + 16 * ((y - 6) & 0x0F)];
        a = sat * cosf(hue);
        b = sat * sinf(hue);
        return GetClosestLabColour(L, a, b);
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
    float Labpal[16][4];
    float Labpalvec[4][16]; //to aid vectorisation
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