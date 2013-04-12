/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCS.LogDlg.cpp
/// \author			Mike Hore
/// \date_created	25 March 2013
/// \rcs_id $Id:
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description This is the implementation file for the DVCS dialog showing the history (log).
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "DVCSLogDlg.h"
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
#include "DVCSLogDlg.h"


// event handler table

BEGIN_EVENT_TABLE (DVCSLogDlg, AIModalDialog)
//    EVT_BUTTON (ID_BTN_PREV,   DVCSNavDlg::OnPrev)
//    EVT_BUTTON (ID_BTN_NEXT,   DVCSNavDlg::OnNext)
//    EVT_BUTTON (ID_ACCEPT,     DVCSNavDlg::OnAccept)
//    EVT_BUTTON (ID_LATEST,     DVCSNavDlg::OnLatest)
END_EVENT_TABLE()



DVCSLogDlg::DVCSLogDlg (wxWindow* parent)
                : AIModalDialog (   parent, -1, wxString(_T("Save in History")),
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_dlgSizer = DVCSLogDlgFunc ( this, TRUE, TRUE );
    
//    m_version_comment = (wxTextCtrl*) FindWindowById(ID_VERSION_COMMENT);
//    m_version_date    = (wxStaticText*) FindWindowById(ID_VERSION_DATE);
//    m_version_committer = (wxStaticText*) FindWindowById(ID_COMMITTER);
    
    m_pApp = &wxGetApp();
    m_pDoc = m_pApp->GetDocument();
    
    PopulateList();
}

DVCSLogDlg::~DVCSLogDlg(void)
{ }

/*
void DVCSNavDlg::OnPrev (wxCommandEvent& event)
{
    CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
    
    pDoc->DoChangeRevision (m_pApp->m_trialRevNum + 1);
    
    m_version_comment->SetValue (m_pApp->m_pDVCS->m_version_comment);
    m_version_date->SetLabel (m_pApp->m_pDVCS->m_version_date);
    m_version_committer->SetLabel (m_pApp->m_pDVCS->m_version_committer);
};

void DVCSNavDlg::OnNext (wxCommandEvent& event)
{
    CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
    
    pDoc->DoChangeRevision (m_pApp->m_trialRevNum - 1);

    m_version_comment->SetValue (m_pApp->m_pDVCS->m_version_comment);
    m_version_date->SetLabel (m_pApp->m_pDVCS->m_version_date);
    m_version_committer->SetLabel (m_pApp->m_pDVCS->m_version_committer);
};

void DVCSNavDlg::OnAccept (wxCommandEvent& event)
{
    m_pApp->GetDocument()->DoAcceptRevision();      // handles everything, so all we do here is call it
};

void DVCSNavDlg::OnLatest (wxCommandEvent& event)
{
    CAdapt_ItDoc* pDoc = m_pApp->GetDocument();
    
    pDoc->DoChangeRevision (0);     // zero is the latest - this also removes the dialog and cleans up
};
*/

void DVCSLogDlg::PopulateList()
{
	CAdapt_ItApp*   pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
        
	wxListCtrl*     pLst = (wxListCtrl*) FindWindowById (ID_LST_VERSIONS);
    
// clear out items if necessary
	pLst->ClearAll();
// Add columns
    wxListItem      col0, col1, col2;

    col0.SetId(0); // checkbox
    col0.SetWidth(22);
    col0.SetImage(-1);
    pLst->InsertColumn(0, col0);
    
    col1.SetId(1); // toolbar button (icon and text)
    col1.SetWidth(250);
    col1.SetImage(-1);
    pLst->InsertColumn(1, col1);

    col2.SetId(1); // toolbar button (icon and text)
    col1.SetWidth(250);
    col1.SetImage(-1);
    pLst->InsertColumn(2, col2);

    pLst->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    pLst->SetColumnWidth( 1, wxLIST_AUTOSIZE );
    pLst->SetColumnWidth( 2, wxLIST_AUTOSIZE );

	// add the toolbar buttons
//	wxImageList *il = new wxImageList(16, 16);
            
	// set the column widths to auto-resize
    //    pLst->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    //    pLst->SetColumnWidth( 1, wxLIST_AUTOSIZE );
    
}


