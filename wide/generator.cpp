//
//  wide
//
//  Created by @pd3v_
//

#include "generator.h"
#include <vector>
#include <queue>
#include <thread>

Generator::Generator() {
  notesQueue.emplace(midiNote(SILENCE));
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


note_t Generator::note() {
  note_t note;
  
  if (!notesQueue.empty()) {
    note = notesQueue.front();
    notesQueue.pop();
  }
  
  return note;
}

void Generator::clearNotesQueue() {
  std::queue<note_t> empty;
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

void Generator::f(std::function<std::vector<float>(void)> f) {
  _f = f;
  clearNotesQueue();
}

note_t Generator::midiNote(const std::function<std::vector<float>(void)>& f) {
  note_t note;
  std::vector<float> n = f();
  
  note.note = (int) _scale[n[0]]+12*n[3];
  note.vel = ampToVel(n[1]);
  note.dur = (int) duration[n[2]]/(_bpm/60.0);
  note.oct = (int) n[3];
  
  return note;
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

/*note_t Generator::midiFreqToNote(std::function<note_t(void)>& f) {
  vector<int> n = Generator::_f();
  note_t note;
  
  note.note = midiNoteToFreq(n[0] + 12 * n[3]);
  note.vel = midiVelToAmp(n[1]);
  note.dur = n[2];
  
  return note;
}*/