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
	EVT_BUTTON(ID_BUTTON_REMOVE_UPDATE, KBEditSearch::OnBnClickedRemoveUpdate)
	EVT_BUTTON(wxID_CANCEL, KBEditSearch::OnBnClickedCancel)
	EVT_TEXT_ENTER(ID_TEXTCTRL_EDITBOX, KBEditSearch::OnEnterInEditBox)
	EVT_TEXT(ID_TEXTCTRL_LOCAL_SEARCH, OnChangeLocalSearchText)
	EVT_LISTBOX(ID_LISTBOX_MATCHED, KBEditSearch::OnMatchListSelectItem)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_MATCHED, KBEditSearch::OnMatchListDoubleclickItem)
	EVT_LISTBOX(ID_LISTBOX_UPDATED, KBEditSearch::OnUpdateListSelectItem)
	EVT_LISTBOX_DCLICK(ID_LISTBOX_UPDATED, KBEditSearch::OnUpdateListDoubleclickItem)

END_EVENT_TABLE()

KBEditSearch::KBEditSearch(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Respell or Inspect Matched Knowledge Base Items"),
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
	m_pMatchStrArray = new wxArrayString;
	//m_pUpdateStrArray = new wxArrayString;

	// now a dummy match record on heap, in case the match list is empty
	// ~KBEditSearch() will delete it
	m_pDummyMatchRecord = new KBMatchRecord;
	wxString msg = _("No Matches");
	m_pDummyMatchRecord->strOriginal = msg;

}

KBEditSearch::~KBEditSearch() // destructor
{
	// prevent leaks, clear out the structs and their storage arrays
	size_t count = m_pMatchRecordArray->GetCount();
	size_t index;
	KBMatchRecord* pMR = NULL;
	if (m_bMatchesExist)
	{
		// the m_pDummyMatchRecord is still on heap and will not have been
		// used, so delete it here; but if there were no matches, then the
		// next block below will delete it instead
		delete m_pDummyMatchRecord;
	}
	if (!m_pMatchRecordArray->IsEmpty())
	{
		for (index = 0; index < count; index++)
		{
			pMR = m_pMatchRecordArray->Item(index);
			delete pMR;
		}
	}
	else

	delete m_pMatchRecordArray;

	count = m_pUpdateRecordArray->GetCount();
	KBUpdateRecord* pUR = NULL;
	if (!m_pUpdateRecordArray->IsEmpty())
	{
		for (index = 0; index < count; index++)
		{
			pUR = m_pUpdateRecordArray->Item(index);
			delete pUR;
		}
	}
	delete m_pUpdateRecordArray;

	m_pMatchStrArray->Clear();
	delete m_pMatchStrArray;
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
	wxASSERT(m_pKB != NULL);
	m_pTUList = m_pKB->m_pTargetUnits;
	wxASSERT(m_pTUList != NULL);

	m_pMatchListBox->Clear();
	m_pUpdateListBox->Clear();

	// *** TODO **** set up a progress dialog here



	// find the matches -- all search strings are tested against all of KB contents in the
	// m_pTUList list of CTargetUnit pointer instances
	SetupMatchArray(&pApp->m_arrSearches, m_pKB, m_pMatchRecordArray, &gbIsGlossing);

	// populate the Matched array
	m_bMatchesExist = PopulateMatchLabelsArray(m_pMatchRecordArray, m_pMatchStrArray);
	PopulateMatchedList(m_pMatchStrArray, m_pMatchRecordArray, m_pMatchListBox);



	//  *** TODO **** end the progress dialog here

}

bool KBEditSearch::PopulateMatchLabelsArray(KBMatchRecordArray* pMatchRecordArray, 
											wxArrayString* pMatchStrArray)
{
	size_t count = pMatchRecordArray->GetCount();
	size_t index;
	bool returnValue;
	if (count > 0)
	{
		for (index = 0; index < count; index++)
		{
			KBMatchRecord* pMR = pMatchRecordArray->Item(index);
			pMatchStrArray->Add(pMR->strOriginal);
		}
		returnValue = TRUE;
	}
	else
	{
		// put "No Matches" label, and the dummy record as its "data"
		pMatchRecordArray->Add(m_pDummyMatchRecord);
		pMatchStrArray->Add(m_pDummyMatchRecord->strOriginal); // "No Match" is label
		// the return value of FALSE is important - it tells the caller not to treat the
		// data in these two arrays as real data, since we just use it to inform user of
		// the lack of any match, and so we need a way to disable buttons
		returnValue = FALSE;
	}
	return returnValue;
}


