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
#include "GoogleApiImpl.h"
#include "log.h"
#include "JsonResponse.h"

//TODO FIX - intead pass a pointer to necessary data
#include "main.h" //transferProgress 

/**
 * The function checks if a resource exists on google drive and returns the item
 * NB It depends on Google APP permissions what will be found
 *  - scope '.../auth/drive.file' - only this app specific resources, ie. manually created folders and files will be hidden
 *  - scope '.../auth/drive' - unrestricted access to drive 
 * NB For unrelated data upload we are forced to use wider scope
 * 
 * @param resourceTitle the title of the resource to find
 * @param mimeType the mimetype of the resource
 * @param parentId the parentid of the resource's parent (if you don't know this, this function is useless, unless you're searching under root)
 * @return the resource object, otherwise null
 */
Json GoogleApiImpl::checkIfRemoteResourceExists(std::string resourceTitle,std::string mimeType,std::string parentId)
{
    long ret;
    Json resourceObject;
    
    Header hdr;
    hdr.Add("Authorization: Bearer " + m_auth_token->AccessToken());
    
    std::string query="mimeType='"+mimeType+"' and trashed=false and title='"+resourceTitle+"' and '"+parentId+"' in parents"; 
    JsonResponse resp;
    
    std::string url = "https://www.googleapis.com/drive/v2/files?q=" + http.Escape(query) + "&fields=" + http.Escape(queryFields)+"&orderBy=createdDate";
    
    while( ShouldRetryCheck( ret = http.Get(url, &resp, hdr)));
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("checkIfRemoteResourceExists: HTTP Server Error...%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
    }
    else
    {
        Json parsedResp = resp.Response();
        Json::Array parsedArray = parsedResp["items"].AsArray();

        debugPrintf("checkIfRemoteResourceExists: found %d entries\n", parsedArray.size());
        if(parsedArray.size() > 0)
        {
            for(uint8_t i = 0; i<parsedArray.size(); i++){
                debugPrintf("checkIfRemoteResourceExists: parsedArray[%d] = %s\n", i, parsedArray.at(i).Str().c_str());
            }
            resourceObject = parsedArray.at(0);
        }
    }
    
    return resourceObject;
}

/**
 * Retrieves a Resource tree of the resources under a folder in google drive
 * @param parentId the parentid for whomm the resources are to be listed
 * @return the resource array containing all children
 */
Json::Array GoogleApiImpl::getResourcesUnderFolder(std::string parentId)
{
    long ret;
    Json::Array parsedArray;
    
    Header hdr;
    hdr.Add("Authorization: Bearer " + m_auth_token->AccessToken());
    
    std::string query=parentId+"' in parents"; 
    JsonResponse resp;

    while( ShouldRetryCheck( ret = http.Get("https://www.googleapis.com/drive/v2/files?q="+query, &resp, hdr)));
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("getResourcesUnderFolder: HTTP Server Error...%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
    }
    else
    {
        Json parsedResp = resp.Response();
        parsedArray = parsedResp["items"].AsArray();
    }
    return parsedArray;
}

/**
 * Downloads a file from Google drive to local folder
 * @param localPath the local path to save the file to
 * @param fileId the id of the file in google drive to download
 */
bool GoogleApiImpl::downloadFile(std::string localPath, std::string fileId)
{
    long ret;
    Download file(localPath);
    
    Header hdr;
    hdr.Add("Authorization: Bearer " + m_auth_token->AccessToken());

    JsonResponse resp;

    while( ShouldRetryCheck( ret = http.Get("https://www.googleapis.com/drive/v2/files/"+fileId + "?fields=downloadUrl" , &resp, hdr)));
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("downloadFile: Could not get downloadUrl...%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
        return false;
    }

    // the content upload URL is in the "Location" HTTP header
    Json fileRes = resp.Response();
    std::string downloadLink;
    
    if(fileRes.Has("downloadUrl"))
    {
        downloadLink = fileRes["downloadUrl"].Str();
    }
    else
    {
        return false;
    }
    resp.Clear();

    debugPrintf("Downlink is %s\n", downloadLink.c_str());

    m_prog_func_last_progress = 0.0;
    
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, localPath.c_str());
    
    while( ShouldRetryCheck( ret = http.customGet(downloadLink, &file, transferProgress, hdr)));
    
    debugPrintf("Return Code for file download is %d\n", ret);
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("downloadFile: Could not download File..%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
        return false;
    }
    
    return true;
}

