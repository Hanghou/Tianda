//////////////////////////////////////////////////////////////////////////
////////////////	HM_UdmGenerator   ////////////////////
/////////////////////////////////////////////////////////////////////////
#pragma once
#include <vector>
using namespace std;

#ifdef  _HM_UDM_EXPORT
#define  HM_HashuAPI   extern "C" int _declspec (dllexport)
#define  HM_HashuAPI2   extern "C" float _declspec (dllexport)
#else
#define  HM_HashuAPI   extern "C" int _declspec (dllimport)
#define  HM_HashuAPI2   extern "C" float _declspec (dllexport)
#endif

#ifndef  _STRUCT_UDM_POS
#define _STRUCT_UDM_POS
//UDM打标位置使用的数据结构
typedef struct structUdmPos 
{
	float x;
	float y;
	float z;
	float a;
}structUdmPos;
#endif

#ifndef  _MARK_PARAMETER
#define _MARK_PARAMETER
typedef struct MarkParameter
{
	unsigned int MarkSpeed;//打标速度(mm/s)
	unsigned int JumpSpeed;//跳转速度(mm/s)
	unsigned int MarkDelay;//打标延时(us)
	unsigned int JumpDelay;//跳转延时(us)
	unsigned int PolygonDelay;//转弯延时(us)
	unsigned int MarkCount;//打标次数
	float LaserOnDelay;//开激光延时(单位us)
	float LaserOffDelay;//关激光延时(单位us)
	float FPKDelay;//首脉冲抑制延时(单位us)
	float FPKLength;//首脉冲抑制长度(单位us)
	float QDelay;//出光Q频率延时(单位us)
	float DutyCycle;//出光时占空比，(0~1)
	float Frequency;//出光时频率kHz
	float StandbyFrequency;//不出光Q频率(单位kHz);
	float StandbyDutyCycle;//不出光Q占空比(0~1);
	float LaserPower;//激光能量百分比(0~100)，50代表50%
	unsigned int AnalogMode;//1代表使用模拟量输出来控制激光器能量（0~10V）
	unsigned int Waveform;//SPI激光器波形号（0~63）
	unsigned int PulseWidthMode;//0,不开启MOPA脉宽使能模式， 1,开启MOPA激光器脉宽使能
	unsigned int PulseWidth;//MOPA激光器脉宽值 单位（ns）
}MarkParameter;
#endif

HM_HashuAPI	  UDM_NewFile();
HM_HashuAPI	  UDM_GetUDMBuffer(char** pUdmBuffer, int* nBytesCount);
HM_HashuAPI	  UDM_SaveToFile(char* strFilePath);
HM_HashuAPI   UDM_Main();
HM_HashuAPI   UDM_EndMain();
HM_HashuAPI	  UDM_RepeatStart(int repeatCount);
HM_HashuAPI	  UDM_RepeatEnd(int startAddress);
HM_HashuAPI   UDM_Jump(float x, float y, float z);
HM_HashuAPI   UDM_Wait(float msTime);//单位ms
HM_HashuAPI   UDM_SetGuidLaser(bool enable);//开启关闭红光
HM_HashuAPI	  UDM_SetInput(unsigned int uInIndex);//索引从0开始(0~13)，相应输入信号触发后才继续往下执行，否则一直等待输入信号。
HM_HashuAPI	  UDM_SetOutPutAll(unsigned int uData);//一键全部控制输出信号（此信息会写入UDM.BIN文件中去）"二进制1111代表四个输出全部拉高，二进制0011代表out0、out1拉高，out2/out3拉低"
HM_HashuAPI	  UDM_SetOutPutOn(unsigned int nOutIndex);//单独拉高输出信号（此信息会写入UDM.BIN文件中去）
HM_HashuAPI	  UDM_SetOutPutOff(unsigned int nOutIndex);//单独拉低输出信号（此信息会写入UDM.BIN文件中去）
HM_HashuAPI   UDM_SetOutputOn_GMC4(unsigned int nOutIndex);//单独拉高输出信号（此信息会写入UDM.BIN文件中去）,GMC4卡
HM_HashuAPI   UDM_SetOutputOff_GMC4(unsigned int nOutIndex);//单独拉低输出信号（此信息会写入UDM.BIN文件中去）,GMC4卡
HM_HashuAPI   UDM_SetAnalogValue(float VoutA,float VoutB);//设置两路(VOUTA/VOUTB)模拟量的值，0表示0V，0.5表示5V，1表示10V电压
HM_HashuAPI   UDM_SetOffset(float offsetX, float offsetY, float offsetZ);//所有点坐标平移
HM_HashuAPI   UDM_SetRotate(float angle, float centryX, float centryY);//旋转
HM_HashuAPI	  UDM_SetProtocol(int nProtocol,int nDimensional);//nProtocol:0表示SPI协议，1表示XY2-100协议，2表示SL2协议;nDimensional:0是2D打标，1是3D打标
HM_HashuAPI   UDM_FootTrigger(unsigned int nDelayTime,int nTriggerType);//启用脚踏触发打标,nDelayTime触发后延时多少ms开始打标，单位ms.nTriggerType,0上升沿触发，1电平触发

