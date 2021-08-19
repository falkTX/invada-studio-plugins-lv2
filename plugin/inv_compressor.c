/* 
    This LV2 plugin provides a mono and stereo compressor.

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
#include "inv_compressor.h"




static LV2_Descriptor *ICompMonoDescriptor = NULL;
static LV2_Descriptor *ICompStereoDescriptor = NULL;


typedef struct {


	/* Ports */
	float * ControlBypass;  
	float * ControlRms;         
	float * ControlAttack;
	float * ControlRelease;  
	float * ControlThresh; 
	float * ControlRatio;
	float * ControlGain;
	float * ControlNoClip;
	float * ControlMeter;
	float * AudioInputBufferL;
	float * AudioOutputBufferL;
	float * AudioInputBufferR; 
	float * AudioOutputBufferR;
	float * MeterInputL;
	float * MeterOutputL;
	float * MeterInputR; 
	float * MeterOutputR;
	float * MeterDrive;

	double SampleRate; 

	struct Envelope EnvAD[4];

	/* these params are used to remember the control values and the converted (internal) value to save a bit of cpu converting them every run */
	float LastBypass;  
	float LastRms;         
	float LastAttack;
	float LastRelease;  
	float LastThresh; 
	float LastRatio;
	float LastGain;
	float LastNoClip;

	float ConvertedBypass;   
	float ConvertedRms;         
	float ConvertedAttack;
	float ConvertedRelease;  
	float ConvertedThresh; 
	float ConvertedRatio;
	float ConvertedGain;
	float ConvertedNoClip;
	float EnvInLLast; 
	float EnvOutLLast; 
	float EnvInRLast; 
	float EnvOutRLast;
	float EnvDriveLast;  

	/* this stuff needs to be remembered between run calls */
	float Envelope; 
	float Rms;

} IComp;


static LV2_Handle instantiateIComp(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IComp *plugin = (IComp *)malloc(sizeof(IComp));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;

	return (LV2_Handle)plugin;
}


static void connectPortIComp(LV2_Handle instance, uint32_t port, void *data)
{
	IComp *plugin = (IComp *)instance;

	switch (port) {
		case ICOMP_BYPASS:
			plugin->ControlBypass = data;
			break;
		case ICOMP_RMS:
			plugin->ControlRms = data;
			break;
		case ICOMP_ATTACK:
			plugin->ControlAttack  = data;
			break;
		case ICOMP_RELEASE:
			plugin->ControlRelease = data;
			break;
		case ICOMP_THRESH:
			plugin->ControlThresh = data;
			break;
		case ICOMP_RATIO:
			plugin->ControlRatio = data;
			break;
		case ICOMP_GAIN:
			plugin->ControlGain = data;
			break;
		case ICOMP_NOCLIP:
			plugin->ControlNoClip = data;
			break;
		case ICOMP_METER_GR:
			plugin->ControlMeter = data;
			break;
		case ICOMP_AUDIO_INPUTL:
			plugin->AudioInputBufferL = data;
			break;
		case ICOMP_AUDIO_OUTPUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case ICOMP_AUDIO_INPUTR:
			plugin->AudioInputBufferR = data;
			break;
		case ICOMP_AUDIO_OUTPUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case ICOMP_METER_INL:
			plugin->MeterInputL = data;
			break;
		case ICOMP_METER_INR:
			plugin->MeterInputR = data;
			break;
		case ICOMP_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case ICOMP_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
		case ICOMP_METER_DRIVE:
			plugin->MeterDrive = data;
			break;
	}
}


