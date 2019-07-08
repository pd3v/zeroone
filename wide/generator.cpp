//
//  wide
//
//  Created by @pd3v_
//

#include "generator.h"
#include <vector>
#include <queue>
#include <thread>

#include <iostream>

Generator::Generator() {
  notesQueue.emplace((Notes){{0},0,4,1});
  queueNote();
}

Generator::~Generator() {};

void Generator::queueNote() {
  std::thread t([&](){
    while(true) {
      if (notesQueue.size() < QUEUE_SIZE || notesQueue.empty())
        notesQueue.push(midiNote(_f));
      
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });
  t.detach();
}

/*void Generator::queuecc() {
  std::thread t([&](){
    while(true) {
      if (ccQueue.size() < QUEUE_SIZE || ccQueue.empty())
        ccQueue.push(midicc(_cc));
      
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });
  t.detach();
}*/


Notes Generator::notes() {
  Notes notes;
  
  if (!notesQueue.empty()) {
    notes = notesQueue.front();
    notesQueue.pop();
  }
  
  return notes;
}

void Generator::clearNotesQueue() {
  std::queue<Notes> empty;
  std::swap(notesQueue, empty);
}

void Generator::bpm(int bpm){
  _bpm = bpm;
  clearNotesQueue();
}

void Generator::scale(std::vector<int>scale) {
  _scale = scale;
  clearNotesQueue();
}

std::vector<int> Generator::scale() {
  return _scale;
}

void Generator::f(std::function<Notes(void)> f) {
  _f = f;
  clearNotesQueue();
}

Notes Generator::midiNote(const std::function<Notes(void)>& f) {
  Notes notes = f();
  int i = 0;
  
  for (auto& n : notes.notes) {
    notes.notes[i] = (int) _scale[n]+12*notes.oct;
    i++;
  }
  
  notes.amp = ampToVel(notes.amp);
  notes.dur = static_cast<int>(duration[notes.dur]/(_bpm/60.0));
  notes.oct = (int) notes.oct;
  
  return notes;
}

// cc channel(s) and function(s)
void Generator::cc(std::vector<cc_t>& ccs) {
  ccVct.clear();
  
  for (auto &cc: ccs)
    ccVct.push_back(cc);
}

std::vector<int> Generator::midicc(cc_t _cc) {
  std::vector<int> cc{_cc.ch,_cc.f()};
  return cc;
}

int Generator::ampToVel(float amp) {
  return std::round(127*amp);
}

/*double Generator::midiNoteToFreq(double midiNote) {
   return (pow(2, ((midiNote-69.0)/12))*440.0);
}*/

/*double Generator::midiVelToAmp(double midiNote) {
  return midiNote/127.;
}*/

/*Notes Generator::midiFreqToNote(std::function<Notes(void)>& f) {
  vector<int> n = Generator::_f();
  Notes notes;
  
  notes.note = midiNoteToFreq(n[0] + 12 * n[3]);
  notes.vel = midiVelToAmp(n[1]);
  notes.dur = n[2];
  
  return notes;
}*/