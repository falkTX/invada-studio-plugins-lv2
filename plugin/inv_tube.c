/* 

    This LV2 plugin provides tube distortion

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
#include <string.h>
#include <math.h>
#include <lv2.h>
#include "library/common.h"
#include "inv_tube.h"


static LV2_Descriptor *ITubeMonoDescriptor = NULL;
static LV2_Descriptor *ITubeStereoDescriptor = NULL;


typedef struct {
	float *ControlBypass; 
	float *ControlDrive;         
	float *ControlDcoffset;
	float *ControlPhase;  
	float *ControlMix; 
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

	/* params get remembered */
	float LastBypass; 
	float LastDrive;         
	float LastDcoffset;
	float LastPhase;         
	float LastMix;

	float ConvertedBypass; 
	float ConvertedDrive;  
	float ConvertedDcoffset;
	float ConvertedPhase;  
	float ConvertedMix;

	/* stuff we need to remember between calls */
	float EnvInLLast; 
	float EnvOutLLast; 
	float EnvInRLast; 
	float EnvOutRLast; 
	float EnvDriveLast;
 
} ITube;


static LV2_Handle 
instantiateITube(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	ITube *plugin = (ITube *)malloc(sizeof(ITube));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;

	return (LV2_Handle)plugin;
}


static void 
connectPortITube(LV2_Handle instance, uint32_t port, void *data)
{
	ITube *plugin = (ITube *)instance;

	switch (port) {
		case ITUBE_BYPASS:
			plugin->ControlBypass = data;
			break;
		case ITUBE_DRIVE:
			plugin->ControlDrive = data;
			break;
		case ITUBE_DCOFFSET:
			plugin->ControlDcoffset  = data;
			break;
		case ITUBE_PHASE:
			plugin->ControlPhase = data;
			break;
		case ITUBE_MIX:
			plugin->ControlMix = data;
			break;
		case ITUBE_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case ITUBE_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case ITUBE_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case ITUBE_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case ITUBE_METER_INL:
			plugin->MeterInputL = data;
			break;
		case ITUBE_METER_INR:
			plugin->MeterInputR = data;
			break;
		case ITUBE_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case ITUBE_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
		case ITUBE_METER_DRIVE:
			plugin->MeterDrive = data;
			break;
	}
}



static void 
activateITube(LV2_Handle instance) 
{

	ITube *plugin = (ITube *)instance;

	/* set some defaults */
	plugin->LastBypass=0;  
	plugin->LastDrive=0;         
	plugin->LastDcoffset=0;
	plugin->LastPhase=0;         
	plugin->LastMix=75;

	plugin->EnvInLLast = 0; 
	plugin->EnvOutLLast = 0; 
	plugin->EnvInRLast = 0; 
	plugin->EnvOutRLast = 0; 
	plugin->EnvDriveLast = 0; 

	plugin->ConvertedBypass   = convertParam(ITUBE_BYPASS,   plugin->LastBypass,    plugin->SampleRate);
	plugin->ConvertedDrive    = convertParam(ITUBE_DRIVE,    plugin->LastDrive,     plugin->SampleRate);
	plugin->ConvertedDcoffset = convertParam(ITUBE_DCOFFSET, plugin->LastDcoffset,  plugin->SampleRate);
	plugin->ConvertedPhase    = convertParam(ITUBE_PHASE,    plugin->LastPhase,     plugin->SampleRate);
	plugin->ConvertedMix      = convertParam(ITUBE_MIX,      plugin->LastMix,       plugin->SampleRate);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void 
runMonoITube(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float OutL,EnvInL,EnvOutL;
	float TubeOut,drive,EnvDrive;
	float fBypass,fAudioL, fDrive, fDCOffset, DCOffsetADJ, fPhase, fMix;
	double fDriveDelta,fDCDelta,fMixDelta;
	int   HasDelta,HasTubeDelta;
	uint32_t lSampleIndex;
			   
	ITube *plugin = (ITube *)instance;
	pParamFunc = &convertParam;

	/* check for any params changes */
	checkParamChange(ITUBE_BYPASS,   plugin->ControlBypass,   &(plugin->LastBypass),   &(plugin->ConvertedBypass),   plugin->SampleRate, pParamFunc);
	checkParamChange(ITUBE_PHASE,    plugin->ControlPhase,    &(plugin->LastPhase),    &(plugin->ConvertedPhase),    plugin->SampleRate, pParamFunc);
	fDriveDelta = getParamChange(ITUBE_DRIVE,    plugin->ControlDrive,    &(plugin->LastDrive),    &(plugin->ConvertedDrive),    plugin->SampleRate, pParamFunc);
	fDCDelta    = getParamChange(ITUBE_DCOFFSET, plugin->ControlDcoffset, &(plugin->LastDcoffset), &(plugin->ConvertedDcoffset), plugin->SampleRate, pParamFunc);
	fMixDelta   = getParamChange(ITUBE_MIX,      plugin->ControlMix,      &(plugin->LastMix),      &(plugin->ConvertedMix),      plugin->SampleRate, pParamFunc);

	fBypass   = plugin->ConvertedBypass;
	fPhase    = plugin->ConvertedPhase;

	if(fDriveDelta == 0 && fDCDelta==0 && fMixDelta ==0) {
		HasDelta=0;
		HasTubeDelta=0;
		fDrive    = plugin->ConvertedDrive;
		fDCOffset = plugin->ConvertedDcoffset;
		fMix      = plugin->ConvertedMix;
	} else {
		HasDelta=1;
		fDrive    = plugin->ConvertedDrive    - fDriveDelta;
		fDCOffset = plugin->ConvertedDcoffset - fDCDelta;
		fMix      = plugin->ConvertedMix      - fMixDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fDriveDelta  = fDriveDelta/(float)SampleCount;
			fDCDelta     = fDCDelta/(float)SampleCount;
			fMixDelta    = fMixDelta/(float)SampleCount;
		}
		if(fDriveDelta == 0 && fDCDelta==0) {
			HasTubeDelta=0;
		} else {
			HasTubeDelta=1;
		}
	}

	DCOffsetADJ=ITube_do(fDCOffset,fDrive);	
			   
	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioOutputL = plugin->AudioOutputBufferL;
	EnvInL     = plugin->EnvInLLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvDrive   = plugin->EnvDriveLast;

	if(fBypass==0) {   
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {
			fAudioL=*(pfAudioInputL++);
			TubeOut=ITube_do(fAudioL+fDCOffset,fDrive)-DCOffsetADJ;
			OutL= fPhase <= 0 ? 
					(fAudioL*(1-fMix)) + TubeOut*fMix :
					(fAudioL*(1-fMix)) - TubeOut*fMix ;
			*(pfAudioOutputL++) = OutL;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioL, EnvInL);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL,    EnvOutL);

			drive = fabs(fabs(fabs((fAudioL+fDCOffset)*fDrive) - fabs(fDCOffset*fDrive)) - fabs(TubeOut));
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fMix      += fMixDelta;
				if(HasTubeDelta==1) {
					fDrive    += fDriveDelta;
					fDCOffset += fDCDelta;
					DCOffsetADJ=ITube_do(fDCOffset,fDrive);	
				}
			}

		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
		}
		//evelope on in and out for meters
		EnvInL  =0;
		EnvOutL =0;
		EnvDrive =0;
	}
	// remember for next time round  
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterDrive)=EnvDrive;
}