void GoogleApiImpl::downloadChanges(int *dialog_action)
{
    u32 remSize = m_remote_resource_root["data"].AsArray().size();
    u32 i=0;
    u32 progressCounter =0;
    u32 totalProgress = 0;
    u32 prevProgress = 0;
    //u32 globProgress = 0;
            
    std::string result;
    
    for(i=0; i < remSize; i++)
    {
        Json resource = m_remote_resource_root["data"][i];
        if(resource["status"].Str() == "synced")
        {
            sysFSStat entry;
                
            if(sysFsStat(resource["path"].Str().c_str(), &entry) != 0)
            {
                totalProgress++;
            }
        }
    }
    
    debugPrintf("downloadChanges Step 1: Creating directories first on HDD\n");
    
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Downloading Parent Entries...");
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, 0);
    
    for(i=0; i < remSize; i++)
    {
        if(*dialog_action != 0) break;
        
        Json resource = m_remote_resource_root["data"][i];
        
        if(resource["status"].Str() == "synced")
        {
            if(resource["mimeType"].Str() == DIR_MIME)
            {
                sysFSStat entry;
                result = resource["path"].Str();
                
                if(sysFsStat(result.c_str(), &entry) != 0)
                {
                    progressCounter++;
                    debugPrintf("downloadChanges: Creating Directory with path %s\n", result.c_str());
                    
                    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, 0);
                    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, resource["path"].Str().c_str());

                    createDownloadDir(result);
                    
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1,100);
                    
                    std::stringstream ss;
                    ss<<"Downloading Parent Entries: "<<progressCounter<<" of "<<totalProgress;
                        
                    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0,ss.str().c_str());
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    
                    /*if(userType == "three")
                    {
                        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    }
                    if(userType == "two")
                    {   
                        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 40) / totalProgress) - globProgress);                      
                    }
                    globProgress = (progressCounter * 40) / totalProgress;*/
                    prevProgress = (progressCounter * 100) / totalProgress;
                }
            }
        }
    }

    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Downloading Child Entries...");
    
    debugPrintf("downloadChanges Step 2: Creating files on HDD (dia_action %d)\n", *dialog_action);
        
    for(i=0; i < remSize; i++)
    {
        if(*dialog_action != 0) break;
        
        Json resource = m_remote_resource_root["data"][i];
        
        if(resource["status"].Str() == "synced")
        {
            if(resource["mimeType"].Str() != DIR_MIME)
            {
                sysFSStat entry;
                result = resource["path"].Str();
            
                if(sysFsStat(result.c_str(), &entry) != 0)
                {
                    progressCounter++;
                    debugPrintf("downloadChanges: Creating file with path %s\n", result.c_str() );
                    
                    if(!downloadFile(result,resource["id"].Str()))
                    {
                        continue;
                    }

                    std::stringstream ss;
                    ss<<"Downloading Child Entries: "<<progressCounter<<" of "<<totalProgress;
                        
                    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0,ss.str().c_str());
                    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    
                    /*if(userType == "three")
                    {
                        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 100) / totalProgress) - prevProgress);
                    }
                    if(userType == "two")
                    {   
                        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, ((progressCounter * 40) / totalProgress) - globProgress);                      
                    }
                    globProgress = (progressCounter * 40) / totalProgress;*/
                    prevProgress = (progressCounter * 100) / totalProgress;
                }
            }
        }
    }
    
    debugPrintf("downloadChanges finished, All synced files Downloaded\n");
}

/**
 * Uploads a file to Google Drive
 * @param path the local file path
 * @param filename the name of the file with extension
 * @param mimeType the mimetype of the file, default is octet
 * @param parentId the parent id in drive of the file
 * @return the created resource in Json format
 */
