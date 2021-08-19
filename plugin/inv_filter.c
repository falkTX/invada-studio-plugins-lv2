/* 

    This LV2 plugin provides mono and stereo filters

    (c) Fraser Stuart 2009

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdlib.h>
#include <math.h>
#include <lv2.h>
#include "library/common.h"
#include "inv_filter.h"


static LV2_Descriptor *IFilterMonoLPFDescriptor = NULL;
static LV2_Descriptor *IFilterStereoLPFDescriptor = NULL;
static LV2_Descriptor *IFilterMonoHPFDescriptor = NULL;
static LV2_Descriptor *IFilterStereoHPFDescriptor = NULL;


typedef struct {
 
	/* Ports */
	float *ControlBypass;
	float *ControlFreq;         
	float *ControlGain;
	float *ControlNoClip;  
	float *AudioInputBufferL;
	float *AudioOutputBufferL;
	float *AudioInputBufferR; 
	float *AudioOutputBufferR;
	float *MeterInputL;
	float *MeterOutputL;
	float *MeterInputR; 
	float *MeterOutputR;
	float *MeterDrive;

	double SampleRate;
	struct Envelope EnvAD[4];

	/* stuff we need to remember */
	float LastBypass;
	float LastFreq;         
	float LastGain;
	float LastNoClip;

	float ConvertedBypass;
	float ConvertedFreq;         
	float ConvertedGain;
	float ConvertedNoClip;

	/* stuff we need to remember between calls */
	float AudioLLast; 
	float AudioRLast;
	float EnvInLLast; 
	float EnvOutLLast; 
	float EnvInRLast; 
	float EnvOutRLast;
	float EnvDriveLast;  

} IFilter;


static LV2_Handle instantiateIFilter(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IFilter *plugin = (IFilter *)malloc(sizeof(IFilter));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;

	return (LV2_Handle)plugin;
}


static void connectPortIFilter(LV2_Handle instance, uint32_t port, void *data)
{
	IFilter *plugin = (IFilter *)instance;
	switch (port) {
		case IFILTER_BYPASS:
			plugin->ControlBypass = data;
			break;
		case IFILTER_FREQ:
			plugin->ControlFreq = data;
			break;
		case IFILTER_GAIN:
			plugin->ControlGain  = data;
			break;
		case IFILTER_NOCLIP:
			plugin->ControlNoClip = data;
			break;
		case IFILTER_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case IFILTER_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case IFILTER_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case IFILTER_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case IFILTER_METER_INL:
			plugin->MeterInputL = data;
			break;
		case IFILTER_METER_INR:
			plugin->MeterInputR = data;
			break;
		case IFILTER_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case IFILTER_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
		case IFILTER_METER_DRIVE:
			plugin->MeterDrive = data;
			break;
	}
}


