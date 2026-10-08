#pragma once

#include "m_Do/m_Do_ext.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace kingdom {

// Own the five shield-arm tracks before resetUpperAnime releases its animation
// slot. The sword arm (joints 6-10), torso and locomotion are never overridden.
// Human Link's shield branch is shoulder 11, upper arm 12, forearm 13,
// hand 14 and item attachment 15. Wolf/preview/event poses do not use this layer.
class ShieldGesture final : public J3DAnmTransform {
public:
    static constexpr unsigned firstJoint=11, jointCount=5, capacity=128;
    static constexpr unsigned fadeTicks=5;
    static constexpr float playbackSpeed=0.75f;
    struct Sample { float frame{};std::array<J3DTransformInfo,jointCount> joints{}; };
    std::array<Sample,capacity> samples{};
    unsigned count=0,age=0;
    bool drawing=false;

    ShieldGesture():J3DAnmTransform(0,nullptr,nullptr,nullptr) {}
    void reset() {count=0;age=0;}
    bool active() const {return count!=0&&age<count+fadeTicks;}
    bool begin(J3DAnmTransform* source,const J3DFrameCtrl& control,bool draw) {
        reset();drawing=draw;
        const float rate=control.getRate()*playbackSpeed;
        if(!source||!std::isfinite(rate)||std::fabs(rate)<0.001f)return false;
        float frame=control.getFrame();
        const float end=rate<0?float(control.getStart()):float(control.getEnd());
        const float saved=source->getFrame();
        if(!std::isfinite(frame)||!std::isfinite(end))return false;
        for(unsigned i=0;i<capacity;++i) {
            auto& sample=samples[count++];sample.frame=frame;
            source->setFrame(frame);
            for(unsigned j=0;j<jointCount;++j)source->getTransform(firstJoint+j,&sample.joints[j]);
            if(frame==end)break;
            frame=rate<0?std::max(end,frame+rate):std::min(end,frame+rate);
        }
        source->setFrame(saved);
        if(count==capacity&&samples[count-1].frame!=end) {reset();return false;}
        return active();
    }
    void advance() {if(active()&&++age>=count+fadeTicks)reset();}
    float frame() const {return count?samples[std::min(age,count-1)].frame:0.0f;}
    bool onHand() const {return drawing?frame()<=9.5f:frame()<9.5f;}
    float weight() const {
        if(!active())return 0;
        // Native old-frame morphing also blends the first poses. Fade out the
        // independent layer so interruption/completion returns to the base pose.
        if(age<count)return 1;
        return 1.0f-float(age-count+1)/float(fadeTicks+1);
    }
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        if(count&&joint>=firstJoint&&joint<firstJoint+jointCount)
            *out=samples[std::min(age,count-1)].joints[joint-firstJoint];
    }
};

} // namespace kingdom
