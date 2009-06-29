/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseTranslation.cpp
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CChooseTranslation class. 
/// The CChooseTranslation class provides a dialog in which the user can choose 
/// either an existing translation, or enter a new translation for a given source phrase.
/// \derivation		The CChooseTranslation class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ChooseTranslation.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ChooseTranslation.h"
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
#include <wx/tooltip.h> // for wxToolTip

#include "Adapt_It.h"
#include "ChooseTranslation.h"
#include "TargetUnit.h"
#include "RefString.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "KB.h"
#include "helpers.h"
#include "Adapt_ItView.h"

// event handler table
BEGIN_EVENT_TABLE(CChooseTranslation, AIModalDialog)
EVT_INIT_DIALOG(CChooseTranslation::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CChooseTranslation::OnOK)
	EVT_BUTTON(IDC_BUTTON_MOVE_UP, CChooseTranslation::OnButtonMoveUp)
	EVT_UPDATE_UI(IDC_BUTTON_MOVE_UP, CChooseTranslation::OnUpdateButtonMoveUp)
	EVT_BUTTON(IDC_BUTTON_MOVE_DOWN, CChooseTranslation::OnButtonMoveDown)
	EVT_UPDATE_UI(IDC_BUTTON_MOVE_DOWN, CChooseTranslation::OnUpdateButtonMoveDown)
	EVT_LISTBOX(IDC_MYLISTBOX_TRANSLATIONS, CChooseTranslation::OnSelchangeListboxTranslations)
	EVT_LISTBOX_DCLICK(IDC_MYLISTBOX_TRANSLATIONS, CChooseTranslation::OnDblclkListboxTranslations)
	EVT_BUTTON(IDC_BUTTON_REMOVE, CChooseTranslation::OnButtonRemove)
	EVT_UPDATE_UI(IDC_BUTTON_REMOVE, CChooseTranslation::OnUpdateButtonRemove)
	EVT_BUTTON(IDC_BUTTON_CANCEL_ASK, CChooseTranslation::OnButtonCancelAsk)
	EVT_BUTTON(IDC_BUTTON_CANCEL_AND_SELECT, CChooseTranslation::OnButtonCancelAndSelect)
	EVT_KEY_DOWN(CChooseTranslation::OnKeyDown)
END_EVENT_TABLE()

// for support of auto-capitalization

/// This global is defined in Adapt_It.cpp.
extern bool	gbAutoCaps;

/// This global is defined in Adapt_It.cpp.
extern bool	gbSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNonSourceIsUpperCase;

/// This global is defined in Adapt_It.cpp.
extern bool	gbMatchedKB_UCentry;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoSourceCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoTargetCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern bool	gbNoGlossCaseEquivalents;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharNonSrcUC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcLC;

/// This global is defined in Adapt_It.cpp.
extern wxChar gcharSrcUC;

extern bool	gbCallerIsRemoveButton;

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

// This global is defined in Adapt_ItView.cpp.
//extern bool	gbEnableGlossing; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// This global is defined in Adapt_It.cpp.
//extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the NRoman version, using the extra Layout menu
extern CTargetUnit* pCurTargetUnit;
extern wxString		curKey;
extern int			nWordsInPhrase;
extern bool			gbInspectTranslations;


