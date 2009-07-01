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
/// \derivation		The CViewFilteredMaterialDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ViewFilteredMaterialDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
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
BEGIN_EVENT_TABLE(CViewFilteredMaterialDlg, wxDialog)
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
	: wxDialog(parent, -1, _("View Filtered Material"),
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



void CViewFilteredMaterialDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();

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
	wxString tempStr, bareMkrStr, initialMkrStr, tempMkr;
	int curPos;

	// Locate the appropriate source phrase whose m_markers member is being
	// displayed/edited. Its m_nSequNumber is stored in the m_nSequNumBeingViewed global
	CAdapt_ItView* pView = gpApp->GetView();
	CSourcePhrase* pSrcPhrase;
	pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nSequNumBeingViewed);
	// markers is a local copy of m_markers via m_nSequNumBeingViewed that was assigned by the
	// routine in OnLButtonDown() in the View.
	// Parse it to get individual markers and associated text available for populating our 
	// list boxes, edit box, and static text descriptions.
	markers = pSrcPhrase->m_markers;
	wxASSERT(!markers.IsEmpty());
	pMarkers->Clear();
	pEndMarkers->Clear();
	bareMarkerArray.Clear();
	markerLBIndexIntoAllMkrList.Clear();
	assocTextArrayBeforeEdit.Clear();
	assocTextArrayAfterEdit.Clear();

	AllMkrsList.Clear(); // list of all markers in m_markers (both filtered and non-filtered)
	AllMkrsFilteredFlags.Clear(); // array of ints that flag if marker in AllMkrsList is filtered (1) or not (0)
	AllWholeMkrsArray.Clear(); // array of all bare markers encountered in m_markers
	AllEndMkrsArray.Clear(); // array of all end markers encountered in m_markers (contains a space if no end marker)

	// parse markers gathering all marker strings it contains into our CStringList
	// AllMkrsList will contain only one marker-text-endmarker segment per stored string
	pDoc->GetMarkersAndTextFromString(&AllMkrsList, markers);
	int indexIntoMkrList = 0;
	int ct;
	for (ct = 0; ct < (int)AllMkrsList.GetCount(); ct++)
	{
		markerStr = AllMkrsList.Item(ct);
		// We ignore non-filter markers and their associated text
		if (markerStr.Find(filterMkr) != -1)
		{
			AllMkrsFilteredFlags.Add(1);
			// It is a filtered marker, so parse the markerStr into three parts: 
			// actual marker, associated text, end marker (if any), and load the 
			// parts in the appropriate controls

			// First we need to remove the filter bracketing markers
			tempStr = pDoc->RemoveAnyFilterBracketsFromString(markerStr);
			// there will always be a marker immediately following the \~FILTER bracket, so 
			// there should also be an initial backslash as required for well formed filtered 
			// material
			wxASSERT(tempStr.Find(gSFescapechar,0) == 0);
			// get the position of the usual space following the filtered marker
			curPos = tempStr.Find(_T(' '));
			if (curPos == -1)
			{
				// there is no space following the marker, so we'll assume it is one of
				// the few like \hr that are not followed by a space and some text, nor
				// any end marker.
				// whm Note 3Jul05 the contentless marker such as \hr and \b are now
				// filter="0" and userCanSetFilter="0" so they should never appear, but
				// to be really safe (in the event that the user has monkeyed with the
				// AI_USFM.xml file or data is somehow garbled so that there is no space
				// following the marker but it now is suffixed with some spurious text, 
				// we'll check to insure that the end of the marker following the backslash 
				// is also the end of the tempStr.
				int mkrLen = 0; 
				int strLen = tempStr.Length();
				const wxChar* pBuffer = tempStr.GetData();
				wxChar* pBufStart = (wxChar*)pBuffer;
				wxChar* ptr = pBufStart;		// point to start of text
				wxChar* pEnd = pBufStart + strLen;// bound past which we must not go // whm 19Jun06 removed + 1
				while (ptr != pEnd && !pDoc->IsWhiteSpace(ptr))
				{
					ptr++;
					mkrLen++;
				}
				mkrLen--;
				wxASSERT(mkrLen == strLen);

				// the tempStr is composed of a single marker
				tempMkr = tempStr;
			}
			else
			{
				// a space follows at curPos so store the marker in tempMkr
				tempMkr = tempStr.Mid(0,curPos);
			}

			// add whole marker to AllWholeMkrsArray
			AllWholeMkrsArray.Add(tempMkr); // used for reassembling m_markers in OnBnClickedOk

			// get bare marker and add it to left list box
			pMarkers->Append(tempMkr);
			initialMkrStr = tempMkr;
			tempMkr.Remove(0,1); // delete the backslash
			bareMkrStr = tempMkr;
			bareMarkerArray.Add(bareMkrStr);
			markerLBIndexIntoAllMkrList.Add(indexIntoMkrList);
			if (curPos != -1)
				tempStr.Remove(0,curPos + 1); // delete intervening space too
			else
				tempStr.Empty();	// no space following the marker so we assume there 
									//is no text to edit nor any end marker

			// After parsing off the begin marker, do an intelligent ReverseFind. If a marker 
			// is found beyond the initial position, check to see if the marker is a corresponding 
			// end marker. If not, we know that we can then consider the remaining text to be 
			// entirely associated text to be inserted into the CEdit. If the marker is a 
			// corresponding end marker, this end marker should normally be located at the end of 
			// the string assuming the filtering process that earlier identified the text-to-be-filtered 
			// did its job properly. To cover the possibility of a malformed filtered string, we should 
			// sill check to insure there is no text after this corresponding end marker. Normally there 
			// would not be anything, but, if there is we should at least wxASSERT such a case.
			curPos = tempStr.Find(gSFescapechar,TRUE); // TRUE is find from right end //curPos = tempStr.ReverseFind(gSFescapechar);
			if (curPos != -1)
			{
				// there is a marker of some sort, is it a corresponding end marker?
				tempMkr = tempStr.Mid(curPos, tempStr.Length());
				int strLen = tempStr.Length();
				// we only need a read-only buffer so use GetData()
				const wxChar* pBuffer = tempStr.GetData();
				wxChar* pBufStart = (wxChar*)pBuffer;
				wxChar* pAtMkr = pBufStart + curPos;
				wxChar* pEnd = pBufStart + strLen; // bound past which we must not go
				if (pDoc->IsCorresEndMarker(initialMkrStr, pAtMkr, pEnd))
				{
					// add the end marker to the right list box
					pEndMarkers->Append(tempMkr);
					AllEndMkrsArray.Add(tempMkr); // used for reassembling m_markers in OnBnClickedOk
					// delete the whole marker
					tempStr.Remove(curPos,tempStr.Length());
				}
				else
				{
					// tempStr is normal text that contains an embedded marker of some sort
					// to be placed along with any associated text in the CEdit further below
					// add a space to the right list box
					pEndMarkers->Append(_T(" "));
					AllEndMkrsArray.Add(_T(" ")); // used for reassembling m_markers in OnBnClickedOk
				}
			}
			else
			{
				// no other marker of any kind including an end marker so just add a space to the 
				// list item
				pEndMarkers->Append(_T(" "));
				AllEndMkrsArray.Add(_T(" ")); // used for reassembling m_markers in OnBnClickedOk
			}
			// add associated text to center CEdit box
			// The associated text is all that is left in tempStr so place it in the CEdit
			tempStr.Trim(FALSE); // trim left end
			tempStr.Trim(TRUE); // trim right end
			assocTextArrayBeforeEdit.Add(tempStr);
		}
		else
		{
			// this marker is not filtered
			AllMkrsFilteredFlags.Add(0);
			tempStr = markerStr;
			curPos = tempStr.Find(_T(' '));
			if (curPos == -1)
			{
				// there is no space following the marker, so we'll assume it is one of
				// the few like \hr that are not followed by a space and some text, nor
				// any end marker.
				tempMkr = tempStr;
			}
			else
			{
				tempMkr = tempStr.Mid(0,curPos);
			}

			// add whole marker to AllWholeMkrsArray
			AllWholeMkrsArray.Add(tempMkr); // used for reassembling m_markers in OnBnClickedOk

			tempMkr.Remove(0,1);
			bareMkrStr = tempMkr;
			if (curPos != -1)
				tempStr.Remove(0,curPos + 1); // delete intervening space too
			else
				tempStr.Empty();	// no space following the marker so we assume there 
									//is no text to edit nor any end marker
			curPos = tempStr.Find(gSFescapechar,TRUE); // TRUE is find from right end //curPos = tempStr.ReverseFind(gSFescapechar);
			if (curPos != -1)
			{
				// there is an end marker
				tempMkr = tempStr.Mid(curPos, tempStr.Length());
				AllEndMkrsArray.Add(tempMkr); // used for reassembling m_markers in OnBnClickedOk
				tempStr.Remove(curPos,tempStr.Length());
			}
			else
			{
				// no end marker so just add a space to the list/array item
				AllEndMkrsArray.Add(_T(" ")); // used for reassembling m_markers in OnBnClickedOk
			}
			// add associated text to center CEdit box
			// The associated text is all that is left in tempStr so place it in the CEdit
			tempStr.Trim(FALSE); // trim left end
			tempStr.Trim(TRUE); // trim right end
			assocTextArrayBeforeEdit.Add(tempStr);
		}

		indexIntoMkrList++;
		wxASSERT((int)AllMkrsFilteredFlags.GetCount() == indexIntoMkrList);
	}

	// the number of list entries should be the same in Markers and EndMarkers
	wxASSERT(pMarkers->GetCount() == pEndMarkers->GetCount());

	indexIntoMarkersLB = 0; // current selection index for Markers list box
	prevMkrSelection = indexIntoMarkersLB;
	// determine index of first item in list box as current selection
	indexIntoAllMkrSelection = markerLBIndexIntoAllMkrList.Item(0);

	// Change the text in the CEdit to correspond to the marker clicked on in the Marker list box
	tempStr = assocTextArrayBeforeEdit.Item(indexIntoAllMkrSelection);
	// whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the wxEVT_COMMAND_TEXT_UPDATED
	// event, which now deprecated SetValue() generates.
	pMkrTextEdit->ChangeValue(tempStr);

	// Look up the marker description of first item in the bareMarkerArray and place the description
	// in the static text to the right of the "Marker Description:"
	GetAndShowMarkerDescription(indexIntoAllMkrSelection);

	// to detect editing changes copy the assocTextArrayBeforeEdit array to the
	// assocTextArrayAfterEdit array so they start being identical
	//assocTextArrayAfterEdit.Copy(assocTextArrayBeforeEdit);
	// wxArrayInt has no copy method so do it manually below:
	int act;
	assocTextArrayAfterEdit.Clear();
	for (act = 0; act < (int)assocTextArrayBeforeEdit.GetCount(); act++)
	{
		assocTextArrayAfterEdit.Add(assocTextArrayBeforeEdit.Item(act));
	}
	changesMade = FALSE;

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

