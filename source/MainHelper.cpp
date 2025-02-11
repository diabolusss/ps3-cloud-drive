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
#include "MainHelper.h"
#include "Json.h"

#include <polarssl/md5.h>
#include <SDL/SDL_image.h>
#include <sys/systime.h> //sysUsleep

std::string epochTsToString(time_t *ts){
    char dt[28];
    strftime(dt, 28, "%FT%TZ%z", gmtime(ts));

    return dt;
}

std::string calculateMD5Checksum(std::string fpath)
{
    unsigned char md5Out[16];
    char convBuf[64];

    if (md5_file(fpath.c_str(),md5Out) == 0)
    {
        for(int k = 0; k < 16; k++)
        {
                snprintf(convBuf, 64, "%s%02x", convBuf, md5Out[k]);
        }
        //std::string res = convBuf;
        return convBuf;
    }
    return "nill";
}

/**
 * Creates a directory locally
 * @param path the path of the directory to create
 */
void createDownloadDir(std::string path)
{    
    sysFsMkdir(path.c_str(),0755);
}

/**
 * Utility function to create meta data when a resource needs to be created on google drive
 * @param title the title of the resource
 * @param mimeType the mimetype. folder, octet etc
 * @param parentid the parentid of the resource, default is root
 * @return the json metadata in string notation
 */
std::string constructMetaData(std::string title, std::string mimeType, std::string parentid)
{
    Json parent;
    parent.Add("id", Json(parentid));

    std::vector<Json> array;
    array.push_back(parent);

    Json meta;
    meta.Add("title", Json(title));
    meta.Add("parents", Json(array));
    meta.Add("mimeType", Json(mimeType));

    return meta.Str();
}

bool pathExists(std::string pathToCheck)
{
    sysFSStat entry;
    return sysFsStat(pathToCheck.c_str(), &entry) == 0;
}

SDL_Surface *Load_Image(std::string filePath)
{
	SDL_Surface* loadedImage = NULL;
	SDL_Surface* optimizedImage = NULL;
	loadedImage = IMG_Load(filePath.c_str());
	if(loadedImage!=NULL){
		optimizedImage = SDL_DisplayFormatAlpha(loadedImage);
		SDL_FreeSurface(loadedImage);
	}
	return optimizedImage;
}

void draw_surface(SDL_Surface* destination, SDL_Surface* source, int x, int y)
{
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface( source, NULL, destination, &offset );
}
