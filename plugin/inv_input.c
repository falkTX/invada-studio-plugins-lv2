/* 

    This LV2 plugin provides a stereo input module

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lv2.h>
#include "library/common.h"
#include "inv_input.h"


static LV2_Descriptor *IInputDescriptor = NULL;


typedef struct {

	/* Ports */
	float *ControlBypass;
	float *ControlPhaseL;
	float *ControlPhaseR;         
	float *ControlGain;
	float *ControlPan;  
	float *ControlWidth;  
	float *ControlNoClip;  
	float *AudioInputBufferL;
	float *AudioInputBufferR; 
	float *AudioOutputBufferL;
	float *AudioOutputBufferR;
	float *MeterInputL;
	float *MeterInputR; 
	float *MeterOutputL;
	float *MeterOutputR;
	float *MeterPhase;
	float *MeterDrive;

	/* stuff we need to remember to reduce cpu */ 
	double SampleRate; 
	struct Envelope EnvAD[4];

	/* stuff we need to remember to reduce cpu */ 
	float LastBypass;
	float LastPhaseL;
	float LastPhaseR;     
	float LastGain;
	float LastPan;  
	float LastWidth;  
	float LastNoClip; 

	float ConvertedBypass;
	float ConvertedPhaseL;
	float ConvertedPhaseR; 
	float ConvertedGain;
	float ConvertedPan;  
	float ConvertedWidth;  
	float ConvertedNoClip; 
	float EnvInLLast; 
	float EnvInRLast; 
	float EnvOutLLast; 
	float EnvOutRLast; 
	float EnvPhaseLast; 
	float EnvDriveLast; 


} IInput;

static LV2_Handle 
instantiateIInput(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IInput *plugin = (IInput *)malloc(sizeof(IInput));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;

	return (LV2_Handle)plugin;
}



static void 
connectPortIInput(LV2_Handle instance, uint32_t port, void *data)
{
	IInput *plugin = (IInput *)instance;

	switch (port) {
		case IINPUT_BYPASS:
			plugin->ControlBypass = data;
			break;
		case IINPUT_PHASEL:
			plugin->ControlPhaseL = data;
			break;
		case IINPUT_PHASER:
			plugin->ControlPhaseR = data;
			break;
		case IINPUT_GAIN:
			plugin->ControlGain  = data;
			break;
		case IINPUT_PAN:
			plugin->ControlPan = data;
			break;
		case IINPUT_WIDTH:
			plugin->ControlWidth = data;
			break;
		case IINPUT_NOCLIP:
			plugin->ControlNoClip = data;
			break;
		case IINPUT_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case IINPUT_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case IINPUT_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case IINPUT_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case IINPUT_METER_INL:
			plugin->MeterInputL = data;
			break;
		case IINPUT_METER_INR:
			plugin->MeterInputR = data;
			break;
		case IINPUT_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case IINPUT_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
		case IINPUT_METER_PHASE:
			plugin->MeterPhase = data;
			break;
		case IINPUT_METER_DRIVE:
			plugin->MeterDrive = data;
			break;
	}
}


