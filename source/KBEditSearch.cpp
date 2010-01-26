/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBEditSearch.cpp
/// \author			Bruce Waters
/// \date_created	25 January 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License
///                 (see license directory)
/// \description	This is the implementation file for the KBEditSearch class. 
/// The KBEditSearch class provides a dialog interface for the user (typically an
/// administrator) to be able to move or copy files or folders or both from a source
/// location into a destination folder. 
/// \derivation The KBEditSearch class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////


// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBEditSearch.h"
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
#include <wx/valgen.h> // for wxGenericValidator
#include "Adapt_It.h"
#include "helpers.h" // it has the Get... functions for getting list of files, folders
					 // and optionally sorting
#include "Adapt_It_wdr.h" 

#include "KBEditor.h"
#include "TargetUnit.h"
#include "RefString.h"
#include "KB.h"
#include "KBEditSearch.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;
extern bool gbIsGlossing;
extern bool gbGlossingUsesNavFont;

// event handler table
BEGIN_EVENT_TABLE(KBEditSearch, AIModalDialog)

	EVT_INIT_DIALOG(KBEditSearch::InitDialog)
	EVT_BUTTON(wxID_OK, KBEditSearch::OnOK)
	EVT_BUTTON(ID_BUTTON_UPDATE, KBEditSearch::OnBnClickedUpdate)	
	EVT_BUTTON(ID_BUTTON_FIND_NEXT, KBEditSearch::OnBnClickedFindNext)
	EVT_BUTTON(ID_BUTTON_RESTORE, KBEditSearch::OnBnClickedRestoreOriginalSpelling)
	EVT_BUTTON(ID_BUTTON_ACCEPT_EDIT, KBEditSearch::OnBnClickedAcceptEdit)
	EVT_BUTTON(ID_BUTTON_REMOVE_UPDATE, KBEditSearch::OnBnClickedRemoveUpdate)
	EVT_BUTTON(wxID_CANCEL, KBEditSearch::OnBnClickedCancel)

	//EVT_SIZE(KBEditSearch::OnSize)

	EVT_LISTBOX(ID_LISTBOX_MATCHED, KBEditSearch::OnMatchListSelectItem)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_MATCHED, KBEditSearch::OnMatchListDoubleclickItem)
	EVT_LISTBOX(ID_LISTBOX_UPDATED, KBEditSearch::OnUpdateListSelectItem)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_UPDATED, KBEditSearch::OnUpdateListDoubleclickItem)

END_EVENT_TABLE()

KBEditSearch::KBEditSearch(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Edit Knowledge Base Items Matched In The Search"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	KBEditSearchFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

	pKBEditorDlg = (CKBEditor*)parent;
	m_pKB = NULL;
	m_pTUList = NULL;

	// the comparison functions, CompareMatchRecords() and CompareUpdateRecords(), each
	// returning int, are defined as global functions in Adapt_It.h and implemented in
	// Adapt_It.cpp 
	m_pMatchRecordArray = new KBMatchRecordArray(CompareMatchRecords);
	m_pUpdateRecordArray = new KBUpdateRecordArray(CompareUpdateRecords);
}

KBEditSearch::~KBEditSearch() // destructor
{
}

///////////////////////////////////////////////////////////////////////////////////
///
///    START OF GUI FUNCTIONS 
///
///////////////////////////////////////////////////////////////////////////////////



