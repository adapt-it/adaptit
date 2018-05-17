/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseTranslation.cpp
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \rcs_id $Id$
/// \copyright		2018 Bruce Waters, Bill Martin, SIL International
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
#include <wx/combo.h>
#include <wx/odcombo.h>


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
#include "MainFrm.h"
#include "Pile.h" // BEW added 12May18

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern bool	gbRTL_Layout;	// ANSI version is always left to right reading; this flag can only
                            // be changed in the NRoman version, using the extra Layout menu
extern bool gbVerticalEditInProgress;

extern int ID_PHRASE_BOX;

// ******************************************************************************************
// ************************* CChooseTranslation dialog class ********************************
// ******************************************************************************************

// event handler table for the CChooseTranslation dialog class
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
END_EVENT_TABLE()

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
	m_nWordsInPhrase = gpApp->m_pTargetBox->m_nWordsInPhrase; // RHS is a member of CPhraseBox

    wxASSERT( m_nWordsInPhrase <= MAX_WORDS && m_nWordsInPhrase > 0);
	if (gbIsGlossing)
	{
		m_pKB = gpApp->m_pGlossingKB;
	}
	else
	{
		m_pKB = gpApp->m_pKB;
	}
	m_pMap = m_pKB->m_pMap[m_nWordsInPhrase-1]; // whm note: m_pMap is not used anywhere within this CChooseTranslation class

	// Note: We don't need to set up any SetValidators for data transfer
	// in this class since all assignment of values is done in OnOK()
}

CChooseTranslation::~CChooseTranslation() // destructor
{

}

