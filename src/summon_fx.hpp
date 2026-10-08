#pragma once

// Original procedural recreation of a brief white/cyan Keyblade light burst.
// No Kingdom Hearts particle textures or data are included. Simulation is pure
// 30 Hz code; queued render packets own copies and never advance its clock.
#include "chain_physics.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace kingdom::summonfx {

constexpr std::size_t kCapacity = 40;
constexpr unsigned kDurationTicks = 18;
constexpr float kTickSeconds = 1.0f / 30.0f;
constexpr float kPi = 3.14159265358979323846f;

struct Basis {
    V3 x{1,0,0}, y{0,1,0}, z{0,0,1}, origin{};
    V3 direction(V3 v) const { return x*v.x + y*v.y + z*v.z; }
    V3 point(V3 v) const { return origin + direction(v); }
};

enum class Kind : std::uint8_t { Halo, Star, Mote, Streak };
struct Color { std::uint8_t r=255, g=255, b=255; };

// All fields are values. A Snapshot remains valid after reset/retrigger/unload
// of the simulation that produced it, and can be sampled at any render rate.
struct Seed {
    V3 target{}, spread{}, direction{1,0,0};
    float delay{}, lifetime{}, radius{}, span{}, phase{};
    Kind kind=Kind::Mote;
    Color color{};
};
struct Particle {
    V3 position{}, direction{1,0,0};
    float radius{}, span{}, alpha{}, angle{};
    Kind kind=Kind::Mote;
    Color color{};
};
struct Frame {
    std::array<Particle,kCapacity> particles{};
    std::size_t count{};
};

inline float saturate(float x) { return std::clamp(x,0.0f,1.0f); }
inline float smooth(float x) { x=saturate(x); return x*x*(3.0f-2.0f*x); }

struct Snapshot {
    std::array<Seed,kCapacity> seeds{};
    std::size_t count{};
    unsigned previousTick{}, currentTick{};
    bool summon{};

    // alpha interpolates the two most recent fixed simulation ticks. The
    // caller may use 1 when presentation interpolation is unavailable.
    Frame sample(float alpha=1.0f) const {
        Frame frame;
        const float a=std::isfinite(alpha)?saturate(alpha):1.0f;
        const float time=(float(previousTick)+(float(currentTick)-float(previousTick))*a)*kTickSeconds;
        for(std::size_t i=0;i<std::min(count,kCapacity);++i) {
            const Seed& s=seeds[i];
            const float age=time-s.delay;
            if(age<0.0f || age>=s.lifetime)continue;
            const float t=saturate(age/s.lifetime);
            Particle p;
            p.kind=s.kind;p.color=s.color;p.direction=s.direction;
            p.angle=s.phase+age*(summon?-1.1f:1.1f);
            p.span=s.span;
            const float attack=smooth(age/0.025f);
            if(s.kind==Kind::Halo) {
                // Localized pools of light along the blade, not a screen-sized
                // explosion. The separate solid silhouette supplies the flash.
                p.position=s.target;
                p.radius=s.radius*(0.70f+0.30f*smooth(t));
                p.alpha=attack*std::pow(1.0f-t,1.8f)*0.72f;
            } else {
                const float travel=summon ? 1.0f-smooth(age/std::min(0.24f,s.lifetime*0.65f))
                                           : smooth(t);
                p.position=s.target+s.spread*travel;
                p.radius=s.radius*(0.75f+0.25f*(1.0f-t));
                p.alpha=attack*std::pow(1.0f-t,1.25f);
                if(s.kind==Kind::Star) {
                    const float twinkle=0.70f+0.30f*std::sin(s.phase+age*29.0f);
                    p.alpha*=twinkle;
                    p.radius*=0.82f+0.18f*twinkle;
                } else if(s.kind==Kind::Streak) {
                    p.alpha*=0.62f;
                    p.span*=0.6f+0.4f*(1.0f-t);
                } else p.alpha*=0.86f;
            }
            if(p.alpha>0.0001f)frame.particles[frame.count++]=p;
        }
        return frame;
    }
};

class System {
    Snapshot state_{};
    static std::uint32_t next(std::uint32_t& s) {
        s^=s<<13;s^=s>>17;s^=s<<5;return s;
    }
    static float random(std::uint32_t& s) { return float(next(s)>>8)*(1.0f/16777216.0f); }
public:
    void reset() { state_={}; }
    bool active() const { return state_.count!=0; }
    Snapshot snapshot() const { return state_; }

