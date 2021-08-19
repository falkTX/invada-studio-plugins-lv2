/* 

    This LV2 plugin provides munged delay plugins

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
#include <stdio.h> 
#include <string.h>
#include <math.h>
#include <lv2.h>
#include "library/common.h"
#include "inv_delay.h"

static LV2_Descriptor *IDelayMonoDescriptor = NULL;
static LV2_Descriptor *IDelaySumDescriptor = NULL;



typedef struct {

	/* Ports */
	float *ControlBypass;
	float *ControlMode;
	float *ControlMungeMode;
	float *ControlMunge;
	float *ControlCycle;
	float *ControlWidth;
	float *Control1Delay;
	float *Control1FB; 
	float *Control1Pan;
	float *Control1Vol;
	float *Control2Delay;
	float *Control2FB; 
	float *Control2Pan;
	float *Control2Vol;

	float *AudioOutputBufferL;
	float *AudioOutputBufferR;
	float *AudioInputBufferL;
	float *AudioInputBufferR; 

	float *LampLFO;
	float *MeterInputL;
	float *MeterOutputL;
	float *MeterOutputR;

	double SampleRate;

	struct Envelope EnvAD[4];

	/* Stuff to remember to avoid recalculating the delays every run */
	float LastBypass;
	float LastMode;
	float LastMungeMode;
	float LastMunge;
	float LastCycle;
	float LastWidth;
	float Last1Delay;
	float Last1FB; 
	float Last1Pan;
	float Last1Vol;
	float Last2Delay;
	float Last2FB; 
	float Last2Pan;
	float Last2Vol;


	float LFOAngle; 
	float AudioLPF1; 
	float AudioLPF2; 
	float AudioHPF1; 
	float AudioHPF2; 
	float AudioDegrain1; 
	float AudioDegrain2; 
	float EnvInLLast; 
	float EnvOutLLast; 
	float EnvOutRLast;

	float ConvertedBypass; 
	float ConvertedMode; 
	float ConvertedMungeMode;
	float ConvertedMunge;
	float ConvertedCycle;
	float ConvertedWidth;
	float ConvertedLPFsamples;
	float ConvertedHPFsamples;
	float Converted1Delay;
	float Converted1FB; 
	float Converted1Pan;
	float Converted1Vol;
	float Converted2Delay;
	float Converted2FB; 
	float Converted2Pan;
	float Converted2Vol;


	/* Delay Space Data */
	unsigned long SpaceSize;
	float * SpaceL;
	float * SpaceR;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

} IDelay;


static LV2_Handle 
instantiateIDelay(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IDelay *plugin = (IDelay *)malloc(sizeof(IDelay));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;
	plugin->SpaceSize = IDELAY_SPACE_SIZE * s_rate / 1000;

	/* the delay space */
	if((plugin->SpaceL  = (float *)malloc(sizeof(float) * plugin->SpaceSize))==NULL)
    		return NULL;
	
	if((plugin->SpaceR  = (float *)malloc(sizeof(float) * plugin->SpaceSize))==NULL)
    		return NULL;

	return (LV2_Handle)plugin;
}

static void 
connectPortIDelay(LV2_Handle instance, uint32_t port, void *data)
{
	IDelay *plugin = (IDelay *)instance;
	switch (port) {
		case IDELAY_BYPASS:
			plugin->ControlBypass = data;
			break;
		case IDELAY_MODE:
			plugin->ControlMode = data;
			break;
		case IDELAY_MUNGEMODE:
			plugin->ControlMungeMode = data;
			break;
		case IDELAY_MUNGE:
			plugin->ControlMunge = data;
			break;
		case IDELAY_LFO_CYCLE:
			plugin->ControlCycle = data;
			break;
		case IDELAY_LFO_WIDTH:
			plugin->ControlWidth = data;
			break;
		case IDELAY_1_DELAY:
			plugin->Control1Delay = data;
			break;
		case IDELAY_1_FB:
			plugin->Control1FB = data;
			break;
		case IDELAY_1_PAN:
			plugin->Control1Pan = data;
			break;
		case IDELAY_1_VOL:
			plugin->Control1Vol = data;
			break;
		case IDELAY_2_DELAY:
			plugin->Control2Delay = data;
			break;
		case IDELAY_2_FB:
			plugin->Control2FB = data;
			break;
		case IDELAY_2_PAN:
			plugin->Control2Pan = data;
			break;
		case IDELAY_2_VOL:
			plugin->Control2Vol = data;
			break;
		case IDELAY_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case IDELAY_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case IDELAY_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case IDELAY_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case IDELAY_METER_INL:
			plugin->MeterInputL = data;
			break;
		case IDELAY_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case IDELAY_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
		case IDELAY_LAMP_LFO:
			plugin->LampLFO = data;
			break;
	}
}


