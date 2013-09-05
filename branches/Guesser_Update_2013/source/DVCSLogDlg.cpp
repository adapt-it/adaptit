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


// event handler table -- we don't need to modify the default

BEGIN_EVENT_TABLE (DVCSLogDlg, AIModalDialog)
END_EVENT_TABLE()


DVCSLogDlg::DVCSLogDlg (wxWindow* parent)
                : AIModalDialog (   parent, -1, wxString(_T("Document History")),
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	m_dlgSizer = DVCSLogDlgFunc ( this, TRUE, TRUE );
    m_pApp = &wxGetApp();
    wxASSERT(m_pApp != NULL);

    m_pDoc = m_pApp->GetDocument();
    
    PopulateList();
}

DVCSLogDlg::~DVCSLogDlg(void)
{ }

void DVCSLogDlg::PopulateList()
{        
	m_pList = (wxListCtrl*) FindWindowById (ID_LST_VERSIONS);
    wxASSERT(m_pList != NULL);

    int         row_count = m_pApp->m_DVCS_log->GetCount();
    wxString    nextLine, name, date, comment, str;
    
// clear out items if necessary
//	pList->ClearAll();
    
// Add the columns -- author, date, comment
    wxListItem      col0, col1, col2;

    col0.SetId(0);
    col0.SetText( _T("Author") );
    col0.SetWidth(150);
    m_pList->InsertColumn(0, col0);
    
    col1.SetId(1);
    col1.SetText( _T("Date") );
    col1.SetWidth(300);
    m_pList->InsertColumn(1, col1);
    
    col2.SetId(2);
    col2.SetText( _T("Comment") );
    col2.SetWidth(600);
    m_pList->InsertColumn(2, col2);

    for (int n=0; n<row_count; n++)
    {        
        wxListItem item;
        item.SetId(n);
        m_pList->InsertItem( item );

        nextLine = m_pApp->m_DVCS_log->Item (n);    // hash#name#date#comment
        
        str = nextLine.AfterFirst(_T('#'));         // skip hash
        name = str.BeforeFirst(_T('#'));            // get committer name        
        str = str.AfterFirst(_T('#'));              // now skip the name
        date = str.BeforeFirst(_T('#'));            // and get commit date
        comment = str.AfterFirst(_T('#'));          // and the rest of the string, after the separator, is the comment.
                                                    // By making this the last field, it can contain anything, even our # separator
    // now set the column contents:
        m_pList->SetItem (n, 0, name);
        m_pList->SetItem (n, 1, date);
        m_pList->SetItem (n, 2, comment);
        
    }
}


