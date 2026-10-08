#pragma once
// Portable, bounds-checked XIONM001 data and skinning. No game/GX dependency.
#include "chain_physics.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace kingdom::xion {
constexpr std::size_t kBodyBones=35,kFaceBones=5,kBoneCount=kBodyBones+kFaceBones;
constexpr std::size_t kMaxMaterials=32,kMaxVertices=30000,kMaxIndices=240000;
struct Matrix {float m[3][4]{};};
inline Matrix identity() {Matrix a;for(int i=0;i<3;++i)a.m[i][i]=1;return a;}
inline V3 point(const Matrix& a,V3 p) {return {
    a.m[0][0]*p.x+a.m[0][1]*p.y+a.m[0][2]*p.z+a.m[0][3],
    a.m[1][0]*p.x+a.m[1][1]*p.y+a.m[1][2]*p.z+a.m[1][3],
    a.m[2][0]*p.x+a.m[2][1]*p.y+a.m[2][2]*p.z+a.m[2][3]};}
inline V3 direction(const Matrix& a,V3 p) {return {
    a.m[0][0]*p.x+a.m[0][1]*p.y+a.m[0][2]*p.z,
    a.m[1][0]*p.x+a.m[1][1]*p.y+a.m[1][2]*p.z,
    a.m[2][0]*p.x+a.m[2][1]*p.y+a.m[2][2]*p.z};}
