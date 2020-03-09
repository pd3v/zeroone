#include <vector>
#include <deque>
#include <mutex>
#include <future>

template<typename T>
struct TaskPool_ {
  TaskPool_() {
    isRunning = true;
  }
  
  static std::vector<std::future<int>> tasks;
  static uint8_t numTasks;
  static std::deque<T> jobs;
  
  static bool isRunning;
  static std::mutex mtx;
  
  static uint16_t step;
  
  static void stopRunning() {
    isRunning = false;
    
    for (auto& t : tasks)
      t.get();
    
    jobs.clear();
    tasks.clear();
  }
};

template <typename T>
std::vector<std::future<int>> TaskPool_<T>::tasks{};

template <typename T>
uint8_t TaskPool_<T>::numTasks = 0;

template <typename T>
std::deque<T> TaskPool_<T>::jobs{};

template <typename T>
bool TaskPool_<T>::isRunning = false;

template <typename T>
std::mutex TaskPool_<T>::mtx;

template <typename T>
uint16_t TaskPool_<T>::step = 1010;


/*class MySeq {
public:
  static int step;
};

int MySeq::step = 0;*/