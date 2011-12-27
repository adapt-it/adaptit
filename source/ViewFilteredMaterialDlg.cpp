/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ViewFilteredMaterialDlg.cpp
/// \author			Bill Martin
/// \date_created	2 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CViewFilteredMaterialDlg class. 
/// The CViewFilteredMaterialDlg class provides a modeless dialog enabling the user to view
/// and edit filtered information. It is the dialog that appears when the user clicks on a
/// (green) wedge signaling the presence of filtered information hidden within the document.
/// The CViewFilteredMaterialDlg is created as a Modeless dialog. It is created on the heap and
/// is displayed with Show(), not ShowModal().
/// \derivation		The CViewFilteredMaterialDlg class is derived from wxScrollingDialog.
/// 
/// BEW 22Mar10, updated for support of doc version 5 (some changes needed)
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ViewFilteredMaterialDlg.h"
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
#include "Adapt_It.h"
#include "ViewFilteredMaterialDlg.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "Pile.h"
#include "Layout.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CPile* gpGreenWedgePile;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

extern const wxChar* filterMkr;
extern const wxChar* filterMkrEnd;

/// This global is defined in Adapt_It.cpp.
extern wxChar gSFescapechar;

/// This global is defined in Adapt_It.cpp.
extern bool gbFreeTranslationJustRemovedInVFMdialog;

wxFontEncoding editBoxEncoding; // stores an enum for which of the encodings is the current one

// event handler table
BEGIN_EVENT_TABLE(CViewFilteredMaterialDlg, wxScrollingDialog)
	EVT_INIT_DIALOG(CViewFilteredMaterialDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CViewFilteredMaterialDlg::OnOK)
	EVT_BUTTON(wxID_CANCEL, CViewFilteredMaterialDlg::OnCancel)
	EVT_LISTBOX(IDC_LIST_MARKER, CViewFilteredMaterialDlg::OnLbnSelchangeListMarker)
	EVT_LISTBOX(IDC_LIST_MARKER_END, CViewFilteredMaterialDlg::OnLbnSelchangeListMarkerEnd)
	EVT_TEXT(IDC_EDIT_MARKER_TEXT, CViewFilteredMaterialDlg::OnEnChangeEditMarkerText)
	EVT_TEXT_ENTER(IDC_EDIT_MARKER_TEXT,CViewFilteredMaterialDlg::ReinterpretEnterKeyPress)
#ifdef _UNICODE
	EVT_BUTTON(IDC_BUTTON_SWITCH_ENCODING, CViewFilteredMaterialDlg::OnButtonSwitchEncoding)
#endif
	EVT_BUTTON(IDC_REMOVE_BTN, CViewFilteredMaterialDlg::OnBnClickedRemoveBtn)
END_EVENT_TABLE()