// BEW 25Jun10, changes needed for support of kbVersion 2
void CChooseTranslation::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	m_bEmptyAdaptationChosen = FALSE;
	//m_bCancelAndSelect = FALSE;

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

	// BEW 23Apr15, m_CurKey ( CPhraseBox wxString, set when the phrasebox lands at a location)
	// is used here. In the event that it is a key for a merged phrase, it may contain ZWSP
	// but we do not need to restore / for each such ZWSP in m_CurKey, if we are currently
	// supporting / as a word-breaking pseudo whitespace character, because the m_pSourcePhraseBox
	// is read-only. (The user can copy from there though, which would copy any ZWSP unconverted,
	// but we may not be able to do much about that.)

	// set the "matched source text" edit box contents
	m_pSourcePhraseBox->ChangeValue(gpApp->m_pTargetBox->m_CurKey);

	// set the "new translation" edit box contents to a null string
	m_pNewTranslationBox->ChangeValue(_T(""));

	// BEW 23Apr15 - if supporting / as a word-breaking character currently, we don't convert
	// any ZWSP to / in the list, because we don't edit the list directly. The m_pNewTranslationBox
	// box needs to show / between words, but we don't populate the box by any list selection - only 
	// by the user explicitly typing text there. If / is given this word-breaking status, then the
	// user should type / explicitly between words when he types into that box.
	PopulateList(gpApp->pCurTargetUnit, 0, No);

	// select the first string in the listbox by default
	// BEW changed 3Dec12, if the list box is empty, the ASSERT below trips. So we need a test
	// that checks for an empty list, and sets nothing in that case
	if (!m_pMyListBox->IsEmpty())
	{
		m_pMyListBox->SetSelection(0);
		wxString str = m_pMyListBox->GetStringSelection();
		int nNewSel = gpApp->FindListBoxItem(m_pMyListBox, str, caseSensitive, exactString);
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
	wxWindow* pButton = FindWindowById(ID_BUTTON_CANCEL_ASK);
	wxASSERT(pButton != NULL);
	if (nItems == 1 && gpApp->pCurTargetUnit->m_bAlwaysAsk)
	{
		// only one, so allow user to stop the forced ask by hitting the button, so show it
		pButton->Show(TRUE);

	}
	else
	{
		// more than one, so we don't want to see the button
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
// BEW 12May18 refactored (by an addition) to support consistency between dropdown listbox behaviour
// and the PopulateList() function. In PhraseBox.cpp, there is code to check for the phrasebox landing
// causing a CRefString with m_refCount set to 1, while being cleared from the KB, the adaptation to be
// retained in the dropdown list - so user can see it having it being auto-reinserted at the correct
// location according to the order of CRefStrings in the CTargetUnit instance. The legacy PopulateList()
// code below would only show non-deleted entries from the CTargetUnit. Since ChooseTranslation() is
// available along with the dropdown-list phrasebox, the user is gunna get confused if the dropdown
// has an entry which, if he chooses the ChooseTranslation() option, won't appear in the ChooseTranslation
// dialog's list. So, I need to refactor, to test for all the conditions satified that justify showing
// this dialog's list with the deleted adaptation (not gloss, I don't support doing this for glossing
// mode) in its proper place in the list. That will keep things in sync when the list in either place
// is viewed. Fortunately, all that I need to do is provide an else block for the test
// if (!pRefString->GetDeletedFlag()), and do the refactoring work in there. Unlike in the phrasebox 
// dropdown list, if we match the relevant removed adaptation here and restore it to the list, we
// will not show it selected to the user - because ChooseTranslation() dialog parallels code in
// EditKnowledgeBase and we don't want to stipulate what the user's choice is when the dialog shows.
void CChooseTranslation::PopulateList(CTargetUnit* pTU, int selectionIndex, enum SelectionWanted doSel)
{
	m_pMyListBox->Clear();
	wxString s = _("<no adaptation>");

	// set the list box contents to the translation or gloss strings stored
	// in the global variable pCurTargetUnit, which has just been matched
	// BEW 25Jun10, ignore any CRefString instances for which m_bDeleted is TRUE
	CRefString* pRefString;
	int nLocation = -1;
	wxString str = wxEmptyString;
	TranslationsList::Node* pos = pTU->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
//	bool bWasReinserted = FALSE;		// BEW 14May18 -- commented out, I may need it if I reinstate the code at 338++
	while (pos != NULL)
	{
		pRefString = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		if (!pRefString->GetDeletedFlag())
		{
			// this one is not deleted, so show it to the user
			str = pRefString->m_translation;
			if (str.IsEmpty())
			{
				str = s;
			}
			m_pMyListBox->Append(str);
			// m_pMyListBox is NOT sorted but it is safest to find the just-inserted
			// item's index before calling SetClientData()
			nLocation = gpApp->FindListBoxItem(m_pMyListBox, str, caseSensitive, exactString);
			wxASSERT(nLocation != -1); // we just added it so it must be there!
			m_pMyListBox->SetClientData(nLocation, &pRefString->m_refCount);
		}
/* BEW 14May18 This gets the temporarialy deleted adaptation shown in the list, but clicking it does nothing useful yet - I'm unsure whether to do this or not
		else // this else block deals with deleted CRefString instances only
		{
			// BEW 12May18 check for a deleted adaptation at the active location, provided not
			// glossing and various other constraints are satisfied. If so, restore it to the
			// list in the correct location it had before the box landed at the current location
			if (!gbIsGlossing && !gpApp->m_bFreeTranslationMode)
			{
				// So far so good. Next, check pSrcPhrase at the active location for felicity
				// conditions there which enable checking further here
				CSourcePhrase* pSrcPhrase = gpApp->m_pActivePile->GetSrcPhrase();
				if (!pSrcPhrase->m_bHasKBEntry && !pSrcPhrase->m_bNotInKB &&
					!pSrcPhrase->m_bNullSourcePhrase && !pSrcPhrase->m_bRetranslation)
				{
					// We know pRefString has its deleted flag set TRUE, so grab the m_refCount
					// and m_translation values. If m_refCount is 1 then this is a candidate
					// for being deleted at the landing of the phrasebox - check this first
					if (pRefString->m_refCount == 1)
					{
						// It's a candidate. It's a deleted one we want to restore if the
						// contents of it's m_translation member match the value in 
						// pSrcPhrase->m_adaption
						if (pSrcPhrase->m_adaption == pRefString->m_translation)
						{
							// Yep, this is one to be restored to the list. To do that right
							// we have to Append() it now, and set its nLocation
							str = pRefString->m_translation;
							if (str.IsEmpty())
							{
								str = s;
							}
							m_pMyListBox->Append(str);
							// m_pMyListBox is NOT sorted but it is safest to find the just-inserted
							// item's index before calling SetClientData()
							nLocation = gpApp->FindListBoxItem(m_pMyListBox, str, caseSensitive, exactString);
							wxASSERT(nLocation != -1); // we just added it so it must be there!
							m_pMyListBox->SetClientData(nLocation, &pRefString->m_refCount);

							bWasReinserted = TRUE; // we can use this for some good purpose if necessary
												   // but for now we won't use it to set selectionIndex
							//selectionIndex = nLocation;
						}
					}
				}
			}
		} // end of else block for test: if (!pRefString->GetDeletedFlag())
*/
	} // end of loop
	if (doSel == Yes)
	{
		m_pMyListBox->SetSelection(selectionIndex);
	}
}

void CChooseTranslation::OnButtonCancelAsk(wxCommandEvent& WXUNUSED(event))
{
	gpApp->pCurTargetUnit->m_bAlwaysAsk = FALSE;
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
	int numNotDeleted = gpApp->pCurTargetUnit->CountNonDeletedRefStringInstances(); // the visible ones
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
		nListIndex = gpApp->pCurTargetUnit->FindRefString(itemStr); // handles empty string correctly
		wxASSERT(nListIndex != wxNOT_FOUND);
		pos = gpApp->pCurTargetUnit->m_pTranslations->Item(nListIndex);
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
        gpApp->pCurTargetUnit->m_pTranslations->DeleteNode(posOld);
		wxASSERT(pos != NULL);

        // now do the insertion, bringing the pCurTargetUnit's list into line with what the
        // listbox in the GUI shows to the user
        // Note: wxList::Insert places the item before the given item and the inserted item
        // then has the pos node position.
		pos = gpApp->pCurTargetUnit->m_pTranslations->Insert(pos,pRefString);
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
		PopulateList(gpApp->pCurTargetUnit, nSel, Yes);
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
	int numNotDeleted = gpApp->pCurTargetUnit->CountNonDeletedRefStringInstances(); // the visible ones
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
		nListIndex = gpApp->pCurTargetUnit->FindRefString(itemStr); // handles empty string correctly
		wxASSERT(nListIndex != wxNOT_FOUND);
		pos = gpApp->pCurTargetUnit->m_pTranslations->Item(nListIndex);
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
            gpApp->pCurTargetUnit->m_pTranslations->Append(pRefString);
		}
		else
		{
			// we are at a CRefString instance, so we can insert before it
            gpApp->pCurTargetUnit->m_pTranslations->Insert(pos,pRefString);
		}
		// delete the node containing the old location's instance
        gpApp->pCurTargetUnit->m_pTranslations->DeleteNode(posOld);

		// check the insertion or append got done right, a simple message will do (in
		// English) for the developer if it didn't work - this error is unlikely to ever
		// happen
		pos = gpApp->pCurTargetUnit->m_pTranslations->Find(pRefString);
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
		PopulateList(gpApp->pCurTargetUnit, nSel, Yes);
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
	CAdapt_ItApp* pApp = &wxGetApp();
	wxASSERT(pApp != NULL);

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
        pApp->m_pTargetBox->m_bEmptyAdaptationChosen = TRUE; // this will be used to set the
								// m_bEmptyAdaptationChosen used by PlacePhraseBox
	}
	m_chosenTranslation = str;
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
    EndModal(wxID_OK); //EndDialog(IDOK);
    // whm Correction 12Jan09 - The zero parameter given previously to EndModal(0) was incorrect. The
    // parameter needs to be the value that gets returned from the ShowModal() being invoked on this
    // dialog - which in the case of a double-click, should be wxID_OK.

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
	int itemIndex = gpApp->pCurTargetUnit->FindRefString(str);
	wxASSERT(itemIndex != wxNOT_FOUND);
	TranslationsList::Node* pos = gpApp->pCurTargetUnit->m_pTranslations->Item(itemIndex);
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
    CKB* pKB;
    if (gbIsGlossing)
        pKB = gpApp->m_pGlossingKB;
    else
        pKB = gpApp->m_pKB;
    if (pKB != NULL)
        pKB->m_bCallerIsRemoveButton = TRUE;

	// BEW added 19Feb13 for kbserver support
#if defined(_KBSERVER)
    //  GDLC 20JUL16 Use KbAdaptRunning() and KbGlossRunning()to avoid crashes if a previously
    //  used KB Server happens to be unavailable now
    if ((gpApp->m_bIsKBServerProject && gpApp->KbAdaptRunning() )
        ||
        (gpApp->m_bIsGlossingKBServerProject && gpApp->KbGlossRunning() ) )
	{
		KbServer* pKbSvr = gpApp->GetKbServer(gpApp->GetKBTypeForServer());

		if (!gpApp->pCurTargetUnit->IsItNotInKB())
		{
			int rv = pKbSvr->Synchronous_PseudoDelete(pKbSvr, gpApp->m_pTargetBox->m_CurKey, pRefString->m_translation);
			wxUnusedVar(rv);
		}
	}
#endif
	// remove the corresponding CRefString instance from the knowledge base...
	// BEW 25Jun10, 'remove' now means, set m_bDeleted = TRUE, etc, and hide it in the GUI
	wxASSERT(pRefString != NULL);
	pRefString->SetDeletedFlag(TRUE);
	pRefString->GetRefStringMetadata()->SetDeletedDateTime(GetDateTimeNow());
	pRefString->m_refCount = 0;

	// get the count of non-deleted CRefString instances for this CTargetUnit instance
	int numNotDeleted = gpApp->pCurTargetUnit->CountNonDeletedRefStringInstances();

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
	if ((numNotDeleted == 1) && gpApp->pCurTargetUnit->m_bAlwaysAsk)
	{
		//wxWindow* pButton = FindWindowById(IDC_BUTTON_CANCEL_ASK);
		wxWindow* pButton = FindWindowById(ID_BUTTON_CANCEL_ASK);
		wxASSERT(pButton != NULL);
		pButton->Show(TRUE);
	}
	//TransferDataToWindow(); // whm removed 21Nov11
	m_pEditReferences->ChangeValue(m_refCountStr); // whm added 21Nov11
    if (pKB != NULL)
        pKB->m_bCallerIsRemoveButton = FALSE; // reestablish the safe default
    // If the last translation was removed set focus to the New Translation edit box,
    // otherwise to the Translations list box. (requested by Wolfgang Stradner).
	if (numNotDeleted == 0)
		m_pNewTranslationBox->SetFocus();
	else
		m_pMyListBox->SetFocus();
}