CChooseTranslation::CChooseTranslation(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose Translation"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pChooseTransSizer = ChooseTranslationDlgFunc(this, TRUE, TRUE);
	// The declaration is: ChooseTranslationDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// whm modification 27Feb09: wxDesigner doesn't have an easy way to incorporate a custom derived
	// listbox control such as our CMyListBox control. Hence, it has used the stock library wxListBox
	// in the ChooseTranslationDlgFunc() dialog building function. I'll get a pointer to the sizer
	// which encloses our wxDesigner-provided wxListBox, then delete the list box that wxDesigner
	// provided and substitute an instance of our CMyListBox class created for the purpose. Our custom
	// substituted list box will have the same parent, i.e., "this" (CChooseTranslation) so it will be
	// destroyed when its parent CChooseTranslation is destroyed.
	// Get the pointer to the containing sizer of the provided wxListBox
	wxListBox* pLB = (wxListBox*)FindWindowById(IDC_MYLISTBOX_TRANSLATIONS);
	wxASSERT(pLB != NULL);
	wxBoxSizer* pContSizerOfLB = (wxBoxSizer*)pLB->GetContainingSizer();
	wxASSERT(pContSizerOfLB != NULL);
	// get the list box's existing tooltip
	wxToolTip* pLBToolTip = pLB->GetToolTip();
	wxString ttLBStr;
	if (pLBToolTip != NULL)
	{
		ttLBStr = pLBToolTip->GetTip();
	}
	// delete the existing list box
	delete pLB;
	// create an instance of our CMyListBox class
	m_pMyListBox = new CMyListBox(this, IDC_MYLISTBOX_TRANSLATIONS, wxDefaultPosition, wxSize(400,-1), 0, NULL, wxLB_SINGLE);
	wxASSERT(m_pMyListBox != NULL);
	// Transfer the tooltip to the new custom list box.
	if (pLBToolTip != NULL)
	{
		m_pMyListBox->SetToolTip(ttLBStr);
	}
	// add our custom list box to the sizer in the place of the wxListBox that used to be there.
	pContSizerOfLB->Add(m_pMyListBox, 1, wxGROW|wxALL, 0);
	m_pMyListBox->SetFocus();

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	m_refCount = 0;
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	// wx version note: The parent of our dialogs is not the View, so we'll get the view elsewhere
	m_bHideCancelAndSelectButton = FALSE;

	m_pMyListBox = (CMyListBox*)FindWindowById(IDC_MYLISTBOX_TRANSLATIONS);

	m_pSourcePhraseBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_MATCHED_SOURCE);
	m_pSourcePhraseBox->SetBackgroundColour(gpApp->sysColorBtnFace);
	m_pSourcePhraseBox->Enable(FALSE); // it is readonly and should not receive focus on Tab

	m_pNewTranslationBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_NEW_TRANSLATION);

	m_pEditReferences = (wxTextCtrl*)FindWindowById(IDC_EDIT_REFERENCES);
	m_pEditReferences->SetValidator(wxGenericValidator(&m_refCountStr));
	m_pEditReferences->SetBackgroundColour(gpApp->sysColorBtnFace);
	m_pEditReferences->Enable(FALSE); // it is readonly and should not receive focus on Tab


	// Note: We don't need to set up any SetValidators for data transfer 
	// in this class since all assignment of values is done in OnOK()
}

CChooseTranslation::~CChooseTranslation() // destructor
{
	
}

