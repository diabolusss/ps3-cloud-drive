#pragma once

#include <iostream>
#include <SDL/SDL.h>

std::string calculateMD5Checksum(std::string fpath);
void createDownloadDir(std::string path);

std::string constructMetaData(std::string title, std::string mimeType, std::string parentid);

void draw_surface(SDL_Surface* destination, SDL_Surface* source, int x, int y);
SDL_Surface *Load_Image(std::string filePath);

bool pathExists(std::string pathToCheck);