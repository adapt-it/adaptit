/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MyListBox.cpp
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \date_revised	28 February 2009
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CMyListBox class. 
/// The CMyListBox class changes certain behaviors of the native wxListBox as implemented on all
/// platforms. Its main purpose is to control the "Translations" list box behavior in the "Choose
/// Translation" dialog. It makes all platforms that use CMyListBox have the same behavior (needed 
/// because their native list box behaviors are all different). 
/// The CMyListBox class detects if the ALT and Right Arrow key combination is pressed when the 
/// list of translations in the ChooseTranslation dialog has focus. If so, it calls its parent 
/// dialog's OnKeyDown() method which can interpret the key combination as a "cancel and select" 
/// command in the dialog. The CMyListBox class also causes the list box on all platforms to 
/// automatically jump to items in the list when the first letter of the item is typed, progressively 
/// stepping through all items which start with the same key character, and cycling back through 
/// all such items on repeated key presses. Finally, the up and down arrow keys cycle back through 
/// items when the end or beginning of the list is reached and the down or up arrow keys respectively 
/// are pressed while the selection is at those ends of the list.
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

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// IMPLEMENT_CLASS(CMyListBox, wxBaseMyListBox)
IMPLEMENT_DYNAMIC_CLASS(CMyListBox, wxListBox)

// event handler table
BEGIN_EVENT_TABLE(CMyListBox, wxListBox)
	EVT_KEY_UP(CMyListBox::OnSysKeyUp)
	EVT_KEY_DOWN(CMyListBox::OnSysKeyDown)
END_EVENT_TABLE()


CMyListBox::CMyListBox() // default constructor
{
	
}

CMyListBox::CMyListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
		const wxSize& size, int n, const wxString choices[], long style) // constructor
		: wxListBox(parent, id, pos, size, n, choices, style)
{
	
}

CMyListBox::~CMyListBox() // destructor
{
	
}

// event handling functions


// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxKeyEvent that is generated after a key is released by the user 
///                         when the list box is in focus
/// \remarks
/// Called from: The EVT_KEY_UP(CMyListBox::OnSysKeyUp) message handling macro.
/// The ALT+RightArrow key press causes the parent dialog's OnKeyDown() method to be called.
/// In the case of the Choose Translation dialog, this can allow its "Cancel and Select" 
/// action to take place.
/// Note: This OnSysKeyUp() handler is activated AFTER the default wxListBox gets key up events.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMyListBox::OnSysKeyUp(wxKeyEvent& event) 
{
	// Note: This OnSysKeyUp() handler is activated AFTER the default wxListBox gets key up events.
	// 
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
	// Note: The special list box selection behaviors are implemented in OnSysKeyDown() below since we
	// want those behaviors to replace the default wxListBox native behaviors on all platforms.
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxKeyEvent that is generated before a key is released by the  
///                         user (and before default wxListBox handling occurs) when the list 
///                         box is in focus
/// \remarks
/// Called from: The EVT_KEY_DOWN(CMyListBox::OnSysKeyDown) message handling macro.
/// Makes all ports have same selection behavior for up and down arrow keys to cycle around the
/// list so that a down arrow press when last item is selected selects the first item in list,
/// and an up arrow press when first item is selected selects the last item in list.
/// Also, makes all ports have same selection behavior for key press jumping to items whose string
/// values start with the char that was typed. Windows has this behavior natively in its list
/// boxes, the Mac only partially, and Linux/wxGTK does not have these behaviors at all without
/// this handler's intervention.
/// Note: This OnSysKeyDown() handler is activated BEFORE the default wxListBox gets key down
/// events. Therefore, any special behaviors that are to replace the default wxListBox behaviors
/// should call "return" after their implementation rather than allowing control to pass through the
/// event.Skip() call at the end of the function.
// //////////////////////////////////////////////////////////////////////////////////////////
void CMyListBox::OnSysKeyDown(wxKeyEvent& event) 
{
	// Note: This OnSysKeyDown() handler is activated BEFORE the default wxListBox gets key down
	// events. Therefore, any special behaviors that are to replace the default wxListBox behaviors
	// should call "return" after their implementation rather than allowing control to pass through the
	// event.Skip() call at the end of the function.
	// 
	// The native Windows and Mac list box has the characteristic that when more than one item is in a
	// list box, if the user types a key the selected item will change to an item that starts with the
	// letter the user typed (while the list box is in focus). wxGTK's native list box, however, doesn't
	// appear to be implemented to behave similarly. Wolfgang Stradner asked if we could implement a
	// circular list selection with the up and down arrow keys (he was using Linux/wxGTK).
	// Implement cycling of list box item selection for up an down arrow key
	if (this->GetCount() > 1)
	{
		// Make all ports have same selection behavior for up and down arrow keys to cycle around the
		// list so that a down arrow press when last item is selected selects the first item in list,
		// and an up arrow press when first item is selected selects the last item in list.
		if (event.GetKeyCode() == WXK_UP && this->GetSelection() == 0)
		{
			// List box has more than one item; first item is selected and user has pressed the up
			// arrow key, so recycle down to select the last item in the list.
			this->SetSelection(this->GetCount() -1);
			this->SetFocus();
			return; // don't pass on the key event to be handled by wxListBox base class
		}
		if (event.GetKeyCode() == WXK_DOWN && this->GetSelection() == (int)this->GetCount()-1)
		{
			// List box has more than one item; the last item is selected and user has pressed the down
			// arrow key, so recycle up to select the first item in the list.
			this->SetSelection(0);
			this->SetFocus();
			return; // don't call Skip() to pass on the key event to be handled by wxListBox base class
		}

		// Make all ports have same selection behavior for key press jumping to items whose string
		// values start with the char that was typed. Windows has this behavior natively in its list
		// boxes, the Mac only partially, and Linux/wxGTK does not have these behaviors at all without
		// the implementation below.
		if (!event.HasModifiers() && this->GetCount() > 1)
		{
			// Enable automatic search and select when user types letter keys on focused list box.
			// When more than one item is in list box, try to select a list item that starts with the
			// character typed. This behavior is native to Windows and Mac list boxes, but apparently is
			// not a native behavior for wxGTK, so we'll implement it here for wxGTK.
			wxChar searchCh = event.GetUnicodeKey();
			wxString searchStr = searchCh;
			int nFound = -1;
			// Search from the current selection position (using the FindListBoxItem() override function
			nFound = gpApp->FindListBoxItem(this,searchStr,caseInsensitive, subString ,fromCurrentSelPosCyclingBack);
			if (nFound != -1)
			{
				this->SetSelection(nFound);
				this->SetFocus();
				return; // don't call Skip() to pass on the key event to be handled by wxListBox base class
			}
		}
	}
	event.Skip(); // all other key strokes are passed on for default handling
}