inline Matrix multiply(const Matrix& a,const Matrix& b) {
    Matrix out;
    for(int r=0;r<3;++r)for(int c=0;c<4;++c) {
        for(int k=0;k<3;++k)out.m[r][c]+=a.m[r][k]*b.m[k][c];
        if(c==3)out.m[r][c]+=a.m[r][3];
    }
    return out;
}
inline bool finite(V3 p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline bool finite(const Matrix& a) {
    for(const auto& row:a.m)for(float x:row)if(!std::isfinite(x)||std::fabs(x)>10000000.0f)return false;
    return true;
}
inline bool inverse(const Matrix& a,Matrix& out) {
    const auto& m=a.m;
    const float det=m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])
                   -m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
                   +m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
    if(!std::isfinite(det)||std::fabs(det)<1.0e-8f)return false;
    const float d=1.0f/det;
    out.m[0][0]=(m[1][1]*m[2][2]-m[1][2]*m[2][1])*d;
    out.m[0][1]=(m[0][2]*m[2][1]-m[0][1]*m[2][2])*d;
    out.m[0][2]=(m[0][1]*m[1][2]-m[0][2]*m[1][1])*d;
    out.m[1][0]=(m[1][2]*m[2][0]-m[1][0]*m[2][2])*d;
    out.m[1][1]=(m[0][0]*m[2][2]-m[0][2]*m[2][0])*d;
    out.m[1][2]=(m[0][2]*m[1][0]-m[0][0]*m[1][2])*d;
    out.m[2][0]=(m[1][0]*m[2][1]-m[1][1]*m[2][0])*d;
    out.m[2][1]=(m[0][1]*m[2][0]-m[0][0]*m[2][1])*d;
    out.m[2][2]=(m[0][0]*m[1][1]-m[0][1]*m[1][0])*d;
    const V3 t=direction(out,{m[0][3],m[1][3],m[2][3]})*-1.0f;
    out.m[0][3]=t.x;out.m[1][3]=t.y;out.m[2][3]=t.z;
    return finite(out);
}
inline bool normalTransform(const Matrix& a,Matrix& out) {
    Matrix inv;if(!inverse(a,inv))return false;
    out={};for(int r=0;r<3;++r)for(int c=0;c<3;++c)out.m[r][c]=inv.m[c][r];return true;
}
enum MaterialFlags : std::uint32_t {AlphaTest=1,AlphaBlend=2,TwoSided=4,Unlit=8};
struct Vertex {
    V3 position{},normal{};float u{},v{};
    std::array<std::uint8_t,4> color{255,255,255,255};
    std::array<std::uint16_t,4> bones{};
    std::array<float,4> weights{};
};
static_assert(sizeof(Vertex)==60);
struct Material {
    std::string name,texturePath;
    std::uint32_t flags{};
    std::array<float,4> factor{1,1,1,1};
    float alphaCutoff{};
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
};
struct Model {
    std::array<Matrix,kBoneCount> inverseBind{};
    std::array<Matrix,kBoneCount> bind{};
    std::vector<Material> materials;
    std::size_t vertexCount{},indexCount{};
};
struct Reader {
    const std::uint8_t* bytes{};std::size_t size{},offset{};bool good=true;
    std::uint8_t u8() {if(offset>=size){good=false;return 0;}return bytes[offset++];}
    std::uint16_t u16() {auto a=u8();auto b=u8();return std::uint16_t(a|(unsigned(b)<<8));}
    std::uint32_t u32() {auto a=u16();auto b=u16();return std::uint32_t(a)|(std::uint32_t(b)<<16);}
    float f32() {return std::bit_cast<float>(u32());}
    std::string string(std::size_t n) {
        if(offset>size||n>size-offset){good=false;return {};}
        std::string s(reinterpret_cast<const char*>(bytes+offset),n);offset+=n;return s;
    }
    std::string text(std::size_t maximum) {const auto n=u32();if(!n||n>maximum){good=false;return {};}return string(n);}
};
inline bool safeTexturePath(const std::string& path) {
    if(!path.starts_with("characters/xion/")||path.find("..")!=std::string::npos||!path.ends_with(".rgba"))return false;
    for(unsigned char c:path)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'||c=='/'))return false;
    return true;
}
inline bool decodeModel(const std::vector<std::uint8_t>& bytes,Model& output,std::string& error) {
    auto fail=[&](const char* message){error=message;return false;};
    Reader r{bytes.data(),bytes.size()};Model model;
    if(r.string(8)!="XIONM001"||r.u32()!=kBoneCount)return fail("Invalid Xion model header or joint palette.");
    for(std::size_t i=0;i<kBoneCount;++i) {
        for(auto& row:model.inverseBind[i].m)for(float& v:row)v=r.f32();
        if(!finite(model.inverseBind[i])||!inverse(model.inverseBind[i],model.bind[i]))return fail("Invalid Xion bind matrix.");
    }
    const auto count=r.u32();if(!count||count>kMaxMaterials)return fail("Invalid Xion material count.");
    model.materials.reserve(count);
    for(std::uint32_t n=0;n<count;++n) {
        Material material;material.name=r.text(128);material.texturePath=r.text(200);material.flags=r.u32();
        if(!r.good||material.name.find('\0')!=std::string::npos||!safeTexturePath(material.texturePath)||material.flags&~15u)return fail("Invalid Xion material definition.");
        for(float& f:material.factor){f=r.f32();if(!std::isfinite(f)||f<0||f>4)return fail("Invalid Xion material color.");}
        material.alphaCutoff=r.f32();if(!std::isfinite(material.alphaCutoff)||material.alphaCutoff<0||material.alphaCutoff>1)return fail("Invalid Xion material alpha cutoff.");
        const auto vc=r.u32(),ic=r.u32();
        if(!vc||vc>65535||!ic||ic%3||ic>kMaxIndices||model.vertexCount+vc>kMaxVertices||model.indexCount+ic>kMaxIndices)return fail("Xion geometry exceeds the supported rendering budget.");
        const std::size_t payload=std::size_t(vc)*60+std::size_t(ic)*2;
        if(r.offset>r.size||payload>r.size-r.offset)return fail("Truncated Xion geometry.");
        material.vertices.resize(vc);material.indices.resize(ic);
        for(auto& v:material.vertices) {
            v.position={r.f32(),r.f32(),r.f32()};v.normal={r.f32(),r.f32(),r.f32()};v.u=r.f32();v.v=r.f32();
            for(auto& c:v.color)c=r.u8();for(auto& b:v.bones)b=r.u16();for(float& w:v.weights)w=r.f32();
            if(!finite(v.position)||!finite(v.normal)||length(v.position)>1000||length(v.normal)<0.1f||!std::isfinite(v.u)||!std::isfinite(v.v)||std::fabs(v.u)>256||std::fabs(v.v)>256)return fail("Invalid Xion vertex data.");
            float sum=0;
            for(std::size_t j=0;j<4;++j){if(v.bones[j]>=kBoneCount||!std::isfinite(v.weights[j])||v.weights[j]<0||v.weights[j]>1.001f)return fail("Invalid Xion skin weights.");sum+=v.weights[j];}
            if(std::fabs(sum-1)>0.02f)return fail("Xion skin weights are not normalized.");
            for(float& w:v.weights)w/=sum;v.normal=unit(v.normal);
        }
        for(auto& index:material.indices){index=r.u16();if(index>=vc)return fail("Invalid Xion triangle index.");}
        model.vertexCount+=vc;model.indexCount+=ic;model.materials.push_back(std::move(material));
    }
    if(!r.good||r.offset!=r.size)return fail("Unexpected Xion model payload length.");
    output=std::move(model);error.clear();return true;
}
struct SkinnedVertex {V3 position,normal;};
inline SkinnedVertex skin(const Vertex& v,const std::array<Matrix,kBoneCount>& positions,const std::array<Matrix,kBoneCount>& normals) {
    SkinnedVertex out{};
    for(std::size_t i=0;i<4;++i)if(v.weights[i]>0){out.position+=point(positions[v.bones[i]],v.position)*v.weights[i];out.normal+=direction(normals[v.bones[i]],v.normal)*v.weights[i];}
    out.normal=unit(out.normal,v.normal);return out;
}
struct RgbaImage {std::uint32_t width{},height{};std::vector<std::uint8_t> pixels;};
inline bool decodeImage(const std::vector<std::uint8_t>& bytes,RgbaImage& output,std::string& error) {
    Reader r{bytes.data(),bytes.size()};if(r.string(8)!="KKRGBA01"){error="Invalid Xion texture header.";return false;}
    const auto w=r.u32(),h=r.u32();
    if(!r.good||!w||!h||w>2048||h>2048||bytes.size()!=16+std::size_t(w)*h*4){error="Invalid Xion texture dimensions.";return false;}
    output.width=w;output.height=h;output.pixels.assign(bytes.begin()+16,bytes.end());return true;
}
inline std::vector<std::uint8_t> tileRgba(const RgbaImage& image) {
    const auto bw=(image.width+3)/4,bh=(image.height+3)/4;std::vector<std::uint8_t> out(std::size_t(bw)*bh*64);
    for(std::uint32_t y=0;y<bh*4;++y)for(std::uint32_t x=0;x<bw*4;++x) {
        const auto sy=std::min(y,image.height-1),sx=std::min(x,image.width-1);
        const std::size_t source=(std::size_t(sy)*image.width+sx)*4,tile=(std::size_t(y/4)*bw+x/4)*64,local=((y%4)*4+x%4)*2;
        out[tile+local]=image.pixels[source+3];out[tile+local+1]=image.pixels[source];
        out[tile+32+local]=image.pixels[source+1];out[tile+32+local+1]=image.pixels[source+2];
    }
    return out;
}
} // namespace kingdom::xion
