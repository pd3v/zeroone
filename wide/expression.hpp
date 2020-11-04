//
//  wide - Live coding DSLish API MIDI sequencer
//
//  Created by @pd3v_
//

#pragma once

#include <vector>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <iterator>
#include <time.h>
#include "instrument.hpp"

extern const int REST_NOTE;
const float PI = 3.14159265;

typedef vector<int> scale, chord, note;
using amp = double;
using dur = int;

template <typename T>
T rnd(const T& max,typename enable_if<!is_floating_point<T>::value,void*>::type() = nullptr) {
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
float rnd(const T& max,typename enable_if<is_floating_point<T>::value,void*>::type() = nullptr) {
  return range<T>(static_cast<T>(rand()%1000),static_cast<T>(1000),0.f,static_cast<float>(max));
};

template <typename T>
T rnd(const T& min,const T& max,typename enable_if<!is_floating_point<T>::value,void*>::type() = nullptr) {
  return max != 0 ? rand()%static_cast<T>(max-min)+min : 0;
};

template <typename T>
float rnd(const T& min,const T& max,typename enable_if<is_floating_point<T>::value,void*>::type() = nullptr) {
  return range<T>(static_cast<T>(rand()%1000),static_cast<T>(1000),static_cast<float>(min),static_cast<float>(max));
};

template <typename T=int>
T rnd(vector<T> bunch) {
  return static_cast<T>(bunch[rand()%bunch.size()]);
}

template <typename T=int>
vector<T> rnd(vector<vector<T>> bunch) {
  return static_cast<vector<T>>(bunch[rand()%bunch.size()]);
}

template <typename T=int>
vector<T> scramble(vector<T> n) {
  random_shuffle(n.begin(),n.end());
  
  return n;
}

template <typename T=int>
vector<vector<T>> scramble(vector<vector<T>> n) {
  random_shuffle(n.begin(),n.end());
  
  return n;
}

template <typename T=int>
vector<T> rndw(T min,T max,T repeatNum,int weight,int size=10) {
  vector<T> wVec(size);
  
  fill(wVec.begin(),wVec.end(),repeatNum);
  transform(wVec.begin(),wVec.begin()+static_cast<int>(size-round(weight/10.)),wVec.begin(),[&](T v){
    return rnd<T>(min,max);
  });
  
  return scramble(wVec);
}

template <typename T=int>
vector<T> rnd25(T min,T max,T repeatNum,int size=10) {
  return rndw(min,max,repeatNum,25,size);
}

template <typename T=int>
vector<T> rnd50(T min,T max,T repeatNum,int size=10) {
  return rndw(min,max,repeatNum,50,size);
}

template <typename T=int>
vector<T> rnd75(T min,T max,T repeatNum,int size=10) {
  return rndw(min,max,repeatNum,75,size);
}

int mod(int countTurn) {
  return countTurn != 0 ? Metro::sync(4)%countTurn : 0;
}

int mod(int countTurn, unsigned long step) {
  return countTurn != 0 ? step%countTurn : 0;
}

bool when(int countTurn, unsigned long step) {
  return countTurn != 0 ? step%countTurn == 0 : false;
}

template <typename T=int>
T thisthat(T _this, T _that, int turn, int every) {
  return (mod(turn) < every ? _this : _that);
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
T cycle(T max) {
  return (max != 0 ? Metro::sync(4)%max : 0);
}

template <typename T=int>
T cycle(T max, unsigned long step) {
  return max != 0 ? step%max : 0;
}

template <typename T=int>
T cycle(T min, T max) {
  return (max != 0 && max-min != 0) ? Metro::sync(4)%(max-min)+min : 0;
}

template <typename T=int>
T cycle(T min, T max, unsigned long step) {
  return (max != 0 && max-min != 0) ? step%(max-min)+min : 0;
}

template <typename T=int>
T cycle(vector<T> v) {
  return v.at(Metro::sync(4)%v.size());
}

template <typename T=int>
T cycle(vector<T> v, unsigned long step) {
  return v.at(step%v.size());
}

template <typename T=int>
vector<T> cycle(vector<vector<T>> v) {
  return v.at(Metro::sync(4)%v.size());
}

template <typename T=int>
vector<T> cycle(vector<vector<T>> v, unsigned long step) {
  return v.at(step%v.size());
}

template <typename T=int>
T rcycle(T max) {
  return max-Metro::sync(4)%max-1;
}

template <typename T=int>
T rcycle(T max, unsigned long step) {
  return max-step%max-1;
}

template <typename T=int>
T rcycle(T min, T max) {
  return (max-1)-(Metro::sync(4)%(max-min));
}

template <typename T=int>
T rcycle(T min, T max, unsigned long step) {
  return (max-1)-(step%(max-min));
}

template <typename T=int>
T rcycle(vector<T> v) {
  return v.at(v.size()-Metro::sync(4)%v.size()-1);
}

template <typename T=int>
T rcycle(vector<T> v, unsigned long step) {
  return v.at(v.size()-step%v.size()-1);
}

template <typename T=int>
vector<T> rcycle(vector<vector<T>> v) {
  return v.at(v.size()-Metro::sync(4)%v.size()-1);
}

template <typename T=int>
vector<T> rcycle(vector<vector<T>> v, unsigned long step) {
  return v.at(v.size()-step%v.size()-1);
}

template <typename T=int>
vector<T> trim(const vector<T>& v, int nValues) {
  vector<T> newVec(v.cbegin(), v.cbegin()+nValues);
  
  return newVec;
}

template <typename T=int>
vector<T> rtrim(const vector<T>& v, int nValues) {
  vector<T> newVec(v.cend()-nValues, v.cend());
  
  return newVec;
}

template <typename T=int>
vector<T> merge(vector<T> v1, vector<T> v2) {
  vector<T> vMerged(v1);
  vMerged.insert(vMerged.cend(),v2.cbegin(),v2.cend());
  
  return vMerged;
}

template <typename T=int>
T edge(T min,T max) {
  T avg = (max-min)/2;
  return Metro::sync(4)%2 == 0 ? cycle(min,avg) : rcycle(avg,max);
}

template <typename T=int>
T edge(vector<T> v) {
  T midSize = v.size()/2;
  return Metro::sync(4)%2 == 0 ? cycle(trim(v,midSize)) : rcycle(rtrim(v,midSize));
}

template <typename T=int>
T edge(T min,T max,unsigned long step) {
  T avg = (max-min)/2;
  return step%2 == 0 ? cycle(min,avg,step) : rcycle(avg,max,step);
}

template <typename T=int>
T edge(vector<T> v,unsigned long step) {
  T midSize = v.size()/2;
  return Metro::sync(4)%2 == 0 ? cycle(trim(v,midSize),step) : rcycle(rtrim(v,midSize),step);
}

template <typename T=int>
T edgex(T min,T max) {
  return Metro::sync(4)%2 == 0 ? cycle(min,max) : rcycle(min,max);
}

template <typename T=int>
T edgex(T min,T max,unsigned long step) {
  return step%2 == 0 ? cycle(min,max,step) : rcycle(min,max,step);
}

template <typename T=int>
T edgex(vector<T> v) {
  return Metro::sync(4)%2 == 0 ? cycle(v) : rcycle(v);
}

template <typename T=int>
T edgex(vector<T> v,unsigned long step) {
  return step%2 == 0 ? cycle(v,step) : rcycle(v,step);
}

template <typename T=int>
T swarm(T value,T spread) {
  int sign = rnd(0,2);
  return sign == 0 ? value-rnd(0,spread) : value+rnd(0,spread);
}

int parts(int _parts,int size=127) {
  return size/_parts;
}

vector<int> chop(int parts,int size=127) {
  vector<int> v;
  
  for (int i = 0;i < parts;++i)
    v.push_back(floor(size/parts*i));
  
  return v;
}

vector<int> chopr(int parts,int size=127) {
  vector<int> v;
  
  for (int i = 1;i <= parts;++i) {
    v.push_back(floor(size/parts*i));
  }
  
  return v;
}

template <typename T=int>
T bounce(T min,T max) {
  return mod(max*2,Metro::sync(4)) < max ? cycle(min,max,Metro::sync(4)) : rcycle(min,max,Metro::sync(4));
}

template <typename T=int>
T bounce(T min,T max,unsigned long step) {
  return mod(max*2,step) < max ? cycle(min,max,step) : rcycle(min,max,step);
}

template <typename T=int>
T bounce(vector<T> v) {
  return mod((v.size()-1)*2,Metro::sync(4)) < v.at(v.size()-1) ? cycle(v) : rcycle(v);
}

template <typename T=int>
T bounce(vector<T> v,unsigned long step) {
  return mod((v.size()-1)*2,step) < v.at(v.size()-1) ? cycle(v,step) : rcycle(v,step);
}

template <typename T=int>
int slow(T value,float xtimes) {
  return floor((static_cast<int>(value)%static_cast<int>(127*xtimes))/xtimes);
}

template <typename T=int>
int fast(T value,float xtimes) {
  return floor(static_cast<int>(value*xtimes)%127);
}

template <typename T=int>
vector<T> rotl(vector<T> v) {
  rotate(v.begin(),v.begin()+mod(v.size(),Metro::sync(1)),v.end());
  
  return v;
}

template <typename T=int>
vector<T> rotr(vector<T> v) {
  rotate(v.begin(),v.begin()+(v.size()-mod(v.size(),Metro::sync(1))),v.end());
  
  return v;
}

vector<int> transp(vector<int> v,int oct) {
  transform(v.begin(),v.end(),v.begin(),[&](int note){return 12*oct+note;});
  
  return v;
}

vector<int> transp(vector<int> v,vector<int> o) {
  transform(v.begin(),v.end(),o.begin(),v.begin(),[&](int note,int oct) {
    auto noteTransp = 12*oct+note;
    return (noteTransp < 0 || noteTransp > 127) ? note : noteTransp;
  });
  
  return v;
}


float sine(int degrees) {
  return fabs(sin(degrees*PI/180));
}

// ====================================

int x = REST_NOTE;

// Scales
scale Chromatic  {0,1,2,3,4,5,6,7,8,9,10,11};
scale Major      {0,2,4,5,7,9,11};
scale Minor      {0,2,3,5,7,8,10};
scale Whole      {0,2,4,6,8,10};

// Chords
chord C{0,4,7}, Cmin{0,3,7}; // just for testing purposes
