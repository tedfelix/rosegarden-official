/*
 *  ErrCode.h
 *  rgplayer
 *
 *  Created by Guillaume Laurent on 3/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef RG_ERR_CODE
#define RG_ERR_CODE

#include <AudioToolbox/AudioToolbox.h>

class ErrCode {
public:    
    ErrCode() : m_lastErrorCode(0) {

    }
    OSStatus getLastErrorCode() {
        return m_lastErrorCode;
    }
    
protected:
    OSStatus m_lastErrorCode;
};

#endif
