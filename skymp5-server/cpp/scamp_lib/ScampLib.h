#include <stdint.h>

#if defined(SCAMPLIB_IMPLEMENTATION)
#define SLExport __declspec(dllexport)
#else
#define SLExport __declspec(dllimport)
#endif

#ifdef __cplusplus
class ScampServer;
#else
typedef void* ScampServer;
#endif

struct Position {
    float x;
    float y;
    float z;
};

typedef void* SLString;

typedef struct {
    SLString(*CreateString)(char* cstr);
} UtilsApi;

typedef void onConnectFn(unsigned short userId);
typedef void onDisconnectFn(unsigned short userId);
typedef void OnCustomPacketFn(unsigned short userId, char* content);
typedef void OnMpApiEventFn(char* eventName, char* args, uint32_t formId);

#ifdef __cplusplus
extern "C" {
#endif

SLExport void Init(UtilsApi* utilsApi);
SLExport ScampServer* CreateServer(uint32_t port, uint32_t maxConnections);
SLExport void SetConnectHandler(ScampServer* ss, onConnectFn* handler);
SLExport void SetDisconnectHandler(ScampServer* ss, onDisconnectFn* handler);
SLExport void SetCustomPacketHandler(ScampServer* ss, OnCustomPacketFn* handler);
SLExport void SetMpApiEventHandler(ScampServer* ss, OnMpApiEventFn* handler);
SLExport void Tick(ScampServer* sl);
SLExport uint32_t CreateActor(ScampServer* sl, uint32_t formId, struct Position pos, float angleZ, uint32_t cellOrWorld, int32_t profileId);
SLExport void SetUserActor(ScampServer* sl, unsigned short userId, uint32_t actorFormId);
SLExport uint32_t GetUserActor(ScampServer* sl, unsigned short userId);
SLExport char* GetActorName(ScampServer* sl, uint32_t actorFormId);
SLExport struct Position GetActorPos(ScampServer* sl, uint32_t actorFormId);
SLExport uint32_t GetActorCellOrWorld(ScampServer* sl, uint32_t actorFormId);
SLExport void DestroyActor(ScampServer* sl, uint32_t actorFormId);
SLExport void SetRaceMenuOpen(ScampServer* sl, uint32_t actorFormId, bool open);
SLExport uint32_t* GetActorsByProfileId(ScampServer* sl, int32_t profileId, size_t* result_len);
SLExport void SetEnabled(ScampServer* sl, uint32_t actorFormId, bool enabled);
SLExport unsigned short GetUserByActor(ScampServer* sl, uint32_t formId);
SLExport void AttachSaveStorage(ScampServer* sl);
SLExport void SendCustomPacket(ScampServer* sl, unsigned short userId, char* json_data);
SLExport void Clear(ScampServer* ss);
SLExport void MakeEventSource(ScampServer* ss, char* eventName, char* functionBody);

#ifdef __cplusplus
}
#endif