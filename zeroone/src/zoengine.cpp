//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//
#include "zoengine.h"

#ifdef __linux__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#elif __APPLE__
  #pragma cling load("$DYLD_LIBRARY_PATH/librtmidi.dylib")
#elif __unix__
  #pragma cling load("$LD_LIBRARY_PATH/librtmidi.dylib")
#endif

using namespace std;

using scaleType = const vector<int>;
using chordType = vector<int>;
using rhythmType = vector<int>;
using ampT = double;
using durT = rhythmType;
using label = int;

void pushSJob(vector<Instrument>& insts) {
  SJob j;
  int id = 0;
    
  while (TaskPool<SJob>::isRunning) {
    if (TaskPool<SJob>::jobs.size() < JOB_QUEUE_SIZE) {
      id = id%insts.size();
      if (!insts.at(id).isWritingPlayFunc->load()) {
        j.id = id;
        j.job = &*insts.at(id).f;
      
        TaskPool<SJob>::jobs.push_back(j);
      
        id++;
      }
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

void pushCCJob(vector<Instrument>& insts) {
  CCJob j;
  int id = 0;
  
  while (TaskPool<CCJob>::isRunning) {
    if (TaskPool<CCJob>::jobs.size() < JOB_QUEUE_SIZE) {
      id = id%insts.size();
      if (!insts.at(id).isWritingCCFunc->load()) {
        j.id = id;
        j.job = &*insts.at(id).ccs;
      
        TaskPool<CCJob>::jobs.push_back(j);
      
        id++;
      }
    }
    this_thread::sleep_for(chrono::milliseconds(5));
  }
}

// MARK: Not beeing used
// play function algorithm changes have immediate effect on every note parameter but duration
inline Notes checkPlayingFunctionChanges(function<Notes()>& newFunc, function<Notes()>& currentFunc) {
  Notes playNotes;
  
  if (newFunc) {
    if (playNotes.dur != Generator::midiNote(newFunc).dur)
      playNotes = Generator::midiNoteExcludeDur(currentFunc);
    else
      playNotes = Generator::midiNoteExcludeDur(newFunc);
  } else
      playNotes = Generator::midiNoteExcludeDur(currentFunc);
  
  return playNotes;
}

int taskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  std::unique_lock<mutex> lock(TaskPool<SJob>::mtx,std::defer_lock);
  long barStartTime, barElapsedTime, barDeltaTime, noteStartTime, noteElapsedTime, noteDeltaTime, loopIterTime = 0;
  long t = 0;
  double barDur;
  vector<unsigned char> noteMessage;
  
  SJob j;
  Notes playNotes;
  function<Notes()> playFunc;
  vector<int> durationsPattern;
  
  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);

  while (TaskPool<SJob>::isRunning) {
    barStartTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
    barDur = Generator::barDur();
    
    if (!TaskPool<SJob>::jobs.empty()) {
      TaskPool<SJob>::mtx.lock();
      j = TaskPool<SJob>::jobs.front();
      TaskPool<SJob>::jobs.pop_front();
      TaskPool<SJob>::mtx.unlock();
      
      if (j.job) {
        playFunc = *j.job;
        
        playNotes = Generator::midiNote(playFunc);
        durationsPattern = playNotes.dur;
        
        insts[j.id].out = Generator::protoNotes;
        
        for (auto& dur : durationsPattern) {
          playNotes = Generator::midiNoteExcludeDur(playFunc);
          
          noteStartTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          
          for (auto& note : playNotes.notes) {
            noteMessage[0] = 144+j.id;
            noteMessage[1] = note;
            noteMessage[2] = (note != REST_NOTE && !insts[j.id].isMuted()) ? playNotes.amp : 0;
            midiOut.sendMessage(&noteMessage);
          }
          
          insts.at(j.id).step++;
          
          if (!TaskPool<SJob>::isRunning) goto finishTask;
          
          t = dur-round(dur/barDur*Metro::instsWaitingTimes.at(j.id)+loopIterTime);
          t = (t > 0 && t < barDur) ? t : dur; // prevent negative values for duration and thread overrun
          
          this_thread::sleep_for(chrono::microseconds(t));
          
          for (auto& note : playNotes.notes) {
            noteMessage[0] = 128+j.id;
            noteMessage[1] = note;
            noteMessage[2] = 0;
            midiOut.sendMessage(&noteMessage);
          }
          
          noteElapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          noteDeltaTime = noteElapsedTime-noteStartTime;
          loopIterTime = noteDeltaTime > t ? noteDeltaTime-t : 0;
        }
      }
      Metro::syncInstTask(j.id);
    }
    
    barElapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
    barDeltaTime = barElapsedTime-barStartTime;
    
    Metro::instsWaitingTimes.at(j.id) = barDeltaTime > barDur ? barDeltaTime-barDur : 0;
  }
  
  finishTask:
  // silencing playing notes before task finishing
  for (auto& note : playNotes.notes) {
    noteMessage[0] = 128+j.id;
    noteMessage[1] = note;
    noteMessage[2] = 0;
    midiOut.sendMessage(&noteMessage);
  }
  return j.id;
}

int ccTaskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  std::unique_lock<mutex> lock(TaskPool<CCJob>::mtx,std::defer_lock);
  long startTime, elapsedTime;
  vector<unsigned char> ccMessage;
  CCJob j;
  std::vector<std::function<CC()>> ccs;
  vector<CC> ccComputed;

  midiOut.openPort(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  
  while (TaskPool<CCJob>::isRunning) {
    if (!TaskPool<CCJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      
      TaskPool<CCJob>::mtx.lock();
      j = TaskPool<CCJob>::jobs.front();
      TaskPool<CCJob>::jobs.pop_front();
      TaskPool<CCJob>::mtx.unlock();
      
      if (j.job) {
        ccs = *j.job;
        ccComputed = Generator::midiCC(ccs);
    
        for (auto &cc : ccComputed) {
          ccMessage[0] = 176+j.id;
          ccMessage[1] = cc.ch;
          ccMessage[2] = cc.value;
          midiOut.sendMessage(&ccMessage);
        }
      
        insts.at(j.id).ccStep++;
      
        elapsedTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        this_thread::sleep_for(chrono::milliseconds(CC_RESOLUTION-(elapsedTime-startTime)));
      }
    }
  }
  
  return j.id;
}

Instrument& i(uint8_t ch) {
  return insts.at(ch-1);
}

void bpm(int _bpm) {
  Generator::barDur(_bpm);
}

void bpm() {
  cout << floor(Generator::bpm) << " bpm" << endl;
}

uint32_t sync(int dur) {
  return Metro::sync(dur);
}

uint32_t isync(uint8_t ch) {
  return insts.at(ch-1).step;
}
uint32_t ccsync(uint8_t ch) {
  return insts.at(ch-1).ccStep;
}

uint32_t playhead() {
  return Metro::playhead();
}

void mute() {
  for (auto &inst : insts)
    inst.mute();
}

void mute(int inst) {
  insts[inst-1].mute();
}

void solo(int inst) {
  insts[inst-1].unmute();
  
  for (auto &_inst : insts)
    if (_inst.id != inst-1)
      _inst.mute();
}

void unmute() {
  for (auto &inst : insts)
    inst.unmute();
}

void unmute(int inst) {
  insts[inst-1].unmute();
}

void noctrl() {
  for (auto &inst : insts)
    inst.noctrl();
}

void stop() {
  mute();
  noctrl();
}

Instrument& i(int id) {
  return insts.at(id-1);
}

void zeroone() {
  if (TaskPool<SJob>::isRunning) {
    std::thread([&](){
      TaskPool<SJob>::numTasks = NUM_TASKS;
      TaskPool<CCJob>::numTasks = NUM_TASKS;
      
      // init instruments
      for (int id = 0;id < TaskPool<SJob>::numTasks;++id)
        insts.push_back(Instrument(id));

      Metro::start();
      
      auto futPushSJob = async(launch::async,pushSJob,ref(insts));
      auto futPushCCJob = async(launch::async,pushCCJob,ref(insts));
      
      // init sonic task pool
      for (int i = 0;i < TaskPool<SJob>::numTasks;++i)
        TaskPool<SJob>::tasks.push_back(async(launch::async,taskDo,ref(insts)));
      
      // init cc task pool
      for (int i = 0;i < TaskPool<CCJob>::numTasks;++i)
        TaskPool<CCJob>::tasks.push_back(async(launch::async,ccTaskDo,ref(insts)));
    }).detach();
    
    cout << PROJ_NAME << " on <((()))>" << endl;
  }
}

void on(){
  if (!TaskPool<SJob>::isRunning) {
    bpm(60);
    
    TaskPool<SJob>::isRunning = true;
    TaskPool<CCJob>::isRunning = true;
    
    zeroone();
    
  } else {
      cout << PROJ_NAME << " : already : on <((()))>" << endl;
  }
}

void off() {
  Metro::stop();
  
  TaskPool<SJob>::stopRunning();
  TaskPool<CCJob>::stopRunning();
  
  insts.clear();
  
  cout << PROJ_NAME << " off <>" << endl;
}

int main() {
  return 0;
}