// InitDialog is method of wxWindow
void KBEditSearch::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	CAdapt_ItApp* pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(pApp != NULL);

	// set up pointers to interface objects
	m_pSrcPhraseBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_INFO_SOURCE);
	m_pSrcPhraseBox->SetValidator(wxGenericValidator(&m_strSourceText));
	m_pNumReferencesBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_INFO_REFS);
	m_pNumReferencesBox->SetValidator(wxGenericValidator(&m_strNumRefs));
	m_pLocalSearchBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_LOCAL_SEARCH);
	m_pLocalSearchBox->SetValidator(wxGenericValidator(&m_strLocalSearch));
	m_pEditBox = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_EDITBOX);
	m_pEditBox->SetValidator(wxGenericValidator(&m_strEditBox));

	m_pUpdateButton = (wxButton*)FindWindowById(ID_BUTTON_UPDATE);
	wxASSERT(m_pUpdateButton != NULL);
	m_pFindNextButton = (wxButton*)FindWindowById(ID_BUTTON_FIND_NEXT);
	wxASSERT(m_pFindNextButton != NULL);
	m_pRestoreOriginalSpellingButton = (wxButton*)FindWindowById(ID_BUTTON_RESTORE);
	wxASSERT(m_pRestoreOriginalSpellingButton != NULL);
	m_pAcceptEditButton = (wxButton*)FindWindowById(ID_BUTTON_ACCEPT_EDIT);
	wxASSERT(m_pAcceptEditButton != NULL);
	m_pRemoveUpdateButton = (wxButton*)FindWindowById(ID_BUTTON_REMOVE_UPDATE);
	wxASSERT(m_pRemoveUpdateButton != NULL);

	m_pMatchListBox = (wxListBox*)FindWindowById(ID_LISTBOX_MATCHED);
	wxASSERT(m_pMatchListBox != NULL);
	m_pUpdateListBox = (wxListBox*)FindWindowById(ID_LISTBOX_UPDATED);
	wxASSERT(m_pUpdateListBox != NULL);

	// start with lower buttons disabled (they rely on selections to become enabled)
	EnableUpdateButton(FALSE);
	EnableFindNextButton(FALSE);
	EnableRestoreOriginalSpellingButton(FALSE);
	EnableAcceptEditButton(FALSE);
	EnableRemoveUpdateButton(FALSE);


	// set fonts and directionality
	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pNavTextFont, m_pEditBox, m_pLocalSearchBox,
						m_pMatchListBox, m_pUpdateListBox, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pNavTextFont, m_pNumReferencesBox, NULL,
						NULL, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pSourceFont, m_pSrcPhraseBox, NULL,
						NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		pApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditBox, m_pLocalSearchBox, 
						m_pMatchListBox, m_pUpdateListBox, gpApp->m_pDlgGlossFont);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pNavTextFont, m_pNumReferencesBox, NULL,
						NULL, NULL, gpApp->m_pDlgGlossFont);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pSourceFont, m_pSrcPhraseBox, NULL,
						NULL, NULL, gpApp->m_pDlgSrcFont);
		#endif
	}
	else // adapting, or glossing but with default use of target text encoding for glosses
	{
		#ifdef _RTL_FLAGS
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pTargetFont, m_pEditBox, m_pLocalSearchBox,
						m_pMatchListBox, m_pUpdateListBox, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pNavTextFont, m_pNumReferencesBox, NULL,
						NULL, NULL, gpApp->m_pDlgGlossFont, gpApp->m_bNavTextRTL);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pSourceFont, m_pSrcPhraseBox, NULL,
						NULL, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
		#else // Regular version, only LTR scripts supported, so use default FALSE for last parameter
		pApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditBox, m_pLocalSearchBox, 
						m_pMatchListBox, m_pUpdateListBox, gpApp->m_pDlgTgtFont);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pNavTextFont, m_pNumReferencesBox, NULL,
						NULL, NULL, gpApp->m_pDlgGlossFont);
		pApp->SetFontAndDirectionalityForDialogControl(pApp->m_pSourceFont, m_pSrcPhraseBox, NULL,
						NULL, NULL, gpApp->m_pDlgSrcFont);
		#endif
	}

	// pKBEditorDlg is the pointer to the parent CKBEditor instance, set in the creator's block
	// Now get the pointer to the particular KB we are dealing with, and its list of
	// CTargetUnit pointers
	m_pKB = pKBEditorDlg->pKB;
	wxASSERT(!m_pKB != NULL);
	m_pTUList = m_pKB->m_pTargetUnits;
	wxASSERT(!m_pTUList != NULL);

	// *** TODO **** set up a progress dialog here



	// find the matches -- all search strings are tested against all of KB contents in the
	// m_pTUList list of CTargetUnit pointer instances
	SetupMatchList(	&pApp->m_arrSearches, m_pKB, m_pMatchRecordArray, &gbIsGlossing);


	//  *** TODO **** end the progress dialog here
	



}

