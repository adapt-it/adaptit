/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ComposeBarEditBox.cpp
/// \author			Bill Martin
/// \date_created	22 August 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
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
#include "ComposeBarEditBox.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern wxArrayPtrVoid* gpCurFreeTransSectionPileArray; // new creates on heap in InitInstance, and disposes in ExitInstance

IMPLEMENT_DYNAMIC_CLASS(CComposeBarEditBox, wxTextCtrl)

// event handler table
BEGIN_EVENT_TABLE(CComposeBarEditBox, wxTextCtrl)
	EVT_TEXT(IDC_EDIT_COMPOSE, CComposeBarEditBox::OnEditBoxChanged)
	EVT_CHAR(CComposeBarEditBox::OnChar)
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
	CAdapt_ItView* pView = gpApp->GetView();
	wxASSERT(pView != NULL);
	// intercept the Enter key and make it call the OnAdvanceButton() handler

	if (event.GetKeyCode() == WXK_RETURN)
	{
		wxCommandEvent bevent;
		pView->OnAdvanceButton(bevent);
		return; // don't call skip - we don't want the end-of-line character entered
				// into the edit box
	}
	// only call skip when ALT key is NOT down (to avoid base class making a beep)
	if (!event.AltDown())
		event.Skip();
	// The actual text characters typed in the compose bar's edit box go through here
}

void CComposeBarEditBox::OnEditBoxChanged(wxCommandEvent& WXUNUSED(event))
{
	// we only update edits in free translation mode
	if (gpApp->m_bFreeTranslationMode)
	{
		if (this->IsModified())
		{
			CAdapt_ItView* pView = gpApp->GetView();
			wxASSERT(pView != NULL);
			wxClientDC dc((wxWindow*)gpApp->GetMainFrame()->canvas);
			pView->canvas->DoPrepareDC(dc); // need to call this because we are drawing outside OnDraw()
			pView->canvas->pFrame->PrepareDC(dc); // wxWidgets' drawing.cpp sample also calls PrepareDC on the owning frame
			CPile* pOldActivePile; // set in StoreFreeTranslation but unused here
			CPile* saveThisPilePtr; // set in StoreFreeTranslation but unused here
			// StoreFreeTranslation uses the current (edited) content of the edit box
			pView->StoreFreeTranslation(gpCurFreeTransSectionPileArray,pOldActivePile,saveThisPilePtr,
				retain_editbox_contents, this->GetValue());
			// for wx version we need to set the background mode to wxSOLID and the text background 
			// to white in order to clear the background as we write over it with spaces during
			// real-time edits of free translation.
			dc.SetBackgroundMode(gpApp->m_backgroundMode); // do not use wxTRANSPARENT here!!!
			dc.SetTextBackground(wxColour(255,255,255)); // white
			pView->DrawFreeTranslations(&dc, gpApp->m_pBundle, call_from_edit);
			// return to the default background mode
			dc.SetBackgroundMode(gpApp->m_backgroundMode);
		}
	}
}

void CComposeBarEditBox::OnKeyUp(wxKeyEvent& event)
{
	if (gpApp->m_bFreeTranslationMode)
	{
		CAdapt_ItView* pView = gpApp->GetView();
		wxASSERT(pView != NULL);
		wxCommandEvent bevent;
		// the following block is a work-around to get the ALT+key short-cut keys to work 
		// for the buttons on the composebar
		if (event.AltDown())
		{
			int key = event.GetKeyCode();
			if (wxChar(key) == _T('S'))
			{
				pView->OnShortenButton(bevent);
			}
			else if (wxChar(key) == _T('L'))
			{
				pView->OnLengthenButton(bevent);
			}
			else if (wxChar(key) == _T('R'))
			{
				pView->OnRemoveFreeTranslationButton(bevent);
			}
			else if (wxChar(key) == _T('P'))
			{
				pView->OnPrevButton(bevent);
			}
			else if (wxChar(key) == _T('N'))
			{
				pView->OnNextButton(bevent);
			}
			else if (wxChar(key) == _T('V'))
			{
				pView->OnAdvanceButton(bevent);
			}
			else if (wxChar(key) == _T('U'))
			{
				pView->OnRadioDefineByPunctuation(bevent);
			}
			else if (wxChar(key) == _T('E'))
			{
				pView->OnRadioDefineByVerse(bevent);
			}
		}
	}
	// do not call Skip here to avoid beep in base class' control handlers
	//event.Skip();
}

void CComposeBarEditBox::OnKeyDown(wxKeyEvent& event)
{
	//
	event.Skip();
}

