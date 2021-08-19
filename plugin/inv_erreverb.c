/* 

    This LV2 plugin provides an early reflection reverb from a mono source

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


    Some Notes:
    ===========
    This plugin is not your classic sort of reverb that makes long washy spaces.
    It simulates a room by calculating the early reflections that occur off the walls/roof etc.

    It's useful for:
    a) putting dry signals in a 'natural' space so they sit with sounds recorded with 'room' anbience in them.
    b) thickening strings, vocals etc without softening or washing out.
    c) as a singal preprocess for reverb that does not have early reflections.
    d) accurate stereo placement by setting the 'source' LR the same as any panning the dry signle has. This is 
    because the early reflections from the off-center single reinforce the location of the sound rather than 
    contradicting it as a normal reverb would do. 

    Parameter description:
    RoomLength,RoomWidth,RoomHeight - the dimensions (in meters) of the room
    SourceLR, SourceFB (FB=Front/Back) - where the sound source is in the room (always the back half)
    DestLR, DestFB (FB=Front/Back) - where the destination (or listener) is in the room (always the front half)
    HPF - roll off some bottom end
    Warmth - roll off top end (amount depends on reflection count)
    Diffusion - makes the relections less perfect to simulate objects in the room.

*/

#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <lv2.h>
#include "library/common.h"
#include "inv_erreverb.h"

static LV2_Descriptor *IReverbERMonoDescriptor = NULL;
static LV2_Descriptor *IReverbERSumDescriptor = NULL;


typedef struct {

	/* Ports */
	float *ControlBypass;
	float *ControlRoomLength;
	float *ControlRoomWidth; 
	float *ControlRoomHeight;
	float *ControlSourceLR;
	float *ControlSourceFB;
	float *ControlDestLR; 
	float *ControlDestFB;
	float *ControlHPF;
	float *ControlWarmth;
	float *ControlDiffusion;

	float *AudioOutputBufferL;
	float *AudioOutputBufferR;
	float *AudioInputBufferL;
	float *AudioInputBufferR; 

	float *MeterInput;
	float *MeterOutputL;
	float *MeterOutputR;

	double SampleRate;
	struct Envelope EnvAD[4];

	/* Stuff to remember to avoid recalculating the delays every run */
	float LastBypass;
	float LastRoomLength;
	float LastRoomWidth; 
	float LastRoomHeight;
	float LastSourceLR;
	float LastSourceFB;
	float LastDestLR; 
	float LastDestFB;
	float LastHPF;
	float LastWarmth; 
	float LastDiffusion;

	float EnvInLast; 
	float EnvOutLLast; 
	float EnvOutRLast;

	float ConvertedBypass; 
	float ConvertedHPF; 
	float ConvertedWarmth; 

	/* fade in flag */
	int fadeIn;

	/* Delay and Reverb Space Data */
	unsigned int er_size;
	struct ERunit * er;
	unsigned long SpaceSize;
	float * SpaceL;
	float * SpaceR;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	float AudioHPFLast;
	float AudioIn1Last;
	float AudioIn2Last;
	float AudioIn3Last; 
	float AudioIn4Last;

} IReverbER;


static LV2_Handle 
instantiateIReverbER(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IReverbER *plugin = (IReverbER *)malloc(sizeof(IReverbER));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;
	plugin->SpaceSize = REVERB_SPACE_SIZE * s_rate;

	/* the delay space */
	if((plugin->SpaceL  = (float *)malloc(sizeof(float) * plugin->SpaceSize))==NULL)
    		return NULL;
	
	if((plugin->SpaceR  = (float *)malloc(sizeof(float) * plugin->SpaceSize))==NULL)
    		return NULL;
	
	/* the delays */
	if((plugin->er  = (struct ERunit *)malloc(sizeof(struct ERunit) * MAX_ER))==NULL)
    		return NULL;	

	return (LV2_Handle)plugin;
}



