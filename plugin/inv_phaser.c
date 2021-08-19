/* 

    This LV2 plugin provides phaser plugins

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
#include "inv_phaser.h"

static LV2_Descriptor *IPhaserMonoDescriptor = NULL;
static LV2_Descriptor *IPhaserStereoDescriptor = NULL;
static LV2_Descriptor *IPhaserSumDescriptor = NULL;



typedef struct {

	/* Ports */
	float *ControlBypass;
	float *ControlCycle;
	float *ControlPhase;
	float *ControlWidth; 
	float *ControlDepth;
	float *ControlNoClip;

	float *AudioOutputBufferL;
	float *AudioOutputBufferR;
	float *AudioInputBufferL;
	float *AudioInputBufferR; 

	float *MeterInputL;
	float *MeterInputR;
	float *MeterOutputL;
	float *MeterOutputR;
	float *LampLfoL;
	float *LampLfoR;
	float *LampNoClip;

	double SampleRate;
	float Offset;
	struct Envelope EnvAD[4];

	/* Stuff to remember to avoid recalculating the delays every run */
	float LastBypass;
	float LastCycle;
	float LastPhase;
	float LastWidth; 
	float LastDepth;
	float LastNoClip;

	float AngleLast; 
	float EnvInLLast; 
	float EnvInRLast; 
	float EnvOutLLast; 
	float EnvOutRLast;
	float EnvDriveLast; 

	float ConvertedBypass; 
	float ConvertedCycle;
	float ConvertedPhase;
	float ConvertedWidth; 
	float ConvertedDepth;
	float ConvertedNoClip;

	/* Delay Space Data */
	unsigned long SpaceSize;
	float * SpaceL;
	float * SpaceR;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

} IPhaser;


static LV2_Handle 
instantiateIPhaser(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IPhaser *plugin = (IPhaser *)malloc(sizeof(IPhaser));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;
	plugin->Offset =  s_rate / 10000;  /* 10khz wavelength */
	plugin->SpaceSize = PHASER_SPACE_SIZE * s_rate / 1000;

	/* the delay space */
	if((plugin->SpaceL  = (float *)malloc(sizeof(float) * plugin->SpaceSize))==NULL)
    		return NULL;
	
	if((plugin->SpaceR  = (float *)malloc(sizeof(float) * plugin->SpaceSize))==NULL)
    		return NULL;

	return (LV2_Handle)plugin;
}


static void 
connectPortIPhaser(LV2_Handle instance, uint32_t port, void *data)
{
	IPhaser *plugin = (IPhaser *)instance;
	switch (port) {
		case IPHASER_BYPASS:
			plugin->ControlBypass = data;
			break;
		case IPHASER_CYCLE:
			plugin->ControlCycle = data;
			break;
		case IPHASER_PHASE:
			plugin->ControlPhase = data;
			break;
		case IPHASER_WIDTH:
			plugin->ControlWidth = data;
			break;
		case IPHASER_DEPTH:
			plugin->ControlDepth = data;
			break;
		case IPHASER_NOCLIP:
			plugin->ControlNoClip = data;
			break;
		case IPHASER_LAMP_L:
			plugin->LampLfoL = data;
			break;
		case IPHASER_LAMP_R:
			plugin->LampLfoR = data;
			break;
		case IPHASER_LAMP_NOCLIP:
			plugin->LampNoClip = data;
			break;
		case IPHASER_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case IPHASER_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case IPHASER_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case IPHASER_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case IPHASER_METER_INL:
			plugin->MeterInputL = data;
			break;
		case IPHASER_METER_INR:
			plugin->MeterInputR = data;
			break;
		case IPHASER_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case IPHASER_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
	}
}