void KBEditSearch::SetupMatchList(wxArrayString* pArrSearches,
					CKB* pKB, KBMatchRecordArray* pMatchRecordArray, bool* pbIsGlossing)
{
	bool bIsGlossing = *pbIsGlossing;
	size_t numSearchStrings = pArrSearches->GetCount();
	size_t subStrIndex;
	wxArrayPtrVoid arrSubStringSet; // array of numSearchStrings of wxArrayString for the 
								 // substrings tokenized from each of the user's search
								 // strings (each array of substrings must match, in order
								 // within any given adaptation (or gloss) string being
								 // tested for a match, for the test to succeed
	// create the substring arrays, and fill each with its list of substrings (we use
	// the helpers.cpp function, SmartTokenize(), using <space character> as delimiter
	for (subStrIndex = 0; subStrIndex < numSearchStrings; subStrIndex++)
	{
		wxArrayString* pSubStrArray = new wxArrayString;
		arrSubStringSet.Add(pSubStrArray);
		// get the substrings, don't bother to get the returned count
		wxString delim = _T(" ");
		SmartTokenize(delim, pArrSearches->Item(subStrIndex),(*pSubStrArray), FALSE); 
        // FALSE means don't store any empty substrings (even though there can't be any
        // such if we use a space as delimiter)
	}

	// set up an iterator that will iterate across all the maps (we have to do it this way
	// if we want to show the user the particular source text string which is associated
	// with a given matched adaptation (or gloss), since the m_pTargetUnits list has no
	// knowledge at all of which source text words/phrases go with the data it stores);
	// and the find each and every stored word or phrase, and against it test the
	// substrings for a match - if there are any matches, a KBMatchRecord pointer has to
	// be created on the heap, it's members filled out, and Add()ed to pMatchRecordArray -
	// it is the latter which, on return, is used to populate the "Matched" wxListBox
	// which the user sees (code for the following loop is plagiarized from DoKBExport()
	// which is a member function of CAdapt_ItView class, see Adapt_ItView.cpp)
	wxASSERT(pKB != NULL);
	wxString strNotInKB = _T("<Not In KB>");
	wxString key;
	KBMatchRecord* pMatchRec = NULL;
	CTargetUnit* pTU = NULL;
	CRefString* pRefStr = NULL;
	int nRefCount = 0;
	size_t numWords;
	wxString testStr; // string from each CRefString instance is put here prior to testing
					  // for any search substring matches within it (it could be a gloss,
					  // or an adaptation, depending on what kind of KB we are dealing with)
	MapKeyStringToTgtUnit::iterator iter;
	for (numWords = 1; numWords <= MAX_WORDS; numWords++)
	{
		if (bIsGlossing && numWords > 1)
			continue; // when dealing with a glossing KB, there is only one map used, so 
					  // we consider only the first map, the others are all empty
		if (pKB->m_pMap[numWords-1]->size() == 0) 
			continue; // a map with no content has nothing we could edit anyway in the
					  // m_pTargetUnits list
		else
		{
			iter = pKB->m_pMap[numWords-1]->begin(); // get first association pair
			do {
				testStr.Empty(); // make ready for a new value
				key = iter->first; // the source text (or gloss if bIsGlossing is TRUE) 
				pTU = (CTargetUnit*)iter->second;
				wxASSERT(pTU != NULL); 

				// get the reference strings in an inner loop
				TranslationsList::Node* posRef = NULL; 

				// if the data somehow got corrupted by a CTargetUnit being retained in the
				// list but which has an empty list of reference strings, this illegal
				// instance would cause a crash - so test for it and if such occurs, then
				// remove it from the list and then just continue looping
				if (pTU->m_pTranslations->IsEmpty())
				{
					// don't expect an empty list of CRefString* instances, but code defensively
					continue;
				}
				else
				{
					// there is at least one CRefString* instance in the list in pTU
					posRef = pTU->m_pTranslations->GetFirst(); 
					wxASSERT(posRef != NULL);
					pRefStr = (CRefString*)posRef->GetData();
					posRef = posRef->GetNext(); // prepare for possibility of another CRefString*
					testStr = pRefStr->m_translation;
					nRefCount = pRefStr->m_refCount;

					// reject empty strings - we can't match anything in one
					if (testStr.IsEmpty())
						continue;

					// reject any testStr which contains "<Not In KB>" (if present, it is
					// the only CRefString entry in the pTU, so only need test here)
					if (testStr.Find(strNotInKB) != wxNOT_FOUND)
					{
						// the entry has <Not In KB> so don't accept this string
						continue;
					}

					bool bSuccessfulMatch = TestForMatch(&arrSubStringSet, testStr); 
					if (bSuccessfulMatch)
					{
						pMatchRec = new KBMatchRecord;

						// fill it out
						pMatchRec->strOriginal = testStr; // adaptation, or gloss
						pMatchRec->nUpdateIndex = 0xFFFF; // as yet, undefined
						pMatchRec->nIndexToMap = numWords-1;
						pMatchRec->strMapKey = key;
						pMatchRec->pTU = pTU;
						pMatchRec->nRefStrIndex = nRefCount;

						// store it
						pMatchRecordArray->Add(pMatchRec);
					}

					// now deal with any additional CRefString instances within the same
					// CTargetUnit instance
					while (posRef != NULL)
					{
						pRefStr = (CRefString*)posRef->GetData();
						wxASSERT(pRefStr != NULL); 
						posRef = posRef->GetNext(); // prepare for possibility of yet another
						testStr = pRefStr->m_translation;
						nRefCount = pRefStr->m_refCount;

						// reject empty strings - we can't match anything in one
						if (testStr.IsEmpty())
							continue;

						bSuccessfulMatch = TestForMatch(&arrSubStringSet, testStr); 
						if (bSuccessfulMatch)
						{
							pMatchRec = new KBMatchRecord;

							// fill it out
							pMatchRec->strOriginal = testStr; // adaptation, or gloss
							pMatchRec->nUpdateIndex = 0xFFFF; // as yet, undefined
							pMatchRec->nIndexToMap = numWords-1;
							pMatchRec->strMapKey = key;
							pMatchRec->pTU = pTU;
							pMatchRec->nRefStrIndex = nRefCount;

							// store it
							pMatchRecordArray->Add(pMatchRec);
						}

					}

					// point at the next CTargetUnit instance, or at end() (which is NULL) if
					// completeness has been obtained in traversing the map 
					iter++;
				}
			} while (iter != pKB->m_pMap[numWords-1]->end());
		} // end of else block for test pTU->m_pTranslations->IsEmpty()
	} // end of numWords outer loop

	// delete the substring arrays from the heap
	for (subStrIndex = 0; subStrIndex < numSearchStrings; subStrIndex++)
	{
		wxArrayString* pSubStrArray = (wxArrayString*)arrSubStringSet.Item(subStrIndex);
		delete pSubStrArray;
	}

}