CViewFilteredMaterialDlg::CViewFilteredMaterialDlg(wxWindow* parent) // dialog constructor
	: wxScrollingDialog(parent, -1, _("View Filtered Material"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pViewFilteredMaterialDlgSizer = ViewFilteredMaterialDlgFunc(this, TRUE, TRUE);
	// The declaration is: ViewFilteredMaterialDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	// get pointers for dialog's controls
	pMarkers = (wxListBox*)FindWindowById(IDC_LIST_MARKER);
	wxASSERT(pMarkers != NULL);
	pEndMarkers = (wxListBox*)FindWindowById(IDC_LIST_MARKER_END); 
	wxASSERT(pEndMarkers != NULL);
	pMkrTextEdit = (wxTextCtrl*)FindWindowById(IDC_EDIT_MARKER_TEXT);
	wxASSERT(pMkrTextEdit != NULL);
	pMkrDescStatic = (wxStaticText*)FindWindowById(IDC_STATIC_MARKER_DESCRIPTION);
	wxASSERT(pMkrDescStatic != NULL);
	pMkrStatusStatic = (wxStaticText*)FindWindowById(IDC_STATIC_MARKER_STATUS);
	wxASSERT(pMkrStatusStatic != NULL);
	pSwitchEncodingButton = (wxButton*)FindWindowById(IDC_BUTTON_SWITCH_ENCODING);
	wxASSERT(pSwitchEncodingButton != NULL);
	pRemoveBtn = (wxButton*)FindWindowById(IDC_REMOVE_BTN);
	wxASSERT(pRemoveBtn != NULL);
}

CViewFilteredMaterialDlg::~CViewFilteredMaterialDlg() // destructor
{
	
}

void CViewFilteredMaterialDlg::ReinterpretEnterKeyPress(wxCommandEvent& event)
{
	// now update the data and invoke OnOK() on the dialog.
	// 
	// A nice thing wxWidgets does is if the wxTE_PROCESS_ENTER style is used for the wxTextCtrl, then
	// wxWidgets itself blocks any newline or carriage return from being entered into the data string,
	// and so no manual intervention is needed here in order to remove such characters. We just need the
	// call below.
	// Here we can't call TransferDataFromWindow() because no validator was setup due to having to deal
	// with different wxArrayString elements that can appear in the edit box.
	// Since this is a modeless dialog we don't call EndModal() here, but instead we simply call the 
	// OnOK() handler, which conveniently also handles the manual transfer of data from the edit box 
	// to the appropriate array element, then closes/hides the dialog.
	OnOK(event);
}

#ifdef _UNICODE
// BEW 3Mar10, no changes needed for support of doc version 5
void CViewFilteredMaterialDlg::OnButtonSwitchEncoding(wxCommandEvent& WXUNUSED(event)) 
{
	wxASSERT(pMkrTextEdit != NULL); 
	if (editBoxEncoding == gpApp->m_navtextFontEncoding)
	{
		// switch to Target text's font and encoding and directionality
		// whm note: SetFont() sets the text control to both the font and its encoding
		CopyFontBaseProperties(gpApp->m_pTargetFont,gpApp->m_pDlgTgtFont);
		gpApp->m_pDlgTgtFont->SetPointSize(gpApp->m_dialogFontSize);
		pMkrTextEdit->SetFont(*gpApp->m_pDlgTgtFont);
		if (gpApp->m_bTgtRTL)
			pMkrTextEdit->SetLayoutDirection(wxLayout_RightToLeft);
		else
			pMkrTextEdit->SetLayoutDirection(wxLayout_LeftToRight);
	}
	else if (editBoxEncoding == gpApp->m_tgtEncoding)
	{
		// switch to Source text's font and encoding and directionality
		// whm note: SetFont() sets the text control to both the font and its encoding
		CopyFontBaseProperties(gpApp->m_pSourceFont,gpApp->m_pDlgTgtFont);
		gpApp->m_pDlgTgtFont->SetPointSize(gpApp->m_dialogFontSize);
		pMkrTextEdit->SetFont(*gpApp->m_pDlgTgtFont);
		if (gpApp->m_bSrcRTL)
			pMkrTextEdit->SetLayoutDirection(wxLayout_RightToLeft);
		else
			pMkrTextEdit->SetLayoutDirection(wxLayout_LeftToRight);
	}
	else if (editBoxEncoding == gpApp->m_srcEncoding)
	{
		// switch to NavText's font and encoding and directionality
		// whm note: SetFont() sets the text control to both the font and its encoding
		CopyFontBaseProperties(gpApp->m_pNavTextFont,gpApp->m_pDlgTgtFont);
		gpApp->m_pDlgTgtFont->SetPointSize(gpApp->m_dialogFontSize);
		pMkrTextEdit->SetFont(*gpApp->m_pDlgTgtFont);
		if (gpApp->m_bNavTextRTL)
			pMkrTextEdit->SetLayoutDirection(wxLayout_RightToLeft);
		else
			pMkrTextEdit->SetLayoutDirection(wxLayout_LeftToRight);
	}

	pMkrTextEdit->Refresh();
	pViewFilteredMaterialDlgSizer->Layout();
}
#endif

// BEW 3Mar10, updated for support of doc version 5 (changes needed)
void CViewFilteredMaterialDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();

	// get the strings to be used for the Remove... button's title
	btnStr = _("Remove %s"); //IDS_REMOVE_BT_OR_FREE
	ftStr = _("Free Translation"); //IDS_FREE_TRANSLATION
	btStr = _("Back Translation"); //IDS_BACK_TRANSLATION
	removeBtnTitle = removeBtnTitle.Format(btnStr.c_str(),ftStr.c_str()); // make "Remove Free Translation" be the default button label
	pRemoveBtn->SetLabel(removeBtnTitle);
	bCanRemoveFT = FALSE;
	bCanRemoveBT = FALSE;
	bRemovalDone = FALSE; // true only while the Remove button's handler is being processed
						  // (and used in it's call of OnBnClickedOk() to suppress updating of the
						  // just-removed text for the array entry which no longer is present
	
	wxString titleStr, SfmSetStr;
	titleStr = this->GetTitle();
	switch(pApp->gCurrentSfmSet)
	{
	case UsfmOnly: SfmSetStr = _("USFM version 2.0 Marker Set"); break; //IDS_USFM_SET_ABBR
	case PngOnly: SfmSetStr = _("PNG 1998 Marker Set"); break; //IDS_PNG_SET_ABBR
	case UsfmAndPng: SfmSetStr = _("USFM 2.0 and PNG 1998 Marker Sets"); break; //IDS_BOTH_SETS_ABBR
	default: SfmSetStr = _("USFM version 2.0 Marker Set"); //IDS_USFM_SET_ABBR
	}
	titleStr += _T(" - ");
	titleStr += SfmSetStr;
	this->SetLabel(titleStr);

	// set up the source font in user's desired point size for m_markers edit and list boxes
	// BEW changed 20Oct05, to display first in the navText's encoding and font -- because
	// filtered info might be stuff which could be English or numerals, and we'll allow the
	// Switch Encoding button to cycle through all 3 possible encodings and fonts in case
	// this assumption is invalid for some or all of the user's data

	// we'll press m_pDlgTgtFont into service for the marker description static texts,
	// but use the source text font for displaying them, at 12 point size
	// BEW changed 20Oct05: if a single-byte font like Annapurna (for Devenagri) is used,
	// using the source font as the base gives Devenagri rubbish, rather than English,
	// so use navText instead; and we'll use the same encoding for the lists of beginning
	// and end markers, rather than source text's encoding (else we see Devenagri rubbish
	// for marker names as well)
	CopyFontBaseProperties(pApp->m_pNavTextFont,pApp->m_pDlgTgtFont);
	pApp->m_pDlgTgtFont->SetPointSize(pApp->m_dialogFontSize);

	// whm note: I've commented out the following as I see no point in setting a 
	// special font/font size for displaying the sfm markers and their description
	// in the "Marker Description" edit box (it turns out too large and long in the
	// dialog).
	//pMkrDescStatic->SetFont(*pApp->m_pDlgTgtFont);
	//pMarkers->SetFont(*pApp->m_pDlgTgtFont);
	//pEndMarkers->SetFont(*pApp->m_pDlgTgtFont);
	pMkrTextEdit->SetFont(*pApp->m_pDlgTgtFont);
	
	// whm note: we start by using the nav text font's encoding for the edit box
	editBoxEncoding = pApp->m_pNavTextFont->GetEncoding(); 
#ifdef _RTL_FLAGS
	// BEW modified 02Aug05 to give differing directionality possibilities when in the
	// Unicode application. When in free translation mode, it is possible (or likely) that
	// the user will get a default free translation by invoking the menu command which says
	// to get default text by copying the relevant target text from the current section's
	// piles. And free translations also use the target text's encoding. So users will almost
	// certainly prefer to see RTL right-justified text in the View Filtered Material dialog
	// if the target text is RTL-rendering; whereas when not in free translation mode the
	// user may prefer the rendering direction to follow the navText's directionality.
	// (I also added a checkbox to cycle through the encodings and potentially the
	// justification on the fly when the dialog is open)
	if (gpApp->m_bFreeTranslationMode)
	{
		// when in free trans mode use the target text's directionality
		if (gpApp->m_bTgtRTL)
		{
			pMkrTextEdit->SetLayoutDirection(wxLayout_RightToLeft);
			pMkrDescStatic->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			pMkrTextEdit->SetLayoutDirection(wxLayout_LeftToRight);
			pMkrDescStatic->SetLayoutDirection(wxLayout_LeftToRight);
		}
	}
	else
	{
		// when not free trans mode use the nav text's directionality
		if (gpApp->m_bNavTextRTL)
		{
			pMkrTextEdit->SetLayoutDirection(wxLayout_RightToLeft);
			pMkrDescStatic->SetLayoutDirection(wxLayout_RightToLeft);
		}
		else
		{
			pMkrTextEdit->SetLayoutDirection(wxLayout_LeftToRight);
			pMkrDescStatic->SetLayoutDirection(wxLayout_LeftToRight);
		}
	}

	pSwitchEncodingButton->Show(TRUE);

#else
	// the ANSI application - hide the button and use navText encoding and LTR rendering 
	// and left justification (the font settings above give the latter)
	pSwitchEncodingButton->Show(FALSE);
#endif

	wxString markerStr;
	wxString tempStr, bareMkrStr, tempMkr;

	// Locate the appropriate source phrase whose filtering members are being
	// displayed/edited. Its m_nSequNumber is stored in the m_nSequNumBeingViewed global
	CAdapt_ItView* pView = gpApp->GetView();
	CSourcePhrase* pSrcPhrase;
	pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nSequNumBeingViewed);
	// markers is a local copy of m_markers via m_nSequNumBeingViewed that was assigned by the
	// routine in OnLButtonDown() in the View.
	// Parse it to get individual markers and associated text available for populating our 
	// list boxes, edit box, and static text descriptions.
	pMarkers->Clear();
	pEndMarkers->Clear();
	bareMarkerArray.Clear();
	assocTextArrayBeforeEdit.Clear();
	assocTextArrayAfterEdit.Clear();
	AllWholeMkrsArray.Clear(); // array of all bare markers encountered in m_markers
	AllEndMkrsArray.Clear(); // array of all end markers encountered in m_markers (contains a space if no end marker)

	// first, 3 wxArrayString arrays for extracting from m_filteredInfo any marker &
	// endmarker pairs and intervening text content (endmarker might be absent for some
	// marker types) - use them to do the extractions, if there is stuff there
	wxArrayString arrMkrs;
	wxArrayString arrEndMkrs;
	wxArrayString arrTextContent;
	bool bHasFilteredInfo = !pSrcPhrase->GetFilteredInfo().IsEmpty();
	if (bHasFilteredInfo)
	{
		// there is filtered info in m_filteredInfo - so get it all (TRUE means
		// "use space for any endmarker which is the empty string"
		bHasFilteredInfo = pSrcPhrase->GetFilteredInfoAsArrays(&arrMkrs, 
							&arrEndMkrs, &arrTextContent, TRUE);
	}
	//bool bHasFreeTrOrNoteOrBackTr = FALSE; set but not used
	wxString strMkr;
	wxString strEndMkr;
	wxString strContent;
	if (pSrcPhrase->m_bStartFreeTrans || !pSrcPhrase->GetFreeTrans().IsEmpty())
	{
		// empty free translation sections are permitted, so the string might be empty
		//bHasFreeTrOrNoteOrBackTr = TRUE;
		strMkr = _T("\\free");
		strEndMkr = strMkr + _T("*");
		strContent = pSrcPhrase->GetFreeTrans();
		AllWholeMkrsArray.Add(strMkr);
		AllEndMkrsArray.Add(strEndMkr);
		assocTextArrayBeforeEdit.Add(strContent);

		// add marker to left list box, then make it a bare marker for lookup & add to list
		pMarkers->Append(strMkr);
		tempMkr = strMkr;
		bareMkrStr = tempMkr.Remove(0,1); // delete the backslash
		bareMarkerArray.Add(bareMkrStr);
		// do the right list box entry
		pEndMarkers->Append(strEndMkr);
	}
	// we'll code here as if empty notes are permitted, but actually currently if an empty
	// note is created, it doesn't actually get created once the user returns to the
	// caller; nevertheless, by allowing empties here, we'll not have to change this code
	// if we later permit empty notes to be created
	if (pSrcPhrase->m_bHasNote || !pSrcPhrase->GetNote().IsEmpty())
	{
		//bHasFreeTrOrNoteOrBackTr = TRUE;
		strMkr = _T("\\note");
		strEndMkr = strMkr + _T("*");
		strContent = pSrcPhrase->GetNote();
		AllWholeMkrsArray.Add(strMkr);
		AllEndMkrsArray.Add(strEndMkr);
		assocTextArrayBeforeEdit.Add(strContent);

		// add marker to left list box, then make it a bare marker for lookup & add to list
		pMarkers->Append(strMkr);
		tempMkr = strMkr;
		bareMkrStr = tempMkr.Remove(0,1); // delete the backslash
		bareMarkerArray.Add(bareMkrStr);
		// do the right list box entry
		pEndMarkers->Append(strEndMkr);
	}
	// now, collected back translation -- always and only has an initial \bt marker, so use
	// a space for the 'endmarker' - so that there is white space in the slot in the dialog
	if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
	{
		//bHasFreeTrOrNoteOrBackTr = TRUE;
		strMkr = _T("\\bt");
		strEndMkr = _T(" ");
		strContent = pSrcPhrase->GetCollectedBackTrans();
		AllWholeMkrsArray.Add(strMkr);
		AllEndMkrsArray.Add(strEndMkr);
		assocTextArrayBeforeEdit.Add(strContent);

		// add marker to left list box, then make it a bare marker for lookup & add to list
		pMarkers->Append(strMkr);
		tempMkr = strMkr;
		bareMkrStr = tempMkr.Remove(0,1); // delete the backslash
		bareMarkerArray.Add(bareMkrStr);
		// do the right list box entry
		pEndMarkers->Append(strEndMkr);
	}
	// All of the above may not be present, but if they are, they must be shown in the
	// above order. Now we append any other filtered stuff from the m_filteredInfo member
	if (bHasFilteredInfo)
	{
		int count = arrMkrs.GetCount();
		if (count > 0)
		{
			int index;
			for (index = 0; index < count; index++)
			{
				strMkr = arrMkrs.Item(index);
				AllWholeMkrsArray.Add(strMkr);
				strEndMkr = arrEndMkrs.Item(index);
				AllEndMkrsArray.Add(strEndMkr);
				assocTextArrayBeforeEdit.Add(arrTextContent.Item(index));

				// add marker to left list box, then make it a bare marker for lookup & add to list
				pMarkers->Append(strMkr);
				tempMkr = strMkr;
				bareMkrStr = tempMkr.Remove(0,1); // delete the backslash
				bareMarkerArray.Add(bareMkrStr);
				// do the right list box entry
				pEndMarkers->Append(strEndMkr);
			}
		}
	}

	// the number of list entries should be the same in Markers and EndMarkers
	wxASSERT(pMarkers->GetCount() == pEndMarkers->GetCount());

	indexIntoMarkersLB = 0; // current selection index for Markers list box
	prevMkrSelection = indexIntoMarkersLB;

    // Change the text in the CEdit to correspond to the marker clicked on in the Marker
    // list box 
    tempStr = assocTextArrayBeforeEdit.Item(indexIntoMarkersLB);
	pMkrTextEdit->ChangeValue(tempStr);

    // Look up the marker description of first item in the bareMarkerArray and place the
    // description in the static text to the right of the "Marker Description:"
	GetAndShowMarkerDescription(indexIntoMarkersLB);

	// to detect editing changes copy the assocTextArrayBeforeEdit array to the
	// assocTextArrayAfterEdit array so they start off as being identical
	// (wxArrayInt has no copy method so do it manually below:)
	int act;
	assocTextArrayAfterEdit.Clear();
	for (act = 0; act < (int)assocTextArrayBeforeEdit.GetCount(); act++)
	{
		assocTextArrayAfterEdit.Add(assocTextArrayBeforeEdit.Item(act));
	}

	// set the selection to the 1st item in Markers, but set the focus to the edit box 
	// with no text selected
	pMkrTextEdit->SetFocus();
	pMkrTextEdit->SetSelection(0,0); // MFC uses -1,0 WX uses 0,0 for no selection
	pMkrTextEdit->SetInsertionPointEnd(); // puts the caret at the end of any text in the edit box
	pMarkers->SetSelection(indexIntoMarkersLB);
	pEndMarkers->SetSelection(indexIntoMarkersLB);
	currentMkrSelection = indexIntoMarkersLB;

	// determine the bool flag values for the IDC_REMOVE_BTN's title, and to determine
	// whether or not it is shown visible
	SetRemoveButtonFlags(pMarkers,indexIntoMarkersLB,bCanRemoveFT,bCanRemoveBT);
	if (bCanRemoveFT)
	{
		// this one is already set up as the default, so we have nothing to do here
		;
	}
	else if (bCanRemoveBT)
	{
		// set up "Remove Back Translation" as its name text
		removeBtnTitle = removeBtnTitle.Format(btnStr.c_str(),btStr.c_str());
		pRemoveBtn->SetLabel(removeBtnTitle);
	}
	else
	{
		// neither bool is TRUE, so it's some other marker, in which case we
		// need to hide the button
		pRemoveBtn->Show(FALSE);
	}
	pViewFilteredMaterialDlgSizer->Layout();
}

