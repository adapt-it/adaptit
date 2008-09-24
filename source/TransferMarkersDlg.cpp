/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			TransferMarkersDlg.cpp
/// \author			Bill Martin
/// \date_created	13 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CTransferMarkersDlg class. 
/// The CTransferMarkersDlg class provides a secondary dialog to the CEditSourceTextDlg 
/// that gets called to enable the user to adjust the position of markers within a
/// span of source text being edited.
/// \derivation		The CTransferMarkersDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in TransferMarkersDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TransferMarkersDlg.h"
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
#include "TransferMarkersDlg.h"
#include "Adapt_ItDoc.h"
#include "SourcePhrase.h"
#include "helpers.h"

extern const wxChar* filterMkr;

/// This global is defined in Adapt_It.cpp.
extern wxChar	gSFescapechar;

extern SPList* gpOldSrcPhraseList;
extern SPList* gpNewSrcPhraseList;
extern int gnCount;    // count of old srcphrases (user selected these) after unmerges, etc
extern int gnNewCount; // count of the new srcphrases (after user finished editing the source text)
int		   gnOldItem = -1; // index of the earlier selected item in the m_comboNewWords combobox
						  // (used for detecting when we change items, in case user selects the 
						  // same one - in which case certain updates are not wanted, but would
						  // be for a changed item)

/// This global is TRUE if the TextType needs to be propagated to sourcephrase instances 
/// following the new sublist, after all housekeeping is done & propagation is done in 
/// OnEditSourceText().
bool gbPropagationNeeded; 

/// Indicates the TextType to be propagated when the gbPropagationNeeded global is TRUE
TextType gPropagationType;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

extern const wxChar* filterMkr;

// event handler table
BEGIN_EVENT_TABLE(CTransferMarkersDlg, AIModalDialog)
	EVT_INIT_DIALOG(CTransferMarkersDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CTransferMarkersDlg::OnOK)
	
	EVT_BUTTON(IDC_BUTTON_TRANSFER, CTransferMarkersDlg::OnButtonTransfer)
	EVT_BUTTON(IDC_BUTTON_UPDATE_MARKERS, CTransferMarkersDlg::OnButtonUpdateMarkers)
	EVT_BUTTON(IDC_BUTTON_REMOVE_MARKERS, CTransferMarkersDlg::OnButtonRemoveMarkers)
	EVT_LISTBOX(IDC_LIST_OLD_WORDS, CTransferMarkersDlg::OnSelchangeListOldWordsWithSfm)
	EVT_LISTBOX(IDC_LIST_OLD_SF_MARKERS, CTransferMarkersDlg::OnSelchangeListSfMarkers)
	EVT_LISTBOX(IDC_LIST_NEW_WORDS, CTransferMarkersDlg::OnSelchangeListNewWords)
END_EVENT_TABLE()