static void 
activateIInput(LV2_Handle instance) 
{

	IInput *plugin = (IInput *)instance;

	// these values force the conversion to take place      

	plugin->LastBypass = 0;
	plugin->LastPhaseL = 0;
	plugin->LastPhaseR = 0;     
	plugin->LastGain   = 0;
	plugin->LastPan    = 0;  
	plugin->LastWidth  = 0;  
	plugin->LastNoClip = 0; 
	plugin->EnvInLLast = 0; 
	plugin->EnvInRLast = 0; 
	plugin->EnvOutLLast = 0; 
	plugin->EnvOutRLast = 0; 
	plugin->EnvPhaseLast = 0; 
	plugin->EnvDriveLast = 0; 

	plugin->ConvertedBypass = convertParam(IINPUT_BYPASS, plugin->LastBypass,  plugin->SampleRate);
	plugin->ConvertedPhaseL = convertParam(IINPUT_PHASEL, plugin->LastPhaseL,  plugin->SampleRate);
	plugin->ConvertedPhaseR = convertParam(IINPUT_PHASER, plugin->LastPhaseR,  plugin->SampleRate);
	plugin->ConvertedGain   = convertParam(IINPUT_GAIN,   plugin->LastGain,    plugin->SampleRate);
	plugin->ConvertedPan    = convertParam(IINPUT_PAN,    plugin->LastPan,     plugin->SampleRate);
	plugin->ConvertedWidth  = convertParam(IINPUT_WIDTH,  plugin->LastWidth,   plugin->SampleRate);
	plugin->ConvertedNoClip = convertParam(IINPUT_NOCLIP, plugin->LastNoClip,  plugin->SampleRate);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void 
runIInput(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float fBypass,fPhaseL,fPhaseR,fGain,fPan,fLPan,fRPan,fWidth,fMono,fStereoL,fStereoR,fNoClip;
	double fGainDelta,fPanDelta,fWidthDelta;
	int   HasDelta;
	float fAudioL,fAudioR;
	float drive;
	float driveL=0;
	float driveR=0;
	float InL,EnvInL,EnvOutL;
	float InR,EnvInR,EnvOutR;
	float CurrentPhase,EnvPhase,EnvDrive;
	uint32_t lSampleIndex;

	IInput *plugin = (IInput *)instance;

	pParamFunc = &convertParam;

	checkParamChange(IINPUT_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IINPUT_PHASEL, plugin->ControlPhaseL, &(plugin->LastPhaseL), &(plugin->ConvertedPhaseL), plugin->SampleRate, pParamFunc);
	checkParamChange(IINPUT_PHASER, plugin->ControlPhaseR, &(plugin->LastPhaseR), &(plugin->ConvertedPhaseR), plugin->SampleRate, pParamFunc);
	checkParamChange(IINPUT_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fGainDelta  = getParamChange(IINPUT_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);
	fPanDelta   = getParamChange(IINPUT_PAN,    plugin->ControlPan,    &(plugin->LastPan),    &(plugin->ConvertedPan),    plugin->SampleRate, pParamFunc);
	fWidthDelta = getParamChange(IINPUT_WIDTH,  plugin->ControlWidth,  &(plugin->LastWidth),  &(plugin->ConvertedWidth),  plugin->SampleRate, pParamFunc);


 
	fBypass    = plugin->ConvertedBypass;
	fPhaseL    = plugin->ConvertedPhaseL;
	fPhaseR    = plugin->ConvertedPhaseR;
	fNoClip    = plugin->ConvertedNoClip;

	if(fGainDelta == 0 && fPanDelta==0 && fWidthDelta==0) {
		HasDelta=0;
		fGain      = plugin->ConvertedGain;
		fPan       = plugin->ConvertedPan;
		fWidth     = plugin->ConvertedWidth;
	} else {
		HasDelta=1;
		fGain      = plugin->ConvertedGain  - fGainDelta;
		fPan       = plugin->ConvertedPan   - fPanDelta;
		fWidth     = plugin->ConvertedWidth - fWidthDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fGainDelta  = fGainDelta/(float)SampleCount;
			fPanDelta   = fPanDelta/(float)SampleCount;
			fWidthDelta = fWidthDelta/(float)SampleCount;
		}
	}

	fLPan=1-fPan;
	fRPan=1+fPan;

	pfAudioInputL = plugin->AudioInputBufferL;
	pfAudioInputR = plugin->AudioInputBufferR;
	pfAudioOutputL = plugin->AudioOutputBufferL;
	pfAudioOutputR = plugin->AudioOutputBufferR;

	EnvInL     = plugin->EnvInLLast;
	EnvInR     = plugin->EnvInRLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvOutR    = plugin->EnvOutRLast;
	EnvPhase   = plugin->EnvPhaseLast;
	EnvDrive   = plugin->EnvDriveLast;
 
	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			InL=*(pfAudioInputL++);
			InR=*(pfAudioInputR++);
	  
			fAudioL =  fPhaseL > 0 ? -InL : InL ;
			fAudioR =  fPhaseR > 0 ? -InR : InR ;
			fAudioL *= fGain;
			fAudioR *= fGain;
			fAudioL *= fLPan;
			fAudioR *= fRPan;
		  
			if(fWidth<=0) {
				fMono = (fAudioL + fAudioR) / 2;
				fAudioL = (1+fWidth)*fAudioL - fWidth*fMono;
				fAudioR = (1+fWidth)*fAudioR - fWidth*fMono;
			} else {
				fStereoL = (fAudioL - fAudioR) / 2;
				fStereoR = (fAudioR - fAudioL) / 2;
				fAudioL = (1-fWidth)*fAudioL + fWidth*fStereoL;
				fAudioR = (1-fWidth)*fAudioR + fWidth*fStereoR;
			}

			fAudioL = fNoClip > 0 ? InoClip(fAudioL,&driveL) : fAudioL;
			fAudioR = fNoClip > 0 ? InoClip(fAudioR,&driveR) : fAudioR;
			*(pfAudioOutputL++) = fAudioL;
			*(pfAudioOutputR++) = fAudioR;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL,     EnvInL);
			EnvInR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InR,     EnvInR);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioL, EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioR, EnvOutR);

			if(fabs(fAudioL) > 0.001 || fabs(fAudioR) > 0.001) {  // -60 db
				CurrentPhase = fabs(fAudioL+fAudioR) > 0.000001 ? atan(fabs((fAudioL-fAudioR)/(fAudioL+fAudioR))) : PI_ON_2;
			} else {
				CurrentPhase =0;
			}
			EnvPhase  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE],CurrentPhase,EnvPhase);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);


			//update any changing parameters
			if(HasDelta==1) {
				fGain  += fGainDelta;
				fPan   += fPanDelta;
				fWidth += fWidthDelta;
				fLPan=1-fPan;
				fRPan=1+fPan;
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
		EnvPhase =0;
		EnvDrive =0;
	}

	// store values for next loop
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvInRLast = (fabs(EnvInR)<1.0e-10)  ? 0.f : EnvInR; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 
	plugin->EnvPhaseLast = (fabs(EnvPhase)<1.0e-10)  ? 0.f : EnvPhase; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterInputR) =(EnvInR  > 0.001) ? 20*log10(EnvInR)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
	*(plugin->MeterPhase)=EnvPhase;
	*(plugin->MeterDrive)=EnvDrive;
}