static void 
connectPortIReverbER(LV2_Handle instance, uint32_t port, void *data)
{
	IReverbER *plugin = (IReverbER *)instance;
	switch (port) {
		case IERR_BYPASS:
			plugin->ControlBypass = data;
			break;
		case IERR_ROOMLENGTH:
			plugin->ControlRoomLength = data;
			break;
		case IERR_ROOMWIDTH:
			plugin->ControlRoomWidth = data;
			break;
		case IERR_ROOMHEIGHT:
			plugin->ControlRoomHeight = data;
			break;
		case IERR_SOURCELR:
			plugin->ControlSourceLR = data;
			break;
		case IERR_SOURCEFB:
			plugin->ControlSourceFB = data;
			break;
		case IERR_DESTLR:
			plugin->ControlDestLR = data;
			break;
		case IERR_DESTFB:
			plugin->ControlDestFB = data;
			break;
		case IERR_HPF:
			plugin->ControlHPF = data;
			break;
		case IERR_WARMTH:
			plugin->ControlWarmth = data;
			break;
		case IERR_DIFFUSION:
			plugin->ControlDiffusion = data;
			break;
		case IERR_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case IERR_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case IERR_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case IERR_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case IERR_METER_IN:
			plugin->MeterInput = data;
			break;
		case IERR_METER_OUTL:
			plugin->MeterOutputL = data;
			break;
		case IERR_METER_OUTR:
			plugin->MeterOutputR = data;
			break;
	}
}