void CChooseTranslation::OnButtonCancelAsk(wxCommandEvent& WXUNUSED(event))
{
	pCurTargetUnit->m_bAlwaysAsk = FALSE;
	m_pMyListBox->SetFocus();
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated for the ChooseTranslation dialog's
///                         Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever the Choose Translation dialog is showing.
/// If the dialog's list box (m_pMyListBox) has more than one item in it the dialog's "Move Up"
/// button is enabled, otherwise it is disabled.
// //////////////////////////////////////////////////////////////////////////////////////////
void CChooseTranslation::OnUpdateButtonMoveUp(wxUpdateUIEvent& event)
{
	if (m_pMyListBox->GetCount() > 1)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

void CChooseTranslation::OnButtonMoveUp(wxCommandEvent& WXUNUSED(event)) 
{
	// Bruce's email of July 10, 2006 has an idea of how to consolidate and simplify
	// the OnButtonMoveUp and OnButtonMoveDown here and in the KB Editor dialog. I've
	// not implemented his suggestion. He says,
	// "In the legacy app, the KB editor dialog, and the Choose Translation dialog, each 
	// have Move Up and Move Down buttons. The handlers for these have overlapping code, 
	// so are wasteful. It is better to have just a enum such as the following:
	// typedef enum {
	//	 Up,
	//	 Down
	// } Move;
	//and then have just the one handler for each button, with the signature having and 
	// additional member "enum Move direction" in it. Then it is possible to consolidate 
	// the handlers into one. (Well, at least one for the KB editor dialog, and maybe 
	// another for the Choose Translation dialog - I've not checked out how compatible 
	// they are between those dialogs.)"
	//
	int nSel;
	nSel = m_pMyListBox->GetSelection();
	// whm Note: The next check for lack of a good selection here is OK. We need not use our
	// ListBoxPassesSanitCheck() here since a Move Up action should do nothing if the user
	// hasn't selected anything to move up.
	if (nSel == -1) // LB_ERR
	{
		// In wxGTK, when m_pMyListBox->Clear() is called it triggers this OnSelchangeListExistingTranslations
		// handler. The following message is of little help to the user even if it were called for a genuine
		// problem, so I've commented it out, so the present handler can exit gracefully
		//wxMessageBox(_("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
		return;
	}
	int nOldSel = nSel; // save old selection index

	// change the order of the string in the list box
	int count;
	count = pCurTargetUnit->m_pTranslations->GetCount();
	wxASSERT(nSel < count);
	if (nSel > 0)
	{
		nSel--;
		wxString tempStr;
		tempStr = m_pMyListBox->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != -1); // we just added it so it must be there!
		// wx note: In the GetClientData() call below it returns a pointer to void; therefore we need
		// to first cast it to a 32-bit int (wxUint32*) then dereference it to its value with *
		wxUint32 value = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);
		m_pMyListBox->Delete(nLocation);
		m_pMyListBox->InsertItems(1,&tempStr,nSel);
		// m_pMyListBox is NOT sorted but it is safest to find the just-inserted item's index before calling SetClientData()
		nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != -1); // we just added it so it must be there!
		m_pMyListBox->SetSelection(nSel);
		m_pMyListBox->SetClientData(nLocation,&value);
		m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
		return; // impossible to move up the first element in the list!

	// now change the order of the CRefString in pCurTargetUnit to match the new order
	CRefString* pRefString;
	if (nSel < nOldSel)
	{
		TranslationsList::Node* pos = pCurTargetUnit->m_pTranslations->Item(nOldSel);
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pCurTargetUnit->m_pTranslations->DeleteNode(pos);
		pos = pCurTargetUnit->m_pTranslations->Item(nSel);
		wxASSERT(pos != NULL);
		// Note: wxList::Insert places the item before the given item and the inserted item then
		// has the insertPos node position.
		TranslationsList::Node* newPos = pCurTargetUnit->m_pTranslations->Insert(pos,pRefString);
		if (newPos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T("Error: Move Up button failed to reinsert the translation being moved\n"),
				_T(""), wxICON_ERROR);
			wxASSERT(FALSE);
			wxExit();
		}
	}
	TransferDataToWindow();
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated for the ChooseTranslation dialog's
///                         Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever the Choose Translation dialog is showing.
/// If the dialog's list box (m_pMyListBox) has more than one item in it the dialog's "Move Down"
/// button is enabled, otherwise it is disabled.
// //////////////////////////////////////////////////////////////////////////////////////////
void CChooseTranslation::OnUpdateButtonMoveDown(wxUpdateUIEvent& event)
{
	if (m_pMyListBox->GetCount() > 1)
	{
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

void CChooseTranslation::OnButtonMoveDown(wxCommandEvent& WXUNUSED(event)) 
{
	int nSel;
	nSel = m_pMyListBox->GetSelection();
	// whm Note: The next check for lack of a good selection here is OK. We need not use our
	// ListBoxPassesSanitCheck() here since a Move Down action should do nothing if the user
	// hasn't selected anything to move down.
	if (nSel == -1)// LB_ERR
	{
		// In wxGTK, when m_pMyListBox->Clear() is called it triggers this OnSelchangeListExistingTranslations
		// handler. The following message is of little help to the user even if it were called for a genuine
		// problem, so I've commented it out, so the present handler can exit gracefully
		//wxMessageBox(_("List box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
		return;
	}
	int nOldSel = nSel; // save old selection index

	// change the order of the string in the list box
	int count = pCurTargetUnit->m_pTranslations->GetCount();
	wxASSERT(nSel < count);
	if (nSel < count-1)
	{
		nSel++;
		wxString tempStr;
		tempStr = m_pMyListBox->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != -1); //LB_ERR
		wxUint32 value = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);
		m_pMyListBox->Delete(nLocation);
		m_pMyListBox->InsertItems(1,&tempStr,nSel);
		// m_pMyListBox is NOT sorted but it is safest to find the just-inserted item's index before calling SetClientData()
		nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != -1); //LB_ERR
		m_pMyListBox->SetSelection(nSel);
		m_pMyListBox->SetClientData(nLocation,&value);
		m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
		return; // impossible to move the list element of the list further down!

	// now change the order of the CRefString in pCurTargetUnit to match the new order
	CRefString* pRefString;
	if (nSel > nOldSel)
	{
		TranslationsList::Node* pos = pCurTargetUnit->m_pTranslations->Item(nOldSel);
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pCurTargetUnit->m_pTranslations->DeleteNode(pos);
		pos = pCurTargetUnit->m_pTranslations->Item(nOldSel);
		wxASSERT(pos != NULL);
		// wxList has no equivalent to InsertAfter(). The wxList Insert() method 
		// inserts the new node BEFORE the current position/node. To emulate what
		// the MFC code does, I can advance one node before calling Insert()
		// Get a node called posNextHigher which points to the next node beyond savePos
		// in pList and use its position in the Insert() call (which only inserts 
		// BEFORE the indicated position). The result should be that the insertions 
		// will get placed in the list the same way that MFC's InsertAfter() places them.
		// wx additional note: If the item is to be inserted after the last item in the list 
		// posNextHigher will return NULL, in that case, just append the new item to the list.
		TranslationsList::Node* posNextHigher = pos->GetNext();
		TranslationsList::Node* newPos = NULL;
		if (posNextHigher == NULL)
			pCurTargetUnit->m_pTranslations->Append(pRefString);
		else
			pCurTargetUnit->m_pTranslations->Insert(posNextHigher,pRefString);
		newPos = pCurTargetUnit->m_pTranslations->Find(pRefString);
		if (newPos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T("Error: Move Down button failed to reinsert the translation being moved\n"),
				_T(""), wxICON_ERROR);
			wxASSERT(FALSE);
			//wxExit();
		}
	}

	TransferDataToWindow();
}