// event handling functions

// BEW 5Mar10, updated for support of doc version 5
void CViewFilteredMaterialDlg::OnLbnSelchangeListMarker(wxCommandEvent& WXUNUSED(event))
{
    // wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a
    // call to Clear() so we must check to see if the listbox contains no items and if so
    // return immediately
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pMarkers))
		return;

	// User clicked on a marker in Markers list box
	newMkrSelection = pMarkers->GetSelection();
	// don't do anything if user clicks on the currently selected marker
	if (newMkrSelection == currentMkrSelection)
		return; 

    // BEW added 17Nov05: if the marker just clicked on was \free or \bt ( then we have to
    // make the Remove... button visible and set up its name correctly, so that the user
    // can remove the back translation or the free translation if he wishes.
	SetRemoveButtonFlags(pMarkers,newMkrSelection,bCanRemoveFT,bCanRemoveBT);
	if (bCanRemoveFT)
	{
		// set up "Remove Free Translation" as its name text
		removeBtnTitle = removeBtnTitle.Format(btnStr.c_str(),ftStr.c_str());
		pRemoveBtn->SetLabel(removeBtnTitle);
		pRemoveBtn->Show(TRUE);
	}
	else if (bCanRemoveBT)
	{
		// set up "Remove Back Translation" as its name text
		removeBtnTitle = removeBtnTitle.Format(btnStr.c_str(),btStr.c_str());
		pRemoveBtn->SetLabel(removeBtnTitle);
		pRemoveBtn->Show(TRUE);
	}
	else
	{
		// neither bool is TRUE, so it's some other marker, in which case we
		// need to hide the button
		pRemoveBtn->Show(FALSE);
	}
	pRemoveBtn->Update();

	// User clicked on a different marker in Markers list box, so if changes
	// have been made, we need to first store the current associated text in 
	// assocTextArrayAfterEdit.
	wxString curText;
	curText = pMkrTextEdit->GetValue();
	if (curText != assocTextArrayBeforeEdit.Item(currentMkrSelection))
	{
		assocTextArrayAfterEdit[currentMkrSelection] = curText;
	}

	// Now update the text in the CEdit to correspond to the marker 
	// clicked on (newMkrSelection) in the Marker list box
	wxString tempStr;
	tempStr = assocTextArrayAfterEdit.Item(newMkrSelection);

    // whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the
    // wxEVT_COMMAND_TEXT_UPDATED event, which now deprecated SetValue() generates.
	pMkrTextEdit->ChangeValue(tempStr);

	// Look up the marker description of selected item in the bareMarkerArray 
	// and place the description in the static text to the right of the 
	// "Marker Description:"
	GetAndShowMarkerDescription(newMkrSelection);

	pMkrTextEdit->SetFocus();
	pMkrTextEdit->SetSelection(0,0); // MFC uses -1,0 WX uses 0,0 for no selection
	pMkrTextEdit->SetInsertionPointEnd(); // puts the caret at the end of any text in the edit box
	pEndMarkers->SetSelection(newMkrSelection);
	currentMkrSelection = newMkrSelection;
	pViewFilteredMaterialDlgSizer->Layout();
}