static void 
activateIReverbER(LV2_Handle instance) 
{
	IReverbER *plugin = (IReverbER *)instance;

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
	plugin->LastBypass     = 0.0;
	plugin->LastRoomLength = 25.0;
	plugin->LastRoomWidth  = 30.0; 
	plugin->LastRoomHeight = 10;
	plugin->LastSourceLR   = -0.01;
	plugin->LastSourceFB   = 0.8;
	plugin->LastDestLR     = 0.01; 
	plugin->LastDestFB     = 0.2;
	plugin->LastHPF        = 1000.0;
	plugin->LastWarmth     = 50;
	plugin->LastDiffusion  = 50;

	plugin->fadeIn=0;

	plugin->AudioHPFLast=0;
	plugin->AudioIn1Last=0;
	plugin->AudioIn2Last=0;
	plugin->AudioIn3Last=0; 
	plugin->AudioIn4Last=0;

	plugin->EnvInLast = 0; 
	plugin->EnvOutLLast = 0; 
	plugin->EnvOutRLast = 0; 

	plugin->ConvertedBypass = convertParam(IERR_BYPASS, plugin->LastBypass, plugin->SampleRate);  
	plugin->ConvertedHPF    = convertParam(IERR_HPF,    plugin->LastHPF,    plugin->SampleRate);  
	plugin->ConvertedWarmth = convertParam(IERR_WARMTH, plugin->LastWarmth, plugin->SampleRate);  
	calculateIReverbERWrapper(instance);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void 
runMonoIReverbER(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float In,EnvIn;
	float OutL,EnvOutL;
	float OutR,EnvOutR;
	float AudioIn,AudioHPF,AudioIn1,AudioIn2,AudioIn3,AudioIn4,AudioProc;
	float fBypass,HPFsamples,WarmthSamples;
	int fadeOut,fadeIn;
	float fadeVol;
	double fWarmthDelta,fHPFDelta;
	int   HasDelta;
	struct ERunit * er;
	unsigned long lSampleIndex;
	unsigned int i;
	unsigned int er_size;
	unsigned long SpaceSize;
	float * SpaceLStr;
	float * SpaceRStr;
	float * SpaceLCur;
	float * SpaceRCur;
	float * SpaceLEnd;
	float * SpaceREnd;

	IReverbER *plugin = (IReverbER *)instance;
	pParamFunc = &convertParam;

	fadeIn=plugin->fadeIn;
	fadeOut=0;

	/* see if the room has changed and recalculate the reflection details if needed */
	if(*(plugin->ControlRoomLength) != plugin->LastRoomLength || 
	   *(plugin->ControlRoomWidth)  != plugin->LastRoomWidth  ||
	   *(plugin->ControlRoomHeight) != plugin->LastRoomHeight ||
	   *(plugin->ControlSourceLR)   != plugin->LastSourceLR   ||
	   *(plugin->ControlSourceFB)   != plugin->LastSourceFB   ||
	   *(plugin->ControlDestLR)     != plugin->LastDestLR     ||
	   *(plugin->ControlDestFB)     != plugin->LastDestFB     ||
	   *(plugin->ControlDiffusion)  != plugin->LastDiffusion  ) {
		  plugin->LastRoomLength = *(plugin->ControlRoomLength);
		  plugin->LastRoomWidth  = *(plugin->ControlRoomWidth);
		  plugin->LastRoomHeight = *(plugin->ControlRoomHeight);
		  plugin->LastSourceLR   = *(plugin->ControlSourceLR);
		  plugin->LastSourceFB   = *(plugin->ControlSourceFB);
		  plugin->LastDestLR     = *(plugin->ControlDestLR);
		  plugin->LastDestFB     = *(plugin->ControlDestFB);
		  plugin->LastDiffusion  = *(plugin->ControlDiffusion);
		  
		if(fadeIn==0) {
			//rather than updating the er straight away, fade out the existing ones.
			fadeOut=1;
		} else {
			//we've just done a fade out so we can update the ers (seens we have to do it again!) and fade in these ones instead.
			calculateIReverbERWrapper(instance);
		}
	}

	/* check if any other params have changed */
	checkParamChange(IERR_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	fWarmthDelta    = getParamChange(IERR_WARMTH, plugin->ControlWarmth, &(plugin->LastWarmth), &(plugin->ConvertedWarmth), plugin->SampleRate, pParamFunc);
	fHPFDelta       = getParamChange(IERR_HPF,    plugin->ControlHPF,    &(plugin->LastHPF),    &(plugin->ConvertedHPF),    plugin->SampleRate, pParamFunc);

	fBypass         = plugin->ConvertedBypass;

	if(fWarmthDelta == 0 && fHPFDelta==0) {
		HasDelta=0;
		WarmthSamples   = plugin->ConvertedWarmth;
		HPFsamples   	= plugin->ConvertedHPF;
	} else {
		HasDelta=1;
		WarmthSamples   = plugin->ConvertedWarmth - fWarmthDelta;
		HPFsamples   	= plugin->ConvertedHPF    - fHPFDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fWarmthDelta = fWarmthDelta/(float)SampleCount;
			fHPFDelta    = fHPFDelta/(float)SampleCount;
		}
	}
	
	er_size   	= plugin->er_size;
	SpaceSize 	= plugin->SpaceSize;

	SpaceLStr	= plugin->SpaceL;
	SpaceRStr 	= plugin->SpaceR;
	SpaceLCur 	= plugin->SpaceLCur;
	SpaceRCur 	= plugin->SpaceRCur;
	SpaceLEnd 	= plugin->SpaceLEnd;
	SpaceREnd 	= plugin->SpaceREnd;

	AudioHPF	= plugin->AudioHPFLast;
	AudioIn1	= plugin->AudioIn1Last;
	AudioIn2	= plugin->AudioIn2Last;
	AudioIn3	= plugin->AudioIn3Last;
	AudioIn4	= plugin->AudioIn4Last;

	pfAudioInputL 	= plugin->AudioInputBufferL;
	pfAudioOutputL 	= plugin->AudioOutputBufferL;
	pfAudioOutputR 	= plugin->AudioOutputBufferR;

	EnvIn     	= plugin->EnvInLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;

	fadeVol		= 1.0;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			In=*(pfAudioInputL++);
			// apply HPF as bottom end in reverbs sounds crap
			AudioHPF = ((HPFsamples-1) * AudioHPF + In) / HPFsamples;  
			AudioIn = In - AudioHPF;

			// apply simple LPF filter repeatedly to audio to simluate frequency loss with each reflection
			AudioIn1=((WarmthSamples-1) * AudioIn1 + AudioIn) / WarmthSamples; 
			AudioIn2=((WarmthSamples-1) * AudioIn2 + AudioIn1) / WarmthSamples; 
			AudioIn3=((WarmthSamples-1) * AudioIn3 + AudioIn2) / WarmthSamples; 
			AudioIn4=((WarmthSamples-1) * AudioIn4 + AudioIn3) / WarmthSamples; 
			  
			er = plugin->er;

			// calculate any fading
			if(fadeIn==1) {
				fadeVol=(float)lSampleIndex/(float)SampleCount;
			}
		
			if(fadeOut==1) {
				fadeVol=1.0-((float)lSampleIndex/(float)SampleCount);
			}

			// now calculate the reflections
			for(i=0;i<er_size;i++) {
				// pick the right version of the audio as per reflection count
				switch(er->Reflections) {
					case 0:
						AudioProc=AudioIn;
						break;
					case 1:
						AudioProc=AudioIn1;
						break;
					case 2:
						AudioProc=AudioIn2;
						break;
					case 3:
						AudioProc=AudioIn3;
						break;
					case 4:
					default:
						AudioProc=AudioIn4;
						break;
				}
				// do any fading
				if(fadeIn==1 || fadeOut==1) {
					AudioProc=AudioProc*fadeVol;
				}
				// add to the delay space
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, er->Delay, er->DelayOffset, AudioProc*er->GainL);
				SpaceAdd(SpaceRCur, SpaceREnd, SpaceSize, er->Delay, er->DelayOffset, AudioProc*er->GainR);
	 
				er++;
			}
			// read the audio out of the delay space
			OutL = *(SpaceLCur);
			OutR = *(SpaceRCur);
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

			//update any changeing parameters
			if(HasDelta==1) {
				WarmthSamples += fWarmthDelta;
				HPFsamples    += fHPFDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = 0;
			*(pfAudioOutputR++) = 0;

			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero filters
		AudioHPF=0; 
		AudioIn1=0; 
		AudioIn2=0; 
		AudioIn3=0; 
		AudioIn4=0; 
		//zero evelope on in and out for meters
		EnvIn   =0;
		EnvOutL =0;
		EnvOutR =0;
	}
	if(fadeIn==1) {
		//we've fadded in the new er, so turn off the flag
		plugin->fadeIn=0;
	}
	if(fadeOut==1) {
		//we've fadded out the exisiting er, update the ers for the next run and flag it to be fadded in
		plugin->fadeIn=1;
		calculateIReverbERWrapper(instance);
	}

	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;
	plugin->AudioHPFLast=(fabs(AudioHPF)<1.0e-10)  ? 0.f : AudioHPF; 
	plugin->AudioIn1Last=(fabs(AudioIn1)<1.0e-10)  ? 0.f : AudioIn1; 
	plugin->AudioIn2Last=(fabs(AudioIn2)<1.0e-10)  ? 0.f : AudioIn2; 
	plugin->AudioIn3Last=(fabs(AudioIn3)<1.0e-10)  ? 0.f : AudioIn3; 
	plugin->AudioIn4Last=(fabs(AudioIn4)<1.0e-10)  ? 0.f : AudioIn4; 

	plugin->EnvInLast   = (fabs(EnvIn)<1.0e-10)   ? 0.f : EnvIn; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 

	// update the meters
	*(plugin->MeterInput)  =(EnvIn  > 0.001) ? 20*log10(EnvIn)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}


