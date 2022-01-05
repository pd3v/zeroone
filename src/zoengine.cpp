//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//
#include "zoengine.h"

using ampT = double;
using durT = vector<int>;
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
        TaskPool<SJob>::jobs.emplace_back(j);
      
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
      
        TaskPool<CCJob>::jobs.emplace_back(j);
      
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
  long barStartTime, barElapsedTime, barDeltaTime, noteStartTime, noteElapsedTime, noteDeltaTime, loopIterTime = 0;
  long t = 0;
  double barDur;
  vector<unsigned char> noteMessage;
  
  SJob j;
  Notes playNotes;
  int jId = 0;
  function<Notes(void)> jF = SILENCE;
  vector<int> durationsPattern;
  
  midiOut.openPort(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);

  while (TaskPool<SJob>::isRunning) {
    barStartTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
    barDur = Generator::barDur();

    if (!TaskPool<SJob>::jobs.empty()) {
      { 
        std::lock_guard<std::mutex> lock(TaskPool<SJob>::mtx);
        j = TaskPool<SJob>::jobs.front();
        jId = j.id; 
        jF = *j.job;
        TaskPool<SJob>::jobs.pop_front();
      }

      if (jId != NO_INST_ASSIGN) {
        // Metro::syncInstTask(jId);
        playNotes = Generator::midiNote(jF);
        durationsPattern = playNotes.dur;
        
        insts[jId].out = Generator::protoNotes;
        
        for (auto& dur : durationsPattern) {
          noteStartTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          
          for (auto& note : playNotes.notes) {
            noteMessage[0] = 144+jId;
            noteMessage[1] = note;
            noteMessage[2] = (note != REST_NOTE && !insts[jId].isMuted()) ? playNotes.amp : 0;
            midiOut.sendMessage(&noteMessage);
          }
          
          insts.at(jId).step++;
          
          if (!TaskPool<SJob>::isRunning) goto finishTask;
          
          t = dur-round(dur/barDur*Metro::instsWaitingTimes.at(jId)+loopIterTime);
          t = (t > 0 && t < barDur) ? t : dur; // prevent negative values for duration and thread overrun
          
          this_thread::sleep_for(chrono::microseconds(t));
          
          for (auto& note : playNotes.notes) {
            noteMessage[0] = 128+jId;
            noteMessage[1] = note;
            noteMessage[2] = 0;
            midiOut.sendMessage(&noteMessage);
          }
          
          noteElapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
          noteDeltaTime = noteElapsedTime-noteStartTime;
          loopIterTime = noteDeltaTime > t ? noteDeltaTime-t : 0;

          playNotes = Generator::midiNoteExcludeDur(jF);
      }
      
        Metro::syncInstTask(jId);
      }
    
      barElapsedTime = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      barDeltaTime = barElapsedTime-barStartTime;
    
      Metro::instsWaitingTimes.at(jId) = barDeltaTime > barDur ? barDeltaTime-barDur : 0;
    }
    jId = NO_INST_ASSIGN;
  }
  finishTask:
  // silencing playing notes before task finishing
  for (auto& note : playNotes.notes) {
    noteMessage[0] = 128+jId;
    noteMessage[1] = note;
    noteMessage[2] = 0;
    midiOut.sendMessage(&noteMessage);
  }
  return jId;
}

int ccTaskDo(vector<Instrument>& insts) {
  RtMidiOut midiOut = RtMidiOut();
  long startTime, elapsedTime;
  vector<unsigned char> ccMessage;
  
  CCJob j;
  int jId = NO_INST_ASSIGN;
  std::vector<std::function<CC()>> ccs;
  vector<CC> ccComputed;

  midiOut.openPort(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  ccMessage.push_back(0);
  
  while (TaskPool<CCJob>::isRunning) {
    if (!TaskPool<CCJob>::jobs.empty()) {
      startTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
      
      {
        std::lock_guard<std::mutex> lock(TaskPool<CCJob>::mtx);
        j = TaskPool<CCJob>::jobs.front();
        jId = j.id;
        ccs = *j.job;
        TaskPool<CCJob>::jobs.pop_front();
      }

      if (jId != NO_INST_ASSIGN) {
        ccComputed = Generator::midiCC(ccs);
    
        for (auto &cc : ccComputed) {
          ccMessage[0] = 176+jId;
          ccMessage[1] = cc.ch;
          ccMessage[2] = cc.value;
          midiOut.sendMessage(&ccMessage);
        }
      
        insts.at(jId).ccStep++;
      
        elapsedTime = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now()).time_since_epoch().count();
        this_thread::sleep_for(chrono::milliseconds(CC_RESOLUTION-(elapsedTime-startTime)));
      }
    }
    jId = NO_INST_ASSIGN;
  }
  
  return jId;
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