CTransferMarkersDlg::CTransferMarkersDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Transfer (Or Change) Standard Format Marker Assignments"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{

	m_strOldSourceText = _T("");
	m_strNewSourceText = _T("");
	m_strNewMarkers = _T("");

	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	TransferMarkersDlgFunc(this, TRUE, TRUE);
	// The declaration is: TransferMarkersDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	wxColour sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);

	// associate pointers and validators with newly created dialog's text controls
	m_pEditAttachedMarkers = (wxTextCtrl*)FindWindowById(IDC_EDIT_ATTACHED_MARKERS);
	m_pEditAttachedMarkers->SetValidator(wxGenericValidator(&m_strNewMarkers));

	m_pEditNewWords = (wxTextCtrl*)FindWindowById(IDC_EDIT_NEW_SOURCE);
	m_pEditNewWords->SetValidator(wxGenericValidator(&m_strNewSourceText));
	m_pEditNewWords->SetBackgroundColour(sysColorBtnFace); // read only background color

	m_pEditOldWords = (wxTextCtrl*)FindWindowById(IDC_EDIT_OLD_SOURCE);
	m_pEditOldWords->SetValidator(wxGenericValidator(&m_strOldSourceText));
	m_pEditOldWords->SetBackgroundColour(sysColorBtnFace); // read only background color

	// the buttons don't need validators
	m_pButtonTransfer = (wxButton*)FindWindowById(IDC_BUTTON_TRANSFER);
	m_pButtonUpdateMkrs = (wxButton*)FindWindowById(IDC_BUTTON_UPDATE_MARKERS);
	m_pButtonRemoveMkrs = (wxButton*)FindWindowById(IDC_BUTTON_REMOVE_MARKERS);
	//m_pButtonAddMkrs = (wxButton*)FindWindowById(IDC_BUTTON_ADD_MARKERS);

	// the comboboxes don't need validators
	m_pListOldWords = (wxListBox*)FindWindowById(IDC_LIST_OLD_WORDS);
	m_pListMarkers = (wxListBox*)FindWindowById(IDC_LIST_OLD_SF_MARKERS);
	m_pListNewWords = (wxListBox*)FindWindowById(IDC_LIST_NEW_WORDS);

	wxColor backgrndColor = this->GetBackgroundColour();
	// the following text controls function as static text on the dialog
	pTextCtrlAsStaticTransfMkrs1 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_1);
	wxASSERT(pTextCtrlAsStaticTransfMkrs1 != NULL);
	pTextCtrlAsStaticTransfMkrs1->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs2 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_2);
	wxASSERT(pTextCtrlAsStaticTransfMkrs2 != NULL);
	pTextCtrlAsStaticTransfMkrs2->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs3 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_3);
	wxASSERT(pTextCtrlAsStaticTransfMkrs3 != NULL);
	pTextCtrlAsStaticTransfMkrs3->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs4 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_4);
	wxASSERT(pTextCtrlAsStaticTransfMkrs4 != NULL);
	pTextCtrlAsStaticTransfMkrs4->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs5 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_5);
	wxASSERT(pTextCtrlAsStaticTransfMkrs5 != NULL);
	pTextCtrlAsStaticTransfMkrs5->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs6 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_6);
	wxASSERT(pTextCtrlAsStaticTransfMkrs6 != NULL);
	pTextCtrlAsStaticTransfMkrs6->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs7 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_7);
	wxASSERT(pTextCtrlAsStaticTransfMkrs7 != NULL);
	pTextCtrlAsStaticTransfMkrs7->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs8 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_8);
	wxASSERT(pTextCtrlAsStaticTransfMkrs8 != NULL);
	pTextCtrlAsStaticTransfMkrs8->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs9 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_9);
	wxASSERT(pTextCtrlAsStaticTransfMkrs9 != NULL);
	pTextCtrlAsStaticTransfMkrs9->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs10 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_10);
	wxASSERT(pTextCtrlAsStaticTransfMkrs10 != NULL);
	pTextCtrlAsStaticTransfMkrs10->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs11 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_11);
	wxASSERT(pTextCtrlAsStaticTransfMkrs11 != NULL);
	pTextCtrlAsStaticTransfMkrs11->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs12 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_12);
	wxASSERT(pTextCtrlAsStaticTransfMkrs12 != NULL);
	pTextCtrlAsStaticTransfMkrs12->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs13 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_13);
	wxASSERT(pTextCtrlAsStaticTransfMkrs13 != NULL);
	pTextCtrlAsStaticTransfMkrs13->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs14 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_14);
	wxASSERT(pTextCtrlAsStaticTransfMkrs14 != NULL);
	pTextCtrlAsStaticTransfMkrs14->SetBackgroundColour(backgrndColor);
	pTextCtrlAsStaticTransfMkrs15 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_TRANSFER_MARKERS_15);
	wxASSERT(pTextCtrlAsStaticTransfMkrs15 != NULL);
	pTextCtrlAsStaticTransfMkrs15->SetBackgroundColour(backgrndColor);
}

CTransferMarkersDlg::~CTransferMarkersDlg() // destructor
{
	
}

void CTransferMarkersDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	CAdapt_ItDoc* pDoc = pApp->GetDocument();
	gnOldItem = -1;

	// set up the dialog's controls based on the source font
	CopyFontBaseProperties(pApp->m_pSourceFont,pApp->m_pDlgSrcFont);
	// use the preferred dialog font size
	pApp->m_pDlgSrcFont->SetPointSize(pApp->m_dialogFontSize);
	m_pEditOldWords->SetFont(*pApp->m_pDlgSrcFont);
	m_pEditNewWords->SetFont(*pApp->m_pDlgSrcFont);
	m_pEditAttachedMarkers->SetFont(*pApp->m_pDlgSrcFont);
	m_pListOldWords->SetFont(*pApp->m_pDlgSrcFont);
	m_pListMarkers->SetFont(*pApp->m_pDlgSrcFont);
	m_pListNewWords->SetFont(*pApp->m_pDlgSrcFont);
	m_pButtonTransfer->SetFont(*pApp->m_pDlgSrcFont);

	m_pListOldWords->Clear();
	m_pListMarkers->Clear();
	m_pListNewWords->Clear();
	
	// whm added the following to track selection info separately from client data
	m_oldWordsSelectionBegin.Clear();
	m_oldWordsSelectionEnd.Clear();
	m_newWordsSelectionBegin.Clear();
	m_newWordsSelectionEnd.Clear();


#ifdef _RTL_FLAGS
	if (gpApp->m_bSrcRTL)
	{
		m_pEditOldWords->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditOldWords->SetLayoutDirection(wxLayout_LeftToRight);
	}

	if (gpApp->m_bSrcRTL)
	{
		m_pEditNewWords->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pEditNewWords->SetLayoutDirection(wxLayout_LeftToRight);
	}


	// m_pButtonTransfer this one is the Button, which mixes English text and possibly RTL text
	// so I'm not sure what the setting should be for RTL, I'll assume right
	// alignment and RTL reading order is correct, and hope it can handle
	// bidirectional text correctly
	if (gpApp->m_bSrcRTL)
	{
		m_pButtonTransfer->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pButtonTransfer->SetLayoutDirection(wxLayout_LeftToRight);
	}

	if (gpApp->m_bSrcRTL)
	{
		m_pListOldWords->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pListOldWords->SetLayoutDirection(wxLayout_LeftToRight);
	}

	if (gpApp->m_bSrcRTL)
	{
		m_pListMarkers->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pListMarkers->SetLayoutDirection(wxLayout_LeftToRight);
	}

	if (gpApp->m_bSrcRTL)
	{
		m_pListNewWords->SetLayoutDirection(wxLayout_RightToLeft);
	}
	else
	{
		m_pListNewWords->SetLayoutDirection(wxLayout_LeftToRight);
	}
