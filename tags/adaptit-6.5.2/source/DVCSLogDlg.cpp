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
// BEW 16Dec13 the following 5 needed for the Reposition...() function in InitDialog()
#include "helpers.h"
#include "Pile.h"
#include "Cell.h"
#include "MainFrm.h"
#include "Adapt_ItCanvas.h"

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

	// Need to set m_ptBoxTopLeft here, it's not set by the caller for this dialog
	wxASSERT(m_pApp->m_pActivePile);
	CCell* pCell = m_pApp->m_pActivePile->GetCell(1);
	this->m_ptBoxTopLeft = pCell->GetTopLeft(); // logical coords
}

DVCSLogDlg::~DVCSLogDlg(void)
{ }


void DVCSLogDlg::InitDialog (void)
{
 	// BEW 16Dec13, added code to have dialog position itself on the monitor on which the
	// running Adapt It app is located (otherwise, it can open far away on a different monitor)
	// work out where to place the dialog window
	int myTopCoord, myLeftCoord, newXPos, newYPos;
	wxRect rectDlg;
	GetSize(&rectDlg.width, &rectDlg.height); // dialog's window frame
	CMainFrame* pMainFrame = m_pApp->GetMainFrame();
	wxClientDC dc(pMainFrame->canvas);
	pMainFrame->canvas->DoPrepareDC(dc);// adjust origin
	// wxWidgets' drawing.cpp sample calls PrepareDC on the owning frame
	pMainFrame->PrepareDC(dc); 
	// CalcScrolledPosition translates logical coordinates to device ones, m_ptBoxTopLeft
	// has been initialized to the topleft of the cell (from m_pActivePile) where the
	// phrase box currently is
	pMainFrame->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x, m_ptBoxTopLeft.y,&newXPos,&newYPos);
	pMainFrame->canvas->ClientToScreen(&newXPos, &newYPos); // now it's screen coords
	RepositionDialogToUncoverPhraseBox_Version2(m_pApp, 0, 0, rectDlg.width, rectDlg.height,
										newXPos, newYPos, myTopCoord, myLeftCoord); // see helpers.cpp
	SetSize(myLeftCoord, myTopCoord, wxDefaultCoord, wxDefaultCoord, wxSIZE_USE_EXISTING);

	PopulateList();
    m_pList->SetFocus();        // initially we want the list focussed so up and down arrows are effective
    m_pList->Select(0);         // and we select the top item (the latest version)
    m_pList->Focus(0);          // and we need this too, so the arrows work from there and not the end of the list!
}


void DVCSLogDlg::PopulateList()
{
	m_pList = (wxListView*) FindWindowById (ID_LST_VERSIONS);
    wxASSERT(m_pList != NULL);

    int         row_count = (int)m_pApp->m_DVCS_log.GetCount();
    wxString    nextLine, name, date, comment, str;

// clear out items if necessary
	m_pList->ClearAll();

// Add the columns -- author, date, comment
    wxListItem      col0, col1, col2;

    col0.SetId(0);
    col0.SetText( _T("Author") );
    col0.SetWidth(120);
    m_pList->InsertColumn(0, col0);

    col1.SetId(1);
    col1.SetText( _T("Date") );
    col1.SetWidth(250);
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

        nextLine = m_pApp->m_DVCS_log.Item (n);     // hash#name#date#comment

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


