/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PlaceInternalMarkers.cpp
/// \author			Bill Martin
/// \date_created	29 May 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CPlaceInternalMarkers class. 
/// The CPlaceInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during adaptation.
/// \derivation		The CPlaceInternalMarkers class is derived from AIModalDialog.
/// BEW 12Apr10, completed all changes needed for support of doc version 5 in this file
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "PlaceInternalMarkers.h"
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
#include <wx/valgen.h> // for wxGenericValidator

#include "Adapt_It.h" // whm added 18Feb05
#include "Adapt_ItDoc.h" // whm added 18Feb05
#include "PlaceInternalMarkers.h"
#include "SourcePhrase.h"
#include "helpers.h"

// globals

extern CSourcePhrase* gpSrcPhrase; // defined in Adapt_ItView.cpp

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
BEGIN_EVENT_TABLE(CPlaceInternalMarkers, AIModalDialog)
	EVT_INIT_DIALOG(CPlaceInternalMarkers::InitDialog)
	EVT_BUTTON(wxID_OK, CPlaceInternalMarkers::OnOK)
	EVT_BUTTON(IDC_BUTTON_PLACE, CPlaceInternalMarkers::OnButtonPlace)
END_EVENT_TABLE()


CPlaceInternalMarkers::CPlaceInternalMarkers(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Placement Of Phrase-Medial Standard Format Markers"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	PlaceInternalMarkersDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_srcPhrase = _T("");
	m_tgtPhrase = _T("");

	wxTextCtrl* pTextCtrlAsStaticPlaceIntMkrs = (wxTextCtrl*)FindWindowById(IDC_TEXTCTRL_AS_STATIC_PLACE_INT_MKRS);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticPlaceIntMkrs->SetBackgroundColour(backgrndColor);

	// **** the following initializations were moved from InitDialog() to ****
	// **** here, see comments in that function body's top for an         ****
	// **** explanation of the reason                                     ****
	
	// make the fonts show user's desired point size in the dialog
	pEditDisabled = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC);
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pEditDisabled, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, pEditDisabled, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	pListBox = (wxListBox*)FindWindowById(IDC_LIST_MARKERS);
	pEditTarget = (wxTextCtrl*)FindWindowById(IDC_EDIT_TGT);

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pEditTarget, NULL,
								pListBox, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, pEditTarget, NULL, 
								pListBox, NULL, gpApp->m_pDlgTgtFont);
	#endif
}

CPlaceInternalMarkers::~CPlaceInternalMarkers() // destructor
{
	
}

// BEW 24Mar10, changes needed for support of doc version 5
void CPlaceInternalMarkers::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
    // BEW note 1Apr10, for doc version 5 and support of removing the data input using
    // globals in favour of doing it with public access functions, ... on investigation it
    // turns out to be the case that OnInit() is not called at instantiation of the
    // instance, but rather when ShowModal() is called. Hence, because the setters must be
    // called earlier than that than ShowModal(), the pointers to the controls will not be
    // initialized if we leave it for InitDialog() to do it. So, I've moved these
    // initializations to the creator instead.
	;
}

// event handling functions