static void 
activateIPhaser(LV2_Handle instance) 
{
	IPhaser *plugin = (IPhaser *)instance;

	unsigned long i;
	float * p;
	float * q;
	
	//set ourselves at the beginning of space
	plugin->SpaceLCur=plugin->SpaceL;
	plugin->SpaceRCur=plugin->SpaceR;

	// clear space	
	p=plugin->SpaceL;
	q=plugin->SpaceR;
	for(i=0; i < plugin->SpaceSize; i++) {
		*(p++)=0;
		*(q++)=0;
	}
	plugin->SpaceLEnd=--p;
	plugin->SpaceREnd=--q;
  
	//set defaults
	plugin->LastBypass    	= 0.0;
	plugin->LastCycle 	= 25.0;
	plugin->LastPhase  	= 45.0; 
	plugin->LastWidth 	= 10.5;
	plugin->LastDepth   	= 75.0;
	plugin->LastNoClip   	= 1.0;

	plugin->EnvInLLast 	= 0; 
	plugin->EnvInRLast 	= 0; 
	plugin->EnvOutLLast 	= 0; 
	plugin->EnvOutRLast 	= 0; 
	plugin->EnvDriveLast 	= 0; 

	plugin->AngleLast 	= 0; 

	plugin->ConvertedBypass = convertParam(IPHASER_BYPASS, plugin->LastBypass, plugin->SampleRate);  
	plugin->ConvertedCycle  = convertParam(IPHASER_CYCLE,  plugin->LastCycle,  plugin->SampleRate);  
	plugin->ConvertedPhase  = convertParam(IPHASER_PHASE,  plugin->LastPhase,  plugin->SampleRate);  
	plugin->ConvertedWidth  = convertParam(IPHASER_WIDTH,  plugin->LastWidth,  plugin->SampleRate);  
	plugin->ConvertedDepth  = convertParam(IPHASER_DEPTH,  plugin->LastDepth,  plugin->SampleRate);  
	plugin->ConvertedNoClip = convertParam(IPHASER_NOCLIP, plugin->LastNoClip, plugin->SampleRate);  

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void 
runMonoIPhaser(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float * pfAudioOutputR;	
	float fOffset;
	float fAngle;
	float In,OutL,OutR;
	float EnvIn,EnvOutL,EnvOutR,EnvDrive;
	float drive;
	float driveL=0;
	float driveR=0;
	float fBypass,fAngleDelta,fRPhase,fMaxDelay2,fDepth,fNoClip;
	double fCycleDelta,fPhaseDelta,fWidthDelta,fDepthDelta;
	int   HasDelta;
	float fDelayL,fDelayOffsetL;
	float fDelayR,fDelayOffsetR;
	unsigned long lDelaySampleL;
	unsigned long lDelaySampleR;
	unsigned long lSampleIndex;
	unsigned long SpaceSize;
	float * SpaceLStr;
	float * SpaceRStr;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	IPhaser *plugin = (IPhaser *)instance;
	pParamFunc = &convertParam;

	/* check if any other params have changed */
	checkParamChange(IPHASER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IPHASER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fCycleDelta = getParamChange(IPHASER_CYCLE,  plugin->ControlCycle,  &(plugin->LastCycle),  &(plugin->ConvertedCycle),  plugin->SampleRate, pParamFunc);
	fPhaseDelta = getParamChange(IPHASER_PHASE,  plugin->ControlPhase,  &(plugin->LastPhase),  &(plugin->ConvertedPhase),  plugin->SampleRate, pParamFunc);
	fWidthDelta = getParamChange(IPHASER_WIDTH,  plugin->ControlWidth,  &(plugin->LastWidth),  &(plugin->ConvertedWidth),  plugin->SampleRate, pParamFunc);
	fDepthDelta = getParamChange(IPHASER_DEPTH,  plugin->ControlDepth,  &(plugin->LastDepth),  &(plugin->ConvertedDepth),  plugin->SampleRate, pParamFunc);


	fBypass         = plugin->ConvertedBypass;
	fNoClip		= plugin->ConvertedNoClip;

	if(fCycleDelta == 0 && fPhaseDelta==0 && fWidthDelta==0 && fDepthDelta==0) {
		HasDelta=0;
		fAngleDelta	= plugin->ConvertedCycle;
		fRPhase   	= plugin->ConvertedPhase;
		fMaxDelay2	= plugin->ConvertedWidth;
		fDepth		= plugin->ConvertedDepth;
	} else {
		HasDelta=1;
		fAngleDelta	= plugin->ConvertedCycle - fCycleDelta;
		fRPhase   	= plugin->ConvertedPhase - fPhaseDelta;
		fMaxDelay2	= plugin->ConvertedWidth - fWidthDelta;
		fDepth		= plugin->ConvertedDepth - fDepthDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fCycleDelta  = fCycleDelta/(float)SampleCount;
			fPhaseDelta  = fPhaseDelta/(float)SampleCount;
			fWidthDelta  = fWidthDelta/(float)SampleCount;
			fDepthDelta  = fDepthDelta/(float)SampleCount;
		}
	}

	SpaceSize 	= plugin->SpaceSize;

	SpaceLStr	= plugin->SpaceL;
	SpaceRStr 	= plugin->SpaceR;
	SpaceLCur 	= plugin->SpaceLCur;
	SpaceRCur 	= plugin->SpaceRCur;
	SpaceLEnd 	= plugin->SpaceLEnd;
	SpaceREnd 	= plugin->SpaceREnd;

	pfAudioInputL 	= plugin->AudioInputBufferL;
	pfAudioOutputL 	= plugin->AudioOutputBufferL;
	pfAudioOutputR 	= plugin->AudioOutputBufferR;

	fOffset		= plugin->Offset;
	fAngle		= plugin->AngleLast;
	EnvIn     	= plugin->EnvInLLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;
	EnvDrive   	= plugin->EnvDriveLast;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			In=*(pfAudioInputL++);

			fDelayL=fOffset + (fMaxDelay2 * ( 1 + cos(fAngle)));
			fDelayR=fOffset + (fMaxDelay2 * ( 1 + cos(fAngle+fRPhase)));

			fAngle += fAngleDelta;

			lDelaySampleL=(unsigned long)fDelayL;
			lDelaySampleR=(unsigned long)fDelayR;

			fDelayOffsetL=fDelayL-(float)lDelaySampleL;
			fDelayOffsetR=fDelayR-(float)lDelaySampleR;

			// subtract from the delay space
			SpaceSub(SpaceLCur, SpaceLEnd, SpaceSize, lDelaySampleL, fDelayOffsetL, fDepth*In);
			SpaceSub(SpaceRCur, SpaceREnd, SpaceSize, lDelaySampleR, fDelayOffsetR, fDepth*In);

			// read the audio out of the delay space
			OutL = In + *(SpaceLCur);
			OutR = In + *(SpaceRCur);
			OutL = fNoClip > 0 ? InoClip(OutL,&driveL) : OutL;
			OutR = fNoClip > 0 ? InoClip(OutR,&driveR) : OutR;
			*(pfAudioOutputL++) = OutL;
			*(pfAudioOutputR++) = OutR;
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;

			//evelope on in and out for meters
			EnvIn  		+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], In,   EnvIn);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR, EnvOutR);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fAngleDelta += fCycleDelta;
				fRPhase     += fPhaseDelta;
				fMaxDelay2  += fWidthDelta;
				fDepth	    += fDepthDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *pfAudioInputL;
			*(pfAudioOutputR++) = *(pfAudioInputL++);
			fAngle += fAngleDelta;
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero envelope on in and out for meters
		EnvIn   =0;
		EnvOutL =0;
		EnvOutR =0;
		EnvDrive=0;
	}

	while(fAngle > PI_2) {
		fAngle -= PI_2;
	}
	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;

	plugin->AngleLast   = fAngle; 
	plugin->EnvInLLast  = (fabs(EnvIn)<1.0e-10)   ? 0.f  : EnvIn; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->LampNoClip)=EnvDrive;
	*(plugin->LampLfoL) = 1.75*(1+(cos(fAngle)));
	*(plugin->LampLfoR) = 1.75*(1+(cos(fAngle+fRPhase)));
	*(plugin->MeterInputL) =(EnvIn   > 0.001) ? 20*log10(EnvIn)   : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}


