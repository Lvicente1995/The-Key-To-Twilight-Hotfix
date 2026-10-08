// Build without the game SDK: cl /nologo /EHsc /std:c++20 /O2 /W4
// tools/kh3_fx_regression.cpp /Fe:kh3_fx_regression.exe
#define KINGDOM_FX_NO_GX
#include "../src/kh3_fx.hpp"
#include <cassert>
#include <iostream>
using namespace kingdom;
using namespace kingdom::kh3fx;
static unsigned long long checks=0;
void check(bool good){++checks;assert(good);}
bool near(float a,float b,float epsilon=1e-4f){return std::abs(a-b)<epsilon;}
bool finite(V3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
void validate(const Frame& frame) {
    check(frame.count<=kCapacity);
    for(std::size_t i=0;i<frame.count;++i) {
        const auto& p=frame.particles[i];check(p.definition<definitions().size());check(finite(p.basis.origin));check(finite(p.size));check(finite(p.color));
        check(finite(p.basis.x)&&finite(p.basis.y)&&finite(p.basis.z));check(std::isfinite(p.alpha)&&p.alpha>=0);check(p.life>0&&p.age>=0&&p.age<p.life);
        check(finite(p.uvOffset));
    }
}
int main() {
    // Original central glow and spiral meshes grow along local +Z. Their
    // source Pitch=-90 must send them toward the Keyblade's +X blade, not
    // backward through the hand. Check all three Unreal rotator conventions.
    const V3 forward=rotation({0,-90,0}).direction({0,0,1});check(near(forward.x,1)&&near(forward.y,0)&&near(forward.z,0));
    const V3 pitch=rotation({0,90,0}).direction({1,0,0});check(near(pitch.x,0)&&near(pitch.y,0)&&near(pitch.z,1));
    const V3 yaw=rotation({0,0,90}).direction({1,0,0});check(near(yaw.x,0)&&near(yaw.y,1)&&near(yaw.z,0));
    const V3 roll=rotation({90,0,0}).direction({0,1,0});check(near(roll.x,0)&&near(roll.y,0)&&near(roll.z,-1));
    // Separate source material inputs: radial emission falloff affects RGB;
    // a white-emission sparkle keeps its RGB even when opacity is faint.
    const auto dual=materialSample(.2f,.6f,1,0,1,0,{0,0,0},{1,1,1});check(near(dual.emission.x,.2f)&&near(dual.opacity,.6f));
    const auto white=materialSample(1,.2f,1,0,1,0,{0,0,0},{1,1,1});check(near(white.emission.x,1)&&near(white.opacity,.2f));
    const auto opacityGain=materialSample(.2f,.2f,1,0,3,0,{.3f,.7f,.9f},{1,1,1});check(near(opacityGain.emission.x,.44f)&&near(opacityGain.opacity,.6f));
    check(definitions()[7].uvScroll&&definitions()[9].uvScroll&&definitions()[5].uvScroll);
    check(near(definitions()[7].uvU.eval(0).x,0)&&near(definitions()[7].uvU.eval(1).x,.5f));
    // Source constants/curves: actual neutral starts, half-range uniforms,
    // locked axes, cubic tangents, duplicate keys, and original child timings.
    check(definitions().size()==23);check(near(definitions()[2].color.eval(0).y,1));
    check(near(definitions()[18].life.eval(0,{0,0,0}).x,.5f));check(near(definitions()[18].life.eval(0,{1,1,1}).x,.7f));
    check(near(definitions()[18].size.eval(0,{.5f,.1f,.9f}).x,definitions()[18].size.eval(0,{.5f,.1f,.9f}).z));
    CurveKey keys[]={{0,{0,0,0,0,0,0},{0,0,0,0,0,0},{2,0,0,0,0,0},1},{1,{1,0,0,0,0,0},{0,0,0,0,0,0},{},0}};
    Distribution cubic{{},{},keys,2};check(near(cubic.eval(.5f).x,.75f));check(near(cubic.eval(-1).x,0));check(near(cubic.eval(2).x,1));
    check(near(definitions()[4].colorScale.eval(0).x,7.5f));
    const auto& app=schedule(0);bool keyFound=false,dotFound=false,lateDotFound=false;unsigned spirals=0;
    for(std::size_t i=0;i<app.count;++i) {const auto& inst=app.entries[i];if(inst.definition==1){check(near(inst.begin,.18311f));keyFound=true;}if(inst.definition==14){check(near(inst.begin,.137383f));dotFound=true;}if(inst.definition==13){check(near(inst.begin,.287383f));lateDotFound=true;}if(inst.definition==7||inst.definition==9)++spirals;}
    check(keyFound&&dotFound&&lateDotFound&&spirals==4);
    System system;Basis basis;basis.origin={20,50,80};system.trigger(basis,true,7);auto summoned=system.snapshot();
    System dismiss;dismiss.trigger(basis,false,7);auto dismissed=dismiss.snapshot();check(summoned.count==dismissed.count);
    for(std::size_t i=0;i<summoned.count;++i){const auto& a=summoned.seeds[i];const auto& b=dismissed.seeds[i];check(a.birth==b.birth&&a.life==b.life&&a.randomSeed==b.randomSeed&&a.instance==b.instance);}
    unsigned keySeeds=0;for(std::size_t i=0;i<summoned.count;++i)if(app.entries[summoned.seeds[i].instance].definition==1){++keySeeds;check(near(summoned.seeds[i].birth,.18311f));}check(keySeeds==1);
    System impact;impact.impact({1,2,3},99);auto hit=impact.snapshot();check(hit.count==9);unsigned fragments=0,glows=0,flashes=0;
    for(std::size_t i=0;i<hit.count;++i){const auto def=schedule(1).entries[hit.seeds[i].instance].definition;fragments+=def==18;glows+=def==16;flashes+=def==17;}check(fragments==7&&glows==1&&flashes==1);
    // Every presentation rate samples the exact same fixed simulation state.
    for(unsigned tick=0;tick<90;++tick){for(unsigned sub=0;sub<=8;++sub)validate(system.snapshot().sample(float(sub)/8));system.tick();}check(!system.active());
    const auto frozen=summoned;system.reset();system.trigger({},true,918);check(frozen.count==summoned.count);check(frozen.seeds[0].randomSeed==summoned.seeds[0].randomSeed);
    // Retained snapshots and sustained conflicting/retriggering events remain
    // bounded and finite; resets, missing/interpolated frames do not mutate them.
    for(unsigned tick=0;tick<1800;++tick){if(tick%3==0)system.impact({float(tick),10,20},tick+1);if(tick%7==0)system.trigger(basis,tick%2!=0,tick+5);basis.origin.y=50+std::sin(float(tick)*.1f)*10;system.updateBasis(basis);auto s=system.snapshot();check(s.count<=kCapacity);for(float a:{0.0f,.25f,.5f,.75f,1.0f})validate(s.sample(a));check(s.count==system.snapshot().count);system.tick();}
    system.reset();check(!system.active()&&system.snapshot().count==0);
    Basis scaled=basis;scaled.x=scaled.x*1.26f;scaled.y=scaled.y*1.26f;scaled.z=scaled.z*1.26f;check(near(length(interpolate(scaled,scaled,.5f).x),1.26f));
    // Exact half-turn cancels both interpolated x/y axes. Fallback directions
    // must be normalized before the separately preserved scale is applied.
    Basis halfTurn=scaled;halfTurn.x=halfTurn.x*-1;halfTurn.y=halfTurn.y*-1;
    for(float t:{0.0f,.5f,1.0f}){const auto b=interpolate(scaled,halfTurn,t);check(near(length(b.x),1.26f));check(near(length(b.y),1.26f));check(near(length(b.z),1.26f));check(near(dot(b.x,b.y),0));check(near(dot(b.y,b.z),0));check(near(dot(b.z,b.x),0));}
    std::cout<<"PASS "<<checks<<" assertions; APP0 seeds="<<summoned.count<<", source child instances="<<app.count<<", hit=1 glow+1 flash+7 fragments; Snapshot="<<sizeof(Snapshot)<<" bytes\n";
}
