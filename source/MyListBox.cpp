/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MyListBox.cpp
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CMyListBox class. 
/// The CMyListBox class simply detects if the ALT and Right Arrow key combination
/// is pressed when the list of translations in the ChooseTranslation dialog has 
/// focus. If so, it is interpreted as a "cancel and select" command in the dialog.
/// \derivation		The CMyListBox class is derived from wxListBox.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in MyListBox.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "MyListBox.h"
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
#include "MyListBox.h"
#include "ChooseTranslation.h"

// IMPLEMENT_CLASS(CMyListBox, wxBaseMyListBox)
IMPLEMENT_DYNAMIC_CLASS(CMyListBox, wxListBox)

// event handler table
BEGIN_EVENT_TABLE(CMyListBox, wxListBox)
	EVT_KEY_UP(CMyListBox::OnSysKeyUp)
END_EVENT_TABLE()


CMyListBox::CMyListBox() // constructor
{
	
}

CMyListBox::~CMyListBox() // destructor
{
	
}

// event handling functions
void CMyListBox::OnSysKeyUp(wxKeyEvent& event) 
{
	CChooseTranslation* pParent = (CChooseTranslation*)GetParent();
	wxASSERT(pParent);

	if (event.AltDown())
	{
		// ALT key is down, so if right arrow key pressed, interpret the ALT key press as
		// a click on "Cancel And Select" button
		if (event.GetKeyCode() == WXK_RIGHT)
		{
			// there is no WM_SYSKEYUP or DOWN message available in the dialog subclass, so
			// use this one instead, and don't pass anything else on in the OnKeyDown function
			// so we get no surprises
			pParent->OnKeyDown(event);
			return;
		}
	}
}
