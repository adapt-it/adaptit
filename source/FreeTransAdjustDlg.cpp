/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTransAdjustDlg.cpp
/// \author			Bruce Waters
/// \date_created	16 November 2013
/// \rcs_id $Id: FreeTransAdjustDlg.cpp 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the FreeTransAdjustDlg class.
/// The FreeTransAdjustDlg class provides a handler for the "Adjust..." button in the 
/// compose bar. It provides options for the user when his typing of a free translation
/// exceeds the space available for displaying it. Options include joining to the
/// following or previous section, splitting the text (a child dialog allows the user to
/// specify where to make the split), or removing the last word typed and otherwise do
/// nothing except close the dialog - which allows the user to make further edits (for
/// example, to use fewer words that convey the correct meaning) without the dialog
/// forcing itself open (unless the edits again exceed allowed space).
/// The wxDesigner resource is FTAdjustFunc
/// \derivation		The FreeTransAdjustDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "FreeTransAdjustDlg.h"
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
#include "helpers.h"
#include "Pile.h"
#include "FreeTrans.h"
#include  "MainFrm.h"
#include "Adapt_ItCanvas.h"
#include "FreeTransAdjustDlg.h"

extern bool gbIsGlossing;

// event handler table
BEGIN_EVENT_TABLE(FreeTransAdjustDlg, AIModalDialog)
	EVT_INIT_DIALOG(FreeTransAdjustDlg::InitDialog)
	EVT_BUTTON(wxID_OK, FreeTransAdjustDlg::OnOK)
	//EVT_BUTTON(wxID_CANCEL, FreeTransAdjustDlg::OnCancel)
	//EVT_RADIOBUTTON(ID_RADIO_JOIN_TO_NEXT, FreeTransAdjustDlg::OnRadioJoinToNext)
	//EVT_RADIOBUTTON(ID_RADIO_JOIN_TO_PREVIOUS, FreeTransAdjustDlg::OnRadioJoinToPrevious)
END_EVENT_TABLE()

FreeTransAdjustDlg::FreeTransAdjustDlg(
		wxWindow* parent) : AIModalDialog(parent, -1, _("Adjust Section or Typing"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	m_pFreeTransAdjustSizer = FTAdjustFunc(this, TRUE, TRUE);

	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pApp = &wxGetApp();
	m_pMainFrame = (CMainFrame*)parent;
	m_pFreeTrans = m_pApp->GetFreeTrans(); // for access to the one and only CFreeTrans class's instance
	//bool bOK;
	//bOK = m_pApp->ReverseOkCancelButtonsForMac(this); // <- unneeded so long as we have
														// no Cancel button
	//bOK = bOK; // avoid warning

	CentreOnParent();
}

FreeTransAdjustDlg::~FreeTransAdjustDlg() // destructor
{
}

void FreeTransAdjustDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pRadioJoinToNext = (wxRadioButton*)FindWindowById(ID_RADIO_JOIN_TO_NEXT);
	m_pRadioJoinToPrevious = (wxRadioButton*)FindWindowById(ID_RADIO_JOIN_TO_PREVIOUS);
	m_pRadioSplitIt = (wxRadioButton*)FindWindowById(ID_RADIO_SPLIT_OFF);
	m_pRadioManualEdit = (wxRadioButton*)FindWindowById(ID_RADIO_INSERT_WIDENER);
	m_pRadioDoNothing = (wxRadioButton*)FindWindowById(ID_RADIO_DO_NOTHING);

	// work out where to place the dialog window
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

	m_pFreeTrans->m_savedTypingOffsetForJoin = wxNOT_FOUND; // default is to set it to a meaningless
					// value and only make it 0 or +ve when a join is done
}