static void 
runStereoIPhaser(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float fOffset;
	float fAngle;
	float InL,InR,OutL,OutR;
	float EnvInL,EnvInR,EnvOutL,EnvOutR,EnvDrive;
	float drive;
	float driveL=0;
	float driveR=0;
	float fBypass,fAngleDelta,fRPhase,fMaxDelay2,fDepth,fNoClip;
	double fCycleDelta,fPhaseDelta,fWidthDelta,fDepthDelta;
	int   HasDelta;
	float fDelayL,fDelayOffsetL;
	float fDelayR,fDelayOffsetR;
	unsigned long lDelaySampleL;
	unsigned long lDelaySampleR;
	unsigned long lSampleIndex;
	unsigned long SpaceSize;
	float * SpaceLStr;
	float * SpaceRStr;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	IPhaser *plugin = (IPhaser *)instance;
	pParamFunc = &convertParam;

	/* check if any other params have changed */
	checkParamChange(IPHASER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IPHASER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fCycleDelta = getParamChange(IPHASER_CYCLE,  plugin->ControlCycle,  &(plugin->LastCycle),  &(plugin->ConvertedCycle),  plugin->SampleRate, pParamFunc);
	fPhaseDelta = getParamChange(IPHASER_PHASE,  plugin->ControlPhase,  &(plugin->LastPhase),  &(plugin->ConvertedPhase),  plugin->SampleRate, pParamFunc);
	fWidthDelta = getParamChange(IPHASER_WIDTH,  plugin->ControlWidth,  &(plugin->LastWidth),  &(plugin->ConvertedWidth),  plugin->SampleRate, pParamFunc);
	fDepthDelta = getParamChange(IPHASER_DEPTH,  plugin->ControlDepth,  &(plugin->LastDepth),  &(plugin->ConvertedDepth),  plugin->SampleRate, pParamFunc);


	fBypass         = plugin->ConvertedBypass;
	fNoClip		= plugin->ConvertedNoClip;

	if(fCycleDelta == 0 && fPhaseDelta==0 && fWidthDelta==0 && fDepthDelta==0) {
		HasDelta=0;
		fAngleDelta	= plugin->ConvertedCycle;
		fRPhase   	= plugin->ConvertedPhase;
		fMaxDelay2	= plugin->ConvertedWidth;
		fDepth		= plugin->ConvertedDepth;
	} else {
		HasDelta=1;
		fAngleDelta	= plugin->ConvertedCycle - fCycleDelta;
		fRPhase   	= plugin->ConvertedPhase - fPhaseDelta;
		fMaxDelay2	= plugin->ConvertedWidth - fWidthDelta;
		fDepth		= plugin->ConvertedDepth - fDepthDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fCycleDelta  = fCycleDelta/(float)SampleCount;
			fPhaseDelta  = fPhaseDelta/(float)SampleCount;
			fWidthDelta  = fWidthDelta/(float)SampleCount;
			fDepthDelta  = fDepthDelta/(float)SampleCount;
		}
	}
	
	SpaceSize 	= plugin->SpaceSize;

	SpaceLStr	= plugin->SpaceL;
	SpaceRStr 	= plugin->SpaceR;
	SpaceLCur 	= plugin->SpaceLCur;
	SpaceRCur 	= plugin->SpaceRCur;
	SpaceLEnd 	= plugin->SpaceLEnd;
	SpaceREnd 	= plugin->SpaceREnd;

	pfAudioInputL 	= plugin->AudioInputBufferL;
	pfAudioInputR 	= plugin->AudioInputBufferR;
	pfAudioOutputL 	= plugin->AudioOutputBufferL;
	pfAudioOutputR 	= plugin->AudioOutputBufferR;

	fOffset		= plugin->Offset;
	fAngle		= plugin->AngleLast;
	EnvInL     	= plugin->EnvInLLast;
	EnvInR     	= plugin->EnvInRLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;
	EnvDrive   	= plugin->EnvDriveLast;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			InL=*(pfAudioInputL++);
			InR=*(pfAudioInputR++);

			fDelayL=fOffset + (fMaxDelay2 * ( 1 + cos(fAngle)));
			fDelayR=fOffset + (fMaxDelay2 * ( 1 + cos(fAngle+fRPhase)));

			fAngle += fAngleDelta;

			lDelaySampleL=(unsigned long)fDelayL;
			lDelaySampleR=(unsigned long)fDelayR;

			fDelayOffsetL=fDelayL-(float)lDelaySampleL;
			fDelayOffsetR=fDelayR-(float)lDelaySampleR;


			// subtract from the delay space
			SpaceSub(SpaceLCur, SpaceLEnd, SpaceSize, lDelaySampleL, fDelayOffsetL, fDepth*InL);
			SpaceSub(SpaceRCur, SpaceREnd, SpaceSize, lDelaySampleR, fDelayOffsetR, fDepth*InR);

			// read the audio out of the delay space
			OutL = InL + *(SpaceLCur);
			OutR = InR + *(SpaceRCur);
			OutL = fNoClip > 0 ? InoClip(OutL,&driveL) : OutL;
			OutR = fNoClip > 0 ? InoClip(OutR,&driveR) : OutR;
			*(pfAudioOutputL++) = OutL;
			*(pfAudioOutputR++) = OutR;
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;

			//evelope on in and out for meters
			EnvInL 		+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL,  EnvInL);
			EnvInR 		+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InR,  EnvInR);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR, EnvOutR);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fAngleDelta += fCycleDelta;
				fRPhase     += fPhaseDelta;
				fMaxDelay2  += fWidthDelta;
				fDepth	    += fDepthDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
			*(pfAudioOutputR++) = *(pfAudioInputR++);
			fAngle += fAngleDelta;
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero envelope on in and out for meters
		EnvInL  =0;
		EnvInR  =0;
		EnvOutL =0;
		EnvOutR =0;
		EnvDrive=0;
	}

	while(fAngle > PI_2) {
		fAngle -= PI_2;
	}
	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;

	plugin->AngleLast    = fAngle; 
	plugin->EnvInLLast   = (fabs(EnvInL)<1.0e-10)   ? 0.f : EnvInL; 
	plugin->EnvInRLast   = (fabs(EnvInR)<1.0e-10)   ? 0.f : EnvInR; 
	plugin->EnvOutLLast  = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast  = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10) ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->LampNoClip)=EnvDrive;
	*(plugin->LampLfoL) = 1.75*(1+(cos(fAngle)));
	*(plugin->LampLfoR) = 1.75*(1+(cos(fAngle+fRPhase)));
	*(plugin->MeterInputL) =(EnvInL  > 0.001) ? 20*log10(EnvInL)  : -90.0;
	*(plugin->MeterInputR) =(EnvInR  > 0.001) ? 20*log10(EnvInR)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}



