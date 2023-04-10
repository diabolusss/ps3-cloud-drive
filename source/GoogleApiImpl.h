
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
                GOOGLE_API_ID, 
                GOOGLE_API_KEY,
                "https://www.googleapis.com/oauth2/v4/token",
                "https://accounts.google.com/o/oauth2/device/code"
            )
        {
            const std::string _map[E_NUM] = {
                    /* DEVICE_CODE_KEY */ "code",
                    /* GRANT_TYPE_VALUE */ "http://oauth.net/grant_type/device/1.0",
            };
            m_api_key_map = _map;
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
