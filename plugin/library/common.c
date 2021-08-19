/* 

    Common functions

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
#include "common.h"

#ifdef _WIN32
inline double drand48(void) { return (double)rand()/RAND_MAX; }
inline void srand48(long int val) { srand(val); }
#endif

/* a function that checks to see if a control has been changed and calls the provided conversion fuction */
void 
checkParamChange(
	unsigned long param, 
	float * control, 
	float * last, 
	float * converted, 
	double sr,
	float (*ConvertFunction)(unsigned long, float, double)
	) 
{
	if(*control != *last) {
		*last=*control;
		*converted=(*ConvertFunction)(param, *control, sr);
	}
}

float 
getParamChange(
	unsigned long param, 
	float * control, 
	float * last, 
	float * converted, 
	double sr,
	float (*ConvertFunction)(unsigned long, float, double)
	) 
{
	float delta;
	float old;

	if(*control != *last) {
		old = *converted;
		*last=*control;
		*converted=(*ConvertFunction)(param, *control, sr);
		delta=*converted-old;
	} else {
		delta=0.0;
	}

	return delta;
}

/* a function which maps an envelope to a sound */
void 
initIEnvelope(struct Envelope * Env, int mode, double sr)
{
	switch(mode) {
		case INVADA_METER_VU:  
			Env->attack = 1 - pow(10, -301.0301 / ((float)sr * 150.0));
			Env->decay  = Env->attack;
		break;
		case INVADA_METER_PEAK: 
			Env->attack = 1 - pow(10, -301.0301 / ((float)sr *   0.5));
			Env->decay  = 1 - pow(10, -301.0301 / ((float)sr * 100.0));
		break;
		case INVADA_METER_PHASE: 
			Env->attack = 1 - pow(10, -301.0301 / ((float)sr * 20.0));
			Env->decay  = Env->attack;
		break;
		case INVADA_METER_LAMP: 
			Env->attack = 1 - pow(10, -301.0301 / ((float)sr *  10.0));
			Env->decay  = 1 - pow(10, -301.0301 / ((float)sr * 100.0));
		break;
	}
}

float 
applyIEnvelope(struct Envelope * Env, float audio_value, float envelope_value)
{
	float valueA;

	valueA=fabs(audio_value);

	return (valueA > envelope_value)  ? Env->attack * (valueA - envelope_value)  : Env->decay * (valueA - envelope_value);
}

void 
SpaceAdd(float *SpacePos, float *SpaceEnd, unsigned long SpaceSize, unsigned long Delay, float Offset, float Value)
{
	if(SpacePos+Delay > SpaceEnd)
		*(SpacePos+Delay-SpaceSize)	+=Value*(1-Offset);
	else
		*(SpacePos+Delay)		+=Value*(1-Offset);

	if(SpacePos+Delay+1 > SpaceEnd)
		*(SpacePos+Delay-SpaceSize+1)	+=Value*Offset;
	else
		*(SpacePos+Delay+1)		+=Value*Offset;
}

void 
SpaceSub(float *SpacePos, float *SpaceEnd, unsigned long SpaceSize, unsigned long Delay, float Offset, float Value)
{
	if(SpacePos+Delay > SpaceEnd)
		*(SpacePos+Delay-SpaceSize)	-=Value*(1-Offset);
	else
		*(SpacePos+Delay)		-=Value*(1-Offset);

	if(SpacePos+Delay+1 > SpaceEnd)
		*(SpacePos+Delay-SpaceSize+1)	-=Value*Offset;
	else
		*(SpacePos+Delay+1)		-=Value*Offset;
}


/* this function is linear between -0.7 & 0.7 (approx -3db) and returns a value bewteen 0.7 and 1 for an input from 0.7 to infinity */
float 
InoClip(float in, float * drive)
{
	float out;
	if ( fabs(in) < 0.7 ) {
	  	out = in;
		*drive=0;
	} else { 
	  out = (in>0) ?
	            (  0.7 + 0.3 * (1-pow(2.718281828, 3.33333333*(0.7-in)))):
	            ( -0.7 - 0.3 * (1-pow(2.718281828, 3.33333333*(0.7+in))));
		*drive=fabs(in) - fabs(out); /* out is always going to be lower than in */
	}
	return out;
}

/* distortion function based on sin() */
float 
ITube_do(float in, float Drive)
{
	float out;
	out = (in>0) ? 
		 pow( fabs(sin( in*Drive*PI_ON_2)),ITUBE_MAGIC ) : 
		-pow( fabs(sin(-in*Drive*PI_ON_2)),ITUBE_MAGIC ) ;
	return out;
}


