#pragma once
#include <stdint.h>

#if defined(SCAMPLIB_IMPLEMENTATION)
#include "v.h"

#define SLExport __declspec(dllexport)

class ScampServer;
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

#else
#define SLExport __declspec(dllimport)

typedef void* ScampServer;
typedef void* MpObjectReference;
typedef void* MpActor;
typedef void* FormDesc;
#endif

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

//ScampServer
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

SLExport Option_server__MpObjectReference GetMpObjectReference(ScampServer* ss, uint32_t formId);
SLExport Option_server__MpActor GetMpActor(ScampServer* ss, uint32_t formId);
SLExport struct Option_server__MpActor CastMpObjectReferenceToMpActor(MpObjectReference* refr);

//MpObjectReference
SLExport uint32_t GetBaseId(MpObjectReference* ref);
SLExport void RemoveAllItems(MpObjectReference* ref, MpObjectReference* target);
SLExport FormDesc* GetCellOrWorld(MpObjectReference* ref);
SLExport void SetCellOrWorld(MpObjectReference* ref, FormDesc* desc);
SLExport struct Position GetAngle(MpObjectReference* ref);
SLExport void SetAngle(MpObjectReference* ref, struct Position angle);
SLExport struct Position GetPosition(MpObjectReference* ref);
SLExport void SetPosition(MpObjectReference* ref, struct Position pos);
SLExport bool IsOpen(MpObjectReference* ref);
SLExport void SetOpen(MpObjectReference* ref, bool state);
SLExport bool IsHarvested(MpObjectReference* ref);
SLExport void SetHarvested(MpObjectReference* ref, bool state);
SLExport bool IsDisabled(MpObjectReference* ref);
SLExport void Disable(MpObjectReference* ref);
SLExport void Enable(MpObjectReference* ref);
SLExport bool IsActivationBlocked(MpObjectReference* ref);
SLExport void SetActivationBlocked(MpObjectReference* ref, bool state);

//TODO
SLExport void Clear(ScampServer* ss);
SLExport void MakeEventSource(ScampServer* ss, char* eventName, char* functionBody);

#ifdef __cplusplus
}
#endif