bool KBEditSearch::TestForMatch(wxArrayPtrVoid* pSubStringSet, wxString& testStr)
{
	// test for match of any line (all substrings in the line must match within testStr)
	// 
	size_t searchLineCount = pSubStringSet->GetCount();
	size_t subStringsCount;
	size_t lineIndex;
	size_t subStringIndex;
	wxArrayString* pSubStrArray;
	wxString subStr;
	wxString str;
	int subStrLen;
	int offset = wxNOT_FOUND;
	for (lineIndex = 0; lineIndex < searchLineCount; lineIndex++)
	{
		// start each line with the full testStr
		str = testStr; // make a new copy, as we'll modify str as we test for a match
					   // in the inner loop below
		// get the array of substrings for this search-line
		pSubStrArray = (wxArrayString*)pSubStringSet->Item(lineIndex);
		wxASSERT(pSubStrArray);
		subStringsCount = pSubStrArray->GetCount();

		// loop over the substrings which are to be searched for in str
		for (subStringIndex = 0; subStringIndex < subStringsCount; subStringIndex++)
		{
			// test the next substring for a match
			subStr = pSubStrArray->Item(subStringIndex);
			subStrLen = subStr.Len(); // substrings are never the empty string
			offset = str.Find(subStr);
			if (offset == wxNOT_FOUND)
			{
				// try next searchline string, we've not got a match
				break; 
			}
			else
			{
				// we matched at offset, so check if we are done
				if (subStringIndex == subStringsCount - 1)
				{
					// we matched them all, so return with TRUE
					return TRUE;
				}
				
                // we've not tested them all yet, so reduce the length of str by offset +
                // subStrLen and test for a match of the next substring in that, provided
                // it is not an empty string
				str = str.Mid(offset + subStrLen);
				if (str.IsEmpty())
				{
					// no possibility of a further match, so break out to try next search
					// line
					break;
				}
			} // end of else block for test if (offset == wxNOT_FOUND)
		} // end of inner loop which tests the series of substrings for each to match in 
		  // sequence within the copy of testStr
	} // end of outer loop which checks each of the user's typed search strings for a match

	// if control gets here, there was no match
	return FALSE;
}

