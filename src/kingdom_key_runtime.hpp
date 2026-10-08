#pragma once
// v0.2.3g weapon implementation, isolated without changing its FX, geometry,
// shader, physics or audio algorithms. Included after the host service imports
// and native hook declarations in mod.cpp. Character rendering is independent.
namespace kingdom::keyblade_runtime {
using namespace kingdom;
// Runtime activation gates every weapon override independently of character selection.
bool enabled=false;
bool refreshCollectionLabel=false;
struct Vertex { V3 position,normal; float color[4];float metallic,roughness; };
static_assert(sizeof(Vertex)==48);
struct Part { std::string name;V3 pivot,tangent;std::vector<Vertex> vertices;std::vector<u16> indices; };
std::vector<Part> parts;
V3 localAnchor;
Chain chain;
daAlink_c* owner=nullptr;
J3DModel* ownerModel=nullptr;
bool held=false;
Mtx simulatedParts[32]{};
Mtx simulatedAnchor{};
Mtx dismissedParts[32]{};
V3 lastActorPosition{};
bool hasHeldPose=false;
KeybladePresence presence;
kh3fx::System summonEffects;
audio::System keybladeAudio;
ui::System keybladeUI;
kingdom::text::System keybladeText;
uint32_t effectSeed=1;
ShieldGesture shieldGesture;
daAlink_c* shieldOwner=nullptr;
J3DModel* shieldBody=nullptr;
J3DModel* shieldModel=nullptr;
V3 shieldActorPosition{};
uint64_t gameTick=0;
struct HitStamp {fpc_ProcID actor=0;uint64_t tick=0;bool valid=false;};
std::array<HitStamp,16> hitStamps{};
size_t hitStampIndex=0;

void onItemNameString(ModContext*,void* args,void*,void*) {
    if(!enabled)return;
    const auto destination=mods::arg<TextSpan>(args,1);
    kingdom::text::replacePlainTitle(mods::arg<u32>(args,0),destination.buffer,destination.size);
}

void onItemDescriptionString(ModContext*,void* args,void*,void*) {
    if(!enabled)return;
    // Member hook args: 0=this, 1=message id, 2=primary J2DTextBox.
    const auto messageId=mods::arg<u32>(args,1);
    auto* textBox=mods::arg<J2DTextBox*>(args,2);
    if(!textBox)return;
    const auto destination=textBox->getStringPtr();
    kingdom::text::replacePlainDescription(messageId,destination.buffer,destination.size);
}

J2DPicture* collectionSwordPane(void* args) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(!menu||!menu->mpScreen)return nullptr;
    auto* pane=menu->mpScreen->search(MULTI_CHAR('ken_01'));
    return pane&&pane->getTypeID()==18?static_cast<J2DPicture*>(pane):nullptr;
}
HookAction onCollectionDraw(ModContext*,void* args,void*,void*) {
    if(refreshCollectionLabel) {
        auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
        if(menu&&menu->mpScreen) {
            menu->setItemNameString(menu->getCursorX(),menu->getCursorY());
            refreshCollectionLabel=false;
        }
    }
    keybladeUI.beginCollectionDraw(collectionSwordPane(args),J2DMirror_X);
    return HOOK_CONTINUE;
}
void onCollectionDrawFinished(ModContext*,void* args,void*,void*) {
    keybladeUI.endCollectionDraw(collectionSwordPane(args));
}
J2DPane* hudSwordPane(void* args) {
    auto* meter=mods::arg<dMeter2Draw_c*>(args,0);
    if(!meter||meter->mButtonBItem!=dItemNo_SWORD_e||!meter->mpItemB)return nullptr;
    return meter->mpItemB->getPanePtr();
}
HookAction onHudDraw(ModContext*,void* args,void*,void*) {
    keybladeUI.beginHudDraw(hudSwordPane(args));
    return HOOK_CONTINUE;
}
void onHudDrawFinished(ModContext*,void* args,void*,void*) {
    keybladeUI.endHudDraw(hudSwordPane(args));
}

