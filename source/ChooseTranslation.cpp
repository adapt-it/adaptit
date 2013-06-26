/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseTranslation.cpp
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CChooseTranslation class.
/// The CChooseTranslation class provides a dialog in which the user can choose
/// either an existing translation, or enter a new translation for a given source phrase.
/// \derivation		The CChooseTranslation class is derived from AIModalDialog.
/// BEW 2July10, this class has been updated to support kbVersion 2
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
#include "KB.h"
#include "ChooseTranslation.h"
#include "TargetUnit.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "helpers.h"
#include "Adapt_ItView.h"
#include "KbServer.h"
#include "Thread_PseudoDelete.h"

// event handler table
BEGIN_EVENT_TABLE(CChooseTranslation, AIModalDialog)
EVT_INIT_DIALOG(CChooseTranslation::InitDialog)
	EVT_BUTTON(wxID_OK, CChooseTranslation::OnOK)
	EVT_BUTTON(wxID_CANCEL, CChooseTranslation::OnCancel)
	EVT_BUTTON(IDC_BUTTON_MOVE_UP, CChooseTranslation::OnButtonMoveUp)
	EVT_UPDATE_UI(IDC_BUTTON_MOVE_UP, CChooseTranslation::OnUpdateButtonMoveUp)
	EVT_BUTTON(IDC_BUTTON_MOVE_DOWN, CChooseTranslation::OnButtonMoveDown)
	EVT_UPDATE_UI(IDC_BUTTON_MOVE_DOWN, CChooseTranslation::OnUpdateButtonMoveDown)
	EVT_LISTBOX(IDC_MYLISTBOX_TRANSLATIONS, CChooseTranslation::OnSelchangeListboxTranslations)
	EVT_LISTBOX_DCLICK(IDC_MYLISTBOX_TRANSLATIONS, CChooseTranslation::OnDblclkListboxTranslations)
	EVT_BUTTON(IDC_BUTTON_REMOVE, CChooseTranslation::OnButtonRemove)
	EVT_UPDATE_UI(IDC_BUTTON_REMOVE, CChooseTranslation::OnUpdateButtonRemove)
	EVT_BUTTON(ID_BUTTON_CANCEL_ASK, CChooseTranslation::OnButtonCancelAsk) // ID_ was IDC_
	EVT_BUTTON(ID_BUTTON_CANCEL_AND_SELECT, CChooseTranslation::OnButtonCancelAndSelect) // ID_ was IDC_
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
//extern bool	gbGlossingVisible; // TRUE makes Adapt It revert to Shoebox functionality only

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// This global is defined in Adapt_It.cpp.
//extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
							// be changed in the NRoman version, using the extra Layout menu
extern CTargetUnit* pCurTargetUnit; // defined PhraseBox.cpp
extern wxString		curKey;
extern int			nWordsInPhrase;
extern bool			gbInspectTranslations;


