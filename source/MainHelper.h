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

#include <iostream>
#include <SDL/SDL.h>

std::string epochTsToString(time_t *ts);
std::string calculateMD5Checksum(std::string fpath);
void createDownloadDir(std::string path);

std::string constructMetaData(std::string title, std::string mimeType, std::string parentid);

void draw_surface(SDL_Surface* destination, SDL_Surface* source, int x, int y);
SDL_Surface *Load_Image(std::string filePath);

bool pathExists(std::string pathToCheck);