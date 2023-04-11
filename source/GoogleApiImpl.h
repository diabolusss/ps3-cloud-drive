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
class GoogleApiImpl: public APIInterface {
   public:
        GoogleApiImpl() : 
            APIInterface(
                "https://www.googleapis.com/",
                GOOGLE_API_ID, 
                GOOGLE_API_KEY,
                "https://www.googleapis.com/oauth2/v4/token",
                "https://accounts.google.com/o/oauth2/device/code"
            )
        {
            /* scope
                http.Escape("email profile ") +*/ 
            // http.Escape("https://www.googleapis.com/auth/userinfo.email") + http.Escape(" ") +
            // http.Escape("https://docs.google.com/feeds") + http.Escape(" ") + // Deprecated, but still working
            // http.Escape("https://www.googleapis.com/auth/userinfo.profile");*/
            m_api_key_map = {
                    /* DEVICE_CODE_KEY */           "code",
                    /* GRANT_TYPE_VALUE */          "http://oauth.net/grant_type/device/1.0",
                    /* DEVICE_GRANT_SCOPE_VALUE */  "https://www.googleapis.com/auth/drive.file",
                    /* DEVICE_CODE_USER_URL */      "verification_url"
            };
            
            init();
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
