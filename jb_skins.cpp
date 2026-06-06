#include "jb_skins.h"
#include <random>
#include <cstdio>
#include <algorithm>
#include <random>

jb_skins g_jb_skins;
PLUGIN_EXPOSE(jb_skins, g_jb_skins);

#define CS_TEAM_NONE 0
#define CS_TEAM_SPECTATOR 1
#define CS_TEAM_T 2
#define CS_TEAM_CT 3

// SYSTEM API`s
IVEngineServer2* engine = nullptr;
CGlobalVars* gpGlobals = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;


std::random_device g_rd;
std::mt19937 g_gen(g_rd());


// API
IUtilsApi* utils;
IJailbreakApi* jailbreak_api;
IResourcePrecacher* precacher_api;

std::map<std::string,std::string> mWardenModels;
std::map<std::string,std::string> mCTModels;
std::map<std::string,std::string> mTModels;

void LoadConfig() {
    KeyValues* config = new KeyValues("Config");
    const char* path = "addons/configs/Jailbreak/skins.ini";
    if (!config->LoadFromFile(g_pFullFileSystem, path)) {
        utils->ErrorLog("%s Failed to load: %s", g_PLAPI->GetLogTag(), path);
        delete config;
        return;
    }

    KeyValues* warden = config->FindKey("WardenModels");
    if (warden){
        for (auto kv = warden->GetFirstSubKey();kv;kv = kv->GetNextKey()){
            std::string modelKey = kv->GetName();
            std::string modelPath = kv->GetString();
            mWardenModels[modelKey] = modelPath;
            META_CONPRINTF("[%s] Loaded model '%s' with path: %s\n",g_PLAPI->GetLogTag(),modelKey.c_str(),modelPath.c_str());
        }
    } else META_CONPRINTF("[%s] Cant find 'WardenModels' key in config.\n",g_PLAPI->GetLogTag());

    KeyValues* ctmodel = config->FindKey("CTModels");
    if (ctmodel){
        for (auto kv = ctmodel->GetFirstSubKey();kv;kv = kv->GetNextKey()){
            std::string modelKey = kv->GetName();
            std::string modelPath = kv->GetString();
            mCTModels[modelKey] = modelPath;
            META_CONPRINTF("[%s] Loaded model '%s' with path: %s",g_PLAPI->GetLogTag(),modelKey.c_str(),modelPath.c_str());
        }
    } else META_CONPRINTF("[%s] Cant find 'CTModels' key in config.\n",g_PLAPI->GetLogTag());

    KeyValues* tmodel = config->FindKey("TModels");
    if (tmodel){
        for (auto kv = tmodel->GetFirstSubKey();kv;kv = kv->GetNextKey()){
            std::string modelKey = kv->GetName();
            std::string modelPath = kv->GetString();
            mTModels[modelKey] = modelPath;
            META_CONPRINTF("[%s] Loaded model '%s' with path: %s",g_PLAPI->GetLogTag(),modelKey.c_str(),modelPath.c_str());
        }
    } else META_CONPRINTF("[%s] Cant find 'TModels' key in config.\n",g_PLAPI->GetLogTag());

    delete config;
}

CGameEntitySystem* GameEntitySystem() {
    return utils ? utils->GetCGameEntitySystem() : nullptr;
}

void StartupServer() {
    g_pGameEntitySystem = GameEntitySystem();
    g_pEntitySystem = utils->GetCEntitySystem();
    gpGlobals = utils->GetCGlobalVars();

    LoadConfig();
}

void SetModel(CCSPlayerPawn* pPawn, std::string path){
    if (!pPawn || path.empty()) return;
    utils->SetEntityModel(pPawn, path.c_str());
    utils->SetStateChanged(pPawn, "CBaseModelEntity", "m_nModelIndex");
    utils->SetStateChanged(pPawn, "CBaseModelEntity", "m_ModelName");
    Vector vPos = pPawn->GetAbsOrigin();
    utils->TeleportEntity(pPawn, &vPos, nullptr, nullptr);
}

