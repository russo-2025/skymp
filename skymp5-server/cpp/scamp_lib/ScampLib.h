#pragma once
#include <stdint.h>
#if defined(SCAMPLIB_IMPLEMENTATION)
#include "v.h"
#include "PartOne.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SCAMPLIB_IMPLEMENTATION)
#define SLExport __declspec(dllexport)

class ScampServer;
class MpForm;
class MpObjectReference;
class MpActor;
class FormDesc;

#define GenWrapper(returnType, prefix, name, thisType) \
    SLExport returnType prefix##name##(##thisType thisArg) { \
        return thisArg->##name##(); \
    };

#define GenWrapper1(returnType, prefix, name, thisType, arg1Type) \
    SLExport returnType prefix##name##(##thisType thisArg, arg1Type arg1) { \
        return thisArg->##name##(arg1); \
    };

#define GenWrapperVoid(prefix, name, thisType) \
    SLExport void prefix##name##(##thisType thisArg) { \
        thisArg->##name##(); \
    };

#define GenWrapperVoid1(prefix, name, thisType, arg1Type) \
    SLExport void prefix##name##(##thisType thisArg, arg1Type arg1) { \
        thisArg->##name##(arg1); \
    };

#define GenCastObj(fromType, ToType) \
    SLExport ToType* Cast##fromType##To##ToType(fromType* obj) { \
        return dynamic_cast<ToType*>(obj); \
    }
#else
#define SLExport __declspec(dllimport)

typedef void* ScampServer;
typedef void* MpForm;
typedef void* MpObjectReference;
typedef void* MpActor;
typedef void* FormDesc;

#define GenWrapper(returnType, prefix, name, thisType) \
    SLExport returnType prefix##name##(##thisType thisArg);

#define GenWrapper1(returnType, prefix, name, thisType, arg1Type) \
    SLExport returnType prefix##name##(##thisType thisArg, arg1Type arg1);

#define GenWrapperVoid(prefix, name, thisType) \
    SLExport void prefix##name##(##thisType thisArg);

#define GenWrapperVoid1(prefix, name, thisType, arg1Type) \
    SLExport void prefix##name##(##thisType thisArg, arg1Type arg1);

#define GenCastObj(fromType, ToType) \
    SLExport ToType* Cast##fromType##To##ToType(fromType* obj);

#endif

#define GenFormWrapper(returnType, name) GenWrapper(returnType, MpForm_, name, MpForm*)
#define GenFormWrapper1(returnType, name, arg1Type) GenWrapper1(returnType, MpForm_, name, MpForm*, arg1Type)
#define GenFormWrapperVoid(name) GenWrapperVoid(MpForm_, name, MpForm*)
#define GenFormWrapperVoid1(name, arg1Type) GenWrapperVoid1(MpForm_, name, MpForm*, arg1Type)

#define GenObjectReferenceWrapper(returnType, name) GenWrapper(returnType, MpObjectReference_, name, MpObjectReference*)
#define GenObjectReferenceWrapper1(returnType, name, arg1Type) GenWrapper1(returnType, MpObjectReference_, name, MpObjectReference*, arg1Type)
#define GenObjectReferenceWrapperVoid(name) GenWrapperVoid(MpObjectReference_, name, MpObjectReference*)
#define GenObjectReferenceWrapperVoid1(name, arg1Type) GenWrapperVoid1(MpObjectReference_, name, MpObjectReference*, arg1Type)

#define GenActorWrapper(returnType, name) GenWrapper(returnType, MpActor_, name, MpActor*)
#define GenActorWrapper1(returnType, name, arg1Type) GenWrapper1(returnType, MpActor_, name, MpActor*, arg1Type)
#define GenActorWrapperVoid(name) GenWrapperVoid(MpActor_, name, MpActor*)
#define GenActorWrapperVoid1(name, arg1Type) GenWrapperVoid1(MpActor_, name, MpActor*, arg1Type)

struct Position {
    float x;
    float y;
    float z;
};

typedef void onConnectFn(unsigned short userId);
typedef void onDisconnectFn(unsigned short userId);
typedef void OnCustomPacketFn(unsigned short userId, char* content);
typedef void PacketHandlerFn(unsigned short userId, const unsigned char* data, size_t length);

SLExport ScampServer* CreateServer(uint32_t port, uint32_t maxConnections);

