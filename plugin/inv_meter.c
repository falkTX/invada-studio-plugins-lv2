/* 

    This LV2 plugin provides meters

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
#include "inv_meter.h"


static LV2_Descriptor *IMeterDescriptor = NULL;


typedef struct {

	/* Ports */
	float *ControlBypass;
	float *AudioInputBufferL;
	float *AudioInputBufferR; 
	float *AudioOutputBufferL;
	float *AudioOutputBufferR;
	float *MeterL;
	float *MeterR; 
	float *VuL;
	float *VuR; 
	float *MeterPhase;
	float *Spec[FILTER_COUNT];


	/* stuff we need to remember to reduce cpu */ 
	double SampleRate; 

	/* stuff we need to remember to reduce cpu */ 
	float LastBypass;
	float ConvertedBypass;

	struct Envelope EnvAD[4];
	float EnvMeterLLast; 
	float EnvMeterRLast; 
	float EnvVuLLast; 
	float EnvVuRLast; 
	float EnvPhaseLast; 
	float EnvSpecLast[FILTER_COUNT];

	/* filters */
	struct FilterP * filters;

} IMeter;

static LV2_Handle 
instantiateIMeter(const LV2_Descriptor *descriptor, double s_rate, const char *path, const LV2_Feature * const* features)
{
	IMeter *plugin = (IMeter *)malloc(sizeof(IMeter));
	if(plugin==NULL)
		return NULL;

	/* set some initial params */
	plugin->SampleRate=s_rate;

	/* the delays */
	if((plugin->filters  = (struct FilterP *)malloc(sizeof(struct FilterP) * FILTER_COUNT))==NULL)
    		return NULL;	

	return (LV2_Handle)plugin;
}



static void 
connectPortIMeter(LV2_Handle instance, uint32_t port, void *data)
{
	IMeter *plugin = (IMeter *)instance;

	switch (port) {
		case IMETER_BYPASS:
			plugin->ControlBypass = data;
			break;
		case IMETER_AUDIO_INL:
			plugin->AudioInputBufferL = data;
			break;
		case IMETER_AUDIO_INR:
			plugin->AudioInputBufferR = data;
			break;
		case IMETER_AUDIO_OUTL:
			plugin->AudioOutputBufferL = data;
			break;
		case IMETER_AUDIO_OUTR:
			plugin->AudioOutputBufferR = data;
			break;
		case IMETER_METER_L:
			plugin->MeterL = data;
			break;
		case IMETER_METER_R:
			plugin->MeterR = data;
			break;
		case IMETER_VU_L:
			plugin->VuL = data;
			break;
		case IMETER_VU_R:
			plugin->VuR = data;
			break;
		case IMETER_METER_PHASE:
			plugin->MeterPhase = data;
			break;
		case IMETER_SPEC_20: 
		case IMETER_SPEC_25: 
		case IMETER_SPEC_31: 
		case IMETER_SPEC_40: 
		case IMETER_SPEC_50:
		case IMETER_SPEC_63: 
		case IMETER_SPEC_80: 
		case IMETER_SPEC_100: 
		case IMETER_SPEC_125: 
		case IMETER_SPEC_160: 
		case IMETER_SPEC_200: 
		case IMETER_SPEC_250: 
		case IMETER_SPEC_315: 
		case IMETER_SPEC_400: 
		case IMETER_SPEC_500: 
		case IMETER_SPEC_630: 
		case IMETER_SPEC_800: 
		case IMETER_SPEC_1000: 
		case IMETER_SPEC_1250: 
		case IMETER_SPEC_1600: 
		case IMETER_SPEC_2000: 
		case IMETER_SPEC_2500: 
		case IMETER_SPEC_3150: 
		case IMETER_SPEC_4000: 
		case IMETER_SPEC_5000:
		case IMETER_SPEC_6300:	
		case IMETER_SPEC_8000:
		case IMETER_SPEC_10000:  
		case IMETER_SPEC_12500:  
		case IMETER_SPEC_16000: 
		case IMETER_SPEC_20000:  
			plugin->Spec[port-10] = data;
			break;
	}
}


