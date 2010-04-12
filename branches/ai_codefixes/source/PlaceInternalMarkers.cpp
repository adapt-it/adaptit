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
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in PlaceInternalMarkers.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
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
	EVT_INIT_DIALOG(CPlaceInternalMarkers::InitDialog)// not strictly necessary for dialogs based on wxDialog
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

#if defined (_DOCVER5)

	// **** the following initializations were moved from InitDialog() to ****
	// **** here, see comments in that function body's top for an         ****
	// **** explanation of the reason                                     ****
	
	// make the fonts show user's desired point size in the dialog
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
#endif // _DOCVER5
}

CPlaceInternalMarkers::~CPlaceInternalMarkers() // destructor
{
	
}

// BEW 24Mar10, no changes needed for support of _DOCVER5
void CPlaceInternalMarkers::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
    // BEW note 1Apr10, for doc version 5 and support of removing the data input using
    // globals in favour of doing it with public access functions, ... on investigation it
    // turns out to be the case that OnInit() is not called at instantiation of the
    // instance, but rather when ShowModal() is called. Hence, because the setters must be
    // called earlier than that than ShowModal(), the pointers to the controls will not be
    // initialized if we leave it for InitDialog() to do it. So, I've moved these
    // initializations to the creator instead.
	
#if defined (_DOCVER5)
	
	/*	
	// make the edit boxes & list box use the correct fonts, use default size
	// use the current target language font for the list box, etc
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	wxASSERT(pDoc != NULL);

	pEditDisabled = (wxTextCtrl*)FindWindowById(IDC_EDIT_SRC);

	// make the fonts show user's chosen point size in the dialog
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
	*/
#else
	// set up edit boxes and list box
	wxString markers;
	wxString markerStr;
	wxArrayString* pList = gpSrcPhrase->m_pMedialMarkers;
	wxArrayString MkrList;
	wxArrayString* pMkrList = &MkrList;
	int ct;
	for (ct = 0; ct < (int)pList->GetCount(); ct++)
	{
		markers = pList->Item(ct);
		wxASSERT(!markers.IsEmpty());
		// whm added 18Feb05
		// markers here can potentially have filtered marker text as
		// well as other sfms. We'll parse out each of the markers and
		// their associated text into separate items in m_listBox to enable
		// user to be able to place each marker separately as desired in the
		// TGT edit box.
		pMkrList->Clear();
		pDoc->GetMarkersAndTextFromString(pMkrList, markers);
		int ctms;
		for (ctms = 0; ctms < (int)pMkrList->GetCount(); ctms++)
		{
			markerStr = pMkrList->Item(ctms);
			pListBox->Append(markerStr);
		}
	}

	// hilight first in the listbox
	if (pListBox->GetCount() > 0)
		pListBox->SetSelection(0,TRUE);
	

	// compose the original source text - using the saved original
	// source phrases
	SPList* pSrcList = gpSrcPhrase->m_pSavedWords;
	wxASSERT(pSrcList->GetCount() > 1); // must be dealing with a genuine phrase
	wxString fullStr = _T("");
	SPList::Node* pos1 = pSrcList->GetFirst();
	wxASSERT(pos1 != NULL);
	while (pos1 != NULL)
	{
		wxString phrase = _T("");
		CSourcePhrase* pSPhr = (CSourcePhrase*)pos1->GetData();
		pos1 = pos1->GetNext();
		if (pSPhr->m_markers.IsEmpty())
		{
			if (!pSPhr->m_bNullSourcePhrase)
			{
				if (!fullStr.IsEmpty())
					phrase += _T(" ");
			}
		}
		else
		{
			phrase = pSPhr->m_markers + phrase;
		}
		if (!pSPhr->m_bNullSourcePhrase)
			phrase += pSPhr->m_srcPhrase;
		// accumulate the phrase
		fullStr += phrase;
	}
	m_srcPhrase = fullStr;
	m_tgtPhrase = tgtStr;
	pEditDisabled->SetValue(m_srcPhrase);
	pEditTarget->SetValue(m_tgtPhrase);
#endif
}

// event handling functions

// whm revised 20Feb05 for SFM Filtering support
void CPlaceInternalMarkers::OnButtonPlace(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
	{
		wxMessageBox(_("List box error when getting the current selection, place manually instead"), _T(""), wxICON_EXCLAMATION);
		return; // whm added
	}

	int nSel;
	nSel = pListBox->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	wxMessageBox(_("List box error when getting the current selection, place manually instead"), _T(""), wxICON_EXCLAMATION);
	//}
	m_markers = pListBox->GetString(nSel);


	// get the selection for the CEdit - we are interested in the starting
	// position of the selection
	wxString str;
	long nStartChar;
	long nEndChar;
	int len;
	str = pEditTarget->GetValue(); 
	// Note: Under wxTextCtrl set for multiline contents, the lines are separated by Unix-style \n
	// characters, even under Windows (where they would be separated by \r\n sequences under MFC).
	len = str.Length();
	pEditTarget->GetSelection(&nStartChar,&nEndChar);
	const wxChar* pBuff = str.GetData(); // assume it won't fail
	wxChar* pBufStart = (wxChar*)pBuff;
	wxChar* pEnd;
	pEnd = pBufStart + len; // whm added
	wxASSERT(*pEnd == _T('\0'));
	wxChar* ptr = pBufStart;

	// find a suitable place to put the marker(s) - if user clicked on a
	// word, put it at end of preceding word, if contiguous to space, then
	// preceding spaces (and reduce spaces to one)

	ptr += nStartChar;
	// whm Note: If m_tgtPhrase is not preceded by a space the following loop
	// could become infinite, so I've recoded it below to be safe.
	while (ptr > pBufStart && *ptr != _T(' '))
	{
		ptr--;
	}
	if (*ptr == _T(' '))
	{
		nStartChar = (int)(ptr - pBufStart);
		nEndChar = nStartChar + 1;
	}
	else
	{
		nEndChar = nStartChar; // if something is wrong, just insert at
							   // start of the selection
	}
	pEditTarget->SetSelection(nStartChar,nEndChar);
	// insure m_markers is padded with a space on each end
	m_markers.Trim(FALSE); // trim left end
	m_markers.Trim(TRUE); // trim right end
	m_markers = _T(' ') + m_markers + _T(' ');
	pEditTarget->Replace(nStartChar, nEndChar, m_markers);

	// remove the top item from the list box (ie. the selected item)
	pListBox->Delete(nSel); //int nLeft = m_listBox.DeleteString(nSel);
	if (pListBox->GetCount() > 0)
		pListBox->SetSelection(0);
}


#if defined (_DOCVER5)

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

#else
// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CPlaceInternalMarkers::OnOK(wxCommandEvent& event) 
{
	m_tgtPhrase = pEditTarget->GetValue();
	tgtStr = m_tgtPhrase;

	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}
#endif

// other class methods