void CChooseTranslation::OnCancel(wxCommandEvent& WXUNUSED(event)) 
{
	// don't need to do anything
	
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
}

void CChooseTranslation::OnSelchangeListboxTranslations(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (m_pMyListBox->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pMyListBox))
		return;

	wxString s;
	// IDS_NO_ADAPTATION
	s = s.Format(_("<no adaptation>")); // get ready "<no adaptation>" in case needed
	int nSel;
	nSel = m_pMyListBox->GetSelection();
	//if (nSel == -1)//LB_ERR
	//{
	//	// In wxGTK, when m_pMyListBox->Clear() is called it triggers this OnSelchangeListExistingTranslations
	//	// handler. The following message is of little help to the user even if it were called for a genuine
	//	// problem, so I've commented it out, so the present handler can exit gracefully
	//	//wxMessageBox(_("List box error when getting the current selection"), 
	//	//	_T(""), wxICON_ERROR);
	//	//wxASSERT(FALSE);
	//	return; //wxExit();
	//}
	wxString str;
	str = m_pMyListBox->GetString(nSel);
	int nNewSel = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nNewSel);
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	if (str == s)
		str = _T(""); // restore null string to be shown later in the phrase box
	m_chosenTranslation = str;
	TransferDataToWindow();
}

void CChooseTranslation::OnDblclkListboxTranslations(wxCommandEvent& WXUNUSED(event)) 
{
    // whm Note: Sinced this is a "double-click" handler we want the behavior to be essentially
	// equivalent to calling both the OnSelchangeListBoxTranslations(), followed by any code handling
	// that goes into the OnOK() handler. Testing shows that when making a double-click on a list
	// box the OnSelchangeListBoxTranslations() is called first, then this
	// OnDblclkListboxTranslations() handler is called.
	
	wxString s;
	// IDS_NO_ADAPTATION
	s = s.Format(_("<no adaptation>")); // ready "<no adaptation>" in case it's needed
	
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pMyListBox))
	{
		// In wxGTK, when m_pMyListBox->Clear() is called it triggers this OnSelchangeListExistingTranslations
		// handler. The following message is of little help to the user even if it were called for a genuine
		// problem, so I've commented it out, so the present handler can exit gracefully
		wxMessageBox(_("List box error when getting the current selection"),
			_T(""), wxICON_EXCLAMATION);
		return; //wxASSERT(FALSE);
	}

	int nSel;
	nSel = m_pMyListBox->GetSelection();
	//if (nSel == -1)// LB_ERR
	//{
	//	// In wxGTK, when m_pMyListBox->Clear() is called it triggers this OnSelchangeListExistingTranslations
	//	// handler. The following message is of little help to the user even if it were called for a genuine
	//	// problem, so I've commented it out, so the present handler can exit gracefully
	//	wxMessageBox(_("List box error when getting the current selection"),
	//		_T(""), wxICON_EXCLAMATION);
	//	wxASSERT(FALSE);
	//	//wxExit();
	//}
	wxString str;
	str = m_pMyListBox->GetString(nSel);
	int nNewSel = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nNewSel);
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	if (str == s)
	{
		str = _T(""); // restore null string to be shown in the phrase box
		m_bEmptyAdaptationChosen = TRUE; // this will be used to set the 
								// gbEmptyAdaptationChosen global used by PlacePhraseBox
	}
	m_chosenTranslation = str;
	TransferDataToWindow();
    EndModal(wxID_OK); //EndDialog(IDOK);
    // whm Correction 12Jan09 - The zero parameter given previously to EndModal(0) was incorrect. The
    // parameter needs to be the value that gets returned from the ShowModal() being invoked on this
    // dialog - which in the case of a double-click, should be wxID_OK.
	
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	pApp->m_pTargetBox->m_bAbandonable = FALSE;
}