V3 transform(const Mtx m,V3 p) {
    return {m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z+m[0][3],
            m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z+m[1][3],
            m[2][0]*p.x+m[2][1]*p.y+m[2][2]*p.z+m[2][3]};
}
V3 rotate(const Mtx m,V3 p) {
    return {m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z,
            m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z,
            m[2][0]*p.x+m[2][1]*p.y+m[2][2]*p.z};
}
V3 turn(V3 v,V3 from,V3 to) {
    from=unit(from);to=unit(to);
    float c=std::clamp(dot(from,to),-1.0f,1.0f);
    V3 axis=cross(from,to);
    if(c<-0.9998f) {
        axis=unit(cross(from,std::fabs(from.x)<0.8f?V3{1,0,0}:V3{0,0,1}));
        return axis*(2*dot(axis,v))-v;
    }
    return v+cross(axis,v)+cross(axis,cross(axis,v))*(1.0f/(1.0f+c));
}
void partMatrix(Mtx out,const Mtx base,const Part& part,V3 position,V3 direction) {
    V3 from=rotate(base,part.tangent);
    for(int j=0;j<3;++j) {
        V3 column{base[0][j],base[1][j],base[2][j]};
        V3 r=turn(column,from,direction);
        out[0][j]=r.x;out[1][j]=r.y;out[2][j]=r.z;
    }
    out[0][3]=position.x;out[1][3]=position.y;out[2][3]=position.z;
}
bool active(daAlink_c* link) {
    return enabled&&link&&link->mpSwAModel&&link->mSwordModel==link->mpSwAModel;
}
std::vector<V3> restPose(const Mtx matrix) {
    std::vector<V3> result{transform(matrix,localAnchor)};
    for(size_t i=1;i<parts.size();++i)result.push_back(transform(matrix,parts[i].pivot));
    return result;
}
void fillMatrices(Mtx out[32],const Mtx base,const std::vector<V3>& pose) {
    std::memcpy(out[0],base,sizeof(Mtx));
    for(size_t i=1;i<parts.size();++i) {
        V3 direction=i+1<pose.size()?pose[i+1]-pose[i]:pose[i]-pose[i-1];
        partMatrix(out[i],base,parts[i],pose[i],direction);
    }
}
void clearInterpolation() {
    for(auto& matrix:simulatedParts)svc_interp->forget_mtx(mod_ctx,matrix);
    svc_interp->forget_mtx(mod_ctx,simulatedAnchor);
}
void resetPresence() {
    if(owner||chain.ready)clearInterpolation();
    presence.reset();summonEffects.reset();chain.ready=false;
    owner=nullptr;ownerModel=nullptr;hasHeldPose=false;held=false;
    shieldGesture.reset();shieldOwner=nullptr;shieldBody=nullptr;shieldModel=nullptr;
}
bool replaceEquipGesture(daAlink_c* link) {
    return active(link)&&!link->checkWolf()&&!link->checkEventRun()
        &&!link->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW)
        &&link->mDemo.getDemoMode()!=daPy_demo_c::DEMO_SWORD_UNEQUIP_SP_e;
}
void captureShieldGesture(daAlink_c* link) {
    shieldGesture.reset();shieldOwner=nullptr;
    if(!link->checkShieldGet()||!link->mShieldModel||link->mShieldChangeWaitTimer
        ||link->checkPlayerGuardAndAttack())return;
    if(shieldGesture.begin(link->getNowAnmPackUpper(daAlink_c::UPPER_2),link->mUpperFrameCtrl[2],
                           link->field_0x2fde==0x103)) {
        shieldOwner=link;shieldBody=link->mpLinkModel;shieldModel=link->mShieldModel;
        shieldActorPosition={link->current.pos.x,link->current.pos.y,link->current.pos.z};
    }
}
bool shouldYieldShield(daAlink_c* link) {
    const bool attack=link->mProcID>=daAlink_c::PROC_CUT_NORMAL&&link->mProcID<=daAlink_c::PROC_CUT_LARGE_JUMP_LAND;
    return !replaceEquipGesture(link)||!link->checkShieldGet()
        ||link->checkPlayerGuardAndAttack()||link->checkUpperReadyThrowAnime()
        ||(link->mEquipItem!=0x103&&link->mEquipItem!=dItemNo_NONE_e)
        ||(attack&&link->mEquipItem==0x103)||link->checkCutDashAnime();
}
HookAction onExecuteStart(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    ++gameTick;
    if(!shieldGesture.active())return HOOK_CONTINUE;
    if(link->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW))return HOOK_CONTINUE;
    V3 position{link->current.pos.x,link->current.pos.y,link->current.pos.z};
    if(shieldOwner!=link||shieldBody!=link->mpLinkModel||shieldModel!=link->mShieldModel
        ||shouldYieldShield(link)||length(position-shieldActorPosition)>350) {
        shieldGesture.reset();shieldOwner=nullptr;return HOOK_CONTINUE;
    }
    shieldActorPosition=position;shieldGesture.advance();
    return HOOK_CONTINUE;
}
void onShieldArmCalc(ModContext*,void* args,void*,void*) {
    auto* calc=mods::arg<mDoExt_MtxCalcAnmBlendTblOld*>(args,0);
    const auto joint=J3DMtxCalc::getJoint()->getJntNo();
    // Input can start a guard/attack later in this tick, after execute-pre.
    // Yield before its first skeleton evaluation, not one tick afterwards.
    if(shieldGesture.active()&&shieldOwner&&j3dSys.getModel()==shieldBody
        &&!shieldOwner->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW)
        &&shouldYieldShield(shieldOwner)) {shieldGesture.reset();shieldOwner=nullptr;}
    if(!shieldGesture.active()||!shieldOwner||j3dSys.getModel()!=shieldBody
        ||calc!=shieldOwner->field_0x1f24||calc->mNum<1||calc->mNum>3
        ||shieldOwner->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW)
        ||joint<ShieldGesture::firstJoint||joint>=ShieldGesture::firstJoint+ShieldGesture::jointCount) {
        ShieldArmCalc::g_orig(calc);return;
    }
    // Append a per-joint layer without touching Link's real animation packs or
    // retaining a pointer into an animation heap that another item may reuse.
    mDoExt_AnmRatioPack layers[4];
    const int savedCount=calc->mNum;auto* savedPacks=calc->mAnmRatio;
    for(int i=0;i<savedCount;++i)layers[i]=savedPacks[i];
    layers[savedCount].setAnmTransform(&shieldGesture);
    layers[savedCount].setRatio(shieldGesture.weight());
    calc->mNum=savedCount+1;calc->mAnmRatio=layers;
    ShieldArmCalc::g_orig(calc);
    calc->mNum=savedCount;calc->mAnmRatio=savedPacks;
}
HookAction onShieldModelCalc(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    auto* model=mods::arg<J3DModel*>(args,1);
    if(!shieldGesture.active()||link!=shieldOwner||model!=shieldModel
        ||link->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW)
        ||link->mpLinkModel!=shieldBody)return HOOK_CONTINUE;
    if(shouldYieldShield(link)) {shieldGesture.reset();shieldOwner=nullptr;return HOOK_CONTINUE;}
    // The sword's state already changed, but the physical shield transfers at
    // the native hand/back contact frame while the real shield arm reaches it.
    if(shieldGesture.onHand())model->setBaseTRMtx(link->mpLinkModel->getAnmMtx(link->mRightItemJntNo));
    else {
        Mtx back,offset;
        MTXTrans(offset,4.2f,-4.4f,-20.0f);
        MTXConcat(link->mpLinkModel->getAnmMtx(link->field_0x30b6),offset,back);
        mDoMtx_XYZrotM(back,cM_deg2s(91.0f),cM_deg2s(57.0f),cM_deg2s(180.0f));
        model->setBaseTRMtx(back);
    }
    return HOOK_CONTINUE;
}
void onKeybladeHit(V3 position) {
    // Integration point for the impact particles/audio. A real nonblocked
    // enemy contact has been accepted and deduplicated; no collision is edited.
    Vec point{position.x,position.y,position.z};
    summonEffects.impact(position,effectSeed++);
    // The seven entries are the Kingdom Key's original hit sound family.
    const auto cue=static_cast<audio::Cue>(static_cast<unsigned>(audio::Cue::Hit)+(effectSeed++%7));
    keybladeAudio.play(cue,&point);
}
HookAction onPrepareKeybladeSound(ModContext*,void* args,void* result,void*) {
    if(!keybladeAudio.prepareSequence(mods::arg<JAISe*>(args,0)))return HOOK_CONTINUE;
    *static_cast<bool*>(result)=true;return HOOK_SKIP_ORIGINAL;
}
HookAction onSwordSound(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!replaceEquipGesture(link))return HOOK_CONTINUE;
    const auto id=mods::arg<u32>(args,1);
    if((id==0x20000&&keybladeAudio.ready(audio::Cue::Summon))
        ||(id==0x20001&&keybladeAudio.ready(audio::Cue::Dismiss)))return HOOK_SKIP_ORIGINAL;
    return HOOK_CONTINUE;
}
HookAction onSwordHitmark(ModContext*,void* args,void*,void*) {
    if(!mods::arg<bool>(args,1)||!mods::arg<bool>(args,2)||mods::arg<bool>(args,12))return HOOK_CONTINUE;
    auto* at=mods::arg<dCcD_GObjInf*>(args,5);
    auto* target=mods::arg<dCcD_GObjInf*>(args,6);
    auto* targetStatus=mods::arg<dCcD_GStts*>(args,10);
    auto* point=mods::arg<cXyz*>(args,11);
    auto* link=static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if(!at||!target||!targetStatus||!point||!active(link)||link->checkWolf()
        ||link->mEquipItem!=0x103||at->GetAc()!=link
        ||!(at->GetAtType()&AT_TYPE_NORMAL_SWORD)||at->GetAtAtp()==0
        ||!daAlink_c::checkEnemyGroup(target->GetAc())
        ||at->ChkAtNoHitMark()||target->ChkTgNoHitMark()
        ||target->GetTgSpl()==dCcG_Tg_Spl_UNK_1||target->GetTgHitMark()==8
        ||!targetStatus->ChkNoneActorPerfTblId())return HOOK_CONTINUE;
    V3 position{point->x,point->y,point->z};
    if(!std::isfinite(position.x)||!std::isfinite(position.y)||!std::isfinite(position.z))return HOOK_CONTINUE;
    const auto id=fopAcM_GetID(target->GetAc());
    for(const auto& stamp:hitStamps)if(stamp.valid&&stamp.actor==id&&stamp.tick==gameTick)return HOOK_SKIP_ORIGINAL;
    hitStamps[hitStampIndex++%hitStamps.size()]={id,gameTick,true};
    onKeybladeHit(position);
    // This native function only draws the generic hit mark. Damage, collision
    // and enemy reaction handling live outside it and continue unchanged.
    return HOOK_SKIP_ORIGINAL;
}
void finishNativeItemChange(daAlink_c* link) {
    // Keep native item cleanup, flags and sword setup, then finish the upper
    // animation before the reach-behind pose is evaluated by the skeleton.
    captureShieldGesture(link);
    link->commonChangeItem();
    link->resetUpperAnime(daAlink_c::UPPER_2,3.0f);
    link->setHorseSwordUp(1);
}
void onSwordEquip(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(replaceEquipGesture(link)&&link->field_0x2fde==0x103&&link->mEquipItem!=0x103)
        finishNativeItemChange(link);
}
void onSwordUnequip(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(replaceEquipGesture(link)&&link->mEquipItem==0x103) {
        // allUnequip normally assigns this AFTER swordUnequip returns.
        link->field_0x2fde=dItemNo_NONE_e;
        finishNativeItemChange(link);
    }
}
HookAction onSwordUnequipSpecial(ModContext*,void* args,void* result,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!replaceEquipGesture(link)||link->mEquipItem!=0x103)return HOOK_CONTINUE;
    link->mSwordFlourishTimer=0;
    link->swordUnequip();
    *static_cast<int*>(result)=1;
    return HOOK_SKIP_ORIGINAL;
}
void onExecute(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!active(link)||parts.empty()||link->checkWolf()) {resetPresence();return;}
    auto* model=link->mpSwAModel;
    auto* matrix=model->getBaseTRMtx();
    bool nowHeld=link->mEquipItem==0x103;
    V3 actorPosition{link->current.pos.x,link->current.pos.y,link->current.pos.z};
    bool discontinuity=owner!=link||ownerModel!=model||length(actorPosition-lastActorPosition)>350.0f;
    if(discontinuity) {
        clearInterpolation();chain.ready=false;hasHeldPose=false;
        presence.synchronize(nowHeld);summonEffects.reset();
    }
    summonEffects.tick();
    auto event=presence.tick(nowHeld);
    // Scripted events retain their native animation timing and never trigger
    // cosmetic bursts while the game is setting up a scene or equipment preview.
    if(link->checkEventRun()) {
        presence.synchronize(nowHeld);summonEffects.reset();
        event=KeybladePresence::Event::None;
    }
    if(event==KeybladePresence::Event::Dismiss) {
        if(hasHeldPose) {
            std::memcpy(dismissedParts,simulatedParts,sizeof(dismissedParts));
            summonEffects.trigger(kh3fx::fromMtx(dismissedParts[0]),false,effectSeed++);
            keybladeAudio.play(audio::Cue::Dismiss);
        } else presence.synchronize(false);
    }
    owner=link;ownerModel=model;lastActorPosition=actorPosition;
    if(!nowHeld) {
        if(chain.ready)clearInterpolation();
        chain.ready=false;held=false;
        return;
    }
    V3 anchor=transform(matrix,localAnchor);
    bool reset=!chain.ready||!held||discontinuity;
    if(!reset&&length(anchor-chain.points[0])>chain.reach()*3.0f+100.0f)reset=true;
    if(reset) {
        clearInterpolation();
        chain.reset(restPose(matrix));
    }
    else chain.tick(anchor,actorPosition,true,link->mLinkAcch.GetGroundH());
    held=true;
    fillMatrices(simulatedParts,matrix,chain.points);
    hasHeldPose=true;
    summonEffects.updateBasis(kh3fx::fromMtx(matrix));
    MTXIdentity(simulatedAnchor);
    simulatedAnchor[0][3]=anchor.x;simulatedAnchor[1][3]=anchor.y;simulatedAnchor[2][3]=anchor.z;
    // Each rigid piece uses the host's supported interpolation service. Physics
    // advances only on simulation ticks; presentation frames read these matrices.
    for(size_t i=0;i<parts.size();++i)svc_interp->record_mtx(mod_ctx,simulatedParts[i]);
    svc_interp->record_mtx(mod_ctx,simulatedAnchor);
    if(event==KeybladePresence::Event::Summon) {
        summonEffects.trigger(kh3fx::fromMtx(matrix),true,effectSeed++);
        keybladeAudio.play(audio::Cue::Summon);
    }
}