static void 
cleanupIInput(LV2_Handle instance)
{
	free(instance);
}


static void 
init()
{
	IInputDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IInputDescriptor->URI 		= IINPUT_URI;
	IInputDescriptor->activate 	= activateIInput;
	IInputDescriptor->cleanup 	= cleanupIInput;
	IInputDescriptor->connect_port 	= connectPortIInput;
	IInputDescriptor->deactivate 	= NULL;
	IInputDescriptor->instantiate 	= instantiateIInput;
	IInputDescriptor->run 		= runIInput;
	IInputDescriptor->extension_data = NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IInputDescriptor) init();

	switch (index) {
	case 0:
		return IInputDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/


float 
convertParam(unsigned long param, float value, double sr) {
	float result;
	switch(param)  {
		case IINPUT_BYPASS:
		case IINPUT_PHASEL:
		case IINPUT_PHASER:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		case IINPUT_GAIN:
			if(value<-24)
				result= pow(10, -24/20);
			else if (value < 24)
				result= pow(10, value/20);
			else
				result= pow(10, 1.2);
			break;
		case IINPUT_PAN:
		case IINPUT_WIDTH:
			if(value < -1)
				result= -1;
			else if (value < 1)
				result= value;
			else
				result= 1;
			break;
		case IINPUT_NOCLIP:
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


