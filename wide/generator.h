//
//  wide
//
//  Created by @pd3v_
//

#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <queue>

#define SILENCE []()->Notes{return {{0},0,4,1};} // silent Notes

using noteDur = std::pair<int,float>;

struct Notes {
  std::vector<int> notes;
  float amp;
  unsigned long dur; // nanoseconds
  int oct;
  
  void print() {
    std::cout << "{";
    for_each(notes.begin(),notes.end(),[](int n){std::cout << n << " ";});
    std::cout << "} ";
    std::cout << amp << " ";
    std::cout << dur << " ";
    std::cout << oct << " ";
    std::cout << std::endl;
  }
};

struct cc_t {
  int ch;
  std::function<int(void)> f;
};

class StepTimer {
public:
  unsigned int step; // increment's resolution; set as 1/200
  unsigned long playhead; // global step sequencer's counter; each increment <=> 1/4 Notes
  
  bool beat_end() {
    if (step == endStep-stepOffset)
      return true;
    else
      return false;
  }
  
  bool bar_start() {
    if (playhead%timeSignature == 0)
      return true;

    return false;
  }
  
  void print() {
    std::cout << " step=" << step << " playhead=" << playhead << " bar=" << playhead%timeSignature << std::endl;
  }
private:
  const int timeSignature = 4; // default time signature - 4/4
  const unsigned int endStep = 200;
  unsigned int stepOffset = 0;
};

extern StepTimer stepTimer;

const int QUEUE_SIZE = 10;

class Generator {
public:
  Generator();
  ~Generator();
  void queueNote();
  //void queuecc();
  void bpm(int bpm);
  int bpm();
  Notes midiNote(const std::function<Notes(void)>& f);
  Notes notes();
  Notes notes(std::function<Notes(void)>& f);
  std::vector<int> midicc(cc_t _cc);
  void cc(std::vector<cc_t>& ccs);
  void scale(std::vector<int>scale);
  std::vector<int> scale();
  void f(std::function<Notes(void)> f);
  int ampToVel(float amp);
  
  unsigned long int step = 0; // +1 for each Notes played
  std::vector<int> metaNotes;
  std::queue<Notes> notesQueue;
  //std::queue<cc_t> ccQueue;
  
  // milliseconds
  //std::unordered_map<int,float> duration {noteDur(1,4000),noteDur(2,2000),noteDur(4,1000),noteDur(8,500),noteDur(3,333.333333),noteDur(16,250),noteDur(6,166.6666666),noteDur(32,125),noteDur(64,62.5)};
  
  // nanoseconds
  std::unordered_map<int,unsigned long> duration {noteDur(1,4000000000),noteDur(2,2000000000),noteDur(4,1000000000),noteDur(8,500000000),noteDur(3,333333333),noteDur(16,250000000),noteDur(6,166666667),noteDur(32,125000000),noteDur(64,62500000)};
  
  std::vector<cc_t> ccVct;
  
private:
  void clearNotesQueue();
  
  int _bpm = 60;
  bool _queueRun = true;
  std::vector<int> _scale {0,2,4,5,7,9,11}; //Major scale
  std::function<Notes(void)> _f = SILENCE;
};