    // Replaces an earlier burst. Repeated draw/stow transitions cannot grow
    // storage or retain emitters after the actor/scene has gone away.
    void trigger(const Basis& basis,bool summon,std::uint32_t seed=1) {
        state_={};state_.summon=summon;
        if(seed==0)seed=0x9e3779b9u;
        auto add=[&](Kind kind,V3 local,Color color,float radius,float lifetime,float delay,float span) {
            Seed& s=state_.seeds[state_.count++];
            const float angle=random(seed)*2.0f*kPi;
            const float distance=9.0f+random(seed)*10.0f;
            s.target=basis.point(local);
            s.spread=basis.direction({(random(seed)-0.5f)*11.0f,std::cos(angle)*distance,std::sin(angle)*distance});
            s.direction=unit(basis.x,{1,0,0});
            s.delay=delay;s.lifetime=lifetime;s.radius=radius;s.span=span;
            s.phase=random(seed)*2.0f*kPi;s.kind=kind;s.color=color;
        };
        for(unsigned i=0;i<4;++i)
            add(Kind::Halo,{7.0f+float(i)*27.0f,0,0},{182,226,255},8.0f+float(i==1)*2.0f,0.20f,0.0f,0.0f);
        for(unsigned i=0;i<10;++i) {
            const float x=-7.0f+random(seed)*108.0f;
            const float radius=2.1f+random(seed)*2.1f;
            const float lifetime=0.36f+random(seed)*0.18f;
            const float delay=random(seed)*0.025f;
            const Color color=i==0?Color{255,244,214}:Color{226,245,255};
            add(Kind::Star,{x,0,0},color,radius,lifetime,delay,0.0f);
        }
        for(unsigned i=0;i<20;++i) {
            const float x=-12.0f+random(seed)*113.0f;
            const float radius=0.40f+random(seed)*0.65f;
            const float lifetime=0.34f+random(seed)*0.22f;
            const float delay=random(seed)*0.025f;
            add(Kind::Mote,{x,0,0},{105,186,255},radius,lifetime,delay,0.0f);
        }
        for(unsigned i=0;i<5;++i) {
            const float lifetime=0.18f+random(seed)*0.12f;
            const float span=9.0f+random(seed)*8.0f;
            const float offset=(random(seed)-0.5f)*5.0f;
            const float width=0.25f+random(seed)*0.20f;
            add(Kind::Streak,{12.0f+float(i)*18.0f,offset,0},{196,235,255},width,lifetime,0.0f,span);
        }
    }
    void tick() {
        if(!active())return;
        state_.previousTick=state_.currentTick;
        if(state_.currentTick<kDurationTicks)++state_.currentTick;
        else reset();
    }
};

} // namespace kingdom::summonfx

#ifndef KINGDOM_FX_NO_GX
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DShape.h"

