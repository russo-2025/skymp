
/*
  std::shared_ptr<PartOne> partOne;
  std::shared_ptr<Networking::IServer> server;
  std::shared_ptr<Networking::MockServer> serverMock;
  std::shared_ptr<ScampServerListener> listener;
  Napi::Env tickEnv;
  Napi::ObjectReference emitter;
  Napi::FunctionReference emit;
  std::shared_ptr<spdlog::logger> logger;
  nlohmann::json serverSettings;
  std::shared_ptr<JsEngine> chakraEngine;
  TaskQueue chakraTaskQueue;
  std::optional<Napi::FunctionReference> sendUiMessageImplementation;
  GamemodeApi::State gamemodeApiState;
*/


#include "AsyncSaveStorage.h"
//#include "EspmGameObject.h"
#include "FileDatabase.h"
//#include "FormCallbacks.h"
//#include "GamemodeApi.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
//#include "MpFormGameObject.h"
#include "Networking.h"
#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "ScriptStorage.h"
#include "SqliteDatabase.h"
//#include <JsEngine.h>
//#include <cassert>
//#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <string>
#include <iostream>

namespace ScampLib {
    typedef void onConnectFn(Networking::UserId userId);
    typedef void onDisconnectFn(Networking::UserId userId);
    typedef void OnCustomPacketFn(Networking::UserId userId, char* content);
}

class ScampServerListener : public PartOne::Listener
{
public:
    ScampServerListener() {
        connect = [](Networking::UserId userId) {};
        disconnect = [](Networking::UserId userId) {};
        customPacket = [](Networking::UserId userId, char* content) {};
    }

    void setConnectHandler(ScampLib::onConnectFn* a_connect) {
        connect = a_connect;
    }

    void setDisconnectHandler(ScampLib::onDisconnectFn* a_disconnect) {
        disconnect = a_disconnect;
    }

    void setCustomPacketHandler(ScampLib::OnCustomPacketFn* a_customPacket) {
        customPacket = a_customPacket;
    }

    ScampLib::onConnectFn* connect;
    ScampLib::onDisconnectFn* disconnect;
    ScampLib::OnCustomPacketFn* customPacket;

    void OnConnect(Networking::UserId userId) override
    {
        connect(userId);
    }

    void OnDisconnect(Networking::UserId userId) override
    {
        disconnect(userId);
    }

    void OnCustomPacket(Networking::UserId userId, const simdjson::dom::element& content) override
    {
        using std::string;
        std::string str = simdjson::minify(content);
        customPacket(userId, (char*)str.c_str());
    }

    bool OnMpApiEvent(const char* eventName, std::optional<simdjson::dom::element> args, std::optional<uint32_t> formId)
    {
        return true;
    }
};

class ScampServer {
public:
    ScampServer() {}

    std::shared_ptr<PartOne> partOne;
    std::shared_ptr<ScampServerListener> listener;
    std::shared_ptr<Networking::MockServer> serverMock;
    std::shared_ptr<Networking::IServer> server;
    std::shared_ptr<spdlog::logger> logger;
    nlohmann::json serverSettings;
};

/*
namespace Scamp {
    typedef void onConnectFn(Networking::UserId userId);
    typedef void onDisconnectFn(Networking::UserId userId);
    typedef void OnCustomPacketFn(Networking::UserId userId, char* content);

    onConnectFn* connect = [](Networking::UserId userId) {};
    onDisconnectFn* disconnect = [](Networking::UserId userId) {};
    OnCustomPacketFn* customPacket = [](Networking::UserId userId, char* content) {};
}

std::shared_ptr<PartOne> partOne;
std::shared_ptr<ScampServerListener> listener;
std::shared_ptr<Networking::MockServer> serverMock;
std::shared_ptr<Networking::IServer> server;
std::shared_ptr<spdlog::logger> logger;
nlohmann::json serverSettings;
*/