// whm 24Feb2018 removed - not needed with dropdown integrated in CPhraseBox class
//void CChooseTranslation::OnButtonCancelAndSelect(wxCommandEvent& event)
//{
//	CAdapt_ItApp* pApp = &wxGetApp();
//	wxASSERT(pApp != NULL);
//	//m_bCancelAndSelect = TRUE; // alter behaviour when Cancel is wanted (assume user wants instead
//								// to create a merge of the source words at the selection location)
//	pApp->m_curDirection = toright; // make sure any left-direction earlier drag selection
//									// is cancelled; & BEW 2Oct13 changed from right to toright 
//									// due to ambiguity
//	OnCancel(event);
//}

// whm 24Feb2018 removed - not needed with dropdown integrated in CPhraseBox class
//void CChooseTranslation::OnKeyDown(wxKeyEvent& event)
//// applies only when adapting; when glossing, just immediately exit
//{
//	if (gbIsGlossing) return; // glossing being ON must not allow this shortcut to work
//	if (event.AltDown())
//	{
//		// ALT key is down, so if right arrow key pressed, interpret the ALT key press as
//		// a click on "Cancel And Select" button
//		if (event.GetKeyCode() == WXK_RIGHT)
//		{
//			wxButton* pButton = (wxButton*)FindWindowById(ID_BUTTON_CANCEL_AND_SELECT);
//			if (pButton->IsShown())
//			{
//				wxCommandEvent evt = wxID_CANCEL;
//				OnButtonCancelAndSelect(evt);
//				return;
//			}
//			else
//				return; // ignore the keypress when the button is invisible
//		}
//	}
//}

