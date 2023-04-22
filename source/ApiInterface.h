/*
# Playstation 3 Cloud Drive
# Copyright (C) 2013-2014   Mohammad Haseeb aka MHAQS
# Copyright (C) 2023        Vitaly Hodiko aka vitaly.x
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
#pragma once

#include <string>
#include <sysutil/msg.h> //msgDialogProgress...

#include "Json.h"
#include "Download.h"
#include "Header.h"

#include "CurlAgent.h"
#include "OAuth2.h"

#include <vector>

class APIInterface {
   public:         
       double m_prog_func_last_progress;

       std::vector<std::string> m_api_key_map;
         
       const std::string m_api_root_url;
       const std::string m_client_id;
       const std::string m_client_secret;
       const std::string m_token_url;
       const std::string m_device_url;
       OAuth2* m_auth_token;
       CurlAgent http;

       Json m_remote_resource_root;

        APIInterface(
                std::string _api_url, 
                std::string _client_id, 
                std::string _client_secret,
                std::string _token_url, 
                std::string _device_url
        ):
                m_api_root_url(_api_url),
                m_client_id(_client_id),
                m_client_secret(_client_secret),
                m_token_url(_token_url),
                m_device_url(_device_url)
        {}

        void init() {
                m_auth_token = new OAuth2(m_token_url, m_device_url, m_client_id, m_client_secret, &m_api_key_map);
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

                virtual Json getRootMountId() = 0;
                //virtual std::string search() = 0;
};