// BEW 5Mar10, no change required for doc version 5
void CViewFilteredMaterialDlg::OnLbnSelchangeListMarkerEnd(wxCommandEvent& event)
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (pEndMarkers->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pEndMarkers))
		return;

	// if user selects a marker (or empty line) in the end markers list box
	// this forces the initial markers list to be selected and static text
	// information also updated.
	newMkrSelection = pEndMarkers->GetSelection();
	// don't do anything if user clicks on the currently selected marker
	if (newMkrSelection == currentMkrSelection)
		return;
	pMarkers->SetSelection(newMkrSelection);
	// do everything else as though the user clicked on the corresponding left Marker list box item
	OnLbnSelchangeListMarker(event);
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
// BEW 5Mar10, updated for support of doc version 5
void CViewFilteredMaterialDlg::OnOK(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItView* pView = gpApp->GetView();

	// get a pointer to the CSourcePhrase instance
	CSourcePhrase* pSrcPhrase;
	pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nSequNumBeingViewed);

	// test for no filtered information being present now (user may have removed any free
	// translation or back translation present earlier, and they were all that was there)
	int count = AllWholeMkrsArray.GetCount();
	wxString emptyStr = _T("");
	wxString strMkr, strEndMkr, strContent;
	wxString strFree = _T("\\free");
	wxString strNote = _T("\\note");
	wxString strBT = _T("\\bt");
	if (count == 0)
	{
		// nothing filtered any more on this pSrcPhrase instance
		pSrcPhrase->m_bStartFreeTrans = FALSE;
		pSrcPhrase->m_bHasFreeTrans = FALSE;
		pSrcPhrase->m_bEndFreeTrans = FALSE; // OnBnClickedRemoveBtn() should have cleared these
											 // 3 flags in the free translation section already
		// notes can't be removed from within this dialog, so we know there was no note; so
		// just clear out the other places where we squirrel away filtered information
		pSrcPhrase->SetFreeTrans(emptyStr);
		pSrcPhrase->SetCollectedBackTrans(emptyStr);
		pSrcPhrase->SetFilteredInfo(emptyStr);
	}
	else
	{
		// there is filtered info, this could be information for the m_freeTrans member,
		// and / or the m_note member, and / or the m_collectedBackTrans member, and / or
		// the m_filteredInfo member. Handle each possibility in turn - in that order
		
		// first, we must update assocTextArrayAfterEdit for the currently shown content,
		// otherwise if the user has edited it and then clicks OK button, the update won't
		// be done and the edit change would be lost
		wxString curText;
		curText = pMkrTextEdit->GetValue();
		if (curText != assocTextArrayBeforeEdit.Item(currentMkrSelection))
		{
			assocTextArrayAfterEdit[currentMkrSelection] = curText;
		}

		if (count > 0)
		{
			strMkr = AllWholeMkrsArray.Item(0); // free trans, if present, is always first
			if (strMkr == strFree)
			{
				// there is a free translation present, so store it's possibly new value
				strContent = assocTextArrayAfterEdit.Item(0);
				pSrcPhrase->m_bStartFreeTrans = TRUE; // redundant, but ensures safety
				pSrcPhrase->m_bHasFreeTrans = TRUE;   // ditto
				pSrcPhrase->SetFreeTrans(strContent);

				// shorten the arrays
				assocTextArrayAfterEdit.RemoveAt(0,1);
				assocTextArrayBeforeEdit.RemoveAt(0,1);
				AllWholeMkrsArray.RemoveAt(0,1);
				AllEndMkrsArray.RemoveAt(0,1);
				count = AllWholeMkrsArray.GetCount();
			}
		}
		// handle a note, if present
		if (count > 0)
		{
			strMkr = AllWholeMkrsArray.Item(0); // note, if present, follows free trans
			if (strMkr == strNote)
			{
				// there is a note present, so store it's possibly new value
				strContent = assocTextArrayAfterEdit.Item(0);
				pSrcPhrase->m_bHasNote = TRUE; // redundant, but ensures safety
				pSrcPhrase->SetNote(strContent);

				// shorten the arrays
				assocTextArrayAfterEdit.RemoveAt(0,1);
				assocTextArrayBeforeEdit.RemoveAt(0,1);
				AllWholeMkrsArray.RemoveAt(0,1);
				AllEndMkrsArray.RemoveAt(0,1);
				count = AllWholeMkrsArray.GetCount();
			}
		}
		// handle a collected back translation, if present
		if (count > 0)
		{
			strMkr = AllWholeMkrsArray.Item(0); // collected back trans, if present, follows note
			if (strMkr == strBT)
			{
				// there is a collected back trans present, so store it's possibly new value
				strContent = assocTextArrayAfterEdit.Item(0);
				pSrcPhrase->SetCollectedBackTrans(strContent);

				// shorten the arrays
				assocTextArrayAfterEdit.RemoveAt(0,1);
				assocTextArrayBeforeEdit.RemoveAt(0,1);
				AllWholeMkrsArray.RemoveAt(0,1);
				AllEndMkrsArray.RemoveAt(0,1);
				count = AllWholeMkrsArray.GetCount();
			}
		}
		if (count > 0)
		{
			// more remains, anything else belongs in m_filteredInfo (in the call below,
			// TRUE means that a placeholding space character in any item in the endmarkers
			// list should be ignored, and an empty endmarker string assumed instead)
			pSrcPhrase->SetFilteredInfoFromArrays(&AllWholeMkrsArray, 
							&AllEndMkrsArray, &assocTextArrayAfterEdit, TRUE);
			assocTextArrayAfterEdit.Clear();
			assocTextArrayBeforeEdit.Clear();
			AllWholeMkrsArray.Clear();
			AllEndMkrsArray.Clear();
		}
	}
	gpApp->m_nSequNumBeingViewed = -1;	// -1 can be used in the view to indicate if the 
										// ViewFilteredMaterialDlg dialog is active/inactive
	Destroy();
	// wx version: See note in OnCancel(). Here, however calling delete doesn't appear to be necessary.
	// No memory leaks detected in the wx version.
	//delete gpApp->m_pViewFilteredMaterialDlg; // BEW added 19Nov05, to prevent memory leak
	gpApp->m_pViewFilteredMaterialDlg = NULL;
	gpGreenWedgePile = NULL;
	pView->Invalidate();
	gpApp->m_pLayout->PlaceBox();
	//AIModalDialog::OnOK(event); // we are running modeless so don't call the base method
}


