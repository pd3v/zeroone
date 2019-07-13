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
#include "texpression.h"

class Instrument {
public:
  Instrument(std::string _id, int _ch);
  ~Instrument();
  
  void bpm(int bpm);
  std::vector<int> scale();
  void scale(std::vector<int> scale);
  int scaleSize();
  void playbar(std::function<Notes(void)> f);
  void play(std::function<Notes(void)> f);
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
  Notes n = {{0},0,4,1}; // Initial Notes
  TExpression express;
  
private:
  Generator _generator;
  unsigned long _startTime;
  unsigned long _elapsedTime;
  cc_t _cc; // struct with channel and function
  std::vector<int> _ccCompute;
  std::function<Notes(void)> _f;
};
