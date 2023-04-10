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
