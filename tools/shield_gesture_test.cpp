#include "../src/shield_gesture.hpp"
#include <cassert>
#include <iostream>
extern "C" void GXDestroyTexObj(GXTexObj*) {}

// Minimal engine lifecycle stubs let the production track sampler run in a
// standalone executable. These tests do not validate real animation data/GX.
J3DAnmBase::J3DAnmBase():J3DAnmBase(0) {}
J3DAnmBase::J3DAnmBase(s16 n):mAttribute(0),field_0x5(0),mFrameMax(n),mFrame(0) {}
J3DAnmBase::~J3DAnmBase() {}
void J3DAnmBase::setFrame(f32 x) {mFrame=x;}
J3DAnmTransform::J3DAnmTransform(s16 n,f32* s,s16* r,f32* t):J3DAnmBase(n),mScaleData(s),mRotData(r),mTransData(t) {}
J3DFrameCtrl::~J3DFrameCtrl() {}
void J3DFrameCtrl::init(s16 n) {mAttribute=0;mState=0;mStart=0;mEnd=n;mLoop=0;mRate=1;mFrame=0;}
class Fake final:public J3DAnmTransform {
public:
    float bias=0;
    Fake():J3DAnmTransform(30,nullptr,nullptr,nullptr) {}
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        *out={};out->mScale={1,1,1};out->mTranslate={getFrame()+bias,float(joint),0};
    }
};
int main() {
    unsigned checked=0;
    Fake source;source.setFrame(7.25f);
    kingdom::ShieldGesture gesture;
    for(float rate:{0.5f,1.0f,1.4f,2.0f,2.5f,30.0f})for(bool draw:{false,true}) {
        J3DFrameCtrl control;control.mEnd=24;control.mRate=draw?-rate:rate;control.mFrame=draw?24.0f:0.0f;
        assert(gesture.begin(&source,control,draw));
        assert(source.getFrame()==7.25f);
        source.bias=1000; // The captured pose must no longer reference source data.
        const unsigned expectedCount=gesture.count;
        bool priorHand=gesture.onHand();unsigned changes=0,ticks=0;
        while(gesture.active()) {
            assert(gesture.weight()>0&&gesture.weight()<=1);
            assert(gesture.onHand()==(draw?gesture.frame()<=9.5f:gesture.frame()<9.5f));
            changes+=gesture.onHand()!=priorHand;priorHand=gesture.onHand();
            for(unsigned joint=11;joint<=15;++joint) {
                J3DTransformInfo result;gesture.getTransform(joint,&result);
                assert(result.mTranslate.x==gesture.frame());assert(result.mTranslate.y==joint);++checked;
            }
            gesture.advance();++ticks;assert(ticks<132);
        }
        assert(changes==1);assert(priorHand==draw);assert(ticks==expectedCount+gesture.fadeTicks);
        source.bias=0;
    }
    J3DFrameCtrl c;c.mEnd=24;c.mFrame=0;c.mRate=0;
    assert(!gesture.begin(&source,c,false));
    c.mRate=0.01f;assert(!gesture.begin(&source,c,false)); // bounded storage
    c.mRate=1;assert(!gesture.begin(nullptr,c,false));
    assert(gesture.begin(&source,c,false));gesture.reset();assert(!gesture.active());
    std::cout<<"PASS: "<<checked<<" owned joint samples; six rates both directions; exact contact edge; endpoint/fade duration; source-frame restoration; source mutation isolation; cancellation; capacity bounds.\n";
}
