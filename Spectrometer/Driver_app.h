
#ifndef __DRIVER_FUNC_H_
#define __DRIVER_FUNC_H_

#pragma comment(lib,"Driver.lib")                  //Call the library

///////////header file//////////////////////////////
//////////////////////////////////////
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long  int u32;

//////////////////////////////////////
#define EXPORTED extern "C" __declspec(dllimport)       //The import application
#define STDCALL  __stdcall

//////////////////////////////////////////////////
///////////////Initializes the return status flag////////////////////////////
typedef  int ErrorFlag;        
#define  INIT_SUCCESS       0
#define  THREAD_ERRORFLAG  0X01
#define  GAIT_ERRORFLAG    0X02
#define  RSCFS_ERRORFLAG   0X04
#define  RLCFS_ERRORFLAG   0X08
#define  RWCFS_ERRORFLAG   0X10
#define  RDCFS_ERRORFLAG   0X20

//////////////////When the spectral data is ready to be collected, return marks are completed////////////////
#define  SPECTRUMDATA_READY     1
#define  SPECTRUMDATA_VALID     1
#define  SPECTRUMDATA_INVALID   0

////////////////////////////////////////////
///////////////////////////////////////////

/////////////////Message number///////////////////////////

#define ON_SPECTRUM_MSG_BASE		    (WM_USER + 700	) 	//!< the basis point of message number (the query mark bit can be used to determine whether the collection is completed or not, or the message can be received) 
#define ON_SPECTRUM_RECEIVEDATA			(ON_SPECTRUM_MSG_BASE + 0)	//

//////////////////////////º¯Êý//////////////////////////////////////////////////
EXPORTED bool STDCALL openSpectraMeter();   //Turn on the device, turn on the usb bus
EXPORTED bool STDCALL closeSpectraMeter();   //Turn off usb bus

EXPORTED ErrorFlag STDCALL initialize();                              //Initialization equipment

EXPORTED bool STDCALL setIntegrationTime(int timeMicros);  //Set integral time

EXPORTED bool STDCALL SetWnd(HWND hWnd);       //Associated message window
EXPORTED HWND STDCALL GetWnd();

EXPORTED bool STDCALL getSpectrum(int integrationtimeMicros);     //Start collecting spectra
//EXPORTED bool STDCALL getDarkSpectrum(int integrationtimeMicros);     //Start collecting spectra
EXPORTED Spectrumsp STDCALL ReadSpectrum();     //Reading spectral data

EXPORTED int STDCALL getSpectrumDataReadyFlag();     //When the acquisition of returned spectral data is completed, the signal bit is ready


EXPORTED int STDCALL getActualIntegrationTime();                 //Gets the current integral time, count in milliseconds
EXPORTED int STDCALL getIntegrationTimeBase();                   //Gets the base of integration time, milliseconds
EXPORTED int STDCALL getIntegrationTimeIncrement();              //Gets the incremental milliseconds of integration time
EXPORTED int STDCALL getIntegrationTimeMaximum();                //Get the maximum integral time
EXPORTED int STDCALL getIntegrationTimeMinimum();                //Get the minimum integration time


EXPORTED bool STDCALL getManufacturers(u8 dataout[]);
EXPORTED bool STDCALL getProductDate(u8 dataout[]);
EXPORTED bool STDCALL getProductPN(u8 dataout[]);
EXPORTED bool STDCALL getVersion(u8 dataout[]);
EXPORTED bool STDCALL getProductSN(u8 dataout[]);
EXPORTED int STDCALL getSlitSize();
EXPORTED bool STDCALL getCircuitboardTemperature(u8 dataout[]);


//***********************************None************************************************************//
EXPORTED void STDCALL deductDark(int original[], DarkFactorArraysp DarkFactorStru, int original_dark[],int integrationtime);
EXPORTED void STDCALL wavelenthCalibration(float lamda[], FactorArraysp WaveFactorStru);
EXPORTED void STDCALL shapeCalibration(float denoised_dark[], float power[],FactorArraysp ShapeFactorArrayStru);
EXPORTED void STDCALL linearCalibration(float power[], float power_linear[],FactorArraysp LinearFactorStru);
EXPORTED void STDCALL setlinearpowerarray(FactorArraysp LinearFactorStru);
EXPORTED void STDCALL linearCalibration_search(float power[], float power_linear[]);

//********************************************************************************************//

EXPORTED void STDCALL gaussFilter(int original_dark[],float denoised_dark[], int sigma_selected);//filter  
//EXPORTED void STDCALL setdeductdarkstatus(bool deductdarkstate);
//EXPORTED void STDCALL setnonlinearcorrectstatus(bool nonlinearcorrectstate);
EXPORTED float* STDCALL getWavelength();    //Acquired spectral wavelength

EXPORTED float* STDCALL getdata1(int original_data[], int sigma_selected, bool deductdark, bool linear, bool wavecorretion);

EXPORTED void STDCALL findSpectraMeters(spectrum_device_info& device_info);


EXPORTED bool STDCALL switchSpectraMeters(const char* serial);


EXPORTED bool STDCALL findSpectraMeterBySerial(const char* serail);

EXPORTED bool STDCALL setAverage(int num);

EXPORTED bool STDCALL startScan(int integrationTime);

EXPORTED Spectrumsp STDCALL getSpectrumdata();

EXPORTED int STDCALL getCcdSize();

#endif 