static void 
runSumIPhaser(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float fOffset;
	float fAngle;
	float In,OutL,OutR;
	float EnvIn,EnvOutL,EnvOutR,EnvDrive;
	float drive;
	float driveL=0;
	float driveR=0;
	float fBypass,fAngleDelta,fRPhase,fMaxDelay2,fDepth,fNoClip;
	double fCycleDelta,fPhaseDelta,fWidthDelta,fDepthDelta;
	int   HasDelta;
	float fDelayL,fDelayOffsetL;
	float fDelayR,fDelayOffsetR;
	unsigned long lDelaySampleL;
	unsigned long lDelaySampleR;
	unsigned long lSampleIndex;
	unsigned long SpaceSize;
	float * SpaceLStr;
	float * SpaceRStr;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	IPhaser *plugin = (IPhaser *)instance;
	pParamFunc = &convertParam;

	/* check if any other params have changed */
	checkParamChange(IPHASER_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	checkParamChange(IPHASER_NOCLIP, plugin->ControlNoClip, &(plugin->LastNoClip), &(plugin->ConvertedNoClip), plugin->SampleRate, pParamFunc);
	fCycleDelta = getParamChange(IPHASER_CYCLE,  plugin->ControlCycle,  &(plugin->LastCycle),  &(plugin->ConvertedCycle),  plugin->SampleRate, pParamFunc);
	fPhaseDelta = getParamChange(IPHASER_PHASE,  plugin->ControlPhase,  &(plugin->LastPhase),  &(plugin->ConvertedPhase),  plugin->SampleRate, pParamFunc);
	fWidthDelta = getParamChange(IPHASER_WIDTH,  plugin->ControlWidth,  &(plugin->LastWidth),  &(plugin->ConvertedWidth),  plugin->SampleRate, pParamFunc);
	fDepthDelta = getParamChange(IPHASER_DEPTH,  plugin->ControlDepth,  &(plugin->LastDepth),  &(plugin->ConvertedDepth),  plugin->SampleRate, pParamFunc);


	fBypass         = plugin->ConvertedBypass;
	fNoClip		= plugin->ConvertedNoClip;

	if(fCycleDelta == 0 && fPhaseDelta==0 && fWidthDelta==0 && fDepthDelta==0) {
		HasDelta=0;
		fAngleDelta	= plugin->ConvertedCycle;
		fRPhase   	= plugin->ConvertedPhase;
		fMaxDelay2	= plugin->ConvertedWidth;
		fDepth		= plugin->ConvertedDepth;
	} else {
		HasDelta=1;
		fAngleDelta	= plugin->ConvertedCycle - fCycleDelta;
		fRPhase   	= plugin->ConvertedPhase - fPhaseDelta;
		fMaxDelay2	= plugin->ConvertedWidth - fWidthDelta;
		fDepth		= plugin->ConvertedDepth - fDepthDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fCycleDelta  = fCycleDelta/(float)SampleCount;
			fPhaseDelta  = fPhaseDelta/(float)SampleCount;
			fWidthDelta  = fWidthDelta/(float)SampleCount;
			fDepthDelta  = fDepthDelta/(float)SampleCount;
		}
	}
	
	SpaceSize 	= plugin->SpaceSize;

	SpaceLStr	= plugin->SpaceL;
	SpaceRStr 	= plugin->SpaceR;
	SpaceLCur 	= plugin->SpaceLCur;
	SpaceRCur 	= plugin->SpaceRCur;
	SpaceLEnd 	= plugin->SpaceLEnd;
	SpaceREnd 	= plugin->SpaceREnd;

	pfAudioInputL 	= plugin->AudioInputBufferL;
	pfAudioInputR 	= plugin->AudioInputBufferL;
	pfAudioOutputL 	= plugin->AudioOutputBufferL;
	pfAudioOutputR 	= plugin->AudioOutputBufferR;

	fOffset		= plugin->Offset;
	fAngle		= plugin->AngleLast;
	EnvIn     	= plugin->EnvInLLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;
	EnvDrive   	= plugin->EnvDriveLast;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			In=(*(pfAudioInputL++) + *(pfAudioInputR++))/2;

			fDelayL=fOffset + (fMaxDelay2 * ( 1 + cos(fAngle)));
			fDelayR=fOffset + (fMaxDelay2 * ( 1 + cos(fAngle+fRPhase)));

			fAngle += fAngleDelta;

			lDelaySampleL=(unsigned long)fDelayL;
			lDelaySampleR=(unsigned long)fDelayR;

			fDelayOffsetL=fDelayL-(float)lDelaySampleL;
			fDelayOffsetR=fDelayR-(float)lDelaySampleR;

			// subtract from the delay space
			SpaceSub(SpaceLCur, SpaceLEnd, SpaceSize, lDelaySampleL, fDelayOffsetL, fDepth*In);
			SpaceSub(SpaceRCur, SpaceREnd, SpaceSize, lDelaySampleR, fDelayOffsetR, fDepth*In);

			// read the audio out of the delay space
			OutL = In + *(SpaceLCur);
			OutR = In + *(SpaceRCur);
			OutL = fNoClip > 0 ? InoClip(OutL,&driveL) : OutL;
			OutR = fNoClip > 0 ? InoClip(OutR,&driveR) : OutR;
			*(pfAudioOutputL++) = OutL;
			*(pfAudioOutputR++) = OutR;
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;

			//evelope on in and out for meters
			EnvIn  		+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], In,   EnvIn);
			EnvOutL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutL, EnvOutL);
			EnvOutR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], OutR, EnvOutR);

			drive = driveL > driveR ? driveL : driveR;
			EnvDrive  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP], drive,   EnvDrive);

			//update any changing parameters
			if(HasDelta==1) {
				fAngleDelta += fCycleDelta;
				fRPhase     += fPhaseDelta;
				fMaxDelay2  += fWidthDelta;
				fDepth	    += fDepthDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *pfAudioInputL;
			*(pfAudioOutputR++) = *(pfAudioInputL++);
			fAngle += fAngleDelta;
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero envelope on in and out for meters
		EnvIn   =0;
		EnvOutL =0;
		EnvOutR =0;
		EnvDrive=0;
	}

	while(fAngle > PI_2) {
		fAngle -= PI_2;
	}
	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;

	plugin->AngleLast   = fAngle; 
	plugin->EnvInLLast  = (fabs(EnvIn)<1.0e-10)   ? 0.f  : EnvIn; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 
	plugin->EnvDriveLast = (fabs(EnvDrive)<1.0e-10)  ? 0.f : EnvDrive; 

	// update the meters
	*(plugin->LampNoClip)=EnvDrive;
	*(plugin->LampLfoL) = 1.75*(1+(cos(fAngle)));
	*(plugin->LampLfoR) = 1.75*(1+(cos(fAngle+fRPhase)));
	*(plugin->MeterInputL) =(EnvIn   > 0.001) ? 20*log10(EnvIn)   : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}



