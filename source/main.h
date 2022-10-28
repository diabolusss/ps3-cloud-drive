#pragma once

#include <lv2/syscalls.h>
#include <sstream>
#include <sys/systime.h> //sysUsleep

#include "Json.h"
#include "OAuth2.h"

extern Json remoteResourceRoot;

extern const std::string JSON_MIME;
extern const std::string DIR_MIME;
extern const std::string OCTET_MIME;

extern double progFuncLastProgress;

    void downloadDirectory(std::string path);
    int transferProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    std::string constructMetaData(std::string title, std::string mimeType, std::string parentid);