CChooseTranslation::CChooseTranslation(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose Translation"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which should normally be
    // "this". The second and third parameters should both be TRUE to utilize the sizers
    // and create the right size dialog.
	//pChooseTransSizer = ChooseTranslationDlgFunc(this, TRUE, TRUE); <- wastes space,
	//list too small, so I deprecated it
	pChooseTransSizer = ChooseTranslationDlgFunc2(this, TRUE, TRUE); // BEW added 15Mar13
    // The declaration is: ChooseTranslationDlgFunc( wxWindow *parent, bool call_fit, bool
    // set_sizer );

   // whm modification 27Feb09: wxDesigner doesn't have an easy way to incorporate a custom
   // derived listbox control such as our CMyListBox control. Hence, it has used the stock
   // library wxListBox in the ChooseTranslationDlgFunc() dialog building function. I'll
   // get a pointer to the sizer which encloses our wxDesigner-provided wxListBox, then
   // delete the list box that wxDesigner provided and substitute an instance of our
   // CMyListBox class created for the purpose. Our custom substituted list box will have
   // the same parent, i.e., "this" (CChooseTranslation) so it will be destroyed when its
   // parent CChooseTranslation is destroyed.
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
	if (pLB != NULL) // whm 11Jun12 added NULL test
	    delete pLB;
    // create an instance of our CMyListBox class
    //m_pMyListBox = new CMyListBox(this, IDC_MYLISTBOX_TRANSLATIONS, wxDefaultPosition,
	//								wxSize(400,-1), 0, NULL, wxLB_SINGLE);
    m_pMyListBox = new CMyListBox(this, IDC_MYLISTBOX_TRANSLATIONS, wxDefaultPosition,
									wxSize(300,-1), 0, NULL, wxLB_SINGLE);
    wxASSERT(m_pMyListBox != NULL);
    // Transfer the tooltip to the new custom list box.
    if (pLBToolTip != NULL)
    {
        m_pMyListBox->SetToolTip(ttLBStr);
    }
    // add our custom list box to the sizer in the place of the wxListBox that used to be
    // there.
    pContSizerOfLB->Add(m_pMyListBox, 1, wxGROW|wxALL, 0);
    m_pMyListBox->SetFocus();

	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning
	m_refCount = 0;
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	// wx version note: The parent of our dialogs is not the View, so we'll get the
	// view elsewhere
	m_bHideCancelAndSelectButton = FALSE;

	m_pMyListBox = (CMyListBox*)FindWindowById(IDC_MYLISTBOX_TRANSLATIONS);

	m_pSourcePhraseBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_MATCHED_SOURCE);
	m_pSourcePhraseBox->SetBackgroundColour(gpApp->sysColorBtnFace);
	m_pSourcePhraseBox->Enable(FALSE); // it is readonly and should not receive focus on Tab

	m_pNewTranslationBox = (wxTextCtrl*)FindWindowById(IDC_EDIT_NEW_TRANSLATION);

	m_pEditReferences = (wxTextCtrl*)FindWindowById(IDC_EDIT_REFERENCES);
	//m_pEditReferences->SetValidator(wxGenericValidator(&m_refCountStr)); // whm removed 21Nov11
	m_pEditReferences->SetBackgroundColour(gpApp->sysColorBtnFace);
	m_pEditReferences->Enable(FALSE); // it is readonly and should not receive focus on Tab

	// get pointers to the CKB instance & the map which stores the pCurTargetUnit contents
	// being viewed
	m_nWordsInPhrase = nWordsInPhrase; // RHS is a global we need to eliminate eventually,
									   // as also is gbIsGlossing, and gpApp too
	if (gbIsGlossing)
	{
		wxASSERT(m_nWordsInPhrase == 1);
		m_pKB = gpApp->m_pGlossingKB;
	}
	else
	{
		wxASSERT( m_nWordsInPhrase <= MAX_WORDS && m_nWordsInPhrase > 0);
		m_pKB = gpApp->m_pKB;
	}
	m_pMap = m_pKB->m_pMap[m_nWordsInPhrase-1];

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

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated for the ChooseTranslation dialog's
///                         Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever the Choose Translation dialog is showing.
/// If the dialog's list box (m_pMyListBox) has more than one item in it the dialog's "Move Up"
/// button is enabled, otherwise it is disabled.
////////////////////////////////////////////////////////////////////////////////////////////
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

