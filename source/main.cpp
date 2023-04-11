/*
# Playstation 3 Cloud Drive
# Copyright (C) 2013-2014 Mohammad Haseeb aka MHAQS
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
*/

#include <iostream>
#include <string.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>

#include <sys/systime.h>
#include <sys/file.h>
#include <io/pad.h>

#include <sysutil/msg.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <sysutil/save.h>

#include <net/net.h>
#include <net/netctl.h>
#include <netinet/in.h>
#include <algorithm>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_thread.h>

#include <polarssl/md5.h>

#include "JsonResponse.h"
#include "Download.h"
#include "log.h"
#include "main.h"
#include "MainHelper.h"

#include "GoogleApiImpl.h"
#include "KoofrApiImpl.h"

/*  Function declarations  */
    void get_free_memory();
    bool pathExists(std::string pathToCheck);

    void sysevent_callback(u64 status, u64 param, void *userdata);
    static void dialog_handler(msgButton button, void *usrData);
    void draw_surface(SDL_Surface *destination, SDL_Surface *source, int x, int y);
    SDL_Surface *Load_Image(std::string filePath);

    void storeConfig();
    std::string isUserAuthenticated();

    std::string getDirectoryIdByName(std::string folderName);
    void writeRemoteResourceChangesToFile();
    void uploadChanges(); 
    void detectChangesForSync();
    std::string registerDevice();
    void buildRemoteResourceTree();
    void buildLocalResourceTree();
    int initCloudDrive(void *arg);

const std::string APP_TITLE     = "ps3cloudsync";
const std::string JSON_MIME     = "application/json";
const std::string DIR_MIME      = "application/vnd.google-apps.folder";
const std::string OCTET_MIME    = "application/octet-stream";
const std::string APP_PATH              = "/dev_hdd0/game/PSCS00001/USRDIR/";
const std::string appConfigFile               = APP_PATH+APP_TITLE+".conf";
const std::string appConfigFileBackup         = APP_PATH+APP_TITLE+".conf.backup";
const std::string LOCAL_RESOURCE_CONFIG_FILENAME     = APP_PATH+"local.json";
const std::string REMOTE_RESOURCE_CONFIG_FILENAME    = APP_PATH+"remote.json";
const std::string REMOTE_RESOURCE_CONFIG_BACKUP      = APP_PATH+"remote.backup";

OAuth2* authToken;
APIInterface *api;

/** app configuration file variables */
typedef struct {
    std::string selectedApi = "google"; //by default use google api config (limited)
    std::string rootFolderId = "root";
    std::string deviceId     = "nill";
    std::string remoteJsonId = "root";
    std::string last_sync = "0";
    std::string api_scope = "";
    u64 curr_sync_ts = 0;
} SelectedApiConf_t;
SelectedApiConf_t API_CONF;

Json localResourceRoot;
Json* remoteResourceRoot;

SDL_Event event;
SDL_Surface* screen = NULL;
SDL_Surface *image = NULL;
TTF_Font *Sans;
SDL_Color GREY = {57, 57, 57};

msgType dialog_type;
static int dialog_action = 0;

int exitapp = 0;
int xmbopen = 0;
int syncing = 0;

double progFuncLastProgress = 0.0;

int appWidth = 1280;
int appHeight = 720;

//one
//  First Time: App never run AND no data on Google Drive
//two 
//  Second Time: App ran before AND data possibly exists on Google Drive
//three
//  First Time: App is installed on a new system but Data exists on Google Drive 
//@see initCloudDrive() for details
std::string userType = "one";

typedef struct {
	uint32_t total;
	uint32_t avail;
} _meminfo;
_meminfo meminfo;

int main(int argc, char **argv)
{
    padInfo padinfo;
    padData paddata;
    SDL_Thread *thread = NULL;

    std::string imagePath;
    std::string FontPath = APP_PATH+"font.ttf";

    netInitialize();
    debugInit();

    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();

    videoState state;
    videoGetState(0, 0, &state);
    videoResolution res;
    videoGetResolution(state.displayMode.resolution, &res);

    debugPrintf("Resolution %u,%u\n", res.width, res.height);

    appWidth = res.width;
    appHeight = res.height;

    switch (appHeight)
    {
    case 1080:
        imagePath = APP_PATH+"bg.png";
        break;
    case 720:
        imagePath = APP_PATH+"bg-720.png";
        break;
    case 480:
        imagePath = APP_PATH+"bg-480.png";
        break;
    case 576:
        imagePath = APP_PATH+"bg-576.png";
        break;
    default:
        imagePath = APP_PATH+"bg.png";
        break;
    }

    screen = SDL_SetVideoMode(appWidth, appHeight, 32, SDL_SWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN);

    image = Load_Image(imagePath);
    draw_surface(screen, image, 0, 0);

    Sans = TTF_OpenFont(FontPath.c_str(), 20.0);

    // current date/time based on current system
    sysGetCurrentTime(&API_CONF.curr_sync_ts, new u64()); 
    debugPrintf("Current sync time: %s -> %lld sec\n", epochTsToString((time_t*) &API_CONF.curr_sync_ts).c_str(), API_CONF.curr_sync_ts);

    thread = SDL_CreateThread(initCloudDrive, NULL);

    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sysevent_callback, NULL);

    ioPadInit(7);

    dialog_action = 0;
    dialog_type = static_cast<msgType>(MSG_DIALOG_MUTE_ON | MSG_DIALOG_DOUBLE_PROGRESSBAR);

    while (exitapp == 0)
    {
        sysUtilCheckCallback();

        if (xmbopen != 1)
        {
            ioPadGetInfo(&padinfo);
            for (int i = 0; i < MAX_PADS; i++)
            {
                if (padinfo.status[i])
                {
                    ioPadGetData(i, &paddata);
                    if (paddata.BTN_CROSS)
                    {
                        debugPrintf("Quit Called...%d\n", exitapp);
                        if (syncing == 0)
                        {
                            dialog_action = 2;
                            exitapp = 1; // Quit application
                        }
                    }
                }
            }
        }
        SDL_Flip(screen);
    }

    debugPrintf("The application exited...%d\n", exitapp);

    msgDialogAbort();

    SDL_KillThread(thread);
    SDL_FreeSurface(image);
    SDL_FreeSurface(screen);
    TTF_CloseFont(Sans);

    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    debugClose();
    netDeinitialize();
    ioPadEnd();

    return 0;
}

