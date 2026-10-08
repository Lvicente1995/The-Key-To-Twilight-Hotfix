#pragma once
// Original KH3 Cascade timing/distributions and extracted mesh/mask assets.
// The GX renderer approximates Unreal material graphs (Fresnel, erosion, curl,
// scene lights and HDR); it does not claim to reproduce Unreal's renderer.
#include "chain_physics.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace kingdom::kh3fx {
constexpr float kPi=3.14159265358979323846f;
constexpr float kTickSeconds=1.0f/30.0f;
constexpr std::size_t kCapacity=384, kEventCapacity=8;
inline float saturate(float v) { return std::clamp(v,0.0f,1.0f); }
inline V3 product(V3 a,V3 b) { return {a.x*b.x,a.y*b.y,a.z*b.z}; }
struct MaterialSample {V3 emission;float opacity;};
inline MaterialSample materialSample(float emissionMask,float opacityMask,float emissionMultiply,float emissionAdd,float opacityMultiply,float opacityAdd,V3 black,V3 white) {
    // Cooked materials retain the two-color/brightness function parameters,
    // but not node wiring. This affine reconstruction keeps the two sampled
    // inputs independent; it must not force emissive RGB to white or blindly
    // square materials whose emission input is the source white pixel.
    V3 rgb=mix(black,white,emissionMask*emissionMultiply+emissionAdd);
    return {{saturate(rgb.x),saturate(rgb.y),saturate(rgb.z)},saturate(opacityMask*opacityMultiply+opacityAdd)};
}
inline float random(std::uint32_t& state) { state^=state<<13;state^=state>>17;state^=state<<5;return float(state>>8)*(1.0f/16777216.0f); }
inline V3 random3(std::uint32_t& s) { return {random(s),random(s),random(s)}; }
struct Basis {
    V3 x{1,0,0},y{0,0,-1},z{0,1,0},origin{};
    V3 direction(V3 p) const { return x*p.x+y*p.y+z*p.z; }
    V3 point(V3 p) const { return origin+direction(p); }
};
inline Basis rotation(V3 d) {
    d=d*(kPi/180.0f);
    // Source values are Unreal rotators: positive pitch raises +X toward +Z,
    // while positive roll lowers +Y toward -Z. Those two signs differ from a
    // conventional mathematical XYZ Euler matrix; yaw retains its +X -> +Y sign.
    // In particular the source -90-degree pitch must send mesh +Z ALONG +X.
    d.x=-d.x;d.y=-d.y;
    const float cx=std::cos(d.x),sx=std::sin(d.x),cy=std::cos(d.y),sy=std::sin(d.y),cz=std::cos(d.z),sz=std::sin(d.z);
    return {{cy*cz,cy*sz,-sy},{sx*sy*cz-cx*sz,sx*sy*sz+cx*cz,sx*cy},{cx*sy*cz+sx*sz,cx*sy*sz-sx*cz,cx*cy},{}};
}
inline Basis compose(const Basis& a,const Basis& b) { return {a.direction(b.x),a.direction(b.y),a.direction(b.z),a.point(b.origin)}; }
inline Basis interpolate(const Basis& a,const Basis& b,float t) {
    // Gram-Schmidt prevents a sheared emitter during presentation interpolation.
    const V3 bx=unit(b.x,{1,0,0}),by=unit(b.y,{0,1,0}),bz=unit(b.z,{0,0,1});
    V3 x=unit(mix(a.x,b.x,t),bx),y=mix(a.y,b.y,t);y=unit(y-x*dot(x,y),by);
    const float sx=length(a.x)*(1-t)+length(b.x)*t,sy=length(a.y)*(1-t)+length(b.y)*t,sz=length(a.z)*(1-t)+length(b.z)*t;
    return {x*sx,y*sy,unit(cross(x,y),bz)*sz,mix(a.origin,b.origin,t)};
}
struct CurveKey { float time;float value[6],arrive[6],leave[6];int mode; };
struct Distribution {
    V3 lo{},hi{};const CurveKey* keys=nullptr;unsigned count=0;bool uniform=false,lockedXYZ=false;
    V3 eval(float t,V3 r={0.5f,0.5f,0.5f}) const {
        float values[6]={lo.x,lo.y,lo.z,hi.x,hi.y,hi.z};
        if(count) {
            unsigned a=0;while(a+1<count&&keys[a+1].time<=t)++a;
            const CurveKey& p=keys[a];
            if(a+1==count||t<=p.time||p.mode==2)std::copy(p.value,p.value+6,values);
            else {
                const CurveKey& q=keys[a+1];const float dt=q.time-p.time,u=saturate((t-p.time)/std::max(dt,1e-6f));
                for(unsigned c=0;c<6;++c) {
                    if(p.mode==0)values[c]=p.value[c]+(q.value[c]-p.value[c])*u;
                    else { const float u2=u*u,u3=u2*u;
                        values[c]=(2*u3-3*u2+1)*p.value[c]+(u3-2*u2+u)*dt*p.leave[c]+(-2*u3+3*u2)*q.value[c]+(u3-u2)*dt*q.arrive[c];
                    }
                }
            }
        }
        V3 result{values[0],values[1],values[2]};
        if(uniform)result={values[0]+(values[3]-values[0])*r.x,values[1]+(values[4]-values[1])*r.y,values[2]+(values[5]-values[2])*r.z};
        if(lockedXYZ)result.y=result.z=result.x;
        return result;
    }
};
struct EmitterDef {
    const char *name="",*material="",*mesh="";int asset=0;
    bool control=false,additive=false,light=false,attached=false,billboard=false,child=false;
    float delay=0,duration=1;V3 origin{},emitterRotation{};int burst=0;
    Distribution rate{},rateScale{{1,1,1}},life{{.5f,.5f,.5f}},size{{1,1,1}},sizeScale{{1,1,1}},color{{1,1,1}},colorScale{{1,1,1}},alpha{{1,1,1}},alphaScale{{1,1,1}};
    Distribution velocity{},drag{};V3 acceleration{};
    Distribution localPosition{},localRotation{},localScale{{1,1,1}},startRotation{},rotation{};bool directRotation=false;
    Distribution meshRotation{},meshRotationRate{},rangeRadius{},rangeSpeed{};bool range=false;V3 box{};
    Distribution curl{},cameraOffset{},lightColor{{1,1,1}},lightBrightness{{1,1,1}};
    bool uvScroll=false;Distribution uvU{},uvV{},uvPhaseU{},uvPhaseV{};
};
struct ChildDef { int parent,child;float time;V3 position,rotation,scale; };
#include "kh3_fx_data.hpp"

