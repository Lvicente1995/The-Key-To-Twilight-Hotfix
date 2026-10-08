// Standalone C++17+ test. No game SDK, import library, or original assets needed.
// Example: c++ -std=c++17 -O2 summon_fx_test.cpp -o summon_fx_test
#define KINGDOM_FX_NO_GX
#include "../src/summon_fx.hpp"
#include "../src/keyblade_presence.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace kingdom;
using namespace kingdom::summonfx;
static unsigned checks=0;
static void require(bool value,const char* reason) {
    ++checks;
    if(!value) { std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1); }
}
static bool close(V3 a,V3 b,float epsilon=0.001f) { return length(a-b)<epsilon; }
static bool finite(V3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
static bool equal(const Frame& a,const Frame& b) {
    if(a.count!=b.count)return false;
    for(std::size_t i=0;i<a.count;++i) {
        const auto& p=a.particles[i];const auto& q=b.particles[i];
        if(!close(p.position,q.position,0.00001f)||p.radius!=q.radius||p.span!=q.span||p.alpha!=q.alpha||p.angle!=q.angle||p.kind!=q.kind||p.color.r!=q.color.r||p.color.g!=q.color.g||p.color.b!=q.color.b)return false;
    }
    return true;
}
int main() {
    static_assert(std::is_trivially_copyable_v<Snapshot>,"Queued effects must own plain value state");
    static_assert(kCapacity<=64,"Effect pool must remain bounded");
    System first,second;
    require(!first.active(),"Starts inactive");
    first.trigger({},true,1234);second.trigger({},true,1234);
    require(first.snapshot().count==39,"Expected bounded emission count");
    for(unsigned tick=0;tick<19;++tick) {
        for(float alpha : {0.0f,0.125f,0.5f,0.875f,1.0f})
            require(equal(first.snapshot().sample(alpha),second.snapshot().sample(alpha)),"Same seed/ticks produce identical trajectories");
        first.tick();second.tick();
    }
    require(!first.active(),"Burst expires after final interpolation interval");
    require(first.snapshot().sample().count==0,"Expired snapshot has no visible particles");

    first.trigger({},false,71);first.tick();first.tick();
    const Snapshot frozen=first.snapshot();
    const Frame frozenFrame=frozen.sample(0.7f);
    require(frozenFrame.count>0,"Burst produces visible particles");
    for(unsigned i=0;i<1000;++i)require(equal(frozen.sample(0.7f),frozenFrame),"Rendering is a pure repeatable read");
    first.reset();first.trigger({},true,99);
    require(equal(frozen.sample(0.7f),frozenFrame),"Queued snapshot survives reset/retrigger");
    require(equal(frozen.sample(-1.0f),frozen.sample(0.0f)),"Negative interpolation clamps");
    require(equal(frozen.sample(2.0f),frozen.sample(1.0f)),"Overshoot interpolation clamps");
    require(equal(frozen.sample(std::numeric_limits<float>::quiet_NaN()),frozen.sample(1.0f)),"Nonfinite interpolation is contained");

    // A rotated/translated weapon basis must move the entire world-space burst,
    // including its trajectories, without affecting timing or relative shape.
    const Basis rotated{{0,0,-1},{0,1,0},{1,0,0},{140,62,-330}};
    first.trigger({},true,845);second.trigger(rotated,true,845);
    for(unsigned tick=0;tick<18;++tick) {
        first.tick();second.tick();
        const auto a=first.snapshot().sample(0.6f),b=second.snapshot().sample(0.6f);
        require(a.count==b.count,"Rigid transform keeps emission timing");
        for(std::size_t i=0;i<a.count;++i)
            require(close(rotated.point(a.particles[i].position),b.particles[i].position),"Particles respect full weapon rotation/translation");
    }

    // Compare every moving seed in isolation before it expires. Dismissal must
    // spread away from the blade; summoning must converge toward it.
    first.trigger({},false,432);
    for(std::size_t i=0;i<first.snapshot().count;++i) {
        const Seed seed=first.snapshot().seeds[i];
        if(seed.kind==Kind::Halo)continue;
        for(bool summon : {false,true}) {
            Snapshot sample;sample.count=1;sample.seeds[0]=seed;sample.summon=summon;
            sample.previousTick=sample.currentTick=2;const Frame early=sample.sample();
            sample.previousTick=sample.currentTick=5;const Frame later=sample.sample();
            require(early.count==1&&later.count==1,"Motion samples within lifetime");
            const float before=length(early.particles[0].position-seed.target);
            const float after=length(later.particles[0].position-seed.target);
            require(summon?after<before:after>before,"Summon converges and dismiss spreads");
        }
    }

    // Stress burst replacement, zero seeds, interpolation and fixed-step expiry.
    unsigned long long sampled=0;
    for(unsigned sequence=0;sequence<2000;++sequence) {
        first.trigger(rotated,(sequence&1)!=0,sequence);
        require(first.snapshot().count<=kCapacity,"Retrigger never grows pool");
        for(unsigned tick=0;tick<19;++tick) {
            const auto frame=first.snapshot().sample(float((tick*7)%11)/10.0f);
            for(std::size_t i=0;i<frame.count;++i) {
                const auto& p=frame.particles[i];++sampled;
                require(finite(p.position)&&finite(p.direction)&&std::isfinite(p.radius)&&std::isfinite(p.span)&&std::isfinite(p.alpha),"Stress sample remains finite");
                require(p.alpha>=0&&p.alpha<=1&&p.radius>0&&p.radius<=10,"Opacity and visual size stay bounded");
                require(length(p.position-rotated.origin)<126,"Burst remains close to weapon");
            }
            first.tick();
        }
        require(!first.active(),"Every stressed burst expires");
    }

    using Presence=KeybladePresence;
    Presence presence;
    require(!presence.visible(),"Reset presence is hidden");
    require(presence.tick(true)==Presence::Event::None,"Initial held state synchronizes without effect");
    require(presence.visible()&&presence.phase()==Presence::Phase::Held,"Initial held model is visible");
    for(unsigned i=0;i<100;++i)require(presence.tick(true)==Presence::Event::None,"Steady equip never repeats effect");
    require(presence.tick(false)==Presence::Event::Dismiss,"Stow edge emits one dismissal");
    for(int i=1;i<Presence::transitionTicks;++i) {
        require(presence.tick(false)==Presence::Event::None,"Vanish hold emits no duplicate");
        require(presence.visible()==(i<Presence::visibleDelayTicks),"Dismiss visibility matches KH3 delay");
    }
    require(presence.tick(false)==Presence::Event::None&&!presence.visible(),"Dismiss resolves hidden");
    require(presence.tick(true)==Presence::Event::Summon,"Draw edge emits one summon");
    require(presence.emission()==0.0f&&!presence.visible(),"Solid weapon waits for the original visibility delay");
    for(int i=0;i<Presence::transitionTicks;++i)presence.tick(true);
    require(presence.visible()&&presence.emission()==0.0f,"Summon resolves into normal held weapon");
    for(unsigned i=0;i<10000;++i) {
        const bool held=(i&1)!=0;
        const auto expected=held?Presence::Event::Summon:Presence::Event::Dismiss;
        require(presence.tick(held)==expected,"Every rapid reversal produces exactly one matching event");
        require(presence.visible()==!held,"Rapid reversal restarts direction-specific visibility delay");
    }
    presence.reset();require(!presence.visible()&&presence.emission()==0,"Reset cancels visible transition");
    require(presence.tick(false)==Presence::Event::None&&!presence.visible(),"Initial stowed state synchronizes without effect");
    std::printf("PASS: %u checks, %llu finite bounded particle samples, 2000 replacement bursts, 10000 presence reversals.\n",checks,sampled);
    std::printf("Historical procedural FX: fixed 30 Hz, burst <=0.60s, 39 owned particles; current KH3 weapon visibility delay verified.\n");
    return 0;
}