static void activateIFilter(LV2_Handle instance) 
{

	IFilter *plugin = (IFilter *)instance;

	plugin->AudioLLast = 0;
	plugin->AudioRLast = 0;
	plugin->EnvInLLast = 0; 
	plugin->EnvOutLLast = 0; 
	plugin->EnvInRLast = 0; 
	plugin->EnvOutRLast = 0; 
	plugin->EnvDriveLast = 0; 

	/* defaults */
	plugin->LastBypass = 0;
	plugin->LastFreq = 1000;      
	plugin->LastGain = 0;
	plugin->LastNoClip = 0;

	plugin->ConvertedBypass = convertParam(IFILTER_BYPASS, plugin->LastBypass,  plugin->SampleRate);
	plugin->ConvertedFreq   = convertParam(IFILTER_FREQ,   plugin->LastFreq,    plugin->SampleRate);
	plugin->ConvertedGain   = convertParam(IFILTER_GAIN,   plugin->LastGain,    plugin->SampleRate);
	plugin->ConvertedNoClip = convertParam(IFILTER_NOCLIP, plugin->LastNoClip,  plugin->SampleRate);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void runMonoLPFIFilter(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float InL,OutL,EnvInL,EnvOutL,EnvDrive;
	float fBypass,fSamples,fGain,fNoClip;
	double fFreqDelta,fGainDelta;
	int   HasDelta;
	float fAudioL,fAudioLSum;
	float drive=0;
	unsigned long lSampleIndex;

	IFilter *plugin = (IFilter *)instance;
	pParamFunc = &convertParam;

	checkParamChange(IFILTER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IFILTER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fFreqDelta = getParamChange(IFILTER_FREQ,   plugin->ControlFreq,   &(plugin->LastFreq),   &(plugin->ConvertedFreq),   plugin->SampleRate, pParamFunc);
	fGainDelta = getParamChange(IFILTER_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);


	fBypass  = plugin->ConvertedBypass;
	fNoClip  = plugin->ConvertedNoClip;

	if(fFreqDelta == 0 && fGainDelta==0 ) {
		HasDelta=0;
		fSamples = plugin->ConvertedFreq;
		fGain    = plugin->ConvertedGain;
	} else {
		HasDelta=1;
		fSamples = plugin->ConvertedFreq - fFreqDelta;
		fGain    = plugin->ConvertedGain - fGainDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fFreqDelta  = fFreqDelta/(float)SampleCount;
			fGainDelta  = fGainDelta/(float)SampleCount;
		}
	}

	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioOutputL = plugin->AudioOutputBufferL;

	fAudioLSum = plugin->AudioLLast;
	EnvInL     = plugin->EnvInLLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvDrive   = plugin->EnvDriveLast;
 
	if(fBypass==0) {  
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
		{
			InL=*(pfAudioInputL++);
			fAudioLSum = ((fSamples-1) * fAudioLSum + InL) / fSamples;  
			fAudioL = fAudioLSum*fGain; 

			OutL=fNoClip > 0 ? InoClip(fAudioL, &drive) : fAudioL;
			*(pfAudioOutputL++)= OutL; 

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL,  EnvInL);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fSamples  += fFreqDelta;
				fGain     += fGainDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
		}
		//evelope on in and out for meters
		fAudioLSum =0;
		EnvInL     =0;
		EnvOutL    =0;
		EnvDrive   =0;
	}

	// remember for next time round  
	plugin->AudioLLast = (fabs(fAudioLSum)<1.0e-10)  ? 0.f : fAudioLSum;  // and store values for next loop
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterDrive)=EnvDrive;
}