static void 
activateIDelay(LV2_Handle instance) 
{
	IDelay *plugin = (IDelay *)instance;

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
	plugin->LastMode	= 0.0;
	plugin->LastMungeMode	= 0.0;
	plugin->LastMunge	= 50.0;
	plugin->LastCycle	= 20.0;
	plugin->LastWidth	= 0.0;
	plugin->Last1Delay	= 0.3;
	plugin->Last1FB		= 50.0; 
	plugin->Last1Pan	= -0.7;
	plugin->Last1Vol	= 100.0;
	plugin->Last2Delay	= 0.2;
	plugin->Last2FB		= 50.0; 
	plugin->Last2Pan	= 0.7;
	plugin->Last2Vol	= 100.0;

	plugin->LFOAngle 	= 0; 
	plugin->AudioLPF1 	= 0; 
	plugin->AudioLPF2 	= 0;  
	plugin->AudioHPF1 	= 0; 
	plugin->AudioHPF2 	= 0; 
	plugin->AudioDegrain1 	= 0; 
	plugin->AudioDegrain2 	= 0; 
	plugin->EnvInLLast 	= 0; 
	plugin->EnvOutLLast 	= 0; 
	plugin->EnvOutRLast 	= 0; 

	plugin->ConvertedBypass     = convertParam(IDELAY_BYPASS,    plugin->LastBypass,    plugin->SampleRate);  
	plugin->ConvertedMode       = convertParam(IDELAY_MODE,	     plugin->LastMode,      plugin->SampleRate);  
	plugin->ConvertedMungeMode  = convertParam(IDELAY_MUNGEMODE, plugin->LastMungeMode, plugin->SampleRate);  
	plugin->ConvertedMunge      = convertParam(IDELAY_MUNGE,     plugin->LastMunge,     plugin->SampleRate); 
	plugin->ConvertedLPFsamples = convertMunge(0,                plugin->LastMunge,     plugin->SampleRate); 
	plugin->ConvertedHPFsamples = convertMunge(1,                plugin->LastMunge,     plugin->SampleRate);  
	plugin->ConvertedCycle      = convertParam(IDELAY_LFO_CYCLE, plugin->LastCycle,     plugin->SampleRate);  
	plugin->ConvertedWidth      = convertParam(IDELAY_LFO_WIDTH, plugin->LastWidth,     plugin->SampleRate); 
	plugin->Converted1Delay     = convertParam(IDELAY_1_DELAY,   plugin->Last1Delay,    plugin->SampleRate);  
	plugin->Converted1FB        = convertParam(IDELAY_1_FB,      plugin->Last1FB,       plugin->SampleRate);  
	plugin->Converted1Pan       = convertParam(IDELAY_1_PAN,     plugin->Last1Pan,      plugin->SampleRate);  
	plugin->Converted1Vol       = convertParam(IDELAY_1_VOL,     plugin->Last1Vol,      plugin->SampleRate);  
	plugin->Converted2Delay     = convertParam(IDELAY_2_DELAY,   plugin->Last2Delay,    plugin->SampleRate);  
	plugin->Converted2FB        = convertParam(IDELAY_2_FB,      plugin->Last2FB,       plugin->SampleRate); 
	plugin->Converted2Pan       = convertParam(IDELAY_2_PAN,     plugin->Last2Pan,      plugin->SampleRate); 
	plugin->Converted2Vol       = convertParam(IDELAY_2_VOL,     plugin->Last2Vol,      plugin->SampleRate); 

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void 
runMonoIDelay(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float * pfAudioOutputR;	
	float In,Out1,Out2,OutL,OutR;
	float LPF1,LPF2,HPF1,HPF2,Degrain1,Degrain2;
	float EnvIn,EnvOutL,EnvOutR;
	float fBypass,fMode,fMunge,fMungeMode,fHPFsamples,fLPFsamples,fAngle,fAngleDelta,fLFO;
	float f1Delay,f1DelayOffset,fLFOsamp1,fActualDelay1;
	float f2Delay,f2DelayOffset,fLFOsamp2,fActualDelay2;
	float f1DelayDelta,f1DelayOld,f1DelayOffsetOld,fLFOsamp1Old,fActualDelay1Old;
	float f2DelayDelta,f2DelayOld,f2DelayOffsetOld,fLFOsamp2Old,fActualDelay2Old;
	float oldVol,newVol;
	float f1Vol,f1Pan,f1PanLGain,f1PanRGain,f1FBraw,f1FB,In1FB,In1FBmix,In1Munged;
	float f2Vol,f2Pan,f2PanLGain,f2PanRGain,f2FBraw,f2FB,In2FB,In2FBmix,In2Munged;
	double fMungeDelta,fLPFDelta,fHPFDelta,fCycleDelta,fWidthDelta,fFB1Delta,fPan1Delta,fVol1Delta,fFB2Delta,fPan2Delta,fVol2Delta;
	int   HasDelta,HasDelay1Old,HasDelay2Old;
	unsigned long l1DelaySample, l2DelaySample;
	unsigned long l1DelaySampleOld, l2DelaySampleOld;
	unsigned long lSampleIndex;
	unsigned long SpaceSize;
	float * SpaceLStr;
	float * SpaceRStr;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	IDelay *plugin = (IDelay *)instance;
	pParamFunc = &convertParam;

	/* check if any other params have changed */
	checkParamChange(IDELAY_BYPASS,    plugin->ControlBypass,    &(plugin->LastBypass),    &(plugin->ConvertedBypass),    plugin->SampleRate, pParamFunc);
	checkParamChange(IDELAY_MODE,      plugin->ControlMode,      &(plugin->LastMode),      &(plugin->ConvertedMode),      plugin->SampleRate, pParamFunc);
	checkParamChange(IDELAY_MUNGEMODE, plugin->ControlMungeMode, &(plugin->LastMungeMode), &(plugin->ConvertedMungeMode), plugin->SampleRate, pParamFunc);

	fMungeDelta  = getParamChange(IDELAY_MUNGE,     plugin->ControlMunge,  &(plugin->LastMunge),  &(plugin->ConvertedMunge),  plugin->SampleRate, pParamFunc);
	fCycleDelta  = getParamChange(IDELAY_LFO_CYCLE, plugin->ControlCycle,  &(plugin->LastCycle),  &(plugin->ConvertedCycle),  plugin->SampleRate, pParamFunc);
	fWidthDelta  = getParamChange(IDELAY_LFO_WIDTH, plugin->ControlWidth,  &(plugin->LastWidth),  &(plugin->ConvertedWidth),  plugin->SampleRate, pParamFunc);
	f1DelayDelta = getParamChange(IDELAY_1_DELAY,   plugin->Control1Delay, &(plugin->Last1Delay), &(plugin->Converted1Delay), plugin->SampleRate, pParamFunc);
	fFB1Delta    = getParamChange(IDELAY_1_FB,      plugin->Control1FB,    &(plugin->Last1FB),    &(plugin->Converted1FB),    plugin->SampleRate, pParamFunc);
	fPan1Delta   = getParamChange(IDELAY_1_PAN,     plugin->Control1Pan,   &(plugin->Last1Pan),   &(plugin->Converted1Pan),   plugin->SampleRate, pParamFunc);
	fVol1Delta   = getParamChange(IDELAY_1_VOL,     plugin->Control1Vol,   &(plugin->Last1Vol),   &(plugin->Converted1Vol),   plugin->SampleRate, pParamFunc);
	f2DelayDelta = getParamChange(IDELAY_2_DELAY,   plugin->Control2Delay, &(plugin->Last2Delay), &(plugin->Converted2Delay), plugin->SampleRate, pParamFunc);
	fFB2Delta    = getParamChange(IDELAY_2_FB,      plugin->Control2FB,    &(plugin->Last2FB),    &(plugin->Converted2FB),    plugin->SampleRate, pParamFunc);
	fPan2Delta   = getParamChange(IDELAY_2_PAN,     plugin->Control2Pan,   &(plugin->Last2Pan),   &(plugin->Converted2Pan),   plugin->SampleRate, pParamFunc);
	fVol2Delta   = getParamChange(IDELAY_2_VOL,     plugin->Control2Vol,   &(plugin->Last2Vol),   &(plugin->Converted2Vol),   plugin->SampleRate, pParamFunc);

	fBypass         = plugin->ConvertedBypass;
	fMode           = plugin->ConvertedMode;
	fMungeMode	= plugin->ConvertedMungeMode;

	if(fMungeDelta==0 && fCycleDelta == 0 && fWidthDelta==0 && fFB1Delta==0 && fFB2Delta==0 && fPan1Delta==0 && fPan2Delta==0 && fVol1Delta==0 && fVol2Delta==0) {
		HasDelta=0;
		fMunge		= plugin->ConvertedMunge;
		fLPFDelta	= 0.0;
		fHPFDelta	= 0.0;
		fLPFsamples	= plugin->ConvertedLPFsamples;
		fHPFsamples	= plugin->ConvertedHPFsamples;
		fAngleDelta	= plugin->ConvertedCycle;
		fLFO		= plugin->ConvertedWidth;
		f1FBraw		= plugin->Converted1FB;
		f1Pan		= plugin->Converted1Pan;
		f1Vol		= plugin->Converted1Vol;
		f2FBraw		= plugin->Converted2FB;
		f2Pan		= plugin->Converted2Pan;
		f2Vol		= plugin->Converted2Vol;
	} else {
		HasDelta	= 1;
		fMunge		= plugin->ConvertedMunge - fMungeDelta;
		if(fMungeDelta == 0) {
			fLPFDelta			= 0.0;
			fHPFDelta			= 0.0;
		} else {
			// this is a bit wonky i know but creating varibles for the old values and then calculating the delta with new-old doesn't save any cpu
			fLPFDelta			= -plugin->ConvertedLPFsamples;
			fHPFDelta			= -plugin->ConvertedHPFsamples;
			plugin->ConvertedLPFsamples	= convertMunge(0, plugin->LastMunge, plugin->SampleRate); 
			plugin->ConvertedHPFsamples	= convertMunge(1, plugin->LastMunge, plugin->SampleRate); 
			fLPFDelta			+= plugin->ConvertedLPFsamples;
			fHPFDelta			+= plugin->ConvertedHPFsamples;
		}
		fLPFsamples	= plugin->ConvertedLPFsamples	- fLPFDelta;
		fHPFsamples	= plugin->ConvertedHPFsamples 	- fHPFDelta;
		fAngleDelta	= plugin->ConvertedCycle 	- fCycleDelta;
		fLFO		= plugin->ConvertedWidth 	- fWidthDelta;
		f1FBraw		= plugin->Converted1FB   	- fFB1Delta;
		f1Pan		= plugin->Converted1Pan  	- fPan1Delta;
		f1Vol		= plugin->Converted1Vol  	- fVol1Delta;
		f2FBraw		= plugin->Converted2FB   	- fFB2Delta;
		f2Pan		= plugin->Converted2Pan  	- fPan2Delta;
		f2Vol		= plugin->Converted2Vol  	- fVol2Delta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fLPFDelta    = fLPFDelta/(float)SampleCount;
			fHPFDelta    = fHPFDelta/(float)SampleCount;
			fMungeDelta  = fMungeDelta/(float)SampleCount;
			fCycleDelta  = fCycleDelta/(float)SampleCount;
			fWidthDelta  = fWidthDelta/(float)SampleCount;
			fFB1Delta    = fFB1Delta/(float)SampleCount;
			fPan1Delta   = fPan1Delta/(float)SampleCount;
			fVol1Delta   = fVol1Delta/(float)SampleCount;
			fFB2Delta    = fFB2Delta/(float)SampleCount;
			fPan2Delta   = fPan2Delta/(float)SampleCount;
			fVol2Delta   = fVol2Delta/(float)SampleCount;
		}
	}

	if(f1DelayDelta==0.0) {
		HasDelay1Old	= 0;
		f1Delay		= plugin->Converted1Delay;
		l1DelaySample	= (unsigned long)f1Delay;
		f1DelayOffset	= f1Delay-(float)l1DelaySample;
		fLFOsamp1	= fLFO * f1Delay;
		f1DelayOld	= 0.0;
		l1DelaySampleOld= 0;
		f1DelayOffsetOld= 0.0;
		fLFOsamp1Old	= 0.0;
	} else {
		HasDelay1Old	= 1;
		f1Delay		= plugin->Converted1Delay;
		l1DelaySample	= (unsigned long)f1Delay;
		f1DelayOffset	= f1Delay-(float)l1DelaySample;
		fLFOsamp1	= fLFO * f1Delay;
		f1DelayOld	= plugin->Converted1Delay-f1DelayDelta;
		l1DelaySampleOld= (unsigned long)f1DelayOld;
		f1DelayOffsetOld= f1DelayOld-(float)l1DelaySampleOld;
		fLFOsamp1Old	= fLFO * f1DelayOld;
	}

	if(f2DelayDelta==0.0) {
		HasDelay2Old	= 0;
		f2Delay		= plugin->Converted2Delay;
		l2DelaySample	= (unsigned long)f2Delay;
		f2DelayOffset	= f2Delay-(float)l2DelaySample;
		fLFOsamp2	= fLFO * f2Delay;
		f2DelayOld	= 0.0;
		l2DelaySampleOld= 0;
		f2DelayOffsetOld= 0.0;
		fLFOsamp2Old	= 0.0;
	} else {
		HasDelay2Old	= 1;
		f2Delay		= plugin->Converted2Delay;
		l2DelaySample	= (unsigned long)f2Delay;
		f2DelayOffset	= f2Delay-(float)l2DelaySample;
		fLFOsamp2	= fLFO * f2Delay;
		f2DelayOld	= plugin->Converted2Delay-f2DelayDelta;
		l2DelaySampleOld= (unsigned long)f2DelayOld;
		f2DelayOffsetOld= f2DelayOld-(float)l2DelaySampleOld;
		fLFOsamp2Old	= fLFO * f2DelayOld;
	}
	oldVol=0.0;
	newVol=1.0;

	f1PanLGain	= f1Vol * (1-f1Pan)/2;
	f1PanRGain	= f1Vol * (1+f1Pan)/2;
	f2PanLGain	= f2Vol * (1-f2Pan)/2;
	f2PanRGain	= f2Vol * (1+f2Pan)/2;
	if(fMungeMode < 0.5) {
		f1FB	= f1FBraw / (1+fMunge);
		f2FB	= f2FBraw / (1+fMunge);
	} else {
		f1FB	= f1FBraw * pow(2.0,-2.0*fMunge);
		f2FB	= f2FBraw * pow(2.0,-2.0*fMunge);
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

	fAngle     	= plugin->LFOAngle;
	LPF1     	= plugin->AudioLPF1;
	LPF2     	= plugin->AudioLPF2;
	HPF1     	= plugin->AudioHPF1;
	HPF2     	= plugin->AudioHPF2;
	Degrain1     	= plugin->AudioDegrain1;
	Degrain2     	= plugin->AudioDegrain2;
	EnvIn     	= plugin->EnvInLLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			if(HasDelay1Old==1 || HasDelay2Old==1) {
				newVol = (float)lSampleIndex/(float)SampleCount;               // 0 -> 1
				oldVol = (float)(SampleCount-lSampleIndex)/(float)SampleCount; // 1 -> 0
			}

			// read the audio out of the delay space
			Out1 = *(SpaceLCur);
			Out2 = *(SpaceRCur);

			// read the input
			In = *(pfAudioInputL++);

			// and mix the feedback in
			if(fMode<0.5 ) {
				// normal
				In1FBmix= In+(f1FB*Out1);
				In2FBmix= In+(f2FB*Out2);
			} else {
 				//ping pong
				In1FBmix= In+(f2FB*Out2);
				In2FBmix= In+(f1FB*Out1);
			}

			// munge it
			In1FB	= (1-fMunge)*In1FBmix + fMunge*ITube_do(In1FBmix,1+fMunge);
			In2FB	= (1-fMunge)*In2FBmix + fMunge*ITube_do(In2FBmix,1+fMunge);
			HPF1 	= ((fHPFsamples-1) * HPF1 + In1FB) / fHPFsamples;  
			HPF2 	= ((fHPFsamples-1) * HPF2 + In2FB) / fHPFsamples; 
			LPF1 	= ((fLPFsamples-1) * LPF1 + (In1FB-HPF1)) / fLPFsamples; 
			LPF2 	= ((fLPFsamples-1) * LPF2 + (In2FB-HPF2)) / fLPFsamples;  
			if(fMungeMode<0.5) {
				In1Munged = LPF1;
				In2Munged = LPF2;
			} else {
				In1Munged = 2 * (In1FB-HPF1) - LPF1;
				In2Munged = 2 * (In2FB-HPF2) - LPF2;
			} 
			Degrain1     	= (In1Munged+(2*Degrain1))/3;
			Degrain2     	= (In2Munged+(2*Degrain2))/3;

			// LFO
			if(fLFO > 0) {
				fActualDelay1	= f1Delay + (fLFOsamp1 * cos(fAngle));
				l1DelaySample	= (unsigned long)fActualDelay1;
				f1DelayOffset	= fActualDelay1-(float)l1DelaySample;

				fActualDelay2	= f2Delay + (fLFOsamp2 * cos(fAngle));
				l2DelaySample	= (unsigned long)fActualDelay2;
				f2DelayOffset	= fActualDelay2-(float)l2DelaySample;

				if(HasDelay1Old==1) {
					fActualDelay1Old	= f1DelayOld + (fLFOsamp1Old * cos(fAngle));
					l1DelaySampleOld	= (unsigned long)fActualDelay1Old;
					f1DelayOffsetOld	= fActualDelay1Old-(float)l1DelaySampleOld;
				}
				if(HasDelay2Old==1) {
					fActualDelay2Old	= f2DelayOld + (fLFOsamp2Old * cos(fAngle));
					l2DelaySampleOld	= (unsigned long)fActualDelay2Old;
					f2DelayOffsetOld	= fActualDelay2Old-(float)l2DelaySampleOld;
				}

				fAngle += fAngleDelta;
			}

			// add to the delay space
			if(HasDelay1Old==0) {
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l1DelaySample,    f1DelayOffset,    Degrain1);
			} else {
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l1DelaySample,    f1DelayOffset,    newVol*Degrain1);
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l1DelaySampleOld, f1DelayOffsetOld, oldVol*Degrain1);

			}

			if(HasDelay2Old==0) {
				SpaceAdd(SpaceRCur, SpaceREnd, SpaceSize, l2DelaySample,    f2DelayOffset,    Degrain2);
			} else {
				SpaceAdd(SpaceRCur, SpaceREnd, SpaceSize, l2DelaySample,    f2DelayOffset,    newVol*Degrain2);
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l2DelaySampleOld, f2DelayOffsetOld, oldVol*Degrain2);

			}

			// mix the two delays in
			OutL = f1PanLGain*Out1 + f2PanLGain*Out2;
			OutR = f1PanRGain*Out1 + f2PanRGain*Out2;
			// write the output
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

			//update any changing parameters
			if(HasDelta==1) {
				fMunge	    += fMungeDelta;
				fLPFsamples += fLPFDelta;
				fHPFsamples += fHPFDelta;
				fAngleDelta += fCycleDelta;
				fLFO	    += fWidthDelta;
				f1FBraw	    += fFB1Delta;
				f1Pan	    += fPan1Delta;
				f1Vol	    += fVol1Delta;
				f2FBraw	    += fFB2Delta;
				f2Pan	    += fPan2Delta;
				f2Vol	    += fVol2Delta;

				fLFOsamp1	= fLFO * f1Delay;
				fLFOsamp2	= fLFO * f2Delay;
				f1PanLGain	= f1Vol * (1-f1Pan)/2;
				f1PanRGain	= f1Vol * (1+f1Pan)/2;
				f2PanLGain	= f2Vol * (1-f2Pan)/2;
				f2PanRGain	= f2Vol * (1+f2Pan)/2;

				if(fMungeMode < 0.5) {
					f1FB	= f1FBraw / (1+fMunge);
					f2FB	= f2FBraw / (1+fMunge);
				} else {
					f1FB	= f1FBraw * pow(2.0,-2.0*fMunge);
					f2FB	= f2FBraw * pow(2.0,-2.0*fMunge);
				}

				if(HasDelay1Old==1) {
					fLFOsamp1Old	= fLFO * f1DelayOld;
				}

				if(HasDelay2Old==1) {
					fLFOsamp2Old	= fLFO * f2DelayOld;
				}
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *pfAudioInputL;
			*(pfAudioOutputR++) = *(pfAudioInputL++);
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero filters
		LPF1    =0;
		LPF2    =0;
		HPF1    =0;
		HPF2    =0;
		Degrain1    =0;
		Degrain2    =0;
		//zero envelope on in and out for meters
		EnvIn   =0;
		EnvOutL =0;
		EnvOutR =0;
	}
	if(fLFO > 0) {
		while(fAngle > PI_2) {
			fAngle -= PI_2;
		}
	} else {
		fAngle=0;
	}

	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;

	plugin->LFOAngle    = fAngle; 
	plugin->AudioLPF1   = (fabs(LPF1)<1.0e-10)     ? 0.f : LPF1; 
	plugin->AudioLPF2   = (fabs(LPF2)<1.0e-10)     ? 0.f : LPF2; 
	plugin->AudioHPF1   = (fabs(HPF1)<1.0e-10)     ? 0.f : HPF1; 
	plugin->AudioHPF2   = (fabs(HPF2)<1.0e-10)     ? 0.f : HPF2; 
	plugin->AudioDegrain1   = (fabs(Degrain1)<1.0e-10)     ? 0.f : Degrain1; 
	plugin->AudioDegrain2   = (fabs(Degrain2)<1.0e-10)     ? 0.f : Degrain2; 
	plugin->EnvInLLast  = (fabs(EnvIn)<1.0e-10)    ? 0.f : EnvIn; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 

	// update the meters
	*(plugin->LampLFO)      = 1.75*(1-(cos(fAngle)));
	*(plugin->MeterInputL)  = (EnvIn   > 0.001) ? 20*log10(EnvIn)   : -90.0;
	*(plugin->MeterOutputL) = (EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR) = (EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}