static void 
runStereoITube(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float OutL,EnvInL,EnvOutL;
	float OutR,EnvInR,EnvOutR;
	float TubeOutL,TubeOutR,drive,EnvDrive;
	float driveL=0;
	float driveR=0;
	float fBypass,fAudioL, fAudioR, fDrive, fDCOffset, DCOffsetADJ, fPhase, fMix;
	double fDriveDelta,fDCDelta,fMixDelta;
	int   HasDelta,HasTubeDelta;
	uint32_t lSampleIndex;
			   
	ITube *plugin = (ITube *)instance;
	pParamFunc = &convertParam;
			   
	/* check for any params changes */
	checkParamChange(ITUBE_BYPASS,   plugin->ControlBypass,   &(plugin->LastBypass),   &(plugin->ConvertedBypass),   plugin->SampleRate, pParamFunc);
	checkParamChange(ITUBE_PHASE,    plugin->ControlPhase,    &(plugin->LastPhase),    &(plugin->ConvertedPhase),    plugin->SampleRate, pParamFunc);
	fDriveDelta = getParamChange(ITUBE_DRIVE,    plugin->ControlDrive,    &(plugin->LastDrive),    &(plugin->ConvertedDrive),    plugin->SampleRate, pParamFunc);
	fDCDelta    = getParamChange(ITUBE_DCOFFSET, plugin->ControlDcoffset, &(plugin->LastDcoffset), &(plugin->ConvertedDcoffset), plugin->SampleRate, pParamFunc);
	fMixDelta   = getParamChange(ITUBE_MIX,      plugin->ControlMix,      &(plugin->LastMix),      &(plugin->ConvertedMix),      plugin->SampleRate, pParamFunc);

	fBypass   = plugin->ConvertedBypass;
	fPhase    = plugin->ConvertedPhase;

	if(fDriveDelta == 0 && fDCDelta==0 && fMixDelta ==0) {
		HasDelta=0;
		HasTubeDelta=0;
		fDrive    = plugin->ConvertedDrive;
		fDCOffset = plugin->ConvertedDcoffset;
		fMix      = plugin->ConvertedMix;
	} else {
		HasDelta=1;
		fDrive    = plugin->ConvertedDrive    - fDriveDelta;
		fDCOffset = plugin->ConvertedDcoffset - fDCDelta;
		fMix      = plugin->ConvertedMix      - fMixDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fDriveDelta  = fDriveDelta/(float)SampleCount;
			fDCDelta     = fDCDelta/(float)SampleCount;
			fMixDelta    = fMixDelta/(float)SampleCount;
		}
		if(fDriveDelta == 0 && fDCDelta==0) {
			HasTubeDelta=0;
		} else {
			HasTubeDelta=1;
		}
	}

	DCOffsetADJ=ITube_do(fDCOffset,fDrive);	
  
	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioInputR = plugin->AudioInputBufferR;
	pfAudioOutputL = plugin->AudioOutputBufferL;
	pfAudioOutputR = plugin->AudioOutputBufferR;

	EnvInL     = plugin->EnvInLLast;
	EnvInR     = plugin->EnvInRLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvOutR    = plugin->EnvOutRLast;
	EnvDrive   = plugin->EnvDriveLast;
  
	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {
			fAudioL=*(pfAudioInputL++);
			TubeOutL=ITube_do(fAudioL+fDCOffset,fDrive)-DCOffsetADJ;
			OutL = fPhase <= 0 ? 
					(fAudioL*(1-fMix)) + TubeOutL*fMix :
					(fAudioL*(1-fMix)) - TubeOutL*fMix ;
			*(pfAudioOutputL++) = OutL;
			  
			fAudioR=*(pfAudioInputR++);
			TubeOutR=ITube_do(fAudioR+fDCOffset,fDrive)-DCOffsetADJ;
			OutR = fPhase <= 0 ? 
					(fAudioR*(1-fMix)) + TubeOutR*fMix :
					(fAudioR*(1-fMix)) - TubeOutR*fMix ;
			*(pfAudioOutputR++) = OutR;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioL, EnvInL);
			EnvInR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioR, EnvInR);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL,    EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR,    EnvOutR);

			driveL = fabs(fabs(fabs((fAudioL+fDCOffset)*fDrive) - fabs(fDCOffset*fDrive)) - fabs(TubeOutL));
			driveR = fabs(fabs(fabs((fAudioR+fDCOffset)*fDrive) - fabs(fDCOffset*fDrive)) - fabs(TubeOutR));
			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fMix      += fMixDelta;
				if(HasTubeDelta==1) {
					fDrive    += fDriveDelta;
					fDCOffset += fDCDelta;
					DCOffsetADJ=ITube_do(fDCOffset,fDrive);	
				}
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
			*(pfAudioOutputR++) = *(pfAudioInputR++);
		}
		//evelope on in and out for meters
		EnvInL  =0;
		EnvInR  =0;
		EnvOutL =0;
		EnvOutR =0;
		EnvDrive =0;
	}
	// store values for next loop
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