void CChooseTranslation::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// don't need to do anything except
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

		// BEW added 16May18 to support getting this new adaptation into the dropdown 
		// phrasebox's list - we use it to provide a new processing path in the caller
		// which is OnButtonChooseTranslation(), because the legacy path amongst other 
		// things (mostly unnecessary or unhelpful) get PlacePhraseBox() called which
		// was particular unfortunate because it treated the opening of the ChooseTranslation()
		// dialog as a 'landing' event - which then immediately removed the new CRefString
		// for the new meaning typed, thereby undoing what we wanted to do - that is, get
		// the new meaning into the dropdown list. So we'll do differently with this
		// newly added app boolean. Note, if the user chooses a value from the current
		// ChooseTranslation() dialog's list - Bill's legacy code should then apply
		// as control will not enter this comment's block to set this new bool TRUE
		pApp->m_bTypedNewAdaptationInChooseTranslation = TRUE;
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
                pApp->m_pTargetBox->m_bEmptyAdaptationChosen = TRUE; // this will be used to set the
										// m_bEmptyAdaptationChosen used by PlacePhraseBox
			}

			m_chosenTranslation = str;
		}
	}
	pApp->m_pTargetBox->m_bAbandonable = FALSE;

//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15 - in case the user typed a translation manually (with / as word-delimiter)
	// convert any / back to ZWSP, in case KB storage follows. If the string ends up in m_targetBox
	// then the ChangeValue() call within CPhraseBox will convert the ZWSP instances back to forward
	// slashes for display, in case the user does subsequent edits there
	m_chosenTranslation = FwdSlashtoZWSP(m_chosenTranslation);