u8 byte(float v) {return static_cast<u8>(std::clamp(v,0.0f,255.0f));}
void loadLight(const J3DLightInfo& info,unsigned index) {
    GXLightObj light;
    GXInitLightPos(&light,info.mLightPosition.x,info.mLightPosition.y,info.mLightPosition.z);
    GXInitLightDir(&light,info.mLightDirection.x,info.mLightDirection.y,info.mLightDirection.z);
    GXInitLightColor(&light,info.mColor);
    GXInitLightAttn(&light,info.mCosAtten.x,info.mCosAtten.y,info.mCosAtten.z,
                   info.mDistAtten.x,info.mDistAtten.y,info.mDistAtten.z);
    GXLoadLightObjImm(&light,static_cast<GXLightID>(1u<<index));
}
class KeybladePacket final : public J3DPacket {
public:
    Mtx matrices[32]{};
    Mtx lightViewInverse{};
    dKy_tevstr_c lighting;
    bool interpolate=false;
    bool castsShadow=true;
    void presentationMatrices(Mtx out[32]) const {
        for(size_t i=0;i<parts.size();++i) {
            if(!interpolate||!svc_interp->lookup_replacement_mtx(simulatedParts[i],out[i]))
                std::memcpy(out[i],matrices[i],sizeof(Mtx));
        }
        // Rotational interpolation traces an arc at the pommel, while the chain
        // centers trace straight lines. Keep that interpolated root attached.
        Mtx anchor;
        if(interpolate&&svc_interp->lookup_replacement_mtx(simulatedAnchor,anchor)) {
            V3 correction=transform(out[0],localAnchor)-V3{anchor[0][3],anchor[1][3],anchor[2][3]};
            for(size_t i=1;i<parts.size();++i) {
                out[i][0][3]+=correction.x;out[i][1][3]+=correction.y;out[i][2][3]+=correction.z;
            }
        }
    }
    void draw() override {drawInView(j3dSys.getViewMtx());}
    void drawInView(const Mtx drawView) {
        if(parts.empty())return;
        Mtx world[32];presentationMatrices(world);
        j3dSys.reinitGX();
        J3DShape::resetVcdVatCache();
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR1,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR1,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetNumChans(2);
        GXSetChanAmbColor(GX_COLOR0A0,{byte(lighting.AmbCol.r),byte(lighting.AmbCol.g),byte(lighting.AmbCol.b),255});
        // The original Ordon material enables all eight light slots. The sun,
        // moon and room lights occupy slots 2-7, not the actor's primary slot 0.
        GXSetChanCtrl(GX_COLOR0,GX_ENABLE,GX_SRC_REG,GX_SRC_VTX,0xff,GX_DF_CLAMP,GX_AF_SPOT);
        GXSetChanCtrl(GX_ALPHA0,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1,GX_DISABLE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
        V3 lightPosition=transform(drawView,{lighting.field_0x32c.x,lighting.field_0x32c.y,lighting.field_0x32c.z});
        J3DLightInfo renderLights[7];
        renderLights[0]=lighting.mLightObj.mInfo;
        renderLights[0].mLightPosition={lightPosition.x,lightPosition.y,lightPosition.z};
        Mtx lightViewTransform;MTXConcat(drawView,lightViewInverse,lightViewTransform);
        for(unsigned i=1;i<7;++i) {
            renderLights[i]=lighting.mLights[i-1].mInfo;
            const auto& info=lighting.mLights[i-1].mInfo;
            V3 position=transform(lightViewTransform,{info.mLightPosition.x,info.mLightPosition.y,info.mLightPosition.z});
            V3 direction=rotate(lightViewTransform,{info.mLightDirection.x,info.mLightDirection.y,info.mLightDirection.z});
            renderLights[i].mLightPosition={position.x,position.y,position.z};
            renderLights[i].mLightDirection={direction.x,direction.y,direction.z};
        }
        dKy_setLight_again();
        loadLight(renderLights[0],0);
        for(unsigned i=1;i<7;++i)loadLight(renderLights[i],i+1);
        dKy_GxFog_tevstr_set(&lighting);
        GXSetNumTexGens(0);
        GXSetNumIndStages(0);
        GXSetNumTevStages(2);
        GXSetTevDirect(GX_TEVSTAGE0);
        GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP0,GX_TEV_SWAP0);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
        // Match this full-range PBR palette to the game's lighting: 2x keeps
        // steel and gold readable without the original texture shader's 4x clipping.
        GXSetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_2,GX_TRUE,GX_TEVPREV);
        GXSetTevDirect(GX_TEVSTAGE1);
        GXSetTevSwapMode(GX_TEVSTAGE1,GX_TEV_SWAP0,GX_TEV_SWAP0);
        GXSetTevOrder(GX_TEVSTAGE1,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR1A1);
        GXSetTevColorIn(GX_TEVSTAGE1,GX_CC_ZERO,GX_CC_RASC,GX_CC_ONE,GX_CC_CPREV);
        GXSetTevColorOp(GX_TEVSTAGE1,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE1,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_APREV);
        GXSetTevAlphaOp(GX_TEVSTAGE1,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
        GXSetZMode(GX_ENABLE,GX_LEQUAL,GX_ENABLE);
        GXSetZCompLoc(GX_TRUE);
        GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_COPY);
        GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
        GXSetColorUpdate(GX_TRUE);
        GXSetAlphaUpdate(GX_TRUE);
        GXSetCullMode(GX_CULL_NONE);
        GXSetCoPlanar(GX_FALSE);
        GXSetClipMode(GX_CLIP_ENABLE);
        GXSetCurrentMtx(GX_PNMTX0);
        for(size_t partIndex=0;partIndex<parts.size();++partIndex) {
            const auto& part=parts[partIndex];
            Mtx view;
            MTXConcat(drawView,world[partIndex],view);
            GXLoadPosMtxImm(view,GX_PNMTX0);
            GXLoadNrmMtxImm(view,GX_PNMTX0);
            // Aurora has a 5 MiB per-frame vertex stream. Indexed submission
            // preserves every original triangle while sharing exact vertices.
            // GXBeginIndexed copies indices immediately; direct attributes are
            // then copied into the FIFO, so no temporary buffer outlives a draw.
            GXBeginIndexed(GX_VTXFMT0,static_cast<u16>(part.vertices.size()),part.indices.data(),static_cast<u32>(part.indices.size()));
                for(const Vertex& x:part.vertices) {
                    GXPosition3f32(x.position.x,x.position.y,x.position.z);
                    GXNormal3f32(x.normal.x,x.normal.y,x.normal.z);
                    GXColor4u8(byte(x.color[0]*255),byte(x.color[1]*255),byte(x.color[2]*255),255);
                    // The supplied solid PBR materials retain their metalness and
                    // roughness. Add a camera-dependent highlight to native diffuse.
                    V3 n=unit(rotate(view,x.normal));
                    V3 p=transform(view,x.position);
                    V3 eye=unit(p*-1.0f);
                    float rough=std::clamp(x.roughness,0.12f,1.0f);
                    float exponent=std::clamp(2.0f/(rough*rough)-2.0f,2.0f,128.0f);
                    float rgb[3]={};
                    for(const auto& light:renderLights) {
                        if(!(light.mColor.r||light.mColor.g||light.mColor.b))continue;
                        V3 delta=V3{light.mLightPosition.x,light.mLightPosition.y,light.mLightPosition.z}-p;
                        float distance=length(delta);
                        V3 l=unit(delta);
                        float diffuse=std::max(0.0f,dot(n,l));
                        if(diffuse==0)continue;
                        V3 half=unit(l+eye);
                        float cosine=std::max(0.0f,dot(l,{light.mLightDirection.x,light.mLightDirection.y,light.mLightDirection.z}));
                        float cosAttenuation=light.mCosAtten.x+light.mCosAtten.y*cosine+light.mCosAtten.z*cosine*cosine;
                        float distAttenuation=light.mDistAtten.x+light.mDistAtten.y*distance+light.mDistAtten.z*distance*distance;
                        float attenuation=std::max(0.0f,cosAttenuation/std::max(0.000001f,distAttenuation));
                        float shine=std::pow(std::max(0.0f,dot(n,half)),exponent)*diffuse*attenuation;
                        float fresnel=std::pow(1.0f-std::max(0.0f,dot(eye,half)),5.0f);
                        const u8 lc[3]={light.mColor.r,light.mColor.g,light.mColor.b};
                        for(int c=0;c<3;++c) {
                            float f0=0.04f*(1-x.metallic)+x.color[c]*x.metallic;
                            rgb[c]+=shine*(f0+(1-f0)*fresnel)*lc[c]*0.8f;
                        }
                    }
                    // The original KHIII Key Fresnel particle supplies the
                    // transition glow; retain the weapon's actual materials.
                    GXColor4u8(byte(rgb[0]),byte(rgb[1]),byte(rgb[2]),255);
                }
            GXEnd();
        }
        J3DShape::resetVcdVatCache();
    }
    void drawSilhouette(const Mtx shadowView) {
        if(parts.empty()||!castsShadow)return;
        // Preserve the shadow pass's projection, channel mask, additive blend,
        // and TEVREG0 color. A full GX reset would erase other actors' shadows.
        Mtx world[32];presentationMatrices(world);
        GXCullMode oldCull;GXGetCullMode(&oldCull);
        J3DShape::resetVcdVatCache();
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetNumChans(0);GXSetNumTexGens(0);GXSetNumIndStages(0);GXSetNumTevStages(1);
        GXSetTevDirect(GX_TEVSTAGE0);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR_NULL);
        GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP0,GX_TEV_SWAP0);
        GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_C0);
        GXSetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_A0);
        GXSetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
        GXSetCurrentMtx(GX_PNMTX0);GXSetCullMode(GX_CULL_NONE);
        for(size_t i=0;i<parts.size();++i) {
            Mtx view;MTXConcat(shadowView,world[i],view);GXLoadPosMtxImm(view,GX_PNMTX0);
            const auto& part=parts[i];
            GXBeginIndexed(GX_VTXFMT0,static_cast<u16>(part.vertices.size()),part.indices.data(),static_cast<u32>(part.indices.size()));
            for(const auto& vertex:part.vertices) {
                const auto& p=vertex.position;GXPosition3f32(p.x,p.y,p.z);
            }
            GXEnd();
        }
        GXSetCullMode(oldCull);
        J3DShape::resetVcdVatCache();
    }
};
std::array<KeybladePacket,8> packets;
size_t packetIndex=0;
std::array<kh3fx::Packet,8> effectPackets;
size_t effectPacketIndex=0;
KeybladePacket* gameplayPacket=nullptr;
J3DModel* gameplaySword=nullptr;
J3DModel* gameplaySheath=nullptr;