static void activateIComp(LV2_Handle instance) 
{

	IComp *plugin = (IComp *)instance;

	plugin->Envelope=0;
	plugin->Rms=0;

	/* default values */
	plugin->LastBypass	= 0.0;   
	plugin->LastRms    	= 0.5;         
	plugin->LastAttack 	= 0.015;
	plugin->LastRelease	= 0.050;  
	plugin->LastThresh 	= 0.0; 
	plugin->LastRatio  	= 1.0;
	plugin->LastGain   	= 0.0;
	plugin->LastNoClip 	= 1.0;
	plugin->EnvInLLast 	= 0.0; 
	plugin->EnvOutLLast 	= 0.0; 
	plugin->EnvInRLast 	= 0.0; 
	plugin->EnvOutRLast 	= 0.0; 
	plugin->EnvDriveLast 	= 0.0; 

	plugin->ConvertedBypass =convertParam(ICOMP_BYPASS,  plugin->LastBypass,  plugin->SampleRate);
	plugin->ConvertedRms    =convertParam(ICOMP_RMS,     plugin->LastRms,     plugin->SampleRate);
	plugin->ConvertedAttack =convertParam(ICOMP_ATTACK,  plugin->LastAttack,  plugin->SampleRate);
	plugin->ConvertedRelease=convertParam(ICOMP_RELEASE, plugin->LastRelease, plugin->SampleRate);
	plugin->ConvertedThresh =convertParam(ICOMP_THRESH,  plugin->LastThresh,  plugin->SampleRate);
	plugin->ConvertedRatio  =convertParam(ICOMP_RATIO,   plugin->LastRatio,   plugin->SampleRate);
	plugin->ConvertedGain   =convertParam(ICOMP_GAIN,    plugin->LastGain,    plugin->SampleRate);
	plugin->ConvertedNoClip =convertParam(ICOMP_NOCLIP,  plugin->LastNoClip,  plugin->SampleRate);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void runMonoIComp(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float OutL,EnvInL,EnvOutL,EnvDrive;
	float fAudioL,fEnvelope,fRms,fRmsSize;
	float fBypass,fAttack,fRelease,fThresh,fRatio,fGain,fCompGain,fNoClip;
	double fRmsDelta,fAttackDelta,fReleaseDelta,fThreshDelta,fRatioDelta,fGainDelta;
	int   HasDelta;
	float drive=0;
	unsigned long lSampleIndex;
			   
	IComp *plugin = (IComp *)instance;
	pParamFunc = &convertParam;

	/* see if any params have changed */
	checkParamChange(ICOMP_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(ICOMP_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);

	fRmsDelta     = getParamChange(ICOMP_RMS,    plugin->ControlRms,    &(plugin->LastRms),    &(plugin->ConvertedRms),    plugin->SampleRate, pParamFunc);
	fAttackDelta  = getParamChange(ICOMP_ATTACK, plugin->ControlAttack, &(plugin->LastAttack), &(plugin->ConvertedAttack), plugin->SampleRate, pParamFunc);
	fReleaseDelta = getParamChange(ICOMP_RELEASE,plugin->ControlRelease,&(plugin->LastRelease),&(plugin->ConvertedRelease),plugin->SampleRate, pParamFunc);
	fThreshDelta  = getParamChange(ICOMP_THRESH, plugin->ControlThresh, &(plugin->LastThresh), &(plugin->ConvertedThresh), plugin->SampleRate, pParamFunc);
	fRatioDelta   = getParamChange(ICOMP_RATIO,  plugin->ControlRatio,  &(plugin->LastRatio),  &(plugin->ConvertedRatio),  plugin->SampleRate, pParamFunc);
	fGainDelta    = getParamChange(ICOMP_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);

	fBypass   = plugin->ConvertedBypass;
	fNoClip   = plugin->ConvertedNoClip;

	if(fRmsDelta == 0 && fAttackDelta==0 && fReleaseDelta==0 && fThreshDelta==0 && fRatioDelta==0 && fGainDelta==0 ) {
		HasDelta=0;
		fRmsSize  = plugin->ConvertedRms;
		fAttack   = plugin->ConvertedAttack;
		fRelease  = plugin->ConvertedRelease;
		fThresh   = plugin->ConvertedThresh;
		fRatio    = plugin->ConvertedRatio;
		fGain     = plugin->ConvertedGain;
	} else {
		HasDelta=1;
		fRmsSize  = plugin->ConvertedRms     - fRmsDelta;
		fAttack   = plugin->ConvertedAttack  - fAttackDelta;
		fRelease  = plugin->ConvertedRelease - fReleaseDelta;
		fThresh   = plugin->ConvertedThresh  - fThreshDelta;
		fRatio    = plugin->ConvertedRatio   - fRatioDelta;
		fGain     = plugin->ConvertedGain    - fGainDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fRmsDelta     = fRmsDelta/(float)SampleCount;
			fAttackDelta  = fAttackDelta/(float)SampleCount;
			fReleaseDelta = fReleaseDelta/(float)SampleCount;
			fThreshDelta  = fThreshDelta/(float)SampleCount;
			fRatioDelta   = fRatioDelta/(float)SampleCount;
			fGainDelta    = fGainDelta/(float)SampleCount;
		}
	}

	fEnvelope = plugin->Envelope;   
	fRms      = plugin->Rms;
	fCompGain = 1; // this is set before it is used unless we are given no samples in which case it doesn't matter
	EnvInL     = plugin->EnvInLLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvDrive   = plugin->EnvDriveLast;

	pfAudioInputL  = plugin->AudioInputBufferL;
	pfAudioOutputL = plugin->AudioOutputBufferL;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
		{
			fAudioL=*(pfAudioInputL++);
			// work out the rms
			fRms = sqrt(( (fRmsSize-1)*fRms*fRms + fAudioL*fAudioL ) / fRmsSize); 
			// work out the envelope
			fEnvelope += (fRms > fEnvelope) ? fAttack * (fRms - fEnvelope) : fRelease * (fRms - fEnvelope);
			// work out the gain	  
			fCompGain = (fEnvelope > fThresh) ? (pow((fEnvelope/fThresh), ((1.0/fRatio)-1.0) )) : 1;

			OutL= fNoClip > 0 ? InoClip(fAudioL*fCompGain * fGain,&drive ) : fAudioL*fCompGain * fGain ;
			*(pfAudioOutputL++) =OutL;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioL, EnvInL);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL,    EnvOutL);
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fRmsSize  += fRmsDelta;
				fAttack   += fAttackDelta;
				fRelease  += fReleaseDelta;
				fThresh   += fThreshDelta;
				fRatio    += fRatioDelta;
				fGain     += fGainDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
		}
		//zero everything
		fRms 		= 0.0;
		fEnvelope 	= 0.0;
		fCompGain	= 1.0;
		EnvInL  	= 0.0;
		EnvOutL 	= 0.0;
		EnvDrive 	= 0.0;
	}
	// remember for next time round
	plugin->Envelope = (fabs(fEnvelope)<1.0e-10)  ? 0.f : fEnvelope; 
	plugin->Rms = (fabs(fRms)<1.0e-10)  ? 0.f : fRms; 
	plugin->EnvInLLast = (fabs(EnvInL)<1.0e-10)  ? 0.f : EnvInL; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterDrive)=EnvDrive;
	// 0.015848932=-36dB (the max gain reduction this compressor can do)
	*(plugin->ControlMeter)=(fCompGain > 0.015848932) ? 20*log10(fCompGain) : -36.0;
}