// BEW 28Jun10, changes needed for support of kbVersion 2 & its m_bDeleted flag
void CChooseTranslation::OnButtonMoveUp(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	nSel = m_pMyListBox->GetSelection();
    // whm Note: The next check for lack of a good selection here is OK. We need not use
    // our ListBoxPassesSanitCheck() here since a Move Up action should do nothing if the
    // user hasn't selected anything to move up.
	if (nSel == -1)
	{
		return;
	}
	int nOldSel = nSel; // save old selection index
	wxString itemStr = m_pMyListBox->GetString(nOldSel);
	wxString s;
	s = s.Format(_("<no adaptation>"));
	if (itemStr == s)
	{
		// CTargetUnit stores empty string, not <no adaptation>
		itemStr.Empty();
	}
	int nListIndex = (int)wxNOT_FOUND;
	TranslationsList::Node* pos = NULL;

	// change the order of the string in the list box
	// BEW 28Jun10, kbVersion 2 complicates things now, because the pCurTargetUnit pointer
	// may contain one or more "removed" (i.e. with their m_bDeleted flags set TRUE)
	// CRefString instances, and so we can't rely on the list box index for the selection
	// matching an actual undeleted CRefString instance in the m_pTranslations list - so
	// we have to find by searching, and we have to skip over any removed ones, etc
	int count;
	count = m_pMyListBox->GetCount(); // how many there are that are visible
	int numNotDeleted = pCurTargetUnit->CountNonDeletedRefStringInstances(); // the visible ones
	wxASSERT(count == numNotDeleted);
	wxASSERT(nSel < count);
	count = count; // avoid warning
	numNotDeleted = numNotDeleted; // prevent compiler warning in Release build

	if (nSel > 0)
	{
		nSel--;
		wxString tempStr;
		tempStr = m_pMyListBox->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != wxNOT_FOUND); // we just added it so it must be there!
        // wx note: In the GetClientData() call below it returns a pointer to void;
        // therefore we need to first cast it to a 32-bit int (wxUint32*) then dereference
        // it to its value with *
		wxUint32 value = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);

		// get the index for the selected CRefString instance being moved (this call
		// handles the possible presence of deleted instances) and from it, the
		// CRefString instance -- this index is for the CTargetUnit's list, not ListBox
		nListIndex = pCurTargetUnit->FindRefString(itemStr); // handles empty string correctly
		wxASSERT(nListIndex != wxNOT_FOUND);
		pos = pCurTargetUnit->m_pTranslations->Item(nListIndex);
		wxASSERT(pos != NULL);

		// now delete the label at nLocation, so the label following then occupies its index
		// value in the ListBox, and then insert the deleted label preceding the latter,
		// and nSel will index its new location
		m_pMyListBox->Delete(nLocation);
		m_pMyListBox->Insert(tempStr,nSel,(void*)&value);
		// m_pMyListBox is NOT sorted but it is safest to find the just-inserted item's
		// index before calling a function which needs to know the location
		nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != wxNOT_FOUND); // we just added it so it must be there!
		// nLocation and nSel now index the same location
		m_pMyListBox->SetSelection(nSel);
		m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
	{
		return; // impossible to move up the first element in the list!
	}
	// now change the order of the CRefString in pCurTargetUnit to match the new order
	CRefString* pRefString = NULL;
	if (nSel < nOldSel)
	{
		// BEW 28Jun10, for support of kbVersion 2, nSel and nOldSel apply only to the GUI
		// list, which does not show stored CRefString instances marked as deleted. The
		// potential presence of deleted instances means that we must search for the
		// instance to be moved earlier in the list, and moving it means we must move over
		// each preceding deleted instance, if any, until we get to the location of the
		// first preceding non-deleted element, and insert at that location
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		TranslationsList::Node* posOld = pos; // save for later on, when we want to delete it
		CRefString* aRefStrPtr = NULL;
		do {
            // jump over any deleted previous CRefString instances, break out when a
            // non-deleted one is found
			pos = pos->GetPrevious();
			aRefStrPtr = pos->GetData();
			if (!aRefStrPtr->GetDeletedFlag())
			{
				break;
			}
		} while(aRefStrPtr->GetDeletedFlag() && pos != NULL);

		// pos now points at the first preceding non-deleted CRefString instance, which is
		// the one before which we want to put pOldRefStr by way of insertion, but first
		// delete the node containing the old location's instance
		pCurTargetUnit->m_pTranslations->DeleteNode(posOld);
		wxASSERT(pos != NULL);

        // now do the insertion, bringing the pCurTargetUnit's list into line with what the
        // listbox in the GUI shows to the user
        // Note: wxList::Insert places the item before the given item and the inserted item
        // then has the pos node position.
		pos = pCurTargetUnit->m_pTranslations->Insert(pos,pRefString);
		if (pos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T("Error: Move Up button failed to reinsert the translation being moved\n"),
				_T(""), wxICON_ERROR | wxOK);
			wxASSERT(FALSE);
		}
	}
	if (nSel < nOldSel)
	{
		//TransferDataToWindow(); // whm removed 21Nov11
		m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
		// try repopulating the ListBox to see if it cures the failure to retain the
		// client data of the moved list item beyond a single Move Up button click...
		// Yes, it does. See comments in OnButtonMoveDown() for more information about
		// this error and its fix.
		PopulateList(pCurTargetUnit, nSel, Yes);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated for the ChooseTranslation dialog's
///                         Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever the Choose Translation dialog is showing.
/// If the dialog's list box (m_pMyListBox) has more than one item in it the dialog's "Move Down"
/// button is enabled, otherwise it is disabled.
////////////////////////////////////////////////////////////////////////////////////////////
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

// BEW 30Jun10, changes needed for support of kbVersion 2 & its m_bDeleted flag
void CChooseTranslation::OnButtonMoveDown(wxCommandEvent& WXUNUSED(event))
{
	int nSel;
	nSel = m_pMyListBox->GetSelection();
	// whm Note: The next check for lack of a good selection here is OK. We need not use our
	// ListBoxPassesSanitCheck() here since a Move Down action should do nothing if the user
	// hasn't selected anything to move down.
	if (nSel == -1)// LB_ERR
	{
		return;
	}
	int nOldSel = nSel; // save old selection index
	wxString itemStr = m_pMyListBox->GetString(nOldSel);
	wxString s;
	s = s.Format(_("<no adaptation>"));
	if (itemStr == s)
	{
		// CTargetUnit stores empty string, not <no adaptation
		itemStr.Empty();
	}
	int nListIndex = (int)wxNOT_FOUND;
	TranslationsList::Node* pos = NULL;

	// change the order of the string in the list box
	// BEW 30Jun10, kbVersion 2 complicates things now, because the pCurTargetUnit pointer
	// may contain one or more "removed" (i.e. with their m_bDeleted flags set TRUE)
	// CRefString instances, and so we can't rely on the list box index for the selection
	// matching an actual undeleted CRefString instance in the m_pTranslations list - so
	// we have to find by searching, and we have to skip over any removed ones, etc
	int count = m_pMyListBox->GetCount(); // how many there are that are visible
	int numNotDeleted = pCurTargetUnit->CountNonDeletedRefStringInstances(); // the visible ones
	wxASSERT(count == numNotDeleted);
	wxASSERT(nSel < count);
	numNotDeleted = numNotDeleted; // prevent compiler warning in Release build

	if (nSel < count-1)
	{
		nSel++;
		wxString tempStr;
		tempStr = m_pMyListBox->GetString(nOldSel);
        // This wxLogDebug section exposes a bug wherein a second Move Down button click
        // fails to retain the client data value - it becomes a huge garbage value after
        // the item has been deleted, then inserted elsewhere and the client data reset;
        // the cure was to repopulate the ListBox after every Move Down click, resetting
        // the client data values for each entry - see PopulateListBox() call at end of
        // this function. The probable cause may be explained by this wxWidgets
        // documentation comment for SetClientData() in wxControlWithItems. "...it is an
        // error to call this function if any typed client data pointers had been
        // associated with the control items before"
		//#ifdef _DEBUG
		//	int index;
		//	for (index = 0; index < count; index++)
		//	{
		//		wxLogDebug(_T("Item label:   %s    index:   %d    client data value:  %d"),
		//			m_pMyListBox->GetString(index),index,*(wxUint32*)m_pMyListBox->GetClientData(index));
		//	}
		//#endif
		int nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != wxNOT_FOUND);
		wxUint32 value = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);

		// get the index for the selected CRefString instance being moved (this call
		// handles the possible presence of deleted instances) and from it, the
		// CRefString instance -- this index is for the CTargetUnit's list, not ListBox
		nListIndex = pCurTargetUnit->FindRefString(itemStr); // handles empty string correctly
		wxASSERT(nListIndex != wxNOT_FOUND);
		pos = pCurTargetUnit->m_pTranslations->Item(nListIndex);
		wxASSERT(pos != NULL);

		// now delete the label at nLocation, so the label following then occupies its index
		// value in the ListBox, and then insert the deleted label preceding the one which
		// follows the latter -- that is, insert at nSel index value
		m_pMyListBox->Delete(nLocation);
		if (nSel >= count - 1)
		{
			m_pMyListBox->Append(tempStr,(void*)&value);
		}
		else
		{
			m_pMyListBox->Insert(tempStr,nSel,(void*)&value);
		}
		// m_pMyListBox is NOT sorted but it is safest to find the just-inserted item's
		// index before calling a function which needs to know the location
		nLocation = gpApp->FindListBoxItem(m_pMyListBox,tempStr,caseSensitive,exactString);
		wxASSERT(nLocation != wxNOT_FOUND);
		m_pMyListBox->SetSelection(nSel);
		m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nLocation);
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
	{
		return; // impossible to move the list element of the list further down!
	}

	// now change the order of the CRefString in pCurTargetUnit to match the new order
	CRefString* pRefString = NULL;
	if (nSel > nOldSel)
	{
        // BEW 30Jun10, for support of kbVersion 2, nSel and nOldSel apply only to the GUI
        // list, which does not show stored CRefString instances marked as deleted. The
        // potential presence of deleted instances means that we must search for the
        // instance to be moved later in the list, and moving it means we must move over
        // each following deleted instance, if any, until we get to the location of the
        // first following non-deleted element, move over it and then insert at that
        // location
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		TranslationsList::Node* posOld = pos; // save for later on, when we want to delete it
		CRefString* aRefStrPtr = NULL;
		do {
			// jump over any deleted following CRefString instances, break when a
			// non-deleted one is found
			pos = pos->GetNext();
			aRefStrPtr = pos->GetData();
			if (!aRefStrPtr->GetDeletedFlag())
			{
				break;
			}
		} while(aRefStrPtr->GetDeletedFlag() && pos != NULL);
        // now advance over this non-deleted one -- this may make the iterator return NULL
        // if we are at the end of the list; then the insertion, bringing the
        // pCurTargetUnit's list into line with what the listbox in the GUI shows to the
        // user
        // Note: wxList::Insert places the item before the given item and the inserted item
        // then has the pos node position
		pos = pos->GetNext();
		if (pos == NULL)
		{
			// we are at the list's end
			pCurTargetUnit->m_pTranslations->Append(pRefString);
		}
		else
		{
			// we are at a CRefString instance, so we can insert before it
			pCurTargetUnit->m_pTranslations->Insert(pos,pRefString);
		}
		// delete the node containing the old location's instance
		pCurTargetUnit->m_pTranslations->DeleteNode(posOld);

		// check the insertion or append got done right, a simple message will do (in
		// English) for the developer if it didn't work - this error is unlikely to ever
		// happen
		pos = pCurTargetUnit->m_pTranslations->Find(pRefString);
		if (pos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T("Error: Move Down button failed to reinsert the translation being moved\n"),
				_T(""), wxICON_ERROR | wxOK);
			wxASSERT(FALSE);
		}
	}
	if (nSel > nOldSel)
	{
		//TransferDataToWindow(); // whm removed 21Nov11
		m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
		// try repopulating the ListBox to see if it cures the failure to retain the
		// client data of the moved list item beyond a single Move Down button click...
		// Yes, it does. So this is a fix, see the comment above the wxLogDebug() call
		// above for a potential explanation for this error.
		// (If you comment out this next PopulateList() call and the error will reappear,
		// and if you uncomment out the wxLogDebug code above, you'll see what I mean)
		PopulateList(pCurTargetUnit, nSel, Yes);
	}
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
	wxString str = m_pMyListBox->GetString(nSel);
	int nNewSel = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	m_refCount = *(wxUint32*)m_pMyListBox->GetClientData(nNewSel);
	m_refCountStr.Empty();
	m_refCountStr << m_refCount;
	if (str == s)
		str = _T(""); // restore null string to be shown later in the phrase box
	m_chosenTranslation = str;
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
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
        // In wxGTK, when m_pMyListBox->Clear() is called it triggers this
        // OnSelchangeListExistingTranslations handler.
		wxMessageBox(_("List box error when getting the current selection"),
		_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}

	int nSel;
	nSel = m_pMyListBox->GetSelection();
	wxString str = m_pMyListBox->GetString(nSel);
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
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
    EndModal(wxID_OK); //EndDialog(IDOK);
    // whm Correction 12Jan09 - The zero parameter given previously to EndModal(0) was incorrect. The
    // parameter needs to be the value that gets returned from the ShowModal() being invoked on this
    // dialog - which in the case of a double-click, should be wxID_OK.

	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);
	pApp->m_pTargetBox->m_bAbandonable = FALSE;
}

