#pragma once

#include <lv2/syscalls.h>
#include <sstream>

/**
 * keep track of application sync mode: 
*    (1) first_time - new device and cloud storage have no APP folder
*                      configure and sync
*    (3) new_device - new device, but cloud storage have APP folder
*                      configure and sync
*    (2) configured - device was already configured and synced to cloud storage
*                 check synchronization status and re-sync
*/
enum SYNC_DEVICE_MODE{
    MODE_FIRST_TIME = 1,
    MODE_CONFIGURED = 2,
    MODE_NEW_DEVICE = 3,
};

const std::string RESOURCE_REMOTE_STATUS_SYNCED_KEY     = "synced";
const std::string RESOURCE_REMOTE_STATUS_UPLOAD_KEY     = "upload";
const std::string RESOURCE_REMOTE_STATUS_UPDATE_KEY     = "update";

extern const std::string JSON_MIME;
extern const std::string DIR_MIME;
extern const std::string OCTET_MIME;

    void createDownloadDir(std::string path);
    int transferProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    std::string constructMetaData(std::string title, std::string mimeType, std::string parentid);