HM_HashuAPI   UDM_SetSkyWritingMode(int enable,int mode,float uniformLen,float accLen,float angleLimit);//enable=0关闭SkyWriting， 1启用SkyWriting功能
HM_HashuAPI   UDM_SetSkyWritingModeRTC5(int mode,int nPrev,int nPost,float angleLimit);//mode=0关闭，大于0开启
HM_HashuAPI   UDM_SetJumpExtendLen(float jumpExtendLen);//设置跳转延长
HM_HashuAPI   UDM_SetEndPower(float power);//设置打标完成后能量值，即维持能量（0~100）
HM_HashuAPI   UDM_SetLayersPara(MarkParameter *layersParameter,int count);//layersParameter层数组，count层数组个数
HM_HashuAPI   UDM_AddPolyline2D(structUdmPos *nPos, int nCount,int layerIndex);//nPos图形点数组，nCount点个数，layerIndex所在图层索引
HM_HashuAPI   UDM_AddPolyline3D(structUdmPos *nPos, int nCount,int layerIndex);//nPos图形点数组，nCount点个数，layerIndex所在图层索引
HM_HashuAPI	  UDM_AddPoint2D(structUdmPos pos, float time,int layerIndex);//pos打标点，time打此点时间ms，layerIndex所在图层索引
HM_HashuAPI	  UDM_AddPoint2DPower(structUdmPos pos, float time,float power, int layerIndex);//pos打标点，time打此点时间ms,最大时长2147483ms，layerIndex所在图层索引
HM_HashuAPI	  UDM_AddArc2D(structUdmPos stCenter, float radius, float startAngle, float sweepAngle,int layerIndex);//sweepAngle正数逆时针，负数顺时针
HM_HashuAPI	  UDM_SetCloseLoop(bool enable, int galvoType, int followErrorMax,int followErrorCount);//设置闭环控制
HM_HashuAPI	  UDM_Set3dCorrectionPara(float baseFocal,double *paraK, int nCount);//设置3D校正表参数
HM_HashuAPI2  UDM_GetZvalue(float x,float y, float height);
HM_HashuAPI   UDM_AddBreakAndCorPolyline3D(structUdmPos *nPos, int nCount,float p2pGap, int layerIndex);//nPos图形点数组，nCount点个数，layerIndex所在图层索引
HM_HashuAPI   UDM_SetFirstPulse(bool enable,float firstDutyCycle,float incrementDutyCycle);
HM_HashuAPI   UDM_SetVariablePolygonDelay(int enable,int laserOffEnable,int EdgeLevel);//拐弯延时随角度变化，enable=1启用，laserOffEnable =1，拐弯延时大于EdgeLevel(us)时关光
HM_HashuAPI   UDM_SetVariableJumpDelay(int jumpDelayMinTime,float jumpMinLen);//可变跳转延时
HM_HashuAPI   UDM_SetSCANAhead(int enable,int mode);//启用SCANAhead
//HM_HashuAPI   UDM_SetSCANAheadAcc(int acceleration);//启用acceleration加速度
//HM_HashuAPI   UDM_SetSCANAheadCornerSpeed(int cornerSpeed);//cornerSpeed拐角速度
HM_HashuAPI   UDM_SetLaserOnLevel(int level);//0高电平出光，1低电平出光

