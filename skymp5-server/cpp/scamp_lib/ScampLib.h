#pragma once
#include <stdint.h>

#if defined(SCAMPLIB_IMPLEMENTATION)
#include "v.h"
#include "PartOne.h"

#define SLExport __declspec(dllexport)

class ScampServer;
class MpForm;
class MpObjectReference;
class MpActor;
class FormDesc;

struct Option_server__MpActor {
    byte state;
    IError err;
    byte data[sizeof(MpActor*)];
};

struct Option_server__MpObjectReference {
    byte state;
    IError err;
    byte data[sizeof(MpObjectReference*)];
};

#define GenWrapper(returnType, name, thisType) \
    SLExport returnType name##(##thisType thisArg) { \
        return thisArg->##name##(); \
    };

#define GenWrapper1(returnType, name, thisType, arg1Type) \
    SLExport returnType name##(##thisType thisArg, arg1Type arg1) { \
        return thisArg->##name##(arg1); \
    };

#define GenWrapperVoid(name, thisType) \
    SLExport void name##(##thisType thisArg) { \
        thisArg->##name##(); \
    };

#define GenWrapperVoid1(name, thisType, arg1Type) \
    SLExport void name##(##thisType thisArg, arg1Type arg1) { \
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

#define GenWrapper(returnType, name, thisType) \
    SLExport returnType name##(##thisType thisArg);

#define GenWrapper1(returnType, name, thisType, arg1Type) \
    SLExport returnType name##(##thisType thisArg, arg1Type arg1);

#define GenWrapperVoid(name, thisType) \
    SLExport void name##(##thisType thisArg);

#define GenWrapperVoid1(name, thisType, arg1Type) \
    SLExport void name##(##thisType thisArg, arg1Type arg1);

#define GenCastObj(fromType, ToType) \
    SLExport ToType* Cast##fromType##To##ToType(fromType* obj);

#endif

#define GenObjectReferenceWrapper(returnType, name) GenWrapper(returnType, name, MpObjectReference*)
#define GenObjectReferenceWrapper1(returnType, name, arg1Type) GenWrapper1(returnType, name, MpObjectReference*, arg1Type)
#define GenObjectReferenceWrapperVoid(name) GenWrapperVoid(name, MpObjectReference*)
#define GenObjectReferenceWrapperVoid1(name, arg1Type) GenWrapperVoid1(name, MpObjectReference*, arg1Type)

#define GenActorWrapper(returnType, name) GenWrapper(returnType, name, MpActor*)
#define GenActorWrapper1(returnType, name, arg1Type) GenWrapper1(returnType, name, MpActor*, arg1Type)
#define GenActorWrapperVoid(name) GenWrapperVoid(name, MpActor*)
#define GenActorWrapperVoid1(name, arg1Type) GenWrapperVoid1(name, MpActor*, arg1Type)

struct Position {
    float x;
    float y;
    float z;
};

typedef void onConnectFn(unsigned short userId);
typedef void onDisconnectFn(unsigned short userId);
typedef void OnCustomPacketFn(unsigned short userId, char* content);
typedef void PacketHandlerFn(unsigned short userId, const unsigned char* data, size_t length);

#ifdef __cplusplus
extern "C" {
#endif

SLExport ScampServer* CreateServer(uint32_t port, uint32_t maxConnections);

//================================================================
//ScampServer
//================================================================
SLExport void SetConnectHandler(ScampServer* ss, onConnectFn* handler);
SLExport void SetDisconnectHandler(ScampServer* ss, onDisconnectFn* handler);
SLExport void SetCustomPacketHandler(ScampServer* ss, OnCustomPacketFn* handler);
SLExport void SetPacketHandler(ScampServer* ss, PacketHandlerFn* handler);
SLExport Option_void Tick(ScampServer* sl);
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
SLExport Option_server__MpObjectReference GetMpObjectReference(ScampServer* ss, uint32_t formId);
SLExport Option_server__MpActor GetMpActor(ScampServer* ss, uint32_t formId);

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
//MpObjectReference
//================================================================
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

SLExport FormDesc* GetCellOrWorld(MpObjectReference* ref);
SLExport void SetCellOrWorld(MpObjectReference* ref, FormDesc* desc);
SLExport struct Position GetAngle(MpObjectReference* ref);
SLExport void SetAngle(MpObjectReference* ref, struct Position angle);
SLExport struct Position GetPosition(MpObjectReference* ref);
SLExport void SetPosition(MpObjectReference* ref, struct Position pos);

//================================================================
//MpActor
//================================================================
GenActorWrapper(uint32_t, GetFormId)

#ifdef __cplusplus
}
#endif