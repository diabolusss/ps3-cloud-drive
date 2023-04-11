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
#include <string>
#include "Json.h"
#include "Header.h"

#include "ApiInterface.h"
#include "APIHelper.h"

#include "credentials.h"
#include "log.h"
class KoofrApiImpl: public APIInterface {
   public:
        KoofrApiImpl() : 
            APIInterface(
                "https://app.koofr.net/api/v2.1/",
                KOOFR_API_ID, 
                KOOFR_API_KEY,
                "https://app.koofr.net/oauth2/token",
                "https://app.koofr.net/oauth2/devicecode"
            )
        {
           // const std::string _map[E_NUM] = {
             //       /* DEVICE_CODE_KEY */           //"device_code",
               //     "device_code",
                //    /* GRANT_TYPE_VALUE */          //"urn:ietf:params:oauth:grant-type:device_code",
                 //   "test",
                  //  /* DEVICE_GRANT_SCOPE_VALUE */  "public"
           // };
            m_api_key_map = {
                    /* DEVICE_CODE_KEY */           "device_code",
                    /* GRANT_TYPE_VALUE */          "urn:ietf:params:oauth:grant-type:device_code",
                    /* DEVICE_GRANT_SCOPE_VALUE */  "public",
                    /* DEVICE_CODE_USER_URL */      "verification_uri"
            };
            //debugPrintf("  init.%d\n", E_NUM);
            //debugPrintf("  init[%d].%s\n", GRANT_TYPE_VALUE, _map[GRANT_TYPE_VALUE].c_str());
            //debugPrintf("  init[%d].%s\n", DEVICE_CODE_KEY, _map[DEVICE_CODE_KEY].c_str());
            //debugPrintf("  init[%d].%s\n", DEVICE_GRANT_SCOPE_VALUE, _map[DEVICE_GRANT_SCOPE_VALUE].c_str());
            //m_api_key_map = _map;

            /*debugPrintf("  init[%d].%s\n", GRANT_TYPE_VALUE, m_api_key_map[GRANT_TYPE_VALUE].c_str());
            debugPrintf("  init[%d].%s\n", DEVICE_CODE_KEY, m_api_key_map[DEVICE_CODE_KEY].c_str());
            debugPrintf("  init[%d].%s\n", DEVICE_GRANT_SCOPE_VALUE, m_api_key_map[DEVICE_GRANT_SCOPE_VALUE].c_str());
            */init();
            //m_auth_token->vec2 = &vec;
        }

        //derived virtual methods for implementation
        Json checkIfRemoteResourceExists(std::string resourceTitle,std::string mimeType,std::string parentId);
        Json::Array getResourcesUnderFolder(std::string parentId);
        bool downloadFile(std::string localPath, std::string fileId);
        void downloadChanges(int *dialog_action);
        Json uploadFile(std::string path, std::string filename, std::string mimeType, std::string parentId, std::string fileId = "");
        Json uploadDirectory(std::string dirName, std::string parentId);

    private:
        const std::string queryFields   = "items(fileSize,id,md5Checksum,mimeType,createdDate,modifiedDate,parents/id,title)";
    
};