//================================================================
//ScampServer
//================================================================
SLExport void SetConnectHandler(ScampServer* ss, onConnectFn* handler);
SLExport void SetDisconnectHandler(ScampServer* ss, onDisconnectFn* handler);
SLExport void SetCustomPacketHandler(ScampServer* ss, OnCustomPacketFn* handler);
SLExport void SetPacketHandler(ScampServer* ss, PacketHandlerFn* handler);
SLExport void Tick(ScampServer* sl);
SLExport uint32_t CreateActor(ScampServer* sl, uint32_t formId, struct Position pos, float angleZ, uint32_t cellOrWorld, int32_t profileId);
SLExport void SetUserActor(ScampServer* sl, unsigned short userId, uint32_t actorFormId);
SLExport uint32_t GetUserActor(ScampServer* sl, unsigned short userId);
SLExport MpActor* ActorByUser(ScampServer* ss, unsigned short userId); // Returns nullptr if no actor found
SLExport char* GetActorName(ScampServer* sl, uint32_t actorFormId);
SLExport struct Position GetActorPos(ScampServer* sl, uint32_t actorFormId);
SLExport uint32_t GetActorWorldOrCell(ScampServer* sl, uint32_t actorFormId);
SLExport void DestroyActor(ScampServer* sl, uint32_t actorFormId);
SLExport void SetRaceMenuOpen(ScampServer* sl, uint32_t actorFormId, bool open);
SLExport uint32_t* GetActorsByProfileId(ScampServer* sl, int32_t profileId, size_t* result_len);
SLExport void SetEnabled(ScampServer* sl, uint32_t actorFormId, bool enabled);
SLExport unsigned short GetUserByActor(ScampServer* sl, uint32_t formId);
SLExport void AttachSaveStorage(ScampServer* sl);
SLExport void SendCustomPacket(ScampServer* sl, unsigned short userId, char* json_data);
SLExport uint32_t FindHoster(ScampServer* ss, uint32_t formId); //Returns 0 if no found // че за host? не понимаю что это
SLExport MpForm* GetMpForm(ScampServer* ss, uint32_t formId); //Returns 0 if no found
SLExport MpObjectReference* GetMpObjectReference(ScampServer* ss, uint32_t formId); //Returns 0 if no found
SLExport MpActor* GetMpActor(ScampServer* ss, uint32_t formId); //Returns 0 if no found

//================================================================
//Cast Object
//================================================================
GenCastObj(MpForm, MpObjectReference)
GenCastObj(MpForm, MpActor)
GenCastObj(MpObjectReference, MpForm)
GenCastObj(MpObjectReference, MpActor)
GenCastObj(MpActor, MpForm)
GenCastObj(MpActor, MpObjectReference)

//================================================================
//MpForm
//================================================================

GenFormWrapper(uint32_t, GetFormId)

//================================================================
//MpObjectReference
//================================================================
GenObjectReferenceWrapper(uint32_t, GetIdx)
GenObjectReferenceWrapper(uint32_t, GetFormId)
GenObjectReferenceWrapper(uint32_t, GetBaseId)
GenObjectReferenceWrapperVoid1(RemoveAllItems, MpObjectReference*)
GenObjectReferenceWrapper(bool, IsOpen)
GenObjectReferenceWrapperVoid1(SetOpen, bool)
GenObjectReferenceWrapper(bool, IsHarvested)
GenObjectReferenceWrapperVoid1(SetHarvested, bool)
GenObjectReferenceWrapper(bool, IsDisabled)
GenObjectReferenceWrapperVoid(Disable)
GenObjectReferenceWrapperVoid(Enable)
GenObjectReferenceWrapper(bool, IsActivationBlocked)
GenObjectReferenceWrapperVoid1(SetActivationBlocked, bool)

SLExport FormDesc* MpObjectReference_GetCellOrWorld(MpObjectReference* ref);
SLExport void MpObjectReference_SetCellOrWorld(MpObjectReference* ref, FormDesc* desc);
SLExport struct Position MpObjectReference_GetAngle(MpObjectReference* ref);
SLExport void MpObjectReference_SetAngle(MpObjectReference* ref, struct Position angle);
SLExport struct Position MpObjectReference_GetPosition(MpObjectReference* ref);
SLExport void MpObjectReference_SetPosition(MpObjectReference* ref, struct Position pos);

//================================================================
//MpActor
//================================================================
GenActorWrapper(uint32_t, GetIdx)
GenActorWrapper(uint32_t, GetFormId)

SLExport void MpActor_SendToUser(MpActor* ac, string msg);
SLExport void MpActor_SendToNeighbors(MpActor* ac, string msg);

#ifdef __cplusplus
}
#endif