void get_free_memory()
{
	lv2syscall1(SYSCALL_MEMORY_GET_USER_MEMORY_SIZE, (uint64_t) &meminfo);
}

void sysevent_callback(u64 status, u64 param, void * userdata)
{
    switch(status)
    {
        case SYSUTIL_EXIT_GAME: exitapp = 1;	break;
        case SYSUTIL_MENU_OPEN: xmbopen = 1;	break;
        case SYSUTIL_MENU_CLOSE: xmbopen = 0;	break;
    }
}

static void dialog_handler(msgButton button, void *usrData)
{
    switch (button)
    {
        case MSG_DIALOG_BTN_OK:
                dialog_action = 1;
                break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
                dialog_action = 2;
                break;
        case MSG_DIALOG_BTN_NONE:
                dialog_action = -1;
                break;
        default:
                break;
    }
}

/**
 * Stores the current application configuration both in main and backup config json files.
 */
void storeConfig()
{
    debugPrintf(" Saving app configuration...\n");

    StdioFile file(appConfigFile);
    StdioFile backup(appConfigFileBackup, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, 0644);

    Json appConfig;
    if(file.Exists()){
        appConfig = Json::ParseFile(file);
    }
    file.OpenWithFlags(SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, 0644);    

    {//update only selected 'default_api' settings
        Json selectedApiConf;
        if(!appConfig.AsObject().empty() && appConfig.Has("default_api")){
            selectedApiConf = appConfig[API_CONF.selectedApi];
        }else{
            appConfig.Add("default_api",    Json(API_CONF.selectedApi));                    
        }

            selectedApiConf.Add("refresh_token",  Json(authToken->RefreshToken()));
            selectedApiConf.Add("device_id",      Json(authToken->DeviceCode())); //oauth2 device code
            selectedApiConf.Add("last_sync",      Json(epochTsToString((time_t*) &API_CONF.curr_sync_ts)));
            selectedApiConf.Add("remote_fid",     Json(API_CONF.remoteJsonId));
            selectedApiConf.Add("root_id",        Json(API_CONF.rootFolderId));
            selectedApiConf.Add("scope",          Json(authToken->getScope()));
        appConfig.Add(API_CONF.selectedApi, selectedApiConf);
    }

    appConfig.WriteFile(file);
    appConfig.WriteFile(backup);
    
    file.Close();
    backup.Close();
    
    sysFsChmod(appConfigFile.c_str(),0644);
    sysFsChmod(appConfigFileBackup.c_str(),0644);
}

/**
 * Utility function that reads the config file for refresh token etc
 * @return "not_found" if file not found or corrupt else the refresh_token
 * 
 */
std::string isUserAuthenticated()
{
    std::string result = "not_found";
    Json config;
    StdioFile file(appConfigFile);
    
    if(!file.Exists())
    //TODO try to read backup config file
    {
        debugPrintf("  Config file (%s) not found, returning empty handed...\n", appConfigFile.c_str());
        return result;
    }
    debugPrintf("  Config file (%s) found, parsing...\n", appConfigFile.c_str());
    
    config = Json::ParseFile(file);

    if(!config.AsObject().empty())
    {
        debugPrintf("  > Config file not empty, getting refresh token...\n");
        if(config.Has("refresh_token")){ //keep for backward compatibility
            result          = config["refresh_token"].Str();
            API_CONF.deviceId        = config.Has("device_id")   ? config["device_id"].Str() : "nill";
            API_CONF.rootFolderId    = config.Has("root_id")     ? config["root_id"].Str() : "root";
            API_CONF.last_sync       = config.Has("last_sync")   ? config["last_sync"].Str() : "0";
            API_CONF.remoteJsonId    = config.Has("remote_fid")  ? config["remote_fid"].Str() : "root";
            API_CONF.api_scope       = config.Has("scope")       ? config["scope"].Str() : "";

        }else if(config.Has("default_api")){
            API_CONF.selectedApi = config["default_api"].Str();
            if(API_CONF.selectedApi.empty()){ API_CONF.selectedApi = "google";} //assume default

            debugPrintf("  > trying selected api config: '%s'\n", API_CONF.selectedApi.c_str());

            Json selectedApi = config[API_CONF.selectedApi];

            result          = selectedApi["refresh_token"].Str();
            API_CONF.deviceId        = selectedApi.Has("device_id")  ? selectedApi["device_id"].Str() : "nill";
            API_CONF.rootFolderId    = selectedApi.Has("root_id")    ? selectedApi["root_id"].Str() : "root";
            API_CONF.last_sync       = selectedApi.Has("last_sync")  ? selectedApi["last_sync"].Str() : "0";
            API_CONF.remoteJsonId    = selectedApi.Has("remote_fid") ? selectedApi["remote_fid"].Str() : "root";
            API_CONF.api_scope       = selectedApi.Has("scope")      ? selectedApi["scope"].Str() : "";
        }
    }
    
    //init Oauth token based on default api
    if(API_CONF.selectedApi == "google"){ 
        api = new GoogleApiImpl();
        authToken           = api->m_auth_token;
        remoteResourceRoot  = &api->m_remote_resource_root;

    }else if(API_CONF.selectedApi == "koofr"){ 
        api = new KoofrApiImpl();
        authToken           = api->m_auth_token;
        remoteResourceRoot  = &api->m_remote_resource_root;
    }
            
    return result;
}

int transferProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if(dltotal > 0.0)
    {
        double curProg = (dlnow / dltotal) * 100;
        if(curProg - progFuncLastProgress > 1)
        {
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, curProg - progFuncLastProgress);
            
            debugPrintf("DOWN: %f of %f P: %f C: %f\r\n", dlnow, dltotal, progFuncLastProgress,curProg);
            
            progFuncLastProgress = curProg;
        }
    }
    
    if(ultotal > 0.0)
    {
        double curProg = (ulnow / ultotal) * 100;
        if(curProg - progFuncLastProgress > 1)
        {
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, curProg - progFuncLastProgress);
            
            debugPrintf("UP: %f of %f P: %f C: %f\r\n", ulnow, ultotal, progFuncLastProgress,curProg);
                    
            progFuncLastProgress = curProg;
        }
    }
    
    return 0;
}

std::string getDirectoryIdByName(std::string folderName)
{
    std::string result = "not_found";
    if(folderName == APP_TITLE)
    {
        result = API_CONF.rootFolderId;
    }
    else
    {
        int remSize = (*remoteResourceRoot)["data"].AsArray().size();
        for(int i=0; i < remSize; i++)
        {
            Json resource = (*remoteResourceRoot)["data"][i];
            if(resource["mimeType"].Str() == DIR_MIME)
            {
                if(resource["title"].Str() == folderName)
                {
                    result = resource["id"].Str();
                }
            }
        }
    }
    
    debugPrintf("getDirectoryIdByName: Returning Parent id %s for folder %s\n", result.c_str(), folderName.c_str());
    
    return result;
}

/**
 *  Write remoteResourceRoot json tree into remote.json, but first into remote.backup.
 */
void writeRemoteResourceChangesToFile()
{
    debugPrintf("writeRemoteResourceChangesToFile: initiating remote.json write...\n");
    //Backup remote.json before a write to save from any catastrophies
    StdioFile backup(REMOTE_RESOURCE_CONFIG_BACKUP, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, 0644);
    remoteResourceRoot->WriteFile(backup);
    backup.Close();
    sysFsChmod(REMOTE_RESOURCE_CONFIG_BACKUP.c_str(),0644);
    
    StdioFile file(REMOTE_RESOURCE_CONFIG_FILENAME, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, 0644);
    remoteResourceRoot->WriteFile(file);
    file.Close();
    sysFsChmod(REMOTE_RESOURCE_CONFIG_FILENAME.c_str(),0644);
    
    debugPrintf("writeRemoteResourceChangesToFile: Changes to remote.json written successfully\n");
}

/**
 * Uploads everything marked in remote resource Tree 
 */