#endif

	// set up the text for the old source text editbox; and if there are sfmarkers on any of
	// the CSourcePhrase instances, put the word and its preceding markers in the list boxes
	SPList::Node* pos = 0; //POSITION pos = 0;
	SPList* pList = gpOldSrcPhraseList;
	wxASSERT(pList->GetCount() > 0);
	pos = pList->GetFirst();
	wxUint16 nCharCount = 0;
	wxUint16 nWordLength = 0;
	wxUint16 nMarkersLength = 0;
	int nWordsListOffset = -1;
	int index = -1;
	while (pos != 0)
	{
		index++; // index for use in m_snList, to be able to recover the pSrcPhrase value for m_comboOldWords
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		nMarkersLength = pSrcPhrase->m_markers.Length();
		nWordLength = pSrcPhrase->m_srcPhrase.Length();

		// check for markers, and handle them if present
		wxString markersWithPlaceHolders;
		bool bHasMarkers = FALSE;
		if (!pSrcPhrase->m_markers.IsEmpty())
		{
			// add the source word to the first combo box, and the markers to the second combo box
			nWordsListOffset = m_pListOldWords->Append(pSrcPhrase->m_srcPhrase);
			// For m_comboMarkers list items we could reduce filter marked material to place
			// holders, but for Windows, at least, we have a horizontal scroll and, for other
			// platforms one should be able to see the marker and a little bit of text beyond
			// the \~FILTER marker.
			// parse markers gathering all marker strings it contains into our wxArrayString AllMkrsList.
			// AllMkrsList will contain only one marker-text-endmarker segment per stored string, 
			// including filtered material bracketed by \~FILTER...\~FILTER*.
			wxArrayString AllMkrsList;
			AllMkrsList.Clear();
			wxString markers = pSrcPhrase->m_markers;
			pDoc->GetMarkersAndTextFromString(&AllMkrsList, markers);
			int ct;
			for (ct = 0; ct < (int)AllMkrsList.GetCount(); ct++)
			{
				wxString inspectionStr = AllMkrsList.Item(ct);
				// whm 11Aug06 we no longer use [~] placeholder for filtered material because
				// we're using a listbox with a horizontal scroll bar (in Windows), and in any
				// case enough of the string is visible to get an idea of what's there.
				m_pListMarkers->Append(inspectionStr);
				m_snList.Add(index); // need this for pSrcPhrase recovery
			}
			bHasMarkers = TRUE;

		}
		else
			bHasMarkers = FALSE;

		if (m_strOldSourceText.IsEmpty())
		{
			if (bHasMarkers)
			{
				m_strOldSourceText = pSrcPhrase->m_markers + pSrcPhrase->m_srcPhrase + _T(" ");
			}
			else
			{
				m_strOldSourceText = pSrcPhrase->m_srcPhrase + _T(" ");
			}
		}
		else
		{
			if(bHasMarkers)
			{
				m_strOldSourceText += pSrcPhrase->m_markers + pSrcPhrase->m_srcPhrase + _T(" ");
			}
			else
			{
				m_strOldSourceText += pSrcPhrase->m_srcPhrase + _T(" "); // don't care about a final space
			}
		}

		// get the offset to the word, and its length, stored in the data attribute of the combobox item
		nCharCount += nMarkersLength; // the word starts next
		if (bHasMarkers)
		{
			m_oldWordsSelectionBegin.Add(nCharCount);
			m_oldWordsSelectionEnd.Add(nCharCount+nWordLength);
			// whm comment: the client data of m_pListOldWords stores the begin and end selection offset
			// info of the word as found in the m_pEditOldWords edit box.
		}
		nCharCount += nWordLength + 1; // add on the word length and space, to be ready for next iteration
	}
	TransferDataToWindow(); // make the string available for selection operation

	// show the first item in the oldwords combobox's list, and get the markers combobox synchonized to it
	if (m_pListOldWords->GetCount() > 0)
	{
		m_pListOldWords->SetSelection(0); // calls wxControlWithItems::SetSelection(int n)

		// select the initial word in the old source text edit box too & scroll it into view if necessary
		wxString editOldWordsStr;
		editOldWordsStr = m_pEditOldWords->GetValue();
		int nBegin = m_oldWordsSelectionBegin.Item(0);
		int nEnd = m_oldWordsSelectionEnd.Item(0);

		wxASSERT(editOldWordsStr.Length() >= (size_t)nBegin && editOldWordsStr.Length() >= (size_t)nEnd); // whm added
		m_pEditOldWords->SetSelection(nBegin,nEnd);
		// TODO: The following inspectSel shows that the word is selected within the old words edit box, 
		// so why doesn't it show as selected when dialog is shown? Nothing else in the code here appears
		// to kill the selection after this point. Testing shows that being read-only and having a gray 
		// background is not the cause of the selection not showing.
		wxString inspectSel = m_pEditOldWords->GetStringSelection();
	}
	else // whm 16Aug06 added else block to disable the Transfer To button if m_pListOldWords is empty
	{
		m_pButtonTransfer->Enable(FALSE);
	}

	if (m_pListMarkers->GetCount() > 0)
	{
		m_pListMarkers->SetSelection(0);
	}
	// whm note: the selection process does not need the data updated

	// set up the text for the new source text editbox; there are no sfmarkers in it as yet
	pos = 0;
	SPList* pNewList = gpNewSrcPhraseList;
	wxASSERT(pList->GetCount() > 0);
	pos = pNewList->GetFirst();
	wxUint32 nNewCharCount = 0;
	wxUint32 nNewWordLength = 0;
	wxUint32 nNewMarkersLength = 0;
	int nNewWordsListOffset = -1;
	while (pos != 0)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext();
		nNewMarkersLength = 0;
		nNewWordLength = pSrcPhrase->m_srcPhrase.Length();

		// add the new source word to the first combo box on the right
		nNewWordsListOffset = m_pListNewWords->Append(pSrcPhrase->m_srcPhrase);

		if (m_strNewSourceText.IsEmpty())
		{
			m_strNewSourceText = pSrcPhrase->m_srcPhrase + _T(" ");
		}
		else
		{
			m_strNewSourceText += pSrcPhrase->m_srcPhrase + _T(" "); // don't care about a final space
		}

		m_newWordsSelectionBegin.Add(nNewCharCount);
		m_newWordsSelectionEnd.Add(nNewCharCount+nNewWordLength);
		// get the offset to the word, and its length, stored in the data attribute of the combobox item
		// (all the new words must be put in the combobox, since we don't know which ones will end up having
		// a marker or marker string assigned to them.
		nNewCharCount += nNewWordLength + 1; // add on the word length and space, to be ready for next iteration
	}
	TransferDataToWindow(); // make the string available

	// show the first item in the combobox's list
	if (m_pListNewWords->GetCount() > 0)
	{
		m_pListNewWords->SetSelection(0); //SetCurSel

		// select the initial word in the new source text edit box too & scroll it into view if necessary
		wxString editNewWordsStr;
		// The following needs to use GetString(0) rather than GetStringSelection()
		editNewWordsStr = m_pEditNewWords->GetValue();
		int nBegin = m_newWordsSelectionBegin.Item(0);
		int nEnd = m_newWordsSelectionEnd.Item(0);

		wxASSERT(editNewWordsStr.Length() >= (size_t)nBegin && editNewWordsStr.Length() >= (size_t)nEnd); // whm added
		m_pEditNewWords->SetSelection(nBegin,nEnd);
	}
	// whm note: the selection process does not need the data updated

	// set up transfer button's text	
	wxString s, buttonText;
	s = m_pListNewWords->GetStringSelection();
	//IDS_TRANSFER_BTN
	buttonText = buttonText.Format(_("4. Transfer To: %s"),s.c_str());
	m_pButtonTransfer->SetLabel(buttonText);

	// the only data change was done in SetLabel above

	// set the focus to the new word's marker list edit box
	m_pEditAttachedMarkers->SetFocus();
	m_pEditAttachedMarkers->SetSelection(-1,-1); // -1,-1 selects all
}

