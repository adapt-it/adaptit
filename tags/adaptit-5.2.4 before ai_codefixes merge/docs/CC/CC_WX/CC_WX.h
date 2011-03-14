/////////////////////////////////////////////////////////////////////////////
// Name:        CC_WX.h
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   
/////////////////////////////////////////////////////////////////////////////

#ifndef __CC_WX_H__
#define __CC_WX_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "CC_WX.h"
#endif

// Include wxWindows' headers

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "CC_WX_wdr.h"

// WDR: class declarations

//----------------------------------------------------------------------------
// CCFrame
//----------------------------------------------------------------------------

class CCFrame: public wxFrame
{
public:
    // constructors and destructors
    CCFrame( wxWindow *parent, wxWindowID id, const wxString &title,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_FRAME_STYLE );
    
private:
    // WDR: method declarations for CCFrame
    
private:
    // WDR: member variable declarations for CCFrame
    
private:
    // WDR: handler declarations for CCFrame
    void OnAbout( wxCommandEvent &event );
    void OnQuit( wxCommandEvent &event );
    void OnBtnProcess( wxCommandEvent &event );
    void OnBtnOptions( wxCommandEvent &event );
    void OnBtnHelp( wxCommandEvent &event );
    void OnCloseWindow( wxCloseEvent &event );
    
private:
    DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
// CCApp
//----------------------------------------------------------------------------

class CCApp: public wxApp
{
public:
    CCApp();
    
    virtual bool OnInit();
    virtual int OnExit();
};

#endif
