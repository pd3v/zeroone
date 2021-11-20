//
//  wide - Live coding DSLish API + MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once

#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <future>
#include "notes.hpp"

extern const float BAR_DUR_REF; // microseconds
extern const float BPM_REF;
extern const uint16_t NUM_TASKS;

template <typename T>
struct Job {
  int id;
  std::function<T()>* job;
};

template <typename T>
struct TaskPool {
  static std::vector<std::future<int>> tasks;
  static uint16_t numTasks;
  static std::deque<T> jobs;
  static bool isRunning;
  static std::mutex mtx;
  static std::atomic<uint16_t> yieldTaskCntr;
  static Job<T> job;
  
  static void stopRunning() {
    isRunning = false;
    
    for (auto& t : tasks)
      t.get();
    
    tasks.clear();
    jobs.clear();
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
std::atomic<uint16_t> TaskPool<T>::yieldTaskCntr(0);

template <typename T>
bool TaskPool<T>::isRunning = true;

template <typename T>
Job<T> TaskPool<T>::job;

struct SJob : public Job<Notes> {
  int id;
  std::function<Notes()>* job;
};

struct CCJob : public Job<CC> {
  int id;
  std::vector<std::function<CC()>>* job;
};
