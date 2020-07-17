//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once 

#include <vector>
#include <unordered_map>
#include <functional>
#include <math.h>
#include "notes.hpp"

extern const int REST_NOTE;
extern const float BAR_DUR_REF; // microseconds
extern const float BPM_REF;

using namespace std;

using noteDurMs = std::pair<int,float>;

class Generator {
public:
  
  static int barDurMs() {
    return BAR_DUR_REF/(bpm/BPM_REF);
  }
  
  //FIXME: division is not an atomic operation, so is this returning atomically?...hummm
  static std::atomic<float> bpmRatio() {
    return {bpm/BPM_REF};
  }
  
  static int barDurMs(const float _bpm) {
    if (_bpm > 0) {
      bpm = _bpm;
      return static_cast<unsigned int>(BAR_DUR_REF/(bpm/BPM_REF));
    }
    return bpm;
  }
  
  static Notes midiNote(const function<Notes(void)>& fn) {
    Notes notes = fn();
    
    notes.barDur = notes.dur; // hack to hold durations in bar units to be used in instruments' out
    
    transform(notes.notes.begin(), notes.notes.end(), notes.notes.begin(), [&](int n){
      return (n != REST_NOTE ? notes.oct*12+scale[n%scale.size()] : n);
    });
    
    notes.amp = ampToVel(notes.amp);
    notes.dur = parseDurPattern(fn);
    
    transform(notes.dur.begin(), notes.dur.end(), notes.dur.begin(), [&](int d){
      return static_cast<int>(duration[d]);
    });
  
    notes.oct = static_cast<int>(notes.oct);
    
    return notes;
  }
  
  static vector<CC> midiCC(const vector<function<CC()>>& ccsFn) {
    vector<CC> ccValues;
    
    for(auto& _ccFn : ccsFn)
      ccValues.push_back(_ccFn());
    
    return ccValues;
  }
  
  static vector<int> parseDurPattern(const function<Notes(void)>& fn) {
    Notes notes = fn();
    float accDurTotal = 0;
    short offset = 5;
    vector<int> tempDur;
    
    if (notes.dur.size() == 1) {
      notes.dur.resize(BAR_DUR_REF/duration[(notes.dur.front())]);
      fill(notes.dur.begin(),notes.dur.end(),notes.dur.front());
      
      return notes.dur;
    }
    
    // --- All time figures explicit
    for (int d : notes.dur)
      accDurTotal += duration[d]/(bpm/BPM_REF);
    
    if (accDurTotal >= barDurMs()-offset && accDurTotal <= barDurMs()+offset)
      return notes.dur;
    // ---
    
    accDurTotal = 0;
    
    // --- Short-typed note duration pattern. Eg. short-typed {4,3,6,16} parses into {4,3,3,3,6,6,6,6,6,6,16,16,16,16}
    for (int d : notes.dur) {
      auto numDurFigure = (d < 3 ? static_cast<int>(ceil(static_cast<double>(duration[4])/duration[d])) : duration[4]/duration[d]);
      
      for (int i = 0;i < numDurFigure; ++i)
        tempDur.push_back(d);
    }
    
    for (int d : tempDur)
      accDurTotal += duration[d]/(bpm/BPM_REF);
    
    if (accDurTotal >= barDurMs()-offset && accDurTotal <= barDurMs()+offset)
      return tempDur;
    // ---
    
    return {};
  }

  static vector<int> scale;
  static float bpm;
private:
  static int ampToVel(float amp) {
    return round(127*amp);
  }
  
  static unordered_map<int,int> duration;
};

float Generator::bpm = BPM_REF;

unordered_map<int,int> Generator::duration{noteDurMs(1,4000000),noteDurMs(2,2000000),noteDurMs(4,1000000),noteDurMs(8,500000),noteDurMs(3,333333),noteDurMs(16,250000),noteDurMs(6,166666),noteDurMs(32,125000),noteDurMs(64,62500)};
