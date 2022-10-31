#pragma once

#include <lv2/syscalls.h>
#include <sstream>
#include <sys/systime.h> //sysUsleep

#include "Json.h"
#include "OAuth2.h"

const std::string RESOURCE_REMOTE_STATUS_SYNCED_KEY     = "synced";
const std::string RESOURCE_REMOTE_STATUS_UPLOAD_KEY     = "upload";
const std::string RESOURCE_REMOTE_STATUS_UPDATE_KEY     = "update";

extern const std::string JSON_MIME;
extern const std::string DIR_MIME;
extern const std::string OCTET_MIME;

extern Json remoteResourceRoot;

extern double progFuncLastProgress;

    void createDownloadDir(std::string path);
    int transferProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    std::string constructMetaData(std::string title, std::string mimeType, std::string parentid);