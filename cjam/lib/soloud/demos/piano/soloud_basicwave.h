/*
SoLoud audio engine
Copyright (c) 2013-2021 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef BASICWAVE_H
#define BASICWAVE_H

#include "soloud.h"
#include "soloud_adsr.h"

namespace SoLoud
{
	class Basicwave;

	class BasicwaveInstance : public AudioSourceInstance
	{
		Basicwave *mParent;
		float mFreq;
		int mOffset;
		float mT;
	public:
		BasicwaveInstance(Basicwave *aParent);
		virtual unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize);
		virtual bool hasEnded();
	};

	class Basicwave : public AudioSource
	{
	public:
		ADSR mADSR;
		float mFreq;
		float mSuperwaveScale;
		float mSuperwaveDetune;
		int mWaveform;
		bool mSuperwave;
		Basicwave();
		virtual ~Basicwave();
		void setSamplerate(float aSamplerate);
		void setWaveform(int aWaveform);
		void setFreq(float aFreq, bool aSupewave = false);
		virtual AudioSourceInstance *createInstance();
	};
};

#endif