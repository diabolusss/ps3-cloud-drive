#pragma once

#include <string>
#include <sysutil/msg.h> //msgDialogProgress...

#include "Json.h"
#include "Download.h"
#include "Header.h"

#include "CurlAgent.h"
#include "OAuth2.h"

class APIInterface {
   public:         
       double m_prog_func_last_progress;

       const std::string *m_api_key_map;
       const std::string m_client_id;
       const std::string m_client_secret;
       const std::string m_token_url;
       const std::string m_device_url;
       OAuth2* m_auth_token;
       CurlAgent http;

       Json m_remote_resource_root;

        APIInterface(
                std::string _client_id, 
                std::string _client_secret,
                std::string _token_url, 
                std::string _device_url
        ):

                m_client_id(_client_id),
                m_client_secret(_client_secret),
                m_token_url(_token_url),
                m_device_url(_device_url)
        {}

        void init(){
                m_auth_token = new OAuth2(m_token_url, m_device_url, m_client_id, m_client_secret, m_api_key_map);
        }

        bool ShouldRetryCheck( long response );
        //TODO *clientp - possible to pass api or just pointer to double status; @see https://github.com/curl/curl/issues/806 
        //int transferProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

        //api specific methods
                virtual Json checkIfRemoteResourceExists(std::string resourceTitle,std::string mimeType,std::string parentId) = 0;
                virtual Json::Array getResourcesUnderFolder(std::string parentId) = 0;
                virtual bool downloadFile(std::string localPath, std::string fileId) = 0;
                virtual void downloadChanges(int *dialog_action) = 0;
                virtual Json uploadFile(std::string path, std::string filename, std::string mimeType, std::string parentId, std::string fileId = "") = 0;
                virtual Json uploadDirectory(std::string dirName, std::string parentId) = 0;
};