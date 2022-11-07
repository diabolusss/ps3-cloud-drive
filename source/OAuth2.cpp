/*
# Playstation 3 Cloud Drive
# Copyright (C) 2013-2014 Mohammad Haseeb aka MHAQS
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

#include "OAuth2.h"
#include "CurlAgent.h"
#include "JsonResponse.h"
#include "Header.h"
#include "Base64.h" //instead of sending plain secret in url, use auth header

// for debugging
#include <iostream>

const std::string token_url = "https://www.googleapis.com/oauth2/v4/token";
const std::string device_url = "https://accounts.google.com/o/oauth2/device/code";

OAuth2::OAuth2(
               const std::string& refresh_code,
               const std::string& client_id,
               const std::string& client_secret) :
m_refresh(refresh_code),
m_client_id(client_id),
m_client_secret(client_secret)
{
    Refresh();
}

OAuth2::OAuth2(
               const std::string& client_id,
               const std::string& client_secret) :
m_client_id(client_id),
m_client_secret(client_secret),
m_client_auth_basic(base64_encode(M_ToConstUCharPtr((client_id+":"+client_secret).c_str()), client_id.length()+client_secret.length()+1))
{
}

/**
 * 
 * 
 */
std::string OAuth2::Auth()
{
    std::string post =
        "&code=" + m_device +
        "&grant_type=http://oauth.net/grant_type/device/1.0"
    ;

    JsonResponse resp;
    CurlAgent http;

    http.Post(token_url, post, &resp, (Header()+HttpHeaderBasic()));

    std::string authStatus = "not_found";
    Json root = resp.Response();

    if(root.Has("access_token"))
    {
        m_access    = root["access_token"].Str();
        m_refresh   = root["refresh_token"].Str();
        m_scope     = root["scope"].Str();

        authStatus = "authorization_complete";
    }
    
    return authStatus;
}

/**
 * 
 * Example: 
 * {
 *   "device_code": "some looong string",
 *   "user_code": "RJX-PSB-XTX",
 *   "expires_in": 1800,
 *   "interval": 5,
 *   "verification_url": "https://www.google.com/device"
 * }
 */
std::string OAuth2::DeviceAuth()
{
    JsonResponse resp;
    CurlAgent http;

    std::string usercode;

    std::string post =
			"client_id=" + m_client_id +
			"&scope=" + http.Escape("email profile ") + http.Escape("https://www.googleapis.com/auth/drive.file");
            // "scope=" +
            // http.Escape("https://www.googleapis.com/auth/userinfo.email") + http.Escape(" ") +
            // http.Escape("https://docs.google.com/feeds") + http.Escape(" ") + // Deprecated, but still working
            // http.Escape("https://www.googleapis.com/auth/userinfo.profile");
            

    http.Post(device_url, post, &resp, Header());

    Json root = resp.Response();

    if(root.Has("user_code"))
    {
        m_device = root["device_code"].Str();
        usercode = root["user_code"].Str();
        m_verification_url = root["verification_url"].Str();
    }
    else
    {
        usercode = "invalid";
    }
    
    return usercode;
}

std::string OAuth2::Refresh()
{
    std::string post =
            "refresh_token=" + m_refresh +
            "&grant_type=refresh_token";

    JsonResponse resp;
    CurlAgent http;
    std::string status = "invalid";

    http.Post(token_url, post, &resp, Header()+HttpHeaderBasic());
    
    Json root = resp.Response();

    if(root.Has("access_token"))
    {
        m_access    = root["access_token"].Str() ;
        m_scope     = root["scope"].Str() ;
        status      = "valid";
    }
    
    return status;
}

void OAuth2::setRefreshToken(std::string token)
{
    m_refresh = token;
}

void OAuth2::setDeviceCode(std::string code)
{
    m_device = code;
}


void OAuth2::setScope(std::string scope)
{
    m_scope = scope;
}

std::string OAuth2::RefreshToken() const
{
    return m_refresh;
}

std::string OAuth2::AccessToken() const
{
    return m_access;
}

std::string OAuth2::DeviceCode() const
{
    return m_device;
}

std::string OAuth2::getDeviceVerificationUrl() const{
    return m_verification_url;
}

std::string OAuth2::getScope() const{
    return m_scope;
}

std::string OAuth2::HttpHeaderBearer() const
{
    return "Authorization: Bearer " + m_access;
}

std::string OAuth2::HttpHeaderBasic() const
{
    return "Authorization: Basic " + m_client_auth_basic;
}

std::string OAuth2::HostHeader() const
{
    return "Host: accounts.google.com";
}