std::string GetRandomModelFromMap(const std::map<std::string,std::string>& mModelsMap){
    if (mModelsMap.empty()) return "";

    std::uniform_int_distribution<size_t> dist(0, mModelsMap.size() - 1);
    auto it = std::next(mModelsMap.begin(), dist(g_gen));
    return it->second;
}

void OnPlayerSpawn(const char* szName, IGameEvent* pEvent, bool bDontBroadcast){
    int iSlot = pEvent->GetInt("userid");
    utils->NextFrame([iSlot](){
        auto pController = CCSPlayerController::FromSlot(iSlot);
        if (!pController) return;
        auto pPawn = pController->GetPlayerPawn();
        if (!pPawn || !pPawn->IsAlive()) return;

        int iTeam = pController->GetTeam();
        if (iTeam == CS_TEAM_CT) {
            std::string model = GetRandomModelFromMap(mCTModels);
            SetModel(pPawn,model);
            return;
        }
        if (iTeam == CS_TEAM_T) {
            std::string model = GetRandomModelFromMap(mTModels);
            SetModel(pPawn,model);
            return;
        }
    });

}

bool jb_skins::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) {
    PLUGIN_SAVEVARS();

    GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkSystem, INetworkSystem, NETWORKSYSTEM_INTERFACE_VERSION);

    ConVar_Register(FCVAR_SERVER_CAN_EXECUTE | FCVAR_GAMEDLL);
    g_SMAPI->AddListener(this, this);

    return true;
}

void jb_skins::AllPluginsLoaded() {
    int ret;
    utils = (IUtilsApi*)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        META_CONPRINTF("%s | Missing UTILS plugin.\n", g_PLAPI->GetLogTag());
        engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
        return;
    }

    jailbreak_api = (IJailbreakApi*)g_SMAPI->MetaFactory(JAILBREAK_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        META_CONPRINTF("%s | Missing Jailbreak Core plugin.\n", g_PLAPI->GetLogTag());
        engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
        return;
    }

    precacher_api = (IResourcePrecacher*)g_SMAPI->MetaFactory(RESOURCE_PRECACHER_INTERFACE, &ret, nullptr);
    if (ret == META_IFACE_FAILED) {
        META_CONPRINTF("%s | Missing ResourcePrecacher plugin.\n", g_PLAPI->GetLogTag());
        engine->ServerCommand(("meta unload " + std::to_string(g_PLID)).c_str());
        return;
    }

    LoadConfig();

    utils->HookEvent(g_PLID,"player_spawn",OnPlayerSpawn);

    for (const auto& model : mWardenModels) precacher_api->AddPrecache(model.second.c_str());
    for (const auto& model : mCTModels) precacher_api->AddPrecache(model.second.c_str());
    for (const auto& model : mTModels) precacher_api->AddPrecache(model.second.c_str());

    jailbreak_api->OnNewWardenListener(g_PLID,[](int iSlot){
        auto pController = CCSPlayerController::FromSlot(iSlot); 
        if (!pController) return;
        auto pPawn = pController->GetPlayerPawn();
        if (!pPawn || !pPawn->IsAlive()) return;
        std::string model = GetRandomModelFromMap(mWardenModels);
        SetModel(pPawn,model);
    });

    utils->StartupServer(g_PLID, StartupServer);
}

bool jb_skins::Unload(char* error, size_t maxlen) {
    jailbreak_api->ClearAllPluginHooks(g_PLID);
    utils->ClearAllHooks(g_PLID);
    ConVar_Unregister();
    return true;
}

const char* jb_skins::GetAuthor() { return "niffox"; }
const char* jb_skins::GetDate() { return __DATE__; }
const char* jb_skins::GetDescription() { return "[JB] Skins"; }
const char* jb_skins::GetLicense() { return "Private"; }
const char* jb_skins::GetLogTag() { return "[JB] Skins"; }
const char* jb_skins::GetName() { return "[JB] Skins"; }
const char* jb_skins::GetURL() { return "https://t.me/niffox_2q"; }
const char* jb_skins::GetVersion() { return "1.0.0"; }