//#endif

    // whm notes 10Jan2018 to support quick selection of a translation equivalent.
    // The m_chosenTranslation string that is set above was initialized to empty
    // by the calling routines - the View's OnButtonChooseTranslation(), and the now 
    // unused CPhraseBox::ChooseTranslation(). The m_chosenTranslation is used by those
    // calling routines within their if(dlg.ShowModal() == wxID_OK) blocks to assign
    // the value of m_chosenTranslation to the global string m_Translation. Hence,
    // as soon as this dialog is dismissed via OnOK(), the m_Translation string will
    // be updated with any new translation the user entered. Back in the 
    // CPhraseBox::PopulateDropDownList() function, PopulateDropDownList() will 
    // add/append that new translation to the dropdown's list. 
    // If the user simply hit OK with a list item selected, PopulateDropDownList()
    // ensures that same item is selected. The user may have rearranged or deleted 
    // item(s) from the translations list before clicking the dialog's OK button. 
    // The dropdown list changes to be like the dialog's list (with the addition of
    // any new translation typed in the dialog) and the dropdown will automatically 
    // show and popup its list when this dialog is dismissed.
	// BEW 14May18 the above comment of 10Jan is what would be good to happen, but since then
	// someone or Bill has removed the capacity to implement the above protocol. So I'll have
	// a go here. First, I'll try getting m_chosenTranslation into m_pTargetBox's wxTextEdit
	// member, replacing whatever currently was there. We'll not try to support an empty string
	// from here.
	if (!m_chosenTranslation.IsEmpty() && pApp->m_bTypedNewAdaptationInChooseTranslation)
	{
		//pApp->m_pTargetBox->m_Translation = m_chosenTranslation; // avoid, otherwise PopulateDropDownList() will enter 
			// the last block which tests for m_Translation being non-empty and we don't want/need that to happen;
			// besides, for legacy situations the caller will set m_Translation to m_chosenTranslation anyway, but
			// if app's new boolean, m_bTypedNewAdaptationInChooseTranslation is TRUE, we will provide a new path
			// in OnButtonChooseTranslation() that only does the necessary minimal stuff - since the choose translation
			// dialog can only be opened at the active location (I think the legacy code does a lot of unneeded calls
			// at the end of OnButtonChooseTranslation() - Invalidate() is needed to repaint after the dlg closes, but
			// PlacePhraseBox(), PlaceBox(), ScrollIntoView(), RemoveRefString() not so - there's no location change
			// in the phrasebox.
		pApp->m_targetPhrase = m_chosenTranslation;
		wxTextEntry* pTextEntry = pApp->m_pTargetBox->GetTextCtrl();
		// check what's in it currently
		wxString currentStr = pTextEntry->GetValue();
		wxLogDebug(_T("pTextEntry->GetValue(),  returns currentStr:  %s"), currentStr.c_str());
		pApp->m_pTargetBox->SetModify(TRUE);

		// Get the chosen translation string into the dropdown combobox's text entry control
		pTextEntry->ChangeValue(m_chosenTranslation); // doesn't generate a wxEVT_TEXT event
#if defined (_DEBUG)
		wxLogDebug(_T("m_pTargetBox->GetModify()  returns 1 or 0: value is  %d"), (int)pApp->m_pTargetBox->GetModify());
		// check if it got entered
		wxString updatedStr = pTextEntry->GetValue();
		wxLogDebug(_T("pTextEntry->GetValue() returns updatedStr:  %s"), updatedStr.c_str());
#endif
		// Algorithm?  Try just deleting and remaking the list here; may be best to do a StoreText() call, 
		// and then the rebuilding may go easier - append to end of pTU list, and that will get the new 
		// meaning into the shared KB if KB Sharing is turned on, as well, and other useful magic

		// ========================BEW working on the above on 15May18 ======================================
		CPile* pActivePile = gpApp->m_pActivePile;
		CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
		wxASSERT(pSrcPhrase != NULL);
		CKB* pKB = m_pKB;
		CTargetUnit* pTargetUnit = (CTargetUnit*)NULL;
		pApp->m_pTargetBox->m_nWordsInPhrase = 0; // initialize
		pApp->m_pTargetBox->m_nWordsInPhrase = pSrcPhrase->m_nSrcWords;

		// Do this only when in adapting mode; and we don't support empty string adaptations here
		if (!gbIsGlossing && !m_chosenTranslation.IsEmpty() && !pKB->IsThisAGlossingKB())
		{
			bool bSupportNoAdaptationButton = FALSE; // not supporting <no adaptation> choice in Choose Translation dlg

			// Handy to do StoreText() here, as we get a lot for free. In particular, KBserver support, and especially
			// guaranteed appending of the new non-empty adaptation to the appropriate CTargetUnit instance, as well
			// as the on-the-side magic of getting pSrcPhrase's m_adaption and m_targetStr members set, a m_refCount
			// value of 1 (hence greater than zero) for use when building the new dropdown list, and a guaranteed
			// valid pointer to the appropriate CTargetUnit instance
			pKB->StoreText(pSrcPhrase, m_chosenTranslation, bSupportNoAdaptationButton);

			// Plagiarizing code from SetupDropDownComboBox() in what follows, from PhraseBox.cpp ...

			// Get a pointer to the target unit for the current key - could be NULL
			pTargetUnit = pKB->GetTargetUnit(pApp->m_pTargetBox->m_nWordsInPhrase, pSrcPhrase->m_key);

			// Get a count of the number of non-deleted ref string instances for the target unit
			// (as augmented by the StoreText() giving storage of a new CRefString with m_refCount of 1)
			int nRefStrCount = 0; // initialize
			bool bNoAdaptationFlagPresent = FALSE; // the PopulateDropDownList() function may change this value
			int indexOfNoAdaptation = -1; // the PopulateDropDownList() function may change this value
			if (pTargetUnit != NULL)
			{
				nRefStrCount = pTargetUnit->CountNonDeletedRefStringInstances();
#ifdef _DEBUG
				wxLogDebug(_T("ChooseTranslation.cpp OnOK(), line  %d ,nRefStringCount (non-deleted ones) = %d"), 1199, nRefStrCount);
#endif
			}
			// Close the dropdown list if not already closed, then clobber its contents
			if (pApp->m_pTargetBox->IsPopupShown())
			{
				pApp->m_pTargetBox->CloseDropDown();
			}
			pApp->m_pTargetBox->ClearDropDownList();

			// Now populate the dropdown list again, from the pCurTargetUnit pointer, and set the
			// selectionIndex to point at the last item in the list
			if (nRefStrCount > 0)
			{
				int selectionIndex = -1; // initialize, it will be set from the PopulateDropDownList()
						// call, the adaptation stored in pApp->m_targetPhrase, being matched against
						// the list contents generated in that call's internal loop of CRefString
						// instances, i.e. their m_translation member values. (The new adaptation
						// stored in the pTargetUnit list is guaranteed to be last in the list
						// because the StoreText() will have appended, and the user has had no
						// chance to move it up or down)
				if (pTargetUnit != NULL)
				{
					pApp->m_pTargetBox->PopulateDropDownList(pTargetUnit, selectionIndex, bNoAdaptationFlagPresent, indexOfNoAdaptation);
					wxASSERT(selectionIndex == nRefStrCount - 1); // verify it is indeed last in the list
				}
				// set the icon button for the phrasebox
				pApp->m_pTargetBox->SetButtonBitmaps(pApp->m_pTargetBox->dropbutton_normal, false, pApp->m_pTargetBox->dropbutton_pressed, pApp->m_pTargetBox->dropbutton_hover, pApp->m_pTargetBox->dropbutton_disabled);

				// Set the dropdown's list selection to the selectionIndex determined by PopulatDropDownList above.
				// If selectionIndex is -1, it removes any list selection from dropdown list
				// Note: SetSelection() with a single parameter operates to select/highlight the
				// dropdown's list item at the designated selectionIndex parameter. The
				// SetSelection() with two parameters operates to select a substring of text
				// within the dropdown's edit box, delineated by the two parameters.
				pApp->m_pTargetBox->SetSelection(selectionIndex);
				// whm 7Mar2018 addition - SetSelection() highlights the item in the list, and it
				// also has a side effect in that it automatically copies the item string from the 
				// dropdown list (matching the selectionIndex) into the dropdown's edit box.

				// Note: at this point no attempt has been made to capitalize the new list entry;
				// if gbAutoCaps is TRUE, adjusting for case for what is in the dropdown's textEdit
				// could be done here - but it may not be needed, adjustment on moving the phrasebox
				// should happen, if necessary, without further intervention

				//pApp->m_pTargetBox->SetFocus(); BEW removed because this call always fails, even though wx's window.cpp is found, etc
				// The PopulateDropDownList() function determined a selectionIndex to use
				int index = selectionIndex;
				pApp->m_targetPhrase = pApp->m_pTargetBox->GetString(index);
				pApp->m_pTargetBox->ChangeValue(pApp->m_targetPhrase);
				pApp->m_pTargetBox->SetSelection(index);
				pApp->m_pTargetBox->SetSelection(-1, -1); // select all
				// This next line is essential. Without it, the phrasebox will seem right, but moving away by
				// a click or by Enter key will leave a hole at the old location - the reason is that the
				// PlacePhraseBox() call uses m_pTargetBox->m_SaveTargetPhrase to put the old location's
				// adaptation into m_targetPhrase, and into m_pTargetBox; so if left empty, a hole is left
				// when the phrasebox moves off somewhere else -- nope, this didn't fix it, but is needed
				pApp->m_pTargetBox->m_SaveTargetPhrase = m_chosenTranslation; // didn't fix it either- see below
				// Checking CCell::Draw() and it's call of DrawCell() at the relevant location revealed
				// that pSrcPhrase's m_targetStr member is empty, and it is that member which gets
				// passed to pPhrase within DrawCell() for drawing in the GUI. So it draws nothing, leaving
				// an apparent hole.
#if defined (_DEBUG)
				int another_nRefStrCount = pTargetUnit->CountNonDeletedRefStringInstances();
				wxLogDebug(_T("ChooseTranslation.cpp OnOK() at end, line  %d ,nRefStringCount (non-deleted ones) = %d"), 1265, another_nRefStrCount);
				wxLogDebug(_T("ChooseTranslation.cpp OnOK() at end, line  %d , m_targetStr= %s"), 1266, pSrcPhrase->m_targetStr.c_str());
#endif
				// Clear the target unit pointer (unnecesary, it's an auto variable anyway)
				pTargetUnit = (CTargetUnit*)NULL;
			} // end TRUE block for test: if (nRefStrCount > 0)

		}
		// ==============================================================================
	}

	event.Skip();
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

