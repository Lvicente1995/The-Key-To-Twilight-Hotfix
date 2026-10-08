#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace kingdom {
struct V3 {
    float x{}, y{}, z{};
    V3 operator+(V3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    V3 operator-(V3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    V3 operator*(float s) const { return {x*s,y*s,z*s}; }
    V3 operator/(float s) const { return *this*(1.0f/s); }
    V3& operator+=(V3 b) { x+=b.x; y+=b.y; z+=b.z; return *this; }
    V3& operator-=(V3 b) { x-=b.x; y-=b.y; z-=b.z; return *this; }
};
inline float dot(V3 a,V3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline V3 cross(V3 a,V3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline float length(V3 a) { return std::sqrt(dot(a,a)); }
inline V3 unit(V3 a,V3 fallback={0,-1,0}) { float l=length(a); return l>0.00001f?a/l:fallback; }
inline V3 mix(V3 a,V3 b,float t) { return a*(1-t)+b*t; }

// Positions are centimeters. A fixed 30 Hz game tick is split into four steps.
// Each node represents a rigid link center, with a heavier terminal charm.
class Chain {
public:
    std::vector<V3> points, previous, renderedPrevious;
    std::vector<float> rest;
    bool ready=false;

    void reset(const std::vector<V3>& pose) {
        points=previous=renderedPrevious=pose;
        rest.clear();
        for (size_t i=1;i<pose.size();++i) rest.push_back(std::max(0.01f,length(pose[i]-pose[i-1])));
        ready=pose.size()>1;
    }
    float reach() const { float n=0;for(float d:rest)n+=d;return n; }
    void tick(V3 anchor,V3 body,bool human,float groundY) {
        if(!ready)return;
        renderedPrevious=points;
        V3 oldAnchor=points[0];
        constexpr int substeps=4;
        constexpr float dt=1.0f/(30.0f*substeps);
        for(int sub=0;sub<substeps;++sub) {
            V3 target=mix(oldAnchor,anchor,float(sub+1)/substeps);
            for(size_t i=1;i<points.size();++i) {
                V3 p=points[i];
                V3 velocity=(p-previous[i])*0.989f;
                float maxStep=std::max(rest[i-1]*1.5f,2.0f);
                float speed=length(velocity);
                if(speed>maxStep)velocity=velocity*(maxStep/speed);
                points[i]+=velocity+V3{0,-981.0f*dt*dt,0};
                previous[i]=p;
            }
            for(int iteration=0;iteration<32;++iteration) {
                points[0]=target;
                for(size_t j=1;j<points.size();++j) {
                    size_t i=(iteration&1)?points.size()-j:j;
                    V3 delta=points[i]-points[i-1];
                    float d=length(delta);
                    if(d<0.00001f)continue;
                    V3 correction=delta*((d-rest[i-1])/d);
                    float a=(i==1)?0.0f:1.0f;
                    float b=(i+1==points.size())?0.25f:1.0f;
                    points[i-1]+=correction*(a/(a+b));
                    points[i]-=correction*(b/(a+b));
                }
                // Simple torso and ground contact prevents the charm entering Link.
                // Environment walls are intentionally outside this cosmetic solver.
                if(iteration%4==3)for(size_t i=1;i<points.size();++i) {
                    float radius=i+1==points.size()?2.2f:0.55f;
                    if(std::isfinite(groundY)&&groundY>-100000.0f&&points[i].y<groundY+radius)points[i].y=groundY+radius;
                    if(human) {
                        V3 center{body.x,std::clamp(points[i].y,body.y+48.0f,body.y+112.0f),body.z};
                        V3 away=points[i]-center;
                        float d=length(away), r=14.0f+radius;
                        if(d<r&&d>0.001f)points[i]=center+away*(r/d);
                    }
                }
            }
            points[0]=target;
            // Contacts can conflict with the anchor. Preserve connected links above
            // cosmetic contact accuracy, including sudden animation transitions.
            for(size_t i=1;i<points.size();++i) {
                points[i]=points[i-1]+unit(points[i]-points[i-1])*rest[i-1];
            }
            previous[0]=target;
        }
    }
};
}