std::shared_ptr<IDatabase> CreateDatabase(nlohmann::json settings, std::shared_ptr<spdlog::logger> logger)
{
    auto databaseDriver = settings.count("databaseDriver")
        ? settings["databaseDriver"].get<std::string>()
        : std::string("file");

    if (databaseDriver == "sqlite") {
        auto databaseName = settings.count("databaseName")
            ? settings["databaseName"].get<std::string>()
            : std::string("world.sqlite");

        logger->info("Using sqlite with name '" + databaseName + "'");
        return std::make_shared<SqliteDatabase>(databaseName);
    }

    if (databaseDriver == "file") {
        auto databaseName = settings.count("databaseName")
            ? settings["databaseName"].get<std::string>()
            : std::string("world");

        logger->info("Using file with name '" + databaseName + "'");
        return std::make_shared<FileDatabase>(databaseName, logger);
    }

    if (databaseDriver == "mongodb") {
        auto databaseName = settings.count("databaseName")
            ? settings["databaseName"].get<std::string>()
            : std::string("db");

        auto databaseUri = settings["databaseUri"].get<std::string>();
        logger->info("Using mongodb with name '" + databaseName + "'");
        return std::make_shared<MongoDatabase>(databaseUri, databaseName);
    }

    if (databaseDriver == "migration") {
        auto from = settings.at("databaseOld");
        auto to = settings.at("databaseNew");
        auto oldDatabase = CreateDatabase(from, logger);
        auto newDatabase = CreateDatabase(to, logger);
        return std::make_shared<MigrationDatabase>(newDatabase, oldDatabase);
    }

    throw std::runtime_error("Unrecognized databaseDriver: " + databaseDriver);
}

std::shared_ptr<ISaveStorage> CreateSaveStorage(std::shared_ptr<IDatabase> db, std::shared_ptr<spdlog::logger> logger)
{
    return std::make_shared<AsyncSaveStorage>(db, logger);
}