struct Instance { int definition=0,parent=-1;float begin=0;V3 position{},rotation{},scale{1,1,1}; };
struct Schedule {
    std::array<Instance,32> entries{};std::size_t count=0;
    void append(int def,int parent,float begin,V3 pos={},V3 rot={},V3 scale={1,1,1}) {
        if(count==entries.size())return;
        const int index=int(count++);entries[index]={def,parent,begin+definitions()[def].delay,pos,rot,scale};
        for(const auto& child:childDefinitions)if(child.parent==def)
            append(child.child,index,entries[index].begin+child.time,child.position,child.rotation,child.scale);
    }
    explicit Schedule(int asset) { for(std::size_t i=0;i<definitions().size();++i)if(definitions()[i].asset==asset&&!definitions()[i].child)append(int(i),-1,0); }
    Basis local(std::size_t i,float eventAge) const {
        const auto& e=entries[i];const auto& d=definitions()[e.definition];const float age=std::max(0.0f,eventAge-e.begin);
        // The two resonance spiral children are authored as Unreal rotations
        // {pitch=0, yaw=-90, roll=<phase>}.  Treating that tuple as one generic
        // Euler rotation tilts the spiral axis when roll is nonzero.  Apply the
        // -90-degree alignment first, then roll around the already-aligned local
        // spiral axis so its length stays on the Keyblade's +X blade direction.
        Basis b;
        if(e.definition==7||e.definition==9)
            b=compose(rotation({e.rotation.x,e.rotation.y,0}),rotation({0,0,e.rotation.z}));
        else
            b=rotation(e.rotation);
        b.origin=e.position;
        const V3 localRot=d.emitterRotation+d.localRotation.eval(age);
        Basis q;
        if(e.definition==5) {
            // The summon `sho` streak board is authored with {0,-90,-30}:
            // first align its long local axis to the Keyblade, then roll the
            // board around that aligned axis.  Feeding the tuple through the
            // generic Euler path tilts the streaks sideways across Link.
            q=compose(rotation({localRot.x,localRot.y,0}),rotation({0,0,localRot.z}));
        } else {
            q=rotation(localRot);
        }
        q.origin=d.origin+d.localPosition.eval(age);
        const V3 s=product(e.scale,d.localScale.eval(age));q.x=q.x*s.x;q.y=q.y*s.y;q.z=q.z*s.z;
        b=compose(b,q);return e.parent<0?b:compose(local(std::size_t(e.parent),eventAge),b);
    }
};
inline const Schedule& schedule(int asset) { static const Schedule app(0),hit(1),white(2);return asset==0?app:asset==1?hit:white; }
struct Event { Basis previous{},basis{};float start=0,end=0;int asset=0;bool used=false,follow=false; };
struct Seed { std::uint32_t randomSeed=1;float birth=0,life=.5f;std::uint16_t event=0,instance=0; };
struct Particle {
    Basis basis{};V3 size{},color{},rotation{},uvOffset{};float alpha=0,age=0,life=1,cameraOffset=0;
    unsigned definition=0;bool additive=false,billboard=false;
};
struct Frame { std::array<Particle,kCapacity> particles{};std::size_t count=0; };
struct Snapshot {
    std::array<Seed,kCapacity> seeds{};std::array<Event,kEventCapacity> events{};std::size_t count=0;
    float previousTime=0,currentTime=0;
    Frame sample(float interpolation=1) const {
        Frame frame;const float a=std::isfinite(interpolation)?saturate(interpolation):1;
        const float time=previousTime+(currentTime-previousTime)*a;
        for(std::size_t i=0;i<count&&i<kCapacity;++i) {
            const Seed& seed=seeds[i];if(seed.event>=events.size())continue;
            const Event& event=events[seed.event];const float age=time-seed.birth;
            if(!event.used||age<0||age>=seed.life)continue;
            const auto& sch=schedule(event.asset);const auto& inst=sch.entries[seed.instance];const auto& d=definitions()[inst.definition];
            if(d.control||d.light)continue; // Unreal point lights have no equivalent in an isolated GX packet.
            const float t=saturate(age/seed.life),emitterAtBirth=seed.birth-event.start-inst.begin;
            std::uint32_t rng=seed.randomSeed;const V3 r0=random3(rng),r1=random3(rng),r2=random3(rng),r3=random3(rng),r4=random3(rng);
            Basis root=event.follow?interpolate(event.previous,event.basis,a):event.basis;
            const float basisTime=d.attached?time-event.start:seed.birth-event.start;
            Basis b=compose(root,sch.local(seed.instance,basisTime));
            V3 p=product(r1-V3{.5f,.5f,.5f},d.box),v=d.velocity.eval(emitterAtBirth,r2);
            if(d.range) {
                const float z=r1.z*2-1,phi=r1.x*2*kPi,s=std::sqrt(std::max(0.0f,1-z*z));
                const V3 direction{s*std::cos(phi),s*std::sin(phi),z};
                p+=direction*d.rangeRadius.eval(emitterAtBirth,r2).x;v+=direction*d.rangeSpeed.eval(emitterAtBirth,r3).x;
            }
            const float drag=std::max(0.0f,d.drag.eval(emitterAtBirth,r3).x);
            const float movement=drag>1e-5f?(1-std::exp(-drag*age))/drag:age;
            const float gravity=drag>1e-5f?(age-movement)/drag:.5f*age*age;
            b.origin=b.point(p+v*movement);
            // SQEX AccelerationConstant declares world-space Unreal Z gravity.
            b.origin+=V3{d.acceleration.x,d.acceleration.z,-d.acceleration.y}*gravity;
            // Bounded curl approximation; source intensity retained, no invented
            // inward attraction/reversal is added to the original effect.
            const float curl=d.curl.eval(t,r3).x;
            if(curl!=0)b.origin+=root.direction(V3{0,std::sin(age*9+r1.y*6.28f)-std::sin(r1.y*6.28f),std::cos(age*11+r1.z*6.28f)-std::cos(r1.z*6.28f)})*(curl*2);
            V3 rot=d.startRotation.eval(emitterAtBirth,r3)+d.meshRotation.eval(emitterAtBirth,r4)*360;
            rot+=d.rotation.eval(t,r4)*(d.directRotation?1.0f:age);
            rot+=d.meshRotationRate.eval(emitterAtBirth,r0)*(360*age);
            b=compose(b,rotation(rot));
            Particle pOut;pOut.basis=b;pOut.rotation=rot;pOut.definition=unsigned(inst.definition);pOut.age=age;pOut.life=seed.life;
            pOut.size=product(d.size.eval(emitterAtBirth,r0),d.sizeScale.eval(t,r1));
            pOut.color=product(d.color.eval(emitterAtBirth,r2),d.colorScale.eval(t,r3));
            pOut.alpha=std::max(0.0f,d.alpha.eval(emitterAtBirth,r2).x*d.alphaScale.eval(t,r3).x);
            pOut.additive=d.additive;pOut.billboard=d.billboard;pOut.cameraOffset=d.cameraOffset.eval(t,r4).x;
            if(d.uvScroll) {
                // The cooked UV-master describes RG scroll controls and BA
                // spawn phase, but strips its graph connections. This bounded
                // speed*time interpretation preserves the original curves,
                // random phase and lifetime rather than leaving the mask still.
                pOut.uvOffset={d.uvPhaseU.eval(emitterAtBirth,r0).x+d.uvU.eval(t,r1).x*age,
                               d.uvPhaseV.eval(emitterAtBirth,r2).x+d.uvV.eval(t,r3).x*age,0};
            }
            if(pOut.alpha>0.0001f)frame.particles[frame.count++]=pOut;
        }
        return frame;
    }
};
class System {
    Snapshot state_{};
    void addSeed(unsigned event,unsigned instance,float birth,std::uint32_t& seed) {
        if(state_.count==kCapacity)return;
        const auto& inst=schedule(state_.events[event].asset).entries[instance];const auto& d=definitions()[inst.definition];
        const auto randomSeed=seed?seed:0x913fbf83u;V3 r=random3(seed);
        const float life=std::clamp(d.life.eval(birth-state_.events[event].start-inst.begin,r).x,.001f,8.0f);
        state_.seeds[state_.count++]={randomSeed,birth,life,std::uint16_t(event),std::uint16_t(instance)};
        state_.events[event].end=std::max(state_.events[event].end,birth+life);
    }
    void emit(Basis basis,int asset,bool follow,std::uint32_t seed) {
        if(!seed)seed=0x913fbf83u;
        unsigned slot=0;float oldest=1e30f;
        for(unsigned i=0;i<kEventCapacity;++i) {if(!state_.events[i].used){slot=i;oldest=-1;break;}if(state_.events[i].start<oldest){oldest=state_.events[i].start;slot=i;}}
        std::size_t n=0;for(std::size_t i=0;i<state_.count;++i)if(state_.seeds[i].event!=slot)state_.seeds[n++]=state_.seeds[i];state_.count=n;
        auto& event=state_.events[slot];event={basis,basis,state_.currentTime,state_.currentTime,asset,true,follow};
        const auto& sch=schedule(asset);
        for(unsigned index=0;index<sch.count;++index) {
            const auto& inst=sch.entries[index];const auto& d=definitions()[inst.definition];if(d.control||d.light)continue;
            for(int i=0;i<d.burst;++i)addSeed(slot,index,event.start+inst.begin,seed);
            // Integrate the actual source spawn curve at 240 Hz once when an
            // event starts; render frame rate never changes count or RNG order.
            constexpr float dt=1.0f/240.0f;float remainder=0;
            for(float t=0;t<d.duration;t+=dt) {
                const float step=std::min(dt,d.duration-t),rate=std::max(0.0f,d.rate.eval(t+step*.5f).x*d.rateScale.eval(t+step*.5f).x);
                remainder+=rate*step;
                while(remainder>=1) {remainder-=1;addSeed(slot,index,event.start+inst.begin+t+step,seed);}
            }
        }
    }
public:
    void reset() { state_={}; }
    bool active() const { for(const auto& e:state_.events)if(e.used)return true;return false; }
    Snapshot snapshot() const { return state_; }
    void trigger(Basis basis,bool summon,std::uint32_t seed) { // same source APP0 for both directions
        for(auto& e:state_.events)if(e.used&&e.asset==0)e.follow=false;
        emit(basis,0,summon,seed);
    }
    void impact(V3 position,std::uint32_t seed) { Basis b;b.origin=position;emit(b,1,false,seed); }
    void updateBasis(Basis basis) {for(auto& e:state_.events)if(e.used&&e.follow){e.previous=e.basis;e.basis=basis;}}
    void tick() {
        state_.previousTime=state_.currentTime;state_.currentTime+=kTickSeconds;
        for(auto& e:state_.events)if(e.used&&state_.currentTime>e.end+kTickSeconds)e.used=false;
        std::size_t n=0;for(std::size_t i=0;i<state_.count;++i)if(state_.events[state_.seeds[i].event].used&&state_.seeds[i].birth+state_.seeds[i].life>=state_.previousTime)state_.seeds[n++]=state_.seeds[i];state_.count=n;
        if(!active())reset();
    }
};
} // kingdom::kh3fx