void CChooseTranslation::OnButtonRemove(wxCommandEvent& WXUNUSED(event)) 
{
	// whm added: If the m_pMyListBox is empty, just return as there is nothing to remove
	if (m_pMyListBox->GetCount() == 0)
		return;

	wxString s;
	// IDS_NO_ADAPTATION
	s = s.Format(_("<no adaptation>")); // that is, "<no adaptation>" in case we need it

	// this button must remove the selected translation from the KB, which means that
	// user must be shown a child dialog or message to the effect that there are m_refCount instances
	// of that translation in this and previous document files which will then not agree with
	// the state of the knowledge base, and the user is then to be urged to do a Verify operation
	// on each of the existing document files to get the KB and those files in synch with each other.

	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pMyListBox))
	{
		// message can be in English, it's never likely to occur
		wxMessageBox(_("List box error when getting the current selection"), _T(""), wxICON_ERROR);
		return; //wxASSERT(FALSE);
	}

	// get the index of the selected translation string (this will be same index for the CRefString
	// stored in pCurTargetUnit)
	int nSel;
	wxString str;
	nSel = m_pMyListBox->GetSelection();
	//if (nSel == -1)// LB_ERR
	//{
	//	// message can be in English, it's never likely to occur
	//	wxMessageBox(_("List box error when getting the current selection"), _T(""), wxICON_ERROR);
	//	wxASSERT(FALSE);
	//	//wxExit();
	//}

	// get the selected string into str; it could be upper or lower case, even when auto caps
	// is ON, because the user could have made KB entries with auto caps OFF previously. But
	// we take the string as is, and don't make case conversions here
	str = m_pMyListBox->GetString(nSel); // the list box translation string being deleted
	wxString str2 = str;
	wxString message;
	if (str == s)
		str = _T(""); // for comparison's with what is stored in the KB, it must be empty

	// find the corresponding CRefString instance in the knowledge base, and set the
	// nPreviousReferences variable for use in the message box; if user hits Yes
	// then go ahead and do the removals. (Note: the global pCurTargetUnit is set to a
	// target unit in either the glossing KB (when glossing is ON) or to one in the normal KB
	// when adapting, so we don't need to test gbIsGlossing here.
	TranslationsList::Node* pos = pCurTargetUnit->m_pTranslations->Item(nSel);
	wxASSERT(pos != NULL);
	CRefString* pRefString = (CRefString*)pos->GetData();
	wxASSERT(pRefString != NULL);

	// check that we have the right reference string instance; we must allow for equality of two empty
	// strings to be considered to evaluate to TRUE
	if (str.IsEmpty() && pRefString->m_translation.IsEmpty())
		goto a;
	if (str != pRefString->m_translation)
	{
		// message can be in English, it's never likely to occur
		wxMessageBox(_T("OnButtonRemove() error: Knowledge bases's adaptation text does not match that selected in the list box\n"),
			_T(""), wxICON_EXCLAMATION);
	}

	// get the ref count, use it to warn user about how many previous references this will mess up