HM_HashuAPI	  UDM_SetUdmCount(int count);
HM_HashuAPI	  UDM_NewFile_N(int index);
HM_HashuAPI	  UDM_GetUDMBuffer_N(int index,char** pUdmBuffer, int* nBytesCount);
HM_HashuAPI	  UDM_SaveToFile_N(int index,char* strFilePath);
HM_HashuAPI   UDM_Main_N(int index);
HM_HashuAPI   UDM_EndMain_N(int index);
HM_HashuAPI   UDM_RepeatStart_N(int index,int repeatCount);
HM_HashuAPI	  UDM_RepeatEnd_N(int index,int startAddress);
HM_HashuAPI   UDM_Jump_N(int index,float x, float y, float z);
HM_HashuAPI   UDM_Wait_N(int index, float time);
HM_HashuAPI   UDM_SetGuidLaser_N(int index,bool enable);//开启关闭红光
HM_HashuAPI	  UDM_SetInput_N(int index,unsigned int uInIndex);
HM_HashuAPI	  UDM_SetOutPutAll_N(int index,unsigned int uData);
HM_HashuAPI	  UDM_SetOutPutOn_N(int index,unsigned int nOutIndex);
HM_HashuAPI	  UDM_SetOutPutOff_N(int index,unsigned int nOutIndex);
HM_HashuAPI	  UDM_SetOutputOn_GMC4_N(int index,unsigned int nOutIndex);
HM_HashuAPI	  UDM_SetOutputOff_GMC4_N(int index,unsigned int nOutIndex);
HM_HashuAPI   UDM_SetAnalogValue_N(int index,float VoutA,float VoutB);
HM_HashuAPI   UDM_SetOffset_N(int index,float offsetX, float offsetY, float offsetZ);
HM_HashuAPI   UDM_SetRotate_N(int index,float angle, float centryX, float centryY);//旋转
HM_HashuAPI	  UDM_SetProtocol_N(int index, int nProtocol,int nDimensional);
HM_HashuAPI   UDM_SetSkyWritingMode_N(int index,int enable,int mode,float uniformLen,float accLen,float angleLimit);//enable=0关闭SkyWriting， 1启用SkyWriting功能
HM_HashuAPI   UDM_SetSkyWritingModeRTC5_N(int index,int mode,int nPrev,int nPost,float angleLimit);
HM_HashuAPI   UDM_SetEndPower_N(int index,float power);//设置打标完成后能量值，即维持能量（0~100）
HM_HashuAPI   UDM_SetLayersPara_N(int index,MarkParameter *layersParameter,int count);
HM_HashuAPI   UDM_AddPolyline2D_N(int index,structUdmPos *nPos, int nCount,int layerIndex);
HM_HashuAPI   UDM_AddPolyline3D_N(int index,structUdmPos *nPos, int nCount,int layerIndex);
HM_HashuAPI	  UDM_AddPoint2D_N(int index,structUdmPos pos, float time,int layerIndex);
HM_HashuAPI	  UDM_AddPoint2DPower_N(int index,structUdmPos pos, float time,float power, int layerIndex);
HM_HashuAPI	  UDM_AddArc2D_N(int index,structUdmPos stCenter, float radius, float startAngle, float sweepAngle,int layerIndex);
HM_HashuAPI	  UDM_SetCloseLoop_N(int index,bool enable, int galvoType,int followErrorMax,int followErrorCount);
HM_HashuAPI	  UDM_Set3dCorrectionPara_N(int index,float baseFocal,double *paraK, int nCount);//设置3D校正表参数
HM_HashuAPI2  UDM_GetZvalue_N(int index,float x,float y, float height);
HM_HashuAPI   UDM_AddBreakAndCorPolyline3D_N(int index,structUdmPos *nPos, int nCount,float pGap, int layerIndex);
HM_HashuAPI   UDM_SetFirstPulse_N(int index,bool enable,float firstDutyCycle,float incrementDutyCycle);
HM_HashuAPI   UDM_SetVariablePolygonDelay_N(int index,int enable,int laserOffEnable,int EdgeLevel);
HM_HashuAPI   UDM_SetVariableJumpDelay_N(int index,int jumpDelayMinTime,float jumpMinLen);//可变跳转延时
HM_HashuAPI   UDM_SetSCANAhead_N(int index,int enable,int mode);//启用SCANAhead
//HM_HashuAPI   UDM_SetSCANAheadAcc_N(int index,int acceleration);//启用acceleration加速度
//HM_HashuAPI   UDM_SetSCANAheadCornerSpeed_N(int index,int cornerSpeed);//cornerSpeed拐角速度
HM_HashuAPI   UDM_SetLaserOnLevel_N(int index,int level);//0高电平出光，1低电平出光