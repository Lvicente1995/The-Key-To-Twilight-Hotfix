#define KINGDOM_FX_NO_GX
#include "../src/kh3_fx.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
using namespace kingdom::kh3fx;
template<class T>T read(std::ifstream& f){T v{};f.read(reinterpret_cast<char*>(&v),sizeof(v));assert(f);return v;}
int main(int argc,char** argv) {
    std::ifstream file(argc>1?argv[1]:"res/kh3fx/meshes.bin",std::ios::binary);assert(file);
    char magic[9]{};file.read(magic,8);assert(std::string(magic)=="KKFXM002");
    std::map<std::string,std::size_t> sizes;const auto count=read<std::uint32_t>(file);
    for(unsigned i=0;i<count;++i){auto length=read<std::uint32_t>(file);std::string name(length,' ');file.read(name.data(),length);auto vertices=read<std::uint32_t>(file),indices=read<std::uint32_t>(file);sizes[name]=vertices;file.seekg(std::streamoff(vertices)*24+std::streamoff(indices)*2,std::ios::cur);assert(file);}
    auto bytes=[&](const Frame& f){std::size_t n=0;for(std::size_t i=0;i<f.count;++i){const auto& d=definitions()[f.particles[i].definition];if(d.mesh[0]){auto mesh=sizes.find(d.mesh);assert(mesh!=sizes.end());n+=mesh->second*24;}else n+=6*24;}return n;};
    for(unsigned mode=0;mode<3;++mode){System system;std::size_t peak=0,maxParticles=0;for(unsigned tick=0;tick<240;++tick){if(mode==0&&tick==0)system.trigger({},true,17);if(mode==1&&tick==0)system.impact({},17);if(mode==2){if(tick%3==0)system.trigger({},bool(tick%2),tick+1);if(tick%2==0)system.impact({},tick+7);}for(unsigned j=0;j<=8;++j){auto f=system.snapshot().sample(float(j)/8);peak=std::max(peak,bytes(f));maxParticles=std::max(maxParticles,f.count);}system.tick();}
        assert(peak<1024*1024);std::cout<<(mode==0?"APP0":mode==1?"HIT0":"retrigger stress")<<": peak "<<peak<<" GX vertex bytes, "<<maxParticles<<" particles\n";
    }
}