static void runStereoLPFIFilter(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float InL,OutL,EnvInL,EnvOutL;
	float InR,OutR,EnvInR,EnvOutR;
	float fBypass,fSamples,fGain,fNoClip;
	double fFreqDelta,fGainDelta;
	int   HasDelta;
	float fAudioL,fAudioR,fAudioLSum,fAudioRSum;
	float drive,EnvDrive;
	float driveL=0;
	float driveR=0;
	unsigned long lSampleIndex;

	IFilter *plugin = (IFilter *)instance;
	pParamFunc = &convertParam;

	checkParamChange(IFILTER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IFILTER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fFreqDelta = getParamChange(IFILTER_FREQ,   plugin->ControlFreq,   &(plugin->LastFreq),   &(plugin->ConvertedFreq),   plugin->SampleRate, pParamFunc);
	fGainDelta = getParamChange(IFILTER_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);

	fBypass  = plugin->ConvertedBypass;
	fNoClip  = plugin->ConvertedNoClip;

	if(fFreqDelta == 0 && fGainDelta==0 ) {
		HasDelta=0;
		fSamples = plugin->ConvertedFreq;
		fGain    = plugin->ConvertedGain;
	} else {
		HasDelta=1;
		fSamples = plugin->ConvertedFreq - fFreqDelta;
		fGain    = plugin->ConvertedGain - fGainDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fFreqDelta  = fFreqDelta/(float)SampleCount;
			fGainDelta  = fGainDelta/(float)SampleCount;
		}
	}

	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioInputR = plugin->AudioInputBufferR;
	pfAudioOutputL = plugin->AudioOutputBufferL;
	pfAudioOutputR = plugin->AudioOutputBufferR;

	fAudioLSum = plugin->AudioLLast;
	fAudioRSum = plugin->AudioRLast;
	EnvInL     = plugin->EnvInLLast;
	EnvInR     = plugin->EnvInRLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvOutR    = plugin->EnvOutRLast;
	EnvDrive   = plugin->EnvDriveLast;

	if(fBypass==0) {  
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
		{
			InL=*(pfAudioInputL++);
			InR=*(pfAudioInputR++);
			fAudioLSum = ((fSamples-1) * fAudioLSum + InL) / fSamples;  
			fAudioRSum = ((fSamples-1) * fAudioRSum + InR) / fSamples;
			fAudioL = fAudioLSum*fGain; 
			fAudioR = fAudioRSum*fGain; 
			  
			OutL=fNoClip > 0 ? InoClip(fAudioL,&driveL) : fAudioL;  
			OutR=fNoClip > 0 ? InoClip(fAudioR,&driveR) : fAudioR;
			*(pfAudioOutputL++)=OutL;
			*(pfAudioOutputR++)=OutR;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL,  EnvInL);
			EnvInR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InR,  EnvInR);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR, EnvOutR);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fSamples  += fFreqDelta;
				fGain     += fGainDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
			*(pfAudioOutputR++) = *(pfAudioInputR++);
		}
		//evelope on in and out for meters
		fAudioLSum =0;
		fAudioRSum =0;
		EnvInL     =0;
		EnvInR     =0;
		EnvOutL    =0;
		EnvOutR    =0;
		EnvDrive   =0;
	}
  
	// store values for next loop
	plugin->AudioLLast = (fabs(fAudioLSum)<1.0e-10)  ? 0.f : fAudioLSum;  
	plugin->AudioRLast = (fabs(fAudioRSum)<1.0e-10)  ? 0.f : fAudioRSum; 
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvInRLast = (fabs(EnvInR)<1.0e-10)  ? 0.f : EnvInR; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterInputR) =(EnvInR  > 0.001) ? 20*log10(EnvInR)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
	*(plugin->MeterDrive)=EnvDrive;
}



static void runMonoHPFIFilter(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float InL,OutL,EnvInL,EnvOutL,EnvDrive;
	float fBypass,fSamples,fGain,fNoClip;
	double fFreqDelta,fGainDelta;
	int   HasDelta;
	float fAudioL,fAudioLSum;
	float drive=0;
	unsigned long lSampleIndex;

	IFilter *plugin = (IFilter *)instance;
	pParamFunc = &convertParam;

	checkParamChange(IFILTER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IFILTER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fFreqDelta = getParamChange(IFILTER_FREQ,   plugin->ControlFreq,   &(plugin->LastFreq),   &(plugin->ConvertedFreq),   plugin->SampleRate, pParamFunc);
	fGainDelta = getParamChange(IFILTER_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);

	fBypass  = plugin->ConvertedBypass;
	fNoClip  = plugin->ConvertedNoClip;

	if(fFreqDelta == 0 && fGainDelta==0 ) {
		HasDelta=0;
		fSamples = plugin->ConvertedFreq;
		fGain    = plugin->ConvertedGain;
	} else {
		HasDelta=1;
		fSamples = plugin->ConvertedFreq - fFreqDelta;
		fGain    = plugin->ConvertedGain - fGainDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fFreqDelta  = fFreqDelta/(float)SampleCount;
			fGainDelta  = fGainDelta/(float)SampleCount;
		}
	}

	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioOutputL = plugin->AudioOutputBufferL;

	fAudioLSum = plugin->AudioLLast;
	EnvInL     = plugin->EnvInLLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvDrive   = plugin->EnvDriveLast;

	if(fBypass==0) {  
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
		{

			InL=*(pfAudioInputL++);
			fAudioLSum = ((fSamples-1) * fAudioLSum + InL) / fSamples;  
			fAudioL = (InL - fAudioLSum)*fGain;

			OutL=fNoClip > 0 ? InoClip(fAudioL,&drive) : fAudioL;  
			*(pfAudioOutputL++)=OutL;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL,  EnvInL);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fSamples  += fFreqDelta;
				fGain     += fGainDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
		}
		//evelope on in and out for meters
		fAudioLSum =0;
		EnvInL     =0;
		EnvOutL    =0;
		EnvDrive   =0;
	}

	// store values for next loop
	plugin->AudioLLast = (fabs(fAudioLSum)<1.0e-10)  ? 0.f : fAudioLSum;  
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterDrive)=EnvDrive;
}


