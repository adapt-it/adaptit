/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTransSplitterDlg.cpp
/// \author			Bruce Waters
/// \date_created	29 November 2013
/// \rcs_id $Id: FreeTransSplitterDlg.cpp 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the FreeTransSplitterDlg class.
/// The FreeTransSplitterDlg class provides a handler for the "Split" radio button option in the 
/// FreeTransSplitterDlg dialog. The dialog has four multiline edit boxes. All are "read only".
/// The top one displays the text (typically a translation of a source text) of the
/// current free translation section - and typically that free translation will have text
/// in it which doesn't belong in the current free translation section - hence the need
/// for this dialog and the option which invokes it. 
/// The second edit box displays the
/// current section's free translation. This is the text to be split into two parts. This
/// dialog allows the user to click where he wants the text divided into two parts - the
/// first part will be retained in the current free translation section as its sufficient free
/// translation (although it would be possible to edit it later if the user chooses), and
/// the remainder is put into the start of the next section. If the next section does not
/// exist yet, it is created automatically in order to receive the text. The section which
/// receives the remainder also becomes the new active section.
/// The third text box displays the first part of the split text. Splitting is always done
/// between words. If the click was between words, the text is divided there. If it was in
/// a word, that clicked word becomes the first word of the remainder. If a range of
/// characters is selected, the first character in that range is interpretted as an
/// insertion point and the above two rules then apply. We split whole words, never two
/// parts of a single word.
/// The last text box displays the remainder from the split - it will be put into the next
/// section. If the next section already exists (i.e. it exists and its start abutts the
/// end of the current section) and has a free translation, the remainder text is inserted
/// before it and a space divider is guaranteed to be put there. We preserve the user's
/// typing location, which in most circumstances will move to the next section. However,
/// it the typing location was within the first split of part, then in the new section it
/// will be put at the end of the "remainder" text moved to there.
/// The wxDesigner resource is SplitterDlgFunc
/// \derivation		The FreeTransSplitterDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FreeTransSplitterDlg.h"
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
#include "Adapt_It_wdr.h"
#include "FreeTrans.h"
#include  "MainFrm.h"
#include "FreeTransSplitterDlg.h"

// event handler table
BEGIN_EVENT_TABLE(FreeTransSplitterDlg, AIModalDialog)
	EVT_INIT_DIALOG(FreeTransSplitterDlg::InitDialog)
	EVT_BUTTON(wxID_OK, FreeTransSplitterDlg::OnOK)
	EVT_BUTTON(ID_BUTTON_SPLIT_HERE, FreeTransSplitterDlg::OnButtonSplitHere)
END_EVENT_TABLE()

FreeTransSplitterDlg::FreeTransSplitterDlg(
		wxWindow* parent) : AIModalDialog(parent, -1, _("Split Free Translation"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	m_pFreeTransSplitterSizer = SplitterDlgFunc(this, TRUE, TRUE);

	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = &wxGetApp();
	m_pMainFrame = (CMainFrame*)parent;
	m_pFreeTrans = m_pApp->GetFreeTrans(); // for access to the one and only CFreeTrans class's instance
	CentreOnParent();
}

FreeTransSplitterDlg::~FreeTransSplitterDlg() // destructor
{
}

void FreeTransSplitterDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pEditText = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SRC_TRANS);
	m_pEditFreeTrans = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_FREE_TRANS);
	m_pEditForCurrent = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_CURRENT_SECTION);
	m_pEditForNext = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_NEXT_SECTION);

	// work out where to place the dialog window
	/* We probably won't bother to move it to the side - it contains all the user needs for a correct decision
	int myTopCoord, myLeftCoord, newXPos, newYPos;
	wxRect rectDlg;
	GetSize(&rectDlg.width, &rectDlg.height); // dialog's window frame
	wxClientDC dc(m_pMainFrame->canvas);
	m_pMainFrame->canvas->DoPrepareDC(dc);// adjust origin
	// wxWidgets' drawing.cpp sample calls PrepareDC on the owning frame
	m_pMainFrame->PrepareDC(dc); 
	// CalcScrolledPosition translates logical coordinates to device ones, m_ptBoxTopLeft
	// has been initialized to the topleft of the cell (from m_pActivePile) where the
	// phrase box currently is
	m_pMainFrame->canvas->CalcScrolledPosition(m_ptBoxTopLeft.x, m_ptBoxTopLeft.y,&newXPos,&newYPos);
	m_pMainFrame->canvas->ClientToScreen(&newXPos, &newYPos); // now it's screen coords
	RepositionDialogToUncoverPhraseBox(m_pApp, 0, 0, rectDlg.width, rectDlg.height,
										newXPos, newYPos, myTopCoord, myLeftCoord);
	SetSize(myLeftCoord, myTopCoord, wxDefaultCoord, wxDefaultCoord, wxSIZE_USE_EXISTING);
	*/
	// Put the strings into the boxes
	m_pFreeTrans->GetCurrentSectionsTextAndFreeTranslation(m_theText, m_theFreeTrans);
	m_pEditText->ChangeValue(m_theText);
	m_pEditFreeTrans->ChangeValue(m_theFreeTrans);
	m_pEditForCurrent->ChangeValue(_T(""));
	m_pEditForNext->ChangeValue(_T(""));

	// Set all boxes to use the 12 pt target font for dialogs
	m_pEditText->SetFont(*m_pApp->m_pDlgTgtFont);
	m_pEditFreeTrans->SetFont(*m_pApp->m_pDlgTgtFont);
	m_pEditForCurrent->SetFont(*m_pApp->m_pDlgTgtFont);
	m_pEditForNext->SetFont(*m_pApp->m_pDlgTgtFont);

	// initialize with an arbitrary insertion point half way in the string, don't care if
	// in a word or between words - if user then hits Split and doesn't get what he wants,
	// he can click and Split again to his heart's content
	long length = (long)m_theFreeTrans.Len();
	long half = length / 2;
	m_pEditFreeTrans->SetSelection(half,half);
	// Put focus in the second control, so that the location of the cursor can be seen
	m_pEditFreeTrans->SetFocus();
}