// The function either calls EndModal(wxID_CANCEL) if the dialog is modal, or sets the 
// return value to wxID_CANCEL and calls Show(false) if the dialog is modeless.
// BEW 5Mar10, no changes needed for support of doc version 5
void CViewFilteredMaterialDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	CAdapt_ItView* pView = gpApp->GetView();
	Destroy();
	// wx version: Unless delete is called on the App's pointer to the dialog below, a "ghost" dialog
	// appears (with no data in controls) when clicking on a different green wedge when the dialog 
	// was still open at a different green wedge location. 
	delete gpApp->m_pViewFilteredMaterialDlg; // BEW added 19Nov05, to prevent memory leak // harmful in wx!!!
	gpApp->m_pViewFilteredMaterialDlg = NULL; // allow the Note dialog to be opened
	gpGreenWedgePile = NULL;
	pView->Invalidate();
	gpApp->m_pLayout->PlaceBox();
	//wxDialog::OnCancel(event); //don't call base class because we are modeless
}

// BEW 5Mar10, no changes needed for support of doc version 5
void CViewFilteredMaterialDlg::OnEnChangeEditMarkerText(wxCommandEvent& WXUNUSED(event))
{
	// This OnEnChangeEditMarkerText is called every time SetValue() is called which happens
	// anytime the user selects a marker from the list box, even though he makes no changes to
	// the associated text in the IDC_EDIT_MARKER_TEXT control. That is a difference between
	// the EVT_TEXT event macro design and MFC's ON_EN_TEXT macro design. Therefore we need
	// to add the enclosing if block with a IsModified() test to the wx version.
	if (pMkrTextEdit->IsModified())
	{
		// change "OK" button label to "Save Changes"
		wxButton* pOKButton = (wxButton*)FindWindowById(wxID_OK);
		wxASSERT(pOKButton != NULL);
		// BEW changed 17Nov05 to make the button's text be localizable easily
		wxString btnTitle;
		btnTitle = _("&Save Changes"); //IDS_SAVE_FDLG_CHANGES
		pOKButton->SetLabel(btnTitle);
		pViewFilteredMaterialDlgSizer->Layout();
	}
}

