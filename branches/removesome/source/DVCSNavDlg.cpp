/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCS.NavDlg.cpp
/// \author			Mike Hore
/// \date_created	25 March 2013
/// \rcs_id $Id:
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description This is the implementation file for the DVCS version navigation dialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "DVCSNavDlg.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "DVCS.h"
#include "DVCSNavDlg.h"


// event handler table

BEGIN_EVENT_TABLE (DVCSNavDlg, AIModalDialog)
    EVT_BUTTON (ID_BTN_PREV,   DVCSNavDlg::OnPrev)
    EVT_BUTTON (ID_BTN_NEXT,   DVCSNavDlg::OnNext)
    EVT_BUTTON (ID_ACCEPT,     DVCSNavDlg::OnAccept)
    EVT_BUTTON (ID_LATEST,     DVCSNavDlg::OnLatest)
    EVT_BUTTON (wxID_CANCEL,   DVCSNavDlg::OnLatest)        // Cancel button is exactly the same as "return to latest"
    EVT_CLOSE  (               DVCSNavDlg::OnClose)         // Likewise clicking the dialog's close box,
                                                            //  but parm is different so we can't just call OnLatest() here!
END_EVENT_TABLE()



DVCSNavDlg::DVCSNavDlg(wxWindow *parent)
                : AIModalDialog (   parent, -1, wxString(_T("Earlier Document Version")),
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // color used for read-only text controls displaying static text info button face color
    wxColour    sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    
	m_pDlgSizer = DVCSNavDlgFunc ( this, TRUE, TRUE );                          wxASSERT(m_pDlgSizer != NULL);

    m_pVersion_comment   = (wxTextCtrl*) FindWindowById(ID_VERSION_COMMENT);    wxASSERT(m_pVersion_comment != NULL);
    m_pVersion_date      = (wxStaticText*) FindWindowById(ID_VERSION_DATE);     wxASSERT(m_pVersion_date != NULL);
    m_pVersion_committer = (wxStaticText*) FindWindowById(ID_COMMITTER);        wxASSERT(m_pVersion_committer != NULL);
    
    m_pApp = &wxGetApp();               wxASSERT (m_pApp != NULL);
    m_pDoc = m_pApp->GetDocument();     wxASSERT (m_pDoc != NULL);
    
    m_pVersion_comment->SetBackgroundColour (sysColorBtnFace);
}

DVCSNavDlg::~DVCSNavDlg(void)
{ }


void DVCSNavDlg::InitDialog()
{
    wxButton*    defaultButton = (wxButton*)FindWindowById(wxID_CANCEL);

// Set the initial focus to "Return to latest"
    defaultButton->SetFocus();
}

void DVCSNavDlg::ChooseVersion ( int version )
{
    m_pDoc->DoChangeVersion (version);
    
    m_pVersion_comment->ChangeValue (m_pApp->m_pDVCS->m_version_comment);
    m_pVersion_date->SetLabel (m_pApp->m_pDVCS->m_version_date);
    m_pVersion_committer->SetLabel (m_pApp->m_pDVCS->m_version_committer);

}

void DVCSNavDlg::OnPrev (wxCommandEvent& WXUNUSED(event))
{    
    ChooseVersion (m_pApp->m_trialVersionNum + 1);
    Layout();
    Raise();            // Changing version put the doc on top, so we need our dialog back on top
};

void DVCSNavDlg::OnNext (wxCommandEvent& WXUNUSED(event))
{    
    ChooseVersion (m_pApp->m_trialVersionNum - 1);
    Layout();
    Raise();            // Changing version put the doc on top, so we need our dialog back on top
};

void DVCSNavDlg::OnAccept (wxCommandEvent& WXUNUSED(event))
{
    m_pDoc->DoAcceptVersion();      // handles everything, so all we do here is call it
};

void DVCSNavDlg::OnLatest (wxCommandEvent& WXUNUSED(event))
{    
    m_pDoc->DoChangeVersion (-2);     // -2 means latest committed or the backup if it's there. Also removes the dialog and cleans up
};


// We need to catch the situation where the user clicks the dialog's close box while a trial is under way
//  -- the most harmless thing to do is just to treat it as if "return to latest version" had been clicked.

void DVCSNavDlg::OnClose (wxCloseEvent& WXUNUSED(event))
{
    m_pDoc->DoChangeVersion (-2);     // -2 means latest committed or the backup if it's there. Also removes the dialog and cleans up

}

