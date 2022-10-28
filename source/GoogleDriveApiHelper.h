#pragma once

#include <string>
#include "Json.h"
#include "Header.h"

bool ShouldRetryCheck( long response );

Json checkIfRemoteResourceExists(std::string resourceTitle,std::string mimeType,std::string parentId);
Json::Array getResourcesUnderFolder(std::string parentId);
bool downloadFile(std::string localPath, std::string fileId);
void downloadChanges(int *dialog_action);
Json uploadFile(std::string path, std::string filename, std::string mimeType, std::string parentId, std::string fileId = "");
Json uploadDirectory(std::string dirName, std::string parentId);