static void runStereoHPFIFilter(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float InL,OutL,EnvInL,EnvOutL;
	float InR,OutR,EnvInR,EnvOutR;
	float fBypass,fSamples,fGain,fNoClip;
	double fFreqDelta,fGainDelta;
	int   HasDelta;
	float fAudioL,fAudioR,fAudioLSum,fAudioRSum;
	float drive,EnvDrive;
	float driveL=0;
	float driveR=0;
	unsigned long lSampleIndex;

	IFilter *plugin = (IFilter *)instance;
	pParamFunc = &convertParam;

	checkParamChange(IFILTER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IFILTER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fFreqDelta = getParamChange(IFILTER_FREQ,   plugin->ControlFreq,   &(plugin->LastFreq),   &(plugin->ConvertedFreq),   plugin->SampleRate, pParamFunc);
	fGainDelta = getParamChange(IFILTER_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);

	fBypass  = plugin->ConvertedBypass;
	fNoClip  = plugin->ConvertedNoClip;

	if(fFreqDelta == 0 && fGainDelta==0 ) {
		HasDelta=0;
		fSamples = plugin->ConvertedFreq;
		fGain    = plugin->ConvertedGain;
	} else {
		HasDelta=1;
		fSamples = plugin->ConvertedFreq - fFreqDelta;
		fGain    = plugin->ConvertedGain - fGainDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fFreqDelta  = fFreqDelta/(float)SampleCount;
			fGainDelta  = fGainDelta/(float)SampleCount;
		}
	}

	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioInputR = plugin->AudioInputBufferR;
	pfAudioOutputL = plugin->AudioOutputBufferL;
	pfAudioOutputR = plugin->AudioOutputBufferR;

	fAudioLSum = plugin->AudioLLast;
	fAudioRSum = plugin->AudioRLast;
	EnvInL     = plugin->EnvInLLast;
	EnvInR     = plugin->EnvInRLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvOutR    = plugin->EnvOutRLast;
	EnvDrive   = plugin->EnvDriveLast;
  
	if(fBypass==0) {  
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
		{
			InL = *(pfAudioInputL++);
			InR = *(pfAudioInputR++);
			fAudioLSum = ((fSamples-1) * fAudioLSum + InL) / fSamples;  
			fAudioRSum = ((fSamples-1) * fAudioRSum + InR) / fSamples;
			fAudioL = (InL - fAudioLSum)*fGain;
			fAudioR = (InR - fAudioRSum)*fGain;
		
			OutL=fNoClip > 0 ? InoClip(fAudioL,&driveL) : fAudioL;  
			OutR=fNoClip > 0 ? InoClip(fAudioR,&driveR) : fAudioR;
			*(pfAudioOutputL++)=OutL;
			*(pfAudioOutputR++)=OutR;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL,  EnvInL);
			EnvInR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InR,  EnvInR);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR, EnvOutR);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fSamples  += fFreqDelta;
				fGain     += fGainDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
			*(pfAudioOutputR++) = *(pfAudioInputR++);
		}
		//evelope on in and out for meters
		fAudioLSum =0;
		fAudioRSum =0;
		EnvInL     =0;
		EnvInR     =0;
		EnvOutL    =0;
		EnvOutR    =0;
		EnvDrive   =0;
	}

	// store values for next loop
	plugin->AudioLLast = (fabs(fAudioLSum)<1.0e-10)  ? 0.f : fAudioLSum;  
	plugin->AudioRLast = (fabs(fAudioRSum)<1.0e-10)  ? 0.f : fAudioRSum; 
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvInRLast = (fabs(EnvInR)<1.0e-10)  ? 0.f : EnvInR; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterInputR) =(EnvInR  > 0.001) ? 20*log10(EnvInR)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
	*(plugin->MeterDrive)=EnvDrive;
}


