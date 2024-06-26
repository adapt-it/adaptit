/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ComposeBarEditBox.cpp
/// \author			Bill Martin
/// \date_created	22 August 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CComposeBarEditBox class.
/// The CComposeBarEditBox class is subclassed from wxTextCtrl in order to
/// capture certain keystrokes while editing free translation text; and for
/// use in real-time editing of free translation text within the Adapt It
/// main window.
/// \derivation		The CComposeBarEditBox class is derived from wxTextCtrl.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ComposeBarEditBox.cpp (in order of importance): (search for "TODO")
// 1.
//
// Unanswered questions: (search for "???")
// 1.
//
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ComposeBarEditBox.h"
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
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include "Adapt_It.h"
#include "MainFrm.h"
#include "Adapt_ItView.h"
#include "Adapt_ItCanvas.h"
#include "Adapt_ItDoc.h"
#include "FreeTrans.h"
#include "FreeTransAdjustDlg.h"
#include "Pile.h"
#include "Cell.h"
#include "helpers.h"
#include "ComposeBarEditBox.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// The global gpCurFreeTransSectionPileArray was defined in Adapt_It.cpp, but was changed to a member variable
/// of the class CFreeTrans. GDLC 2010-02-16

IMPLEMENT_DYNAMIC_CLASS(CComposeBarEditBox, wxTextCtrl)

// event handler table
BEGIN_EVENT_TABLE(CComposeBarEditBox, wxTextCtrl)
	EVT_TEXT(IDC_EDIT_COMPOSE, CComposeBarEditBox::OnEditBoxChanged)
	EVT_CHAR(CComposeBarEditBox::OnChar)
	// whm 15Mar12 added back for read-only mode handling
	EVT_KEY_DOWN(CComposeBarEditBox::OnKeyDown)
	EVT_KEY_UP(CComposeBarEditBox::OnKeyUp)
END_EVENT_TABLE()

// the following constructor is never executed (see constructor in ComposeBarEditBox.h)
CComposeBarEditBox::CComposeBarEditBox() // constructor
{

}

CComposeBarEditBox::~CComposeBarEditBox() // destructor
{

}

// event handling functions

void CComposeBarEditBox::OnChar(wxKeyEvent& event)
{
	// intercept the Enter key and make it call the OnAdvanceButton() handler
	if (event.GetKeyCode() == WXK_RETURN)
	{
		wxCommandEvent bevent;
		gpApp->GetFreeTrans()->OnAdvanceButton(bevent);
		return; // don't call skip - we don't want the end-of-line character entered
				// into the edit box
	}
	// only call skip when ALT key is NOT down (to avoid base class making a beep)
	// whm 31Aug2021 added AutoCorrected() function to the ComposeBar's Edit Box to
	// have auto-correct functionality working within it, since the compose bar box
	// would generally be used to compose target text.
	if (!event.AltDown())
	{
		bool bSkip = !gpApp->AutoCorrected((CComposeBarEditBox*)this, &event);
		if (bSkip)
			event.Skip();
	}
	// The actual text characters typed in the compose bar's edit box go through here
}

void CComposeBarEditBox::OnEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// we only update edits in free translation mode
	if (gpApp->m_bFreeTranslationMode)
	{
		if (this->IsModified())
		{
			CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
			wxASSERT(pFreeTrans != NULL);
			pFreeTrans->m_adjust_dlg_reentrancy_limit = 1; // make Adjust dialog accessible

			CAdapt_ItView* pView = gpApp->GetView();
			wxASSERT(pView != NULL);

			// whm added 17Jan12 the AI Doc should be set to modified here to fix a bug in
			// which free translations were not being saved in collab mode.
			CAdapt_ItDoc* pDoc = pView->GetDocument();
			if (pDoc != NULL)
			{
				pDoc->Modify(TRUE);
			}

#ifdef _DEBUG
//			wxString amsg = _T("Line 123, OnEditBoxChanged(), in ComposeBarEditBox.cpp");
//			pFreeTrans->DebugPileArray(amsg, pFreeTrans->m_pCurFreeTransSectionPileArray);
#endif
			// BEW 20Nov13
			wxString text = this->GetValue();

			wxClientDC dc((wxWindow*)gpApp->GetMainFrame()->canvas);
			pView->canvas->DoPrepareDC(dc); // need to call this because we are drawing outside OnDraw()
			CPile* pOldActivePile; // set in StoreFreeTranslation but unused here
			CPile* saveThisPilePtr; // set in StoreFreeTranslation but unused here
			dc.SetFont(*gpApp->m_pTargetFont);

			wxString trimmedText = text;
			trimmedText.Trim(); // trims at end by default
			trimmedText.Trim(FALSE); // trims start of text

			// Before it's stored, we update the trimmed string's width (in pixels) - we
			// do this at every wxChar typed, so we can ensure the user does not type
			// beyond what the display rectangles for the current section can display -
			// and if the does, he'll have to either join or split - and will be asked
			wxSize extent;
			dc.GetTextExtent(trimmedText,&extent.x,&extent.y);
			pFreeTrans->m_curTextWidth = extent.x;

			// StoreFreeTranslation uses the current (edited) content of the edit box,
			// trimmed of any final whitespace
			// BEW comment 23Apr15, when supporting / used as a word-breaking character, the
			// StoreFreeTranslation() call will internally change any / characters into
			// ZWSP characters before the store is done to CSourcePhrase's m_freeTrans member,
			// so no tweak is needed in the present function block
			pFreeTrans->StoreFreeTranslation(pFreeTrans->m_pCurFreeTransSectionPileArray,
					pOldActivePile,saveThisPilePtr, retain_editbox_contents, trimmedText);
			// for wx version we need to set the background mode to wxSOLID and the text background
			// to white in order to clear the background as we write over it with spaces during
			// real-time edits of free translation.
			dc.SetBackgroundMode(gpApp->m_backgroundMode); // do not use wxTRANSPARENT here!!!
			dc.SetTextBackground(wxColour(255,255,255)); // white
			pFreeTrans->DrawFreeTranslationsAtAnchor(&dc, gpApp->m_pLayout);

			// whm 4Apr09 note on problem of free translations in main window not being cleared for
			// deletes or other edits that result in a shorter version: We need both Refresh and Update
			// here to force the edit updates to happen in the main window. Note, however, that we must
			// not have Refresh and Update in the View's OnDraw after DrawFreeTranslations is called
			// because there they cause the OnDraw() function to be called repeatedly in a continuous
			// loop resulting in flicker on Windows and program hang on Mac.
            // BEW 21Nov13 removed - it resulted in double draws. Better to define an 
            // EraseDrawRectangle() function and call it prior to drawing each rectangle's contents. 
            // It works well whether typing or deleting characters. See FreeTrans.cpp for its definition
			//pView->canvas->Refresh();
			//pView->canvas->Update();

			// return to the default background mode
			dc.SetBackgroundMode(gpApp->m_backgroundMode);
		}
	}
}