static void 
runSumIReverbER(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float *pfAudioInputL;
	float *pfAudioInputR;
	float *pfAudioOutputL;
	float *pfAudioOutputR;
	float In,EnvIn;
	float OutL,EnvOutL;
	float OutR,EnvOutR;
	float AudioIn,AudioHPF,AudioIn1,AudioIn2,AudioIn3,AudioIn4,AudioProc;
	float fBypass,HPFsamples,WarmthSamples;
	int fadeOut,fadeIn;
	float fadeVol;
	double fWarmthDelta,fHPFDelta;
	int   HasDelta;
	struct ERunit * er;
	unsigned long lSampleIndex;
	unsigned int i;
	unsigned int er_size;
	unsigned long SpaceSize;
	float *SpaceLStr;
	float *SpaceRStr;
	float *SpaceLCur;
	float *SpaceRCur;
	float *SpaceLEnd;
	float *SpaceREnd;

	IReverbER *plugin = (IReverbER *)instance;
	pParamFunc = &convertParam;

	fadeIn=plugin->fadeIn;
	fadeOut=0;

	/* see if the room has changed and recalculate the reflection details if needed */
	if(*(plugin->ControlRoomLength) != plugin->LastRoomLength || 
	   *(plugin->ControlRoomWidth)  != plugin->LastRoomWidth  ||
	   *(plugin->ControlRoomHeight) != plugin->LastRoomHeight ||
	   *(plugin->ControlSourceLR)   != plugin->LastSourceLR   ||
	   *(plugin->ControlSourceFB)   != plugin->LastSourceFB   ||
	   *(plugin->ControlDestLR)     != plugin->LastDestLR     ||
	   *(plugin->ControlDestFB)     != plugin->LastDestFB     ||
	   *(plugin->ControlDiffusion)  != plugin->LastDiffusion  ) {
		  plugin->LastRoomLength = *(plugin->ControlRoomLength);
		  plugin->LastRoomWidth  = *(plugin->ControlRoomWidth);
		  plugin->LastRoomHeight = *(plugin->ControlRoomHeight);
		  plugin->LastSourceLR   = *(plugin->ControlSourceLR);
		  plugin->LastSourceFB   = *(plugin->ControlSourceFB);
		  plugin->LastDestLR     = *(plugin->ControlDestLR);
		  plugin->LastDestFB     = *(plugin->ControlDestFB);
		  plugin->LastDiffusion  = *(plugin->ControlDiffusion);
		  
		if(fadeIn==0) {
			//rather than updating the er straight away, fade out the existing ones.
			fadeOut=1;
		} else {
			//we've just done a fade out so we can update the ers (again!) and fade in these ones instead.
			calculateIReverbERWrapper(instance);
		}
	}

	/* check if any other params have changed */
	checkParamChange(IERR_BYPASS, plugin->ControlBypass, &(plugin->LastBypass), &(plugin->ConvertedBypass), plugin->SampleRate, pParamFunc);
	fWarmthDelta    = getParamChange(IERR_WARMTH, plugin->ControlWarmth, &(plugin->LastWarmth), &(plugin->ConvertedWarmth), plugin->SampleRate, pParamFunc);
	fHPFDelta       = getParamChange(IERR_HPF,    plugin->ControlHPF,    &(plugin->LastHPF),    &(plugin->ConvertedHPF),    plugin->SampleRate, pParamFunc);

	fBypass         = plugin->ConvertedBypass;

	if(fWarmthDelta == 0 && fHPFDelta==0) {
		HasDelta=0;
		WarmthSamples   = plugin->ConvertedWarmth;
		HPFsamples   	= plugin->ConvertedHPF;
	} else {
		HasDelta=1;
		WarmthSamples   = plugin->ConvertedWarmth - fWarmthDelta;
		HPFsamples   	= plugin->ConvertedHPF    - fHPFDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fWarmthDelta = fWarmthDelta/(float)SampleCount;
			fHPFDelta    = fHPFDelta/(float)SampleCount;
		}
	}
	
	er_size   	= plugin->er_size;
	SpaceSize 	= plugin->SpaceSize;

	SpaceLStr	= plugin->SpaceL;
	SpaceRStr 	= plugin->SpaceR;
	SpaceLCur 	= plugin->SpaceLCur;
	SpaceRCur 	= plugin->SpaceRCur;
	SpaceLEnd 	= plugin->SpaceLEnd;
	SpaceREnd 	= plugin->SpaceREnd;

	AudioHPF	= plugin->AudioHPFLast;
	AudioIn1	= plugin->AudioIn1Last;
	AudioIn2	= plugin->AudioIn2Last;
	AudioIn3	= plugin->AudioIn3Last;
	AudioIn4	= plugin->AudioIn4Last;

	pfAudioInputL 	= plugin->AudioInputBufferL;
	pfAudioInputR 	= plugin->AudioInputBufferR;
	pfAudioOutputL 	= plugin->AudioOutputBufferL;
	pfAudioOutputR 	= plugin->AudioOutputBufferR;

	EnvIn     	= plugin->EnvInLast;
	EnvOutL    	= plugin->EnvOutLLast;
	EnvOutR   	= plugin->EnvOutRLast;

	fadeVol		= 1.0;

	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			In=( *(pfAudioInputL++) + *(pfAudioInputR++) )/2;

			// apply HPF as bottom end in reverbs sounds crap
			AudioHPF = ((HPFsamples-1) * AudioHPF + In) / HPFsamples;  
			AudioIn = In - AudioHPF;

			// apply simple filter repeatedly to audio to simluate frequency loss with each reflection
			AudioIn1=((WarmthSamples-1) * AudioIn1 + AudioIn) / WarmthSamples; 
			AudioIn2=((WarmthSamples-1) * AudioIn2 + AudioIn1) / WarmthSamples; 
			AudioIn3=((WarmthSamples-1) * AudioIn3 + AudioIn2) / WarmthSamples; 
			AudioIn4=((WarmthSamples-1) * AudioIn4 + AudioIn3) / WarmthSamples; 
			  
			er = plugin->er;
			// calculate any fading
			if(fadeIn==1) {
				fadeVol=(float)lSampleIndex/(float)SampleCount;
			}
		
			if(fadeOut==1) {
				fadeVol=1.0-((float)lSampleIndex/(float)SampleCount);
			}
			// now calculate the reflections
			for(i=0;i<er_size;i++) {
				// pick the right version of the audio as per reflection count
				switch(er->Reflections) {
					case 0:
						AudioProc=AudioIn;
						break;
					case 1:
						AudioProc=AudioIn1;
						break;
					case 2:
						AudioProc=AudioIn2;
						break;
					case 3:
						AudioProc=AudioIn3;
						break;
					case 4:
					default:
						AudioProc=AudioIn4;
						break;
				}
				// do any fading
				if(fadeIn==1 || fadeOut==1) {
					AudioProc=AudioProc*fadeVol;
				}
				// add to the delay space
				SpaceAdd(SpaceLCur, SpaceLEnd, SpaceSize, er->Delay, er->DelayOffset, AudioProc*er->GainL);
				SpaceAdd(SpaceRCur, SpaceREnd, SpaceSize, er->Delay, er->DelayOffset, AudioProc*er->GainR);
			  
				er++;
			}
			// read the audio out of the delay space
			OutL = *(SpaceLCur);
			OutR = *(SpaceRCur);
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
				WarmthSamples += fWarmthDelta;
				HPFsamples    += fHPFDelta;
			}
	
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = 0;
			*(pfAudioOutputR++) = 0;

			// zero the spot we just read
			*(SpaceLCur)=0;
			*(SpaceRCur)=0;
			// advance the pointer to the next spot
			SpaceLCur = SpaceLCur < SpaceLEnd ? SpaceLCur + 1 : SpaceLStr;
			SpaceRCur = SpaceRCur < SpaceREnd ? SpaceRCur + 1 : SpaceRStr;
		}
		//zero filters
		AudioHPF=0; 
		AudioIn1=0; 
		AudioIn2=0; 
		AudioIn3=0; 
		AudioIn4=0; 
		//zero evelope on in and out for meters
		EnvIn   =0;
		EnvOutL =0;
		EnvOutR =0;
	}
	if(fadeIn==1) {
		//we've fadded in the new er, so turn off the flag
		plugin->fadeIn=0;
	}
	if(fadeOut==1) {
		//we've fadded out the exisiting er, update the ers for the next run and flag it to be fadded in
		plugin->fadeIn=1;
		calculateIReverbERWrapper(instance);
	}
	// remember for next run
	plugin->SpaceLCur=SpaceLCur;
	plugin->SpaceRCur=SpaceRCur;
	plugin->AudioHPFLast=(fabs(AudioHPF)<1.0e-10)  ? 0.f : AudioHPF; 
	plugin->AudioIn1Last=(fabs(AudioIn1)<1.0e-10)  ? 0.f : AudioIn1; 
	plugin->AudioIn2Last=(fabs(AudioIn2)<1.0e-10)  ? 0.f : AudioIn2; 
	plugin->AudioIn3Last=(fabs(AudioIn3)<1.0e-10)  ? 0.f : AudioIn3; 
	plugin->AudioIn4Last=(fabs(AudioIn4)<1.0e-10)  ? 0.f : AudioIn4; 

	plugin->EnvInLast   = (fabs(EnvIn)<1.0e-10)   ? 0.f : EnvIn; 
	plugin->EnvOutLLast = (fabs(EnvOutL)<1.0e-10)  ? 0.f : EnvOutL; 
	plugin->EnvOutRLast = (fabs(EnvOutR)<1.0e-10)  ? 0.f : EnvOutR; 

	// update the meters
	*(plugin->MeterInput)  =(EnvIn  > 0.001) ? 20*log10(EnvIn)  : -90.0;
	*(plugin->MeterOutputL)=(EnvOutL > 0.001) ? 20*log10(EnvOutL) : -90.0;
	*(plugin->MeterOutputR)=(EnvOutR > 0.001) ? 20*log10(EnvOutR) : -90.0;
}


