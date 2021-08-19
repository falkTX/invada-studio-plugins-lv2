/* 

    This LV2 plugin provides a tone generator

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
#include "inv_testtone.h"


static LV2_Descriptor *IToneDescriptor = NULL;


typedef struct {

	/* Ports */
	float *ControlActive;
	float *ControlFreq;         
	float *ControlTrim;
	float *AudioOutputBuffer;
	float *MeterOutput;

	/* stuff we need to remember to reduce cpu */ 
	double SampleRate; 
	struct Envelope EnvAD[4];

	/* stuff we need to remember to reduce cpu */ 
	float LastActive;
	float LastFreq;     
	float LastTrim;

	float ConvertedActive;
	float ConvertedFreq;     
	float ConvertedTrim;

	float AngleLast; 
	float EnvOutLast; 

} ITone;

static LV2_Handle 
instantiateITone(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	ITone *plugin = (ITone *)malloc(sizeof(ITone));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;

	return (LV2_Handle)plugin;
}



static void 
connectPortITone(LV2_Handle instance, uint32_t port, void *data)
{
	ITone *plugin = (ITone *)instance;

	switch (port) {
		case ITONE_ACTIVE:
			plugin->ControlActive = data;
			break;
		case ITONE_FREQ:
			plugin->ControlFreq = data;
			break;
		case ITONE_TRIM:
			plugin->ControlTrim  = data;
			break;
		case ITONE_AUDIO_OUT:
			plugin->AudioOutputBuffer = data;
			break;
		case ITONE_METER_OUT:
			plugin->MeterOutput = data;
			break;
	}
}


static void 
activateITone(LV2_Handle instance) 
{

	ITone *plugin = (ITone *)instance;

	// these values force the conversion to take place      

	plugin->LastActive = 0;
	plugin->LastFreq   = 1000.00;     
	plugin->LastTrim   = 0.0;
	plugin->AngleLast  = 0.0; 
	plugin->EnvOutLast = 0.0; 

	plugin->ConvertedActive = convertParam(ITONE_ACTIVE, plugin->LastActive,  plugin->SampleRate);
	plugin->ConvertedFreq   = convertParam(ITONE_FREQ,   plugin->LastFreq,    plugin->SampleRate);
	plugin->ConvertedTrim   = convertParam(ITONE_TRIM,   plugin->LastTrim,    plugin->SampleRate);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);
}


static void 
runITone(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioOutput;
	float fActive,fAngleDelta,fTrim,fAngle;
	double fFreqDelta,fTrimDelta;
	int   HasDelta;
	float fAudio;
	float EnvOut;
	uint32_t lSampleIndex;

	ITone *plugin = (ITone *)instance;

	pParamFunc = &convertParam;

	checkParamChange(ITONE_ACTIVE, plugin->ControlActive, &(plugin->LastActive), &(plugin->ConvertedActive), plugin->SampleRate, pParamFunc);
	fFreqDelta = getParamChange(ITONE_FREQ, plugin->ControlFreq, &(plugin->LastFreq), &(plugin->ConvertedFreq), plugin->SampleRate, pParamFunc);
	fTrimDelta = getParamChange(ITONE_TRIM, plugin->ControlTrim, &(plugin->LastTrim), &(plugin->ConvertedTrim), plugin->SampleRate, pParamFunc);

	fActive    = plugin->ConvertedActive;

	if(fFreqDelta == 0 && fTrimDelta==0) {
		HasDelta     =0;
		fAngleDelta  = plugin->ConvertedFreq;
		fTrim        = plugin->ConvertedTrim;
	} else {
		HasDelta=1;
		fAngleDelta  = plugin->ConvertedFreq - fFreqDelta;
		fTrim        = plugin->ConvertedTrim - fTrimDelta;
		if(SampleCount > 0) {
			/* these are the incements to use in the run loop */
			fFreqDelta  = fFreqDelta/(float)SampleCount;
			fTrimDelta  = fTrimDelta/(float)SampleCount;
		}
	}

	pfAudioOutput = plugin->AudioOutputBuffer;

	fAngle        = plugin->AngleLast;
	EnvOut        = plugin->EnvOutLast;
 
	if(fActive==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			fAudio=fTrim*sin(fAngle);
			fAngle+=fAngleDelta;
			if(fAngle > PI) {
				fAngle -= PI_2;
			}
			*(pfAudioOutput++) = fAudio;

			//evelope on in and out for meters
			EnvOut  += applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], fAudio,   EnvOut);

			//update any changing parameters
			if(HasDelta==1) {
				fAngleDelta  += fFreqDelta;
				fTrim        += fTrimDelta;
			}
		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {
			*(pfAudioOutput++) = 0.0;
		}
		//evelope on in and out for meters
		EnvOut =0;
	}



	// store values for next loop
	plugin->AngleLast     = fAngle; 
	plugin->EnvOutLast    = (fabs(EnvOut)<1.0e-10)  ? 0.f : EnvOut; 

	// update the meters
	*(plugin->MeterOutput)=(EnvOut > 0.001) ? 20*log10(EnvOut) : -90.0;
}


static void 
cleanupITone(LV2_Handle instance)
{
	free(instance);
}


static void 
init()
{
	IToneDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IToneDescriptor->URI 		= ITONE_URI;
	IToneDescriptor->activate 	= activateITone;
	IToneDescriptor->cleanup 	= cleanupITone;
	IToneDescriptor->connect_port 	= connectPortITone;
	IToneDescriptor->deactivate 	= NULL;
	IToneDescriptor->instantiate 	= instantiateITone;
	IToneDescriptor->run 		= runITone;
	IToneDescriptor->extension_data = NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IToneDescriptor) init();

	switch (index) {
	case 0:
		return IToneDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/


float 
convertParam(unsigned long param, float value, double sr) {
	float result;
	switch(param)  {
		case ITONE_ACTIVE:
			if(value<=0.0)
				result= 0; 
			else
				result= 1;
			break;
		case ITONE_FREQ:
			if(value < 20.0)
				result= PI_2*20.0/(float)sr;
			else if (value < 20000)
				result= PI_2*value/(float)sr;
			else
				result= PI_2*20000.0/(float)sr;
			break;
		case ITONE_TRIM:
			if(value<-24.0)
				result= pow(10, -24.0/20.0);
			else if (value < 0.0)
				result= pow(10, value/20.0);
			else
				result= 1.0;
			break;
		default:
			result=0;
			break;
	}
	return result;
}


