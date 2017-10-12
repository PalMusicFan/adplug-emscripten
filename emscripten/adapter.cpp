/*
* This file adapts "adplug" to the interface expected by my generic JavaScript player..
*
* Copyright (C) 2014 Juergen Wothke
*
* LICENSE
* 
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or (at
* your option) any later version. This library is distributed in the hope
* that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include "../src/adplug.h"
#include "../src/opl.h"
#include "../src/demuopl.h"
#include "../src/surroundopl.h"
#include "../src/database.h"

#include "output.h"

#include <stdio.h>
#include <emscripten.h>

#ifdef EMSCRIPTEN
#define EMSCRIPTEN_KEEPALIVE __attribute__((used))
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#define BUF_SIZE	1024
#define TEXT_MAX	255
#define NUM_MAX	15

short	*outputBuffer=0;
Copl	*opl= 0;
BufPlayer	*player= 0;
const char* infoTexts[6];

char title_str[TEXT_MAX];
char author_str[TEXT_MAX];
char desc_str[TEXT_MAX];
char type_str[TEXT_MAX];
char tracks_str[NUM_MAX];
char speed_str[NUM_MAX];

CAdPlugDatabase *db= 0;

struct StaticBlock {
    StaticBlock(){
		infoTexts[0]= title_str;
		infoTexts[1]= author_str;
		infoTexts[2]= desc_str;
		infoTexts[3]= type_str;
		infoTexts[4]= speed_str;
		infoTexts[5]= tracks_str;
    }
};

static StaticBlock staticBlock;

unsigned long playTime= 0;
unsigned long totalTime= 0;
unsigned int sampleRate= 44100;

extern "C" void emu_teardown (void)  __attribute__((noinline));
extern "C" void EMSCRIPTEN_KEEPALIVE emu_teardown (void) {
  if(player) { delete player; player= 0;}
  if(opl) { delete opl; opl= 0; }
	
  if (outputBuffer) {free(outputBuffer); outputBuffer= 0;}
}

extern "C" int emu_init(int sample_rate, char *basedir, char *songmodule) __attribute__((noinline));
extern "C" EMSCRIPTEN_KEEPALIVE int emu_init(int sample_rate, char *basedir, char *songmodule)
{
	emu_teardown();
	
	outputBuffer = (short *)calloc(BUF_SIZE, sizeof(short));	
		
	std::string	fn = std::string(basedir) + songmodule;  

	opl = new CSurroundopl(
				new CDemuopl(sample_rate, true, false),
				new CDemuopl(sample_rate, true, false),
				true, sample_rate, 384);
	player= new BufPlayer(opl, 16, 2, sample_rate, BUF_SIZE);
	
	// initialize output & player
	player->get_opl()->init();
	player->p = CAdPlug::factory(fn, player->get_opl());
		
	if (db == 0) {
		db= new CAdPlugDatabase();
		db->load("addplug.db");
	}
	CAdPlug::set_database(db);
	
	if(!player->p) {
		std::cout << "Error loading: " << fn << std::endl;
		delete opl; 
		opl= 0; 
		return -1;
	}	
		
	sampleRate= sample_rate;

	return 0;
}

extern "C" int emu_set_subsong(int subsong) __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_set_subsong(int subsong)
{
	player->p->rewind(subsong);
	
	playTime= 0;
	totalTime= player->p->songlength(subsong);	// must be cached bc call corrupts playback
	
	return 0;
}

extern "C" const char** emu_get_track_info() __attribute__((noinline));
extern "C" const char** EMSCRIPTEN_KEEPALIVE emu_get_track_info() {
    if (player && player->p) {
        // more Emscripten/JavaScript friendly structure..
		snprintf(title_str, TEXT_MAX, "%s", player->p->gettitle().c_str());
		snprintf(author_str, TEXT_MAX, "%s", player->p->getauthor().c_str());
		snprintf(desc_str, TEXT_MAX, "%s", player->p->getdesc().c_str());
		snprintf(type_str, TEXT_MAX, "%s", player->p->gettype().c_str());
		
		snprintf(speed_str, NUM_MAX, "%d", player->p->getspeed());
		snprintf(tracks_str, NUM_MAX, "%d",  player->p->getsubsongs());
    }
    return infoTexts;
}

extern "C" char* EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer(void) __attribute__((noinline));
extern "C" char* EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer(void) {
	return (char*)player->getSampleBuffer();
}

extern "C" long EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer_length(void) __attribute__((noinline));
extern "C" long EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer_length(void) {
	return player->getSampleBufferSize();
}

extern "C" int emu_compute_audio_samples() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_compute_audio_samples() {
    player->frame();
	playTime+= (player->getSampleBufferSize()>>2);
	
	if (player->playing) {
		return 0;
	} else {
		return 1;
	}
}

extern "C" int emu_get_current_position() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_get_current_position() {
	return playTime / sampleRate *1000;	
}

extern "C" void emu_seek_position(int pos) __attribute__((noinline));
extern "C" void EMSCRIPTEN_KEEPALIVE emu_seek_position(int pos) {
	playTime= pos/1000*sampleRate;
	player->p->seek(pos);
}

extern "C" int emu_get_max_position() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_get_max_position() {
	return totalTime;
}