// BEW 5Mar10, updated for support of doc version 5
void CViewFilteredMaterialDlg::OnBnClickedRemoveBtn(wxCommandEvent& WXUNUSED(event))
{
	// TODO: The "Remove Free Translation" button needs to be hidden again after removing
	// an existing translation unless a second Free translation exists (not likely) in
	// the Marker listbox.
	bRemovalDone = TRUE;

	if (gpApp->m_bFreeTranslationMode)
	{
		// BEW added 29Apr06, to set bool which will suppress reinsertion of empty \free \free* filtered
		// section (with no content) if the user presses <Prev, Next> or Advance button after having
		// removed a free translation in the view filtered material dialog using its Remove... button
		if (gpGreenWedgePile == gpApp->m_pActivePile)
		{
			// set the flag ONLY when the green wedge clicked is at the start of the current
			// free translation section; because it is here that any <Prev, Next> or Advance button
			// press will kick off from, so it is here that the store of the free trans has to be
			// inhibited; but if the green wedge pile where the removal of the free translation was
			// done is located elsewhere, then we don't inhibit the store at the current location
			gbFreeTranslationJustRemovedInVFMdialog = TRUE;
		}
	}

	CSourcePhrase* pSrcPhrase;
	if (bCanRemoveFT)
	{
		// it is a \free entry we are removing, so we have to also clear the 
		// m_bHasFreeTrans flag in the doc's m_pSourcePhrases list for this particular
		// free translation section, and also the m_bStartFreeTrans and m_bEndFreeTrans
		// flags at its beginning and end as well.
		SPList::Node* pos = gpApp->m_pSourcePhrases->Item(gpApp->m_nSequNumBeingViewed);
		wxASSERT(pos);
		while (pos)
		{
			pSrcPhrase = (CSourcePhrase*)pos->GetData();
			pos = pos->GetNext();
			pSrcPhrase->m_bHasFreeTrans = FALSE;
			pSrcPhrase->m_bStartFreeTrans = FALSE;
			if (pSrcPhrase->m_bEndFreeTrans)
			{
				pSrcPhrase->m_bEndFreeTrans = FALSE;
				break;
			}
		}
	}

	// first remove the beginning marker, its content, and end marker from the dialog
	// (if it doesn't have an end marker, there will be a space there - so we delete that instead)
	pMarkers->Delete(currentMkrSelection);
	int nNewCount = pMarkers->GetCount();
    // whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the
    // wxEVT_COMMAND_TEXT_UPDATED event, which now deprecated SetValue() generates.
	pMkrTextEdit->ChangeValue(_T(""));
	pEndMarkers->Delete(currentMkrSelection);

	// now do the needed deletions in the various arrays, so that the class's
	// storage reflects the deletion just made in the dialog...

	// do it for bareMarkerArray (it stores only the filter markers)
	bareMarkerArray.RemoveAt(currentMkrSelection,1);

	// now remove the stored strings at the index value in currentMkrSelection, from each
	// of the arrays
	assocTextArrayBeforeEdit.RemoveAt(currentMkrSelection,1);
	assocTextArrayAfterEdit.RemoveAt(currentMkrSelection,1);
	AllWholeMkrsArray.RemoveAt(currentMkrSelection,1);
	AllEndMkrsArray.RemoveAt(currentMkrSelection,1);

	// now take stock of what we've done. We may have just deleted the last, or the one
	// and only marker, in which case we don't want to enter any code which assumes the
	// storage arrays are non-empty, otherwise we'd crash.
    // So if we have removed all filtered content, then the nNewCount value computed above
    // will be zero; also we don't want the user to sit there looking at an empty dialog
    // and not being sure what to do next, so we help him out by closing things down for
    // him.
	if (nNewCount == 0)
	{
		// all filtered information has just been removed, so we'll close the dialog down
		// -- this is most easily accomplished at the later call of OnBnClickedOk() it
		// does the required updating
		//UpdateContentOnRemove(); // <<-- no longer needed in docVersion 5
		bRemovalDone = FALSE;
	}
	else
	{
		// the dialog has filtered content remaining, so we need to update it; we'll put the
		// selection at index zero, because this is always safe
		currentMkrSelection = 0;
		indexIntoMarkersLB = 0;
		prevMkrSelection = 0;

		// change the text in the CEdit to correspond to the marker chosen by the
		// currentMkrSelection value
		wxString tempStr = assocTextArrayAfterEdit.Item(currentMkrSelection);
        // whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate
        // the wxEVT_COMMAND_TEXT_UPDATED event, which now deprecated SetValue() generates.
		pMkrTextEdit->ChangeValue(tempStr);

		// look up the marker description and place it in the static text
		GetAndShowMarkerDescription(currentMkrSelection);

		// update the dialog

		// set the focus to the edit box, & no text selected
		pMkrTextEdit->SetFocus();
		pMkrTextEdit->SetSelection(0,0); // MFC uses -1,0 WX uses 0,0 for no selection
		pMkrTextEdit->SetInsertionPointEnd(); // puts the caret at the end of any text in the edit box
		pMarkers->SetSelection(currentMkrSelection);
		pEndMarkers->SetSelection(currentMkrSelection);
		// hide the "Remove Free Translation" button - whm added 25Oct06
		pRemoveBtn->Show(FALSE);
	}
	bRemovalDone = FALSE;
	gpApp->GetDocument()->Modify(TRUE); // mark the document dirty
	pViewFilteredMaterialDlgSizer->Layout();

	// BEW added 16Jul09 to get loss of wedge visibly done when free trans is removed
	gpApp->m_pLayout->Redraw();
}