// wx version we'll draw the lines in the dialog resources, so don't need OnPaint() to do so.

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CTransferMarkersDlg::OnOK(wxCommandEvent& event) 
{
	// before returning to the caller, analyse all the new sourcephrase instances' markers, and
	// set up the appropriate runs of TextType, handle chapter insertions, verse insertions, and so
	// forth
	// whm comment: DoMarkerHousekeeping does not account for filtering changes which if occurring
	// will have to have already been done by custom code for that operation elsewhere. 
	// DoMarkerHousekeeping() can only do the final cleanup of the navigation text and text 
	// colouring and (cryptic) TextType assignments.
	gpApp->GetDocument()->DoMarkerHousekeeping(gpNewSrcPhraseList,gnNewCount,gPropagationType,gbPropagationNeeded);	
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods
void CTransferMarkersDlg::OnButtonUpdateMarkers(wxCommandEvent& event) 
{
	OnButtonAddMarkers(event);
}

void CTransferMarkersDlg::OnButtonRemoveMarkers(wxCommandEvent& WXUNUSED(event)) 
{
	// get the To sourcephrase instance
	CSourcePhrase* pToSrcPhrase = NULL; // whm initialized to NULL
	SPList::Node* pos;
	int nItem;

	// get the item index
	nItem = m_pListNewWords->GetSelection(); // nItem indexes directly into the global list for the scrPhrase
	if (nItem != -1)
	{
		// get pointer to the associated sourcephrase instance
		pos = gpNewSrcPhraseList->Item(nItem);
		pToSrcPhrase = (CSourcePhrase*)pos->GetData();

#ifdef _DEBUG
		// check we have the right sourcephrase
		wxString s;
		s = m_pListNewWords->GetStringSelection();
		wxASSERT(pToSrcPhrase->m_srcPhrase == s);
#endif
	}

	wxASSERT(pToSrcPhrase != NULL); // whm added
	// clear the contents of the attributes involved in transferral, for this source phrase
	ClearForTransfer(pToSrcPhrase);

	// show the marker text cleared in the m_editAttachedMarkers editbox
	m_strNewMarkers.Empty();
	TransferDataToWindow();
	m_pEditAttachedMarkers->SetFocus();
	m_pEditAttachedMarkers->SetSelection(0,0); // sets caret at the beginning of the edit box text following MFC

	// show the markers preceding the selected new source word in the m_editNewWords CEdit box
	UpdateNewSourceTextAndCombo(gpNewSrcPhraseList,nItem);

	TransferDataToWindow();
}

// BEW modified 15Jul05 to better support editing the m_markers string when it could be a rich string
// containing filtered markers, filter brack markers, and filtered text - in particular, we want to be
// able to edit marker typos because it was misspelled or a delimiting space omitted. Because there can be
// several markers it is no longer appropriate to assume there is just one marker in m_markers, and so
// the editing could be done anywhere - so what few tests we continue to allow will have to be pretty 
// general ones
void CTransferMarkersDlg::OnButtonAddMarkers(wxCommandEvent& WXUNUSED(event)) 
{
	// get the To sourcephrase instance
	CSourcePhrase* pToSrcPhrase = NULL; // whm initialized to NULL
	SPList::Node* pos; 
	int nItem;

	// get the item index
	nItem = m_pListNewWords->GetSelection(); // nItem indexes directly into the global list for the scrPhrase
	if (nItem != -1)
	{
		// get pointer to the associated sourcephrase instance
		pos = gpNewSrcPhraseList->Item(nItem);
		pToSrcPhrase = (CSourcePhrase*)pos->GetData();

#ifdef _DEBUG
		// check we have the right sourcephrase
		wxString s;
		s = m_pListNewWords->GetStringSelection();
		wxASSERT(pToSrcPhrase->m_srcPhrase == s);
#endif
	}

	// check there is a marker in the editbox string, don't add the string to the sourcephrase
	// unless there is
	TransferDataFromWindow(); // ensure m_strNewMarkers is up to date

	// whm 16Aug06 added test below to prevent user from trying to add \~FILTER ... \~FILTER* 
	// (filtered material) using the facilities of the TransferMarkersDlg.
	if (m_strNewMarkers.Find(filterMkr) != -1)
	{
		wxMessageBox(_("Markers and their content, in filtered format, cannot be added to the source text from this dialog."),_T(""), wxICON_EXCLAMATION);
		m_strNewMarkers.Empty();
		TransferDataToWindow();
		m_pEditAttachedMarkers->SetFocus();
		// we emptied the edit box so there is nothing to select
		return;
	}

	TransferDataFromWindow(); // ensure m_strNewMarkers is up to date
	int len = m_strNewMarkers.Length();
	int nOffset = m_strNewMarkers.Find(gSFescapechar);
	if (nOffset == -1)
	{
		// no backslash found
		//IDS_NO_MARKER_PRESENT
a:		wxMessageBox(_("Warning: no standard format marker was found. The operation was aborted."),_T(""), wxICON_EXCLAMATION);
		TransferDataToWindow();
		m_pEditAttachedMarkers->SetFocus();
		m_pEditAttachedMarkers->SetSelection(len,len);
		return;
	}
	else
	{
		// backslash or user defined sfm escape char found, but check it is not followed by 
		// space nor at end of string
		if (nOffset == len - 1)
			goto a; // \ is last char in the string, so not a valid marker
		if (m_strNewMarkers.GetChar(nOffset + 1) == _T('~'))
			goto b; // its either a \~FILTER or \~FILTER* marker, so legal
		if (!wxIsalpha(m_strNewMarkers.GetChar(nOffset+1)))
			goto a; // not an alphabetic character following the backslash, so not a valid marker
	}

	wxASSERT(pToSrcPhrase != NULL); // whm added
	// clear the contents of the attributes involved in transferral, for this source phrase
b:	ClearForTransfer(pToSrcPhrase);

	// get the marker text from the m_editAttachedMarkers editbox, check it ends with space, then
	// add to the sourcephrase instance
	wxChar ch = m_strNewMarkers.GetChar(len-1); // get last char
	if (!(ch == _T(' ')))
		m_strNewMarkers += _T(' '); // add space at end, if one is not already present
	TransferDataToWindow();
	len = m_strNewMarkers.Length(); // update, in case it was changed
	m_pEditAttachedMarkers->SetFocus();
	m_pEditAttachedMarkers->SetSelection(len,len);

	// add the marker to the sourcephrase instance
	pToSrcPhrase->m_markers = m_strNewMarkers;

	// show the markers preceding the selected new source word in the m_editNewWords CEdit box,
	// and update the m_comboNewWords list (the data stored there, in particular) accordingly
	UpdateNewSourceTextAndCombo(gpNewSrcPhraseList,nItem);

	TransferDataToWindow();
}

void CTransferMarkersDlg::OnSelchangeListOldWordsWithSfm(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (m_pListOldWords->GetCount() == 0)
		return;

	// make the markers combo display this word's marker list
	int nItem = m_pListOldWords->GetSelection();
	if (nItem != -1) // CB_ERR
	{
		// select the corresponding item on the markers combobox
		//m_pListMarkers->SetSelection(nItem); // whm No, they don't correspond

		wxString editOldWordsStr;
		editOldWordsStr = m_pEditOldWords->GetValue();
		// select the word in the old source text edit box too & scroll it into view if necessary
		int nBegin = m_newWordsSelectionBegin.Item(nItem);
		int nEnd = m_newWordsSelectionEnd.Item(nItem);

		wxASSERT(editOldWordsStr.Length() >= (size_t)nBegin && editOldWordsStr.Length() >= (size_t)nEnd); // whm added
		m_pEditOldWords->SetSelection(nBegin,nEnd);
	}
}

void CTransferMarkersDlg::OnSelchangeListSfMarkers(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (m_pListOldWords->GetCount() == 0)
		return;

	// make the old words combo display this marker string's old source word
	int nItem = m_pListOldWords->GetSelection(); // MFC had m_comboMarkers.GetCurSel();
	if (nItem != -1)
	{
		//m_pListOldWords->SetSelection(nItem); // no, they don't correspond; nItem indexes m_pListMarkers's
												// selection, not m_pListOldWords

		wxString editOldWordsStr;
		editOldWordsStr = m_pEditOldWords->GetValue();
		// select the word in the old source text edit box too & scroll it into view if necessary
		int nBegin = m_oldWordsSelectionBegin.Item(nItem);
		int nEnd = m_oldWordsSelectionEnd.Item(nItem);

		wxASSERT(editOldWordsStr.Length() >= (size_t)nBegin && editOldWordsStr.Length() >= (size_t)nEnd); // whm added
		m_pEditOldWords->SetSelection(nBegin,nEnd);

	}
}

void CTransferMarkersDlg::OnSelchangeListNewWords(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (m_pListNewWords->GetCount() == 0)
		return;

	// make the new source text edit box display the chosen word selected; and update Transfer button
	int nItem = m_pListNewWords->GetSelection();
	if (nItem == gnOldItem)
	{
		// no change, so do nothing
		return;
	}
	if (nItem != -1)
	{
		gnOldItem = nItem; // update, for next time

		wxString editNewWordsStr;
		editNewWordsStr = m_pEditNewWords->GetValue();
		int nBegin = m_newWordsSelectionBegin.Item(nItem);
		int nEnd = m_newWordsSelectionEnd.Item(nItem);

		wxASSERT(editNewWordsStr.Length() >= (size_t)nBegin && editNewWordsStr.Length() >= (size_t)nEnd); // whm added
		m_pEditNewWords->SetSelection(nBegin,nEnd); 
		TransferDataToWindow(); // get it updated, so the next bit of code has the updated word to work with

		// make the Transfer to: button's text agree with the new choice
		wxString s,buttonText;
		s = m_pListNewWords->GetStringSelection();
		// IDS_TRANSFER_BTN
		buttonText = buttonText.Format(_("4. Transfer To: %s"),s.c_str());
		m_pButtonTransfer->SetLabel(buttonText);

		// update the m_editAttachedMarkers editbox contents, to show attached markers (if any)
		SPList::Node* pos = gpNewSrcPhraseList->Item(nItem);
		wxASSERT(pos != 0);
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		wxASSERT(pSrcPhrase != NULL);
		m_strNewMarkers = pSrcPhrase->m_markers;
		int len = m_strNewMarkers.Length();
		TransferDataToWindow();
		m_pEditAttachedMarkers->SetFocus();
		m_pEditAttachedMarkers->SetSelection(len,len);
	}

	TransferDataToWindow();
}

void CTransferMarkersDlg::OnButtonTransfer(wxCommandEvent& WXUNUSED(event)) 
{
	// get the To and From sourcephrase instances
	CSourcePhrase* pToSrcPhrase = (CSourcePhrase*)NULL;
	CSourcePhrase* pFromSrcPhrase = (CSourcePhrase*)NULL;
	SPList::Node* pos;
	int nItem = m_pListOldWords->GetSelection(); // nItem also indexes into m_snList, to get the index
											 // into the gpOldSrcPhraseList for the needed pSrcPhrase
	if (nItem != -1) // CB_ERR
	{
		int index = m_snList.Item(nItem);
		pos = gpOldSrcPhraseList->Item(index);
		pFromSrcPhrase = (CSourcePhrase*)pos->GetData();

#ifdef _DEBUG
		// check we have the right sourcephrase
		wxString s;
		s = m_pListOldWords->GetStringSelection();
		wxASSERT(pFromSrcPhrase->m_srcPhrase == s);
#endif
	}
	nItem = m_pListNewWords->GetSelection(); // nItem also indexes directly into the global list for the scrPhrase
	if (nItem != -1)
	{
		pos = gpNewSrcPhraseList->Item(nItem);
		pToSrcPhrase = (CSourcePhrase*)pos->GetData();

#ifdef _DEBUG
		// check we have the right sourcephrase
		wxString s;
		s = m_pListNewWords->GetStringSelection();
		wxASSERT(pToSrcPhrase->m_srcPhrase == s);
#endif
	}

	// whm added test below to prevent crash in functions below if pToSrcPhrase or pFromSrcPhrase
	// is not initialized to a valid value in one of the blocks above. I also added code to
	// OnInitDialog to disable the "Transfer To..." button if the old words list is empty, so
	// the following code isn't strictly necessary, but a safety test in case the code in
	// OnInitDialog were changed.
	if (pToSrcPhrase == NULL || pFromSrcPhrase == NULL)
	{
		::wxBell();
		return;
	}

	// clear the contents of the attributes involved in the transfer process, for the new sourcephrase
	ClearForTransfer(pToSrcPhrase); // makes pToSrcPhrase->m_markers empty

	// do the transfer
	// wx revision: In the MFC app, the SimpleCopyTransfer call copied m_markers 
	// from pFromSrcPhrase to pToSrcPhrase, but I've modified SimpleCopyTransfer so that
	// it only copies m_markers over if m_markers containes no filtered material; and
	// now returns FALSE if there was filtered material (it couldn't really do a "simple copy".
	if (!SimpleCopyTransfer(pFromSrcPhrase,pToSrcPhrase))
	{
		// wx Note: SimpleCopyTransfer should never return false and this block should
		// never be entered, since the dialog is not called if filtered material is present
		// in the source phrase selection being edited.
		//
		// We now deal with the filtered material here.
		// the pFromSrcPhrase has filtered info within its m_markers member so,  
		// determine if the filtered material is to be transferred to the same source 
		// phrase it was on previously. If so, we can allow it to be done. If not, until 
		// we can insure the successful transfer of filtered material to a different 
		// source phrase, we'll issue a message saying it is not possible, we stop the 
		// transfer and return.
		wxString fromInpsectionStr = pFromSrcPhrase->m_srcPhrase;
		wxString toInspectionStr = pToSrcPhrase->m_srcPhrase;
		wxString selectedMarker = m_pListMarkers->GetStringSelection();
		if (selectedMarker.Find(filterMkr) != -1 && pFromSrcPhrase->m_srcPhrase != pToSrcPhrase->m_srcPhrase)
		{
			wxMessageBox(_T("It is not possible in this version to transfer filtered information to a different or edited word while editing the source text."),_T(""),wxICON_INFORMATION);
			return;
		}
		// add the filter item to pToSrcPhrase's m_markers if it is not already there
		int sel = m_pListMarkers->GetSelection();
		wxString selectedMkrStr = m_pListMarkers->GetString(sel);
		if (m_strNewMarkers.Find(selectedMkrStr) == -1)
		{
			m_strNewMarkers.Trim(FALSE);// trim left end
			m_strNewMarkers.Trim(TRUE); // trim right end
			if (!m_strNewMarkers.IsEmpty())
				m_strNewMarkers = m_strNewMarkers + _T(' ') + selectedMkrStr + _T(' ');
			else
				m_strNewMarkers = selectedMkrStr + _T(' ');
			pToSrcPhrase->m_markers = m_strNewMarkers;
		}
	}

	// show the marker text that was copied, in the m_editAttachedMarkers editbox
	m_strNewMarkers = pToSrcPhrase->m_markers;
	int len = m_strNewMarkers.Length();
	TransferDataToWindow();
	m_pEditAttachedMarkers->SetFocus();
	m_pEditAttachedMarkers->SetSelection(len,len);

	// show the markers preceding the selected new source word in the m_editNewWords CEdit box
	// and update the combobox contents accordingly
	UpdateNewSourceTextAndCombo(gpNewSrcPhraseList,nItem);

	TransferDataToWindow();
}

// *****************************************************************************************************
//
// helper functions
//
// *****************************************************************************************************

void CTransferMarkersDlg::UpdateNewSourceTextAndCombo(SPList* pNewList, int nItem)
// pNewList points to the sourcephrase instances which resulted from the user's editing of the old
// sourcephrase instances list; nItem is the m_comboNewWords box's current selection index, since this
// indicates which word in the combobox (and hence which sourcephrase instances in pNewList) is to have
// any transferred marker, or string of markers. We recreate the contents of the combo box, taking the
// new markers into account; and then update the contents of the m_editNewWords editbox
{
	int nSaveIndex = nItem; // save the combo's current selection index, for when we refill the combo's list
	m_pListNewWords->Clear(); // remove everything

	m_newWordsSelectionBegin.Clear();
	m_newWordsSelectionEnd.Clear();

	SPList::Node* pos = 0;
	SPList* pList = pNewList;
	wxASSERT(pList->GetCount() > 0);
	pos = pList->GetFirst();
	wxUint32 nCharCount = 0;
	wxUint32 nWordLength = 0;
	wxUint32 nMarkersLength = 0;

	m_strNewSourceText.Empty(); // clear out the pre-transfer new source text to make ready for the updated text
	int nWordsListOffset = -1;
	while (pos != 0)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pos->GetData();
		pos = pos->GetNext(); 
		nMarkersLength = pSrcPhrase->m_markers.Length();
		nWordLength = pSrcPhrase->m_srcPhrase.Length();

		// check for markers, and handle them if present
		wxString markers;
		bool bHasMarkers;
		nWordsListOffset = m_pListNewWords->Append(pSrcPhrase->m_srcPhrase);
		if (!pSrcPhrase->m_markers.IsEmpty())
			bHasMarkers = TRUE;
		else
			bHasMarkers = FALSE;

		// accumulate the string for the new source text editbox
		if (m_strNewSourceText.IsEmpty())
		{
			if (bHasMarkers)
			{
				m_strNewSourceText = pSrcPhrase->m_markers + pSrcPhrase->m_srcPhrase + _T(" ");
			}
			else
			{
				m_strNewSourceText = pSrcPhrase->m_srcPhrase + _T(" ");
			}
		}
		else
		{
			if(bHasMarkers)
			{
				m_strNewSourceText += pSrcPhrase->m_markers + pSrcPhrase->m_srcPhrase + _T(" ");
			}
			else
			{
				m_strNewSourceText += pSrcPhrase->m_srcPhrase + _T(" "); // don't care about a final space
			}
		}

		// get the offset to the word, and its length, stored in the data attribute of the combobox item
		nCharCount += nMarkersLength; // the word starts next
		
		m_newWordsSelectionBegin.Add(nCharCount);
		m_newWordsSelectionEnd.Add(nCharCount+nWordLength);
		
		nCharCount += nWordLength + 1; // add on the word length and space, to be ready for next iteration
	}
	TransferDataToWindow();

	// new reset the old selection on the combobox
	int nReturnedItem;
	if (m_pListNewWords->GetCount() > 0) // whm added
		m_pListNewWords->SetSelection(nSaveIndex);
	nReturnedItem = m_pListNewWords->GetSelection();
	gnOldItem = nReturnedItem;
	wxASSERT(nReturnedItem >= 0);

	if (nReturnedItem != -1)
	{
		wxString editNewWordsStr;
		editNewWordsStr = m_pEditNewWords->GetValue();
		int nBegin = m_newWordsSelectionBegin.Item(nReturnedItem);
		int nEnd = m_newWordsSelectionEnd.Item(nReturnedItem);

		wxASSERT(editNewWordsStr.Length() >= (size_t)nBegin && editNewWordsStr.Length() >= (size_t)nEnd); // whm added
		m_pEditNewWords->SetSelection(nBegin,nEnd);
	}
	else
	{
		wxMessageBox(_T("ComboBox returned CB_ERR for unknown reason. Edit box selection not updated.\n"),_T(""), wxICON_EXCLAMATION);
	}
	TransferDataToWindow();
}