static void 
runSumIDelay(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;	
	float In,Out1,Out2,OutL,OutR;
	float LPF1,LPF2,HPF1,HPF2,Degrain1,Degrain2;
	float EnvIn,EnvOutL,EnvOutR;
	float fBypass,fMode,fMunge,fMungeMode,fHPFsamples,fLPFsamples,fAngle,fAngleDelta,fLFO;
	float f1Delay,f1DelayOffset,fLFOsamp1,fActualDelay1;
	float f2Delay,f2DelayOffset,fLFOsamp2,fActualDelay2;
	float f1DelayDelta,f1DelayOld,f1DelayOffsetOld,fLFOsamp1Old,fActualDelay1Old;
	float f2DelayDelta,f2DelayOld,f2DelayOffsetOld,fLFOsamp2Old,fActualDelay2Old;
	float oldVol,newVol;
	float f1Vol,f1Pan,f1PanLGain,f1PanRGain,f1FBraw,f1FB,In1FB,In1FBmix,In1Munged;
	float f2Vol,f2Pan,f2PanLGain,f2PanRGain,f2FBraw,f2FB,In2FB,In2FBmix,In2Munged;
	double fMungeDelta,fLPFDelta,fHPFDelta,fCycleDelta,fWidthDelta,fFB1Delta,fPan1Delta,fVol1Delta,fFB2Delta,fPan2Delta,fVol2Delta;
	int   HasDelta,HasDelay1Old,HasDelay2Old;
	unsigned long l1DelaySample, l2DelaySample;
	unsigned long l1DelaySampleOld, l2DelaySampleOld;
	unsigned long lSampleIndex;
	unsigned long SpaceSize;

	float * SpaceLStr;
	float * SpaceRStr;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	IDelay *plugin = (IDelay *)instance;
	pParamFunc = &convertParam;

	/* check if any other params have changed */
	checkParamChange(IDELAY_BYPASS,    plugin->ControlBypass,    &(plugin->LastBypass),    &(plugin->ConvertedBypass),    plugin->SampleRate, pParamFunc);
	checkParamChange(IDELAY_MODE,      plugin->ControlMode,      &(plugin->LastMode),      &(plugin->ConvertedMode),      plugin->SampleRate, pParamFunc);
	checkParamChange(IDELAY_MUNGEMODE, plugin->ControlMungeMode, &(plugin->LastMungeMode), &(plugin->ConvertedMungeMode), plugin->SampleRate, pParamFunc);

	fMungeDelta  = getParamChange(IDELAY_MUNGE,     plugin->ControlMunge,  &(plugin->LastMunge),  &(plugin->ConvertedMunge),  plugin->SampleRate, pParamFunc);
	fCycleDelta  = getParamChange(IDELAY_LFO_CYCLE, plugin->ControlCycle,  &(plugin->LastCycle),  &(plugin->ConvertedCycle),  plugin->SampleRate, pParamFunc);
	fWidthDelta  = getParamChange(IDELAY_LFO_WIDTH, plugin->ControlWidth,  &(plugin->LastWidth),  &(plugin->ConvertedWidth),  plugin->SampleRate, pParamFunc);
	f1DelayDelta = getParamChange(IDELAY_1_DELAY,   plugin->Control1Delay, &(plugin->Last1Delay), &(plugin->Converted1Delay), plugin->SampleRate, pParamFunc);
	fFB1Delta    = getParamChange(IDELAY_1_FB,      plugin->Control1FB,    &(plugin->Last1FB),    &(plugin->Converted1FB),    plugin->SampleRate, pParamFunc);
	fPan1Delta   = getParamChange(IDELAY_1_PAN,     plugin->Control1Pan,   &(plugin->Last1Pan),   &(plugin->Converted1Pan),   plugin->SampleRate, pParamFunc);
	fVol1Delta   = getParamChange(IDELAY_1_VOL,     plugin->Control1Vol,   &(plugin->Last1Vol),   &(plugin->Converted1Vol),   plugin->SampleRate, pParamFunc);
	f2DelayDelta = getParamChange(IDELAY_2_DELAY,   plugin->Control2Delay, &(plugin->Last2Delay), &(plugin->Converted2Delay), plugin->SampleRate, pParamFunc);
	fFB2Delta    = getParamChange(IDELAY_2_FB,      plugin->Control2FB,    &(plugin->Last2FB),    &(plugin->Converted2FB),    plugin->SampleRate, pParamFunc);
	fPan2Delta   = getParamChange(IDELAY_2_PAN,     plugin->Control2Pan,   &(plugin->Last2Pan),   &(plugin->Converted2Pan),   plugin->SampleRate, pParamFunc);
	fVol2Delta   = getParamChange(IDELAY_2_VOL,     plugin->Control2Vol,   &(plugin->Last2Vol),   &(plugin->Converted2Vol),   plugin->SampleRate, pParamFunc);

	fBypass         = plugin->ConvertedBypass;
	fMode           = plugin->ConvertedMode;
	fMungeMode	= plugin->ConvertedMungeMode;

	if(fMungeDelta==0 && fCycleDelta == 0 && fWidthDelta==0 && fFB1Delta==0 && fFB2Delta==0 && fPan1Delta==0 && fPan2Delta==0 && fVol1Delta==0 && fVol2Delta==0) {
		HasDelta=0;
		fMunge		= plugin->ConvertedMunge;
		fLPFDelta	= 0.0;
		fHPFDelta	= 0.0;
		fLPFsamples	= plugin->ConvertedLPFsamples;
		fHPFsamples	= plugin->ConvertedHPFsamples;
		fAngleDelta	= plugin->ConvertedCycle;
		fLFO		= plugin->ConvertedWidth;
		f1FBraw		= plugin->Converted1FB;
		f1Pan		= plugin->Converted1Pan;
		f1Vol		= plugin->Converted1Vol;
		f2FBraw		= plugin->Converted2FB;
		f2Pan		= plugin->Converted2Pan;
		f2Vol		= plugin->Converted2Vol;
	} else {
		HasDelta	= 1;
		fMunge		= plugin->ConvertedMunge - fMungeDelta;
		if(fMungeDelta == 0) {
			fLPFDelta			= 0.0;
			fHPFDelta			= 0.0;
		} else {
			// this is a bit wonky i know but creating varibles for the old values and then calculating the delta with new-old doesn't save any cpu
			fLPFDelta			= -plugin->ConvertedLPFsamples;
			fHPFDelta			= -plugin->ConvertedHPFsamples;
			plugin->ConvertedLPFsamples	= convertMunge(0, plugin->LastMunge, plugin->SampleRate); 
			plugin->ConvertedHPFsamples	= convertMunge(1, plugin->LastMunge, plugin->SampleRate); 
			fLPFDelta			+= plugin->ConvertedLPFsamples;
			fHPFDelta			+= plugin->ConvertedHPFsamples;
		}
		fLPFsamples	= plugin->ConvertedLPFsamples	- fLPFDelta;
		fHPFsamples	= plugin->ConvertedHPFsamples 	- fHPFDelta;
		fAngleDelta	= plugin->ConvertedCycle 	- fCycleDelta;
		fLFO		= plugin->ConvertedWidth 	- fWidthDelta;
		f1FBraw		= plugin->Converted1FB   	- fFB1Delta;
		f1Pan		= plugin->Converted1Pan  	- fPan1Delta;
		f1Vol		= plugin->Converted1Vol  	- fVol1Delta;
		f2FBraw		= plugin->Converted2FB   	- fFB2Delta;
		f2Pan		= plugin->Converted2Pan  	- fPan2Delta;
		f2Vol		= plugin->Converted2Vol  	- fVol2Delta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fLPFDelta    = fLPFDelta/(float)SampleCount;
			fHPFDelta    = fHPFDelta/(float)SampleCount;
			fMungeDelta  = fMungeDelta/(float)SampleCount;
			fCycleDelta  = fCycleDelta/(float)SampleCount;
			fWidthDelta  = fWidthDelta/(float)SampleCount;
			fFB1Delta    = fFB1Delta/(float)SampleCount;
			fPan1Delta   = fPan1Delta/(float)SampleCount;
			fVol1Delta   = fVol1Delta/(float)SampleCount;
			fFB2Delta    = fFB2Delta/(float)SampleCount;
			fPan2Delta   = fPan2Delta/(float)SampleCount;
			fVol2Delta   = fVol2Delta/(float)SampleCount;
		}
	}

	if(f1DelayDelta==0.0) {
		HasDelay1Old	= 0;
		f1Delay		= plugin->Converted1Delay;
		l1DelaySample	= (unsigned long)f1Delay;
		f1DelayOffset	= f1Delay-(float)l1DelaySample;
		fLFOsamp1	= fLFO * f1Delay;
		f1DelayOld	= 0.0;
		l1DelaySampleOld= 0;
		f1DelayOffsetOld= 0.0;
		fLFOsamp1Old	= 0.0;
	} else {
		HasDelay1Old	= 1;
		f1Delay		= plugin->Converted1Delay;
		l1DelaySample	= (unsigned long)f1Delay;
		f1DelayOffset	= f1Delay-(float)l1DelaySample;
		fLFOsamp1	= fLFO * f1Delay;
		f1DelayOld	= plugin->Converted1Delay-f1DelayDelta;
		l1DelaySampleOld= (unsigned long)f1DelayOld;
		f1DelayOffsetOld= f1DelayOld-(float)l1DelaySampleOld;
		fLFOsamp1Old	= fLFO * f1DelayOld;
	}

	if(f2DelayDelta==0.0) {
		HasDelay2Old	= 0;
		f2Delay		= plugin->Converted2Delay;
		l2DelaySample	= (unsigned long)f2Delay;
		f2DelayOffset	= f2Delay-(float)l2DelaySample;
		fLFOsamp2	= fLFO * f2Delay;
		f2DelayOld	= 0.0;
		l2DelaySampleOld= 0;
		f2DelayOffsetOld= 0.0;
		fLFOsamp2Old	= 0.0;
	} else {
		HasDelay2Old	= 1;
		f2Delay		= plugin->Converted2Delay;
		l2DelaySample	= (unsigned long)f2Delay;
		f2DelayOffset	= f2Delay-(float)l2DelaySample;
		fLFOsamp2	= fLFO * f2Delay;
		f2DelayOld	= plugin->Converted2Delay-f2DelayDelta;
		l2DelaySampleOld= (unsigned long)f2DelayOld;
		f2DelayOffsetOld= f2DelayOld-(float)l2DelaySampleOld;
		fLFOsamp2Old	= fLFO * f2DelayOld;
	}
	oldVol=0.0;
	newVol=1.0;

	f1PanLGain	= f1Vol * (1-f1Pan)/2;
	f1PanRGain	= f1Vol * (1+f1Pan)/2;
	f2PanLGain	= f2Vol * (1-f2Pan)/2;
	f2PanRGain	= f2Vol * (1+f2Pan)/2;
	if(fMungeMode < 0.5) {
		f1FB	= f1FBraw / (1+fMunge);
		f2FB	= f2FBraw / (1+fMunge);
	} else {
		f1FB	= f1FBraw * pow(2.0,-2.0*fMunge);
		f2FB	= f2FBraw * pow(2.0,-2.0*fMunge);
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

	fAngle     	= plugin->LFOAngle;
	LPF1     	= plugin->AudioLPF1;
	LPF2     	= plugin->AudioLPF2;
	HPF1     	= plugin->AudioHPF1;
	HPF2     	= plugin->AudioHPF2;
	Degrain1     	= plugin->AudioDegrain1;
	Degrain2     	= plugin->AudioDegrain2;
	EnvIn     	= plugin->EnvInLLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			if(HasDelay1Old==1 || HasDelay2Old==1) {
				newVol = (float)lSampleIndex/(float)SampleCount;               // 0 -> 1
				oldVol = (float)(SampleCount-lSampleIndex)/(float)SampleCount; // 1 -> 0
			}
			// read the audio out of the delay space
			Out1 = *(SpaceLCur);
			Out2 = *(SpaceRCur);

			// now read the input
			In=( *(pfAudioInputL++) + *(pfAudioInputR++) )/2;

			// and mix the feedback in
			if(fMode<0.5 ) {
				// normal
				In1FBmix= In+(f1FB*Out1);
				In2FBmix= In+(f2FB*Out2);
			} else {
 				//ping pong
				In1FBmix= In+(f2FB*Out2);
				In2FBmix= In+(f1FB*Out1);
			}

			// munge it
			In1FB	= (1-fMunge)*In1FBmix + fMunge*ITube_do(In1FBmix,1+fMunge);
			In2FB	= (1-fMunge)*In2FBmix + fMunge*ITube_do(In2FBmix,1+fMunge);
			HPF1 = ((fHPFsamples-1) * HPF1 + In1FB) / fHPFsamples;  
			HPF2 = ((fHPFsamples-1) * HPF2 + In2FB) / fHPFsamples; 
			LPF1 = ((fLPFsamples-1) * LPF1 + (In1FB-HPF1)) / fLPFsamples; 
			LPF2 = ((fLPFsamples-1) * LPF2 + (In2FB-HPF2)) / fLPFsamples;  
			if(fMungeMode<0.5) {
				In1Munged = LPF1;
				In2Munged = LPF2;
			} else {
				In1Munged = 2 * (In1FB-HPF1) - LPF1;
				In2Munged = 2 * (In2FB-HPF2) - LPF2;
			} 
			Degrain1     	= (In1Munged+(1.1*Degrain1))/2.1;
			Degrain2     	= (In2Munged+(1.1*Degrain2))/2.1;

			//LFO
			if(fLFO > 0) {
				fActualDelay1	= f1Delay + (fLFOsamp1 * cos(fAngle));
				l1DelaySample	= (unsigned long)fActualDelay1;
				f1DelayOffset	= fActualDelay1-(float)l1DelaySample;

				fActualDelay2	= f2Delay + (fLFOsamp2 * cos(fAngle));
				l2DelaySample	= (unsigned long)fActualDelay2;
				f2DelayOffset	= fActualDelay2-(float)l2DelaySample;

				if(HasDelay1Old==1) {
					fActualDelay1Old	= f1DelayOld + (fLFOsamp1Old * cos(fAngle));
					l1DelaySampleOld	= (unsigned long)fActualDelay1Old;
					f1DelayOffsetOld	= fActualDelay1Old-(float)l1DelaySampleOld;
				}
				if(HasDelay2Old==1) {
					fActualDelay2Old	= f2DelayOld + (fLFOsamp2Old * cos(fAngle));
					l2DelaySampleOld	= (unsigned long)fActualDelay2Old;
					f2DelayOffsetOld	= fActualDelay2Old-(float)l2DelaySampleOld;
				}

				fAngle += fAngleDelta;
			}

			// add to the delay space
			if(HasDelay1Old==0) {
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l1DelaySample,    f1DelayOffset,    Degrain1);
			} else {
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l1DelaySample,    f1DelayOffset,    newVol*Degrain1);
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l1DelaySampleOld, f1DelayOffsetOld, oldVol*Degrain1);

			}

			if(HasDelay2Old==0) {
				SpaceAdd(SpaceRCur, SpaceREnd, SpaceSize, l2DelaySample,    f2DelayOffset,    Degrain2);
			} else {
				SpaceAdd(SpaceRCur, SpaceREnd, SpaceSize, l2DelaySample,    f2DelayOffset,    newVol*Degrain2);
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, l2DelaySampleOld, f2DelayOffsetOld, oldVol*Degrain2);

			}

			// mix the two delays in
			OutL = f1PanLGain*Out1 + f2PanLGain*Out2;
			OutR = f1PanRGain*Out1 + f2PanRGain*Out2;
			// write the output
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

			//update any changing parameters
			if(HasDelta==1) {
				fMunge	    += fMungeDelta;
				fLPFsamples += fLPFDelta;
				fHPFsamples += fHPFDelta;
				fAngleDelta += fCycleDelta;
				fLFO	    += fWidthDelta;
				f1FBraw	    += fFB1Delta;
				f1Pan	    += fPan1Delta;
				f1Vol	    += fVol1Delta;
				f2FBraw	    += fFB2Delta;
				f2Pan	    += fPan2Delta;
				f2Vol	    += fVol2Delta;

				fLFOsamp1	= fLFO * f1Delay;
				fLFOsamp2	= fLFO * f2Delay;
				f1PanLGain	= f1Vol * (1-f1Pan)/2;
				f1PanRGain	= f1Vol * (1+f1Pan)/2;
				f2PanLGain	= f2Vol * (1-f2Pan)/2;
				f2PanRGain	= f2Vol * (1+f2Pan)/2;

				if(fMungeMode < 0.5) {
					f1FB	= f1FBraw / (1+fMunge);
					f2FB	= f2FBraw / (1+fMunge);
				} else {
					f1FB	= f1FBraw * pow(2.0,-2.0*fMunge);
					f2FB	= f2FBraw * pow(2.0,-2.0*fMunge);
				}

				if(HasDelay1Old==1) {
					fLFOsamp1Old	= fLFO * f1DelayOld;
				}

				if(HasDelay2Old==1) {
					fLFOsamp2Old	= fLFO * f2DelayOld;
				}
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *pfAudioInputL;
			*(pfAudioOutputR++) = *(pfAudioInputL++);
			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero filters
		LPF1    =0;
		LPF2    =0;
		HPF1    =0;
		HPF2    =0;
		Degrain1    =0;
		Degrain2    =0;
		//zero envelope on in and out for meters
		EnvIn   =0;
		EnvOutL =0;
		EnvOutR =0;
	}
	if(fLFO > 0) {
		while(fAngle > PI_2) {
			fAngle -= PI_2;
		}
	} else {
		fAngle=0;
	}

	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;

	plugin->LFOAngle    = fAngle; 
	plugin->AudioLPF1   = (fabs(LPF1)<1.0e-10)     ? 0.f : LPF1; 
	plugin->AudioLPF2   = (fabs(LPF2)<1.0e-10)     ? 0.f : LPF2; 
	plugin->AudioHPF1   = (fabs(HPF1)<1.0e-10)     ? 0.f : HPF1; 
	plugin->AudioHPF2   = (fabs(HPF2)<1.0e-10)     ? 0.f : HPF2; 
	plugin->AudioDegrain1   = (fabs(Degrain1)<1.0e-10)     ? 0.f : Degrain1; 
	plugin->AudioDegrain2   = (fabs(Degrain2)<1.0e-10)     ? 0.f : Degrain2; 
	plugin->EnvInLLast  = (fabs(EnvIn)<1.0e-10)    ? 0.f : EnvIn; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 

	// update the meters
	*(plugin->LampLFO)     = 1.75*(1-(cos(fAngle)));
	*(plugin->MeterInputL) =(EnvIn   > 0.001) ? 20*log10(EnvIn)   : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}