HookAction queueModel(daAlink_c* link,J3DModel* model,bool hidden,bool preview) {
    if(!active(link)||parts.empty())return HOOK_CONTINUE;
    if(model==link->mpSwASheathModel)return HOOK_SKIP_ORIGINAL;
    if(model!=link->mpSwAModel)return HOOK_CONTINUE;
    if(link->checkWolf())return HOOK_SKIP_ORIGINAL;
    bool currentPresence=owner==link;
    if(!preview) {
        gameplaySword=model;gameplaySheath=link->mpSwASheathModel;
        bool visible=currentPresence?presence.visible():link->mEquipItem==0x103;
        if(!visible)return HOOK_SKIP_ORIGINAL;
    }
    auto& packet=packets[packetIndex++%packets.size()];
    packet.drawClear();
    packet.castsShadow=preview||link->mEquipItem==0x103;
    // Populate native light objects exactly as the original model draw would do.
    g_env_light.setLightTevColorType_MAJI(model,&link->tevStr);
    packet.lighting=link->tevStr;
    if(!MTXInverse(j3dSys.getViewMtx(),packet.lightViewInverse))MTXIdentity(packet.lightViewInverse);
    packet.interpolate=!preview&&owner==link&&ownerModel==model&&chain.ready&&!link->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW);
    if(packet.interpolate)std::memcpy(packet.matrices,simulatedParts,sizeof(simulatedParts));
    else if(!preview&&currentPresence&&presence.disappearing())std::memcpy(packet.matrices,dismissedParts,sizeof(dismissedParts));
    else fillMatrices(packet.matrices,model->getBaseTRMtx(),restPose(model->getBaseTRMtx()));
    if(!preview) {
        // Native camera hiding affects the main view only; a held weapon must
        // still participate in shadows/reflections. Vanished weapons returned above.
        gameplayPacket=&packet;
        daMirror_c::entry(model);
    }
    if(!hidden)j3dSys.getDrawBuffer(0)->entryImm(&packet,0);
    return HOOK_SKIP_ORIGINAL;
}
HookAction onModelDraw(ModContext*,void* args,void*,void*) {
    return queueModel(mods::arg<daAlink_c*>(args,0),mods::arg<J3DModel*>(args,1),mods::arg<int>(args,2)!=0,false);
}
HookAction onBasicModelDraw(ModContext*,void* args,void*,void*) {
    return queueModel(mods::arg<daAlink_c*>(args,0),mods::arg<J3DModel*>(args,1),false,true);
}
HookAction onLinkDraw(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    gameplayPacket=nullptr;
    gameplaySword=active(link)?link->mpSwAModel:nullptr;
    gameplaySheath=active(link)?link->mpSwASheathModel:nullptr;
    return HOOK_CONTINUE;
}
void onLinkDrawFinished(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(owner!=link||!active(link)||link->checkWolf()||!summonEffects.active()
        ||link->checkNoResetFlg2(daAlink_c::FLG2_STATUS_WINDOW_DRAW))return;
    auto& packet=effectPackets[effectPacketIndex++%effectPackets.size()];
    packet.drawClear();packet.set(summonEffects.snapshot());
    j3dSys.getDrawBuffer(1)->entryImm(&packet,0);
}
HookAction onMirrorModelDraw(ModContext*,void* args,void*,void*) {
    auto* mirror=mods::arg<dMirror_packet_c*>(args,0);
    auto* model=mods::arg<J3DModel*>(args,1);
    auto* view=mods::arg<MtxP>(args,2);
    if(model!=gameplaySword||parts.empty())return HOOK_CONTINUE;
    if(!gameplayPacket)return HOOK_SKIP_ORIGINAL;
    const auto& base=model->getBaseTRMtx();
    V3 origin{base[0][3],base[1][3],base[2][3]};
    if(mirror->mViewScale.y<=0||transform(view,origin).z<=transform(j3dSys.getViewMtx(),origin).z)
        gameplayPacket->drawInView(view);
    return HOOK_SKIP_ORIGINAL;
}
void onShadowImageDraw(ModContext*,void* args,void*,void*) {
    auto* shadow=mods::arg<dDlst_shadowReal_c*>(args,0);
    auto* view=mods::arg<MtxP>(args,1);
    if(!gameplaySword||parts.empty()) {ShadowImageDraw::g_orig(shadow,view);return;}
    J3DModel* saved[38];std::memcpy(saved,shadow->mpModels,sizeof(saved));
    u8 savedCount=shadow->mModelNum;
    bool swordPresent=false;u8 retained=0;
    for(u8 i=0;i<savedCount;++i) {
        if(saved[i]==gameplaySword)swordPresent=true;
        else if(saved[i]!=gameplaySheath)shadow->mpModels[retained++]=saved[i];
    }
    shadow->mModelNum=retained;
    if(retained)ShadowImageDraw::g_orig(shadow,view);
    shadow->mModelNum=savedCount;std::memcpy(shadow->mpModels,saved,sizeof(saved));
    if(!swordPresent||!gameplayPacket||!gameplayPacket->castsShadow)return;
    // These keys are the game's documented-in-source shadow matrix keys. The
    // public interpolation service explicitly permits looking up game keys.
    auto key=[&](uintptr_t n){return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(saved[0])^n);};
    Mtx shadowView,projection;
    if(!svc_interp->lookup_replacement_mtx(key(1),shadowView))std::memcpy(shadowView,shadow->mViewMtx,sizeof(Mtx));
    if(svc_interp->lookup_replacement_mtx(key(2),projection))GXSetProjection(projection,GX_ORTHOGRAPHIC);
    else GXSetProjection(shadow->mRenderProjMtx,GX_ORTHOGRAPHIC);
    gameplayPacket->drawSilhouette(shadowView);
}

