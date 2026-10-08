// C++20 offline exercise of the production skinning path with each native outfit.
// Inputs: xion.mesh private-outfits.bin report.json. No game/GX dependency.
#include "xion_model.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <iomanip>
using namespace kingdom;
using namespace kingdom::xion;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> readFile(const char* path){std::ifstream f(path,std::ios::binary);require(bool(f),"Cannot open input");return {(std::istreambuf_iterator<char>(f)),{}};}
struct Outfit{std::string name;std::array<Matrix,40> local;std::array<int,40> parent;};
struct Pose{const char* name;std::array<V3,40> angles{};V3 translation{};};
Matrix rotate(V3 a){
    Matrix x=identity(),y=x,z=x;
    x.m[1][1]=x.m[2][2]=std::cos(a.x);x.m[1][2]=-std::sin(a.x);x.m[2][1]=std::sin(a.x);
    y.m[0][0]=y.m[2][2]=std::cos(a.y);y.m[0][2]=std::sin(a.y);y.m[2][0]=-std::sin(a.y);
    z.m[0][0]=z.m[1][1]=std::cos(a.z);z.m[0][1]=-std::sin(a.z);z.m[1][0]=std::sin(a.z);
    return multiply(z,multiply(y,x));
}
int main(int argc,char** argv)try{
    require(argc==4,"Usage: xion_outfit_test xion.mesh private-outfits.bin report.json");
    Model model;std::string error;require(decodeModel(readFile(argv[1]),model,error),error.c_str());
    const auto bytes=readFile(argv[2]);Reader r{bytes.data(),bytes.size()};
    require(r.string(8)=="XOUTFIT1"&&r.u32()==4,"Invalid outfit fixture");
    std::array<Outfit,4> outfits;
    for(auto& outfit:outfits){outfit.name=r.text(32);for(int j=0;j<40;++j){outfit.parent[j]=int(r.u32())-1;require(outfit.parent[j]<j,"Invalid parent order");for(auto& row:outfit.local[j].m)for(auto& v:row)v=r.f32();}}
    require(r.good&&r.offset==r.size,"Truncated or trailing outfit data");
    std::vector<Pose> poses(12);
    poses[0].name="bind";
    poses[1].name="walk_left";poses[1].angles[18].x=.65f;poses[1].angles[19].x=-.5f;poses[1].angles[23].x=-.65f;poses[1].angles[24].x=.3f;poses[1].angles[7].x=-.3f;poses[1].angles[12].x=.3f;
    poses[2]=poses[1];poses[2].name="walk_right";for(auto& a:poses[2].angles)a=a*-1;
    poses[3].name="sword_raise";poses[3].angles[6].z=.3f;poses[3].angles[7].z=1.2f;poses[3].angles[8].y=.7f;poses[3].angles[9].x=.4f;
    poses[4].name="shield_raise";poses[4].angles[11].z=-.3f;poses[4].angles[12].z=-1.1f;poses[4].angles[13].y=-.8f;
    poses[5].name="crouch";poses[5].angles[16].x=.2f;for(int j:{18,23})poses[5].angles[j].x=.8f;for(int j:{19,24})poses[5].angles[j].x=-1.2f;
    poses[6].name="climb";poses[6].angles[7].z=1.5f;poses[6].angles[12].z=-1.5f;poses[6].angles[18].x=1;
    poses[7].name="swim";poses[7].angles[0].x=1.4f;poses[7].angles[7].y=.6f;poses[7].angles[12].y=-.6f;poses[7].angles[19].x=.4f;
    poses[8].name="feet_and_coat";for(int j:{20,25})poses[8].angles[j].x=.5f;for(int j=27;j<35;++j)poses[8].angles[j].x=(j%2?.2f:-.2f);
    poses[9].name="head_turn";poses[9].angles[3].y=.3f;poses[9].angles[4].y=.6f;
    poses[10].name="jaw_and_brows";poses[10].angles[36].x=.3f;poses[10].angles[37].z=.1f;poses[10].angles[38].z=-.1f;
    poses[11].name="world_transform";poses[11].angles[0]={.3f,1.1f,-.2f};poses[11].translation={400,-80,220};
    // Every joint gets an isolated pulse as well, including weapon markers and
    // the full face palette. These are synthetic poses, not native BCK frames.
    for(int j=0;j<40;++j){Pose p{};p.name="isolated_joint";p.angles[j]={.17f,-.23f,.31f};poses.push_back(p);}
    float maxBindError=0,maxOutfitPositionDelta=0,maxOutfitNormalDelta=0,maxNormalLengthError=0,maxMotion=0;
    std::size_t samples=0;std::vector<SkinnedVertex> baseline(model.vertexCount);
    for(std::size_t pi=0;pi<poses.size();++pi){const auto& pose=poses[pi];
        for(std::size_t oi=0;oi<outfits.size();++oi){const auto& outfit=outfits[oi];
            std::array<Matrix,40> world,transforms,normals;
            for(int j=0;j<40;++j){auto local=multiply(outfit.local[j],rotate(pose.angles[j]));
                world[j]=outfit.parent[j]<0?local:multiply(world[outfit.parent[j]],local);
                if(outfit.parent[j]<0){world[j].m[0][3]+=pose.translation.x;world[j].m[1][3]+=pose.translation.y;world[j].m[2][3]+=pose.translation.z;}
                transforms[j]=multiply(world[j],model.inverseBind[j]);require(normalTransform(transforms[j],normals[j]),"Singular pose");
            }
            std::size_t vi=0;
            for(const auto& material:model.materials)for(const auto& vertex:material.vertices){const auto v=skin(vertex,transforms,normals);
                require(finite(v.position)&&finite(v.normal),"Nonfinite skinned vertex");
                maxNormalLengthError=std::max(maxNormalLengthError,std::fabs(length(v.normal)-1));
                const float motion=length(v.position-vertex.position);if(pi==0)maxBindError=std::max(maxBindError,motion);else maxMotion=std::max(maxMotion,motion);
                if(oi==0)baseline[vi]=v;else{maxOutfitPositionDelta=std::max(maxOutfitPositionDelta,length(v.position-baseline[vi].position));maxOutfitNormalDelta=std::max(maxOutfitNormalDelta,length(v.normal-baseline[vi].normal));}
                ++vi;++samples;
            }
        }
    }
    require(maxBindError<.003f,"Native outfit bind does not reconstruct exported mesh");
    require(maxOutfitPositionDelta<.001f&&maxOutfitNormalDelta<.0001f,"Outfit-dependent deformation");
    require(maxNormalLengthError<.0001f&&maxMotion>10,"Degenerate pose exercise");
    std::ofstream report(argv[3]);require(bool(report),"Cannot write report");
    report<<std::setprecision(9)<<"{\n  \"result\": \"PASS\",\n  \"scope\": \"Offline synthetic pose exercise using each extracted native outfit skeleton and the production Xion skinning implementation; not a runtime outfit or native animation-clip test.\",\n"
      <<"  \"outfits\": 4,\n  \"body_and_face_joints\": 40,\n  \"poses_per_outfit\": "<<poses.size()<<",\n  \"synthetic_scenario_poses\": 12,\n  \"isolated_joint_poses\": 40,\n  \"vertices_per_pose\": "<<model.vertexCount<<",\n  \"skinned_vertex_samples\": "<<samples<<",\n"
      <<"  \"max_bind_position_error_cm\": "<<maxBindError<<",\n  \"max_outfit_position_delta_cm\": "<<maxOutfitPositionDelta<<",\n  \"max_outfit_normal_delta\": "<<maxOutfitNormalDelta<<",\n  \"max_normal_length_error\": "<<maxNormalLengthError<<",\n  \"max_exercised_displacement_cm\": "<<maxMotion<<"\n}\n";
    std::cout<<"PASS: "<<outfits.size()<<" outfits x "<<poses.size()<<" synthetic poses x "<<model.vertexCount<<" vertices = "<<samples<<" samples. Bind error "<<maxBindError<<" cm; outfit difference "<<maxOutfitPositionDelta<<" cm.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