void FreeTransSplitterDlg::OnOK(wxCommandEvent& event)
{
	// Get the split strings for the caller to grab
	m_FreeTransForCurrent = m_pEditForCurrent->GetValue();
	m_FreeTransForCurrent.Trim(); // should not need any space on the end
	m_FreeTransForNext = m_pEditForNext->GetValue();
	// Trim both ends of the next one, we'll give the caller the work of adding a
	// delimiter space to the end if needed
	m_FreeTransForNext.Trim(FALSE);
	m_FreeTransForNext.Trim();

	// Put the split substrings in the CFreeTrans instance, they will be grabbed from
	// there by the DoSplitIt() handler
	m_pFreeTrans->m_strSplitForCurrentSection = m_FreeTransForCurrent;
	m_pFreeTrans->m_strSplitForNextSection = m_FreeTransForNext;

	event.Skip(); // we want the dialog to dispose of itself when OK is clicked
}

void FreeTransSplitterDlg::OnButtonSplitHere(wxCommandEvent& WXUNUSED(event))
{
	// Clear both the two lower editboxes
	m_pEditForCurrent->Clear();
	m_pEditForNext->Clear();
	// Get the insertion point
	long myTo, myFrom;
	m_pEditFreeTrans->GetSelection(&myFrom, &myTo);
	m_offset = (size_t)myFrom;
	// Turn a selection into an insertion point
	if (myFrom != myTo)
	{
		m_pEditFreeTrans->SetSelection(myFrom, myFrom);
	}
	// If the insertion point is within a word, move back and halt at the first space -
	// keep the insertion point following it, that's our split location...
	
    // Pull out the whole string, there are no internal CR or LF characters, only space
    // delimiters (although the user may have typed more than a single space as delimiter -
    // we'll not bother checking or modifying that kind of thing, unless by coincidence a
    // sequence of spaces is at the split location, in which case our use of Trim() will
    // remove them below). The fact that CR and LF are absent means that our m_offset value
    // can be relied on.
	wxString str = m_pEditFreeTrans->GetValue();
	wxChar space = _T(' ');
	// If the insertion point has a space or spaces ahead, delete the space(s) until
	// coming to the next word, or to the end of the string - whichever comes first
	size_t length = str.Len();
	bool bDidRemovals = FALSE;
	while ((m_offset < length) && (str.GetChar(m_offset) == space))
	{
		str.Remove(m_offset,1);
		length = str.Len();
		bDidRemovals = TRUE;
	}
	if (bDidRemovals)
	{
		// m_offset is now at the split location (it could be the end of the string, in
		// which case .Mid() will generate an empty string as the 'remainder')
		m_FreeTransForCurrent = str.Left(m_offset);
		m_FreeTransForNext = str.Mid(m_offset);
		// Trim ends
		m_FreeTransForCurrent.Trim(); // trim whitespace from its end
		m_FreeTransForCurrent.Trim(FALSE); // and at its start
		m_FreeTransForNext.Trim(); // trim whitespace from its end
		m_FreeTransForNext.Trim(FALSE); // there shouldn't be any at the start, but play safe
		// Load the two text controls and return
		m_pEditForCurrent->ChangeValue(m_FreeTransForCurrent);
		m_pEditForNext->ChangeValue(m_FreeTransForNext);
		return;
	}
	// If the insertion place is not at a space, move back until one precedes
	while ((m_offset > 0) && (str.GetChar(m_offset - 1) != space))
	{
		// If we are not at the start of the string, check that the preceding character is
		// not a space - as long as that is so, move back character by character until we
		// come to a space, or to the start the string
		m_offset--;
	}
    // Either m_offset is 0 and the first word will be the start of the remainder string
    // and the string to remain in the current section is empty, or m_offset is pointing at
    // the first character of the first word of the remainder string and a space precedes
	// and whatever precedes that space goes (when trimmed) in the current section. Either
	// way, m_offset is set correctly for splitting using .Left() and .Mid()
	m_FreeTransForCurrent = str.Left(m_offset);
	m_FreeTransForNext = str.Mid(m_offset);
	// Trim ends
	m_FreeTransForCurrent.Trim(); // trim whitespace from its end
	m_FreeTransForCurrent.Trim(FALSE); // and at its start
	m_FreeTransForNext.Trim(); // trim whitespace from its end
	m_FreeTransForNext.Trim(FALSE); // there shouldn't be any at the start, but play safe
	// Load the two text controls and return
	m_pEditForCurrent->ChangeValue(m_FreeTransForCurrent);
	m_pEditForNext->ChangeValue(m_FreeTransForNext);
}