static void 
cleanupIDelay(LV2_Handle instance)
{
	IDelay *plugin = (IDelay *)instance;

	free(plugin->SpaceL);
	free(plugin->SpaceR);
	free(instance);
}


static void 
init()
{
	IDelayMonoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IDelayMonoDescriptor->URI 		= IDELAY_MONO_URI;
	IDelayMonoDescriptor->activate 	= activateIDelay;
	IDelayMonoDescriptor->cleanup 		= cleanupIDelay;
	IDelayMonoDescriptor->connect_port 	= connectPortIDelay;
	IDelayMonoDescriptor->deactivate 	= NULL;
	IDelayMonoDescriptor->instantiate 	= instantiateIDelay;
	IDelayMonoDescriptor->run 		= runMonoIDelay;
	IDelayMonoDescriptor->extension_data	= NULL;

	IDelaySumDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IDelaySumDescriptor->URI 		= IDELAY_SUM_URI;
	IDelaySumDescriptor->activate 		= activateIDelay;
	IDelaySumDescriptor->cleanup 		= cleanupIDelay;
	IDelaySumDescriptor->connect_port 	= connectPortIDelay;
	IDelaySumDescriptor->deactivate 	= NULL;
	IDelaySumDescriptor->instantiate 	= instantiateIDelay;
	IDelaySumDescriptor->run 		= runSumIDelay;
	IDelaySumDescriptor->extension_data	= NULL;

}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IDelayMonoDescriptor) init();

	switch (index) {
		case 0:
			return IDelayMonoDescriptor;
		case 1:
			return IDelaySumDescriptor;

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
		case IDELAY_BYPASS:
		case IDELAY_MODE:
		case IDELAY_MUNGEMODE:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		case IDELAY_MUNGE:
			if(value<0)
				result = 0.0;
			else if (value < 100)
				result = value/100.0;
			else
				result = 1.0;
			break;
		case IDELAY_LFO_CYCLE:
			if (value < 2.0)
				result = PI_2/(2.0*sr);
			else if (value <= 200.0)
				result = PI_2/(value * sr);
			else
				result = PI_2/(200.0 * sr);
			break;
		case IDELAY_1_DELAY:
		case IDELAY_2_DELAY:
			if (value < 0.02)
				result = 0.02 * sr;
			else if (value <= 2.0)
				result = value * sr;
			else
				result = 2.0 * sr;
			break;
		case IDELAY_1_FB:
		case IDELAY_2_FB:
			if(value<0)
				result = 0.0;
			else if (value < 133.333333)
				result = value/100.0;
			else
				result = 1.3333333;
			break;
		case IDELAY_1_PAN:
		case IDELAY_2_PAN:
			if(value<-1.0)
				result = -1.0;
			else if (value < 1.0)
				result = value;
			else
				result = 1.0;
			break;
		case IDELAY_LFO_WIDTH:
			if(value<0.0)
				result = 0.0;
			else if (value < 100.0)
				result = value/400.0;
			else
				result = 0.25;
			break;
		case IDELAY_1_VOL:
		case IDELAY_2_VOL:
			if(value<0.0)
				result = 0.0;
			else if (value < 100.0)
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

float
convertMunge (unsigned int mode, float munge, double sr)
{
	float result;
	switch(mode) {
		case 0: //LPF
			if (munge < 0)
				result = sr/(2*pow(10,(4.34)));  //22kHz
			else if (munge <= 100.0)
				result = sr/(2*pow(10,(4.34-(munge*0.0074))));
			else
				result = sr/(2*pow(10,(3.60)));  //4kHz
			break;
		case 1: //HPF
			if (munge < 0)
				result = sr/(2*pow(10,(1.30)));  //20Hz
			else if (munge <= 100.0)
				result = sr/(2*pow(10,(1.30+(munge*0.0160))));
			else
				result = sr/(2*pow(10,(2.90)));  //800Hz
			break;
		default:
			result=1;
			break;
	}
	return result;
}


