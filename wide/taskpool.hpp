//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once

#include <vector>
#include <deque>
#include <mutex>
#include <future>
#include "notes.hpp"

extern const float BAR_DUR_REF; // microseconds
extern const float BPM_REF;
extern const uint16_t NUM_TASKS;

template<typename T>
struct TaskPool {
  static std::vector<std::future<int>> tasks;
  static uint16_t numTasks;
  static std::deque<T> jobs;
  
  static bool isRunning;
  static std::mutex mtx;
  
  static void stopRunning() {
    isRunning = false;
    
    for (auto& t : tasks)
      t.get();
    
    jobs.clear();
    tasks.clear();
  }
};

template <typename T>
std::vector<std::future<int>> TaskPool<T>::tasks{};

template <typename T>
uint16_t TaskPool<T>::numTasks = NUM_TASKS;

template <typename T>
std::deque<T> TaskPool<T>::jobs{};

template <typename T>
std::mutex TaskPool<T>::mtx;

template <typename T>
bool TaskPool<T>::isRunning = true;

struct SJob {
  int id;
  std::function<Notes()>* job;
};

struct CCJob {
  int id;
  std::vector<std::function<CC()>>* job;
};