namespace kingdom::summonfx {

inline Basis fromMtx(const Mtx m) {
    return {{m[0][0],m[1][0],m[2][0]},
            {m[0][1],m[1][1],m[2][1]},
            {m[0][2],m[1][2],m[2][2]},
            {m[0][3],m[1][3],m[2][3]}};
}

// Queue this in the ordinary scene translucent list. Its projection/scissor
// remain the caller's, so it must not be queued into a shadow or inventory pass.
// Keep the packet alive until that list has been consumed, just like J3DPacket.
class Packet final : public J3DPacket {
    Snapshot data_{};
    float interpolation_=1.0f;
    static V3 transform(const Mtx m,V3 p) {
        return {m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z+m[0][3],
                m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z+m[1][3],
                m[2][0]*p.x+m[2][1]*p.y+m[2][2]*p.z+m[2][3]};
    }
    static void vertex(V3 p,Color color,float alpha) {
        GXPosition3f32(p.x,p.y,p.z);
        GXColor4u8(color.r,color.g,color.b,static_cast<u8>(saturate(alpha)*255.0f+0.5f));
    }
    static void triangle(V3 a,V3 b,V3 c,Color color,float aa,float ab,float ac) {
        vertex(a,color,aa);vertex(b,color,ab);vertex(c,color,ac);
    }
    static void glow(V3 center,float radius,Color color,float alpha) {
        constexpr unsigned segments=12;
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,segments*3);
        for(unsigned i=0;i<segments;++i) {
            const float a=2*kPi*float(i)/segments,b=2*kPi*float(i+1)/segments;
            triangle(center,center+V3{std::cos(a)*radius,std::sin(a)*radius,0},
                     center+V3{std::cos(b)*radius,std::sin(b)*radius,0},color,alpha,0,0);
        }
        GXEnd();
    }
    static void star(V3 center,float radius,float angle,Color color,float alpha) {
        const V3 x{std::cos(angle),std::sin(angle),0},y{-x.y,x.x,0};
        // Four thin rays meet at a small square core, with transparent tips.
        // Distinct cardinal points avoid a circular/hexagonal particle look.
        const float core=radius*0.12f;
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,24);
        for(unsigned i=0;i<4;++i) {
            const V3 axis=i==0?x:i==1?y:i==2?x*-1.0f:y*-1.0f;
            const V3 side{-axis.y,axis.x,0};
            const V3 tip=center+axis*(i%2?radius*1.25f:radius);
            triangle(center,center+side*core,tip,color,alpha,alpha*0.65f,0);
            triangle(center,tip,center-side*core,color,alpha,0,alpha*0.65f);
        }
        GXEnd();
    }
    static void streak(V3 center,V3 direction,float span,float width,Color color,float alpha) {
        V3 axis=unit(V3{direction.x,direction.y,0},{0,1,0});
        const V3 side{-axis.y*width,axis.x*width,0};
        const V3 a=center-axis*span*0.5f,b=center+axis*span*0.5f;
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,12);
        triangle(center,a,center+side,color,alpha,0,0);
        triangle(center,center+side,b,color,alpha,0,0);
        triangle(center,b,center-side,color,alpha,0,0);
        triangle(center,center-side,a,color,alpha,0,0);
        GXEnd();
    }
public:
    void set(const Snapshot& snapshot,float interpolation=1.0f) {
        data_=snapshot;interpolation_=interpolation;
    }
    void draw() override { drawInView(j3dSys.getViewMtx()); }
    void drawInView(const Mtx view) const {
        const Frame frame=data_.sample(interpolation_);
        if(!frame.count)return;
        j3dSys.reinitGX();
        J3DShape::resetVcdVatCache();
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetNumTexGens(0);GXSetNumIndStages(0);GXSetNumTevStages(1);
        GXSetTevDirect(GX_TEVSTAGE0);
        GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP0,GX_TEV_SWAP0);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
        GXSetFog(GX_FOG_NONE,0.0f,1.0f,0.1f,1.0f,{0,0,0,0});
        GXSetZMode(GX_ENABLE,GX_LEQUAL,GX_DISABLE);
        GXSetZCompLoc(GX_TRUE);
        GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_ONE,GX_LO_COPY);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
        GXSetColorUpdate(GX_TRUE);GXSetAlphaUpdate(GX_FALSE);
        GXSetCullMode(GX_CULL_NONE);GXSetCoPlanar(GX_FALSE);GXSetClipMode(GX_CLIP_ENABLE);
        Mtx identity;MTXIdentity(identity);
        GXLoadPosMtxImm(identity,GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
        for(std::size_t i=0;i<frame.count;++i) {
            const Particle& p=frame.particles[i];
            V3 center=transform(view,p.position);
            // A small view-facing offset avoids z-fighting with the weapon's
            // emissive skin, while normal depth testing still hides occlusion.
            center.z+=0.35f;
            switch(p.kind) {
            case Kind::Halo:
                glow(center,p.radius,p.color,p.alpha*0.45f);
                glow(center,p.radius*0.40f,{255,255,255},p.alpha*0.60f);
                break;
            case Kind::Star:
                glow(center,p.radius*0.7f,{138,213,255},p.alpha*0.30f);
                star(center,p.radius,p.angle,p.color,p.alpha);
                break;
            case Kind::Mote:
                glow(center,p.radius*1.8f,p.color,p.alpha*0.34f);
                glow(center,p.radius,{218,244,255},p.alpha);
                break;
            case Kind::Streak: {
                V3 direction=transform(view,p.position+p.direction)-transform(view,p.position);
                streak(center,direction,p.span,p.radius,p.color,p.alpha);
                break;
            }
            }
        }
        // Restore canonical state/cache for the next native material packet.
        j3dSys.reinitGX();
        J3DShape::resetVcdVatCache();
    }
};

} // namespace kingdom::summonfx
#endif
