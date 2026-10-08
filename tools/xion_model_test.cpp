// Standalone C++20 regression checks; no game process or graphics device.
// cl /std:c++20 /EHsc /O2 /I../src xion_model_test.cpp
#include "xion_model.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
using namespace kingdom;
using namespace kingdom::xion;
struct Writer {
    std::vector<std::uint8_t> b;
    void u8(unsigned x){b.push_back(std::uint8_t(x));}
    void u16(unsigned x){u8(x);u8(x>>8);}
    void u32(unsigned x){u16(x);u16(x>>16);}
    void f32(float x){u32(std::bit_cast<std::uint32_t>(x));}
    void raw(const char* s){b.insert(b.end(),s,s+std::strlen(s));}
    void text(const char* s){u32(unsigned(std::strlen(s)));raw(s);}
};
Writer sample() {
    Writer w;w.raw("XIONM001");w.u32(40);
    for(unsigned j=0;j<40;++j)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)w.f32(r==c?1.0f:0.0f);
    w.u32(1);w.text("body");w.text("characters/xion/body.rgba");w.u32(AlphaTest);
    for(int i=0;i<4;++i)w.f32(1);w.f32(.5f);w.u32(3);w.u32(3);
    for(unsigned v=0;v<3;++v) {
        w.f32(v==1?1.0f:0.0f);w.f32(v==2?1.0f:0.0f);w.f32(0);
        w.f32(0);w.f32(0);w.f32(1);w.f32(v==1?1.0f:0.0f);w.f32(v==2?1.0f:0.0f);
        for(int c=0;c<4;++c)w.u8(255);
        for(int j=0;j<4;++j)w.u16(j==0?9:0);
        for(int j=0;j<4;++j)w.f32(j==0?1.0f:0.0f);
    }
    w.u16(0);w.u16(1);w.u16(2);return w;
}
int main(int argc,char** argv) {
    std::string error;Model model;const auto good=sample().b;
    assert(decodeModel(good,model,error));assert(model.vertexCount==3&&model.indexCount==3);
    const auto materialName=model.materials[0].name;
    for(std::size_t n=0;n<good.size();++n) {
        std::vector<std::uint8_t> shortened(good.begin(),good.begin()+n);
        assert(!decodeModel(shortened,model,error));
        assert(model.materials[0].name==materialName); // Failed load is transactional.
    }
    auto invalid=good;invalid.back()=255;assert(!decodeModel(invalid,model,error));
    invalid=good;invalid.push_back(0);assert(!decodeModel(invalid,model,error));
    invalid=good;invalid[8]=39;assert(!decodeModel(invalid,model,error));
    assert(!safeTexturePath("characters/xion/../body.rgba"));assert(!safeTexturePath("C:/body.rgba"));
    assert(!safeTexturePath(std::string("characters/xion/body.rgba")+std::string(1,'\0')+"bad"));
    assert(safeTexturePath("characters/xion/body.rgba"));

    std::mt19937 rng(0x58494f4e);std::uniform_real_distribution<float> angle(-3.14f,3.14f),coordinate(-10000,10000),scale(.2f,3);
    for(unsigned trial=0;trial<3000;++trial) {
        const float a=angle(rng),s=scale(rng);Matrix pose=identity();
        pose.m[0][0]=std::cos(a)*s;pose.m[0][1]=-std::sin(a)*s;pose.m[1][0]=std::sin(a)*s;pose.m[1][1]=std::cos(a)*s;pose.m[2][2]=s;
        for(int r=0;r<3;++r)pose.m[r][3]=coordinate(rng);
        Matrix inv;assert(inverse(pose,inv));const auto ident=multiply(pose,inv);
        for(int r=0;r<3;++r)for(int c=0;c<4;++c)assert(std::fabs(ident.m[r][c]-(r==c?1.0f:0.0f))<.008f);
        std::array<Matrix,kBoneCount> positions,normals;
        for(unsigned j=0;j<kBoneCount;++j){positions[j]=pose;assert(normalTransform(pose,normals[j]));}
        Vertex v=model.materials[0].vertices[1];v.bones={0,7,9,39};v.weights={.1f,.2f,.3f,.4f};
        const auto rendered=skin(v,positions,normals);const auto expected=point(pose,v.position);
        assert(length(rendered.position-expected)<.004f);assert(std::fabs(length(rendered.normal)-1)<1e-5f);
        assert(std::fabs(rendered.normal.z-1)<1e-5f);
    }
    Matrix singular{};Matrix out;assert(!inverse(singular,out));
    RgbaImage source{5,3,std::vector<std::uint8_t>(5*3*4)};
    for(std::size_t i=0;i<source.pixels.size();++i)source.pixels[i]=std::uint8_t(i);
    const auto tiled=tileRgba(source);assert(tiled.size()==128);
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<8;++x) {
        const auto tile=(y/4*2+x/4)*64,local=((y%4)*4+x%4)*2;
        const auto src=(std::min(y,2u)*5+std::min(x,4u))*4;
        assert(tiled[tile+local]==source.pixels[src+3]);assert(tiled[tile+local+1]==source.pixels[src]);
        assert(tiled[tile+32+local]==source.pixels[src+1]);assert(tiled[tile+32+local+1]==source.pixels[src+2]);
    }
    if(argc>1) {
        std::ifstream file(argv[1],std::ios::binary);assert(file);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});
        if(!decodeModel(bytes,model,error)){std::cerr<<error<<'\n';return 1;}
        std::cout<<"Asset PASS: "<<model.materials.size()<<" materials, "<<model.vertexCount<<" unique vertices, "<<model.indexCount/3<<" triangles\n";
    }
    std::cout<<"PASS: all byte-boundary truncations, transactional failure, bad indices/palette/paths, 3000 transformed skinning cases, padded GX RGBA8 tiling\n";
}
