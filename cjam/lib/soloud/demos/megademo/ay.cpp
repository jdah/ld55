/*
SoLoud audio engine
Copyright (c) 2020 Jari Komppa

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

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "imgui.h"
#include "soloud_demo_framework.h"

#include "soloud.h"
#include "soloud_ay.h"
#include "soloud_biquadresonantfilter.h"
#include "soloud_echofilter.h"
#include "soloud_dcremovalfilter.h"
#include "soloud_bassboostfilter.h"

namespace ay
{

	SoLoud::Soloud gSoloud;
	SoLoud::Ay gMusic;
	SoLoud::BiquadResonantFilter gBiquad;
	SoLoud::EchoFilter gEcho;
	SoLoud::DCRemovalFilter gDCRemoval;
	SoLoud::BassboostFilter gBassboost;
	int gMusichandle;

	// Entry point
	int DemoEntry(int argc, char *argv[])
	{
		gMusic.load("audio/adversary.pt3_2ay.zak");

		gEcho.setParams(0.2f, 0.5f, 0.05f);
		gBiquad.setParams(SoLoud::BiquadResonantFilter::LOWPASS, 4000, 2);

		gMusic.setLooping(1);

		gSoloud.setGlobalFilter(0, &gBiquad);
		gSoloud.setGlobalFilter(1, &gBassboost);
		gSoloud.setGlobalFilter(2, &gEcho);
		gSoloud.setGlobalFilter(3, &gDCRemoval);

		gSoloud.init(SoLoud::Soloud::CLIP_ROUNDOFF | SoLoud::Soloud::ENABLE_VISUALIZATION);

		gMusichandle = gSoloud.play(gMusic);
		return 0;
	}

	float filter_param0[4] = { 0, 0, 0, 0 };
	float filter_param1[4] = { 1000, 2, 0, 0 };
	float filter_param2[4] = { 2, 0,  0, 0 };

	void DemoMainloop()
	{
		gSoloud.setFilterParameter(0, 0, 0, filter_param0[0]);
		gSoloud.setFilterParameter(0, 0, 2, filter_param1[0]);
		gSoloud.setFilterParameter(0, 0, 3, filter_param2[0]);

		gSoloud.setFilterParameter(0, 1, 0, filter_param0[1]);
		gSoloud.setFilterParameter(0, 1, 1, filter_param1[1]);

		gSoloud.setFilterParameter(0, 2, 0, filter_param0[2]);

		gSoloud.setFilterParameter(0, 3, 0, filter_param0[3]);


		DemoUpdateStart();

		float *buf = gSoloud.getWave();
		float *fft = gSoloud.calcFFT();

		ONCE(ImGui::SetNextWindowPos(ImVec2(500, 20)));
		ImGui::Begin("Output");
		ImGui::PlotLines("##Wave", buf, 256, 0, "Wave", -1, 1, ImVec2(264, 80));
		ImGui::PlotHistogram("##FFT", fft, 256 / 2, 0, "FFT", 0, 10, ImVec2(264, 80), 8);
		float ayregs[32];
		int i;
		for (i = 0; i < 32; i++)
			ayregs[i] = gSoloud.getInfo(gMusichandle, i);
		ImGui::PlotHistogram("##AY", ayregs, 32, 0, "", 0, 0xff, ImVec2(264, 80), 4);

		ImGui::Text("AY1: %02X %02X %02X %02X %02X %02X %02X %02X", (int)ayregs[0], (int)ayregs[1], (int)ayregs[2], (int)ayregs[3], (int)ayregs[4], (int)ayregs[5], (int)ayregs[6], (int)ayregs[7]);
		ImGui::Text("     %02X %02X %02X %02X %02X %02X", (int)ayregs[8], (int)ayregs[9], (int)ayregs[10], (int)ayregs[11], (int)ayregs[12], (int)ayregs[13]);
		ImGui::Text("AY2: %02X %02X %02X %02X %02X %02X %02X %02X", (int)ayregs[16], (int)ayregs[17], (int)ayregs[18], (int)ayregs[19], (int)ayregs[20], (int)ayregs[21], (int)ayregs[22], (int)ayregs[23]);
		ImGui::Text("     %02X %02X %02X %02X %02X %02X", (int)ayregs[24], (int)ayregs[25], (int)ayregs[26], (int)ayregs[27], (int)ayregs[28], (int)ayregs[29]);

		ImGui::End();

		ONCE(ImGui::SetNextWindowPos(ImVec2(20, 20)));
		ImGui::Begin("Control");
		ImGui::Text("Biquad filter (lowpass)");
		ImGui::SliderFloat("Wet##4", &filter_param0[0], 0, 1);
		ImGui::SliderFloat("Frequency##4", &filter_param1[0], 0, 8000);
		ImGui::SliderFloat("Resonance##4", &filter_param2[0], 1, 20);
		ImGui::Separator();
		ImGui::Text("Bassboost filter");
		ImGui::SliderFloat("Wet##2", &filter_param0[1], 0, 1);
		ImGui::SliderFloat("Boost##2", &filter_param1[1], 0, 11);
		ImGui::Separator();
		ImGui::Text("Echo filter");
		ImGui::SliderFloat("Wet##3", &filter_param0[2], 0, 1);
		ImGui::Separator();
		ImGui::Text("DC removal filter");
		ImGui::SliderFloat("Wet##1", &filter_param0[3], 0, 1);
		ImGui::End();
		DemoUpdateEnd();
	}
}

int DemoEntry_ay(int argc, char *argv[])
{
	return ay::DemoEntry(argc, argv);
}

void DemoMainloop_ay()
{
	return ay::DemoMainloop();
}