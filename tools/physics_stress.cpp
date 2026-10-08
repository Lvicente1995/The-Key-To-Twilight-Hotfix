#include "../src/chain_physics.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
#include <limits>
using namespace kingdom;
template<class T>void read(std::ifstream& f,T& t){f.read(reinterpret_cast<char*>(&t),sizeof(t));}
int main(int argc,char**argv){
 std::ifstream f(argv[1],std::ios::binary);char magic[8];f.read(magic,8);int stride=magic[7]=='2'?48:40;uint32_t n;read(f,n);V3 anchor;read(f,anchor);
 std::vector<V3> pose{anchor};
 for(uint32_t i=0;i<n;i++){uint32_t k,nv;read(f,k);f.seekg(k,std::ios::cur);V3 p,t;read(f,p);read(f,t);read(f,nv);f.seekg(nv*stride,std::ios::cur);if(i)pose.push_back(p);}
 for(int scenario=0;scenario<5;++scenario){
  Chain c; auto original=pose;for(auto&p:original)p.y+=130;
  c.reset(original);float maxError=0,maxRatio=0,lastError=0,maxStep=0;int nonfinite=0;V3 a=original[0];
  for(int frame=0;frame<6000;++frame){
   float time=frame/30.f;V3 target=a,body{0,0,0};bool human=false;float ground=-1000000;
   if(scenario==1)target+=V3{50*std::sin(time*6),20*std::sin(time*9),50*std::cos(time*6)};
   if(scenario==2)target+=V3{(frame%120)<60?60.f:0.f,0,0};
   if(scenario==3){ground=0;target.y=2+20*std::abs(std::sin(time));}
   if(scenario==4){human=true;target={5,90,0};}
   auto old=c.points;c.tick(target,body,human,ground);lastError=0;
   for(size_t i=1;i<c.points.size();++i){
    float distance=length(c.points[i]-c.points[i-1]);float error=std::abs(distance-c.rest[i-1]);
    maxError=std::max(maxError,error);maxRatio=std::max(maxRatio,distance/c.rest[i-1]);lastError=std::max(lastError,error);
    maxStep=std::max(maxStep,length(c.points[i]-old[i]));if(!std::isfinite(distance))nonfinite++;
   }
  }
  std::cout<<"scenario "<<scenario<<" max_error "<<maxError<<" max_ratio "<<maxRatio<<" last_error "<<lastError<<" max_move_per_tick "<<maxStep<<" nonfinite "<<nonfinite<<" reach "<<c.reach()<<" final_drop "<<(c.points[0].y-c.points.back().y)<<"\n";
 }
}