void CViewFilteredMaterialDlg::OnLbnSelchangeListMarker(wxCommandEvent& WXUNUSED(event))
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (pMarkers->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pMarkers))
		return;


	// User clicked on a marker in Markers list box
	newMkrSelection = pMarkers->GetSelection();
	// don't do anything if user clicks on the currently selected marker
	if (newMkrSelection == currentMkrSelection)
		return; 

	// BEW added 17Nov05: if the marker just clicked on was \free or \bt (or a marker
	// beginning with \bt) then we have to make the Remove... button visible and set up
	// its name correctly, so that the user can remove the back translation or the free
	// translation if he wishes.
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
	indexIntoAllMkrSelection = markerLBIndexIntoAllMkrList.Item(currentMkrSelection);
	if (curText != assocTextArrayBeforeEdit.Item(indexIntoAllMkrSelection))
	{
		changesMade = TRUE;
		assocTextArrayAfterEdit[indexIntoAllMkrSelection] = curText;
	}

	// Now update the text in the CEdit to correspond to the marker 
	// clicked on (newMkrSelection) in the Marker list box
	wxString tempStr;
	indexIntoAllMkrSelection = markerLBIndexIntoAllMkrList.Item(newMkrSelection);
	tempStr = assocTextArrayAfterEdit.Item(indexIntoAllMkrSelection);
	// whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the wxEVT_COMMAND_TEXT_UPDATED
	// event, which now deprecated SetValue() generates.
	pMkrTextEdit->ChangeValue(tempStr);

	// Look up the marker description of selected item in the bareMarkerArray 
	// and place the description in the static text to the right of the 
	// "Marker Description:"
	GetAndShowMarkerDescription(indexIntoAllMkrSelection);

	pMkrTextEdit->SetFocus();
	pMkrTextEdit->SetSelection(0,0); // MFC uses -1,0 WX uses 0,0 for no selection
	pMkrTextEdit->SetInsertionPointEnd(); // puts the caret at the end of any text in the edit box
	pEndMarkers->SetSelection(newMkrSelection);
	currentMkrSelection = newMkrSelection;
	pViewFilteredMaterialDlgSizer->Layout();
}

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