static void 
cleanupIPhaser(LV2_Handle instance)
{
	IPhaser *plugin = (IPhaser *)instance;

	free(plugin->SpaceL);
	free(plugin->SpaceR);
	free(instance);
}


static void 
init()
{
	IPhaserMonoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IPhaserMonoDescriptor->URI 		= IPHASER_MONO_URI;
	IPhaserMonoDescriptor->activate 	= activateIPhaser;
	IPhaserMonoDescriptor->cleanup 		= cleanupIPhaser;
	IPhaserMonoDescriptor->connect_port 	= connectPortIPhaser;
	IPhaserMonoDescriptor->deactivate 	= NULL;
	IPhaserMonoDescriptor->instantiate 	= instantiateIPhaser;
	IPhaserMonoDescriptor->run 		= runMonoIPhaser;
	IPhaserMonoDescriptor->extension_data	= NULL;

	IPhaserStereoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IPhaserStereoDescriptor->URI 		= IPHASER_STEREO_URI;
	IPhaserStereoDescriptor->activate 	= activateIPhaser;
	IPhaserStereoDescriptor->cleanup 	= cleanupIPhaser;
	IPhaserStereoDescriptor->connect_port 	= connectPortIPhaser;
	IPhaserStereoDescriptor->deactivate 	= NULL;
	IPhaserStereoDescriptor->instantiate 	= instantiateIPhaser;
	IPhaserStereoDescriptor->run 		= runStereoIPhaser;
	IPhaserStereoDescriptor->extension_data	= NULL;

	IPhaserSumDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IPhaserSumDescriptor->URI 		= IPHASER_SUM_URI;
	IPhaserSumDescriptor->activate 		= activateIPhaser;
	IPhaserSumDescriptor->cleanup 		= cleanupIPhaser;
	IPhaserSumDescriptor->connect_port 	= connectPortIPhaser;
	IPhaserSumDescriptor->deactivate 	= NULL;
	IPhaserSumDescriptor->instantiate 	= instantiateIPhaser;
	IPhaserSumDescriptor->run 		= runSumIPhaser;
	IPhaserSumDescriptor->extension_data	= NULL;

}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IPhaserMonoDescriptor) init();

	switch (index) {
		case 0:
			return IPhaserMonoDescriptor;
		case 1:
			return IPhaserStereoDescriptor;
		case 2:
			return IPhaserSumDescriptor;

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
		case IPHASER_BYPASS:
		case IPHASER_NOCLIP:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		case IPHASER_CYCLE:
			if (value < 0.5)
				result = PI_2/(0.5*sr);
			else if (value <= 500.0)
				result = PI_2/(value * sr);
			else
				result = PI_2/(500.0 * sr);
			break;
		case IPHASER_PHASE:
			if(value<-180)
				result = -PI;
			else if (value < 180)
				result = value*PI/180.0;
			else
				result = PI;
			break;
		case IPHASER_WIDTH:
			if(value<1)
				result = sr/2000.0;
			else if (value < 15)
				result = value*sr/2000.0;
			else
				result = 15.0*sr/2000.0;
			break;
		case IPHASER_DEPTH:
			if(value<0)
				result = 0.0;
			else if (value < 100)
				result = value/100.0;
			else
				result = 1.0;
			break;
		default:
			result=0;
			break;
	}
	return result;
}