static void 
activateIMeter(LV2_Handle instance) 
{

	IMeter *plugin = (IMeter *)instance;
	int i;

	//defaults     
	plugin->LastBypass = 0;
	plugin->EnvMeterLLast = 0; 
	plugin->EnvMeterRLast = 0; 
	plugin->EnvVuLLast = 0; 
	plugin->EnvVuRLast = 0; 
	plugin->EnvPhaseLast = 0; 

	for(i=0;i<FILTER_COUNT;i++) {
		plugin->EnvSpecLast[i] = 0;
	}

	plugin->ConvertedBypass    = convertParam(IMETER_BYPASS,     plugin->LastBypass,     plugin->SampleRate);

	/* initialise envelopes */
	initIEnvelope(&plugin->EnvAD[INVADA_METER_VU],    INVADA_METER_VU,    plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK],  INVADA_METER_PEAK,  plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], INVADA_METER_PHASE, plugin->SampleRate);
	initIEnvelope(&plugin->EnvAD[INVADA_METER_LAMP],  INVADA_METER_LAMP,  plugin->SampleRate);

	/* initialise filters */
	initBandpassFilter(&plugin->filters[0],  plugin->SampleRate,    20.0, 0.33);
	initBandpassFilter(&plugin->filters[1],  plugin->SampleRate,    25.0, 0.33);
	initBandpassFilter(&plugin->filters[2],  plugin->SampleRate,    31.5, 0.33);
	initBandpassFilter(&plugin->filters[3],  plugin->SampleRate,    40.0, 0.33);
	initBandpassFilter(&plugin->filters[4],  plugin->SampleRate,    50.0, 0.33);
	initBandpassFilter(&plugin->filters[5],  plugin->SampleRate,    63.0, 0.33);
	initBandpassFilter(&plugin->filters[6],  plugin->SampleRate,    80.0, 0.33);
	initBandpassFilter(&plugin->filters[7],  plugin->SampleRate,   100.0, 0.33);
	initBandpassFilter(&plugin->filters[8],  plugin->SampleRate,   125.0, 0.33);
	initBandpassFilter(&plugin->filters[9],  plugin->SampleRate,   160.0, 0.33);
	initBandpassFilter(&plugin->filters[10], plugin->SampleRate,   200.0, 0.33);
	initBandpassFilter(&plugin->filters[11], plugin->SampleRate,   250.0, 0.33);
	initBandpassFilter(&plugin->filters[12], plugin->SampleRate,   315.0, 0.33);
	initBandpassFilter(&plugin->filters[13], plugin->SampleRate,   400.0, 0.33);
	initBandpassFilter(&plugin->filters[14], plugin->SampleRate,   500.0, 0.33);
	initBandpassFilter(&plugin->filters[15], plugin->SampleRate,   630.0, 0.33);
	initBandpassFilter(&plugin->filters[16], plugin->SampleRate,   800.0, 0.33);
	initBandpassFilter(&plugin->filters[17], plugin->SampleRate,  1000.0, 0.33);
	initBandpassFilter(&plugin->filters[18], plugin->SampleRate,  1250.0, 0.33);
	initBandpassFilter(&plugin->filters[19], plugin->SampleRate,  1600.0, 0.33);
	initBandpassFilter(&plugin->filters[20], plugin->SampleRate,  2000.0, 0.33);
	initBandpassFilter(&plugin->filters[21], plugin->SampleRate,  2500.0, 0.33);
	initBandpassFilter(&plugin->filters[22], plugin->SampleRate,  3150.0, 0.33);
	initBandpassFilter(&plugin->filters[23], plugin->SampleRate,  4000.0, 0.33);
	initBandpassFilter(&plugin->filters[24], plugin->SampleRate,  5000.0, 0.33);
	initBandpassFilter(&plugin->filters[25], plugin->SampleRate,  6300.0, 0.33);
	initBandpassFilter(&plugin->filters[26], plugin->SampleRate,  8000.0, 0.33);
	initBandpassFilter(&plugin->filters[27], plugin->SampleRate, 10000.0, 0.33);
	initBandpassFilter(&plugin->filters[28], plugin->SampleRate, 12500.0, 0.33);
	initBandpassFilter(&plugin->filters[29], plugin->SampleRate, 16000.0, 0.33);
	initBandpassFilter(&plugin->filters[30], plugin->SampleRate, 20000.0, 0.33);
}


