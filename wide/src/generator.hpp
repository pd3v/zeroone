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
  static float barDur() {
    return BAR_DUR_REF/(bpm/BPM_REF);
  }
  
  //FIXME: division is not an atomic operation, so is this returning atomically?...hummm
  static std::atomic<float> bpmRatio() {
    return {bpm/BPM_REF};
  }
  
  static float barDur(float _bpm) {
    if (_bpm > 0) {
      bpm = _bpm;
      return BAR_DUR_REF/(bpm/BPM_REF);
    }
    return bpm;
  }
  
  static Notes midiNote(const function<Notes(void)>& fn) {
    Notes notes = fn();
    protoNotes = notes; // Notes object before converting to MIDI spec
    
    if (notes.oct != 1) // MIDI note value depends on octave specification
      transform(notes.notes.begin(), notes.notes.end(), notes.notes.begin(), [&](int n){
        return (n != REST_NOTE ? notes.oct*12+scale[n%scale.size()] : n);
      });
  
    notes.amp = ampToVel(notes.amp);
    notes.dur = parseDurPattern(fn);
  
    transform(notes.dur.begin(), notes.dur.end(), notes.dur.begin(), [&](int d){
      return duration[d]/bpmRatio();
    });

    return notes;
  }
  
  static Notes midiNoteExcludeDur(const function<Notes(void)>& fn) {
    Notes notes = fn();
    protoNotes = notes; // Notes object before converting to MIDI spec
    
    if (notes.oct != 1) // MIDI note value depends on octave specification
      transform(notes.notes.begin(), notes.notes.end(), notes.notes.begin(), [&](int n){
        return (n != REST_NOTE ? notes.oct*12+scale[n%scale.size()] : n);
      });
    
    notes.amp = ampToVel(notes.amp);
    
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
    
    if (accDurTotal >= barDur()-offset && accDurTotal <= barDur()+offset)
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
    
    if (accDurTotal >= barDur()-offset && accDurTotal <= barDur()+offset)
      return tempDur;
    // ---
    
    return {};
  }

  static vector<int> scale;
  static float bpm;
  static Notes protoNotes;
private:
  static int ampToVel(float amp) {
    return round(127*amp);
  }
  
  static unordered_map<int8_t,int> duration;
};

vector<int> Generator::scale = {}; // Chromatic scale as default
float Generator::bpm = BPM_REF;
Notes Generator::protoNotes = {{0},0.,{1},1};

unordered_map<int8_t,int> Generator::duration{noteDurMs(1,4000000),noteDurMs(2,2000000),noteDurMs(4,1000000),noteDurMs(8,500000),noteDurMs(3,666666),noteDurMs(16,250000),noteDurMs(6,333333),noteDurMs(32,125000),noteDurMs(64,62500)};