// takes the list of markers in the first list box in the dialog, and nSelection is the index
// of which marker string is being considered, and works out which, if any, of the two
// bool parameters needs to be set. It does this by looking at the marker at nSelection, if it
// is \free, then bCanRemoveFT is set to TRUE and bCanRemoveBT is cleared to FALSE; if it is
// \bt or a longer marker beginning with \bt, then bCanRemoveBT is set TRUE, and bCanRemoveFT is
// cleared to FALSE; if it is neither marker, then both flags are cleared to FALSE (and in the
// interface both being FALSE results in the button being hidden).
// 
// BEW changed 5Mar10, in docVersion 5, only "collected" back translation information (ie.
// back trans info with marker == \bt) can be removed; any other \bt-derived information
// is stored in m_filteredInfo, and anything there we don't permit to be removed in this
// dialog, so \btv, \bts, and so forth (as in Eaton's marking system used in SAG) is not
// affected by this remove button - but still visible in the dialog if present
void CViewFilteredMaterialDlg::SetRemoveButtonFlags(wxListBox* pMarkers, int nSelection, 
													bool& bCanRemoveFT, bool& bCanRemoveBT)
{
	wxString mkrStr;
	mkrStr = pMarkers->GetString(nSelection); //pMarkers->GetText(nSelection,mkrStr);
	mkrStr.Trim(FALSE); // trim left end
	mkrStr.Trim(TRUE); // trim right end
	bCanRemoveFT = FALSE;
	bCanRemoveBT = FALSE;

	// work out if one of the booleans needs to be true, and set it
	if (mkrStr == _T("\\free"))
	{
		bCanRemoveFT = TRUE;
		return;
	}
	if (wxStrncmp(mkrStr,_T("\\bt"),3) == 0)
	{
		int length = mkrStr.Len();
		if (length == 3)
		{
			// it's our own \bt marker from a collection operation done earlier, so it is
			// removable 
			bCanRemoveBT = TRUE;
		}
		else
		{
			// its a longer marker than \bt, and so is not from our collection operation,
			// so it is not removable
			bCanRemoveBT = FALSE;
		}
	}
	return;
}