void CViewFilteredMaterialDlg::UpdateContentOnRemove()
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	// If any edits were made, reassemble the edited markers text
	// before returning to the caller.
	wxString markersStr = _T("");
	wxString tempStr;
	if (changesMade)
	{
		// mark the document dirty
		pDoc->Modify(TRUE);

		// BEW modified 17Nov05 to handle the addition of the Remove Free Translation
		// (alternatively called Remove Back Translation, if \bt is selected) to the
		// dialog. If there is only a back translation or a free translation, or even just 
		// both, it is possible that the user by using this button can remove either, or
		// both, to thus reduce the content of m_markers to nil. If this is the case,
		// we must detect it here, cause m_markers to be emptied, and close the dialog
		wxString tempS;
		if (AllMkrsList.GetCount() == 0)
		{
			// the Remove button has reduced the content (filtered and anything else) to nothing, 
			// so update m_markers to empty and exit the dialog
			markersStr.Empty();
			goto a;
		}

		// get the text string from CEdit and save it in assocTextArrayAfterEdit 
		// since the user may edit the string there without clicking on Markers list. 
		// We have to save it here otherwise we've no other way to save it after any 
		// edits are made.
		// BEW addition 19Nov05, to prevent updating of a just-removed entry which is
		// no longer 'there'
		if (!bRemovalDone)
		{
			// bRemovalDone is only TRUE while the handler for the removal button is
			// in effect - and when the removal has removed the last filtered information
			// in the dialog, it would not be appropriate to keep it open, so the handler
			// calls OnBnClickedOk() to get things updated and the dialog closed down; but
			// the latter is not called if there is content remaining in the dialog.
			tempS = pMkrTextEdit->GetValue();
			indexIntoAllMkrSelection = markerLBIndexIntoAllMkrList.Item(currentMkrSelection);
			assocTextArrayAfterEdit[indexIntoAllMkrSelection] = tempS;
		}

		// reassemble the changed string to return to m_markers
		wxASSERT(AllMkrsList.GetCount() == AllMkrsFilteredFlags.GetCount());
		for (int index = 0; index < (int)AllMkrsList.GetCount(); index++)
		{
			tempStr = AllMkrsList.Item(index);
			if (AllMkrsFilteredFlags[index] == 0)
			{
				// not a filtered marker so just append it and associated text to markersStr
				if (!markersStr.IsEmpty())
				{
					if (markersStr[markersStr.Length() - 1] != _T(' '))
						markersStr += _T(' '); // add delimiting space only if needed
					markersStr += tempStr;
				}
				else
				{
					markersStr += tempStr;
				}
			}
			else
			{
				// It is a filtered marker so we need to rebuild the string by 
				// prefixing the assocTextArrayAfterEdit (edited part), with the whole marker
				// drawn out of AllWholeMkrsArray, and suffixing any end marker drawn out of
				// AllEndMkrsArray (it will have a single space when there is no end marker).
				// Finally we must add filtering bracket markers back to it.
				tempStr = AllWholeMkrsArray.Item(index);
				tempStr += _T(' ');
				tempStr += assocTextArrayAfterEdit.Item(index);
				tempStr += _T(' ');
				tempStr += AllEndMkrsArray.Item(index);
				tempStr.Trim(FALSE); // trim left end
				tempStr.Trim(TRUE); // trim right end
				markersStr.Trim(FALSE); // trim left end
				markersStr.Trim(TRUE); // trim right end
				if (!markersStr.IsEmpty())
				{
					markersStr += _T(' ');
				}
				markersStr += filterMkr;
				markersStr += _T(' ');
				markersStr += tempStr;
				markersStr += _T(' ');
				markersStr += filterMkrEnd;
				markersStr += _T(' '); // BEW added 17Nov05, m_markers should always end with a space
									   // if there is any other content
			}
		}
		// Locate the appropriate source phrase whose m_markers member was edited. 
		// Save the edited string back to its m_markers member.
		// The m_nSequNumber of the source phrase is stored in the View's m_nSequNumBeingViewed member
a:		CAdapt_ItView* pView = gpApp->GetView();
		CSourcePhrase* pSrcPhrase;
		pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nSequNumBeingViewed);
		pSrcPhrase->m_markers = markersStr;
		gpApp->m_nSequNumBeingViewed = -1;	// -1 can be used in the view to indicate if the 
											// ViewFilteredMaterialDlg dialog is active/inactive
	}
	// update the dialog
	pViewFilteredMaterialDlgSizer->Layout();
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CViewFilteredMaterialDlg::OnOK(wxCommandEvent& WXUNUSED(event)) 
{
	CAdapt_ItView* pView = gpApp->GetView();

	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	// If any edits were made, reassemble the edited markers text
	// before returning to the caller.
	wxString markersStr = _T("");
	wxString tempStr;
	if (changesMade)
	{
		// mark the document dirty
		pDoc->Modify(TRUE);

		// BEW modified 17Nov05 to handle the addition of the Remove Free Translation
		// (alternatively called Remove Back Translation, if \bt is selected) to the
		// dialog. If there is only a back translation or a free translation, or even just 
		// both, it is possible that the user by using this button can remove either, or
		// both, to thus reduce the content of m_markers to nil. If this is the case,
		// we must detect it here, cause m_markers to be emptied, and close the dialog
		wxString tempS;
		if (AllMkrsList.GetCount() == 0)
		{
			// the Remove button has reduced the content (filtered and anything else) to nothing, 
			// so update m_markers to empty and exit the dialog
			markersStr.Empty();
			goto a;
		}

		// get the text string from CEdit and save it in assocTextArrayAfterEdit 
		// since the user may edit the string there without clicking on Markers list. 
		// We have to save it here otherwise we've no other way to save it after any 
		// edits are made.
		// BEW addition 19Nov05, to prevent updating of a just-removed entry which is
		// no longer 'there'
		if (!bRemovalDone)
		{
			// bRemovalDone is only TRUE while the handler for the removal button is
			// in effect - and when the removal has removed the last filtered information
			// in the dialog, it would not be appropriate to keep it open, so the handler
			// calls OnBnClickedOk() to get things updated and the dialog closed down; but
			// the latter is not called if there is content remaining in the dialog.
			tempS = pMkrTextEdit->GetValue();
			indexIntoAllMkrSelection = markerLBIndexIntoAllMkrList.Item(currentMkrSelection);
			assocTextArrayAfterEdit[indexIntoAllMkrSelection] = tempS;
		}

		// reassemble the changed string to return to m_markers
		wxASSERT(AllMkrsList.GetCount() == AllMkrsFilteredFlags.GetCount());
		for (int index = 0; index < (int)AllMkrsList.GetCount(); index++)
		{
			tempStr = AllMkrsList.Item(index);
			if (AllMkrsFilteredFlags[index] == 0)
			{
				// not a filtered marker so just append it and associated text to markersStr
				if (!markersStr.IsEmpty())
				{
					if (markersStr[markersStr.Length() - 1] != _T(' '))
						markersStr += _T(' '); // add delimiting space only if needed
					markersStr += tempStr;
				}
				else
				{
					markersStr += tempStr;
				}
			}
			else
			{
				// It is a filtered marker so we need to rebuild the string by 
				// prefixing the assocTextArrayAfterEdit (edited part), with the whole marker
				// drawn out of AllWholeMkrsArray, and suffixing any end marker drawn out of
				// AllEndMkrsArray (it will have a single space when there is no end marker).
				// Finally we must add filtering bracket markers back to it.
				tempStr = AllWholeMkrsArray.Item(index);
				tempStr += _T(' ');
				tempStr += assocTextArrayAfterEdit.Item(index);
				tempStr += _T(' ');
				tempStr += AllEndMkrsArray.Item(index);
				tempStr.Trim(FALSE); // trim left end
				tempStr.Trim(TRUE); // trim right end
				markersStr.Trim(FALSE); // trim left end
				markersStr.Trim(TRUE); // trim right end
				if (!markersStr.IsEmpty())
				{
					markersStr += _T(' ');
				}
				markersStr += filterMkr;
				markersStr += _T(' ');
				markersStr += tempStr;
				markersStr += _T(' ');
				markersStr += filterMkrEnd;
				markersStr += _T(' '); // BEW added 17Nov05, m_markers should always end with a space
									   // if there is any other content
			}
		}
		// Locate the appropriate source phrase whose m_markers member was edited. 
		// Save the edited string back to its m_markers member.
		// The m_nSequNumber of the source phrase is stored in the View's m_nSequNumBeingViewed member
a:		CAdapt_ItView* pView = gpApp->GetView();
		CSourcePhrase* pSrcPhrase;
		pSrcPhrase = pView->GetSrcPhrase(gpApp->m_nSequNumBeingViewed);
		pSrcPhrase->m_markers = markersStr;
		gpApp->m_nSequNumBeingViewed = -1;	// -1 can be used in the view to indicate if the 
											// ViewFilteredMaterialDlg dialog is active/inactive
	}

	Destroy();
	// wx version: See note in OnCancel(). Here, however calling delete doesn't appear to be necessary.
	// No memory leaks detected in the wx version.
	//delete gpApp->m_pViewFilteredMaterialDlg; // BEW added 19Nov05, to prevent memory leak
	gpApp->m_pViewFilteredMaterialDlg = NULL;
	gpGreenWedgePile = NULL;
	pView->Invalidate();
	gpApp->m_pLayout->PlaceBox();
	
	//wxDialog::OnOK(event); // we are running modeless so don't call the base method
}