#ifndef KINGDOM_FX_NO_GX
#include "mods/svc/resource.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DShape.h"
#include <dolphin/gx/GXExtra.h>
namespace kingdom::kh3fx {
inline Basis fromMtx(const Mtx m,float scale=1.26f) {
    // Both actual Key meshes use +X blade, Y thickness, +Z teeth. Attached
    // weapon-local coordinates therefore match directly; only free world
    // effects/gravity need the Unreal Z-up -> TP Y-up conversion.
    return {V3{m[0][0],m[1][0],m[2][0]}*scale,
            V3{m[0][1],m[1][1],m[2][1]}*scale,
            V3{m[0][2],m[1][2],m[2][2]}*scale,
            {m[0][3],m[1][3],m[2][3]}};
}
struct MaterialDef {
    const char *name,*texture;GXTexWrapMode wrap;float u=1,v=1;
    const char* emission="jtx_1dot_wht_00m";
    float emissionMultiply=1,emissionAdd=0,opacityMultiply=1,opacityAdd=0;
    V3 black{},white{1,1,1};
};
inline const MaterialDef materialDefs[]={
    {"jmi_glt_05k","jtx_glt_05a_clamp",GX_CLAMP},
    {"jmi_glw_01sa","jtx_glw_01a_clamp",GX_CLAMP,1,1,"jtx_glw_01m_clamp"},
    {"jmi_glw_01sk","jtx_glw_01a_clamp",GX_CLAMP,1,1,"jtx_glw_01m_clamp"},
    {"jmi_sho_00a","jtx_sho_00a",GX_REPEAT,3,.4f,"jtx_sho_00m"},
    {"jmi_wht_k","jtx_1dot_wht_00m",GX_REPEAT},
    {"nmi_ora_01sla","stx_ora_01m",GX_REPEAT},
    {"omi_fls_01k","otx_fls_00m",GX_MIRROR,1,1,"otx_grd_01m",-1,0,.3f,0,{1,1,1},{0,0,0}},
    {"smt_frn_01a","jtx_1dot_wht_00m",GX_REPEAT},
    {"tmi_fls_05a","ttx_fls_02a",GX_REPEAT},
    {"zmi_crs_03csk","jtx_crs_03a",GX_MIRROR,2,2,"jtx_crs_03a",1.08801699f,0,3,0,{.25f,.8f,1.13f},{1.1f,1.12f,1.08f}},
    {"zmi_glw_00sk","jtx_glt_05a_clamp",GX_CLAMP,1,1,"jtx_glt_05a_clamp",1,.1f,1,0,{.3f,.7f,.9f},{1,1,1}},
    {"zmi_glw_01sk","ztx_glt_00a",GX_MIRROR,2,2,"ztx_glt_00a",1,.1f,1,0,{.3f,.7f,.9f},{1,1,1}},
    {"zmi_rng_00sa","stx_rng_00a",GX_MIRROR}
};
struct MeshVertex { V3 position{};float u=0,v=0;std::array<u8,4> color{255,255,255,255};V3 normal{}; };
struct Mesh { std::string name;std::vector<MeshVertex> vertices;std::vector<u16> indices; };
struct Texture { std::vector<u8> pixels;GXTexObj object{};bool initialized=false; };
struct Assets { std::vector<Mesh> meshes;std::array<Texture,std::size(materialDefs)> textures;bool ready=false; };
inline Assets assets;
inline void clearAssets() {
    assets.ready=false;
    for(auto& t:assets.textures){if(t.initialized)GXDestroyTexObj(&t.object);t.initialized=false;t.pixels.clear();}
    assets.meshes.clear();
}
struct Reader {
    const u8* data=nullptr;std::size_t size=0,offset=0;bool good=true;
    template<class T>T get() { T value{};if(offset>size||sizeof(T)>size-offset){good=false;return value;}std::memcpy(&value,data+offset,sizeof(T));offset+=sizeof(T);return value; }
    std::string string(std::size_t n) {if(n>size-offset){good=false;return {};}std::string s(reinterpret_cast<const char*>(data+offset),n);offset+=n;return s;}
};
inline bool loadAssets(const ResourceService* service,ModContext* context,std::string& error) {
    clearAssets();if(!service){error="No mod resource service";return false;}
    auto read=[&](const std::string& path,std::vector<u8>& bytes) {
        ResourceBuffer buffer=RESOURCE_BUFFER_INIT;
        if(service->load(context,path.c_str(),&buffer)!=MOD_OK){error="Missing KH3 effect resource: "+path;return false;}
        if(buffer.size&&buffer.data)bytes.assign(static_cast<const u8*>(buffer.data),static_cast<const u8*>(buffer.data)+buffer.size);
        service->free(context,&buffer);return true;
    };
    auto fail=[&](const char* message){error=message;clearAssets();return false;};
    std::vector<u8> bytes;if(!read("kh3fx/meshes.bin",bytes))return false;
    Reader r{bytes.data(),bytes.size()};const auto magic=r.string(8);if(magic!="KKFXM002")return fail("Invalid KH3 effect mesh format");
    const auto meshes=r.get<u32>();if(meshes>32)return fail("Invalid KH3 effect mesh count");
    assets.meshes.reserve(meshes);
    for(u32 i=0;i<meshes;++i) {
        Mesh mesh;const auto n=r.get<u32>();if(n==0||n>128)return fail("Invalid KH3 mesh name");mesh.name=r.string(n);
        const auto nv=r.get<u32>(),ni=r.get<u32>();if(!nv||!ni||nv>65535||ni>200000||ni%3)return fail("Invalid KH3 effect mesh size");
        mesh.vertices.resize(nv);mesh.indices.resize(ni);
        for(auto& v:mesh.vertices){v.position={r.get<float>(),r.get<float>(),r.get<float>()};v.u=r.get<float>();v.v=r.get<float>();for(auto& c:v.color)c=r.get<u8>();
            if(!std::isfinite(v.position.x)||!std::isfinite(v.position.y)||!std::isfinite(v.position.z)||!std::isfinite(v.u)||!std::isfinite(v.v))return fail("Nonfinite KH3 mesh vertex");}
        for(auto& index:mesh.indices){index=r.get<u16>();if(index>=nv)return fail("Invalid KH3 mesh index");}
        if(!r.good)return fail("Truncated KH3 mesh data");
        for(std::size_t k=0;k<ni;k+=3){auto& a=mesh.vertices[mesh.indices[k]];auto& b=mesh.vertices[mesh.indices[k+1]];auto& c=mesh.vertices[mesh.indices[k+2]];V3 normal=cross(b.position-a.position,c.position-a.position);a.normal+=normal;b.normal+=normal;c.normal+=normal;}
        for(auto& v:mesh.vertices)v.normal=unit(v.normal,{1,0,0});assets.meshes.push_back(std::move(mesh));
    }
    if(!r.good||r.offset!=r.size)return fail("Unexpected KH3 mesh payload length");
    struct MaskImage {
        std::vector<u8> bytes;unsigned width=0,height=0;
        float sample(float u,float v) const {
            const float x=u*float(width)-.5f,y=v*float(height)-.5f;
            const int ix=int(std::floor(x)),iy=int(std::floor(y));const float fx=x-float(ix),fy=y-float(iy);
            auto pixel=[&](int px,int py){const std::size_t at=16+(std::size_t(std::clamp(py,0,int(height)-1))*width+unsigned(std::clamp(px,0,int(width)-1)))*4;return float(bytes[at])*float(bytes[at+3])/(255.0f*255.0f);};
            return (pixel(ix,iy)*(1-fx)+pixel(ix+1,iy)*fx)*(1-fy)+(pixel(ix,iy+1)*(1-fx)+pixel(ix+1,iy+1)*fx)*fy;
        }
    };
    auto readMask=[&](const char* name,MaskImage& out) {
        if(!read(std::string("kh3fx/")+name+".rgba",out.bytes))return false;
        Reader tr{out.bytes.data(),out.bytes.size()};if(tr.string(8)!="KKRGBA01"){error="Invalid KH3 mask format";return false;}
        out.width=tr.get<u32>();out.height=tr.get<u32>();
        if(!out.width||!out.height||out.width>2048||out.height>2048||out.bytes.size()!=16+std::size_t(out.width)*out.height*4){error="Invalid KH3 mask size";return false;}return true;
    };
    for(std::size_t i=0;i<std::size(materialDefs);++i) {
        const auto& material=materialDefs[i];MaskImage opacity,emission;
        if(!readMask(material.texture,opacity)||!readMask(material.emission,emission)){clearAssets();return false;}
        const unsigned w=std::max(opacity.width,emission.width),h=std::max(opacity.height,emission.height);
        auto& texture=assets.textures[i];const unsigned bw=(w+3)/4,bh=(h+3)/4;texture.pixels.resize(std::size_t(bw)*bh*64);
        // GX RGBA8 tiles are 4x4 AR then GB planes. Bake the actual separate
        // emission RGB and opacity inputs, retaining the larger source image
        // resolution. The GPU then multiplies particle color and source alpha.
        for(unsigned y=0;y<bh*4;++y)for(unsigned x=0;x<bw*4;++x) {
            const std::size_t tile=(std::size_t(y/4)*bw+x/4)*64,local=((y%4)*4+x%4)*2;
            const float u=(float(std::min(x,w-1))+.5f)/float(w),v=(float(std::min(y,h-1))+.5f)/float(h);
            const auto sample=materialSample(emission.sample(u,v),opacity.sample(u,v),material.emissionMultiply,material.emissionAdd,material.opacityMultiply,material.opacityAdd,material.black,material.white);
            texture.pixels[tile+local]=u8(sample.opacity*255+.5f);texture.pixels[tile+local+1]=u8(sample.emission.x*255+.5f);
            texture.pixels[tile+32+local]=u8(sample.emission.y*255+.5f);texture.pixels[tile+32+local+1]=u8(sample.emission.z*255+.5f);
        }
        GXInitTexObj(&texture.object,texture.pixels.data(),u16(w),u16(h),GX_TF_RGBA8,materialDefs[i].wrap,materialDefs[i].wrap,GX_FALSE);
        GXInitTexObjLOD(&texture.object,GX_LINEAR,GX_LINEAR,0,0,0,GX_FALSE,GX_FALSE,GX_ANISO_1);texture.initialized=true;
    }
    assets.ready=true;error.clear();return true;
}
class Packet final:public J3DPacket {
    Snapshot data_{};float interpolation_=1;
    static V3 direction(const Mtx m,V3 p){return {m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z,m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z,m[2][0]*p.x+m[2][1]*p.y+m[2][2]*p.z};}
    static V3 point(const Mtx m,V3 p){return direction(m,p)+V3{m[0][3],m[1][3],m[2][3]};}
    static u8 channel(float v){return u8(saturate(v)*255+.5f);}
    static void vertex(V3 p,V3 color,float alpha,float u,float v) {GXPosition3f32(p.x,p.y,p.z);GXColor4u8(channel(color.x),channel(color.y),channel(color.z),channel(alpha));GXTexCoord2f32(u,v);}
public:
    void set(const Snapshot& s,float interpolation=1){data_=s;interpolation_=interpolation;}
    void draw() override {drawInView(j3dSys.getViewMtx());}
    void drawInView(const Mtx view) const {
        if(!assets.ready)return;const Frame frame=data_.sample(interpolation_);if(!frame.count)return;
        j3dSys.reinitGX();J3DShape::resetVcdVatCache();GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
        GXSetNumChans(1);GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(1);GXSetTexCoordGen(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY);
        GXSetNumIndStages(0);GXSetNumTevStages(1);GXSetTevDirect(GX_TEVSTAGE0);GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP0,GX_TEV_SWAP0);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);GXSetTevOp(GX_TEVSTAGE0,GX_MODULATE);
        GXSetFog(GX_FOG_NONE,0,1,.1f,1,{0,0,0,0});GXSetZMode(GX_ENABLE,GX_LEQUAL,GX_DISABLE);GXSetZCompLoc(GX_TRUE);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);GXSetColorUpdate(GX_TRUE);GXSetAlphaUpdate(GX_FALSE);GXSetCullMode(GX_CULL_NONE);GXSetCoPlanar(GX_FALSE);GXSetClipMode(GX_CLIP_ENABLE);
        Mtx identity;MTXIdentity(identity);GXLoadPosMtxImm(identity,GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
        for(std::size_t i=0;i<frame.count;++i) {
            const auto& p=frame.particles[i];const auto& def=definitions()[p.definition];
            std::size_t material=0;while(material<std::size(materialDefs)&&std::strcmp(def.material,materialDefs[material].name))++material;
            if(material==std::size(materialDefs))continue;const auto& mat=materialDefs[material];GXLoadTexObj(&assets.textures[material].object,GX_TEXMAP0);
            GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,p.additive?GX_BL_ONE:GX_BL_INVSRCALPHA,GX_LO_COPY);
            // Compress HDR uniformly rather than independently clipping R/G/B;
            // the source's warm white -> pale blue -> deep blue progression survives.
            const float maximum=std::max({p.color.x,p.color.y,p.color.z,.0001f});
            const V3 color=p.color*((1-std::exp(-maximum*.70f))/maximum);
            V3 center=point(view,p.basis.origin);center.z+=p.cameraOffset;
            const Mesh* mesh=nullptr;if(def.mesh[0])for(const auto& candidate:assets.meshes)if(candidate.name==def.mesh){mesh=&candidate;break;}
            if(def.mesh[0]&&!mesh)continue;
            Basis b{direction(view,p.basis.x),direction(view,p.basis.y),direction(view,p.basis.z),center};
            if(p.billboard) {
                const V3 scale{length(b.x),length(b.y),length(b.z)};
                if(p.definition==4) {
                    // hrng000 uses jmd_cir_32x1_00, a YZ-plane ring whose local
                    // +X normal is the Keyblade blade axis.  A generic camera
                    // billboard turns that ring into a screen-space loop and makes
                    // its thin arcs sweep in the wrong direction as Link moves.
                    // Keep the already-composed weapon-local basis so the ring
                    // remains wrapped around/perpendicular to the Keyblade.
                } else if(p.definition==12) { // source Z-camera billboard: retain the long ribbon's axis
                    const V3 z=unit(b.z,{0,1,0}),y=unit(cross(z,V3{0,0,1}),{1,0,0}),x=unit(cross(y,z),{0,0,1});b={x*scale.x,y*scale.y,z*scale.z,center};
                } else {
                    const float a=p.rotation.x*kPi/180;const V3 y{std::cos(a),std::sin(a),0},z{-std::sin(a),std::cos(a),0};
                    b={{0,0,scale.x},y*scale.y,z*scale.z,center};
                }
            }
            if(mesh) {
                const bool fresnel=p.definition==1;
                GXBeginIndexed(GX_VTXFMT0,u16(mesh->vertices.size()),mesh->indices.data(),u32(mesh->indices.size()));
                for(const auto& v:mesh->vertices) {
                    float alpha=p.alpha*float(v.color[3])/255;
                    if(fresnel){const V3 n=unit(b.direction(v.normal),{0,0,1});const float edge=1-std::abs(dot(n,unit(center,{0,0,-1})));alpha*=edge*edge;}
                    vertex(b.point(product(v.position,p.size)),product(color,{v.color[0]/255.0f,v.color[1]/255.0f,v.color[2]/255.0f}),alpha,v.u*mat.u+p.uvOffset.x,v.v*mat.v+p.uvOffset.y);
                }GXEnd();
            } else {
                const float width=p.size.x*.5f,height=p.size.y*.5f;
                const V3 corners[4]={b.point({0,-width,-height}),b.point({0,width,-height}),b.point({0,width,height}),b.point({0,-width,height})};
                constexpr unsigned indices[6]={0,1,2,0,2,3};constexpr float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
                GXBegin(GX_TRIANGLES,GX_VTXFMT0,6);for(unsigned j:indices)vertex(corners[j],color,p.alpha,uv[j][0]*mat.u,uv[j][1]*mat.v);GXEnd();
            }
        }
        j3dSys.reinitGX();J3DShape::resetVcdVatCache();
    }
};
} // kingdom::kh3fx
#endif
