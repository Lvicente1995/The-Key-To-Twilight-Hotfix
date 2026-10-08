#pragma once

// Shared render hooks coordinate the independently selected character and
// weapon. Each renderer owns only its replacement, leaving native equipment
// and all player animation/controller work in the game.
namespace kingdom::customization {

HookAction onModelDraw(ModContext* context,void* args,void* result,void* userdata) {
    const auto action=character.queueModel(mods::arg<daAlink_c*>(args,0),
        mods::arg<J3DModel*>(args,1),mods::arg<int>(args,2)!=0,false);
    if(action!=HOOK_CONTINUE)return action;
    return keyblade_runtime::onModelDraw(context,args,result,userdata);
}

HookAction onBasicModelDraw(ModContext* context,void* args,void* result,void* userdata) {
    const auto action=character.queueModel(mods::arg<daAlink_c*>(args,0),
        mods::arg<J3DModel*>(args,1),false,true);
    if(action!=HOOK_CONTINUE)return action;
    return keyblade_runtime::onBasicModelDraw(context,args,result,userdata);
}

HookAction onLinkDraw(ModContext* context,void* args,void* result,void* userdata) {
    character.beginFrame(mods::arg<daAlink_c*>(args,0));
    return keyblade_runtime::onLinkDraw(context,args,result,userdata);
}

HookAction onMirrorModelDraw(ModContext* context,void* args,void* result,void* userdata) {
    if(character.drawMirror(mods::arg<J3DModel*>(args,1),
        mods::arg<dMirror_packet_c*>(args,0),mods::arg<MtxP>(args,2)))
        return HOOK_SKIP_ORIGINAL;
    return keyblade_runtime::onMirrorModelDraw(context,args,result,userdata);
}

void onShadowImageDraw(ModContext* context,void* args,void* result,void* userdata) {
    namespace kk=keyblade_runtime;
    // Preserve the known-good weapon-only route when Link is displayed.
    if(!character.hasGameplayPacket()) {
        kk::onShadowImageDraw(context,args,result,userdata);
        return;
    }
    auto* shadow=mods::arg<dDlst_shadowReal_c*>(args,0);
    auto* view=mods::arg<MtxP>(args,1);
    if(!shadow->mModelNum||shadow->mModelNum>38) {
        ShadowImageDraw::g_orig(shadow,view);
        return;
    }
    J3DModel* saved[38];
    std::memcpy(saved,shadow->mpModels,sizeof(saved));
    const u8 savedCount=shadow->mModelNum;
    const bool weaponActive=kk::gameplaySword&&!kk::parts.empty();
    auto replaced=[&](J3DModel* model) {
        return character.ownsShadowModel(model)||
            (weaponActive&&(model==kk::gameplaySword||model==kk::gameplaySheath));
    };
    bool characterPresent=false,swordPresent=false;
    for(u8 i=0;i<savedCount;++i) {
        characterPresent|=character.ownsShadowModel(saved[i]);
        swordPresent|=weaponActive&&saved[i]==kk::gameplaySword;
    }
    if(!characterPresent) {
        kk::onShadowImageDraw(context,args,result,userdata);
        return;
    }

    // The host records shadow interpolation against the ORIGINAL slot zero.
    // Keep that model as a zero-geometry sentinel when it is replaced, so
    // native shield/weapon shadows still use the exact recorded matrix keys.
    std::vector<J3DShape*> visibleShapes;
    if(replaced(saved[0])) {
        auto* data=saved[0]->getModelData();
        try {
            visibleShapes.reserve(data->getShapeNum());
            for(u16 i=0;i<data->getShapeNum();++i) {
                auto* shape=data->getShapeNodePointer(i);
                if(!shape->checkFlag(1))visibleShapes.push_back(shape);
            }
        } catch(...) {
            // No state has changed yet. A native shadow is a safe fallback.
            ShadowImageDraw::g_orig(shadow,view);
            return;
        }
    }
    {
        struct RestoreNativeShadow {
            dDlst_shadowReal_c* shadow;
            J3DModel** models;
            u8 count;
            const std::vector<J3DShape*>& shapes;
            ~RestoreNativeShadow() {
                for(auto* shape:shapes)shape->show();
                std::memcpy(shadow->mpModels,models,sizeof(shadow->mpModels));
                shadow->mModelNum=count;
            }
        } restore{shadow,saved,savedCount,visibleShapes};
        for(auto* shape:visibleShapes)shape->hide();
        u8 retained=1;
        shadow->mpModels[0]=saved[0];
        for(u8 i=1;i<savedCount;++i)
            if(!replaced(saved[i]))shadow->mpModels[retained++]=saved[i];
        shadow->mModelNum=retained;
        ShadowImageDraw::g_orig(shadow,view);
    }
    auto key=[&](uintptr_t n) {
        return reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(saved[0])^n);
    };
    Mtx shadowView,projection;
    if(!svc_interp->lookup_replacement_mtx(key(1),shadowView))
        std::memcpy(shadowView,shadow->mViewMtx,sizeof(Mtx));
    if(svc_interp->lookup_replacement_mtx(key(2),projection))
        GXSetProjection(projection,GX_ORTHOGRAPHIC);
    else GXSetProjection(shadow->mRenderProjMtx,GX_ORTHOGRAPHIC);
    character.drawSilhouette(shadowView);
    if(swordPresent&&kk::gameplayPacket&&kk::gameplayPacket->castsShadow)
        kk::gameplayPacket->drawSilhouette(shadowView);
}

} // namespace kingdom::customization
