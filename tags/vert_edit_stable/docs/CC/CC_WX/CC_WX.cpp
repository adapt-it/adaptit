/////////////////////////////////////////////////////////////////////////////
// Name:        CC_WX.cpp
// Author:      XX
// Created:     XX/XX/XX
// Copyright:   
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma implementation "CC_WX.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// Include private headers
#include "CC_WX.h"
#include "CC_WX_wdr.h"

// WDR: class implementations

//------------------------------------------------------------------------------
// CCFrame
//------------------------------------------------------------------------------

// WDR: event table for CCFrame

BEGIN_EVENT_TABLE(CCFrame,wxFrame)
    EVT_BUTTON(wxID_ABOUT, CCFrame::OnAbout)
    EVT_BUTTON(wxID_EXIT, CCFrame::OnQuit)
    EVT_BUTTON(ID_BUTTON_PROCESS, CCFrame::OnBtnProcess)
    EVT_BUTTON(ID_BUTTON_OPTIONS, CCFrame::OnBtnOptions)
    EVT_BUTTON(wxID_HELP, CCFrame::OnBtnHelp)
    EVT_CLOSE(CCFrame::OnCloseWindow)
END_EVENT_TABLE()

CCFrame::CCFrame( wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style ) :
    wxFrame( parent, id, title, position, size, style )
{
 	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	CCDialogAppFunc(this, TRUE, TRUE);
	SetBackgroundColour(wxColour(255,255,255)); // white
	// The declaration is: GoToDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
    //CreateStatusBar(1);
    //SetStatusText( wxT("Welcome to CC_WX!") );
    
     // insert main window here
}

// WDR: handler implementations for CCFrame

void CCFrame::OnAbout( wxCommandEvent &event )
{
    wxMessageDialog dialog( this, wxT("CC_WX (Consistent Changes WX) 1.0\nCross-Platform Consistent Changes build on wxWidgets\nCopyright 2008 Bill Martin, Bruce Waters, SIL International"),
        wxT("About CC_WX"), wxOK|wxICON_INFORMATION );
    dialog.ShowModal();
}

void CCFrame::OnQuit( wxCommandEvent &event )
{
     Close( TRUE );
}

void CCFrame::OnBtnProcess( wxCommandEvent &event )
{
     wxMessageBox(_T("Stub only - Process Button not yet implemented"),_T(""),wxICON_INFORMATION);
}

void CCFrame::OnBtnOptions( wxCommandEvent &event )
{
     wxMessageBox(_T("Stub only - Options Button not yet implemented"),_T(""),wxICON_INFORMATION);
}

void CCFrame::OnBtnHelp( wxCommandEvent &event )
{
     wxMessageBox(_T("Stub only - Help Button not yet implemented"),_T(""),wxICON_INFORMATION);
}

void CCFrame::OnCloseWindow( wxCloseEvent &event )
{
    // if ! saved changes -> return
    
    Destroy();
}

//------------------------------------------------------------------------------
// CCApp
//------------------------------------------------------------------------------

IMPLEMENT_APP(CCApp)

CCApp::CCApp()
{
}

bool CCApp::OnInit()
{
    CCFrame *frame = new CCFrame( NULL, -1, wxT("Consistent Changes WX"), wxDefaultPosition, wxDefaultSize );
    frame->Show( TRUE );
    
    return TRUE;
}

int CCApp::OnExit()
{
    return 0;
}