static void cleanupIFilter(LV2_Handle instance)
{
	free(instance);
}


static void init()
{
	IFilterMonoLPFDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IFilterMonoLPFDescriptor->URI 			= IFILTER_MONO_LPF_URI;
	IFilterMonoLPFDescriptor->activate 		= activateIFilter;
	IFilterMonoLPFDescriptor->cleanup 		= cleanupIFilter;
	IFilterMonoLPFDescriptor->connect_port 		= connectPortIFilter;
	IFilterMonoLPFDescriptor->deactivate 		= NULL;
	IFilterMonoLPFDescriptor->instantiate 		= instantiateIFilter;
	IFilterMonoLPFDescriptor->run 			= runMonoLPFIFilter;
	IFilterMonoLPFDescriptor->extension_data 	= NULL;

	IFilterStereoLPFDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IFilterStereoLPFDescriptor->URI 		= IFILTER_STEREO_LPF_URI;
	IFilterStereoLPFDescriptor->activate 		= activateIFilter;
	IFilterStereoLPFDescriptor->cleanup 		= cleanupIFilter;
	IFilterStereoLPFDescriptor->connect_port 	= connectPortIFilter;
	IFilterStereoLPFDescriptor->deactivate 		= NULL;
	IFilterStereoLPFDescriptor->instantiate 	= instantiateIFilter;
	IFilterStereoLPFDescriptor->run 		= runStereoLPFIFilter;
	IFilterStereoLPFDescriptor->extension_data	= NULL;

	IFilterMonoHPFDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IFilterMonoHPFDescriptor->URI 			= IFILTER_MONO_HPF_URI;
	IFilterMonoHPFDescriptor->activate 		= activateIFilter;
	IFilterMonoHPFDescriptor->cleanup 		= cleanupIFilter;
	IFilterMonoHPFDescriptor->connect_port 		= connectPortIFilter;
	IFilterMonoHPFDescriptor->deactivate 		= NULL;
	IFilterMonoHPFDescriptor->instantiate 		= instantiateIFilter;
	IFilterMonoHPFDescriptor->run 			= runMonoHPFIFilter;
	IFilterMonoHPFDescriptor->extension_data 	= NULL;

	IFilterStereoHPFDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IFilterStereoHPFDescriptor->URI 		= IFILTER_STEREO_HPF_URI;
	IFilterStereoHPFDescriptor->activate 		= activateIFilter;
	IFilterStereoHPFDescriptor->cleanup 		= cleanupIFilter;
	IFilterStereoHPFDescriptor->connect_port 	= connectPortIFilter;
	IFilterStereoHPFDescriptor->deactivate 		= NULL;
	IFilterStereoHPFDescriptor->instantiate 	= instantiateIFilter;
	IFilterStereoHPFDescriptor->run 		= runStereoHPFIFilter;
	IFilterStereoHPFDescriptor->extension_data	= NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IFilterMonoLPFDescriptor) init();

	switch (index) {
		case 0:
			return IFilterMonoLPFDescriptor;
		case 1:
			return IFilterStereoLPFDescriptor;
		case 2:
			return IFilterMonoHPFDescriptor;
		case 3:
			return IFilterStereoHPFDescriptor;
	default:
		return NULL;
	}
}

/*****************************************************************************/


float convertParam(unsigned long param, float value, double sr) {

	float result;

	switch(param)
	{
		case IFILTER_FREQ:
			if (value <  20)
				result = sr/(40.0);
			else if (value <= 20000.0)
				result = sr/(2*value);
			else
				result=sr/(40000.0);
			break;
		case IFILTER_GAIN:
			if(value<0)
				result= 1;
			else if (value < 12)
				result = pow(10,value/20);
			else
				result= pow(10,0.6);
			break;
		case IFILTER_BYPASS:
		case IFILTER_NOCLIP:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		default:
			result=0;
			break;
	}
	return result;
}