Json GoogleApiImpl::uploadFile(std::string path, std::string filename, std::string mimeType, std::string parentId, std::string fileId)
{
    long ret;
    StdioFile file(path);
    Json obj;
    sysFSStat entry;
    
    if(sysFsStat(path.c_str(), &entry) != 0)
    {
        debugPrintf("uploadFile: Could not open file, another ps3?\n");
        return obj;
    }

    std::ostringstream xcontent_len;
    xcontent_len << "X-Upload-Content-Length: " << file.Size();

    Header hdr;
    hdr.Add("Authorization: Bearer " + m_auth_token->AccessToken());
    hdr.Add("Content-Type: application/json");
    hdr.Add("X-Upload-Content-Type: " + mimeType);
    hdr.Add(xcontent_len.str());
    //hdr.Add( "If-Match: " + m_etag ) ;
    hdr.Add( "Expect:" ) ;

    std::string meta = constructMetaData(filename, mimeType, parentId);

    JsonResponse resp;
    
    std::string postUrl;
    
    if(fileId == "")
    {
        postUrl = "https://www.googleapis.com/upload/drive/v2/files?uploadType=resumable&fields=" + http.Escape("fileSize,id,md5Checksum,modifiedDate,parents/id");
        while( ShouldRetryCheck( ret = http.Post(postUrl, meta, &resp, hdr)));
    }
    else
    {
        postUrl = "https://www.googleapis.com/upload/drive/v2/files/" + fileId+ "?uploadType=resumable&fields=" + http.Escape("fileSize,id,md5Checksum,modifiedDate,parents/id");
        while( ShouldRetryCheck( ret = http.Put(postUrl, meta, &resp, hdr)));
    }
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("uploadFile: Could not pose meta data..%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
        return obj;
    }

    std::ostringstream content_len;
    content_len << "Content-Length: " << file.Size();
    
    Header uphdr;
    uphdr.Add("Authorization: Bearer " + m_auth_token->AccessToken());
    uphdr.Add("Content-Type: " + mimeType);
    uphdr.Add(content_len.str());
    uphdr.Add("Expect:" );
    uphdr.Add("Accept:" );
    uphdr.Add("Accept-Encoding: gzip");

    // the content upload URL is in the "Location" HTTP header
    JsonResponse resp2;

    m_prog_func_last_progress = 0.0;
    
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, path.c_str());

    while( ShouldRetryCheck( ret = http.customPut(http.RedirLocation(), file, &resp2, transferProgress, uphdr)));
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("uploadFile: Could not upload file..%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
        return obj;
    }

    obj = resp2.Response();
    if (obj.Has("id"))
    {
        debugPrintf("Resource id is %s\n", obj["id"].Str().c_str());
    }
    else
    {
        debugPrintf("Resource id not found");
    }
    
    return obj;
}

/**
 * Creates a directory on the google drive
 * 
 * API Response:
 * {
    "parents": [
        {
            "id": "0AIUOHjtY_mQkUk9PVA"
        }
    ],
    "id": "1Bwn-72gMkpdPvE4EMSBdu_Z6YEnGAjq_",
    "modifiedDate": "2023-04-16T20:06:11.926Z"
 * }
 *
 * {
    "error": {
        "code": 404,
        "message": "File not found: ROOT",
        "errors": [
        {
            "message": "File not found: ROOT",
            "domain": "global",
            "reason": "notFound",
            "location": "file",
            "locationType": "other"
        }
        ]
    }
 * }
 * @param dirName the name to give the directory
 * @param parentId the parentid of this directory, default is root
 * @return the created folder resource in Json format
 */
Json GoogleApiImpl::uploadDirectory(std::string dirName, std::string parentId)
{
    long ret = -1;
    Json obj;

    //Step 1 - Creating SaveData Directory Online
    Header hdr;
    hdr.Add("Authorization: Bearer " + m_auth_token->AccessToken());
    hdr.Add("Content-Type: application/json");
    hdr.Add("Expect:" );

    std::string meta = constructMetaData(dirName, DIR_MIME, parentId);

    JsonResponse resp;

    debugPrintf("Attempting folder creation on GDrive...\n");
        
    while( ShouldRetryCheck( ret = http.Post("https://www.googleapis.com/drive/v2/files?fields=" + http.Escape("id,modifiedDate,parents/id"), meta, &resp, hdr)));
    
    if ( ret >= 400 && ret < 500 )
    {
        debugPrintf("uploadDirectory: Could not upload directory..%l\n", ret);
        // This could mean the internet disconnected, or the server crashed or whatever
        // Possibly exit the application?
        return obj;
    }
    
    obj = resp.Response();
    if (obj.Has("id"))
    {
        debugPrintf("Folder id is %s\n", obj["id"].Str().c_str());
    }
    else
    {
        debugPrintf("Folder creation failed with status %d\n", ret);
    }

    return obj;
}