static void 
cleanupIReverbER(LV2_Handle instance)
{
	IReverbER *plugin = (IReverbER *)instance;

	free(plugin->er);
	free(plugin->SpaceL);
	free(plugin->SpaceR);
	free(instance);
}


static void 
init()
{
	IReverbERMonoDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IReverbERMonoDescriptor->URI 		= IERR_MONO_URI;
	IReverbERMonoDescriptor->activate 	= activateIReverbER;
	IReverbERMonoDescriptor->cleanup 	= cleanupIReverbER;
	IReverbERMonoDescriptor->connect_port 	= connectPortIReverbER;
	IReverbERMonoDescriptor->deactivate 	= NULL;
	IReverbERMonoDescriptor->instantiate 	= instantiateIReverbER;
	IReverbERMonoDescriptor->run 		= runMonoIReverbER;
	IReverbERMonoDescriptor->extension_data	= NULL;

	IReverbERSumDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IReverbERSumDescriptor->URI 		= IERR_SUM_URI;
	IReverbERSumDescriptor->activate 	= activateIReverbER;
	IReverbERSumDescriptor->cleanup 	= cleanupIReverbER;
	IReverbERSumDescriptor->connect_port 	= connectPortIReverbER;
	IReverbERSumDescriptor->deactivate 	= NULL;
	IReverbERSumDescriptor->instantiate 	= instantiateIReverbER;
	IReverbERSumDescriptor->run 		= runSumIReverbER;
	IReverbERSumDescriptor->extension_data	= NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IReverbERMonoDescriptor) init();

	switch (index) {
		case 0:
			return IReverbERMonoDescriptor;
		case 1:
			return IReverbERSumDescriptor;
		default:
			return NULL;
	}
}