static void 
runIMeter(LV2_Handle instance, uint32_t SampleCount) 
{
	float (*pParamFunc)(unsigned long, float, double) = NULL;
	float * pfAudioInputL;
	float * pfAudioInputR;
	float * pfAudioOutputL;
	float * pfAudioOutputR;
	float fBypass;
	float In;
	float InL,EnvMeterL,EnvVuL;
	float InR,EnvMeterR,EnvVuR;
	float EnvPhase,CurrentPhase;
	float EnvSpec[FILTER_COUNT];
	int i;
	struct FilterP *filter;
	uint32_t lSampleIndex;

	IMeter *plugin = (IMeter *)instance;

	pParamFunc = &convertParam;

	checkParamChange(IMETER_BYPASS,     plugin->ControlBypass,    &(plugin->LastBypass),    &(plugin->ConvertedBypass),    plugin->SampleRate, pParamFunc);

	fBypass    	= plugin->ConvertedBypass;

	pfAudioInputL 	= plugin->AudioInputBufferL;
	pfAudioInputR 	= plugin->AudioInputBufferR;
	pfAudioOutputL 	= plugin->AudioOutputBufferL;
	pfAudioOutputR 	= plugin->AudioOutputBufferR;

	EnvMeterL     	= plugin->EnvMeterLLast;
	EnvMeterR     	= plugin->EnvMeterRLast;
	EnvVuL     	= plugin->EnvVuLLast;
	EnvVuR     	= plugin->EnvVuRLast;
	EnvPhase  	= plugin->EnvPhaseLast;
	
	for(i=0;i<FILTER_COUNT;i++) {
		EnvSpec[i]=plugin->EnvSpecLast[i];
	}
 
	if(fBypass==0) { 
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			InL=*(pfAudioInputL++);
			InR=*(pfAudioInputR++);
			
			In=(InL+InR)/2;

			*(pfAudioOutputL++) = InL;
			*(pfAudioOutputR++) = InR;

			//evelope on in and out for meters
			EnvMeterL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InL, EnvMeterL);
			EnvMeterR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], InR, EnvMeterR);

			EnvVuL  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_VU], InL, EnvVuL);
			EnvVuR  	+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_VU], InR, EnvVuR);


			//envelope for phase
			if(fabs(InL) > 0.001 || fabs(InR) > 0.001) {  // -60 db
				CurrentPhase = fabs(InL+InR) > 0.000001 ? atan(fabs((InL-InR)/(InL+InR))) : PI_ON_2;
			} else {
				CurrentPhase =0;
			}
			EnvPhase += applyIEnvelope(&plugin->EnvAD[INVADA_METER_PHASE], CurrentPhase, EnvPhase);

			//envelop for spectrum
			filter=plugin->filters;
			for(i=0;i<FILTER_COUNT;i++) {
				EnvSpec[i]+= applyIEnvelope(&plugin->EnvAD[INVADA_METER_PEAK], applyBandpassFilter(&filter[i],In), EnvSpec[i]);
			}

		}
	} else {
		for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) {

			*(pfAudioOutputL++) = *(pfAudioInputL++);
			*(pfAudioOutputR++) = *(pfAudioInputR++);
		}
		//evelope on in and out for meters
		EnvMeterL  	=0;
		EnvMeterR  	=0;
		EnvVuL  	=0;
		EnvVuR  	=0;
		EnvPhase 	=0;
		for(i=0;i<FILTER_COUNT;i++) {
			EnvSpec[i]=0;
		}
	}

	// store values for next loop
	plugin->EnvMeterLLast 	= (fabs(EnvMeterL)<1.0e-10)  ? 0.f : EnvMeterL; 
	plugin->EnvMeterRLast 	= (fabs(EnvMeterR)<1.0e-10)  ? 0.f : EnvMeterR; 
	plugin->EnvVuLLast 	= (fabs(EnvVuL)<1.0e-10)  ? 0.f : EnvVuL; 
	plugin->EnvVuRLast 	= (fabs(EnvVuR)<1.0e-10)  ? 0.f : EnvVuR; 
	plugin->EnvPhaseLast 	= (fabs(EnvPhase)<1.0e-10)  ? 0.f : EnvPhase; 
	for(i=0;i<FILTER_COUNT;i++) {
		plugin->EnvSpecLast[i] = (fabs(EnvSpec[i])<1.0e-10)  ? 0.f : EnvSpec[i];
	}

	// update the meters
	*(plugin->MeterL) 	= (EnvMeterL  > 0.001) ? 20*log10(EnvMeterL)  : -90.0;
	*(plugin->MeterR) 	= (EnvMeterR  > 0.001) ? 20*log10(EnvMeterR)  : -90.0;
	*(plugin->VuL) 		= EnvVuL;
	*(plugin->VuR) 		= EnvVuR;
	*(plugin->MeterPhase)	= EnvPhase;

	for(i=0;i<FILTER_COUNT;i++) {
		*(plugin->Spec[i]) = (EnvSpec[i] > 0.001) ? 20*log10(EnvSpec[i]) : -90.0;
	}
}


static void 
cleanupIMeter(LV2_Handle instance)
{
	free(instance);
}


static void 
init()
{
	IMeterDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	IMeterDescriptor->URI 		= IMETER_URI;
	IMeterDescriptor->activate 	= activateIMeter;
	IMeterDescriptor->cleanup 	= cleanupIMeter;
	IMeterDescriptor->connect_port 	= connectPortIMeter;
	IMeterDescriptor->deactivate 	= NULL;
	IMeterDescriptor->instantiate 	= instantiateIMeter;
	IMeterDescriptor->run 		= runIMeter;
	IMeterDescriptor->extension_data = NULL;
}


LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!IMeterDescriptor) init();

	switch (index) {
	case 0:
		return IMeterDescriptor;
	default:
		return NULL;
	}
}


/*****************************************************************************/


float 
convertParam(unsigned long param, float value, double sr) {
	float result;
	switch(param)  {
		case IMETER_BYPASS:
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