// whm revised 20Feb05 for SFM Filtering support
void CPlaceInternalMarkers::OnButtonPlace(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
	{
		wxMessageBox(_("List box error when getting the current selection, place manually instead"),
		_T(""), wxICON_EXCLAMATION);
		return; // whm added
	}

	int nSel;
	nSel = pListBox->GetSelection();
	// Note: m_markers is NOT pSrcPhrase's m_markers, but simply a member string variable
	// of the CPlaceInternalMarkers class
	m_markers = pListBox->GetString(nSel);

	// get the selection for the CEdit - we are interested in the starting
	// position of the selection
	wxString aSpace = _T(' ');
	wxString emptyStr; emptyStr.Empty();
	long nStartChar;
	long nEndChar;
	pEditTarget->GetSelection(&nStartChar,&nEndChar);
	// BEW 11Oct10, if the user selected, we'll assume he wants the selection removed - so
	// check and do so here. After doing so, we want nStartChar == nEndChar as that is to
	// be the starting location for the checks to be made (see below)
	// BEW changed 10Feb11, because a selection can be gotten accidently by a mouse move
	// when clicking - if Place is done, the marker replaces the selection and then the
	// code below puts a second copy in as well - so we'll check here for a selection and
	// remove it, leaving the nStartChar and nEndChar values the same, and at a space if
	// possible (the user is free to manually edit in the text box, we just don't want the
	// Place button to do editing
	if (nStartChar < nEndChar)
	{
		//pEditTarget->Replace(nStartChar, nEndChar, m_markers);
		pEditTarget->GetSelection(&nStartChar,&nEndChar);
		// BEW added the following on 10Feb11, and removed the Replace() call above
		wxString rangeStr = pEditTarget->GetRange(nStartChar, nEndChar);
		int offset = rangeStr.Find(aSpace);
		if (offset == wxNOT_FOUND)
		{
			// there is no space; we'll assume nEndChar should be reset to nStartChar
			nEndChar = nStartChar;
		}
		else
		{
			// we found a space - put the insertion point there
			nStartChar += offset;
			nEndChar = nStartChar;
		}
	}
	// nStartChar and nEndChar will how be at the same location
	int len;
	wxString str;
	str = pEditTarget->GetValue(); 
	// Note: Under wxTextCtrl set for multiline contents, the lines are separated by Unix-style \n
	// characters, even under Windows (where they would be separated by \r\n sequences under MFC).
	len = str.Length();
	const wxChar* pBuff = str.GetData(); // assume it won't fail
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd;
	pEnd = pBufStart + len; // whm added
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBufStart;
	ptr += nStartChar; // point at the wxChar that the nStartChar offset points at

    // Legacy protocol: find a suitable place to put the marker(s) - if user clicked on a
    // word, put it at end of preceding word, if contiguous to space, then preceding spaces
    // (and reduce spaces to one)
    // BEW 11Oct10, with the advent of USFM and endmarkers, and we don't support a space
    // following an endmarker, and differences between marker types in relation to
    // punctuation (ie. binding versus non-binding versus normal markers) the legacy
    // protocols won't work. For instance, if inserting \k (keyword) marker, it must go
    // following any opening punctuation on the word, so we don't want to have to do
    // parsing of the text in order to be able to place an inline binding marker correctly.
    // The solution is to place the markers exactly where the click was done, or where the
    // selection was removed. If fine motor skills are not sufficient to get that right,
    // manual editing in the control is possible for the user to correct his mistake.
	bool bIsEndmarker = FALSE;
	if (m_markers.Find(_T('*')) != wxNOT_FOUND)
	{
		bIsEndmarker = TRUE;
	}
	if (gpApp->gCurrentSfmSet == PngOnly && (m_markers.Find(_T("\\fe")) != wxNOT_FOUND ||
		m_markers.Find(_T("\\F")) != wxNOT_FOUND))
	{
		bIsEndmarker = TRUE;
	}
	// if m_markers has a begin marker it should have been stored with a trailing space,
	// check, and if there is no space, then add one
	wxString reverse = MakeReverse(m_markers);
	if (reverse[0] != _T(' '))
	{
		m_markers += aSpace;
	}
	// ensure endmarker(s) don't have a trailing space
	if (bIsEndmarker)
		m_markers.Trim();

	// do the placement -- first correcting for a misplaced click...
	// There are two likely errors, brought about by the user not realizing that an
	// endmarker should not follow a space, or a beginmarker must have a space following.
	// In the former situation, we check if a space precedes ptr, and if so, move ptr
	// leftwards until any leftwards spaces lie to the right, then we insert. In the case
	// of a beginmarker, if there is a space following then we move ptr rightwards until
	// there are no more spaces to the right, then we insert.
	if (bIsEndmarker)
	{
		while (ptr > pBufStart && *(ptr - 1) == _T(' '))
		{
			ptr--;
		}
	}
	else
	{
		while (ptr < pEnd && *ptr == _T(' '))
		{
			ptr++;
		}
	}
	nStartChar = (long)(ptr - pBufStart);
	nEndChar = nStartChar;
	pEditTarget->Replace(nStartChar, nEndChar, m_markers);

	// remove the top item from the list box (ie. the selected item)
	pListBox->Delete(nSel); //int nLeft = m_listBox.DeleteString(nSel);
	if (pListBox->GetCount() > 0)
		pListBox->SetSelection(0);
}

// getters and setters

// sets m_srcPhrase
void CPlaceInternalMarkers::SetNonEditableString(wxString str) 
{
	// set the storage for the non-editable string, and put it in the wxTextCtrl
	m_srcPhrase = str;
    // put the data in the source edit box (Note: any initial filtered markers information
    // is not shown, as placement within it is not possible anyway, and so the caller
    // withholds it, and inserts it at the end of the calling function "in place" after the
    // dialog is dismissed)
	pEditDisabled->SetValue(m_srcPhrase);
}

// sets m_tgtPhrase
void CPlaceInternalMarkers::SetUserEditableString(wxString str) 
{
	m_tgtPhrase = str;

	// first ensure that the editable string starts and ends with a space - this
	// ensures our placement algorithm will be safe
	m_tgtPhrase.Trim();
	m_tgtPhrase.Trim(FALSE);
	m_tgtPhrase = _T(" ") + m_tgtPhrase;
	m_tgtPhrase += _T(" ");

    // put the data in the target edit box (Note: any initial filtered markers information
    // is not shown, as placement within it is not possible anyway, and so the caller
    // withholds it, and inserts it at the end of the calling function "in place" after the
    // dialog is dismissed)
	pEditTarget->SetValue(m_tgtPhrase);
}

// populates m_markersToPlaceArray
void CPlaceInternalMarkers::SetPlaceableDataStrings(wxArrayString* pMarkerDataArray) 
{
	// set up the list box from the passed in string array
	int count = pMarkerDataArray->GetCount();
	wxASSERT(count > 0);
	int index;
	wxString stuff;
	// populate the list control
	for (index = 0; index < count; index++)
	{
		stuff = pMarkerDataArray->Item((size_t)index);
		wxASSERT(!stuff.IsEmpty());
		pListBox->Append(stuff);
	}
	// hilight first in the listbox
	if (pListBox->GetCount() > 0)
		pListBox->SetSelection(0);
}

// for returning m_tgtStr data, after placements, to the caller
wxString CPlaceInternalMarkers::GetPostPlacementString()
{
	return m_tgtPhrase;
}

void CPlaceInternalMarkers::OnOK(wxCommandEvent& event) 
{
	m_tgtPhrase = pEditTarget->GetValue();
	// the caller can get the value of m_tgtPhrase using the public 
	// getter function, GetPostPlacementString() 
	event.Skip();
}

// other class methods

