//
//  wide
//
//  Created by @pd3v_
//

#include <vector>
#include <functional>
#include <algorithm>
#include <type_traits>

using namespace std;

extern const int REST_NOTE = 127;
const float PI = 3.14159265;

typedef vector<int> scale, chord, note;
using amp = float;
using dur = int;

template <typename T>
const T rnd(const T& max,typename enable_if<!is_floating_point<T>::value,void*>::type() = nullptr) {
  return max != 0 ? rand()%static_cast<int>(max) : 0;
};

template <typename T,typename U>
typename std::common_type<T,U>::type range(const T& value,const T& max, const U& toMax) {
  return max != 0 ? static_cast<float>(value)/max*toMax : 0;
}

template <typename T,typename U>
typename std::common_type<T,U>::type range(const T& value,const T& max,const U& toMin,const U& toMax) {
  return max != 0 ? static_cast<float>(value)*(toMax-toMin)/max+toMin : 0;
}

template <typename T,typename U>
typename std::common_type<T,U>::type range(const T& value,const T& min,const T& max,const U& toMin,const U& toMax) {
  return max != 0 ? (static_cast<float>(value)-min)*(toMax-toMin)/(max-min)+toMin : 0;
}

template <typename T>
const float rnd(const T& max,typename enable_if<is_floating_point<T>::value,void*>::type() = nullptr) {
  return range<T>(static_cast<T>(rand()%1000),static_cast<T>(1000),0.f,static_cast<float>(max));
};

template <typename T>
const T rnd(const T& min,const T& max,typename enable_if<!is_floating_point<T>::value,void*>::type() = nullptr) {
  return max != 0 ? rand()%static_cast<T>(max-min)+min : 0;
};

template <typename T>
const float rnd(const T& min,const T& max,typename enable_if<is_floating_point<T>::value,void*>::type() = nullptr) {
  return range<T>(static_cast<T>(rand()%1000),static_cast<T>(1000),static_cast<float>(min),static_cast<float>(max));
};

template <typename T=int>
const T rnd(vector<T> bunch) {
  return static_cast<T>(bunch[rand()%bunch.size()]);
}

template <typename T=int>
vector<T> scramble(vector<T> n) {
  random_shuffle(n.begin(),n.end());
  
  return n;
}

int mod(int countTurn, unsigned long step) {
  return countTurn != 0 ? step%countTurn : 0;
}

// If mod is in sync with inst's own step no need to set step param
/*function<int(unsigned long)> mod(int countTurn) {
  return [&](unsigned long step){return countTurn != 0 ? step%countTurn : 0;};
}*/

bool when(int countTurn, unsigned long step) {
  return countTurn != 0 ? step%countTurn == 0 : false;
}

template <typename T=int>
T thisthat(T _this, T _that, function<bool()> pred) {
  return pred() ? _this : _that;
}

template <typename T=int>
T thisthat(T _this, T _that, bool pred) {
  return (pred == true ? _this : _that);
}

template <typename T=int>
T cycle(T max, unsigned long step) {
  return max != 0 ? step%max : 0;
}

template <typename T=int>
T cycle(T min, T max, unsigned long step) {
  return (max != 0 && max-min != 0) ? step%(max-min)+min : 0;
}

template <typename T=int>
T cycle(vector<T> v, unsigned long step) {
  return v.at(step%v.size());
}

template <typename T=int>
T rcycle(T max, unsigned long step) {
  return max-step%max-1;
}

template <typename T=int>
T rcycle(T min, T max, unsigned long step) {
  return (max-1)-(step%(max-min));
}

template <typename T=int>
T rcycle(vector<T> v, unsigned long step) {
  return v.at(v.size()-step%v.size()-1);
}

template <typename T=int>
T edger(T min,T max,unsigned long step) {
  T avg = (max-min)/2;
  return step%2 == 0 ? cycle(min,avg,step) : rcycle(avg,max,step);
}

template <typename T=int>
T edgerx(T min,T max,unsigned long step) {
  return step%2 == 0 ? cycle(min,max,step) : rcycle(min,max,step);
}

template <typename T=int>
T swarm(T value,T spread,unsigned long step) {
  int sign = rnd(0,2);
  return sign == 0 ? value-rnd(0,spread) : value+rnd(0,spread);
}

int chop(int parts,int size=127) {
  return size/parts;
}

template <typename T=int>
T bounce(T min,T max,unsigned long step) {
  return mod(max*2,step) < max ? cycle(min,max,step) : rcycle(min,max,step);
}

template <typename T=int>
int slow(T value,float xtimes) {
  return floor((static_cast<int>(value)%static_cast<int>(127*xtimes))/xtimes);
}

template <typename T=int>
int fast(T value,float xtimes) {
  return floor(static_cast<int>(value*xtimes)%127);
}

vector<int> rotR(vector<int>& notes, vector<int> scale) {
  transform(notes.begin(),notes.end(),notes.begin(),[&scale](int note){
    note += 1;
    note %= scale.size();
    
    return note;
  });
  
  return notes;
}

vector<int> rotL(vector<int>& notes, vector<int> scale) {
  transform(notes.begin(),notes.end(),notes.begin(),[&scale](int note){
    note -= 1;
    note %= static_cast<int>(scale.size());
    
    if (note < 0)
      note = static_cast<int>(scale.size()-1);
  
    return note;
  });

  return notes;
}

// Not viable
/*vector<int> shuffleoct(vector<int> vect,const vector<int> octaves) {
 transform(vect.begin(),vect.end(),vect.begin(),[&octaves](int n){
 n = octaves[rand()%octaves.size()]*12+n;
 return n;
 });
 return vect;
 }*/

float sine(int degrees) {
  return fabs(sin(degrees*PI/180));
}

// ====================================

int x = REST_NOTE;

// Scales
scale Chromatic  {0,1,2,3,4,5,6,7,8,9,10,11};
scale Major      {0,2,4,5,7,9,11};
scale Minor      {0,2,3,5,7,8,10};

// Chords
chord CMaj{0,4,7}, CMin{0,3,7}; // just for testing purposes