// BEW 28Jun10, changes needed for support of kbVersion 2 & its m_bDeleted flag
void CChooseTranslation::OnButtonRemove(wxCommandEvent& WXUNUSED(event))
{
	// whm added: If the m_pMyListBox is empty, just return as there is nothing to remove
	if (m_pMyListBox->GetCount() == 0)
		return;

	wxString s;
	s = s.Format(_("<no adaptation>")); // that is, "<no adaptation>" in case we need it

    // this button must remove the selected translation from the KB, which means that user
    // must be shown a child dialog or message to the effect that there are m_refCount
    // instances of that translation in this and previous document files which will then
    // not agree with the state of the knowledge base, and the user is then to be urged to
    // do a Verify operation on each of the existing document files to get the KB and those
    // files in synch with each other.
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pMyListBox))
	{
		// message can be in English, it's never likely to occur
		wxMessageBox(_("List box error when getting the current selection"),
		_T(""), wxICON_ERROR | wxOK);
		return;
	}

    // get the index of the selected translation string (this will be same index for the
	// CRefString stored in pCurTargetUnit if there are no deleted CRefString instances,
	// but if there area deleted ones, the indices will not be in sync)
	int nSel;
	wxString str;
	nSel = m_pMyListBox->GetSelection();

    // get the selected string into str; it could be upper or lower case, even when auto
    // caps is ON, because the user could have made KB entries with auto caps OFF
    // previously. But we take the string as is, and don't make case conversions here
	str = m_pMyListBox->GetString(nSel); // the list box translation string being deleted
	wxString str2 = str;
	wxString message;
	if (str == s)
		str = _T(""); // for comparison's with what is stored in the KB, it must be empty

	// find the corresponding CRefString instance in the knowledge base, and set the
	// nPreviousReferences variable for use in the message box; if user hits Yes
	// then go ahead and do the removals.
    // Note: the global pCurTargetUnit is set to a target unit in either the glossing KB
    // (when glossing is ON) or to one in the normal KB when adapting, so we don't need to
    // test for the KB type here.
	// BEW 25Jun10, because of the possible presence of deletions, we must get pos by a
	// find rather than rely on the selection index
	int itemIndex = pCurTargetUnit->FindRefString(str);
	wxASSERT(itemIndex != wxNOT_FOUND);
	TranslationsList::Node* pos = pCurTargetUnit->m_pTranslations->Item(itemIndex);
	wxASSERT(pos != NULL);
	CRefString* pRefString = (CRefString*)pos->GetData();
	wxASSERT(pRefString != NULL);

    // check that we have the right reference string instance; we must allow for equality
    // of two empty strings to be considered to evaluate to TRUE
	if (str.IsEmpty() && pRefString->m_translation.IsEmpty())
	{
		// both empty, so they match
		;
	}
	else
	{
		if (str != pRefString->m_translation)
		{
			// message can be in English, it's never likely to occur
			wxMessageBox(_T(
"OnButtonRemove() error: Knowledge bases's adaptation text does not match that selected in the list box\n"),
			_T(""), wxICON_EXCLAMATION | wxOK);
		}
	}
	// get the ref count, use it to warn user about how many previous references
	// this will mess up
	int nPreviousReferences = pRefString->m_refCount;

	// warn user about the consequences, allow him to get cold feet & back out
	message = message.Format(_(
"Removing: \"%s\", will make %d occurrences of it in the document files inconsistent with the knowledge base.\n(You can fix that later by using the Consistency Check command.)\nDo you want to go ahead and remove it?"),
		str2.c_str(),nPreviousReferences);
	int response = wxMessageBox(message, _T(""), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
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
    // m_refCount will have been set to -1 if we remove the last thing in the list box, so
    // we have to detect that & change it to zero, otherwise bounds checking won't let us
    // exit the box
	if (m_refCount < 0)
		m_refCount = 0;
	if (str == s)
	{
		// if str was reset to "<no adaptation>" then make it empty for any kb check below
		str = _T("");
	}

	// in support of removal when autocapitalization might be on - see the OnButtonRemove handler
	// in KBEditor.cpp for a full explanation of the need for this flag
	gbCallerIsRemoveButton = TRUE;

	// BEW added 19Apr13 to support Remove button in ChooseTranslation.dlg and/or the
	// KBEditor.cpp's Remove button, when the user has the option of two lookup KB regimes
	if (gpApp->m_bDoLegacyLowerCaseLookup || gbAutoCaps == FALSE)
	{
		// This is the option, for versions prior to 6.4.2, if gbAutoCaps is TRUE then it
		// will ignore any pTU's for which the key string starts with an upper case
		// letter; of if gbAutoCaps is FALSE, it will do the lookup for the src key 'as is'

		// BEW added 19Feb13 for kbserver support
#if defined(_KBSERVER)
		if (gpApp->m_bIsKBServerProject &&
			gpApp->GetKbServer(gpApp->GetKBTypeForServer())->IsKBSharingEnabled())
		{
			KbServer* pKbSvr = gpApp->GetKbServer(gpApp->GetKBTypeForServer());

			// create the thread and fire it off
			if (!pCurTargetUnit->IsItNotInKB())
			{
				Thread_PseudoDelete* pPseudoDeleteThread = new Thread_PseudoDelete;
				// populate it's public members (it only has public ones anyway)
				pPseudoDeleteThread->m_pKbSvr = pKbSvr;
				pPseudoDeleteThread->m_source = curKey; // curKey is a global wxString
				pPseudoDeleteThread->m_translation = pRefString->m_translation;
				// now create the runnable thread with explicit stack size of 10KB
				wxThreadError error =  pPseudoDeleteThread->Create(10240);
				if (error != wxTHREAD_NO_ERROR)
				{
					wxString msg;
					msg = msg.Format(_T("Thread_PseudoDelete in ChooseTranslation::OnButtonRemove(): thread creation failed, error number: %d"),
						(int)error);
					wxMessageBox(msg, _T("Thread creation error"), wxICON_EXCLAMATION | wxID_OK);
					//m_pApp->LogUserAction(msg);
				}
				else
				{
					// no error, so now run the thread (it will destroy itself when done)
					error = pPseudoDeleteThread->Run();
					if (error != wxTHREAD_NO_ERROR)
					{
					wxString msg;
					msg = msg.Format(_T("Thread_PseudoDelete in ChooseTranslation::OnButtonRemove(), Thread_Run(): cannot make the thread run, error number: %d"),
					  (int)error);
					wxMessageBox(msg, _T("Thread start error"), wxICON_EXCLAMATION | wxID_OK);
					//m_pApp->LogUserAction(msg);
					}
				}
			}
		}
#endif

		// remove the corresponding CRefString instance from the knowledge base...
		// BEW 25Jun10, 'remove' now means, set m_bDeleted = TRUE, etc, and hide it in the GUI
		wxASSERT(pRefString != NULL);
		pRefString->SetDeletedFlag(TRUE);
		pRefString->GetRefStringMetadata()->SetDeletedDateTime(GetDateTimeNow());
		pRefString->m_refCount = 0;
	}
	else
	{
		// This is the 6.4.2 and later default situation -- it makes use of upper-case
		// initial letter key strings for AutoCapsLookup(), and so the Remove button must
		// remove from both the upper and lower case pTU instances when two such exist in
		// parallel (one for upper case initial key, the other for lower case initial key)
		RemoveParallelEntriesViaRemoveButton(m_pKB, m_nWordsInPhrase - 1, curKey, str);
	}

	// get the count of non-deleted CRefString instances for this CTargetUnit instance
	int numNotDeleted = pCurTargetUnit->CountNonDeletedRefStringInstances();

	// did we remove the last item in the box?
	if (numNotDeleted == 0)
	{
		// this means the pCurTargetUnit has no undeleted CRefString instances left

		// legacy code here has been omitted -- nothing to do in this block now

		// whm Note 3Aug06:
        // The user wanted to delete the translation from the KB, so it would be nice
        // if the the phrasebox did not show the removed translation again after
        // ChooseTranslation dialog exits, but a later call to PlacePhraseBox copies
        // the source again and echos it there. <<-- check if this still applies
		;
	}
    // do we need to show the Do Not Ask Again button? (BEW 28May10: yes, if the flag
    // is true, as in the test in next line)
	if ((numNotDeleted == 1) && pCurTargetUnit->m_bAlwaysAsk)
	{
		//wxWindow* pButton = FindWindowById(IDC_BUTTON_CANCEL_ASK);
		wxWindow* pButton = FindWindowById(ID_BUTTON_CANCEL_ASK);
		wxASSERT(pButton != NULL);
		pButton->Show(TRUE);
	}
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
	gbCallerIsRemoveButton = FALSE; // reestablish the safe default
    // If the last translation was removed set focus to the New Translation edit box,
    // otherwise to the Translations list box. (requested by Wolfgang Stradner).
	if (numNotDeleted == 0)
		m_pNewTranslationBox->SetFocus();
	else
		m_pMyListBox->SetFocus();
}