void KBEditSearch::EnableUpdateButton(bool bEnableFlag)
{
	if (bEnableFlag)
		m_pUpdateButton->Enable(TRUE);
	else
		m_pUpdateButton->Enable(FALSE);
}

void KBEditSearch::EnableFindNextButton(bool bEnableFlag)
{
	if (bEnableFlag)
		m_pFindNextButton->Enable(TRUE);
	else
		m_pFindNextButton->Enable(FALSE);
}

void KBEditSearch::EnableRestoreOriginalSpellingButton(bool bEnableFlag)
{
	if (bEnableFlag)
		m_pRestoreOriginalSpellingButton->Enable(TRUE);
	else
		m_pRestoreOriginalSpellingButton->Enable(FALSE);
}

void KBEditSearch::EnableAcceptEditButton(bool bEnableFlag)
{
	if (bEnableFlag)
		m_pAcceptEditButton->Enable(TRUE);
	else
		m_pAcceptEditButton->Enable(FALSE);
}

void KBEditSearch::EnableRemoveUpdateButton(bool bEnableFlag)
{
	if (bEnableFlag)
		m_pRemoveUpdateButton->Enable(TRUE);
	else
		m_pRemoveUpdateButton->Enable(FALSE);
}

//void KBEditSearch::OnSize(wxSizeEvent& event)
//{
//	if (this == event.GetEventObject())
//	{
//	}
//	event.Skip();
//}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void KBEditSearch::OnOK(wxCommandEvent& event) 
{
	// *** TODO ***   update all the respelled adaptations or glosses from the list

	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void KBEditSearch::OnBnClickedCancel(wxCommandEvent& event)
{
	event.Skip();
}

void KBEditSearch::OnBnClickedUpdate(wxCommandEvent& WXUNUSED(event))
{
}

void KBEditSearch::OnBnClickedFindNext(wxCommandEvent& WXUNUSED(event))
{
}

void KBEditSearch::OnBnClickedRestoreOriginalSpelling(wxCommandEvent& WXUNUSED(event))
{
}

void KBEditSearch::OnBnClickedAcceptEdit(wxCommandEvent& WXUNUSED(event))
{
}

void KBEditSearch::OnBnClickedRemoveUpdate(wxCommandEvent& WXUNUSED(event))
{
}


void KBEditSearch::InsertInUpdateList(KBUpdateRecordArray* pMatchRecordArray, KBUpdateRecord* bRec, int index)
{
}

void KBEditSearch::OnMatchListSelectItem(wxCommandEvent& event)
{
}


void KBEditSearch::OnMatchListDoubleclickItem(wxCommandEvent& event)
{
}


void KBEditSearch::OnUpdateListSelectItem(wxCommandEvent& event)
{
}


void KBEditSearch::OnUpdateListDoubleclickItem(wxCommandEvent& event)
{
}



