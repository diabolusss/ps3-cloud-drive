#pragma once
#define M_ToConstUCharPtr(p)  reinterpret_cast<const unsigned char*>(p)    // Cast to const unsigned char*

std::string base64_encode(const unsigned char *src, size_t len);