a:	int nPreviousReferences = pRefString->m_refCount;

	// warn user about the consequences, allow him to get cold feet & back out
	// IDS_VERIFY_NEEDED
	message = message.Format(_("Removing: \"%s\", will make %d occurrences of it in the document files inconsistent with the knowledge base.\n(You can fix that later by using the Consistency Check command.)\nDo you want to go ahead and remove it?"),str2.c_str(),nPreviousReferences);
	int response = wxMessageBox(message, _T(""), wxYES_NO | wxICON_WARNING);
	if (!(response == wxYES))
	{
		return; // user backed out
	}

	// user hit the Yes button, so go ahead; remove the string from the list
	m_pMyListBox->Delete(nSel);
	if (m_pMyListBox->GetCount() > 0)
	{
		m_pMyListBox->SetSelection(0);
		wxString str = m_pMyListBox->GetStringSelection();
		int nNewSel = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
		wxASSERT(nNewSel != -1);
		m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nNewSel);
	}
	else
	{
		m_refCount = 0;
	}
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	// m_refCount will have been set to -1 (ie. LB_ERR) if we remove the last thing in the list box,
	// so we have to detect that & change it to zero, otherwise bounds checking won't let us exit
	// the box
	if (m_refCount < 0)
		m_refCount = 0;
	if (str == s)
	{
		str = _T(""); // not needed I think, but I'll leave it here, it's harmless
	}

	// in support of removal when autocapitalization might be on - see the OnButtonRemove handler
	// in KBEditor.cpp for a full explanation of the need for this flag
	gbCallerIsRemoveButton = TRUE;

	// remove the corresponding CRefString instance from the knowledge base
	delete pRefString;
	pRefString = (CRefString*)NULL;
	pCurTargetUnit->m_pTranslations->DeleteNode(pos);

	// are we about to delete the last item in the box?
	int nItems = m_pMyListBox->GetCount();
	if (nItems == 0)
	{
		// this means the pTargetUnit has no CRefString instances left, so we have to remove the
		// association of this target unit and its key from the KB as well (global curKey has it)
		wxString str = curKey;
		wxASSERT(!str.IsEmpty());
		CKB* pKB;
		if (gbIsGlossing)
			pKB = gpApp->m_pGlossingKB;
		else
			pKB = gpApp->m_pKB;
		MapKeyStringToTgtUnit* pMap = pKB->m_pMap[nWordsInPhrase-1];
		//pMap->erase(curKey); //pMap->RemoveKey(curKey); // ignore int return value
		// whm 3Aug06 substituted the next 20 lines for the above RemoveKey call, in order to 
		//remove any different case key association existing in pMap. Changes also made to MFC version.
		wxString s1 = curKey;
		bool bNoError = TRUE;
		int nRemoved = 0;
		if (gbAutoCaps)
		{
			bNoError = gpApp->GetView()->SetCaseParameters(s1);
			if (bNoError && gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
			{
				// make it start with lower case letter
				s1.SetChar(0,gcharSrcLC);
			}
			// try removing the lower case one first, this is the most likely one that
			// was found by GetRefString( ) in the caller
			nRemoved = pMap->erase(s1); //bRemoved = pMap->RemoveKey(s1); // also remove it from the map
		}
		if (nRemoved == 0)
		{
			// have a second shot using the unmodified string curKey
			nRemoved = pMap->erase(curKey);// also remove it from the map
		}
		wxASSERT(nRemoved == 1);
		TUList::Node* pos = pKB->m_pTargetUnits->Find(pCurTargetUnit);
		wxASSERT(pos != NULL);
		pKB->m_pTargetUnits->DeleteNode(pos);
		delete pCurTargetUnit; // ensure no memory leak
		pCurTargetUnit = (CTargetUnit*)NULL;
		// whm Note 3Aug06:
		// The user wanted to delete the translation from the KB, so it would be nice if
		// the the phrasebox did not show the removed translation again after ChooseTranslation 
		// dialog exits, but a later call to PlacePhraseBox copies the source again and
		// echos it there. 
	}
	// do we need to show the Do Not Ask Again button?
	if (nItems == 1 && pCurTargetUnit->m_bAlwaysAsk)
	{
		wxWindow* pButton = FindWindowById(IDC_BUTTON_CANCEL_ASK);
		wxASSERT(pButton != NULL);
		pButton->Show(TRUE);
	}
	TransferDataToWindow();
	gbCallerIsRemoveButton = FALSE; // reestablish the safe default
	// If the last translation was removed set focus to the New Translation edit box, otherwise to the
	// Translations list box. (requested by Wolfgang Stradner).
	if (nItems == 0)
		m_pNewTranslationBox->SetFocus();
	else
		m_pMyListBox->SetFocus();
}