void uploadChanges()
{
    // First make sure that all folders are uploaded, we need this because file
    // resources in Google Drive don't have paths, instead they rely on
    // parent ids
    u32 remSize = (*remoteResourceRoot)["data"].AsArray().size();
    u32 i=0;
    u32 syncJson = 0;
    u32 progressCounter =0;
    u32 totalProgress = 0;
    u32 prevProgress = 0;
    //u32 globProgress = 0;
    std::string result;
    
    for(i=0; i < remSize; i++)
    {
        Json resource = (*remoteResourceRoot)["data"][i];
        if(resource["status"].Str() == RESOURCE_REMOTE_STATUS_UPLOAD_KEY || resource["status"].Str() == RESOURCE_REMOTE_STATUS_UPDATE_KEY)
        {
            totalProgress++;
        }
    }
    
    debugPrintf("  uploadChanges Step 1: Creating directories first on Google Drive [%d]\n",totalProgress);
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Uploading Parent Entries...");    
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, 0);
                    
    for(i=0; i < remSize; i++)//sync missing directories
    {
        if(dialog_action != 0) { syncJson = 0; break; }
        
        Json resource = (*remoteResourceRoot)["data"][i];
        
        if(resource["status"].Str() == RESOURCE_REMOTE_STATUS_UPLOAD_KEY)
        {
            if(resource["mimeType"].Str() != DIR_MIME) { continue; }
            
            result = getDirectoryIdByName(resource["parentFolder"].Str());                
            if(result == "not_found"){ continue; }
                
            debugPrintf("  > uploadChanges: Creating Directory with name %s under %s\n",resource["title"].Str().c_str(), result.c_str() );
            
            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, 0);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, resource["path"].Str().c_str());

            Json obj = api->uploadDirectory(resource["title"].Str(),result);
            if(!obj.Has("id")){ continue; }
            
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, 100);
            
            (*remoteResourceRoot)["data"][i].Add("id",Json(obj["id"].Str()));
            (*remoteResourceRoot)["data"][i].Add("parentid",Json(result));
            (*remoteResourceRoot)["data"][i].Add("modifiedDate",Json(obj["modifiedDate"].Str()));
            (*remoteResourceRoot)["data"][i].Add("status", Json(RESOURCE_REMOTE_STATUS_SYNCED_KEY));
            writeRemoteResourceChangesToFile();
            
            syncJson = 1;
            progressCounter++;
            
            std::stringstream ss;
            ss<<"Uploading Parent Entries: "<<progressCounter<<" of "<<totalProgress;
                
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0,ss.str().c_str());
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                                
            /*if(userType == "one")
            {
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
            }
            if(userType == "two")
            {
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 40) / totalProgress) - globProgress);
            }
            globProgress = (progressCounter * 40) / totalProgress;*/
            prevProgress = (progressCounter * 100) / totalProgress;

        }//END_of status == upload
    }
   
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Uploading Child Entries...");    
    debugPrintf("  uploadChanges Step 2: Creating files on Google Drive\n");
        
    for(i=0; i < remSize; i++)
    {   
        if(dialog_action != 0) {break; }
        
        Json resource = (*remoteResourceRoot)["data"][i];
        
        if(resource["status"].Str() == RESOURCE_REMOTE_STATUS_UPLOAD_KEY)
        {
            if(resource["mimeType"].Str() != DIR_MIME)
            {
                result = getDirectoryIdByName(resource["parentFolder"].Str());
             
                if(result != "not_found")
                {
                    debugPrintf("  > uploadChanges: Creating file with name %s under %s\n",resource["title"].Str().c_str(), result.c_str() );
                    
                    Json obj = api->uploadFile(resource["path"].Str(),resource["title"].Str(),OCTET_MIME,result);
                    
                    if(!obj.Has("id")){continue;}
                    
                    (*remoteResourceRoot)["data"][i].Add("id", Json(obj["id"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("parentid", Json(result));
                    (*remoteResourceRoot)["data"][i].Add("modifiedDate", Json(obj["modifiedDate"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("fileSize",  Json(obj["fileSize"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("md5Checksum", Json(obj["md5Checksum"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("status", Json(RESOURCE_REMOTE_STATUS_SYNCED_KEY));

                    writeRemoteResourceChangesToFile();
                    syncJson = 1;
                    progressCounter++;
                    
                    std::stringstream ss;
                    ss<<"Uploading Child Entries: "<<progressCounter<<" of "<<totalProgress;
                        
                    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0,ss.str().c_str());
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    
                    /*if(userType == "one")
                    {
                            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    }
                    if(userType == "two")
                    {
                            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 40) / totalProgress) - globProgress);
                    }
                    globProgress = (progressCounter * 40) / totalProgress;*/
                    prevProgress = (progressCounter * 100) / totalProgress;
                    
                    //get_free_memory();
                    //debugPrintf("System Memory: %u, %u\n",meminfo.avail,meminfo.total);
                }//END_of isFound
                //else mark this file to be deleted, if this feature is ever required
            }//END_of if !isDirectory
        }//END_of status == upload
        else if(resource["status"].Str() == RESOURCE_REMOTE_STATUS_UPDATE_KEY)
        {
            if(resource["mimeType"].Str() != DIR_MIME)
            {
                result = getDirectoryIdByName(resource["parentFolder"].Str());
             
                if(result != "not_found")
                {
                    debugPrintf("  > uploadChanges: Re-visioning file with name %s under %s\n",resource["title"].Str().c_str(), result.c_str() );
                    
                    Json obj = api->uploadFile(resource["path"].Str(),resource["title"].Str(),OCTET_MIME,result, resource["id"].Str());

                    if(!obj.Has("id")) {continue;}
                    
                    (*remoteResourceRoot)["data"][i].Add("modifiedDate", Json(obj["modifiedDate"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("fileSize",  Json(obj["fileSize"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("md5Checksum", Json(obj["md5Checksum"].Str()));
                    (*remoteResourceRoot)["data"][i].Add("status", Json(RESOURCE_REMOTE_STATUS_SYNCED_KEY));

                    writeRemoteResourceChangesToFile();
                    syncJson = 1;
                    progressCounter++;
                    
                    std::stringstream ss;
                    ss<<"Uploading Child Entries: "<<progressCounter<<" of "<<totalProgress;
                        
                    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0,ss.str().c_str());
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    
                    /*if(userType == "one")
                    {
                            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    }
                    if(userType == "two")
                    {
                            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 40) / totalProgress) - globProgress);
                    }
                    globProgress = (progressCounter * 40) / totalProgress;*/
                    prevProgress = (progressCounter * 100) / totalProgress;
                    
                    //get_free_memory();
                    //debugPrintf("System Memory: %u, %u\n",meminfo.avail,meminfo.total);
                }//END_of if wasFileFound -> update
            }//END_of if !isDirectory
        }//END_of if update
    }//END_of for loop
    
    debugPrintf("  uploadChanges Step 3: Uploading remote.json to google drive\n" );
       
    if(API_CONF.remoteJsonId == "root")
    {
        Json remJson;
        remJson = api->uploadFile(REMOTE_RESOURCE_CONFIG_FILENAME,"remote.json",JSON_MIME,API_CONF.rootFolderId);
        if(remJson.Has("id"))
        {
            API_CONF.remoteJsonId = remJson["id"].Str();
            storeConfig();
        }
    }
    else if(syncJson == 1)
    {
        debugPrintf("Updating remote.json on the server...\n" );
        api->uploadFile(REMOTE_RESOURCE_CONFIG_FILENAME,"remote.json",JSON_MIME,API_CONF.rootFolderId,API_CONF.remoteJsonId);
    }
}

/**
 * Compare localResourceTree with remoteResourceTree.
 *  1. If remote has no local resource, then add it to remoteResourceTree for upload
 *  2. If remote has local resource and it has "synced" status, then
 *    a) don't compare directories
 *    b) check file modification time and md5 checksum
 * 
 */
void detectChangesForSync()
{
    // Upload Mode only for now
    // 1 - If an entry exists in local but not in remote, add it and mark for upload
    // 2 - If an entry exists in local and in remote, check if it's changed by size (folder can be ignored)
    
    u32 lrtSize = localResourceRoot["data"].AsArray().size();
    u32 rrtSize = remoteResourceRoot->Has("data") ? (*remoteResourceRoot)["data"].AsArray().size() : 0;
    u32 i = 0;
    u32 j = 0;
    u32 prevProgress = 0;
    bool found;
     
    debugPrintf("  Detecting changes...\n");
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Detecting Changes...");
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, "Finding file differences...");
        
    for(i=0; i < lrtSize; i++)
    {
        found = false;
        
        if(dialog_action != 0){break; }
        
        Json lres = localResourceRoot["data"][i];
        
        for(j=0; j < rrtSize; j++)
        {
            Json rres = (*remoteResourceRoot)["data"][j];
            
            if(lres["path"].Str() == rres["path"].Str())
            {
                found = true;
                if(lres["mimeType"].Str() == DIR_MIME) {continue;}
                
                if(rres["status"].Str() == RESOURCE_REMOTE_STATUS_SYNCED_KEY) //if synced, check for modifications
                {
                    sysFSStat lstat;
                    sysFsStat(lres["path"].Str().c_str(),&lstat) ;

                    time_t fileModTime = lstat.st_mtime;

                    std::string remoteTime = rres["modifiedDate"].Str().substr(0,18);

                    struct tm remoteModTime = {0};
                    strptime(remoteTime.c_str(), "%Y-%m-%dT%H:%M:%S", &remoteModTime);

                    time_t remoteEpoch = mktime(&remoteModTime);

                    //debugPrintf("Times are %lld, %lld\n", (long long int)fileModTime, (long long int)remoteEpoch);

                    double seconds = difftime(fileModTime, remoteEpoch);
                    if (seconds > 0) 
                    {
                        debugPrintf("Local file has been modified %s\n", rres["path"].Str().c_str());
                        
                        std::string md5c = calculateMD5Checksum(lres["path"].Str());
                    
                        if(md5c != "nill")
                        {
                            if(md5c != rres["md5Checksum"].Str())
                            {    
                                debugPrintf("Local file MD5 has been changed %s,%s,%s\n", rres["path"].Str().c_str(),md5c.c_str(),rres["md5Checksum"].Str().c_str());
                                (*remoteResourceRoot)["data"][j].Add("status", Json(RESOURCE_REMOTE_STATUS_UPDATE_KEY));
                            }
                        }
                    }
                }//END_of if isSynced                
            }//END_of if found
        }//END_of for loop remoteResourceTree
              
        if(!found)
        {
            debugPrintf("  > Found New Entry in Local Resource, Adding for sync %s\n", lres["path"].Str().c_str());
            (*remoteResourceRoot)["data"].Add(lres);
        }
        
        std::stringstream ss;
        ss<<"Finding File Differences: "<<i<<" of "<<lrtSize;

        msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1,ss.str().c_str());
        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((i * 100) / lrtSize) - prevProgress);
        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, ((i * 100) / lrtSize) - prevProgress);

        /*if(userType=="two")
        {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((i * 20) / lrtSize) - globProgress);
        }*/

        //globProgress = (i * 20) / lrtSize;
        prevProgress = (i * 100) / lrtSize;

    }//END_of for loop localResourceTree

    //get_free_memory();
    //debugPrintf("System Memory: %u, %u\n",meminfo.avail,meminfo.total);
}

/**
 * Initiates OAuth2 user authentication for limited devices.
 *  As a result gets:
 *   - device code
 *   - access token
 *   - refresh token
 * 
 * @return "authorization_complete" if authenticated and "not_found" if it fails
 */
std::string registerDevice()
{
    std::string authStatus = "not_found";
    std::string usercode;

    debugPrintf("  Device Authentication Function Called...\n");
    usercode = authToken->DeviceAuth();

    debugPrintf("  > Device Code:  %s \n", authToken->DeviceCode().c_str());
    debugPrintf("  > User Code:  %s \n", usercode.c_str());
    
    std::string userCodePrint = "Your User Code is: " + usercode 
            + "\n\n Please visit "+authToken->getDeviceVerificationUrl().c_str()+" for approval"
        ;
    
    SDL_Surface *codeAuthText = NULL;
    codeAuthText = TTF_RenderText_Blended( Sans, userCodePrint.c_str(), GREY);
    draw_surface(screen, codeAuthText, (appWidth - codeAuthText->w) / 2, (86 * appHeight) / 100);

    u64 epochTime, nsec;
    u64 previousCallTime;

    sysGetCurrentTime(&epochTime, &nsec);
    previousCallTime = epochTime;

    while (authStatus != "authorization_complete")
    {
        sysGetCurrentTime(&epochTime, &nsec);
        if (epochTime - previousCallTime > 10)
        {
            debugPrintf("   Polling for user authentication....\n");
            authStatus = authToken->Auth();
            previousCallTime = epochTime;
        }
    }
    SDL_FreeSurface(codeAuthText);
    return authStatus;
}

/**
 * Builds a remote Resource Tree from root folder to every file there is
 */
void buildRemoteResourceTree()
{
    std::vector<Json> resourceTree;
    std::vector<std::string> foldersToTraverse;
    Json resourceObject = api->checkIfRemoteResourceExists(APP_TITLE,DIR_MIME,API_CONF.rootFolderId);
    if(resourceObject.Has("id"))
    {
        resourceTree.push_back(Json(resourceObject));
        foldersToTraverse.push_back(resourceObject["id"].Str());
        while(foldersToTraverse.size() > 0)
        {
            Json::Array tempArray = api->getResourcesUnderFolder(foldersToTraverse.back());
            if(!tempArray.empty())
            {
                for ( Json::Array::iterator i = tempArray.begin() ; i != tempArray.end() ; ++i )
                {
                    Json::Object tmpObject = i->AsObject();
                    if(tmpObject["mimeType"].Str() == DIR_MIME) foldersToTraverse.push_back(tmpObject["id"].Str());
                    resourceTree.push_back(Json(i->AsObject()));
                }          
            }
            foldersToTraverse.pop_back();
        }
    }
    
    debugPrintf("Remote Resource Tree built, writing to Remote Resource Json\n");

    remoteResourceRoot->Add("data",Json(resourceTree));
    
    writeRemoteResourceChangesToFile();
}

/**
 * Builds a local resource tree given the path of a local profile folder
 */
void buildLocalResourceTree()
{
    std::vector<Json> resourceTree;

    debugPrintf("Building Local Resource Tree....\n");

    sysFSDirent entry;
    s32 fd;
    u64 read;
    Header profileList;
    Header profilePaths;

    std::string entryName = "";
    std::string path = "/dev_hdd0/home";
   
    // Check how many profiles exists in user's HDD 0000001 etc
    if (sysFsOpendir(path.c_str(), &fd) == 0)
    {
        debugPrintf("  Checking profile home (%s)....\n", path.c_str());
        std::string previousEntry = "";
        while (!sysFsReaddir(fd, &entry, &read) && strlen(entry.d_name) > 0)
        {
            debugPrintf("  > Reading directory (%s)\n", entry.d_name);
            if(strcmp(previousEntry.c_str(),entry.d_name) == 0){
                debugPrintf("    Double reading resource (%s) - breaking loop (workaround for emulator).\n", entry.d_name);
                break;
            }

            previousEntry = entry.d_name;
            if (strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0) {
                continue;
            }
                       
            entryName.clear();
            entryName.assign(entry.d_name, strlen(entry.d_name));
            
            profileList.Add(entryName);
            profilePaths.Add(path + "/" + entryName + "/savedata");
            
            Json resource;
            resource.Add("id", Json("nill"));
            resource.Add("title", Json(entryName));
            resource.Add("path", Json(path + "/" + entryName + "/savedata"));
            resource.Add("mimeType", Json(DIR_MIME));
            resource.Add("parentid", Json(API_CONF.rootFolderId));
            resource.Add("parentFolder",Json(APP_TITLE));
            resource.Add("modifiedDate", Json("0"));
            resource.Add("status",Json(RESOURCE_REMOTE_STATUS_UPLOAD_KEY));

            resourceTree.push_back(Json(resource));            
        }
        sysFsClosedir(fd);
        fd = -1;
    }else{
        debugPrintf("  Failed to check profile home (%s)....\n", path.c_str());
    }

    Header::iterator k = profileList.begin();
    Header::iterator l = profilePaths.begin();
    Header folderList;
    Header pathList;
    
    // Recursively list all saves under every profile
    while (k != profileList.end())
    {
        debugPrintf("  Entering Save Path %s...\n",l->c_str());
        
        if (sysFsOpendir(l->c_str(), &fd) == 0)
        {
            std::string previousEntry = "";
            while (!sysFsReaddir(fd, &entry, &read) && strlen(entry.d_name) > 0)
            {
                debugPrintf("  > Reading directory (%s)\n", entry.d_name);
                if(strcmp(previousEntry.c_str(),entry.d_name) == 0){
                    debugPrintf("    Double reading resource (%s) - breaking loop.\n", entry.d_name);
                    break;
                }

                previousEntry = entry.d_name;
                if (strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0){ continue; }

                folderList.Add(entry.d_name);
                
                entryName.clear();
                entryName.assign(entry.d_name, strlen(entry.d_name));

                std::stringstream fPath;
                fPath<<*l<<"/"<<entryName;
                
                pathList.Add(fPath.str());
                
                //debugPrintf("Save Path Entry...%s\n",entryName.c_str());
                
                Json resource;
                resource.Add("id", Json("nill"));
                resource.Add("title", Json(entryName));
                resource.Add("path", Json(fPath.str()));
                resource.Add("mimeType", Json(DIR_MIME));
                resource.Add("parentid", Json("nill"));
                resource.Add("parentFolder",Json(k->c_str()));
                resource.Add("modifiedDate", Json("0"));
                resource.Add("status",Json(RESOURCE_REMOTE_STATUS_UPLOAD_KEY));

                resourceTree.push_back(Json(resource));
                
            }//END_of loop profile saves
            sysFsClosedir(fd);
            fd = -1;
        }
        ++k,++l;
    }//END_of loop profiles

    debugPrintf("  Directory enumeration complete, listing files inside them...\n");

    Header::iterator i = pathList.begin();
    Header::iterator j = folderList.begin();
    
    // Recursively list all files under every save directory
    while (i != pathList.end())
    {
        debugPrintf("  Reading dir (%s)....\n", i->c_str());
        if (sysFsOpendir(i->c_str(), &fd) == 0)
        {
            std::string previousEntry = "";
            while (!sysFsReaddir(fd, &entry, &read) && strlen(entry.d_name) > 0)
            {
                debugPrintf("  > Reading file (%s)\n", entry.d_name);
                if(strcmp(previousEntry.c_str(),entry.d_name) == 0){
                    debugPrintf("    Double reading resource (%s) - breaking loop.\n", entry.d_name);
                    break;
                }

                previousEntry = entry.d_name;
                if (strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0){ continue;}
                
                entryName.clear();
                entryName.assign(entry.d_name, strlen(entry.d_name));

                std::stringstream fPath;
                fPath<<*i<<"/"<<entryName;
                
                //debugPrintf("Entry Path...%s\n",fPath.str().c_str());
                
                Json resource;
                resource.Add("id", Json("nill"));
                resource.Add("title", Json(entryName));
                resource.Add("path", Json(fPath.str()));
                resource.Add("mimeType", Json(OCTET_MIME));
                resource.Add("parentFolder",Json(j->c_str()));
                resource.Add("parentid", Json("nill"));
                resource.Add("modifiedDate", Json("0"));
                resource.Add("fileSize",Json("0"));
                resource.Add("md5Checksum",Json("0"));
                resource.Add("status",Json(RESOURCE_REMOTE_STATUS_UPLOAD_KEY));

                resourceTree.push_back(Json(resource));
                
            }//END_of loop files in dirs
            sysFsClosedir(fd);
            fd = -1;
        }
        ++i;++j;
    }//END_of loop directories
    
    //Check for Existence of PS1 and PS2 memory cards
    if(pathExists("/dev_hdd0/savedata"))
    {
        if(pathExists("/dev_hdd0/savedata/vmc"))
        {
            if (sysFsOpendir("/dev_hdd0/savedata/vmc", &fd) == 0)
            {
                std::string previousEntry = "";
                while (!sysFsReaddir(fd, &entry, &read) && strlen(entry.d_name) > 0)
                {
                    debugPrintf("  > Reading directory (%s)\n", entry.d_name);
                    if(strcmp(previousEntry.c_str(),entry.d_name) == 0){
                        debugPrintf("    Double reading resource (%s) - breaking loop.\n", entry.d_name);
                        break;
                    }

                    previousEntry = entry.d_name;
                    if (strcmp(entry.d_name, ".") == 0 || strcmp(entry.d_name, "..") == 0){ continue;}
                                                    
                    entryName.clear();
                    entryName.assign(entry.d_name, strlen(entry.d_name));

                    Json resource;
                    resource.Add("id", Json("nill"));
                    resource.Add("title", Json(entryName));
                    resource.Add("path", Json("/dev_hdd0/savedata/vmc/"+entryName));
                    resource.Add("mimeType", Json(OCTET_MIME));
                    resource.Add("parentid", Json(API_CONF.rootFolderId));
                    resource.Add("parentFolder",Json(APP_TITLE));
                    resource.Add("modifiedDate", Json("0"));
                    resource.Add("status",Json(RESOURCE_REMOTE_STATUS_UPLOAD_KEY));

                    resourceTree.push_back(Json(resource));
                    
                }//END_of loop directory
                sysFsClosedir(fd);
                fd = -1;
            }
        }
    }

    debugPrintf(" Resource Tree built, writing local.json\n");

    // Overwrite local.json every time
    StdioFile file(LOCAL_RESOURCE_CONFIG_FILENAME, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, 0644);

    localResourceRoot.Add("data",Json(resourceTree));
    localResourceRoot.WriteFile(file);

    file.Close();
    sysFsChmod(LOCAL_RESOURCE_CONFIG_FILENAME.c_str(),0644);

    debugPrintf("Resource file has been written successfully...\n");
}

/**
 * Starts the authentication and sync process, should be the Drive class
 */
int initCloudDrive(void *arg)
{
    debugPrintf("Checking if User is Authenticated...\n");
    std::string authStatus = isUserAuthenticated();
    
    SDL_Surface *checkingAuthText = NULL;
    checkingAuthText = TTF_RenderText_Blended( Sans, "Checking if User is authenticated...", GREY);
    draw_surface(screen, image, 0, 0);
    draw_surface(screen, checkingAuthText, (appWidth - checkingAuthText->w) / 2, (70 * appHeight) / 100);
    
    if (authStatus == "not_found")
    {
        debugPrintf("  User is not authenticated (userType 1), starting Authentication...\n");
        registerDevice();
    }
    else
    {
        userType = "two";
        debugPrintf("  Possible Token Read from file (userType 2):  \n\t%s \n   device_code: %s\n   scope: %s\n"
            , authStatus.c_str(), API_CONF.deviceId.c_str(), API_CONF.api_scope.c_str()
        );

        authToken->setRefreshToken(authStatus);
        authToken->setDeviceCode(API_CONF.deviceId);
        authToken->setScope(API_CONF.api_scope);      
        if (authToken->Refresh() != "valid")
        {
            debugPrintf("  Token Read from file was invalid, need to Re-Authenticate\n");
            registerDevice();
        }
    }
    
    storeConfig();
    debugPrintf("< Authentication Phase Complete, checking remote root folder existence\n");
    
    syncing = 1;
    
    SDL_FreeSurface(checkingAuthText);
    
    SDL_Surface *userAuthedText = NULL;
    userAuthedText = TTF_RenderText_Blended( Sans, "Syncing... Please, DO NOT Quit or Shutdown the PS3!", GREY);
    draw_surface(screen, image, 0,0);
    draw_surface(screen, userAuthedText, (appWidth - userAuthedText->w) / 2, (70 * appHeight) / 100);
    
    msgDialogOpen2(dialog_type, "Performing Sync...", dialog_handler, NULL, NULL);
    
    //If user new but remote data exists
    Json rootResource = api->checkIfRemoteResourceExists(APP_TITLE,DIR_MIME,"root");
    if(rootResource.Has("id"))
    {
        API_CONF.rootFolderId = rootResource["id"].Str();
        debugPrintf("User has previous data on Google Drive (userType 3) with id %s\n",API_CONF.rootFolderId.c_str());
        if(userType == "one") userType = "three";
    }
    else
    {
        debugPrintf("User has no previous data on Google Drive\n");
        Json rtId;
        rtId = api->uploadDirectory(APP_TITLE,"root");
        
        if(rtId.Has("id"))
        {
            API_CONF.rootFolderId = rtId["id"].Str();
            debugPrintf("Created root directory on Google Drive with id %s\n",API_CONF.rootFolderId.c_str());
            storeConfig();
        }
        else
        {
            debugPrintf("Error creating root folder on Google Drive\n");
        }
    }
    
    buildLocalResourceTree();
    debugPrintf("Continuing (userType=%s)....\n", userType.c_str());
    //NOTE: LRT = Local Resource Tree and RRT= Remote Resource Tree
    
    //First Time: App never run AND no data on Google Drive
    //DONE: Build Local Resource Tree and write to local.json
    //DONE: Copy Local Resource Tree to Remote Resource Tree
    //DONE: Write remote.json 
    //DONE: Upload all entries in remote.json to Google Drive
    //DONE: Update remote.json with all changes during uploading
    //DONE: Write remote.json to hard drive during changes
    //DONE: Upload remote.json to Google Drive
    if(userType == "one")
    {
        StdioFile file(LOCAL_RESOURCE_CONFIG_FILENAME);
        *remoteResourceRoot = Json::ParseFile(file);
        file.Close();

        StdioFile file2(REMOTE_RESOURCE_CONFIG_FILENAME);
        remoteResourceRoot->WriteFile(file2);
        file.Close();

        uploadChanges();
    }

    //Second Time: App ran before AND data possibly exists on Google Drive
    //DONE: Build Local Resource Tree and write to local.json
    //DONE: Read Remote Resource Tree
    //TODO: Go through all entries in LRT and search for them in RRT
    //TODO: If any entry does not exist in RRT, insert it into RRT
    //TODO: If any entry exists in RRT, match size, if size different mark as dirty
    //TODO: Upload all changes marked as dirty in RRT to Google Drive
    //TODO: Write all changes or uploads back to RRT and save to HDD
    //TODO: Write RRT to HDD
    //TODO: Upload RRT to Google Drive
    if(userType == "two")
    {
        debugPrintf("Reading local remote.json config.\n");
        StdioFile file(REMOTE_RESOURCE_CONFIG_FILENAME);
        if(file.Exists() == true){		
            *remoteResourceRoot = Json::ParseFile(file);
            file.Close();
            
            detectChangesForSync();
            
            uploadChanges();
            api->downloadChanges(&dialog_action);
        }else{
            debugPrintf("File %s doesn't exist\n", file.filepath().c_str());
            userType = "three";
        }
    }

    //First Time: App is installed on a new system but Data exists on Google Drive
    //TODO: Build LRT and write to local.json
    //TODO: Download Remote Resource Tree and write to remote.json
    //TODO: Revert to Second Time scenario
    if(userType == "three")
    {
        debugPrintf("Checking for remote.json (userType=3) on Google Drive with root_id %s\n",API_CONF.rootFolderId.c_str());
        Json remoteJson = api->checkIfRemoteResourceExists("remote.json",JSON_MIME,API_CONF.rootFolderId);
        
        if(remoteJson.Has("id"))
        {
            API_CONF.remoteJsonId = remoteJson["id"].Str();
            debugPrintf("Downloading remote.json from Google Drive with id %s\n",API_CONF.remoteJsonId.c_str());
            api->downloadFile(REMOTE_RESOURCE_CONFIG_FILENAME, API_CONF.remoteJsonId);
            
            StdioFile file(REMOTE_RESOURCE_CONFIG_FILENAME);
            *remoteResourceRoot = Json::ParseFile(file);
            file.Close();
            
            api->downloadChanges(&dialog_action);
            debugPrintf("Data from Google Drive downloaded, Please restart app to upload changes\n");
        }
        else
        {    
            debugPrintf("Wow, there's no remote.json on Google Drive!\n");
            buildRemoteResourceTree();
            api->downloadChanges(&dialog_action);
        }
    } 

    syncing = 0;
    
    msgDialogAbort();
    
    SDL_FreeSurface(userAuthedText);
    
    exitapp = 1;
    
    return 0;
}
