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

#pragma once

#include <string>
#include <vector>
class OAuth2
{
	public :
		std::string m_verification_url; //each api returns own url - use that
		std::string m_scope;	//each authorization returns granted scope

		OAuth2();

		OAuth2(
               const std::string& api_token_url,
               const std::string& api_device_url,
				const std::string&	client_id,
				const std::string&	client_secret,
				const std::vector<std::string>  *api_key_map
			) ;

		std::string Str() const ;

		std::string Auth() ;
		std::string DeviceAuth();
		std::string Refresh( ) ;
		
        void setRefreshToken(std::string token);
        void setDeviceCode(std::string code);
        void setScope(std::string scope);
		std::string RefreshToken( ) const ;
		std::string AccessToken( ) const ;
		std::string DeviceCode( ) const;
		std::string getDeviceVerificationUrl( ) const;
		std::string getScope( ) const;
	
		// adding HTTP auth header
		std::string HttpHeaderBearer( ) const ;
		std::string HttpHeaderBasic( ) const ;
	
	private :
		std::string m_access ;
		std::string m_refresh ;
		std::string m_device;

		//api specific constants
			const std::string m_api_token_url; 			//api url to retrieve/refresh token
			const std::string m_api_device_code_url;  	//api url for limited devices auth
		
			const std::string	m_client_id ;
			const std::string	m_client_secret ;
			const std::string   m_client_auth_basic;
	
			const std::vector<std::string> *m_api_key_map; //map of api specific values needed for oauth
} ;