void CChooseTranslation::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	m_bEmptyAdaptationChosen = FALSE;
	m_bCancelAndSelect = FALSE;
	
	wxString s;
	//IDS_NO_ADAPTATION
	s = s.Format(_("<no adaptation>")); // that is, "<no adaptation>", ready in case we need it

	// first, use the current source and target language fonts for the list box
	// and edit boxes (in case there are special characters)

	// make the fonts show user's desired point size in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pSourcePhraseBox, NULL,
								NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pSourcePhraseBox, NULL, 
								NULL, NULL, gpApp->m_pDlgSrcFont);
	#endif

	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pNewTranslationBox, NULL,
								m_pMyListBox, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
	#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pNewTranslationBox, NULL, 
								m_pMyListBox, NULL, gpApp->m_pDlgTgtFont);
	#endif

	// set the "matched source text" edit box contents
	m_pSourcePhraseBox->SetValue(curKey);

	// set the "new translation" edit box contents to a null string
	m_pNewTranslationBox->SetValue(_T(""));

	// set the list box contents to the translation or gloss strings stored
	// in the global variable pTargetUnit, which has just been matched
	CRefString* pRefString;
	TranslationsList::Node* pos = pCurTargetUnit->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		pRefString = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxString str = pRefString->m_translation;
		if (str.IsEmpty())
		{
			str = s;
		}
		m_pMyListBox->Append(str);
		// m_pMyListBox is NOT sorted but it is safest to find the just-inserted item's index before calling SetClientData()
		int nLocation = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
		wxASSERT(nLocation != -1); // we just added it so it must be there!
		m_pMyListBox->SetClientData(nLocation,&pRefString->m_refCount);
	}

	// select the first string in the listbox by default
	m_pMyListBox->SetSelection(0);
	wxString str = m_pMyListBox->GetStringSelection();
	int nNewSel = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nNewSel);
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	// the above could fail, if nothing is in the list box, in which case -1 will be put in the 
	// m_refCount variable, in which case that is out of bounds, so change to zero
	if (m_refCount < 0)
		m_refCount = 0;

	// hide the Do Not Ask Again button if there is more than one item in the list
	int nItems = m_pMyListBox->GetCount();
	wxWindow* pButton = FindWindowById(IDC_BUTTON_CANCEL_ASK);
	wxASSERT(pButton != NULL);
	if (nItems == 1 && pCurTargetUnit->m_bAlwaysAsk)
	{
		// only one, so allow user to stop the forced ask by hitting the button, so show it
		pButton->Show(TRUE);

	}
	else
	{
		// more than one, so we don't want to see the button
		pButton->Show(FALSE);
	}

	// hide the cancel and select button if the user is just inspecting translations at the active
	// location, or if we get here because we are just looking up a single word so as to put its
	// appropriate translation in the phrase box (eg. when Cancel of LookAhead matched phrase is done)
	// or when glossing is ON
	if (gbInspectTranslations || m_bHideCancelAndSelectButton)
	{
		wxButton* pButton = (wxButton*)FindWindowById(IDC_BUTTON_CANCEL_AND_SELECT);
		pButton->Show(FALSE);
	}
	
	TransferDataToWindow();

	// place the dialog window so as not to obscure things
	// work out where to place the dialog window
	gpApp->GetView()->AdjustDialogPosition(this);

	m_pMyListBox->SetFocus();

}

