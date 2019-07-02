//
//  wide
//
//  Created by @pd3v_
//

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <RtMidi.h>
#include "generator.h"
#include "expresstime.h"

#define SILENCE [](){return std::vector<float> {0,0,4,1};}

class Instrument {
public:
  Instrument(std::string _id, int _ch);
  ~Instrument();
  
  void bpm(int bpm);
  std::vector<int> scale();
  void scale(std::vector<int> scale);
  int scaleSize();
  void playonb(std::function<std::vector<float>(void)> f);
  void play(std::function<std::vector<float>(void)> f);
  void playIt();
  void cc(std::vector<cc_t> ccs);
  void cc(); // cleans cc - same result as "nocc" directive
  void mute();
  void unmute();

  std::thread instThread;
  std::string id;
  int ch;
  std::unique_ptr<RtMidiOut> midiout;
  std::vector<unsigned char> noteMessage;
  std::vector<unsigned char> ccMessage;
  bool isMuted = false;
  bool solo = false;
  bool playing = false;
  note_t n{0,0,4,1}; // Initial note
  ExpressTime express;
  
private:
  Generator _generator;
  //unsigned long int _step = 0; // +1 for each note played
  unsigned long _startTime;
  unsigned long _elapsedTime;
  
  std::function<std::vector<float>(void)> _f;

  cc_t _cc; // struct with channel and function
  std::vector<int> _ccCompute;
};