static void runStereoIComp(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float fAudioL,fAudioR,fMaxAudio,fEnvelope,fRms,fRmsSize;
	float fBypass,fAttack,fRelease,fThresh,fRatio,fGain,fCompGain,fNoClip;
	double fRmsDelta,fAttackDelta,fReleaseDelta,fThreshDelta,fRatioDelta,fGainDelta;
	int   HasDelta;
	float OutL,EnvInL,EnvOutL;
	float OutR,EnvInR,EnvOutR;
	float drive,EnvDrive;
	float driveL=0;
	float driveR=0;
	unsigned long lSampleIndex;
			   
	IComp *plugin = (IComp *)instance;
	pParamFunc = &convertParam;
			   
	/* see if any params have changed */
	checkParamChange(ICOMP_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(ICOMP_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);

	fRmsDelta     = getParamChange(ICOMP_RMS,    plugin->ControlRms,    &(plugin->LastRms),    &(plugin->ConvertedRms),    plugin->SampleRate, pParamFunc);
	fAttackDelta  = getParamChange(ICOMP_ATTACK, plugin->ControlAttack, &(plugin->LastAttack), &(plugin->ConvertedAttack), plugin->SampleRate, pParamFunc);
	fReleaseDelta = getParamChange(ICOMP_RELEASE,plugin->ControlRelease,&(plugin->LastRelease),&(plugin->ConvertedRelease),plugin->SampleRate, pParamFunc);
	fThreshDelta  = getParamChange(ICOMP_THRESH, plugin->ControlThresh, &(plugin->LastThresh), &(plugin->ConvertedThresh), plugin->SampleRate, pParamFunc);
	fRatioDelta   = getParamChange(ICOMP_RATIO,  plugin->ControlRatio,  &(plugin->LastRatio),  &(plugin->ConvertedRatio),  plugin->SampleRate, pParamFunc);
	fGainDelta    = getParamChange(ICOMP_GAIN,   plugin->ControlGain,   &(plugin->LastGain),   &(plugin->ConvertedGain),   plugin->SampleRate, pParamFunc);

	fBypass   = plugin->ConvertedBypass;
	fNoClip   = plugin->ConvertedNoClip;

	if(fRmsDelta == 0 && fAttackDelta==0 && fReleaseDelta==0 && fThreshDelta==0 && fRatioDelta==0 && fGainDelta==0 ) {
		HasDelta=0;
		fRmsSize  = plugin->ConvertedRms;
		fAttack   = plugin->ConvertedAttack;
		fRelease  = plugin->ConvertedRelease;
		fThresh   = plugin->ConvertedThresh;
		fRatio    = plugin->ConvertedRatio;
		fGain     = plugin->ConvertedGain;
	} else {
		HasDelta=1;
		fRmsSize  = plugin->ConvertedRms     - fRmsDelta;
		fAttack   = plugin->ConvertedAttack  - fAttackDelta;
		fRelease  = plugin->ConvertedRelease - fReleaseDelta;
		fThresh   = plugin->ConvertedThresh  - fThreshDelta;
		fRatio    = plugin->ConvertedRatio   - fRatioDelta;
		fGain     = plugin->ConvertedGain    - fGainDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fRmsDelta     = fRmsDelta/(float)SampleCount;
			fAttackDelta  = fAttackDelta/(float)SampleCount;
			fReleaseDelta = fReleaseDelta/(float)SampleCount;
			fThreshDelta  = fThreshDelta/(float)SampleCount;
			fRatioDelta   = fRatioDelta/(float)SampleCount;
			fGainDelta    = fGainDelta/(float)SampleCount;
		}
	}

	fEnvelope = plugin->Envelope;   
	fRms      = plugin->Rms;
	fCompGain = 1; // this is set before it is used unless we are given no samples in which case it doesn't matter
	EnvInL     = plugin->EnvInLLast;
	EnvInR     = plugin->EnvInRLast;
	EnvOutL    = plugin->EnvOutLLast;
	EnvOutR    = plugin->EnvOutRLast;
	EnvDrive   = plugin->EnvDriveLast;

	pfAudioInputL  = plugin->AudioInputBufferL;
	pfAudioInputR  = plugin->AudioInputBufferR;
	pfAudioOutputL = plugin->AudioOutputBufferL;
	pfAudioOutputR = plugin->AudioOutputBufferR;
	
	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
		{
			fAudioL=*(pfAudioInputL++);
			fAudioR=*(pfAudioInputR++);

			// work out the rms
			fMaxAudio = fabs(fAudioL) > fabs(fAudioR) ? fAudioL : fAudioR;
			fRms = sqrt(( (fRmsSize-1)*fRms*fRms + fMaxAudio*fMaxAudio ) / fRmsSize); 
			// work out the envelope
			fEnvelope += (fRms > fEnvelope) ? fAttack * (fRms - fEnvelope) : fRelease * (fRms - fEnvelope);
			// work out the gain	  
			fCompGain = (fEnvelope > fThresh) ? (pow((fEnvelope/fThresh), ((1.0/fRatio)-1.0))) : 1;

			OutL = fNoClip > 0 ? InoClip(fAudioL*fCompGain*fGain,&driveL) : fAudioL*fCompGain*fGain ;
			OutR = fNoClip > 0 ? InoClip(fAudioR*fCompGain*fGain,&driveR) : fAudioR*fCompGain*fGain ;
			*(pfAudioOutputL++) = OutL ;
			*(pfAudioOutputR++) = OutR ;

			//evelope on in and out for meters
			EnvInL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioL, EnvInL);
			EnvInR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudioR, EnvInR);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL,    EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR,    EnvOutR);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fRmsSize  += fRmsDelta;
				fAttack   += fAttackDelta;
				fRelease  += fReleaseDelta;
				fThresh   += fThreshDelta;
				fRatio    += fRatioDelta;
				fGain     += fGainDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
			*(pfAudioOutputR++) = *(pfAudioInputR++);
		}
		//zero everything
		fRms 		= 0.0;
		fEnvelope 	= 0.0;
		fCompGain	= 1.0;
		EnvInL  	= 0.0;
		EnvInR  	= 0.0;
		EnvOutL 	= 0.0;
		EnvOutR 	= 0.0;
		EnvDrive 	= 0.0;
	}

	// remember for next time round
	plugin->Envelope = (fabs(fEnvelope)<1.0e-10)  ? 0.f : fEnvelope; 
	plugin->Rms = (fabs(fRms)<1.0e-10)  ? 0.f : fRms; 
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
	// 0.015848932=-36dB (the max gain reduction this compressor can do)
	*(plugin->ControlMeter)=(fCompGain > 0.015848932) ? 20*log10(fCompGain) : -36.0;
}