void 
calculateSingleIReverbER(struct ERunit * er, float Width, float Length, float Height, int Phase, unsigned int Reflections, float DDist, double sr) {

	float ERAngle,ERDistanceSQRD,ERDistance,ERRelDelayActual,ERRelGain,ERRelGainL,ERRelGainR;

	ERAngle           = atan(Width/Length);
	ERDistanceSQRD    = pow(Length,2) + pow(Width,2)+ pow(Height,2);
	ERDistance        = sqrt(ERDistanceSQRD);
	ERRelDelayActual  = ((ERDistance-DDist) * (float)sr /SPEED_OF_SOUND);
	ERRelGain         = Phase / ERDistanceSQRD;
	ERRelGainL        = (ERRelGain * (1 - (ERAngle/PI_ON_2)))/2;
	ERRelGainR        = (ERRelGain * (1 + (ERAngle/PI_ON_2)))/2;

	er->Active=1;
	er->rand=drand48();
	er->DelayActual=ERRelDelayActual;
	er->Reflections=Reflections;
	er->AbsGain=fabs(ERRelGain);
	er->GainL=ERRelGainL;
	er->GainR=ERRelGainR;
}

int 
calculateIReverbER(struct ERunit *erarray, int erMax, 
			float width, float length, float height, 
			float sourceLR, float sourceFB, 
			float destLR, float destFB, float objectHeight, 
			float diffusion,
			double sr)
{

	float SourceToLeft,SourceToRight,SourceToRear,SourceToFront;
	float DestToLeft,DestToRight,DestToRear,DestToFront;
	float RoofHeight,FloorDepth;
	float DirectLength,DirectWidth,DirectHeight,DirectDistanceSQRD,DirectDistance;
	float ERLength,ERWidth,ERHeight,MaxGain;

	struct ERunit *er, *er2;
	unsigned int Num,TotalNum,i;


	SourceToLeft = (1+sourceLR) /2 * width;
	SourceToRight= (1-sourceLR) /2 * width;
	SourceToFront= sourceFB        * length;
	SourceToRear = (1-sourceFB)    * length;

	DestToLeft = (1+destLR) /2 * width;
	DestToRight= (1-destLR) /2 * width;
	DestToFront= destFB        * length;
	DestToRear = (1-destFB)    * length;

	RoofHeight = height - objectHeight;
	FloorDepth = objectHeight;

	DirectLength = SourceToFront-DestToFront;
	DirectWidth = SourceToLeft-DestToLeft;
	DirectHeight =0; // both the source and the lisenter are at the same height
	DirectDistanceSQRD = pow(DirectLength,2)+pow(DirectWidth,2) < 1 ? 1 : pow(DirectLength,2)+pow(DirectWidth,2);
	DirectDistance = sqrt(DirectDistanceSQRD) < 1 ? 1 : sqrt(DirectDistanceSQRD);

	er=erarray;
	Num=0;
	MaxGain=0.000000000001; /* this is used to scale up the reflections so that the loudest one has a gain of 1 (0db) */

	/* seed the random sequence*/
	srand48(314159265);
  
	// reflections from the left wall
	// 0: S->Left->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, -1, 1, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 1: S->BackWall->Left->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 2: S->Right->Left->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToRight + width + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 3: S->BackWall->Right->Left->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = -(SourceToRight + width + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;  

	// 4: S->Left->Rigth->Left->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToLeft + (2 * width) + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 5: S->BackWall->Left->Right->Left->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = -(SourceToLeft + (2 * width) + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, 1, 4, DirectDistance,  sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;  

	// reflections from the right wall
	// 6: S->Right->D
	ERLength       = DirectLength;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 1, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 7: S->BackWall->Right->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 8: S->Left->Right->D
	ERLength       = DirectLength;
	ERWidth        = SourceToLeft + width + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 9: S->BackWall->Left->Right->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = SourceToLeft + width + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 10: S->Right->Left->Right->D
	ERLength       = DirectLength;
	ERWidth        = SourceToRight + (2 * width) + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 11: S->BackWall->Right->Left->Right->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = SourceToRight + (2 * width) + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, 1, 4, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// reflections from the rear wall
	// 12: S->BackWall->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = DirectWidth;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 1, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 13: S->NearWall->BackWall->D
	ERLength       = SourceToFront + length + DestToRear;
	ERWidth        = DirectWidth;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 14: S->Left->NearWall->BackWall->D
	ERLength       = SourceToFront + length + DestToRear;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 15: S->Right->NearWall->BackWall->D
	ERLength       = SourceToFront + length + DestToRear;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = DirectHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// reflections from the roof
	// 16: S->Roof->Left->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = 2*RoofHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 17: S->Roof->Right->D
	ERLength       = DirectLength;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = 2*RoofHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 1, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 18: S->BackWall->Roof->Left->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = 2*RoofHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 19: S->BackWall->Roof->Right->D
	ERLength       = SourceToRear + DestToRear;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = 2*RoofHeight;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance,sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// reflections from the floor 
	// 20: S->Floor->Left->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = 2*FloorDepth;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 21: S->Floor->Right->D
	ERLength       = DirectLength;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = 2*FloorDepth;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, 1, 2, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// reflections from roof and floor
	// 22: S->Roof->Left->Floor->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToLeft + DestToLeft);
	ERHeight       = 2*RoofHeight + 2*FloorDepth;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 23: S->Roof->Right->Floor->D
	ERLength       = DirectLength;
	ERWidth        = SourceToRight + DestToRight;
	ERHeight       = 2*RoofHeight + 2*FloorDepth;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 3, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 24: S->Roof->Left->Floor->Right->Roof->D
	ERLength       = DirectLength;
	ERWidth        = -(SourceToLeft + DirectWidth + DestToLeft);
	ERHeight       = 4*RoofHeight + 2*FloorDepth;
	calculateSingleIReverbER(er, ERWidth, ERLength,  ERHeight, -1, 5, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	// 25: S->Roof->Right->Floor->Left->Roof->D
	ERLength       = DirectLength;
	ERWidth        = SourceToRight + DirectWidth + DestToRight;
	ERHeight       = 4*RoofHeight + 2*FloorDepth;
	calculateSingleIReverbER(er, ERWidth, ERLength, ERHeight, -1, 5, DirectDistance, sr);
	if(er->AbsGain > MaxGain)
		MaxGain=er->AbsGain;
	er++;
	Num++;

	er2=er;
	er=erarray;
	TotalNum=Num;
	for(i=0;i<Num;i++) {
		//create a new reflection based on the diffusion
		if(diffusion > 0 && 4*er->AbsGain/MaxGain > 1-diffusion) {
			er2->Active=1;
			er2->rand=er->rand;
			er2->DelayActual=er->DelayActual*(1.085+(er->rand*diffusion/7));
			er2->Delay = (unsigned long)er2->DelayActual;
			er2->DelayOffset = er2->DelayActual - (float)er2->Delay;
			er2->Reflections=er->Reflections;
			er2->AbsGain=er->AbsGain*diffusion*0.6/MaxGain;
			er2->GainL=er->GainL*diffusion*0.6/MaxGain;
			er2->GainR=er->GainR*diffusion*0.6/MaxGain;
			TotalNum++;
			er2++;
		}

		//scale up reflection and calculate sample delay
		er->DelayActual=er->DelayActual*(1.01+(er->rand*diffusion/14));
		er->Delay = (unsigned long)er->DelayActual;
		er->DelayOffset = er->DelayActual - (float)er->Delay;
		er->AbsGain=er->AbsGain/MaxGain;
		er->GainL=er->GainL/MaxGain;
		er->GainR=er->GainR/MaxGain;
		er++;
	}
	return TotalNum;
}

void 
initBandpassFilter(struct FilterP *f, double sr, double cf, double bw)
{ 
	int i;
	double w0,alpha,a0,a1,a2,b0,b1,b2;



	if(cf<sr/2.0) {

		//shrink the bandwidth if it takes us over the nyquist frequency 
		if((1.0+bw)*cf > sr/2.0) {
			bw= ( (bw) + ((sr/(2*cf)) -1.0)  )/2;
		}

		f->Active=1;

		for(i=0;i<3;i++) {
			f->x[i] = 0.0; 
			f->x2[i] = 0.0; 
			f->y[i] = 0.0; 
			f->y2[i] = 0.0; 

		}

		w0 = PI_2*cf/sr;

		alpha = sin(w0)*sinh((log(2)/2) * bw * (w0/sin(w0))) ;      

		b0 =   alpha;
		b1 =   0;
		b2 =  -alpha;
		a0 =   1 + alpha;
		a1 =  -2*cos(w0);
		a2 =   1 - alpha;

		f->i[0]=b0/a0;
		f->i[1]=b1/a0;
		f->i[2]=b2/a0;
		f->i[3]=a1/a0;
		f->i[4]=a2/a0;
	} else {
		f->Active=0;
	}

	return;
}

float 
applyBandpassFilter(struct FilterP *f, float in)
{ 
	int i;

	if(f->Active==1) {
		for(i=0;i<2;i++) {
			f->x[i]  = f->x[i+1]; 
			f->x2[i] = f->x2[i+1]; 
			f->y[i]  = f->y[i+1]; 
			f->y2[i] = f->y2[i+1]; 
		}

		f->x[2] = (double)in;
		f->y[2] = f->i[0]*f->x[2] + f->i[1]*f->x[1] + f->i[2]*f->x[0]
		                          - f->i[3]*f->y[1] - f->i[4]*f->y[0];  

		f->x2[2] = f->y[2];
		f->y2[2] = f->i[0]*f->x2[2] + f->i[1]*f->x2[1] + f->i[2]*f->x2[0]
		                            - f->i[3]*f->y2[1] - f->i[4]*f->y2[0];  

		return (float)f->y2[2];
	} else {
		return 0;
	}
}