//The function either calls EndModal(wxID_CANCEL) if the dialog is modal, or sets the 
//return value to wxID_CANCEL and calls Show(false) if the dialog is modeless.
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

void CViewFilteredMaterialDlg::OnEnChangeEditMarkerText(wxCommandEvent& WXUNUSED(event))
{
	// This OnEnChangeEditMarkerText is called every time SetValue() is called which happens
	// anytime the user selects a marker from the list box, even though he makes no changes to
	// the associated text in the IDC_EDIT_MARKER_TEXT control. That is a difference between
	// the EVT_TEXT event macro design and MFC's ON_EN_TEXT macro design. Therefore we need
	// to add the enclosing if block with a IsModified() test to the wx version.
	if (pMkrTextEdit->IsModified())
	{
		changesMade = TRUE;
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
		SPList::Node* pos = gpApp->m_pSourcePhrases->Item(gpApp->m_nSequNumBeingViewed); //POSITION pos = gpApp->m_pSourcePhrases->FindIndex(pView->m_nSequNumBeingViewed);
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

	int nAllMkrsListIndex = markerLBIndexIntoAllMkrList.Item(currentMkrSelection);

	// first remove the beginning marker, its content, and end marker from the dialog
	// (if it doesn't have an end marker, there will be a space there - so we delete that instead)
	pMarkers->Delete(currentMkrSelection); //int nNewCount = pMarkers->DeleteString(currentMkrSelection);
	int nNewCount = pMarkers->GetCount(); // added since wxListBox::Delete doesn't return a new count value
	// whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the wxEVT_COMMAND_TEXT_UPDATED
	// event, which now deprecated SetValue() generates.
	pMkrTextEdit->ChangeValue(_T(""));
	pEndMarkers->Delete(currentMkrSelection);

	// now do the needed deletions in the various lists and arrays, so that the class's
	// storage reflects the deletion just made in the dialog; and update (ie. decrement by one)
	// the indices stored after the deletion index in the markerLBIndexIntoAllMkrList array.
	// We do the last operation first
	int nTotal = markerLBIndexIntoAllMkrList.GetCount();
	int nUpdateIndex = currentMkrSelection + 1;
	if (nUpdateIndex < nTotal - 1)
	{
		// we did not delete the last in the list, so there is at least one more index 
		// to be decremented
		int index;
		for (index = nUpdateIndex; index <= nTotal - 1; index++)
		{
			// get the stored index, decrement it, and resave it in the same place
			int nStoredIndex = markerLBIndexIntoAllMkrList.Item(index);
			nStoredIndex--;
			markerLBIndexIntoAllMkrList[index] = nStoredIndex;
		}
	}
	// now remove the index entry stored at currentMkrSelection
	markerLBIndexIntoAllMkrList.RemoveAt(currentMkrSelection,1);
	// do the same for bareMarkerArray (it stores only the filter markers)
	bareMarkerArray.RemoveAt(currentMkrSelection,1);

	// now remove the stored strings at the index value in nAllMkrsListIndex, from each
	// of the lists or arrays which store ALL marker information (whether filtered or not)
	AllMkrsList.RemoveAt(nAllMkrsListIndex,1);
	assocTextArrayBeforeEdit.RemoveAt(nAllMkrsListIndex,1);
	assocTextArrayAfterEdit.RemoveAt(nAllMkrsListIndex,1);
	AllMkrsFilteredFlags.RemoveAt(nAllMkrsListIndex,1);
	AllWholeMkrsArray.RemoveAt(nAllMkrsListIndex,1);
	AllEndMkrsArray.RemoveAt(nAllMkrsListIndex,1);

	// now take stock of what we've done. We may have just deleted the last, or the one
	// and only marker, in which case we don't want to enter any code which assumes the
	// storage arrays are non-empty, and ditto the AllMkrsList, otherwise we'd crash.
	// So if we have removed all filtered content, then the nNewCount value computed above
	// will be zero; but this is not a sufficient test in itself, because there may be stored
	// unfiltered markers to be dealt with - in which case we DO want to continue normal
	// finalizing processing; also we don't want the user to sit there looking at an
	// empty dialog and not being sure what to do next, so we help him out by closing things
	// down for him.
	changesMade = TRUE; // ensure the reconstruction gets done
	if (nNewCount == 0)
	{
		// all filtered information has just been removed, so we'll close the dialog down
		// -- this is most easily accomplished by calling OnBnClickedOk() here -- the latter
		// checks if AllMkrsList is also empty and if it is, it just updates the sourcephrase
		// with an empty m_markers member and closes off the dialog; if not, it reconstructs
		// the non-filtered information which remains, putting it in m_markers, and updates
		// the sourcephrase with that instead
		//OnBnClickedOk(); // BEW commented out 29Apr06, to use UpdateContentOnRemove() instead
						   // because the internal call to Destroy() was resulting in a later crash
						   // when the view was updated
		UpdateContentOnRemove();
		bRemovalDone = FALSE;
		changesMade = FALSE;
		// don't call OnBnClickedOk() here, else the view update will crash the app; make user click OK button 
	}
	else
	{
		// the dialog has filtered content remaining, so we need to update it; we'll put the
		// selection at index zero, because this is always safe
		currentMkrSelection = 0;
		indexIntoMarkersLB = 0;
		prevMkrSelection = 0;
		indexIntoAllMkrSelection = markerLBIndexIntoAllMkrList.Item(indexIntoMarkersLB);

		// change the text in the CEdit to correspond to the marker chosen by the
		// indexIntoAllMkrSelection value
		wxString tempStr = assocTextArrayAfterEdit.Item(indexIntoAllMkrSelection);
		// whm changed 1Apr09 SetValue() to ChangeValue() below so that is doesn't generate the wxEVT_COMMAND_TEXT_UPDATED
		// event, which now deprecated SetValue() generates.
		pMkrTextEdit->ChangeValue(tempStr);

		// look up the marker description and place it in the static text
		GetAndShowMarkerDescription(indexIntoAllMkrSelection);

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
}

// takes the list of markers in the first list box in the dialog, and nSelection is the index
// of which marker string is being considered, and works out which, if any, of the two
// bool parameters needs to be set. It does this by looking at the marker at nSelection, if it
// is \free, then bCanRemoveFT is set to TRUE and bCanRemoveBT is cleared to FALSE; if it is
// \bt or a longer marker beginning with \bt, then bCanRemoveBT is set TRUE, and bCanRemoveFT is
// cleared to FALSE; if it is neither marker, then both flags are cleared to FALSE (and in the
// interface both being FALSE results in the button being hidden).
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
		bCanRemoveBT = TRUE;
	}
	return;
}

// GetAndShowMarkerDescription takes code formerly in OnInitDialog(), which shows the marker
// description to the user; we need this in more than one place, so more efficient this way
void CViewFilteredMaterialDlg::GetAndShowMarkerDescription(int indexIntoAllMkrSelection)
{
	wxString tempStr;
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	USFMAnalysis* pUsfmAnalysis = NULL;
	wxString newMkr, wholeMkr, statusStr;
	newMkr = AllWholeMkrsArray.Item(indexIntoAllMkrSelection);
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

