#include "AsyncSaveStorage.h"
#include "FileDatabase.h"
#include "MigrationDatabase.h"
#include "MongoDatabase.h"
#include "Networking.h"
#include "NetworkingCombined.h"
#include "NetworkingMock.h"
#include "PartOne.h"
#include "ScriptStorage.h"
#include "formulas/TES5DamageFormula.h"
//#include <JsEngine.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <string>
#include <iostream>

#define SCAMPLIB_IMPLEMENTATION
#include "ScampLib.h"

class ScampServerListener : public PartOne::Listener
{
public:
    ScampServerListener() {
        connect = [](Networking::UserId userId) {};
        disconnect = [](Networking::UserId userId) {};
        customPacket = [](Networking::UserId userId, char* content) {};
    }

    void setConnectHandler(onConnectFn* handler) {
        connect = handler;
    }

    void setDisconnectHandler(onDisconnectFn* handler) {
        disconnect = handler;
    }

    void setCustomPacketHandler(OnCustomPacketFn* handler) {
        customPacket = handler;
    }

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

    onConnectFn* connect;
    onDisconnectFn* disconnect;
    OnCustomPacketFn* customPacket; 
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
    GamemodeApi::State gamemodeApiState;
};

std::shared_ptr<IDatabase> CreateDatabase(nlohmann::json settings, std::shared_ptr<spdlog::logger> logger)
{
    auto databaseDriver = settings.count("databaseDriver")
        ? settings["databaseDriver"].get<std::string>()
        : std::string("file");

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
    //================================================================
    //ScampServer
    //================================================================
    SLExport ScampServer* CreateServer(uint32_t port, uint32_t maxConnections)
    {
        ScampServer* ss = new ScampServer();

        ss->partOne.reset(new PartOne);
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
        ss->partOne->SetDamageFormula(std::make_unique<TES5DamageFormula>());
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

        return ss;
    }

    SLExport void SetConnectHandler(ScampServer* ss, onConnectFn* handler) {
        ss->listener->setConnectHandler(handler);
    }

    SLExport void SetDisconnectHandler(ScampServer* ss, onDisconnectFn* handler) {
        ss->listener->setDisconnectHandler(handler);
    }

    SLExport void SetCustomPacketHandler(ScampServer* ss, OnCustomPacketFn* handler) {
        ss->listener->setCustomPacketHandler(handler);
    }

    SLExport void SetPacketHandler(ScampServer* ss, PacketHandlerFn* handler) {
        ss->partOne->SetPacketHandler(handler);
    }

    SLExport Option_void Tick(ScampServer* ss)
    {
        try {
            ss->server->Tick(PartOne::HandlePacket, ss->partOne.get());
            ss->partOne->Tick();
            return Option_void { 0 };
        }
        catch (std::exception& e) {
            auto msg = CreateString((char*)e.what());
            return Option_void{ 2, _v_error(msg), { 0 } };
        }
        catch (...) {
            auto msg = CreateString("Unknown error");
            return Option_void{ 2, _v_error(msg), { 0 } };
        }
    }

    SLExport uint32_t CreateActor(ScampServer* ss, uint32_t formId, Position vpos, float angleZ, uint32_t cellOrWorld, int32_t profileId)
    {
        NiPoint3 pos = NiPoint3(vpos.x, vpos.y, vpos.z);
        return ss->partOne->CreateActor(formId, pos, angleZ, cellOrWorld, profileId);
    }

    SLExport void SetUserActor(ScampServer* ss, unsigned short userId, uint32_t actorFormId)
    {
        ss->partOne->SetUserActor(userId, actorFormId);
    }

    SLExport uint32_t GetUserActor(ScampServer* ss, unsigned short userId)
    {
        return ss->partOne->GetUserActor(userId);
    }

    SLExport MpActor* ActorByUser(ScampServer* ss, unsigned short userId)
    {
        return ss->partOne->serverState.ActorByUser(userId);
    }

    SLExport char* GetActorName(ScampServer* ss, uint32_t actorFormId)
    {
        return (char*)ss->partOne->GetActorName(actorFormId).c_str();
    }

    SLExport Position GetActorPos(ScampServer* ss, uint32_t actorFormId)
    {
        auto res = ss->partOne->GetActorPos(actorFormId);
        Position pos = { res[0], res[1], res[2] };
        return pos;
    }

    SLExport uint32_t GetActorWorldOrCell(ScampServer* ss, uint32_t actorFormId)
    {
        return ss->partOne->GetActorCellOrWorld(actorFormId);
    }

    SLExport void DestroyActor(ScampServer* ss, uint32_t actorFormId)
    {
        ss->partOne->DestroyActor(actorFormId);
    }

    SLExport void SetRaceMenuOpen(ScampServer* ss, uint32_t actorFormId, bool open)
    {
        ss->partOne->SetRaceMenuOpen(actorFormId, open);
    }

    SLExport uint32_t* GetActorsByProfileId(ScampServer* ss, int32_t profileId, size_t * result_len)
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

    SLExport void SetEnabled(ScampServer* ss, uint32_t actorFormId, bool enabled)
    {
        ss->partOne->SetEnabled(actorFormId, enabled);
    }

    SLExport Networking::UserId GetUserByActor(ScampServer* ss, uint32_t formId)
    {
        return ss->partOne->GetUserByActor(formId);
    }

    SLExport void AttachSaveStorage(ScampServer* ss)
    {
        ss->partOne->AttachSaveStorage(CreateSaveStorage(CreateDatabase(ss->serverSettings, ss->logger), ss->logger));
    }

    SLExport void SendCustomPacket(ScampServer* ss, unsigned short userId, char* data)
    {
        ss->partOne->SendCustomPacket(userId, data);
    }

    SLExport uint32_t FindHoster(ScampServer* ss, uint32_t formId) {
        auto it = ss->partOne->worldState.hosters.find(formId);
        auto hosterId = it == ss->partOne->worldState.hosters.end() ? 0 : it->second;
        return hosterId;
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
    */

    /*
    SLExport void Clear(ScampServer* ss) {
        ss->gamemodeApiState = GamemodeApi::State();
        ss->partOne->NotifyGamemodeApiStateChanged(ss->gamemodeApiState);
    }

    SLExport void MakeEventSource(ScampServer* ss, char* eventName, char* functionBody) {
        std::string _eventName = std::string(eventName);

        if (ss->gamemodeApiState.createdEventSources.count(_eventName)) {
            throw std::runtime_error("'eventName' must be unique");
        }

        ss->gamemodeApiState.createdEventSources[_eventName] = { std::string(functionBody) };
        ss->partOne->NotifyGamemodeApiStateChanged(ss->gamemodeApiState);
    }*/

    /*
    //================================================================
    //VM
    //================================================================

    typedef VarValue* VmValue;
    typedef VarValue** VmFunctionArgs;
    typedef VarValue* (*VmFunction)(VmFunctionArgs args, size_t args_len);

    SLExport VmValue VmValueCreateBool(bool val) {
        return new VarValue(val);
    }

    SLExport VmValue VmValueCreateInt(int32_t val) {
        return new VarValue(val);
    }

    SLExport VmValue VmValueCreateFloat(double val) {
        return new VarValue(val);
    }

    SLExport VmValue VmValueCreateString(char* val) {
        return new VarValue(val);
    }

    SLExport VmValue VmValueCreateNone() {
        return new VarValue(VarValue::None());
    }

    SLExport void RegisterVmFunction(ScampServer* ss, char* className, char* funcName, FunctionType funcType, VmFunction f) {
        auto& vm = ss->partOne->worldState.GetPapyrusVm();
        
        vm.RegisterFunction(className, funcName, funcType,
            [f](const VarValue& self, const std::vector<VarValue>& args) {
                VmFunctionArgs arr = new VmValue[args.size() + 1];

                arr[0] = (VarValue*)&self;

                for(size_t i = 0; i < args.size(); i++) {
                    arr[i + 1] = (VarValue*)&args[i];
                }

                return *f(arr, args.size() + 1);
            }
        );
    }

    SLExport VmValue CallVmFunction(ScampServer* ss, FunctionType funcType, char* className, char* funcName, VmFunctionArgs args, size_t args_len) {
        auto& vm = ss->partOne->worldState.GetPapyrusVm();


        VarValue res;

        std::vector<VarValue> vector_args = std::vector<VarValue>(args[1], args[args_len]);

        if (funcType == FunctionType::Method) {
            res = vm.CallMethod(static_cast<IGameObject*>(*args[0]), funcName, vector_args);
        }
        else {
            res = vm.CallStatic(className, funcName, vector_args);
        }

        return new VarValue(res);
    }*/

    SLExport Option_server__MpObjectReference GetMpObjectReference(ScampServer* ss, uint32_t formId) {
        Option_server__MpObjectReference opt;
        try
        {
            MpObjectReference* ref = &ss->partOne->worldState.GetFormAt<MpObjectReference>(formId);

            MpObjectReference* val[] = { ref };
            opt_ok(&val, (Option*)(&opt), sizeof(MpObjectReference*));

            return opt;
        }
        catch (const std::exception& e)
        {
            auto msg = CreateString((char*)e.what());
            return Option_server__MpObjectReference{ 2, _v_error(msg), { 0 } };
        }
    }

    SLExport Option_server__MpActor GetMpActor(ScampServer* ss, uint32_t formId) {
        Option_server__MpActor opt;
        try
        {
            MpActor* ac = &ss->partOne->worldState.GetFormAt<MpActor>(formId);
            MpActor* val[] = { ac };
            opt_ok(&val, (Option*)(&opt), sizeof(MpActor*));
            return opt;
        }
        catch (const std::exception& e)
        {
            auto msg = CreateString((char*)e.what());
            return Option_server__MpActor{ 2, _v_error(msg), { 0 } };
        }
    }

    //================================================================
    //MpObjectReference
    //================================================================
    SLExport FormDesc* GetCellOrWorld(MpObjectReference* ref) {
        return (FormDesc*)&ref->GetCellOrWorld();
    }

    SLExport void SetCellOrWorld(MpObjectReference* ref, FormDesc* desc) {
        ref->SetCellOrWorld(*desc);
    }

    SLExport Position GetAngle(MpObjectReference* ref) {
        auto angle = ref->GetAngle();
        return Position{ angle.x, angle.y, angle.z };
    }

    SLExport void SetAngle(MpObjectReference* ref, Position angle) {
        ref->SetAngle(NiPoint3(angle.x, angle.y, angle.z));
    }

    SLExport Position GetPosition(MpObjectReference* ref) {
        try
        {
            auto pos = ref->GetPos();
            return Position{ pos.x, pos.y, pos.z };
        }
        catch (const std::exception& e)
        {
            throw e.what();
        }
        catch (...) {
            throw "unk err";
        }
    }

    SLExport void SetPosition(MpObjectReference* ref, Position pos) {
        ref->SetPos(NiPoint3(pos.x, pos.y, pos.z));
    }

    //================================================================
    //MpActor
    //================================================================
};