class Reader {
public:
    const unsigned char* data;size_t remaining;
    bool read(void* target,size_t n) {if(n>remaining)return false;std::memcpy(target,data,n);data+=n;remaining-=n;return true;}
    template<class T>bool read(T& value) {return read(&value,sizeof(value));}
};
bool loadMesh() {
    ResourceBuffer buffer=RESOURCE_BUFFER_INIT;
    if(svc_resource->load(mod_ctx,"kingdom_key.mesh",&buffer)!=MOD_OK)return false;
    struct ReleaseResource {
        ResourceBuffer& buffer;
        ~ReleaseResource() {svc_resource->free(mod_ctx,&buffer);}
    } release{buffer};
    Reader r{static_cast<const unsigned char*>(buffer.data),buffer.size};
    char magic[8];uint32_t count=0;
    bool okay=r.read(magic,8)&&std::memcmp(magic,"KKMESH02",8)==0&&r.read(count)&&count>=3&&count<=32&&r.read(localAnchor);
    std::vector<Part> loaded;
    for(uint32_t i=0;okay&&i<count;++i) {
        uint32_t length=0,n=0;Part part;
        okay=r.read(length)&&length>0&&length<=64;
        if(!okay)break;
        part.name.resize(length);
        okay=r.read(part.name.data(),length)&&r.read(part.pivot)&&r.read(part.tangent)&&r.read(n)&&n>0&&n<=1000000&&n%3==0;
        if(!okay)break;
        if(static_cast<size_t>(n)>r.remaining/sizeof(Vertex)) {okay=false;break;}
        part.vertices.resize(n);
        okay=r.read(part.vertices.data(),sizeof(Vertex)*n);
        for(const Vertex& v:part.vertices) {
            const float* f=reinterpret_cast<const float*>(&v);
            for(int j=0;j<12;++j)if(!std::isfinite(f[j]))okay=false;
        }
        if(!okay)break;
        // Deduplicate bit-identical complete vertices, retaining material and
        // normal seams. No positions, triangles or surface detail are removed.
        std::unordered_map<std::string,u16> indexByVertex;
        std::vector<Vertex> unique;
        part.indices.reserve(part.vertices.size());
        for(const auto& vertex:part.vertices) {
            std::string key(reinterpret_cast<const char*>(&vertex),sizeof(Vertex));
            auto found=indexByVertex.find(key);
            if(found!=indexByVertex.end())part.indices.push_back(found->second);
            else {
                if(unique.size()>=65535) {okay=false;break;}
                u16 index=static_cast<u16>(unique.size());
                indexByVertex.emplace(std::move(key),index);
                unique.push_back(vertex);part.indices.push_back(index);
            }
        }
        part.vertices=std::move(unique);
        loaded.push_back(std::move(part));
    }
    okay=okay&&r.remaining==0&&loaded.size()==count&&loaded.front().name=="rigid"&&loaded.back().name=="charm";
    if(okay)parts=std::move(loaded);
    return okay;
}

// Assets remain resident across choices. In particular audio resource additions
// are made once at mod initialization, preserving the baseline host lifecycle.
// Call between frames, never from a configuration callback or a draw hook.
void deactivate() {
    enabled=false;
    keybladeUI.shutdown();
    keybladeText.shutdown();
    if(keybladeAudio.ready(audio::Cue::Summon))keybladeAudio.stopAll();
    resetPresence();
    gameplayPacket=nullptr;gameplaySword=nullptr;gameplaySheath=nullptr;
    hitStamps={};hitStampIndex=0;
    refreshCollectionLabel=true;
}
bool activate(std::string& error) {
    deactivate();
    if(parts.empty()) {error="Kingdom Key model is unavailable; using Ordon Sword.";return false;}
    if(!keybladeUI.initialize(mod_ctx,svc_resource,svc_texture,error))return false;
    if(keybladeText.initialize(mod_ctx,svc_message)!=MOD_OK) {
        keybladeUI.shutdown();
        error="Kingdom Key text could not be registered; using Ordon Sword.";
        return false;
    }
    enabled=true;
    refreshCollectionLabel=true;
    return true;
}

} // namespace kingdom::keyblade_runtime