static void cleanupIComp(LV2_Handle instance)
{
	free(instance);
}
 

static void init()
{
	ICompMonoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	ICompMonoDescriptor->URI 		= ICOMP_MONO_URI;
	ICompMonoDescriptor->activate 		= activateIComp;
	ICompMonoDescriptor->cleanup 		= cleanupIComp;
	ICompMonoDescriptor->connect_port 	= connectPortIComp;
	ICompMonoDescriptor->deactivate 	= NULL;
	ICompMonoDescriptor->instantiate 	= instantiateIComp;
	ICompMonoDescriptor->run 		= runMonoIComp;
	ICompMonoDescriptor->extension_data 	= NULL;

	ICompStereoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	ICompStereoDescriptor->URI 		= ICOMP_STEREO_URI;
	ICompStereoDescriptor->activate 	= activateIComp;
	ICompStereoDescriptor->cleanup 		= cleanupIComp;
	ICompStereoDescriptor->connect_port 	= connectPortIComp;
	ICompStereoDescriptor->deactivate 	= NULL;
	ICompStereoDescriptor->instantiate 	= instantiateIComp;
	ICompStereoDescriptor->run 		= runStereoIComp;
	ICompStereoDescriptor->extension_data 	= NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!ICompMonoDescriptor) init();

	switch (index) {
	case 0:
		return ICompMonoDescriptor;
	case 1:
		return ICompStereoDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/



float convertParam(unsigned long param, float value, double sr) {
/* some conversion formulae are shared so the bounds are the min/max across all ports */
	float result;
	switch(param)  {
		case ICOMP_RMS:
			if(value<0) 
				result= 1;
			else if (value < 1)
				result= (pow(value,3) * (float)sr/20)+1;
			else
				result= ((float)sr/20)+1;
			break;
		case ICOMP_ATTACK:
			if(value<0.00001)
				result= 1 - pow(10, -301.0301 / ((float)sr * 0.01)); 
			else if (value <0.750)
				result= 1 - pow(10, -301.0301 / ((float)sr * value*1000.0)); 
			else
				result= 1 - pow(10, -301.0301 / ((float)sr * 750.0)); 
			break;
		case ICOMP_RELEASE:
			if(value<0.001)
				result= 1 - pow(10, -301.0301 / ((float)sr * 1)); 
			else if (value <5)
				result= 1 - pow(10, -301.0301 / ((float)sr * value*1000.0)); 
			else
				result= 1 - pow(10, -301.0301 / ((float)sr * 5000.0)); 
			break;
		case ICOMP_THRESH:
		case ICOMP_RATIO:
		case ICOMP_GAIN:
			if(value<-36)
				result= pow(10, -1.8);
			else if (value < 36)
				result= pow(10, value/20.0);
			else
				result= pow(10, 1.8);
			break;
		case ICOMP_NOCLIP:
		case ICOMP_BYPASS:
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