void FreeTransAdjustDlg::OnOK(wxCommandEvent& event)
{
	// Set radio button selection so the caller of FreeTransAdjustDlg can get the user's
	// desired option
	if (m_pRadioJoinToNext->GetValue()) // If top radio button is ON, do this block
	{
		// There are two circumstances where we cannot join with what follows. 
		// (a) A chapter or verse starts immediately following the current section
		// (b) The current section ends at the document's end
		// In either circumstance, tell the user to join with the previous section, (if
		// there is no previous section yet, it will be auto-generated first before the
		// join is done). Filter out the two no-go conditions, returning the user to the
		// still open Adjust dialog.  Note: we are ignoring textType changes, which
		// normally we don't ignore - however, to users it is more important to not see
		// truncations, than to be fastidious about textType changes which they aren't
		// able to see anyway
		CPile* pFollowingPile;
		bool bDoesFreeTransImmediatelyFollow = m_pFreeTrans->DoesFreeTransSectionFollow(pFollowingPile);
		if ((pFollowingPile == NULL) && (!bDoesFreeTransImmediatelyFollow))
		{
			// The current free translation section is at the end of the document - so we
			// can't join with what follows
			wxString title = _("Cannot join with what follows");
			wxString msg = _("The current section is at the document's end. There is nothing ahead to join on to. Instead, try to join with what precedes the current section.");
			wxMessageBox(msg,title,wxICON_WARNING | wxOK);
			return; // stay in the Adjust dialog
		}
		else
		{
            // There is more document ahead, so a join is possible provided the
            // pFollowingPile is not the first in a chapter or verse
			if (m_pFreeTrans->DoesItBeginAChapterOrVerse(pFollowingPile))
			{
                // We cannot join with what follows if the current section is followed by
                // a new verse or chapter
				wxString title = _("Illegal join");
				wxString msg = _("Joining across the boundary between two verses or two chapters is illegal. Instead, try to join with what precedes the current section.");
				wxMessageBox(msg,title,wxICON_WARNING | wxOK);
				return; // stay in the Adjust dialog
			} // end of TRUE block for test: if (!DoesItBeginAChapterOrVerse(pFollowingPile))

		} // end else block for test: if ((pFollowingPile == NULL) && (!bDoesFreeTransImmediatelyFollow))

		// We can do the join...
		//selection = 0;
		m_pFreeTrans->m_pFollowingAnchorPile = pFollowingPile; // it may or may not be
					// the anchor pile in a next free trans section; the boolean
					// CFreeTrans::m_bFreeTransSectionImmediatelyFollows tells us
					// which is the case - if it is FALSE, we have to create the 
					// following section before we join to it
		m_pFreeTrans->m_bFreeTransSectionImmediatelyFollows = bDoesFreeTransImmediatelyFollow;
		m_pFreeTrans->m_bAllowOverlengthTyping = FALSE;

		// Do the join (and creation of a 'next' section if necessary first) -- but delay it
		// until the current Draw() is done, so just post a custom event that will make
		// GetFreeTrans()->DoJoinWithNext() be called soon
		wxCommandEvent eventCustom(wxEVT_Join_With_Next);
		wxPostEvent(m_pMainFrame, eventCustom);

	} else if (m_pRadioJoinToPrevious->GetValue()) // If second radio button is ON, do this block
	{
		// There are two circumstances where we cannot join with what precedes. 
		// (a) The current section is at the start of a chapter or verse
		// (b) The current section starts at the document's start
		// In either circumstance, tell the user to join with the following section, (if
		// there is no following section yet, it will be auto-generated first before the
		// join is done). Filter out the two no-go conditions, returning the user to the
		// still open Adjust dialog. Note: we are ignoring textType changes, which
		// normally we don't ignore - however, to users it is more important to not see
		// truncations, than to be fastidious about textType changes which they aren't
		// able to see anyway
		CPile* pPrecedingPile;
		CPile* pCurAnchorPile = m_pApp->m_pActivePile;
		// Set pPrecedingPile to whatever CPile ptr immediately precedes, and return TRUE
		// if it is the ending pile of a free translation section
		bool bDoesFreeTransImmediatelyPrecede = m_pFreeTrans->DoesFreeTransSectionPrecede(pPrecedingPile);
		m_pFreeTrans->m_pImmediatePreviousPile = pPrecedingPile; // BEWARE, it could be NULL
		if ((pPrecedingPile != NULL) && m_pFreeTrans->IsEndOfFootnoteEndnoteOrXRef(pPrecedingPile))
		{
			// We don't allow joining to previous part of the document (whether in a free
			// translation section or not) if the first pile immediately preceding the
			// start of the current free translation section is one which is the end of a
			// footnote, endnote, or cross reference - if so, warn user and leave Adjust
			// dialog open for the user to do something else (eg. insert a section widener
			// instead)
			wxString title = _("Cannot join with what precedes");
			wxString msg = _("The current free translation section is preceded by one of the following: a footnote, endnote, or cross reference. Joining to one of these is not allowed. Instead, join with what follows the current section, or take the option to insert a section widener.");
			wxMessageBox(msg,title,wxICON_WARNING | wxOK);
			return; // stay in the Adjust dialog
		} 
		else if ((pPrecedingPile == NULL) && (!bDoesFreeTransImmediatelyPrecede))
		{
			// The current free translation section is at the start of the document - so
			// we cannot join with what precedes
			wxString title = _("Cannot join with what precedes");
			wxString msg = _("The current section is at the document's start. There is nothing preceding to join on to. Instead, try to join with what follows the current section.");
			wxMessageBox(msg,title,wxICON_WARNING | wxOK);
			return; // stay in the Adjust dialog
		}
		else
		{
			// There is more document preceding, so a join is possible provided current
			// anchor pile is not starting a chapter of verse 
			if (m_pFreeTrans->DoesItBeginAChapterOrVerse(pCurAnchorPile))
			{
                // We cannot join with what precedes if the current section is the start of
                // a verse or chapter
				wxString title = _("Illegal join");
				wxString msg = _("Joining across the boundary between two verses or two chapters is illegal. Instead, try to join with what follows the current section.");
				wxMessageBox(msg,title,wxICON_WARNING | wxOK);
				return; // stay in the Adjust dialog
			} // end of TRUE block for test: if (!DoesItBeginAChapterOrVerse(pFollowingPile))

		} // end else block for test: if ((pPrecedingPile == NULL) && (!bDoesFreeTransImmediatelyPrecede))

		// We can do the join... there may be an immediately preceding section to join
		// to, or there is no immediately preceding section - in which case we'd need to
		// auto-generate one there first and then join to it
		
		// FindPreviousFreeTranslation() scans back from pPrecedingPile until it comes to
		// a pile for which it's pSrcPhrase has m_bStartFreeTrans set to TRUE -
		// BEWARE, this section's end may not abutt the beginning of the current section,
		// so we'll have to do additional testing to check that out (it does abutt,
		// provided that the above boolean, bDoesFreeTransImmediatelyPrecede, stores TRUE)
		// selection = 1;
		m_pFreeTrans->m_pPreviousAnchorPile = m_pFreeTrans->FindPreviousFreeTransSection(pPrecedingPile);
		m_pFreeTrans->m_bFreeTransSectionImmediatelyPrecedes = bDoesFreeTransImmediatelyPrecede;
		m_pFreeTrans->m_bAllowOverlengthTyping = FALSE;
		// Do the join (and creation of a 'previous' section if necessary first) -- but delay it
		// until the current Draw() is done, so just post a custom event that will make
		// GetFreeTrans()->DoJoinWithPrevious() be called soon
		wxCommandEvent eventCustom(wxEVT_Join_With_Previous);
		wxPostEvent(m_pMainFrame, eventCustom);

	} else if (m_pRadioSplitIt->GetValue())  // If third radio button is ON, do this block
	{
		// This is the block for the "Split it. ..." option
		
		//selection = 2;
		m_pFreeTrans->m_bAllowOverlengthTyping = FALSE;

	} else if (m_pRadioManualEdit->GetValue())  // If fourth radio button is ON, do this block
	{
		// This is the block for removing last typed word and then doing nothing
		



		//selection = 3;
		m_pFreeTrans->m_bAllowOverlengthTyping = FALSE;

	} else if (m_pRadioDoNothing->GetValue())
	{
        // do nothing except facilitate the user typing beyond the allowed space in the
        // section without having the Adjust dialog open at every keystroke
		m_pFreeTrans->m_bAllowOverlengthTyping = TRUE;
		//selection = 4;
	}
	else
	{
		// default is to do nothing except facilitate the user typing beyond the allowed
		// space in the section without having the Adjust dialog open at every keystroke, 
		// however... we should not ever come thru here
		m_pFreeTrans->m_bAllowOverlengthTyping = TRUE;
		//selection = 4;
	}

	// Prevent unwanted reentrancy
	m_pFreeTrans->m_adjust_dlg_reentrancy_limit++;
	event.Skip();
}

//void FreeTransAdjustDlg::OnCancel(wxCommandEvent& event)
//{
//	event.Skip();
//}

/*
void FreeTransAdjustDlg::OnRadioJoinToNext(wxCommandEvent& WXUNUSED(event))
{
	// probably not needed
}

void FreeTransAdjustDlg::OnRadioJoinToPrevious(wxCommandEvent& WXUNUSED(event))
{
	// probably not needed
}
*/