// return FALSE if there was nothing matched, TRUE if one or more matches were made (that
// is, if pMatchRecordArray has an item count of 1 or greater)
void KBEditSearch::PopulateMatchedList(wxArrayString* pMatchStrArray, 
					KBMatchRecordArray* pMatchRecordArray, wxListBox* pListBox)
{
	pListBox->Set((*pMatchStrArray),(void**)&pMatchRecordArray);
}


void KBEditSearch::SetupMatchArray(wxArrayString* pArrSearches,
					CKB* pKB, KBMatchRecordArray* pMatchRecordArray, bool* pbIsGlossing)
{
	#ifdef __WXDEBUG__
	wxLogDebug(_T("Start SetupMatchArray()\n"));
	int anItemsCount = 0;
	#endif
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
//	int nTUListNodeIndex;
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
					iter++;
					continue;
				}
				else
				{
					// there is at least one CRefString* instance in the list in pTU
					posRef = pTU->m_pTranslations->GetFirst(); 
					wxASSERT(posRef != NULL);
					pRefStr = (CRefString*)posRef->GetData();
					//nTUListNodeIndex = pTU->m_pTranslations->IndexOf(pRefStr);
					posRef = posRef->GetNext(); // prepare for possibility of another CRefString*
					testStr = pRefStr->m_translation;

					// reject empty strings - we can't match anything in one
					if (testStr.IsEmpty())
					{
						iter++;
						continue;
					}

					// reject any testStr which contains "<Not In KB>" (if present, it is
					// the only CRefString entry in the pTU, so only need test here)
					int anOffset = testStr.Find(strNotInKB);
					if (anOffset != wxNOT_FOUND)
					{
						// the entry has <Not In KB> so don't accept this string
						iter++;
						continue;
					}

					#ifdef __WXDEBUG__
					wxLogDebug(_T("KB (map=%d):  %s"),numWords,testStr.c_str());
					anItemsCount++;
					#endif __WXDEBUG__

					bool bSuccessfulMatch = TestForMatch(&arrSubStringSet, testStr); 
					if (bSuccessfulMatch)
					{
						pMatchRec = new KBMatchRecord;
						#ifdef __WXDEBUG__
						wxLogDebug(_T("Matched:  %s"),testStr.c_str());
						#endif __WXDEBUG__

						// fill it out
						pMatchRec->strOriginal = testStr; // adaptation, or gloss
						pMatchRec->pUpdateRecord = NULL; // as yet, undefined
						//pMatchRec->nIndexToMap = numWords-1;
						pMatchRec->strMapKey = key;
						//pMatchRec->pTU = pTU;
						//pMatchRec->nRefStrIndex = nTUListNodeIndex;
						pMatchRec->pRefString = pRefStr;

						// store it
						pMatchRecordArray->Add(pMatchRec);
					}

					// now deal with any additional CRefString instances within the same
					// CTargetUnit instance
					while (posRef != NULL)
					{
						pRefStr = (CRefString*)posRef->GetData();
						//nTUListNodeIndex = pTU->m_pTranslations->IndexOf(pRefStr);
						wxASSERT(pRefStr != NULL); 
						posRef = posRef->GetNext(); // prepare for possibility of yet another
						testStr = pRefStr->m_translation;

						// reject empty strings - we can't match anything in one
						if (testStr.IsEmpty())
						{
							iter++;
							continue;
						}

						#ifdef __WXDEBUG__
						wxLogDebug(_T("KB (map=%d):  %s"),numWords,testStr.c_str());
						anItemsCount++;
						#endif __WXDEBUG__

						bSuccessfulMatch = TestForMatch(&arrSubStringSet, testStr); 
						if (bSuccessfulMatch)
						{
							pMatchRec = new KBMatchRecord;
							#ifdef __WXDEBUG__
							wxLogDebug(_T("Matched:  %s"),testStr.c_str());
							#endif __WXDEBUG__
							// fill it out
							pMatchRec->strOriginal = testStr; // adaptation, or gloss
							pMatchRec->pUpdateRecord = NULL; // as yet, undefined
							//pMatchRec->nIndexToMap = numWords-1;
							pMatchRec->strMapKey = key;
							//pMatchRec->pTU = pTU;
							//pMatchRec->nRefStrIndex = nTUListNodeIndex;
							pMatchRec->pRefString = pRefStr;

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

	if (m_pMatchRecordArray->IsEmpty())
	{
		EnableFindNextButton(FALSE);
	}
	else
	{
		EnableFindNextButton(TRUE);
	}

	#ifdef __WXDEBUG__
	wxLogDebug(_T("Items count:   %d"), anItemsCount);
	wxLogDebug(_T("End SetupMatchArray()\n"));
	#endif
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

void KBEditSearch::EnableRemoveUpdateButton(bool bEnableFlag)
{
	if (bEnableFlag)
		m_pRemoveUpdateButton->Enable(TRUE);
	else
		m_pRemoveUpdateButton->Enable(FALSE);
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void KBEditSearch::OnOK(wxCommandEvent& event) 
{
	if (m_bMatchesExist)
	{
        // Update, in the in-memory KB, all the respelled adaptations or glosses from the
        // list if any. Returning to the parent dialog's caller, the OnButtonGo() handler,
        // the rest of the tidy up is done there - which includes puting the search strings
        // (line by line) into the combobox of the parent for potential reuse by the user,
        // clearing the parent's m_arrSearches array (the combobox, and the m_pEditSearches
        // wxTextCtrl, use the Adapt_It.h wxArrayString members, m_arrOldSearches, and
        // m_arrSeaches, respectively), and a forced full KB update to disk to make the
        // changes persistent (the user may otherwise think they have been made persistent
        // and Cancel from the parent, and thus lose the spelling updates, so we prevent
        // this by obligatorily doing the save to disk)
		wxListBox* pLB = m_pUpdateListBox;
		wxASSERT(pLB);
		unsigned int count = pLB->GetCount();
		int index = -1;
		if (count > 0)
		{
			for (index = 0; index < (int)count; index++)
			{
				m_pCurKBUpdateRec = (KBUpdateRecord*)pLB->GetClientData(index);
				m_nCurMatchListIndex = m_pCurKBUpdateRec->nMatchRecordIndex;
				m_pCurKBMatchRec = m_pMatchRecordArray->Item(m_nCurMatchListIndex);

				// use the KBMatchRecord to update the spelling stored on the CRefString
				// instance in the CTargetUnit instance
				m_pCurKBMatchRec->pRefString->m_translation = m_pCurKBUpdateRec->updatedString;
			}
		}
	}
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}

void KBEditSearch::OnBnClickedCancel(wxCommandEvent& event)
{
	event.Skip();
}

void KBEditSearch::OnBnClickedFindNext(wxCommandEvent& WXUNUSED(event))
{
	// get the text in the edit control
	wxString strSearch = m_pLocalSearchBox->GetValue();

	// get the currently selected position (if none, get the top of list position and show
	// it selected -- searching starts with the element following current selection)
	int nCurSel = m_pMatchListBox->GetSelection();
	unsigned int count = m_pMatchListBox->GetCount();
	if (nCurSel == wxNOT_FOUND)
	{
		nCurSel = 0;
		SetMatchListSelection(nCurSel,FALSE); // sets m_nCurMatchListIndex to passed in 
				// nSel value; and FALSE means 'set the list item selection ourselves'
		if (nCurSel == (int)count - 1)
		{
			// can't search a list which only has one item
			::wxBell();
			return;
		}
		nCurSel++;
	}
	else
	{
		if (nCurSel < (int)(count - 1))
		{
			// there is at least one item below the selection
			nCurSel++;
		}
		else
		{
			// beep, and reposition the selection to the top of the list (there will be at
			// least two items in the list, otherwise the Find Next button is disabled)
			::wxBell();
			m_nCurMatchListIndex = 0;
			SetMatchListSelection(m_nCurMatchListIndex,FALSE); // sets m_nCurMatchListIndex
				// to passed in  nSel value; FALSE means 'set the list item selection ourselves'
			return;
		}
	}

	// search
	unsigned int index;
	wxString strLabel = _T("");
	for (index = nCurSel; index < count; index++)
	{
		// get the list's label string at index & check for a match; return with the
		// matched item selected; but if not matched, continue to iterate thru the list
		strLabel = m_pMatchListBox->GetString(index);
		int offset = strLabel.Find(strSearch);
		if (offset == wxNOT_FOUND)
			continue;
		else
		{
			SetMatchListSelection(index, FALSE); // sets m_nCurMatchListIndex to index 
				// value, FALSE means 'set the list item selection ourselves'
			return;
		}
	} // end of search loop

	// when the end has been reached, beep and put the selection back at the top
	::wxBell();
	m_nCurMatchListIndex = 0;
	SetMatchListSelection(m_nCurMatchListIndex,FALSE); // sets m_nCurMatchListIndex
		// to passed in  nSel value; FALSE means 'set the list item selection ourselves'
}

void KBEditSearch::OnBnClickedRestoreOriginalSpelling(wxCommandEvent& WXUNUSED(event))
{
	if (m_pCurKBMatchRec != NULL)
	{
		// change what is in the edit box, and remove the entry in the update list if
		// there is currently one there for this respelled match item (we assume the user
		// will re-edit and use the Update button to put a corrected entry back into the
		// update list)
		m_pEditBox->ChangeValue(m_pCurKBMatchRec->strOriginal);
		if (m_pCurKBMatchRec->pUpdateRecord != NULL)
		{
			// there is an associated entry in the update list, so remove it
			m_pCurKBUpdateRec = m_pCurKBMatchRec->pUpdateRecord;
			m_nCurUpdateListIndex = (int)GetUpdateListIndexFromDataPtr(m_pCurKBUpdateRec);
			m_pCurKBMatchRec->pUpdateRecord = NULL;
			m_pUpdateListBox->Delete(m_nCurUpdateListIndex);
			delete m_pCurKBUpdateRec;
			m_pCurKBUpdateRec = NULL;
			m_pUpdateRecordArray->RemoveAt(m_nCurUpdateListIndex); // get rid of the entry
										// in the sorted array of KBUpdateRecord pointers
			m_nCurUpdateListIndex = wxNOT_FOUND;

		}
		else
		{
			if (m_pCurKBUpdateRec != NULL)
			{
				delete m_pCurKBUpdateRec;
				m_pCurKBUpdateRec = NULL;
			}
			if (m_nCurUpdateListIndex != wxNOT_FOUND)
			{
				m_nCurUpdateListIndex = wxNOT_FOUND;
			}
		}
	}
	else
	{
		::wxBell();
	}
}

//void KBEditSearch::InsertInUpdateList(KBUpdateRecordArray* pMatchRecordArray, KBUpdateRecord* bRec, int index)
//{
//}

unsigned int KBEditSearch::GetUpdateListIndexFromDataPtr(KBUpdateRecord* pCurRecord)
{
	unsigned int index;
	unsigned int count = m_pUpdateListBox->GetCount();
	KBUpdateRecord* pUpdateRec = NULL;
	for (index = 0; index < count; index++)
	{
		pUpdateRec = (KBUpdateRecord*)m_pUpdateListBox->GetClientData(index);
		wxASSERT(pUpdateRec);
		if (pUpdateRec == pCurRecord)
			break;
	}
	return index;
}

// if bUserClicked is TRUE, the user has clicked on a list item, and the call to
// this function will be in an event handler, so this function does not have to
// do the item selection itself, but if bUserClicked is FALSE, the we are using
// this function to programmatically set the selection to the passed in index, and so
// internal code is required to make the list item selection be seen
void KBEditSearch::SetMatchListSelection(int nSelectionIndex, bool bUserClicked)
{
	m_nCurMatchListIndex = nSelectionIndex;
	m_pCurKBMatchRec = m_pMatchRecordArray->Item(m_nCurMatchListIndex);
	if (bUserClicked == FALSE)
	{
		// make the list item selection ourselves
		m_pMatchListBox->SetSelection(nSelectionIndex,TRUE); // TRUE means 'select' (not 'deselect')
	}
	wxString strLabel = m_pMatchListBox->GetStringSelection();
	if (!strLabel.IsEmpty() && (strLabel == m_pCurKBMatchRec->strOriginal))
	{
		// if the selected word or phrase has already been edited and is in the update
		// list, then select it there and show the update list's version in the edit box,
		// otherwise, put the string in the edit box so it can be respelled
		if (m_pCurKBMatchRec->pUpdateRecord != NULL)
		{
			// it's been edited already...
			m_pCurKBUpdateRec = m_pCurKBMatchRec->pUpdateRecord;
			wxString textThatWasEdited = m_pCurKBUpdateRec->updatedString;

            // find the index of the update list's item to select, using the data pointer
            // rather than the label string, because user edits may have resulted in
            // homonyms being in the list, and that would always select the first of such,
            // but the data pointers will be unique (we need to get the index by a loop in
            // a function call because we can't rely on the index return by .Add()-ing to
            // the m_pUpdateRecordArray, because we don't .Add() in this present
            // circumstance and the necessary index value may, by now, have been lost due
            // to user actions subsequent to the last .Add() call)
			unsigned int index = GetUpdateListIndexFromDataPtr(m_pCurKBUpdateRec);
			m_pEditBox->ChangeValue(textThatWasEdited);
			m_pUpdateListBox->SetSelection(index);
			m_nCurUpdateListIndex = index;
		}
		else
		{
			// check for an existing selection in the update list, and if there is one,
			// deselect it
			int nSelIndex = m_pUpdateListBox->GetSelection();
			if (nSelIndex != wxNOT_FOUND)
			{
				m_pUpdateListBox->Deselect(nSelIndex);
			}

			// put the strLabel (the adaptation or gloss that was matched) into the edit box
			// to permit the user to change its spelling; use ChangeValue() in order not to
			// generate a wxEVT_TEXT event; and clear the pointer to the current update record,
			// and the index into the update list
			m_pEditBox->ChangeValue(strLabel);
			m_pCurKBUpdateRec = NULL;
			m_nCurUpdateListIndex = wxNOT_FOUND; // -1
		}
		EnableUpdateButton(TRUE);
		EnableRestoreOriginalSpellingButton(TRUE);
		EnableRemoveUpdateButton(TRUE);

		// show the source text word or phrase in the box under the left list, and below
		// that, the number of times the adaptation (or gloss) has so far been referenced
		// within documents
		m_strSourceText = m_pCurKBMatchRec->strMapKey;
		m_pSrcPhraseBox->ChangeValue(m_strSourceText);
		int nRefCount = m_pCurKBMatchRec->pRefString->m_refCount;
		wxString strNum;
		strNum = strNum.Format(_T("%d"),nRefCount);
		m_pNumReferencesBox->ChangeValue(strNum);
	}
}

void KBEditSearch::OnChangeLocalSearchText(wxCommandEvent& WXUNUSED(event))
{
	unsigned int count = m_pMatchListBox->GetCount();
	bool bEnableFlag = count >= 2 ? TRUE : FALSE;
	EnableFindNextButton(bEnableFlag);
}

void KBEditSearch::OnMatchListSelectItem(wxCommandEvent& event)
{
	// get the index to the KBMatchRecord pointer stored in m_pKBMatchRecordArray
	m_nCurMatchListIndex = event.GetSelection();
	SetMatchListSelection(m_nCurMatchListIndex);
}

void KBEditSearch::OnBnClickedUpdate(wxCommandEvent& WXUNUSED(event))
{
	// don't Update if the word has not been respelled or the box is empty
	if (m_pEditBox->IsEmpty() || (m_pEditBox->GetValue() == m_pCurKBMatchRec->strOriginal))
	{
		::wxBell(); // don't put a word not respelled into the update list, warn user
	}
	else
	{
		// is this the first time this respelling has been added to the list, or is it a
		// re-edit? Check and process accordingly. For a re-edit, m_pCurKBUpdateRec will
		// not be NULL... 
        // *** NOTE ****(if we add a copy of strOriginal to the update record, then we
        //     could here make an additional test that the strOriginal in the match record
        //     matches the string stored as the copy in the update record; do so only if we
        //     need it)
		if (m_pCurKBUpdateRec != NULL)
		{
			// we have possibly re-edited (if the edit box contents have changed), and so
			// we check, and if so, we update the m_pUpdateListBox entry 'in place'
			if (m_pEditBox->IsEmpty() || (m_pEditBox->GetValue() == m_pCurKBUpdateRec->updatedString))
			{
				// the text box entry is not any different from what is in the update list
				// entry, so do nothing except give the user feedback that nothing was done
				::wxBell();
			}
			else
			{
				// update 'in place' -- but we can't really do so because the respelling
				// may have altered its place in the sort order, so we'll only update the
				// struct and then remove the entry and re-insert it
				wxString str = m_pEditBox->GetValue();
				str.Trim();
				str.Trim(FALSE);
				m_pCurKBUpdateRec->updatedString = str; // the struct is uptodate

				// remove the item from the list box, and the struct from the sorted array
				m_pUpdateListBox->Delete(m_nCurUpdateListIndex); // also deletes data
				m_pUpdateRecordArray->RemoveAt(m_nCurUpdateListIndex);

				// now get the reinsertion done -- get a new index etc
				KBUpdateRecord* pUR = m_pCurKBUpdateRec;
				int itsIndex = m_pUpdateRecordArray->Add(pUR);
				if (itsIndex == 0 && (m_pUpdateListBox->GetCount() == 0))
				{
					m_pUpdateListBox->Append(pUR->updatedString,(void*)pUR);
				}
				else
				{
					m_pUpdateListBox->Insert(pUR->updatedString,(unsigned int)itsIndex,(void*)pUR);
				}
				m_nCurUpdateListIndex = itsIndex;
				m_pUpdateListBox->SetSelection(itsIndex);
			}
		}
		else
		{
            // there has been a respelling, so continue processing, and this is a first
            // time edit, not an edit of a respelling
			KBUpdateRecord* pUR = new KBUpdateRecord;
			wxASSERT(pUR);
			pUR->updatedString = m_pEditBox->GetValue();
			pUR->updatedString.Trim(); // trim right end
			pUR->updatedString.Trim(FALSE); // trim left end
			pUR->nMatchRecordIndex = m_nCurMatchListIndex; // index the KBMatchRecord
							// associated with this respelling, in m_pMatchRecordArray
			// now set the pointer to this update record, in the KBMatchRecord instance which
			// is associated with it; and set the class's member too
			m_pCurKBMatchRec->pUpdateRecord = pUR;
			m_pCurKBUpdateRec = pUR;

			// now add this update record to the sorted array, m_pUpdateRecordArray -- it's
			// position will flop about as new records are added due to other edits which the
			// user does, so we've no interest in storing the return index value for the
			// position long term, we use it only for inserting the label string into the
			// m_pUpdateListBox (note, sorted arrays can only use Add(), never Insert(); but
			// m_pUpdateListBox is not sorted, we do the sorting only in the
			// m_pUpdateRecordArray and just mirror the array's contents in the list box)
			int itsIndex = m_pUpdateRecordArray->Add(pUR);
			if (itsIndex == 0 && (m_pUpdateListBox->GetCount() == 0))
			{
				m_pUpdateListBox->Append(pUR->updatedString,(void*)pUR);
			}
			else
			{
				m_pUpdateListBox->Insert(pUR->updatedString,(unsigned int)itsIndex,(void*)pUR);
			}
			m_nCurUpdateListIndex = itsIndex;

			// show the just inserted entry selected, (the matched list's associated entry
			// should still be selected)
			m_pUpdateListBox->SetSelection(itsIndex);
		}
	}
}

void KBEditSearch::OnUpdateListSelectItem(wxCommandEvent& event)
{
	// get the index of the selection
	m_nCurUpdateListIndex = event.GetSelection();

	if (m_nCurUpdateListIndex != wxNOT_FOUND)
	{
        // we have a valid selection index, so get the label string (to put it into the
        // edit box) and the data ptr (the KBUpdateRecord ptr) associated with it, and from
        // the latter, get the index in the m_pMatchListBox where the associated
        // KBMatchRecord ptr is stored, and then select that list item
		wxString strLabel = m_pUpdateListBox->GetStringSelection();
		m_pCurKBUpdateRec = (KBUpdateRecord*)m_pUpdateListBox->GetClientData(m_nCurUpdateListIndex);
		m_nCurMatchListIndex = m_pCurKBUpdateRec->nMatchRecordIndex;
		// the next line gets the pointer from the parallel match record (sorted) array,
		// but we could equally well get it from the m_pMatchListBox item's data ptr at
		// the same index - but the former is easier
		m_pCurKBMatchRec = m_pMatchRecordArray->Item(m_nCurMatchListIndex);
		m_pMatchListBox->SetSelection(m_nCurMatchListIndex);
		m_pEditBox->ChangeValue(strLabel);

		// show the source text word or phrase in the box under the left list, and below
		// that, the number of times the adaptation (or gloss) has so far been referenced
		// within documents
		m_strSourceText = m_pCurKBMatchRec->strMapKey;
		m_pSrcPhraseBox->ChangeValue(m_strSourceText);
		int nRefCount = m_pCurKBMatchRec->pRefString->m_refCount;
		wxString strNum;
		strNum = strNum.Format(_T("%d"),nRefCount);
		m_pNumReferencesBox->ChangeValue(strNum);
	}
	else
	{
		::wxBell();
	}
}

void KBEditSearch::OnBnClickedRemoveUpdate(wxCommandEvent& WXUNUSED(event))
{
	// do it provided there is a selection and m_nCurUpdateListIndex is not -1
	int nSelection = m_pUpdateListBox->GetSelection();
	if (nSelection != wxNOT_FOUND && m_nCurUpdateListIndex != -1)
	{
		m_pCurKBUpdateRec = (KBUpdateRecord*)m_pUpdateListBox->GetClientData(m_nCurUpdateListIndex);
		m_pUpdateRecordArray->RemoveAt(m_nCurUpdateListIndex); // get rid of the entry in the sorted
															   // array of KBUpdateRecord pointers
		m_nCurMatchListIndex = m_pCurKBUpdateRec->nMatchRecordIndex; // get the match record index for
								// the associated KBMatchRecord pointer in the m_pMatchRecordArray
		m_pCurKBMatchRec = m_pMatchRecordArray->Item(m_nCurMatchListIndex);
		m_pCurKBMatchRec->pUpdateRecord = NULL; // clear the pointer to the struct we are deleting 
		m_pUpdateListBox->Delete(m_nCurUpdateListIndex); // delete the entry from the update list
		delete m_pCurKBUpdateRec; // delete its struct too
		m_pEditBox->ChangeValue(_T("")); // clear the edit box
		m_pMatchListBox->Deselect(m_nCurMatchListIndex); // deselect the match list's item
		// the above handles all the visible stuff, now clear out the relevant member variables
		m_nCurMatchListIndex = -1;
		m_nCurUpdateListIndex = -1;
		m_pCurKBUpdateRec = NULL;
		m_pCurKBMatchRec = NULL;
	}
	else
	{
		::wxBell();
	}
}

void KBEditSearch::OnUpdateListDoubleclickItem(wxCommandEvent& event)
{
	// treat as a single click
	OnUpdateListSelectItem(event);
}

void KBEditSearch::OnMatchListDoubleclickItem(wxCommandEvent& event)
{
	// treat as a single click
	OnMatchListSelectItem(event);
}

void KBEditSearch::OnEnterInEditBox(wxCommandEvent& WXUNUSED(event))
{
	// treat as a click on Update button
	wxCommandEvent dummyEvent;
	OnBnClickedUpdate(dummyEvent);
}