/*****************************************************************************/


void 
calculateIReverbERWrapper(LV2_Handle instance)
{
	IReverbER *plugin = (IReverbER *)instance;

	float convertedWidth,convertedLength,convertedHeight,convertedSourceLR,convertedSourceFB,convertedDestLR,convertedDestFB,convertedDiffusion;

	if (plugin->LastRoomWidth < 3.0)
		convertedWidth = 3.0;
	else if (plugin->LastRoomWidth <= 100.0)
		convertedWidth = plugin->LastRoomWidth;
	else
		convertedWidth = 100.0;

	if (plugin->LastRoomLength < 3.0)
		convertedLength = 3.0;
	else if (plugin->LastRoomLength <= 100.0)
		convertedLength = plugin->LastRoomLength;
	else
		convertedLength = 100.0;

	if (plugin->LastRoomHeight < 3.0)
		convertedHeight = 3.0;
	else if (plugin->LastRoomHeight <= 30.0)
		convertedHeight = plugin->LastRoomHeight;
	else
		convertedHeight = 30.0;

	if (plugin->LastSourceLR < -0.99)
		convertedSourceLR = -0.99;
	else if (plugin->LastSourceLR <= 0.99)
		convertedSourceLR = plugin->LastSourceLR;
	else
		convertedSourceLR = 0.99;

	if (plugin->LastSourceFB < 0.51)
		convertedSourceFB = 0.51;
	else if (plugin->LastSourceFB <= 0.99)
		convertedSourceFB = plugin->LastSourceFB;
	else
		convertedSourceFB = 0.99;

	if (plugin->LastDestLR < -0.99)
		convertedDestLR = -0.99;
	else if (plugin->LastDestLR <= 0.99)
		convertedDestLR = plugin->LastDestLR;
	else
		convertedDestLR = 0.99;

	if (plugin->LastDestFB < 0.01)
		convertedDestFB = 0.01;
	else if (plugin->LastDestFB <= 0.49)
		convertedDestFB = plugin->LastDestFB;
	else
		convertedDestFB = 0.49;

	if (plugin->LastDiffusion < 0.0)
		convertedDiffusion = 0.0;
	else if (plugin->LastDiffusion <= 100.0)
		convertedDiffusion = plugin->LastDiffusion/100;
	else
		convertedDiffusion = 1.0;

	plugin->er_size=calculateIReverbER(plugin->er, MAX_ER, 
					convertedWidth, convertedLength, convertedHeight, 
					convertedSourceLR, convertedSourceFB, 
					convertedDestLR, convertedDestFB, OBJECT_HEIGHT, 
					convertedDiffusion,
					plugin->SampleRate);
}



float 
convertParam(unsigned long param, float value, double sr) {

	float result;

	switch(param)
	{
		case IERR_BYPASS:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		case IERR_HPF:

			if (value < 20)
				result = sr/(40.0);
			else if (value <= 2000.0)
				result = sr/(2*value);
			else
				result=sr/(4000.0);
			break;
		case IERR_WARMTH:
			if(value<0)
				result= 1;
			else if (value < 100)
				result = pow(2,value/50);
			else
				result= 4;
			break;
		default:
			result=0;
			break;
	}
	return result;
}