void CChooseTranslation::OnButtonCancelAndSelect(wxCommandEvent& event) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	m_bCancelAndSelect = TRUE; // alter behaviour when Cancel is wanted (assume user wants instead
								// to create a merge of the source words at the selection location)
	pApp->m_curDirection = right; // make sure any left-direction earlier drag selection
									// is cancelled
	OnCancel(event);
}

void CChooseTranslation::OnKeyDown(wxKeyEvent& event) 
// applies only when adapting; when glossing, just immediately exit
{
	if (gbIsGlossing) return; // glossing being ON must not allow this shortcut to work
	if (event.AltDown())
	{
		// ALT key is down, so if right arrow key pressed, interpret the ALT key press as
		// a click on "Cancel And Select" button
		if (event.GetKeyCode() == WXK_RIGHT)
		{
			wxButton* pButton = (wxButton*)FindWindowById(IDC_BUTTON_CANCEL_AND_SELECT);
			if (pButton->IsShown())
			{
				wxCommandEvent evt = wxID_CANCEL;
				OnButtonCancelAndSelect(evt);
				return;
			}
			else
				return; // ignore the keypress when the button is invisible
		}
	}
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CChooseTranslation::OnOK(wxCommandEvent& event) 
{
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

	wxString s;
	// IDS_NO_ADAPTATION
	s = s.Format(_("<no adaptation>")); // get "<no adaptation>" ready in case needed

	wxString strNew;
	strNew = m_pNewTranslationBox->GetValue();
	// if the user typed a new string, use it; otherwise take the list selection
	if (!strNew.IsEmpty())
	{
		m_chosenTranslation = strNew;
	}
	else
	{
		int nSel;
		nSel = m_pMyListBox->GetSelection();
		// whm Note: The following check for no valid selection is OK.
		if (nSel == -1)// LB_ERR
		{
			// assume its an empty listBox and just force an empty string
			m_chosenTranslation = _T("");
		}
		else
		{
			wxString str;
			str = m_pMyListBox->GetString(nSel);
			if (str == s)
			{
				str = _T(""); // restore null string
				m_bEmptyAdaptationChosen = TRUE; // this will be used to set the 
										// gbEmptyAdaptationChosen global used by PlacePhraseBox
			}

			m_chosenTranslation = str;
		}
	}
	pApp->m_pTargetBox->m_bAbandonable = FALSE;
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated for the ChooseTranslation dialog's
///                         Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever the Choose Translation dialog is showing.
/// If the dialog's list box (m_pMyListBox) has at least one item in it the dialog's "Remove From KB"
/// button is enabled, otherwise it is disabled when the list is empty.
// //////////////////////////////////////////////////////////////////////////////////////////
void CChooseTranslation::OnUpdateButtonRemove(wxUpdateUIEvent& event)
{
	if (m_pMyListBox->GetCount() == 0)
	{
		event.Enable(FALSE);
	}
	else
	{
		event.Enable(TRUE);
	}
}