static void 
cleanupITube(LV2_Handle instance)
{
	free(instance);
}


static void 
init()
{
	ITubeMonoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	ITubeMonoDescriptor->URI 		= ITUBE_MONO_URI;
	ITubeMonoDescriptor->activate 		= activateITube;
	ITubeMonoDescriptor->cleanup 		= cleanupITube;
	ITubeMonoDescriptor->connect_port 	= connectPortITube;
	ITubeMonoDescriptor->deactivate 	= NULL;
	ITubeMonoDescriptor->instantiate 	= instantiateITube;
	ITubeMonoDescriptor->run 		= runMonoITube;
	ITubeMonoDescriptor->extension_data 	= NULL;

	ITubeStereoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));
	ITubeStereoDescriptor->URI 		= ITUBE_STEREO_URI;
	ITubeStereoDescriptor->activate 	= activateITube;
	ITubeStereoDescriptor->cleanup 		= cleanupITube;
	ITubeStereoDescriptor->connect_port 	= connectPortITube;
	ITubeStereoDescriptor->deactivate 	= NULL;
	ITubeStereoDescriptor->instantiate 	= instantiateITube;
	ITubeStereoDescriptor->run 		= runStereoITube;
	ITubeStereoDescriptor->extension_data 	= NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!ITubeMonoDescriptor) init();

	switch (index) {
	case 0:
		return ITubeMonoDescriptor;
	case 1:
		return ITubeStereoDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/

float 
convertParam(unsigned long param, float value, double sr) {

	float result;

	switch(param)
	{
		case ITUBE_DRIVE:
			if(value<0)
				result= 1;
			else if (value < 18)
				result = pow(10,value/20.0);
			else
				result= pow(10,0.9);
			break;
		case ITUBE_DCOFFSET:
			if(value<-1)
				result= -1;
			else if (value < 0)
				result = -pow(value,2);
			else if (value < 1)
				result = pow(value,2);
			else
				result= 1;
			break;
		case ITUBE_BYPASS:
		case ITUBE_PHASE:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		case ITUBE_MIX:
			if(value<0)
				result= 0;
			else if (value < 100)
				result = value/100;
			else
				result= 1;
			break;
		default:
			result=0;
			break;
	}
	return result;
}

