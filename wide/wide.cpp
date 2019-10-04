//
//  wide - MIDI sequencer for live coding
//
//  Created by @pd3v_
//

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <deque>
#include <thread>
#include <future>
#include <chrono>
#include <mutex>
#include <algorithm>
#include "RtMidi.h"

#ifdef __linux__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#elif __APPLE__
  #if TARGET_OS_MAC
    #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
  #endif
#elif __unix__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#endif

#define i(x) (insts[x])
#define n(c,a,d,o) [&]()->Notes {return (Notes){(vector<int>c),a,d,o};} // polyphonic

using namespace std;

using noteDur = pair<int,float>;
using scaleType = vector<int>;
using chordType = vector<int>;

const short NUM_TASKS = 4;
const unsigned int BAR_DUR_REF = 4000000; //microseconds
const unsigned int BPM_REF = 60;

scaleType scale{0,1,2,3,4,5,6,7,8,9,10,11}; // Chromatic scale
// vector<int> scale{0,2,4,5,7,9,11}; // Major scale
chordType Maj{0,4,7}, Min{0,3,7};

struct Notes {
  std::vector<int> notes;
  float amp;
  unsigned long dur; // microseconds
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

function<Notes()> SILENCE_FUNC = []()->Notes {return {(vector<int>{0}),0,4,1};};

struct Job {
  int id;
  function<Notes()>* job;
};

struct TaskPool {
  TaskPool() {
    isRunning = true;
  }
  
  vector<future<int>> tasks;
  short numTasks;
  deque<Job> jobs;
  
  bool isRunning;
  mutex mtx;
  
  void stopRunning() {
    isRunning = false;
    
    for (auto& t : tasks)
      t.get();
    
    jobs.clear();
    tasks.clear();
  }
};

class Generator {
public:
  Generator() {
    bpm = BPM_REF;
  }
  
  static Notes midiNote(const std::function<Notes(void)>& f) {
    Notes notes = f();
    
    transform(notes.notes.begin(), notes.notes.end(), notes.notes.begin(), [&](int n){
      return notes.oct*12+scale[n%12];
    });
    
    notes.amp = ampToVel(notes.amp);
    notes.dur = duration[static_cast<int>(notes.dur)]/(bpm/BPM_REF);
    notes.oct = (int) notes.oct;
    
    return notes;
  }
  
  static unsigned int barDur() {
    return BAR_DUR_REF/(bpm/BPM_REF);
  }
  
  static int bpm;
  
private:
  static int ampToVel(float amp) {
    return round(127*amp);
  }
  
  static unordered_map<int,unsigned long> duration;// {noteDur(1,4000000),noteDur(2,2000000),noteDur(4,1000000),noteDur(8,500000),noteDur(3,333333),noteDur(16,250000),noteDur(6,166667),noteDur(32,125000),noteDur(64,62500)};
};

class Instrument {
public:
  Instrument(int id) {
    this->id = id;
    _ch = id;
  }
  
  void play(function<Notes(void)> fn) {
    *f = fn;
  }
  
  int id;
  unsigned long step = 0;
  shared_ptr<function<Notes(void)>> f = make_shared<function<Notes(void)>>([]()->Notes{return {{0},0,4,1};}); // 1/4 note silence
private:
  int _ch;
};

void pushJob(TaskPool& tp,vector<Instrument>& insts) {
  Job j;
  int id = 0;
  
  while (tp.isRunning) {
    if (tp.jobs.size() < 20) {
      id = id%insts.size();
      j.id = id;
      j.job = &*insts.at(id).f;
      
      tp.jobs.push_back(j);
      
      id++;
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

int taskDo(TaskPool& tp,vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  unsigned long startTime, elapsedTime;
  vector<unsigned char> noteMessage;
  Job j;
  Notes n;
  
  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  while (tp.isRunning) {
    if (!tp.jobs.empty()) {
      tp.mtx.lock();
      j = tp.jobs.front();
      tp.jobs.pop_front();
      tp.mtx.unlock();
      
      auto nFunc = *j.job;
      auto durTemp = Generator::midiNote(nFunc).dur;
      auto durTempPtr = Generator::midiNote(*j.job).dur;
      int nTimes = Generator::barDur()/durTemp;
      
      for (int subn = 0;subn < nTimes;++subn) {
        startTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        for (auto& pitch : n.notes) {
          noteMessage[0] = 128+j.id;
          noteMessage[1] = pitch;
          noteMessage[2] = 0;
          midiOut.sendMessage(&noteMessage);
        }
        
        insts.at(j.id).step++;
        if (!tp.isRunning) break;
        
        // Every note param imediate change allowed except duration
        if (durTemp != durTempPtr)
          n = Generator::midiNote(nFunc);
        else
          n = Generator::midiNote(*j.job);
        
        for (auto& pitch : n.notes) {
          noteMessage[0] = 144+j.id;
          noteMessage[1] = pitch;
          noteMessage[2] = n.amp;
          midiOut.sendMessage(&noteMessage);
        }
        
        elapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        this_thread::sleep_for(chrono::microseconds(n.dur-(elapsedTime-startTime)));
      }
    }
  }
  
  // silencing playing notes before exit
  noteMessage[0] = 128+j.id;
  midiOut.sendMessage(&noteMessage);
  
  return j.id;
}

TaskPool tskPool;
vector<Instrument> insts;

void bpm(int _bpm) {
  Generator::bpm = _bpm;
}

void exit() {
  tskPool.stopRunning();
  cout << "wide is off <>" << endl;
}

int Generator::bpm = BPM_REF;

unordered_map<int,unsigned long> Generator::duration {noteDur(1,4000000),noteDur(2,2000000),noteDur(4,1000000),noteDur(8,500000),noteDur(3,333333),noteDur(16,250000),noteDur(6,166667),noteDur(32,125000),noteDur(64,62500)};

void wide() {
  if (tskPool.isRunning) {
    thread th = std::thread([&](){
      tskPool.numTasks = NUM_TASKS;
      
      // init instruments
      for (int id = 0;id < tskPool.numTasks;++id)
        insts.push_back(Instrument(id));
      
      auto futPushJob = async(launch::async,pushJob,ref(tskPool),ref(insts));
      
      // init task pool
      for (int i = 0;i < tskPool.numTasks;++i)
        tskPool.tasks.push_back(async(launch::async,taskDo,ref(tskPool),ref(insts)));
    });
    th.detach();
    
    cout << "wide is on <((()))>" << endl;
  }
}

int main() {
  return 0;
}

