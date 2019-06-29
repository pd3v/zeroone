//
//  wide
//
//  Created by @pd3v_
//

#include "instrument.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <RtMidi.h>
#include "expression.h"

using namespace std::chrono;

Instrument::Instrument(std::string _id, int _ch)
:id(_id), ch(_ch), midiout(std::unique_ptr<RtMidiOut>(new RtMidiOut)), _generator(Generator()) {
  midiout->openPort(0);
  
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
};

Instrument::~Instrument(){
  std::cout << "instrument " << id << " quit." << std::endl;
  noteMessage[0] = 128+ch;
  midiout->sendMessage(&noteMessage);
};

void Instrument::bpm(int bpm) {
  _generator.bpm(bpm);
}

std::vector<int> Instrument::scale() {
  return _generator.scale();
}

void Instrument::scale(std::vector<int> scale) {
  _generator.scale(scale);
}

int Instrument::scaleSize() {
  return (int)_generator.scale().size();
}

void Instrument::play(std::function<std::vector<int>(void)> f) {
  std::thread t([&](){
    while (true) {
      if (stepTimer.bar_start()) {
        _generator.f(f);
        break;
      }
    }
  });
  t.detach();
}

void Instrument::playasap(std::function<std::vector<int>(void)> f) {
  _generator.f(f);
}

void Instrument::cc(std::vector<cc_t> ccs) {
  for(auto& cc : ccs)
    std::cout << cc.ch << " " << &cc.f << " " << cc.f() << std::endl;
  
  _generator.cc(ccs);
}

void Instrument::cc() {
  std::vector<cc_t> ccVct {};
  _generator.cc(ccVct);
}

void Instrument::mute() {
  isMuted = true;
}

void Instrument::unmute() {
  isMuted = false;
}

void Instrument::playIt() {
  std::thread t([&](){
    playing = true;
    _startTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
    
    // note off
    noteMessage[0] = 128+ch ;
    noteMessage[1] = n.note;
    noteMessage[2] = 0;
    midiout->sendMessage(&noteMessage);
    
    n = _generator.note();

    // note on
    noteMessage[0] = 144+ch;
    noteMessage[1] = n.note;
    noteMessage[2] = isMuted == true ? 0 : n.vel;
    
    // cc
    for (auto &cc : _generator.ccVct) {
     _ccCompute = _generator.midicc(cc);
     ccMessage[0] = 176+ch;
     ccMessage[1] = _ccCompute[0];
     ccMessage[2] = _ccCompute[1];
     midiout->sendMessage(&ccMessage);
    }
    
    midiout->sendMessage(&noteMessage);
    
    _elapsedTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();

    std::this_thread::sleep_for(nanoseconds(n.dur-(_elapsedTime-_startTime)));

    Expression::step = _step++;
    playing = false;
  });
  t.detach();
}