void CComposeBarEditBox::OnKeyUp(wxKeyEvent& event)
{
/* turned out to be a pseudo key binding in accelerator keys code, in CMainFrame creator
#if defined(KEY_2_KLUGE) && !defined(__GNUG__) && !defined(__APPLE__)

    // kluge to workaround the problem of a '2' (event.m_keycode = 50) keypress being
    // interpretted as a F10 function keypress (event.m_keyCode == 121)
	if (event.m_keyCode == 50)
	{
		CMainFrame* pFrame = gpApp->GetMainFrame();
		if (pFrame->m_pComposeBar != NULL)
		{
			wxTextCtrl* pEditBox = pFrame->m_pComposeBarEditBox;
			long from; long to;
			pEditBox->GetSelection(&from,&to);
			wxString a2key = _T('2');
			pEditBox->Replace(from,to,a2key);
			return;
		}
	}
#endif
	*/
	
	// BEW 8Jul14 intercept the CTRL+SHIFT+<spacebar> combination to enter a ZWSP
	// (zero width space) into the composebar's editbox; replacing a selection if
	// there is one defined
	if (event.GetKeyCode() == WXK_SPACE)	
	{
		if (!event.AltDown() && event.CmdDown() && event.ShiftDown())
		{
			OnCtrlShiftSpacebar(this); // see helpers.h & .cpp
			return; // don't call skip - we don't want the end-of-line character entered
			// into the edit box
		}
	}

	if (gpApp->m_bFreeTranslationMode)
	{
// GDLC 2010-03-27 pView no longer used
//		CAdapt_ItView* pView = gpApp->GetView();
//		wxASSERT(pView != NULL);
		CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
		wxASSERT(pFreeTrans != NULL);
		wxCommandEvent bevent;
		// the following block is a work-around to get the ALT+key short-cut keys to work
		// for the buttons on the composebar
		if (event.AltDown())
		{
			int key = event.GetKeyCode();
			if (wxChar(key) == _T('S'))
			{
				pFreeTrans->OnShortenButton(bevent);
			}
			else if (wxChar(key) == _T('L'))
			{
				pFreeTrans->OnLengthenButton(bevent);
			}
			else if (wxChar(key) == _T('R'))
			{
				pFreeTrans->OnRemoveFreeTranslationButton(bevent);
			}
			else if (wxChar(key) == _T('P'))
			{
				pFreeTrans->OnPrevButton(bevent);
			}
			else if (wxChar(key) == _T('N'))
			{
				pFreeTrans->OnNextButton(bevent);
			}
			else if (wxChar(key) == _T('V'))
			{
				pFreeTrans->OnAdvanceButton(bevent);
			}
			else if (wxChar(key) == _T('U'))
			{
				pFreeTrans->OnRadioDefineByPunctuation(bevent);
			}
			else if (wxChar(key) == _T('E'))
			{
				pFreeTrans->OnRadioDefineByVerse(bevent);
			}
		}
	}
	// do not call Skip here to avoid beep in base class' control handlers
	// and we don't want any base class behaviours here either
	//event.Skip();
}
// whm 15Mar12 added back for read-only mode handling
void CComposeBarEditBox::OnKeyDown(wxKeyEvent& event)
{
	// whm added 15Mar12. When in read-only mode don't register any key strokes
	if (gpApp->m_bReadOnlyAccess)
	{
		// return without calling Skip(). Beep for read-only feedback
		::wxBell();
		return;
	}

	event.Skip();
}

