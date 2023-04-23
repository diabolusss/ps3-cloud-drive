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
#include "APIHelper.h" //enum OAUTH_API_PARAM_KEYS

// for debugging
#include <iostream>

OAuth2::OAuth2(){};

OAuth2::OAuth2(
               const std::string& api_token_url,
               const std::string& api_device_url,
               const std::string& client_id,
               const std::string& client_secret,
               const std::vector<std::string> *api_key_map
            ) :
    m_api_token_url(api_token_url),
    m_api_device_code_url(api_device_url),
    m_client_id(client_id),
    m_client_secret(client_secret),
    m_client_auth_basic(base64_encode(M_ToConstUCharPtr((client_id+":"+client_secret).c_str()), client_id.length()+client_secret.length()+1)),
    m_api_key_map(api_key_map)
{
}

/**
 * 1. Request a Device Code
 *  (And 2. Tell the User to Enter the user_code into form field from verification_url)
 *  
 * next @see Auth()
 * 
 * Example: 
 * {
 *   "device_code": "some looong string",
 *   "user_code": "RJX-PSB-XTX",
 *   "expires_in": 1800,
 *   "interval": 5,
 *   "verification_url": "https://service.domain/device"
 * }
 */
std::string OAuth2::DeviceAuth()
{
    JsonResponse resp;
    CurlAgent http;

    std::string usercode;
    std::string post =
			"client_id=" + m_client_id +
			"&scope=" + (*m_api_key_map)[DEVICE_GRANT_SCOPE_VALUE]
    ;

    http.Post(m_api_device_code_url, post, &resp, Header());

    Json root = resp.Response();

    if(root.Has("user_code"))
    {
        m_device = root["device_code"].Str();
        usercode = root["user_code"].Str();
        m_verification_url = root[(*m_api_key_map)[DEVICE_CODE_USER_URL]].Str();
    }
    else
    {
        usercode = "invalid";
    }
    
    return usercode;
}

/**
 * (3. Poll the Token Endpoint)
 * When the user approves the request, the token endpoint will respond with the access token.
 * 
 * Koofr:
 *  {
 *      "token_type": "Bearer",
 *      "access_token": "2XTP7NNLRNMMLT52QQVF*******",
 *      "expires_in": 3600,
 *      "refresh_token": "NS6BXWDVTZ7XIK*************",
 *      "scope": "public"
 *  }
 * 
 */
std::string OAuth2::Auth()
{
    std::string post = "&"
        + (*m_api_key_map)[DEVICE_CODE_KEY]+"=" + m_device +
        + "&grant_type=" +(*m_api_key_map)[GRANT_TYPE_VALUE]
    ;

    JsonResponse resp;
    CurlAgent http;

    http.Post(m_api_token_url, post, &resp, (Header()+HttpHeaderBasic()));

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
 * Refresh access token if available
 * 
 * @return invalid or valid
*/
std::string OAuth2::Refresh()
{
    std::string post =
            "refresh_token=" + m_refresh
            + "&grant_type="+"refresh_token"
    ;

    JsonResponse resp;
    CurlAgent http;
    std::string status = "invalid";

    http.Post(m_api_token_url, post, &resp, Header()+HttpHeaderBasic());
    
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
