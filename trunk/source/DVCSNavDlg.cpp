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
	m_dlgSizer = DVCSNavDlgFunc ( this, TRUE, TRUE );

    m_version_comment = (wxTextCtrl*) FindWindowById(ID_VERSION_COMMENT);
    m_version_date    = (wxStaticText*) FindWindowById(ID_VERSION_DATE);
    m_version_committer = (wxStaticText*) FindWindowById(ID_COMMITTER);
    
    m_pApp = &wxGetApp();
    m_pDoc = m_pApp->GetDocument();
}

DVCSNavDlg::~DVCSNavDlg(void)
{ }

void DVCSNavDlg::ChooseVersion ( int version )
{
    m_pDoc->DoChangeVersion (version);
    
    m_version_comment->SetValue (m_pApp->m_pDVCS->m_version_comment);
    m_version_date->SetLabel (m_pApp->m_pDVCS->m_version_date);
    m_version_committer->SetLabel (m_pApp->m_pDVCS->m_version_committer);

}

void DVCSNavDlg::OnPrev (wxCommandEvent& WXUNUSED(event))
{    
    ChooseVersion (m_pApp->m_trialVersionNum + 1);
    Raise();            // Changing version put the doc on top, so we need our dialog back on top
};

void DVCSNavDlg::OnNext (wxCommandEvent& WXUNUSED(event))
{    
    ChooseVersion (m_pApp->m_trialVersionNum - 1);
    Raise();            // Changing version put the doc on top, so we need our dialog back on top
};

void DVCSNavDlg::OnAccept (wxCommandEvent& WXUNUSED(event))
{
    m_pApp->GetDocument()->DoAcceptVersion();      // handles everything, so all we do here is call it
};

void DVCSNavDlg::OnLatest (wxCommandEvent& WXUNUSED(event))
{    
    m_pDoc->DoChangeVersion (0);     // zero is the latest - this also removes the dialog and cleans up
};


// We need to catch the situation where the user clicks the dialog's close box while a trial is under way
//  -- the most harmless thing to do is just to treat it as if "return to latest version" had been clicked.

void DVCSNavDlg::OnClose (wxCloseEvent& WXUNUSED(event))
{
    m_pDoc->DoChangeVersion (0);     // zero is the latest - this also removes the dialog and cleans up
}