void CViewFilteredMaterialDlg::GetAndShowMarkerDescription(int indexIntoMarkersListBox)
{
	wxString tempStr;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	USFMAnalysis* pUsfmAnalysis = NULL;
	wxString newMkr, wholeMkr, statusStr;
	newMkr = AllWholeMkrsArray.Item(indexIntoMarkersListBox);
	wholeMkr = newMkr;
	newMkr.Remove(0,1); // delete the backslash
	pUsfmAnalysis = pDoc->LookupSFM(newMkr);
	// BEW modified 18Nov05 to make the message easily localizable
	wxString msg;
	if (pUsfmAnalysis != NULL)
	{
		pMkrDescStatic->SetLabel(pUsfmAnalysis->description);
		if (pUsfmAnalysis->userCanSetFilter)
		{
			msg = _("The text associated with  %s   can be made available for adaptation."); //IDS_ASSOC_TEXT_CAN
			statusStr = statusStr.Format(msg.c_str(),wholeMkr.c_str());
		}
		else
		{
			msg = _("The text associated with  %s   cannot be made available for adaptation."); //IDS_ASSOC_TEXT_CANNOT
			statusStr = statusStr.Format(msg.c_str(),wholeMkr.c_str());
		}
		pMkrStatusStatic->SetLabel(statusStr);
	}
	else
	{
		tempStr = _("[This Marker Unknown In Currently Selected SFM Set]"); //IDS_MKR_UNKNOWN_TO_CURRENT_SET
		pMkrDescStatic->SetLabel(tempStr);
		// unknown markers which are filtered are always userCanSetFilter and can
		// therefore be made available for adaptation.
		msg = _("The text associated with  %s   can be made available for adaptation."); //IDS_ASSOC_TEXT_CAN
		statusStr = statusStr.Format(msg.c_str(),wholeMkr.c_str());
		pMkrStatusStatic->SetLabel(statusStr);
	}
	pViewFilteredMaterialDlgSizer->Layout();
}
