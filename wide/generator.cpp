//
//  wide
//
//  Created by @pd3v_
//

#include "generator.h"
#include <vector>
#include <queue>
//#include <deque>
#include <thread>
#include <utility>
#include <algorithm>

TaskPool Generator::tskPool;
std::shared_ptr<std::vector<std::function<Notes(void)>>> Generator::patterns = std::make_shared<std::vector<std::function<Notes(void)>>>(5);

Generator::Generator() {
  notesQueue.emplace((Notes){{0},0,4,1});
  //queueNote();
  //initPatterns();
  Generator::pushJob();
}

Generator::~Generator() {
  _queueRun = false;
};

void Generator::queueNote() {
  std::thread t([&](){
    while(_queueRun) {
      if (notesQueue.size() < QUEUE_SIZE || notesQueue.empty()) {
        notesQueue.push(midiNote(_f));
        ++step;
      }
      
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });
  t.detach();
}

void Generator::initPatterns() {
  std::cout << __FUNCTION__ << std::endl;
  std::function<Notes(void)> f = []()->Notes{return {{0},0,4,1};};
  for (auto& p : *Generator::patterns)
    p = f;

  
  
  //std::transform(Generator::patterns->begin(),Generator::patterns->end(),[&](std::function<Notes(void)> f){return f;});
}

void Generator::pushJob() {
  std::thread t([&](){
    Job j;
    int id = 0;
    
    while (true) {
      if (tskPool.jobs.size() < 20) {
        id = id%patterns->size();
        j.id = id;
        j.job = &patterns->at(id);
        
        std::cout << "inst id:" << id << " patterns size:" << patterns->size() << std::endl;
        /*Notes n = (*j.job)();
        std::cout << "inst id:" << j.id << " pattern addr:" << &j.job << " dur:" << n.dur << " oct:" << n.oct << std::endl;*/
        
        tskPool.mtx.lock();
        tskPool.jobs.push_back(j);
        tskPool.mtx.unlock();
        
        id++;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });
  t.detach();
}

//int Generator::popJob() {
//  int j;
//  j = Generator::tskPool.jobs.front();
//  Generator::tskPool.jobs.pop_front();
//  
//  return j;
//}

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
    //++step;
  }
  
  return notes;
}

//Notes Generator::notesTskPool() {
//  Job j;
//  
//  if (!Generator::tskPool.jobs.empty()) {
//    Generator::tskPool.mtx.lock();
//    j = Generator::tskPool.jobs.front();
//    Generator::tskPool.jobs.pop_front();
//    Generator::tskPool.mtx.unlock();
//  }
//  
//  return *j.job();
//}


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

void Generator::printNotes(Notes n) {
  std::cout << "{ ";
  for (auto& _n : n.notes)
    std::cout << _n << " ";
  std::cout << "} " << n.amp << " " << n.dur << " " << n.oct << std::endl;
}

void Generator::f(std::string instId, std::function<Notes(void)> f) {
  _f = f;
  patterns->at(stoi(instId)) = f;
  
  for (auto& pattern : *patterns) {
    printNotes(pattern());
    std::cout << &pattern;
  }
  clearNotesQueue();
}

Notes Generator::midiNote(const std::function<Notes(void)>& f) {
  Notes notes = f();
  
  metaNotes = notes.notes;
  transform(notes.notes.begin(), notes.notes.end(), notes.notes.begin(), [&](int n){
    return notes.oct*12+_scale[n%12];
  });
  
  notes.amp = ampToVel(notes.amp);
  notes.dur = (duration[static_cast<int>(notes.dur)]/(_bpm/60.0));
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