// whm changed signature to bool to return FALSE if the copy attempted to copy filtered material in
// the m_markers members
bool CTransferMarkersDlg::SimpleCopyTransfer(CSourcePhrase* pFromSP, CSourcePhrase* pToSP)
{
	bool bNoFilteredMaterial = TRUE;
	// preserve the things already set to new values (the = operator will clobber these, so we must
	// later restore them)
	wxString source = pToSP->m_srcPhrase;
	wxString key = pToSP->m_key;
	wxString precPunct = pToSP->m_precPunct;
	wxString follPunct = pToSP->m_follPunct;
	//bool bSpecial = pToSP->m_bSpecialText; // whm, No, we want to copy over any special text

	// do the all-attributes copy
	*pToSP = *pFromSP; // uses overloaded = operator // whm also copies m_markers

	// whm changed behavior 11Aug06. If pFromSP had filtered material we empty it out
	// of pToSP, and deal with the filtered material in the caller (TransferMarkersDlg::OnButtonTransfer
	// and the View's ReconcileLists()
	if (pToSP->m_markers.Find(filterMkr) != -1)
	{
		// pFromSP->m_markers had filtered material, so we empty pToSP's m_markers here
		// and deal with the situation in the caller
		pToSP->m_markers.Empty();
		bNoFilteredMaterial = FALSE;
	}
	
	// restore the saved attributes
	pToSP->m_srcPhrase = source;
	pToSP->m_key = key;
	pToSP->m_precPunct = precPunct;
	pToSP->m_follPunct = follPunct;

	return bNoFilteredMaterial;
}
	