// BEW 25Jun10, changes needed for support of kbVersion 2
void CChooseTranslation::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	m_bEmptyAdaptationChosen = FALSE;
	m_bCancelAndSelect = FALSE;

	wxString s;
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
	m_pSourcePhraseBox->ChangeValue(curKey);

	// set the "new translation" edit box contents to a null string
	m_pNewTranslationBox->ChangeValue(_T(""));

	// set the list box contents to the translation or gloss strings stored
	// in the global variable pCurTargetUnit, which has just been matched
	// BEW 25Jun10, ignore any CRefString instances for which m_bDeleted is TRUE
	/*
	CRefString* pRefString;
	TranslationsList::Node* pos = pCurTargetUnit->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		pRefString = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		if (!pRefString->GetDeletedFlag())
		{
			// this one is not deleted, so show it to the user
			wxString str = pRefString->m_translation;
			if (str.IsEmpty())
			{
				str = s;
			}
			m_pMyListBox->Append(str);
			// m_pMyListBox is NOT sorted but it is safest to find the just-inserted
			// item's index before calling SetClientData()
			int nLocation = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
			wxASSERT(nLocation != -1); // we just added it so it must be there!
			m_pMyListBox->SetClientData(nLocation,&pRefString->m_refCount);
		}
	}
	*/
	PopulateList(pCurTargetUnit, 0, No);

	// select the first string in the listbox by default
	// BEW changed 3Dec12, if the list box is empty, the ASSERT below trips. So we need a test
	// that checks for an empty list, and sets nothing in that case
	if (!m_pMyListBox->IsEmpty())
	{
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
        {
            m_refCount = 0;
        }
	}

	// hide the Do Not Ask Again button if there is more than one item in the list
	int nItems = m_pMyListBox->GetCount();
	//wxWindow* pButton = FindWindowById(IDC_BUTTON_CANCEL_ASK);
	wxWindow* pButton = FindWindowById(ID_BUTTON_CANCEL_ASK);
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
		//wxButton* pButton = (wxButton*)FindWindowById(IDC_BUTTON_CANCEL_AND_SELECT);
		wxButton* pButton = (wxButton*)FindWindowById(ID_BUTTON_CANCEL_AND_SELECT);
		pButton->Show(FALSE);
	}

	//TransferDataToWindow(); // whm removed 21Nov11
	m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11

	// place the dialog window so as not to obscure things
	// work out where to place the dialog window
	gpApp->GetView()->AdjustDialogPosition(this);

	m_pMyListBox->SetFocus();

}