extern "C" {
    __declspec(dllexport) ScampServer* CreateServer(uint32_t port, uint32_t maxConnections, ScampLib::onConnectFn* onConnect, ScampLib::onDisconnectFn* onDisconnect, ScampLib::OnCustomPacketFn* OnCustomPacket)
    {
        ScampServer* ss = new ScampServer();

        ss->partOne.reset(new PartOne);
        ss->partOne->EnableProductionHacks();
        ss->listener.reset(new ScampServerListener());
        ss->partOne->AddListener(ss->listener);

        ss->serverMock = std::make_shared<Networking::MockServer>();

        std::string dataDir = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition\\Data";
    #ifndef WIN32
        dataDir = "/skyrim_data_dir";
    #endif

        ss->logger = spdlog::stdout_color_mt("console");
        ss->partOne->AttachLogger(ss->logger);

        std::ifstream f("server-settings.json");
        if (!f.good()) {
            throw std::runtime_error("server-settings.json is missing");
        }

        std::stringstream buffer;
        buffer << f.rdbuf();

        ss->serverSettings = nlohmann::json::parse(buffer.str());

        ss->partOne->worldState.isPapyrusHotReloadEnabled =
            ss->serverSettings.count("isPapyrusHotReloadEnabled") != 0 &&
            ss->serverSettings.at("isPapyrusHotReloadEnabled").get<bool>();
        
        auto hotReloadStatus = ss->partOne->worldState.isPapyrusHotReloadEnabled ? "enabled" : "disabled";
        ss->logger->info("Hot reload is {} for Papyrus", hotReloadStatus);

        if (ss->serverSettings["dataDir"] != nullptr) {
            dataDir = ss->serverSettings["dataDir"];
        }
        ss->logger->info("Using data dir '{}'", dataDir);

        std::vector<espm::fs::path> plugins = { "Skyrim.esm", "Update.esm",
                                                "Dawnguard.esm", "HearthFires.esm",
                                                "Dragonborn.esm" };
        if (ss->serverSettings["loadOrder"].is_array()) {
            plugins.clear();
            for (size_t i = 0; i < ss->serverSettings["loadOrder"].size(); ++i) {
                auto s = static_cast<std::string>(ss->serverSettings["loadOrder"][i]);
                plugins.push_back(s);
            }
        }

        auto scriptStorage = std::make_shared<DirectoryScriptStorage>(
            (espm::fs::path(dataDir) / "scripts").string());

        auto espm = new espm::Loader(dataDir, plugins);
        auto realServer = Networking::CreateServer(port, maxConnections);
        ss->server = Networking::CreateCombinedServer({ realServer,  ss->serverMock });
        ss->partOne->SetSendTarget(ss->server.get());
        ss->partOne->worldState.AttachScriptStorage(scriptStorage);
        ss->partOne->AttachEspm(espm);

        auto reloot = ss->serverSettings["reloot"];
        for (auto it = reloot.begin(); it != reloot.end(); ++it) {
            std::string recordType = it.key();
            auto timeMs = static_cast<int>(it.value());
            auto time = std::chrono::milliseconds(1) * timeMs;
            ss->partOne->worldState.SetRelootTime(recordType, time);
            ss->logger->info("'{}' will be relooted every {} ms", recordType, timeMs);
        }
           
        ss->listener->setConnectHandler(onConnect);
        ss->listener->setDisconnectHandler(onDisconnect);
        ss->listener->setCustomPacketHandler(OnCustomPacket);

        return ss;
    }

    /*
    ScampServer::AttachSaveStorage
    ScampServer::Tick
    ScampServer::CreateActor
    ScampServer::SetUserActor
    ScampServer::GetUserActor
    ScampServer::GetActorPos
    ScampServer::GetActorCellOrWorld
    ScampServer::GetActorName
    ScampServer::DestroyActor
    ScampServer::SetRaceMenuOpen
    ScampServer::SendCustomPacket
    ScampServer::GetActorsByProfileId
    ScampServer::SetEnabled
    ScampServer::GetUserByActor

    ScampServer::On
    ScampServer::CreateBot
    ScampServer::ExecuteJavaScriptOnChakra
    ScampServer::SetSendUiMessageImplementation
    ScampServer::OnUiEvent
    ScampServer::Clear
    */

    struct Position {
        float x;
        float y;
        float z;
    };

    __declspec(dllexport) void Tick(ScampServer* ss)
    {
        ss->server->Tick(PartOne::HandlePacket, ss->partOne.get());
        ss->partOne->Tick();
    }

    __declspec(dllexport) uint32_t CreateActor(ScampServer* ss, uint32_t formId, Position vpos, float angleZ, uint32_t cellOrWorld, int32_t profileId)
    {
         NiPoint3 pos = NiPoint3(vpos.x, vpos.y, vpos.z);
        return ss->partOne->CreateActor(formId, pos, angleZ, cellOrWorld, profileId);
    }

    __declspec(dllexport) void SetUserActor(ScampServer* ss, Networking::UserId userId, uint32_t actorFormId)
    {
        ss->partOne->SetUserActor(userId, actorFormId);
    }

    __declspec(dllexport) uint32_t GetUserActor(ScampServer* ss, Networking::UserId userId)
    {
        return ss->partOne->GetUserActor(userId);
    }

    __declspec(dllexport) const char* GetActorName(ScampServer* ss, uint32_t actorFormId)
    {
        return ss->partOne->GetActorName(actorFormId).c_str();
    }

    __declspec(dllexport) Position GetActorPos(ScampServer* ss, uint32_t actorFormId)
    {
        auto res = ss->partOne->GetActorPos(actorFormId);
        Position pos = { res[0], res[1], res[2] };
        return pos;
    }

    __declspec(dllexport) uint32_t GetActorCellOrWorld(ScampServer* ss, uint32_t actorFormId)
    {
        return ss->partOne->GetActorCellOrWorld(actorFormId);
    }

    __declspec(dllexport) void DestroyActor(ScampServer* ss, uint32_t actorFormId)
    {
        ss->partOne->DestroyActor(actorFormId);
    }

    __declspec(dllexport) void SetRaceMenuOpen(ScampServer* ss, uint32_t actorFormId, bool open)
    {
        ss->partOne->SetRaceMenuOpen(actorFormId, open);
    }

    __declspec(dllexport) uint32_t* GetActorsByProfileId(ScampServer* ss, ProfileId profileId, size_t * result_len)
    {
        std::set<uint32_t> res = ss->partOne->GetActorsByProfileId(profileId);

        if (res.size() > 0) {
            uint32_t* data = (uint32_t*)malloc(res.size() * sizeof(uint32_t));

            //TODO check
            std::set<uint32_t>::iterator it = res.begin();
            for (size_t i = 0; i < res.size(); i++)
            {
                *(data + i * sizeof(uint32_t)) = *it;
                it++;
            }

            *result_len = res.size();

            return data;
        }
        else {
            *result_len = 0;
            return NULL;
        }
    }

    __declspec(dllexport) void SetEnabled(ScampServer* ss, uint32_t actorFormId, bool enabled)
    {
        ss->partOne->SetEnabled(actorFormId, enabled);
    }

    __declspec(dllexport) Networking::UserId GetUserByActor(ScampServer* ss, uint32_t formId)
    {
        return ss->partOne->GetUserByActor(formId);
    }

    __declspec(dllexport) void AttachSaveStorage(ScampServer* ss)
    {
        ss->partOne->AttachSaveStorage(CreateSaveStorage(CreateDatabase(ss->serverSettings, ss->logger), ss->logger));
    }

    __declspec(dllexport) void SendCustomPacket(ScampServer* ss, Networking::UserId userId, char* data)
    {
        std::string content = std::string(data);
        ss->partOne->SendCustomPacket(userId, content);
    }
    /*
    Napi::Value CreateBot(const Napi::CallbackInfo& info)
    {
        if (!this->serverMock)
            throw Napi::Error::New(info.Env(), "Bad serverMock");

        auto bot = std::make_shared<Bot>(this->serverMock->CreateClient());

        auto jBot = Napi::Object::New(info.Env());

        jBot.Set(
            "destroy",
            Napi::Function::New(info.Env(), [bot](const Napi::CallbackInfo& info) {
                bot->Destroy();
                return info.Env().Undefined();
                }));
        jBot.Set(
            "send",
            Napi::Function::New(info.Env(), [bot](const Napi::CallbackInfo& info) {
                auto standardJson = info.Env().Global().Get("JSON").As<Napi::Object>();
                auto stringify = standardJson.Get("stringify").As<Napi::Function>();
                std::string s;
                s += Networking::MinPacketId;
                s += (std::string)stringify.Call({ info[0] }).As<Napi::String>();
                bot->Send(s);

                // Memory leak fix
                // TODO: Provide tick API
                bot->Tick();

                return info.Env().Undefined();
                }));

        return jBot;
    }

    Napi::Value SetSendUiMessageImplementation(
        const Napi::CallbackInfo& info)
    {
        try {
            Napi::Function fn = info[0].As<Napi::Function>();
            sendUiMessageImplementation = Napi::Persistent(fn);
        }
        catch (std::exception& e) {
            throw Napi::Error::New(info.Env(), (std::string)e.what());
        }
        return info.Env().Undefined();
    }

    void OnUiEvent(uint32_t formId, std::string msg)
    {
        //call js function `onUiEvent`
    }

    void Clear()
    {
        try {
            gamemodeApiState = GamemodeApi::State();
            partOne->NotifyGamemodeApiStateChanged(gamemodeApiState);
        }
        catch (std::exception& e) {
            throw (std::string)e.what();
        }
    }

    __declspec(dllexport) void RegisterPapyrusFunction(std::string className, std::string funcName, std::string callType, ) {
        auto& vm = partOne->worldState.GetPapyrusVm();
        
        std::vector<VarValue> args(array_name_from, array_name_to);

        FunctionType fType;
        if (callType == "method") {
            fType = FunctionType::Method;
        }
        else if (callType == "global") {
            fType = FunctionType::GlobalFunction;
        }
        else {
            throw std::runtime_error("Unknown callType " + callType);
        }

        vm.RegisterFunction(className, funcName, fType, args);
    }*/
};