void CTransferMarkersDlg::ClearForTransfer(CSourcePhrase* pSP)
{
	wxASSERT(pSP != NULL);
	pSP->m_inform.Empty();
	pSP->m_chapterVerse.Empty();
	pSP->m_bFirstOfType = FALSE;
	pSP->m_bFootnote = FALSE;
	pSP->m_bFootnoteEnd = FALSE;
	pSP->m_bChapter = FALSE;
	pSP->m_bVerse = FALSE;
	pSP->m_bParagraph = FALSE;
	// pSP->m_bSpecialText = FALSE; // leave this one unchanged, it should already be set, so simple
									// transfer can just leave it unchanged
	pSP->m_bBoundary = FALSE;
	pSP->m_bHasInternalMarkers = FALSE;
	pSP->m_bHasInternalPunct = FALSE;
	pSP->m_bRetranslation = FALSE;
	pSP->m_bNotInKB = FALSE;
	pSP->m_bNullSourcePhrase = FALSE;
	pSP->m_bHasKBEntry = FALSE;
	pSP->m_bBeginRetranslation = FALSE;
	pSP->m_bEndRetranslation = FALSE;
	// leave m_curTextType, it is already set (but AnalyseMarkers will reset it anyway, later)
	pSP->m_pSavedWords->Clear();
	pSP->m_pMedialMarkers->Clear();
	pSP->m_pMedialPuncts->Clear();
	pSP->m_markers.Empty();
	// don't touch punctuation, whether preceding or following, it is already set, ditto m_srcPhrase,m_key
	pSP->m_adaption.Empty();
	pSP->m_targetStr.Empty();
	pSP->m_nSrcWords = 1; // default
	// leave m_nSequNumber, it is already set
	// and don't touch the 5 BOOL values for free translations, notes, and bookmark support - SimpleCopyTransfer()
	// will have transferrred these values and we don't want the editing process to affect them
}