// pass 0 for selectionIndex if doSel has value No
void CChooseTranslation::PopulateList(CTargetUnit* pTU, int selectionIndex, enum SelectionWanted doSel)
{
	m_pMyListBox->Clear();
	wxString s = _("<no adaptation>");

	// set the list box contents to the translation or gloss strings stored
	// in the global variable pCurTargetUnit, which has just been matched
	// BEW 25Jun10, ignore any CRefString instances for which m_bDeleted is TRUE
	CRefString* pRefString;
	TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	while (pos != NULL)
	{
		pRefString = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		if (!pRefString->GetDeletedFlag())
		{
			// this one is not deleted, so show it to the user
			wxString str = pRefString->m_translation;
			if (str.IsEmpty())
			{
				str = s;
			}
			m_pMyListBox->Append(str);
			// m_pMyListBox is NOT sorted but it is safest to find the just-inserted
			// item's index before calling SetClientData()
			int nLocation = gpApp->FindListBoxItem(m_pMyListBox,str,caseSensitive,exactString);
			wxASSERT(nLocation != -1); // we just added it so it must be there!
			m_pMyListBox->SetClientData(nLocation,&pRefString->m_refCount);
		}
	}
	if (doSel == Yes)
	{
		m_pMyListBox->SetSelection(selectionIndex);
	}
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
			//wxButton* pButton = (wxButton*)FindWindowById(IDC_BUTTON_CANCEL_AND_SELECT);
			wxButton* pButton = (wxButton*)FindWindowById(ID_BUTTON_CANCEL_AND_SELECT);
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

void CChooseTranslation::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// don't need to do anything
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
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

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxUpdateUIEvent that is generated for the ChooseTranslation dialog's
///                         Idle handler
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism whenever the Choose Translation dialog is showing.
/// If the dialog's list box (m_pMyListBox) has at least one item in it the dialog's "Remove From KB"
/// button is enabled, otherwise it is disabled when the list is empty.
////////////////////////////////////////////////////////////////////////////////////////////
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

