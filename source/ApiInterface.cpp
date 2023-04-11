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

#include <sys/systime.h> //sysUsleep
#include "ApiInterface.h"    
#include "log.h"

bool APIInterface::ShouldRetryCheck( long response )
{
    // HTTP 500 and 503 should be temperory. just wait a bit and retry
    if ( response == 500 || response == 503 )
    {
        debugPrintf("Request failed due to temporary error: %d, retrying in 5 seconds\n",response);
            
        sysUsleep(5);
        return true ;
    }
    
    // HTTP 401 Unauthorized. the auth token has been expired. refresh it
    else if ( response == 401 )
    {
        debugPrintf("Request failed due to Auth token expired: %d. refreshing token\n",response);
            
        m_auth_token->Refresh() ;
        return true ;
    }
    else
    {
        debugPrintf("OAuth2 session ok, continue... %d\n",response);
        return false ;
    }
}

//TODO access m_prog_func_last_progress from *clientp
/*int APIInterface::transferProgress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    if(dltotal > 0.0)
    {
        double curProg = (dlnow / dltotal) * 100;
        if(curProg - m_prog_func_last_progress > 1)
        {
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, curProg - m_prog_func_last_progress);
            
            debugPrintf("DOWN: %f of %f P: %f C: %f\r\n", dlnow, dltotal, m_prog_func_last_progress, curProg);
            
            m_prog_func_last_progress = curProg;
        }
    }
    
    if(ultotal > 0.0)
    {
        double curProg = (ulnow / ultotal) * 100;
        if(curProg - m_prog_func_last_progress > 1)
        {
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, curProg - m_prog_func_last_progress);
            
            debugPrintf("UP: %f of %f P: %f C: %f\r\n", ulnow, ultotal, m_prog_func_last_progress,curProg);
                    
            m_prog_func_last_progress = curProg;
        }
    }
    
    return 0;
}*/
