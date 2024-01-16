/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBEditor.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CKBEditor class.
/// The CKBEditor class provides a tabbed dialog (one tab for each of one to
/// ten word source phrases stored in the knowledge base). It allows the user
/// to examine, edit, add, or delete the translations that have been stored for
/// a given source phrase. The user can also move up a given translation in the
/// list to be shown automatically in the phrasebox by the ChooseTranslationDlg.
/// A user can also toggle the "Force Choice..." flag or add <no adaptation> for
/// a given source phrase using this dialog.
/// BEW 24Jan13, changes to Update button, because updating an entry so it becomes spelled
/// the same as an already deleted one, needs special treatment (ie. to undelete the
/// deleted one after deleting the one changed)
/// \derivation		The CKBEditor class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBEditor.h"
#endif

// whm 25Jun2015 added the following wxCHECK_GCC_VERSION() statement to prevent
//"unrecognized command line options" when compiling with GCC version 4.8 or earlier
#include <wx/defs.h>
#if defined(__GNUC__) && !wxCHECK_GCC_VERSION(4, 6)
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	#pragma GCC diagnostic ignored "-Wsign-compare"
	#pragma GCC diagnostic ignored "-Wwrite-strings"
	#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"
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

//#if defined(_DEBUG)
//#define DUALS_BUG
// I didn't get to put wxLogDebug calls in the handler for Update button, where the actual
// error was, because I figured out what was wrong before doing so. If DUALS_BUG is reused
// at a later time, then some logging calls in the Update button handler may be warranted
//#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator

#include "Adapt_It.h"
#include "KbServer.h"
#include "KBEditor.h"
#include "KB.h"
#include "TargetUnit.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "Pile.h"
#include "Cell.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"
#include "KBEditSearch.h"
#include "RemoveSomeTgtEntries.h"

// for support of auto-capitalization

/// This global is defined in Adapt_It.cpp
extern bool	gbAutoCaps;

/// These globals are defined in Adapt_It.cpp
extern bool	gbSourceIsUpperCase;
extern bool	gbNonSourceIsUpperCase;
extern wxChar gcharNonSrcLC;
extern wxChar gcharSrcLC;
extern bool gbUCSrcCapitalAnywhere; // TRUE if searching for captial at non-initial position 
									// is enabled, FALSE is legacy initial position only
extern int  gnOffsetToUCcharSrc; // offset to source text location where the upper case
								 // character was found to be located, wxNOT_FOUND if not located

// next two are for version 2.0 which includes the option of a 3rd line for glossing

/// This global is defined in Adapt_ItView.cpp.
extern bool	gbIsGlossing; // when TRUE, the phrase box and its line have glossing text

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

//const int ID_DUMMY = 22000;

// event handler table
BEGIN_EVENT_TABLE(CKBEditor, AIModalDialog)
	EVT_INIT_DIALOG(CKBEditor::InitDialog)
	EVT_BUTTON(wxID_CANCEL, CKBEditor::OnCancel)
	EVT_BUTTON(wxID_OK, CKBEditor::OnOK)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_KB_EDITOR_NOTEBOOK, CKBEditor::OnTabSelChange)
	EVT_LISTBOX(IDC_LIST_SRC_KEYS, CKBEditor::OnSelchangeListSrcKeys)
	EVT_LISTBOX(IDC_LIST_EXISTING_TRANSLATIONS, CKBEditor::OnSelchangeListExistingTranslations)
	EVT_LISTBOX_DCLICK(IDC_LIST_EXISTING_TRANSLATIONS, CKBEditor::OnDblclkListExistingTranslations)
	//EVT_TEXT(IDC_EDIT_EDITORADD, CKBEditor::OnUpdateEditOrAdd)
	EVT_BUTTON(IDC_BUTTON_UPDATE, CKBEditor::OnButtonUpdate)
	EVT_UPDATE_UI(IDC_BUTTON_UPDATE, CKBEditor::OnUpdateButtonUpdate)
	EVT_BUTTON(IDC_BUTTON_ADD, CKBEditor::OnButtonAdd)
	EVT_UPDATE_UI(IDC_BUTTON_ADD, CKBEditor::OnUpdateButtonAdd)
	EVT_BUTTON(IDC_ADD_NOTHING, CKBEditor::OnAddNoAdaptation)
	EVT_UPDATE_UI(IDC_ADD_NOTHING, CKBEditor::OnUpdateAddNoAdaptation)
	EVT_BUTTON(IDC_BUTTON_REMOVE, CKBEditor::OnButtonRemove)
	EVT_UPDATE_UI(IDC_BUTTON_REMOVE, CKBEditor::OnUpdateButtonRemove)
	EVT_BUTTON(ID_BUTTON_REMOVE_SOME, CKBEditor::OnButtonRemoveSomeTgtEntries)
	EVT_BUTTON(IDC_BUTTON_MOVE_UP, CKBEditor::OnButtonMoveUp)
	EVT_UPDATE_UI(IDC_BUTTON_MOVE_UP, CKBEditor::OnUpdateButtonMoveUp)
	EVT_BUTTON(IDC_BUTTON_MOVE_DOWN, CKBEditor::OnButtonMoveDown)
	EVT_UPDATE_UI(IDC_BUTTON_MOVE_DOWN, CKBEditor::OnUpdateButtonMoveDown)
	EVT_BUTTON(IDC_BUTTON_FLAG_TOGGLE, CKBEditor::OnButtonFlagToggle)
	EVT_UPDATE_UI(IDC_BUTTON_FLAG_TOGGLE, CKBEditor::OnUpdateButtonFlagToggle)

	EVT_BUTTON(ID_BUTTON_GO, CKBEditor::OnButtonGo)
	EVT_BUTTON(ID_BUTTON_ERASE_ALL_LINES, CKBEditor::OnButtonEraseAllLines)
	EVT_BUTTON(ID_BUTTON_SRC_FIND_GO, CKBEditor::OnButtonSourceFindGo)
	EVT_COMBOBOX(ID_COMBO_OLD_SEARCHES, CKBEditor::OnComboItemSelected)
	//EVT_IDLE(CKBEditor::OnIdle)
END_EVENT_TABLE()


CKBEditor::CKBEditor(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Edit or Examine the Knowledge Base"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	//: wxDialog(parent, -1, _("Edit or Examine the Knowledge Base"),
	//	wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // This dialog function below is generated in wxDesigner, and defines the controls and
    // sizers for the dialog. The first parameter is the parent which should normally be
    // "this". The second and third parameters should both be TRUE to utilize the sizers
    // and create the right size dialog.

	// See if we can get Idle processing going locally just for this dialog
	wxIdleEvent::SetMode(wxIDLE_PROCESS_ALL);
	wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_ALL);

	// wx Note: no harm in initializing vars here before the dialog controls are
	// created via KBEditorDlgFunc.
	m_edTransStr = _T("");
	m_srcKeyStr = _T("");
	m_refCount = 0;
	m_refCountStr = _T("");
	m_flagSetting = _T("");
	m_entryCountStr = _T("");
	m_entryCountLabel = _T("");

	m_TheSelectedKey = _T("");	// whm moved from the View's global space (where it was
			// gTheSelectedKey) and renamed it to m_TheSelectedKey as member of CKBEditor

	m_nWordsSelected = -1; // whm removed gnWordsInPhrase from global space and
						   // incorporated it into m_nWordsSelected

    // whm: In the following, I've changed the "ON" to "YES" and "OFF" to "NO" because it
    // signifies the status of "Force Choice For This Item is ___". This makes it possible
    // to distinguish localizations for the other place in the program where "ON" and "OFF"
    // is used in relation to Book Folder mode, where we have "Book folder mode is ___"
    // where "ON" and "OFF" is appropriate but "YES" or "NO" doesn't fit so well. In the
    // localization a given string can only have one translation that must work for the
    // string literal used everywhere in the program.
	m_ON = _T("YES");
	m_OFF = _T("NO");
	pApp = (CAdapt_ItApp*)&wxGetApp();
	if (gbIsGlossing)
		pKB = pApp->m_pGlossingKB;
	else
		pKB = pApp->m_pKB;
	wxASSERT(pKB != NULL);
	pCurTgtUnit = NULL;
	pCurRefString = NULL;
    m_currentKey = _T("");

	m_nCurPage = 0; // default to first page (1 Word)
	m_bRemindUserToDoAConsistencyCheck = FALSE; // set TRUE if user does respellings
												// using the KBEditSearch dialog
	bKBEntryTemporarilyAddedForLookup = FALSE;

	KBEditorDlgFunc(this, TRUE, TRUE);
    // The declaration is:
    // KBEditorDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer);

    // whm 5Mar2019 Note: The KBEditorDlgFunc() now uses the wxStdDialogButtonSizer, so
    // we no longer need to call the ReverseOkCancelButtonsForMac() function below.
	//bool bOK = false;
	//bOK = gpApp->ReverseOkCancelButtonsForMac(this);
    //    wxUnusedVar(bOK); // whm added 25Jun2015 to avoid gcc "not used" warning

	// pointers to the controls common to each page (most of them) are obtained within
	// the LoadDataForPage() function

	// the following pointer to the KBEditor's wxNotebook control is a single instance;
	// it can only be associated with a pointer after the KBEditorDlgFunc above call
	m_pKBEditorNotebook = (wxNotebook*)FindWindowById(ID_KB_EDITOR_NOTEBOOK);
	wxASSERT(m_pKBEditorNotebook != NULL);
	m_pStaticSelectATab = (wxStaticText*)FindWindowById(ID_STATIC_TEXT_SELECT_A_TAB);
	wxASSERT(m_pStaticSelectATab != NULL);

	m_pStaticWhichKB = (wxStaticText*)FindWindowById(ID_STATIC_WHICH_KB);
	wxASSERT(m_pStaticWhichKB != NULL);
}

CKBEditor::~CKBEditor()
{
}

//void CKBEditor::OnIdle(wxIdleEvent& WXUNUSED(event))
//{
//	UpdateButtons(); // whm 2May2022 added OnIdle. Now this is the main/only place where UpdateButtons() needs to be called.
//}

// OnTabSelChange added for wx version - handles changes to new tab selection
void CKBEditor::OnTabSelChange(wxNotebookEvent& event)
{
	int pageNumSelected = event.GetSelection();
	if (pageNumSelected == m_nCurPage)
	{
		// user selected same page, so just return
		return;
	}

	// store any search strings in the m_pEditSearches wxTextBox multiline control so that
	// they can be restored to the new page which the user has clicked
	DoRetain();

	// if we get to here user selected a different page
	m_srcKeyStr.Empty(); // we don't want a string from a previous page displayed
						 // on tab with no entries
	m_edTransStr.Empty(); // ditto...
	m_refCountStr.Empty(); // ditto...
	m_flagSetting.Empty(); // ditto...
	m_nCurPage = pageNumSelected;

	// Set up new page data by populating list boxes and controls
	LoadDataForPage(pageNumSelected,0); // start with first item selected for
										// new tab page
}

// BEW 22Jun10, changes needed for support of kbVersion 2's m_bDeleted flag
void CKBEditor::OnSelchangeListSrcKeys(wxCommandEvent& WXUNUSED(event))
{
    // wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a
    // call to Clear() so we must check to see if the listbox contains no items and if so
    // return immediately.
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxKeys))
	{
		return;
	}
	wxString s;
	s = _("<no adaptation>");

	int nSel;
	nSel = m_pListBoxKeys->GetSelection();
	if (nSel == -1) // LB_ERR
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Keys list box error when getting the current selection"),
		_T(""), wxICON_EXCLAMATION | wxOK);
		wxASSERT(FALSE);
	}
	wxString str;
	str = m_pListBoxKeys->GetStringSelection();
	// BEW 9Jun15, if the user clicked an entry in the list, the m_pTypeSourceBox
	// below the list should reflect the choice; but before now it didn't. This
	// next line fixes it.
	// BEW 5May16 removed it, we want the user to be able to repeatedly search
	// ahead for the next occurrence of the substring, and to cycle from top as well
	//m_pTypeSourceBox->ChangeValue(str);
	//int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, str, caseSensitive, subString);
	int nNewSel = nSel;
	// BEW 5May16 changed to use this variant, as it will search on from the current match position
	// and cycle to list top once end of list is reached
	//int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, str, caseSensitive, subString, fromCurrentSelPosCyclingBack);
	wxASSERT(nNewSel != -1);
	pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nNewSel);
	wxASSERT(pCurTgtUnit != NULL);
    m_currentKey = str;

	int nTopOfListItem = 0; // initialize (Mike wants some lines above the selection
		// to be visible; I'll give him 3 above, so selection will be 4th line
		// BEW 4May16, make 3 lines above selection visible, or less if close to the top
	nTopOfListItem = nNewSel - 3;
	// Check we are not out of bounds, adjust as necessary
	if (nTopOfListItem < 0)
	{
		nTopOfListItem++;
		if (nTopOfListItem < 0)
		{
			nTopOfListItem++;
			if (nTopOfListItem < 0)
			{
				nTopOfListItem++;
			}
		}
	}
	m_pListBoxKeys->SetFirstItem(nTopOfListItem);

#if defined(_DEBUG) && defined(DUALS_BUG)
	bool bDoAbaotem = FALSE;
	int counter = 0;
	if (m_currentKey == _T("abaotem"))
	{
		bDoAbaotem = TRUE;
		wxLogDebug(_T("\n")); // want a blank line to separate groups
	}
#endif

	// show the setting for the "Show Alternatives If Matched" checkbox
	bool bFlagValue = (bool)pCurTgtUnit->m_bAlwaysAsk;
	if (bFlagValue)
		m_flagSetting = m_ON;
	else
		m_flagSetting = m_OFF;
	// we're not using validators here so fill the text ctrl
	m_pFlagBox->SetValue(m_flagSetting);

	// now update the rest of the dialog window to show the translations for this
	// target unit, etc.
	// BEW 22Jun10, added support for kbVersion 2's m_bDeleted flag
	m_pListBoxExistingTranslations->Clear();
	if (pCurTgtUnit != NULL)
	{
		int countNonDeleted = pCurTgtUnit->CountNonDeletedRefStringInstances();
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->GetFirst();
		wxASSERT(pos != NULL);
		int countShowable = 0;
		while (pos != NULL)
		{
			pCurRefString = (CRefString*)pos->GetData();

#if defined(_DEBUG) && defined(DUALS_BUG)
			if (bDoAbaotem)
			{
				counter++;
				wxString flag = pCurRefString->GetDeletedFlag() ? _T("TRUE") : _T("FALSE");
				wxLogDebug(_T("OnSelchangeListSrcKeys(): counter = %d  countNonDeleted = %d  m_translation = %s  m_bDeleted = %s"),
					counter, countNonDeleted,
					pCurRefString->m_translation.c_str(), flag.c_str());
			}
#endif

			pos = pos->GetNext();
			// ignore any CRefString instances which have m_bDeleted set to TRUE
			if (!pCurRefString->GetDeletedFlag())
			{
				countShowable++;
				wxString str = pCurRefString->m_translation;
				if (str.IsEmpty())
				{
					str = s;
				}
				// m_pListBoxExistingTranslations is not sorted
				m_pListBoxExistingTranslations->Append(str, pCurRefString);
				// next 4 lines are legacy code, commented out as wxWidgets provides
				// a much quicker 2-parameter overload of Append() as used above
				//int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,
				//									caseSensitive,exactString);
				//wxASSERT(nNewSel != -1); // we just added it so it must be there!
				//m_pListBoxExistingTranslations->SetClientData(nNewSel,pCurRefString);
			}
		} // end while loop
		wxASSERT(countShowable == countNonDeleted);
		countShowable = countNonDeleted; // avoids a compiler warning


		// select the first translation in the listbox by default
		wxASSERT(m_pListBoxExistingTranslations->GetCount() > 0);
		m_pListBoxExistingTranslations->SetSelection(0);
		str = m_pListBoxExistingTranslations->GetStringSelection();
		int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,
															caseSensitive,exactString);
		wxASSERT(nNewSel != -1);
		pCurRefString = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nNewSel);
		wxASSERT(pCurRefString != NULL);

		// a "<Not In KB>" means the source phrase has m_bNotInKB flag set, so such an entry
		// needs special treatment, since it is actually not officially within the KB
		if (pCurRefString->m_translation == _T("<Not In KB>"))
		{
			// do special handling of this case
			m_refCount = 0;
			m_refCountStr = _T("0");
			m_edTransStr = _T("");
		}
		else
		{
			m_refCount = pCurRefString->m_refCount;
			m_refCountStr.Empty();
			m_refCountStr << m_refCount;
			if (m_refCount < 0)
			{
				m_refCount = 0; // an error condition, if the text is not "<Not In KB>"
				m_refCountStr = _T("0");
				wxMessageBox(_T(
				"A value of zero for the reference count means there was an error."));
			}

			// get the selected translation text
			m_edTransStr = m_pListBoxExistingTranslations->GetStringSelection();
		}
	}

	m_pEditRefCount->SetValue(m_refCountStr);
//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15 Punctuation should not get into the KB, so we'll not expect to
	// have to remove punctuation contiguous to / character, but the user may want to edit
	// a returned phrase, so / used as word delimiter must be visible as such - so convert
	// any ZWSP instances back to / for display in the text box. (Add or Update buttons must
	// later reconvert any / back to ZWSP)
	m_edTransStr = ZWSPtoFwdSlash(m_edTransStr);
//#endif
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
}

void CKBEditor::OnSelchangeListExistingTranslations(wxCommandEvent& WXUNUSED(event))
{
    // wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a
    // call to Clear() so we must check to see if the listbox contains no items and if so
    // return immediately
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		return;
	}

	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>"

	//int nSel;
	//nSel = m_pListBoxExistingTranslations->GetSelection();

	wxString str;
	str = m_pListBoxExistingTranslations->GetStringSelection();
	int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,
															caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	CRefString* pRefStr = (CRefString*)
								m_pListBoxExistingTranslations->GetClientData(nNewSel);
	if (pRefStr != NULL)
	{
		pCurRefString = pRefStr;
		m_refCount = pRefStr->m_refCount;
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
		if (str == s)
			m_edTransStr = _T("");
		else
			m_edTransStr = str;
	}
	else
	{
		pCurRefString = 0;
		m_refCount = 0;
		m_refCountStr = _T("0");
		m_edTransStr = _T("");
	}
	m_pEditRefCount->SetValue(m_refCountStr);
//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15 Punctuation should not get into the KB, so we'll not expect to
	// have to remove punctuation contiguous to / character, but the user may want to edit
	// a returned phrase, so / used as word delimiter must be visible as such - so convert
	// any ZWSP instances back to / for display in the text box. (Add or Update buttons must
	// later reconvert any / back to ZWSP)
	m_edTransStr = ZWSPtoFwdSlash(m_edTransStr);
//#endif
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
}

void CKBEditor::OnDblclkListExistingTranslations(wxCommandEvent& event)
{
	OnSelchangeListExistingTranslations(event);
}

/////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent that is generated when the button
///						   ID_BUTTON_SRC_FIND_GO is clicked
/// \remarks
/// Called from: The wxCommandEvent mechanism. If the edit box is empty the handler returns
/// immediately. If the string typed into the edit box is found then that found item is
/// selected in the list and this handler calls OnSelchangeListSrcKeys().
/// BEW 9May16, if the selected item was the last in the list, OnSelchangeListSrcKeys(),
/// when it internally added 1 to nSel value of current selection, that made the selection
/// index go out-og-bounds, and so no search was done. So I have to here detect this end
/// of list position and manually move the selection to the top of the list, so the search
/// can kick off from there
/////////////////////////////////////////////////////////////////////////////////////////
void CKBEditor::OnButtonSourceFindGo(wxCommandEvent& event)
{
	// assuming that another char was typed, find the nearest matching key in the list
	m_srcKeyStr = m_pTypeSourceBox->GetValue(); // this is what user has typed into edit box
	// if user deleted last char in the edit box, we don't want to generate a beep or move
	// the selection
	if (m_srcKeyStr.IsEmpty())
		return;
//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15 The user can be expected to manually type / delimiters here, so
	// for the lookup we need to convert them to ZWSP
	m_srcKeyStr = FwdSlashtoZWSP(m_srcKeyStr);
//#endif
    // m_flagSetting, m_entryCountStr and m_refCountStr don't need to come from window to
    // the variables wx version note: wxListBox::FindString doesn't have a second parameter
    // to find from a certain position in the list. We'll do it differently and use our own
    // function which finds the first item having case insensitive chars the same as what
    // user has typed into the "key to be found" edit box. For this search, we can return an
    // index of a substring at any position in the word or phrase. A case sensitive search
	// gives the user the best manual options for finding what he wants
	// BEW 4May16 changed it to use caseSensitive search
	int nSel = wxNOT_FOUND;
	int myCurrentSel = m_pListBoxKeys->GetSelection();
	if (myCurrentSel != wxNOT_FOUND)
	{
		if (myCurrentSel >= (int)(m_pListBoxKeys->GetCount() - 1))
		{
			// The selection is at the last item in the list, so put it at list start
			myCurrentSel = 0;
		}
		nSel = gpApp->FindListBoxItem(m_pListBoxKeys, m_srcKeyStr, caseSensitive, subString, fromCurrentSelPosCyclingBack);
	}
	if (nSel == -1) // LB_ERR
	{
		::wxBell();
		return;
	}
	m_pListBoxKeys->SetSelection(nSel,TRUE);
	OnSelchangeListSrcKeys(event);
}

////////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent that is generated when the contents of the
///                         KB Dialog Editor's "Edit or Add a Translation" edit box changes
/// \remarks
/// Called from: The wxCommandEvent mechanism when the contents of the KB Dialog Editor's
/// "Edit or Add a Translation" edit box changes. This handler simply calls the CKBEditor's
/// UpdateButtons() helper function whenever the edit box changes.
////////////////////////////////////////////////////////////////////////////////////////////
//void CKBEditor::OnUpdateEditOrAdd(wxCommandEvent& WXUNUSED(event))
//{
//    // This OnUpdateEditOrAdd is called every time SetValue() is called which happens
//    // anytime the user selects a string from the list box, even though he makes no changes
//    // to the associated text in the text control. That is a difference between the
//    // EVT_TEXT event macro design and MFC's ON_EN_TEXT macro design. Therefore, in the
//    // UpdateButtons() function we need to have tests which include a IsModified() test,
//    // for the wx version. We enable the Update button if the text is dirty and the text is
//    // different from the selected item in existing translations, make similar test for the
//    // other buttons.
//	//wxString testStrTransBox,testStrListBox;
//	//testStrTransBox = m_pEditOrAddTranslationBox->GetValue();
//	//testStrListBox = m_pListBoxExistingTranslations->GetStringSelection();
//	UpdateButtons();
//}

void CKBEditor::OnUpdateButtonUpdate(wxUpdateUIEvent& event)
{
	if (m_pEditOrAddTranslationBox->IsModified() && m_pEditOrAddTranslationBox->GetValue() != m_pListBoxExistingTranslations->GetStringSelection())
	{
		// The Update button is enabled whenever the edit translation text box is dirty
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}

// BEW 25Jun10, changes needed for support of kbVersion 2
void CKBEditor::OnButtonUpdate(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}

	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();

	wxString oldText;
	oldText = m_pListBoxExistingTranslations->GetStringSelection();
	wxString s = _("<no adaptation>");

	// ensure that it is not a "<Not In KB>" entry
	if (oldText == _T("<Not In KB>"))
	{
		// IDS_ILLEGAL_EDIT
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_(
		"Editing this kind of entry is not permitted."),
		_T(""), wxICON_INFORMATION | wxOK);
		return;
	}

	wxASSERT(pCurTgtUnit != 0);
	wxString newText = _T("");
	newText = m_pEditOrAddTranslationBox->GetValue();

    // BEW additions 1Sep15, so we can check active location for same text and force a
    // replacement there if it's identical, otherwise when the active location changes, the
    // oldText being at the active location still would cause oldText to be reentered into
    // the KB by the StoreText() call done at phrasebox movement time. So we'll check and
    // replace it with the updated text automatically to prevent this happening (as a
    // consistency check done subsequently would not help, because there would be no
    // inconsistency at that location; so we must prevent it here)
    //
	// BEW 1Sep15, Get the active location's pSrcPhrase, m_adaption so we can
	// make the above check and do the replacement if it is needed
	CPile* pActivePile = gpApp->m_pActivePile;
	CSourcePhrase* pSrcPhrase = pActivePile->GetSrcPhrase();
	// Our further tweaks need to happen after the next couple of bleeding exit clauses

	// BEW 1Sep15 added 2 more lines to the message
	m_messageCount++; //incremement the message count, if greater than 3, don't show the message again in this sesion
	if (m_messageCount < m_maxMessageCount)
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        int ok = wxMessageBox(_(
"Changing the spelling in this editor will leave the instances in the document unchanged.\n(Do a Consistency Check later to fix this problem,\nand tick its checkbox: Do blind fixes whenever possible.)\n(You will see this message only three times in this session.)\nDo you wish to go ahead with the spelling change?"),
	_T(""),wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
        if (ok != wxYES)
        {
            return;
        }
	}

	bool bNewWasNoAdaptation = FALSE; // initialize
	bool bOldWasNoAdaptation = FALSE; // initialise
	if (newText.IsEmpty())
	{
		// IDS_REDUCED_TO_NOTHING
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        int value = wxMessageBox(_(
"You have made the translation nothing. This is okay, but is it what you want to do?"),
		_T(""), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
		if (value != wxYES)
		{
			return;
		}
		bOldWasNoAdaptation = TRUE; // use below to support <no adaptation> being PseudoDeleted
	}
	// BEW 1Sep15 continuing the additions started above...
	wxString activeKey = pSrcPhrase->m_key; // might be upper or lower case
	wxString activeAdaption; // use for m_gloss or m_adaption according to mode that is current
	if (gbIsGlossing)
	{
		activeAdaption = pSrcPhrase->m_gloss;
	}
	else
	{
		activeAdaption = pSrcPhrase->m_adaption;
	}

	// We want this check to work whether or not there is capital letter intial in either
	// source or target to both, so we must do the usual auto-caps fiddles and end up
	// testing for identity of lower case strings if auto-caps is on.
	//
    // But... If using solidus as a word delimiter, and the active location is a merger,
    // then solidus would have replaced ZWSP at the active location's phrasebox, but
    // solidus must not go into the KB - so the from-KB strings will have ZWSP if they
    // are more than one word and solidus support is turned on. So we will convert the
    // from-KB strings to have solidi before we make our checks, because if replacement
    // in the phrasebox is required, it's the solidus-delimited string form that we'll
    // need to use. We'll use new local strings so we don't mess with legacy code...
    // (These strings may be glosses, or adaptations, depending on what kind of KB is current)
    wxString maybeSolidusOldText = oldText;
	wxString maybeSolidusNewText = newText;
	maybeSolidusOldText = ZWSPtoFwdSlash(maybeSolidusOldText);
	maybeSolidusNewText = ZWSPtoFwdSlash(maybeSolidusNewText);
	// New we can do our auto-caps fiddles. If auto-caps is ON, then the from-KB strings
	// will be lower-case initial already, so we only need bother with the pSrcPhrase ones;
	// If auto-caps is not ON, then we just check the unadjusted strings 'as is'
	bool bNoError = TRUE;
	bool bNoError2 = TRUE;
	if (gbAutoCaps)
	{
		// TRUE here means 'we are dealing with source text'
		bNoError = gpApp->GetDocument()->SetCaseParameters(activeKey, TRUE); // TRUE means 'for source text'
		bNoError2 = gpApp->GetDocument()->SetCaseParameters(activeAdaption, FALSE); // NonSource text
	}
	if (gbAutoCaps && bNoError && gbSourceIsUpperCase && (gcharSrcLC != _T('\0')))
	{
		// BEW 25May16, the refactored auto-caps feature needs adjustment if we are allowing
		// for upper case character to be non-first in first word of source text
		if (gbUCSrcCapitalAnywhere)
		{
			// make the character at gnOffsetToUCcharSrc of the source string be the 
			// appropriate lower case one; provided the offset is not wxNOT_FOUND, but if it
			// is, do the legacy replacement instead
			if (gnOffsetToUCcharSrc != wxNOT_FOUND)
			{
				activeKey.SetChar(gnOffsetToUCcharSrc, gcharSrcLC);
			}
			else
			{
				activeKey.SetChar(0, gcharSrcLC);
			}
		}
		else // Legacy protocol, initial char only
		{
			// make the first character of the src string be the appropriate lower case one
			activeKey.SetChar(0, gcharSrcLC); // gcharSrcLC is set within the SetCaseParameters() call
		}
	}
	if (gbAutoCaps && bNoError2 && gbNonSourceIsUpperCase && (gcharNonSrcLC != _T('\0')))
	{
		// change it to lower case
		activeAdaption.SetChar(0, gcharNonSrcLC);
	}
	// Make the tests... and do the replacement if the tests succeed
	if (activeKey == m_currentKey && activeAdaption == maybeSolidusOldText)
	{
		// The pSrcPhrase at the active location has to be updated in order to prevent
		// the oldText from sneaking back into the KB unbidden. Do it here.
		// Also update app's m_pTargetBox, and m_targetPhrase, so that when the phrasebox
		// is later moved, m_targetStr gets calculated correctly etc
		if (gbIsGlossing)
		{
			pSrcPhrase->m_gloss = maybeSolidusNewText;
		}
		else
		{
			pSrcPhrase->m_adaption = maybeSolidusNewText;
		}
		gpApp->m_targetPhrase = maybeSolidusNewText;
		gpApp->m_pTargetBox->GetTextCtrl()->ChangeValue(maybeSolidusNewText);
		// Un-Select the text in the box (focus will be returned there later)
		int len = gpApp->m_targetPhrase.Length();
		gpApp->m_nStartChar = len;
		gpApp->m_nEndChar = len;
        // whm 3Aug2018 Note: No adjustment here needed for 'select all'
		gpApp->m_pTargetBox->GetTextCtrl()->SetSelection(len,len);
	}

//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15 Punctuation should not get into the KB, so we'll not expect to
	// have to restore punctuation contiguous to / character, but the user may have edited
	// the box contents, so any visible / used as word delimiter there must be converted to
	// a ZWSP instance for storage.
	newText = FwdSlashtoZWSP(newText);// m_bFwdSlashDelimiter is checked internally
//#endif
	// Ensure we are not duplicating an undeleted translation already in the list box
	//
	// BEW changed from here on, 24Jan13, because with kbVersion 2 deletions are present
	// but with m_bDeleted = TRUE, and if an edit is done that makes the final form of
	// the translation string identical to that in m_translation of a deleted entry, the
	// legacy code then results in two identical entries - one deleted, one not; and a
	// subsequent StoreText() in the document for the same value as the deleted one would
	// then result in KB Editor having two identical non-deleted translations listed in
	// the box. So I'm doing changes here and below as follows: check if the updated
	// translation form matches any deleted entry, and if so, undelete it; but if it
	// matches a non-deleted one, give the warning below as before, that it already
	// exists, and return
	wxString str = _T("");
	int nStrings = m_pListBoxExistingTranslations->GetCount(); // deletions are not
						// shown and therefore are not counted (kbVersion 2) here, so the
						// subsequent test is only for a match with non-deleted entries
	for (int i=0; i < nStrings; i++)
	{
		str = m_pListBoxExistingTranslations->GetString(i);
		if (str == s) // does str equal _("<no adaptation>")
		{
			str = _T("");
			bOldWasNoAdaptation = TRUE; // use below to support PseudoDeleting <no adaptation>
		}
		if ((str.IsEmpty() && newText.IsEmpty()) || (str == newText))
		{
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_(
"The translation you are attempting to associate with the current source phrase already exists."),
			_T(""),wxICON_INFORMATION | wxOK);
			return;
		}
	} // end of for loop

	// go ahead and do the update (pCurRefString should already be set, but I don't want
	// to rely on that - so first add a few lines to make sure we have it set correctly)
	int nFound = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, oldText,
														caseSensitive, exactString);
	wxASSERT(nFound != wxNOT_FOUND);
	pCurRefString = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nFound);
	wxASSERT(pCurRefString != NULL);

	// BEW 24Jan12 - here is the appropriate place to test if the update string matches a
	// deleted entry. If it does, we must undelete the entry and show it in the list.
	// A match with an existing undeleted listed translation has been bled out of
	// consideration above; so the only match possible in the following loop is with a
	// deleted entry
	bool bUndeleting = FALSE;
	TranslationsList::Node* posKB = pCurTgtUnit->m_pTranslations->GetFirst();
	CRefString* pARefStr = NULL;
	while (posKB != NULL)
	{
		pARefStr = posKB->GetData();
		posKB = posKB->GetNext();
		wxASSERT(pARefStr != NULL);

		// does it match? (The second subtest is redundant, but is safety-first)
		// (tests are in the local KB and the GUI, so no python called here
		if (pARefStr->m_translation == newText && pARefStr->GetDeletedFlag() == TRUE)
		{
			// we've matched a deleted entry, so set the flag and exit the loop
			bUndeleting = TRUE;
			break;
		}
	} // end of while loop

	if (bUndeleting) // pARefStr is the CRefString instance we are undeleting here
	{
		pARefStr->SetDeletedFlag(FALSE);
		pARefStr->m_refCount = 1;
		wxString aTimestamp = GetDateTimeNow(); // do this way, so all these are same timestamp
		(pARefStr->GetRefStringMetadata())->SetCreationDateTime(aTimestamp);
		wxString strEmpty = _T("");
		(pARefStr->GetRefStringMetadata())->SetDeletedDateTime(strEmpty);
		(pARefStr->GetRefStringMetadata())->SetModifiedDateTime(strEmpty);
		// in the SetWho() call, param bool bOriginatedFromTheWeb is default FALSE
		(pARefStr->GetRefStringMetadata())->SetWhoCreated(SetWho());

		// Make the original, i.e. pCurRefString, become a pseudo-deleted KB entry
		pCurRefString->SetDeletedFlag(TRUE);
		//pCurRefString->m_refCount = 1; // leave it unchanged, if later undeleted set = 1 then
		(pCurRefString->GetRefStringMetadata())->SetDeletedDateTime(aTimestamp);

//#if defined(_KBSERVER)
// GDLC 20JUL16 Avoid crash with IsKBSharingEnabled()
		int rv = -1; // initialise
		wxUnusedVar(rv);
        if ((pApp->m_bIsKBServerProject && pApp->KbAdaptRunning())
            ||
            (pApp->m_bIsGlossingKBServerProject && pApp->KbGlossRunning()))
		{
			KbServer* pKbSvr = pApp->GetKbServer(pApp->GetKBTypeForServer());

			// do the update, and support <no adaptation>
			if (!pCurTgtUnit->IsItNotInKB())
			{
				// Note, KbEditorUpdateButton internally calls LookupEntryFields(), case 7
				if (bOldWasNoAdaptation)
				{
					// BEW 20Jan21 I've refactored so that <no adaptation> is stored in the relevant
					// "empty adaptation" rows for kbserver entry table; so that python has a string
					// to store, and to grab when looking up the entry table
					rv = pKbSvr->KbEditorUpdateButton(pKbSvr, m_currentKey, oldText, s); // s is <no adaptation>
				}
				else
				{
					rv = pKbSvr->KbEditorUpdateButton(pKbSvr, m_currentKey, oldText, newText);
				}
			}
		}
//#endif
        // Now get the visible listbox contents to comply with what we've done. nFound is
        // still valid, it's the index for the list entry we need to remove; its clientData
        // pointer which is a ptr to the CRefString instance owning the translation is not
        // to be deleted however, since it must stay in the pCurTgtUnit; and it won't be
        // deleted if the control doesn't own it, which it doesn't
        m_pListBoxExistingTranslations->Delete(nFound);

		// The undeleted translation string now needs to be made visible, and the client
		// data set to the ptr to that CRefString instance; we'll use nFound again, and
		// insert at that location if possible
		size_t countItems = m_pListBoxExistingTranslations->GetCount();
		if ((size_t)nFound > countItems)
		{
			nFound = countItems; // ensure no bounds error
		}
		m_pListBoxExistingTranslations->Insert(newText,(unsigned int)nFound,(void*)pARefStr);

		// re-find the index for the newly undeleted entry in the list, select it and set
		// it's m_refCount to 1
		int nLocation = pApp->FindListBoxItem(m_pListBoxExistingTranslations, newText,
															caseSensitive, exactString);
		m_pListBoxExistingTranslations->SetSelection(nLocation,TRUE);
		m_refCount = 1;

		// update the dialog page to agree with what we've done
		m_refCountStr = _T("1");
		m_pEditRefCount->ChangeValue(m_refCountStr);
		m_pEditOrAddTranslationBox->ChangeValue(newText);

		// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
		pApp->GetDocument()->Modify(TRUE);
		return;
	}
	// If control gets to here, we didn't match a pseudo-deleted translation and have to
	// undelete it and then return, so continue on - the newText is unique in this
	// pCurTgtUnit, and so the pre-January2013 code which follows here still applies

    // BEW changed 24Sep12, to not modify "in place", but rather to clone a copy, the copy
    // then gets the modified datetime, and the new adaptation (or gloss) text, and is
    // shown in the translations list on the edit page, while the old pCurRefString merely
    // gets it's m_deleted flag set TRUE, and a deletion datetime, and is removed from the
    // edit page's list -- this is in anticipation of KBserver support, which will work
    // correctly (ie. the uploaded deletions will cause deletions in the other connected
    // clients, and the new entry will result in a new KBserver pair with the edited
    // adaptation or gloss text as the second member of the pair).

	// clone the pCurRefString, the clone will become the new entry, pCurRefString will
	// become the deleted CRefString instance (eventually)
	CRefString* pEditedRefString = new CRefString(*pCurRefString, pCurTgtUnit); // CRefStringMetadata
																	// is copy-created automatically
	// add the new data values to it
	// BEW 9Jun15, the old one we can leave unmodified by any auto-caps ON setting, since
	// presumably the matchups above needed to be with the unmodified contents; but for the
	// newText, if gbAutoCaps is TRUE, then we should store a lower case entry only
	// BEW 9Jun15, looking at what goes into KBserver, I noticed that no adjustment to
	// initial capital letter is done when gbAutoCaps is TRUE. Fix this.
	bNoError = TRUE;
	if (gbAutoCaps)
	{
		//bNoError = pApp->GetDocument()->SetCaseParameters(m_currentKey); // for source word or phrase
		//if (bNoError)
		//{
		//	m_srcKeyStr = pKB->AutoCapsMakeStorageString(m_srcKeyStr); // might be returned as lower case initial
		//}
		// Now do the calcs for the non-src text (either target, or gloss when in glossing mode), for
		// this we must set the 2nd param, bool bIsSrcText, explicitly to FALSE
		bNoError = pApp->GetDocument()->SetCaseParameters(newText, FALSE);
		if (bNoError)
		{
			newText = pKB->AutoCapsMakeStorageString(newText, FALSE); // might be returned as lower case initial
		}
	}
	pEditedRefString->m_translation = newText; // could be an empty string
	pEditedRefString->m_refCount = 1; // give it a minimal legal ref count value
	// the old creation date should be left unchanged, but change the modification
	// datetime; the old WhoCreated value should also be updated for this edited copy
	// BEW 25Jun10, additions for kbVersion 2 specifically (two lines of code added)
	wxString nowStr = GetDateTimeNow();
	pEditedRefString->GetRefStringMetadata()->SetModifiedDateTime(nowStr);
	pEditedRefString->GetRefStringMetadata()->SetWhoCreated(SetWho());

    // The CTargetUnit which stores pCurRefString may have some "deleted" (and therefore
    // unshown in the dialog's page) CRefString instances which are preceding the
    // pCurRefString one, and so the nSel index won't necessarily be the right value for
    // where pCurRefString is located in the pCurTgtUnit's list. So Find() the appropriate
    // index - search for the pCurRefString ptr value.
	int actualIndex = pCurTgtUnit->m_pTranslations->IndexOf(pCurRefString);
	wxASSERT(actualIndex != wxNOT_FOUND);

	// Insert the pEditedRefString instance at the actualIndex location in pCurTgtUnit, we
	// don't need the returned iterator value, so ignore it
	pCurTgtUnit->m_pTranslations->Insert(actualIndex, pEditedRefString);

	// Now adjust the pCurRefString instance to be a deleted one - we leave it in
	// pCurTgtUnit of course, but further below we must remove it from the page's
	// wxListBox list
	pCurRefString->GetRefStringMetadata()->SetDeletedDateTime(nowStr);
	pCurRefString->SetDeletedFlag(TRUE);

	// BEW added 26Oct12 for KBserver support

//#if defined(_KBSERVER)
// GDLC 20JUL16 Avoid crash with IsKBSharingEnabled()
	int rv = -1;
	wxUnusedVar(rv);
	if ((pApp->m_bIsKBServerProject && pApp->KbAdaptRunning())
        ||
        (pApp->m_bIsGlossingKBServerProject && pApp->KbGlossRunning()))
	{
		KbServer* pKbSvr = pApp->GetKbServer(pApp->GetKBTypeForServer());
		// Do the python code in for case 7 (lookup_entry_fields)
		if (!pCurTgtUnit->IsItNotInKB())
		{
			if (bNewWasNoAdaptation)
			{
				rv = pKbSvr->KbEditorUpdateButton(pKbSvr, m_currentKey, oldText, s);
			}
			else
			{
				rv = pKbSvr->KbEditorUpdateButton(pKbSvr, m_currentKey, oldText, newText);
			}
		}
	}
//#endif

	// That completes what's needed for updating the CTargetUnit instance. The stuff below
	// is to get the page's translations (or glosses) list to comply with the edit done

	// set m_edTransStr to the new adaptation (or gloss) - this will be used below
	// for updating the contents of m_pEditOrAddTranslationBox wxTextCtrl so that the
	// control value will persist
	m_edTransStr = newText;

	// Handle an empty adaptation (or gloss), so that the list will have the appropriate
	// string visible - (This inserts an extra line before the selection, so we need to
	// then delete the following old value's line and renew the selection, etc)
	if (newText.IsEmpty())
		newText = s; // cause list box to show "<no adaptation>"
	m_pListBoxExistingTranslations->Insert(newText,nSel);

	// find the original entry before which we just inserted the edited one, so as to
	// delete it from the list
	int nOldLoc = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, oldText,
														caseSensitive, exactString);
	wxASSERT(nOldLoc != wxNOT_FOUND); // -1
	m_pListBoxExistingTranslations->Delete(nOldLoc);

	// re-find the index for the newly edited entry in the list, select it and set it's
	// clientData member in the relevant wxListBox's node, and set it's m_refCount to 1
	int nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, newText,
														caseSensitive, exactString);
	m_pListBoxExistingTranslations->SetSelection(nLocation,TRUE);
	m_pListBoxExistingTranslations->SetClientData(nLocation, pEditedRefString);
	m_refCount = 1;

	// update the dialog page to agree with what we've done
	m_refCountStr = _T("1");
	m_pEditRefCount->ChangeValue(m_refCountStr);
	m_pEditOrAddTranslationBox->ChangeValue(m_edTransStr);

	// make the state of the various buttons etc in the page agree with the new state
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.

	// we'll also make the doc dirty to ensure that the File > Save is enabled, and the
	// save button on the toolbar is also enabled
	pApp->GetDocument()->Modify(TRUE);
}

void CKBEditor::OnUpdateAddNoAdaptation(wxUpdateUIEvent& event)
{
	if (m_pListBoxExistingTranslations->GetCount() == 0 || IsInListBox(m_pListBoxExistingTranslations, _("<no adaptation>")))
	{
		// the AddNoAdaptation button is always enabled unless "<no adaptation>" is in the
		// m_pListBoxExistingTranslations list
		event.Enable(FALSE);
	}
	else
	{
		event.Enable(TRUE);
	}
}


// BEW 25Jun10, changes needed for support of kbVersion 2
void CKBEditor::OnAddNoAdaptation(wxCommandEvent& event)
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}

	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	//int nSel;
	//nSel = m_pListBoxExistingTranslations->GetSelection();

	wxString oldText;
	oldText = m_pListBoxExistingTranslations->GetStringSelection();

	// ensure that it is not a "<Not In KB>" targetUnit that we are trying to add more to
	if (oldText == _T("<Not In KB>"))
	{
		// can't add to such an entry, if <Not In KB> is present, no other entry is allowed
		::wxBell();
		return;
	}

	wxString newText = _T(""); // this is what the user wants to add, for local KB
	wxString s;
	s = _("<no adaptation>");

	m_edTransStr = m_pEditOrAddTranslationBox->GetValue();
	m_srcKeyStr = m_pTypeSourceBox->GetValue();
	// BEW 16Jan24, m_srcKeyStr set from the m_pTypeSourceBox is not robust, if that box was not used
	// to find an entry in the panels. It would be empty, and lead to an empty src key which is 
	// an error - though the app doesn't fail, MariaDB's entry table just gets "empty string/ <no adaptation>"
	// which is a bogus entry. There will be a selected src key, and it's value will be in m_TheSelectedKey,
	// so test and use that instead
	if (m_srcKeyStr.IsEmpty())
	{
		m_srcKeyStr = m_TheSelectedKey; // in case RHS has the src key value (it won't have in every situation)
		// Also, m_currentKey probably has the correct value, if m_TheSelectedKey is empty
		if (m_srcKeyStr.IsEmpty())
		{
			if (!m_currentKey.IsEmpty())
			{
				m_srcKeyStr = m_currentKey; // BEW 16Jan24 yep, that's where the correct current key is
			}
		}
	}
	wxASSERT(pCurTgtUnit != 0);
	bool bOK = AddRefString(pCurTgtUnit,newText); // adds it, provided it is
												  // not already there
	if (bOK)
	{
		// BEW added 19Feb13 for KBserver support
//#if defined(_KBSERVER)
// GDLC 20JUL16 Avoid crash with IsKBSharingEnabled()
        if ((pApp->m_bIsKBServerProject && pApp->KbAdaptRunning())
            ||
            (pApp->m_bIsGlossingKBServerProject && pApp->KbGlossRunning()))
		{
			KbServer* pKbSvr = pApp->GetKbServer(pApp->GetKBTypeForServer());
			if (!pCurTgtUnit->IsItNotInKB())
			{
				pKbSvr->CreateEntry(pKbSvr, m_srcKeyStr, s); // BEW 20Jan21 we want <no adaptation> 
						// in the entry table because python is not able to pick up a nothing value

				wxLogDebug(_T("KBEditor.cpp (1036) OnAddNoAdaptation(): CreateEntry for src = %s  &  tgt = %s"),
					m_srcKeyStr.c_str(), s.c_str());
			}
		}
//#endif
		// Don't add to the list if the AddRefString call did not succeed
		//m_pListBoxExistingTranslations->Append(newText);
		m_pListBoxExistingTranslations->Append(s); // BEW 21Jan21 -- user needs to 
								// see <no adaptation> rather than an empty line
		// m_pListBoxExistingTranslations is not sorted, but it is safer to always get
		// an index using FindListBoxItem
		int nFound = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, s,
													caseSensitive, exactString);
		if (nFound == -1) // LB_ERR
		{
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_(
			"Error warning: Did not find the translation text just inserted!"),_("<no adaptation>")
			,wxICON_EXCLAMATION | wxOK);
			m_edTransStr = m_pListBoxExistingTranslations->GetString(0);
			CRefString* pRefStr = (CRefString*)
							m_pListBoxExistingTranslations->GetClientData(0);
			m_pListBoxExistingTranslations->SetSelection(0,TRUE);
			m_refCount = pRefStr->m_refCount;
			m_refCountStr.Empty();
			m_refCountStr << m_refCount;
			m_pEditRefCount->ChangeValue(m_refCountStr);
			m_pEditOrAddTranslationBox->ChangeValue(m_edTransStr);
			return;
		}
		m_pListBoxExistingTranslations->SetClientData(nFound,pCurRefString);
		m_pListBoxExistingTranslations->SetSelection(nFound,TRUE); 
		OnSelchangeListExistingTranslations(event);
		m_pEditRefCount->SetValue(m_refCountStr);
		// BEW 20Jan21 don't force to an empty string, user needs to see the <no adaptation>
		//if (m_edTransStr == s)
		//{
			// in the edit box, show nothing rather than "<no adaptation>"
		//	m_edTransStr.Empty();
		//}
		m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
		m_pEditOrAddTranslationBox->DiscardEdits();
		// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
	}
}

void CKBEditor::OnUpdateButtonAdd(wxUpdateUIEvent& event)
{
	wxString stringBeingTyped;
	stringBeingTyped = m_pEditOrAddTranslationBox->GetValue();
	if (m_pEditOrAddTranslationBox->IsModified() && !IsInListBox(m_pListBoxExistingTranslations, stringBeingTyped))
	{
		// The Add button is enabled whenever the edit translation text box is dirty, and
		// the current string in the box is not already in the m_pListBoxExistingTranslations list.
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}


// BEW 22Jun10, changes needed for support of kbVersion 2
void CKBEditor::OnButtonAdd(wxCommandEvent& event)
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}
	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}
	wxString s = _("<no adaptation>");
	bool bWasNoAdaptation = FALSE; // initialise
	bool bOK = TRUE;
	//int nSel;
	//nSel = m_pListBoxExistingTranslations->GetSelection();

	wxString oldText;
	oldText = m_pListBoxExistingTranslations->GetStringSelection();
	wxString strNot = _T("<Not In KB>");

	// ensure that it is not a "<Not In KB>" targetUnit that we are trying to
	// add more to; also ensure the user isn't manually trying to undelete a deleted
	// <Not In KB> CRefString instance
	wxString newText = _T("");
	newText = m_pEditOrAddTranslationBox->GetValue();
	if (oldText == strNot || newText == strNot)
	{
		::wxBell();
		return;
	}
	// adding an empty string is legal, but make sure the user knows that's what is going
	// to happen
	if (newText.IsEmpty())
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        int value = wxMessageBox(_(
		"You are adding a translation which is nothing. That is okay, but is it what you want to do?"),
		_T(""), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
		if (value == wxNO)
		{
			return;
		}
		bWasNoAdaptation = TRUE; // used below to get <no adaptation> accepted into entry table of kbserver
	}
//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15 Punctuation should not get into the KB, so we'll not expect to
	// have to restore punctuation contiguous to / character, but the user may have edited
	// the box contents, so any visible / used as word delimiter there must be converted to
	// a ZWSP instance for storage.
	newText = FwdSlashtoZWSP(newText); // m_bFwdSlashDelimiter flag is tested internally
//#endif

	// BEW 22Jun10, for kbVersion 2, we must allow the user to manually try to add a
	// translation (or gloss, in glossing mode) string which has been deleted from the KB
	// (deletions are now just hidden CRefString units which have m_bDeleted set TRUE). We
	// now must allow such a manual addition to 'undelete' any matched deleted CRefString
	// instance, (but we must not allow undeletion of a deleted "<Not In KB>" one by means
	// of the user typing "<Not In KB> manually and trying to Add it -- but that has been
	// checked for above and prevented).
	m_edTransStr = m_pEditOrAddTranslationBox->GetValue(); // keep showing the editable version
	m_srcKeyStr = m_pTypeSourceBox->GetValue();
	if (m_srcKeyStr.IsEmpty())
	{
		// BEW 16Apr19 m_pTypeSourceBox is the box for source list searching; but this
		// is also used for case adjusting, and if empty, the case adjusting below which
		// looks at the source text to see if making the case of the target text be
		// lower case, won't do anything because there is no src text to check. So if
		// empty, get the src text in there, so the case adjustments can work properly.
		//  Get the source key from the src list selection.
		int nSel = 0;
		nSel = m_pListBoxKeys->GetSelection();
		if (nSel != wxNOT_FOUND)
		{
			m_srcKeyStr = m_pListBoxKeys->GetString(nSel);
		}
	}
	wxASSERT(pCurTgtUnit != 0);

	// BEW 9Jun15, looking at what goes into KBserver, I noticed that no adjustment to
	// initial capital letter is done when gbAutoCaps is TRUE. Fix this.
	bool bNoError = TRUE;
	if (gbAutoCaps)
	{
		bNoError = pApp->GetDocument()->SetCaseParameters(m_srcKeyStr); // for source word or phrase
		if (bNoError)
		{
			m_srcKeyStr = pKB->AutoCapsMakeStorageString(m_srcKeyStr); // might be returned as lower case initial
		}
		// Now do the calcs for the non-src text (either target, or gloss when in glossing mode), for
		// this we must set the 2nd param, bool bIsSrcText, explicitly to FALSE
		bNoError = pApp->GetDocument()->SetCaseParameters(newText, FALSE);
		if (bNoError)
		{
			newText = pKB->AutoCapsMakeStorageString(newText, FALSE); // might be returned as lower case initial
		}
	}

	bOK = AddRefString(pCurTgtUnit,newText); // if 'undelete' happens, AddRefString() will
				// have repositioned the undeleted CRefString to the end of the CTargetUnit
				// instance's list, so that the Append() call on the list box done below
				// will match in position

	// if it was added successfully, show it in the listbox & select it; and do KBserver
	// support if required
	int rv = -1; // initialise
	if (bOK)
	{
		// BEW added 26Oct12 for KBserver support
//#if defined(_KBSERVER)
		bool bCreatedOK = FALSE; // initialise
        bCreatedOK = bCreatedOK; // avoid gcc warning "set but not used"
        if ((pApp->m_bIsKBServerProject && pApp->KbAdaptRunning())
            ||
            (pApp->m_bIsGlossingKBServerProject && pApp->KbGlossRunning()))
		{
			KbServer* pKbSvr = pApp->GetKbServer(pApp->GetKBTypeForServer());
			if (!pCurTgtUnit->IsItNotInKB())
			{
				if (bWasNoAdaptation)
				{
					rv = pKbSvr->CreateEntry(pKbSvr, m_srcKeyStr, s);
				}
				else
				{
					rv = pKbSvr->CreateEntry(pKbSvr, m_srcKeyStr, newText);
				}
				if (rv == 0)
				{
					bCreatedOK = TRUE;
					wxLogDebug(_T("KBEditor.cpp line= %d , OnButtonAdd() Succeeded: CreateEntry for src = %s  &  tgt = %s"),
						__LINE__, m_srcKeyStr.c_str(), newText.c_str());
				}
				else // rv == -1
				{
					wxLogDebug(_T("KBEditor.cpp line= %d , OnButtonAdd() Failed: CreateEntry for src = %s  &  tgt = %s"),
						__LINE__, m_srcKeyStr.c_str(), newText.c_str());

				}
			}
		}
//#endif
		if (newText.IsEmpty())
			newText = s; // i.e. "<no adaptation>"
		m_pListBoxExistingTranslations->Append(newText);
		// m_pListBoxExistingTranslations is not sorted, but it is safer to always
		// get an index using FindListBoxItem
		int nFound = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, newText,
														caseSensitive, exactString);
		if (nFound == -1) // LB_ERR
		{
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(_(
			"Error warning: Did not find the translation text just inserted!"),_T(""),
			wxICON_EXCLAMATION | wxOK);
			m_pListBoxExistingTranslations->SetSelection(0,TRUE);
			m_pEditRefCount->SetValue(m_refCountStr);
			m_pEditOrAddTranslationBox->ChangeValue(m_edTransStr);
			return;
		}
		// for the next call, pCurRefString will have been set within the above call to
		// AddRefString()
		m_pListBoxExistingTranslations->SetClientData(nFound,pCurRefString);
		m_pListBoxExistingTranslations->SetSelection(nFound,TRUE);
		OnSelchangeListExistingTranslations(event);
		m_pEditRefCount->SetValue(m_refCountStr);
		// BEW 16Jan21, I can't think of any good reason why adding an empty string
		// should leave no obvious trace in the tgt box, so I'm commenting out these
		// emptying lines
		//if (newText == s)
		//{
			// in the edit box, show nothing rather than "<no adaptation>"
		//	newText.Empty();
		//}
		m_pEditOrAddTranslationBox->ChangeValue(newText);
		m_pEditOrAddTranslationBox->DiscardEdits(); // resets the internal "modified"
													// flag to no longer "dirty"
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
		gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
	}
} 

void CKBEditor::DoRestoreSearchStrings()
{
	m_pEditSearches->ChangeValue(_T("")); // make sure the control is empty
	m_pEditSearches->SetInsertionPointEnd();
	if (!gpApp->m_arrSearches.IsEmpty())
	{
		size_t max = gpApp->m_arrSearches.GetCount();
		size_t index;
		// we don't store empty strings in m_arrSearches
		for (index = 0; index < max; index++)
		{
			wxString str = gpApp->m_arrSearches.Item(index);
			m_pEditSearches->WriteText(str);
			m_pEditSearches->WriteText(_T("\n"));
		}
		// reposition to the start of the control
		m_pEditSearches->SetSelection(0,0);
	}
}

void CKBEditor::DoRetain()
{
	// get the line strings from the multiline control, into the wxArrayString
	// m_arrSearches which is a member of the CAdapt_ItApp class
	//wxString delims = _T("\n\r");
	wxString delims = _T("\n"); // wxWidgets guarantees the string returned from
	// a multiline wxTestCtrl will only have \n as the
	// line delimiter
	bool bStoreEmptyStringsToo = FALSE;
	wxString contents = m_pEditSearches->GetValue();
	// SmartTokenize always first clears the passed in wxArrayString
	long numSearchStrings = 0L;
	numSearchStrings = SmartTokenize(delims, contents, gpApp->m_arrSearches,
		bStoreEmptyStringsToo);
//#if defined(FWD_SLASH_DELIM)
	if (gpApp->m_bFwdSlashDelimiter)
	{
		long i;
		wxString itemStr;
		for (i = 0; i < numSearchStrings; i++)
		{
			itemStr = gpApp->m_arrSearches.Item((size_t)i);
			itemStr = FwdSlashtoZWSP(itemStr);
			// Replace it in the array
			gpApp->m_arrSearches.RemoveAt((size_t)i);
			gpApp->m_arrSearches.Insert(itemStr, (size_t)i);
		}
	}
//#endif
	// check what we got
#ifdef _DEBUG
	long index;
	wxLogDebug(_T("\n"));
	for (index = 0; index < numSearchStrings; index++)
	{
		wxLogDebug(_T("%s"),(gpApp->m_arrSearches.Item(index)).c_str());
	}
	wxLogDebug(_T("\n"));
#endif
}

void CKBEditor::OnButtonGo(wxCommandEvent& WXUNUSED(event))
{
	// clear and populate app's m_arrSearches string array, provided there is at least a
	// search string defined; and if there is, store the old one or more of them before
	// populating with the new search string(s)
	wxString contents = m_pEditSearches->GetValue();
	if (contents.IsEmpty())
	{
		::wxBell();
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("You have not yet typed something to search for."),
		_("No search strings defined"), wxICON_EXCLAMATION | wxOK);
		return;
	}
	else
	{
		// one or more search strings have been defined

		DoRetain(); // populates the contents of m_arrSearches

		// put up the KBEditSearch dialog, and in its InitDialog() method do the search and
		// populate the m_pMatchRecordArray of that class's instance
		//KBEditSearch* pKBSearchDlg = new KBEditSearch(this);
		KBEditSearch modalKBSearchDlg(this);
		if (modalKBSearchDlg.ShowModal() == wxID_OK)
		{
			// the search and any needed editing were done; so accumulate the search
			// string(s) into the m_arrOldSearches before deleting them (the array
			// CAdapt_ItApp::m_arrOldSearches is not cleared until OnFileCloseProject()
			// is called, so the combobox contents persist until then)
			size_t count = gpApp->m_arrSearches.GetCount();
			if (count > 0)
			{
				// the user has typed some search strings, so we want to add them to the
				// combobox, and to do that we have to temporarily remove the read-only
				// style (and restore it after adding the search strings to it)
				//long style = m_pComboOldSearches->GetWindowStyleFlag();
				//long readonly_mask = wxCB_READONLY;
				//style &= ~readonly_mask ;
				//m_pComboOldSearches->SetWindowStyle(style);

				size_t index;
				for (index = 0; index < count; index++)
				{
					gpApp->m_arrOldSearches.Add(gpApp->m_arrSearches.Item(index));
				}

				count = gpApp->m_arrOldSearches.GetCount(); // has new count value now
				m_pComboOldSearches->Clear();
				// refill with the longer set of items
				for (index = 0; index < count; index++)
				{
					m_pComboOldSearches->Append(gpApp->m_arrOldSearches.Item(index));
				}
				// select the top
				m_pComboOldSearches->SetSelection(0);

				// restore read-only style
				//style |= readonly_mask ;
				//m_pComboOldSearches->SetWindowStyle(style);

				// now empty the multiline text control of the user's typed search strings
				m_pEditSearches->ChangeValue(_T(""));

				// force a KB save to disk (without backup)
				bool bOK = TRUE;
				wxString mess;
				if (gbIsGlossing)
				{
					bOK = gpApp->SaveGlossingKB(FALSE); // don't want backup produced of the glossing KB
				}
				else
				{
					bOK = gpApp->SaveKB(FALSE, TRUE); // don't want backup produced
				}
				if (!bOK)
				{
					// we don't expect to ever see this message, but it's needed, just in case
					mess = _("Saving the knowledge base failed. However, your respellings are still in the knowledge base in memory, so exit from the knowledge base editor with the OK button - doing that will try the save operation again and it might succeed.");
				}

				// finally, the user must be given a reminder -- do it on exit of the KB
				// Editor dialog, but set a flag here which indicates the need for the message
				// BEW changed 20Mar10, we don't want this message unilaterally, because
				// no respellings may have been done in the KBEditSearch dialog, and it's
				// only KB changes that should cause the message about a consistency check
				// to be shown. So we let the child dialog set this flag only when the
				// update list has content
				//m_bRemindUserToDoAConsistencyCheck = TRUE;

// and add a handler for clicking on a combobox line to have it re-entered into
// m_pEditSearches
			}
		}
		else
		{
			// user cancelled (leave the m_pEditSearches text box unchanged, as the user
			// may have wanted to come back to modify the search strings a bit before
			// retrying the search, so we don't want to make him retype them all)
			;
		}
		gpApp->m_arrSearches.Empty();
		//if (modalKBSearchDlg != NULL) // whm 11Jun12 added NULL test
		//	delete modalKBSearchDlg;
	}
}

void CKBEditor::MessageForConsistencyCheck()
{
	wxString msg;
	msg = msg.Format(_("You have respelled some knowledge base entries. This has made the knowledge base inconsistent with the current documents. You should do a consistency check of all documents as soon as you dismiss this message. Do you want the consistency check to start automatically?"));
	wxString title = _("Do Consistency Check Now?");
	long style = wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT | wxCENTRE;
    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
    pApp->m_bUserDlgOrMessageRequested = TRUE;
    int answer = ::wxMessageBox(msg.c_str(),title.c_str(),style);
	if (answer == wxYES)
	{
		// when the handler for the KBEditor dialog returns, a full consistency check will
		// be done if the following flag is found to be TRUE, none is done if it is FALSE
		gpApp->m_bForceFullConsistencyCheck = TRUE;
	}
}

void CKBEditor::OnButtonEraseAllLines(wxCommandEvent& WXUNUSED(event))
{
	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	// the "Forget All Lines" button handler -- erases the control window and also empties
	// the contents of m_arrSearches
	wxTextCtrl* pText = m_pEditSearches;
	wxString emptyStr = _T("");
	pText->ChangeValue(emptyStr);
	gpApp->m_arrSearches.Empty();
}

void CKBEditor::OnUpdateButtonRemoveSomeTgtEntries(wxUpdateUIEvent& event)
{
	if (m_pListBoxKeys->GetCount() == 0)
	{
		event.Enable(FALSE);
	}
	else
	{
		event.Enable(TRUE);
	}
}


void CKBEditor::OnButtonRemoveSomeTgtEntries(wxCommandEvent& WXUNUSED(event))
{
//#if defined(_KBSERVER)
	// Disallow the button with a message, if it's a KB sharing project but sharing is
	// temporarily disabled. (Reason? If allowed in when sharing is disabled, the user may
	// spend an hour or more working through thousands of entries, ticking hundreds of
	// obsolete ones for deletion. If the Remove Selected Entries button were then
	// clicked, the local KB would have those entries pseudo-deleted, but the uploads to
	// the kbserver would not get done. That would result in a KBserver database out of
	// sync with the local KB on this machine, and we cannot correct that with a bulk
	// upload because the latter does not include any pseudo deleted entries in a bulk
	// upload. Nor would retrying the removals in the dialog window work either, because
	// the pseudo deleted entries in the local KB won't be listed in the window. So the
	// "solution" is to disallow entry to the dialog when sharing is disabled. (The
	// ability to save to a file, can't be done either - an unwanted side-effect, but the
	// protection of synchonicity of local and remote kb is much more important.)
	if (pApp->m_bIsKBServerProject || pApp->m_bIsGlossingKBServerProject)
	{
        bool bTellUser = !(pApp->KbAdaptRunning() || pApp->KbGlossRunning() );
// GDLC 20JUL16 KbAdaptRunning() and KbGlossRunning() avoid a crash if the project was used
// with a KB server but the KB server is currently not connected.
//		bool bTellUser = FALSE;
//		// This project is one for sharing entries to a remote KBserver
//		if (gbIsGlossing)
//		{
//			KbServer* pKbServer = pApp->GetKbServer(2);
//			wxASSERT(pKbServer);
//			if (!pKbServer->IsKBSharingEnabled())
//			{
//				// User has disabled sharing of the currently shared glossing KB
//				bTellUser = TRUE;
//			}
//		}
//		else
//		{
//			KbServer* pKbServer = pApp->GetKbServer(1);
//			wxASSERT(pKbServer);
//			if (!pKbServer->IsKBSharingEnabled())
//			{
//				// User has disabled sharing of the currently shared adaptations KB
//				bTellUser = TRUE;
//			}
//		}
		if (bTellUser)
		{
			wxString msg;
			wxString title;
			title = _("Knowledge base sharing is (temporarily) disabled");
			msg = _("This is a shared knowledge base project, but you have disabled sharing.\nSharing must be enabled, and with a working connection to the remote server, before a bulk removal of knowledge base entries can be allowed.\nTo enable sharing again, click Controls For Knowledge Base Sharing... on the Advanced menu, and click the left radio button.");
            // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
            pApp->m_bUserDlgOrMessageRequested = TRUE;
            wxMessageBox(msg, title, wxICON_INFORMATION | wxOK);
			return;
		}
	}
//#endif

	RemoveSomeTgtEntries dlg((wxWindow*)this);
	if (dlg.ShowModal() == wxID_OK)
	{
		// "Remove the selected entries and close" button was clicked
		LoadDataForPage(m_nCurPage,0); // clear out the list boxes etc, and repopulate them

        // Ensure the updated local KB is saved to disk, don't rely on the user exiting the
        // KB Editor with an OK click, otherwise an ill-considered Cancel could mean he
        // unwittingly abandons hours of careful inspection of the KB to delete lots of
        // obsolete entries. Also ignore the returned boolean from these calls, we can
        // assume the calls will work correctly. They've not failed once in 13 years as yet.
		if (gbIsGlossing)
		{
			pApp->SaveGlossingKB(FALSE); // FALSE means: don't want backup produced
		}
		else
		{
			pApp->SaveKB(FALSE, TRUE); // don't want backup produced
		}
	}
	else
	{
		// Cancel button was clicked; do nothing -- but remember that the user, while in the
		// dialog, may have saved the data view to a file, or each view to two different
		// files, before cancelling. If he did so, nothing has to be done here after the
		// Cancel click.
	}
}

void  CKBEditor::OnUpdateButtonRemove(wxUpdateUIEvent& event)
{
	if (m_pListBoxExistingTranslations->GetCount() == 0)
	{
		event.Enable(FALSE);
	}
	else
	{
		event.Enable(TRUE);
	}
}


void CKBEditor::OnButtonRemove(wxCommandEvent& WXUNUSED(event))
{
    // this button must remove the selected translation from the KB, which means that user
    // must be shown a child dialog or message to the effect that there are m_refCount
    // instances of that translation in this and previous document files which will then
    // not agree with the state of the knowledge base, and the user is then to be urged to
    // do a Verify operation on each of the existing document files to get the KB and those
    // files in sync with each other.

	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}

	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	wxString s;
	s = _("<no adaptation>");
	wxString message;
	int	nPreviousReferences = 0;
	bool bWasNoAdaptation = FALSE; // initialise

    // get the index of the selected translation string (this may not be the same index for
    // the CRefString stored in pCurTgtUnit because there may be stored deleted CRefString
    // instances in the pCurTgtUnit which messes up the index correspondences)
	int nTransSel = 0;
	nTransSel = m_pListBoxExistingTranslations->GetSelection();

	// get the selected string
	wxString str;
	str = m_pListBoxExistingTranslations->GetStringSelection(); // the list
									// box translation string being deleted
	wxString str2 = str;

	// BEW 20Jan21 
	if (str == s) // ie. if contents of str is "<no adaptation>"
	{
		str = _T(""); // for comparison's with what is stored in the KB,
					  // it must be empty
		bWasNoAdaptation = TRUE; // use this below to support <no adaptation> being 
								 // in the entry table of kbserver
	}

	// don't allow an attempt to remove <Not In KB>
	if (str == _T("<Not In KB>"))
	{
		// IDS_ILLEGAL_REMOVE
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_(
"Sorry, you can only remove this kind of entry by putting the phrase box at the relevant location in the document, and then click the  \"Save To Knowledge Base\" checkbox back to ON."),_T(""),
		wxICON_INFORMATION | wxOK);
		return;
	}

#if defined(_DEBUG) && defined(DUALS_BUG)
	wxLogDebug(_T("\n OnButtonRemove(): The item text: %s"), str.c_str()); // want a blank line to separate groups
#endif


	// find the corresponding CRefString instance in the knowledge base, and set the
	// nPreviousReferences variable for use in the message box; if user hits Yes
	// then go ahead and do the removals
	// BEW 22Jun10, changed to support kbVersion 2's m_bDeleted flag etc
	wxASSERT(pCurTgtUnit != NULL);
	// use the client data stored in the list box entry to get the CRefString instance's
	// pointer, so we can search for it (we can't use index value as it may reference a
	// deleted instance rather than the one we want)
	CRefString* pTheRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nTransSel);
	wxASSERT(pTheRefStr != NULL);
	// now search for the same CRefString instance in pCurTgtUnit
	TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->Find(pTheRefStr);
	wxASSERT(pos != NULL); // it must be there!
	CRefString* pRefString = (CRefString*)pos->GetData();
	wxASSERT(pRefString == pTheRefStr); // verify we got it

#if defined(_DEBUG) //&& defined(DUALS_BUG)
	wxString flag = pRefString->GetDeletedFlag() ? _T("TRUE") : _T("FALSE");
	wxLogDebug(_T("OnButtonRemove(): found pRefString with m_translation = %s  m_bDeleted = %s"),
		pRefString->m_translation.c_str(), flag.c_str());
#endif

    // do legacy checks that we have the right reference string instance; we must allow for
    // equality of two empty strings to be considered to evaluate to TRUE (these should not
    // be necessary but no harm in retaining them)
	if (str.IsEmpty() && pRefString->m_translation.IsEmpty())
	{
		// we have a match of empty strings, so continue
		;
	}
	else
	{
		// we don't have matching empty strings, so check for matching non-empty ones
		if (str != pRefString->m_translation)
		{
			// message can be in English, it's never likely to occur, let processing continue
			wxMessageBox(_T(
			"Remove button error: Knowledge bases's adaptation text does not match that selected in the list box\n"),
			_T(""), wxICON_EXCLAMATION | wxOK);
		}
	}
	// get the ref count, use it to warn user about how many previous references
	// this will mess up
	nPreviousReferences = pRefString->m_refCount;

	// warn user about the consequences, allow him to get cold feet & back out
	message = message.Format(_(
"Removing: \"%s\", will make %d occurrences of it in the document files inconsistent with the knowledge base.\n(You can fix that later by using the Consistency Check command.)\nDo you want to go ahead and remove it?"),
		str2.c_str(),nPreviousReferences);
    // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
    pApp->m_bUserDlgOrMessageRequested = TRUE;
    int nResult = wxMessageBox(message, _T(""), wxICON_QUESTION | wxYES_NO | wxYES_DEFAULT);
	if (!(nResult == wxYES))
	{
		return; // user backed out
	}

    // Autocapitalization might be ON, and if so and if the user has clicked a translation
    // starting with an upper case character (put there earlier when auto caps was OFF),
    // then the call below to AutoCapsLookup( ) would, if not prevented from doing so,
    // change the upper case to lower case at the start of the key string, and then the
    // lookup could find the wrong entry, or find no entry. The former would leave the data
    // in an invalid state, and eventually lead to a crash. The solution is to use a global
    // flag to alert AutoCapsLookup when the caller was the remove button (same applies in
    // handler for Remove button in the Choose Translation dialog), and use the flag to
    // jump the capitalization smarts and instead just do a lookup on the key as supplied
    // to the function.
    // in support of removal when autocapitalization might be on - see the OnButtonRemove handler
    // in KBEditor.cpp for a full explanation of the need for this flag
    CKB* pKB;
    if (gbIsGlossing)
        pKB = gpApp->m_pGlossingKB;
    else
        pKB = gpApp->m_pKB;
    if (pKB != NULL)
        pKB->m_bCallerIsRemoveButton = TRUE;

    // user hit the Yes button, so go ahead; remove the string from the list and then make
    // the first element in the list the new selection - provided there is one left
	m_pListBoxExistingTranslations->Delete(nTransSel);
	int count = m_pListBoxExistingTranslations->GetCount();
	wxASSERT(count >= 0);
	if (count > 0)
	{
		str = m_pListBoxExistingTranslations->GetString(0);
		int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,
														caseSensitive,exactString);
		wxASSERT(nNewSel != -1);
		pCurRefString = (CRefString*)
							m_pListBoxExistingTranslations->GetClientData(nNewSel);
		m_refCount = pCurRefString->m_refCount;
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
		m_edTransStr = m_pListBoxExistingTranslations->GetString(nNewSel);
		m_pListBoxExistingTranslations->SetSelection(nNewSel);

#if defined(_DEBUG) && defined(DUALS_BUG)
		wxString flag = pCurRefString->GetDeletedFlag() ? _T("TRUE") : _T("FALSE");
		wxLogDebug(_T("OnButtonRemove(): AFTER removal, remainder's top pCurRefString has m_translation = %s  m_bDeleted = %s"),
			pCurRefString->m_translation.c_str(), flag.c_str());
#endif

	}
	else
	{
		// list is now empty (for kbVersion 2 this block should never be entered, since we
		// don't physically remove 'deleted' CRefString instances, but no harm in leaving
		// it here (we need the count value further below anyway)
		pCurRefString = NULL;
		m_refCount = 0;
		m_refCountStr = _T("0");
		m_edTransStr = _T("");
		m_pListBoxExistingTranslations->Clear();
	}

	// BEW added 22Oct12 for KBserver support
//#if defined(_KBSERVER)
// GDLC 20JUL16 KbAdaptRunning() and KbGlossRunning() avoid a crash if there is no
// KB server actually running. This can occur if the project was used with a KB server
// but the KB server happens to be inaccessible now.
	int rv = -1; //initialise
	wxUnusedVar(rv);
    if ((pApp->m_bIsKBServerProject && pApp->KbAdaptRunning())
        ||
        (pApp->m_bIsGlossingKBServerProject && pApp->KbGlossRunning()))
	{
		KbServer* pKbSvr = pApp->GetKbServer(pApp->GetKBTypeForServer());
	
		if (!pCurTgtUnit->IsItNotInKB())
		{
			if (bWasNoAdaptation)
			{
				rv = pKbSvr->PseudoDelete(pKbSvr, m_currentKey, s);
			}
			else
			{
				rv = pKbSvr->PseudoDelete(pKbSvr, m_currentKey, pRefString->m_translation);
			}
		}

	}
//#endif
	// Remove the corresponding CRefString instance from the knowledge base... BEW 22Jun10,
	// 'remove' in the context of kbVersion 2 just means to retain storage of the
	// CRefString instance, but set its m_bDeleted flag to TRUE, and set it's metadata
	// class's m_deletedDateTime value to the current datetime (when the list is refreshed
	// by the call to LoadDataForPage() below, the deleted instance won't then appear in
	// the list), and give it a m_refCount value of 0
	wxASSERT(pRefString != NULL);
	pRefString->SetDeletedFlag(TRUE);
	pRefString->GetRefStringMetadata()->SetDeletedDateTime(GetDateTimeNow());
	pRefString->m_refCount = 0;

#if defined(_DEBUG) && defined(DUALS_BUG)
	flag = pRefString->GetDeletedFlag() ? _T("TRUE") : _T("FALSE");
	wxLogDebug(_T("OnButtonRemove(): KB update section, just deleted pRefString with m_translation = %s  m_bDeleted = %s"),
		pRefString->m_translation.c_str(), flag.c_str());
#endif

	// get the count of non-deleted CRefString instances for this CTargetUnit instance
	int numNotDeleted = pCurTgtUnit->CountNonDeletedRefStringInstances();

 #if defined(_DEBUG) && defined(DUALS_BUG)
	wxLogDebug(_T("OnButtonRemove(): KB update section, number of non-deleted ones remaining = %d   and function ends."),
		numNotDeleted );
#endif
   // We'll also need the index of the source word/phrase selected in the keys list box.
    // If it turns out that we have just deleted all of a source word/phrase's
    // translations, we therefore would need to also delete the source word/phrase from
    // its list box (but the owning CTargetUnit instance must remain in the map); but if
    // there are translations left which are marked as not deleted after removing the
    // current one, nNewKeySel should remain at the same selection.
	int nNewKeySel = m_pListBoxKeys->GetSelection();

    // Are we removing the last item in the translations list box? If so, we have to remove
	// it's owning CTargetUnit from the m_listBoxKeys list box as well
	if (numNotDeleted == 0)
	{
		// the pCurTgtUnit stores only CRefString instances with m_bDeleted set TRUE, so
		// this one should not be shown in the m_listBoxKeys list box...
		// wx note: FindString also only does a caseless find, and we don't have a second
		// parameter in FindString to allow for a search from a certain position, so we'll
		// use our own FindListBoxItem using a case sensitive search.

        // get the selected item from the Source Phrase list, as we need to delete it since
        // all of its translations have been removed
		wxString spStr = m_pListBoxKeys->GetStringSelection();
		nNewKeySel = gpApp->FindListBoxItem(m_pListBoxKeys, spStr, caseSensitive, exactString);
		wxASSERT(nNewKeySel != -1);
		// now delete the found item from the listbox
		int keysCount = 0;
		keysCount = m_pListBoxKeys->GetCount();
		wxASSERT(nNewKeySel < keysCount);
                wxUnusedVar(keysCount); // whm added 25Jun2015 to avoid gcc "not used" warning (release build)
		m_pListBoxKeys->Delete(nNewKeySel);
		// set nNewKeySel to be the previous item in listbox unless it already
		// is the first item
		if (nNewKeySel > 0)
		{
			nNewKeySel--;
		}
	} // end of TRUE block for test: if (numNotDeleted == 0)
	LoadDataForPage(m_nCurPage,nNewKeySel);
	m_pTypeSourceBox->SetSelection(0,0); // sets selection to beginning of type
										 // source edit box following MFC
	m_pTypeSourceBox->SetFocus();

	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
    if (pKB != NULL)
	    pKB->m_bCallerIsRemoveButton = FALSE; // reestablish the safe default
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save
										// button enabled
}

void CKBEditor::OnUpdateButtonMoveUp(wxUpdateUIEvent& event)
{
	// Enable those buttons meeting certain conditions depending on number of items and selection
	int nSel = m_pListBoxExistingTranslations->GetSelection();
	wxString selectedTrans;
	if (m_pListBoxExistingTranslations->GetCount() > 0)
		selectedTrans = m_pListBoxExistingTranslations->GetStringSelection();
	else
		selectedTrans.Empty();
	if (m_pListBoxExistingTranslations->GetCount() > 1 && nSel > 0)
	{
		// The Move Up button is enabled whenever there are more than 1 translations in the list
		// and the selected item is not the last one in the list
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}


// BEW 22Jun10, changes needed for support of kbVersion 2 & its m_bDeleted flag
void CKBEditor::OnButtonMoveUp(wxCommandEvent& WXUNUSED(event))
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_(
"Translations list box error when getting the current selection"),
		_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}

	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();

	int nOldSel = nSel; // save old selection index
	CRefString* pOldRefStr = NULL; // temporary store of CRefString ptr being moved

	// change the order of the string in the list box
	int count = 0;
	count = m_pListBoxExistingTranslations->GetCount();
	wxASSERT(nSel < count);
        wxUnusedVar(count); // whm added 25Jun2015 to avoid gcc "not used" warning (release build)
	if (nSel > 0)
	{
		nSel--;
		wxString tempStr;
		tempStr = m_pListBoxExistingTranslations->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,
											caseSensitive,exactString); // whm added
		wxASSERT(nLocation != wxNOT_FOUND);
		CRefString* pRefStr = (CRefString*)
							m_pListBoxExistingTranslations->GetClientData(nLocation);
		pOldRefStr = pRefStr; // BEW added 22Jun10, for use when searching below
		m_pListBoxExistingTranslations->Delete(nLocation);
		m_pListBoxExistingTranslations->Insert(tempStr,nSel);
		// m_pListBoxExistingTranslations is not sorted
		nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,
											caseSensitive,exactString); // whm added
		wxASSERT(nLocation != -1); // LB_ERR;
		m_pListBoxExistingTranslations->SetSelection(nSel);
		m_pListBoxExistingTranslations->SetClientData(nLocation,pRefStr);
		pRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nLocation);
		m_refCount = pRefStr->m_refCount;
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
		return; // impossible to move up the first element in the list!

	// now change the order of the CRefString in pCurTgtUnit to match the new order
	CRefString* pRefString;
	if (nSel < nOldSel)
	{
		// BEW 22Jun10, for support of kbVersion 2, nSel and nOldSel apply only to the GUI
		// list, which does not show stored CRefString instances marked as deleted. The
		// potential presence of deleted instances means that we must search for the
		// instance to be moved earlier in the list, and moving it means we must move over
		// each preceding deleted instance, if any, until we get to the location of the
		// first preceding non-deleted element, and insert at that location
		int oldLocation = pCurTgtUnit->m_pTranslations->IndexOf(pOldRefStr); // BEW added 22Jun10
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->Item(oldLocation);
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString == pOldRefStr);
		TranslationsList::Node* posOld = pos; // save for later on, when we want to delete it
		CRefString* aRefStrPtr = NULL;
		do {
			// jump over any deleted earlier CRefString instances, break when a
			// non-deleted one is found
			pos = pos->GetPrevious();
			aRefStrPtr = pos->GetData();
			if (!aRefStrPtr->GetDeletedFlag())
			{
				break;
			}
		} while (aRefStrPtr->GetDeletedFlag());

		// pos now points at the first previous non-deleted CRefString instance, which is
		// the one before which we want to put pOldRefStr by way of insertion, but first
		// delete the node containing the old location's instance
		pCurTgtUnit->m_pTranslations->DeleteNode(posOld);
		wxASSERT(pos != NULL);

		// now do the insertion, bringing the pCurTgtUnit's list into line with what the
		// listbox in the GUI shows to the user
        // Note: wxList::Insert places the item before the given item and the inserted item
        // then has the insertPos node position.
		TranslationsList::Node* newPos = pCurTgtUnit->m_pTranslations->Insert(pos,pRefString);
		if (newPos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T(
			"Error: MoveUp button failed to reinsert the translation being moved\n"),
			_T(""), wxICON_EXCLAMATION | wxOK);
			wxASSERT(FALSE);
		}
	}

	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save
										// button enabled
}

void CKBEditor::OnUpdateButtonMoveDown(wxUpdateUIEvent& event)
{
	// Enable those buttons meeting certain conditions depending on number of items and selection
	int nSel = m_pListBoxExistingTranslations->GetSelection();
	if (m_pListBoxExistingTranslations->GetCount() > 1 && nSel < (int)m_pListBoxExistingTranslations->GetCount() - 1)
	{
		// The Move Down button is enabled whenever there are more than 1 translations in the list
		// and the selected item is not the first one in the list
		event.Enable(TRUE);
	}
	else
	{
		event.Enable(FALSE);
	}
}


// BEW 22Jun10, changes needed for support of kbVersion 2 & its m_bDeleted flag
void CKBEditor::OnButtonMoveDown(wxCommandEvent& event)
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        pApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_(
		"Translations list box error when getting the current selection"),
		_T(""), wxICON_EXCLAMATION | wxOK);
		return;
	}

	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	int nOldSel = nSel; // save old selection index
	CRefString* pOldRefStr = NULL; // temporary store of CRefString ptr being moved

	// change the order of the string in the list box
	int count = m_pListBoxExistingTranslations->GetCount();
	wxASSERT(nSel < count);
	if (nSel < count-1)
	{
		nSel++;
		wxString tempStr;
		tempStr = m_pListBoxExistingTranslations->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,
												caseSensitive,exactString); // whm added
		wxASSERT(nLocation != wxNOT_FOUND);
		CRefString* pRefStr = (CRefString*)
								m_pListBoxExistingTranslations->GetClientData(nLocation);
		pOldRefStr = pRefStr; // BEW added 24Jun10, for use when searching below
		m_pListBoxExistingTranslations->Delete(nLocation);
		m_pListBoxExistingTranslations->Insert(tempStr,nSel);
		// m_pListBoxExistingTranslations is not sorted
		nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,
												caseSensitive,exactString); // whm added
		wxASSERT(nLocation != wxNOT_FOUND);
		m_pListBoxExistingTranslations->SetSelection(nSel);
		m_pListBoxExistingTranslations->SetClientData(nSel,pRefStr);
		pRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nSel);
		m_refCount = pRefStr->m_refCount;
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
		return; // impossible to move the list element of the list further down!

	// now change the order of the CRefString in pCurTgtUnit to match the new order
	CRefString* pRefString;
	if (nSel > nOldSel)
	{
		// BEW 24Jun10, for support of kbVersion 2, nSel and nOldSel apply only to the GUI
		// list, which does not show stored CRefString instances marked as deleted. The
		// potential presence of deleted instances means that we must search for the
		// instance to be moved to later in the list, and moving it means we must move over
		// each following deleted instance, if any, until we get to the location of the
		// second following non-deleted element, and insert before that location - but if
		// we reach the end of the list, we just append
		int oldLocation = pCurTgtUnit->m_pTranslations->IndexOf(pOldRefStr); // BEW added 24Jun10
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->Item(oldLocation);
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString  == pOldRefStr);
		TranslationsList::Node* posOld = pos; // save for later on, when we want to delete it
		CRefString* aRefStrPtr = NULL;

		// BEW 24Jun10, moving down becomes a two step process - first step is to loop
		// over any deleted CRefString instances until the iterator points at the next
		// non-deleted one (there will ALWAYS be such a one, as this block's entry
        // condition guarantees it), and then second step is to advance the iterator one
        // node further - this will take it either to the end of the list, or to a
        // CRefString instance (and this one we don't care whether it is deleted or not))
		do {
			// jump over any deleted lower-down CRefString instances, break when a
			// non-deleted one is found
			pos = pos->GetNext();
			aRefStrPtr = pos->GetData();
			if (!aRefStrPtr->GetDeletedFlag())
			{
				break;
			}
		} while (aRefStrPtr->GetDeletedFlag());
		// now advance over this non-deleted one -- this may make the iterator return NULL
		// if we are at the end of the list
		pos = pos->GetNext();
		if (pos == NULL)
		{
			// we are at the list's end
			pCurTgtUnit->m_pTranslations->Append(pRefString);
		}
		else
		{
			// we are at a CRefString instance, so we can insert before it
			pCurTgtUnit->m_pTranslations->Insert(pos,pRefString);
		}

        // check the insertion or appending was done correctly - tell developer if not so,
        // and don't delete the original if the error message shows, instead, maintain the
        // earlier state of the CTargetUnit & because the GUI doesn't agree with it any
        // more, the message should tell the user/developer it will automatically close
        // the KB Editor without saving (by calling OnClose()), and then the KB Editor can
        // be launched again without data loss from the KB and the user/developer can try
        // again
		pos = pCurTgtUnit->m_pTranslations->Find(pRefString);
		if (pos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T(
				"Error: MoveDown button failed to reinsert the translation being moved.\nTo avoid data loss from the knowledge base the application will close the KB editor, without saving your changes, when you dismiss this message. \n(Afterwards, you can safely retry the Edit Knowledge Base... command if you wish.)"),
			_T(""), wxICON_EXCLAMATION | wxOK);
			OnCancel(event);
			return;
		}
		else
		{
			// all's well, we can delete the old location's node
			pCurTgtUnit->m_pTranslations->DeleteNode(posOld);
		}
	}
	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save
										// button enabled
}

void CKBEditor::OnUpdateButtonFlagToggle(wxUpdateUIEvent& event)
{
	if (m_pListBoxExistingTranslations->GetCount() == 0)
	{
		event.Enable(FALSE);
	}
	else
	{
		event.Enable(TRUE);
	}
}


void CKBEditor::OnButtonFlagToggle(wxCommandEvent& WXUNUSED(event))
{
	// whm added 15Mar12 for read-only mode
	if (gpApp->m_bReadOnlyAccess)
	{
		::wxBell();
		return;
	}

	if (pCurTgtUnit == NULL)
	{
		::wxBell();
		return;
	}
	wxASSERT(pCurTgtUnit != NULL);
	if (m_flagSetting == m_ON)
	{
		m_flagSetting = m_OFF;
		pCurTgtUnit->m_bAlwaysAsk = FALSE;
	}
	else
	{
		m_flagSetting = m_ON;
		pCurTgtUnit->m_bAlwaysAsk = TRUE;
	}
	// we're not using validators here so fill the text ctrl
	m_pFlagBox->SetValue(m_flagSetting);
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save
										// button enabled
}

// BEW 13Nov10, changes to support Bob Eaton's request for glosssing KB to use all maps
void CKBEditor::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is
															   // method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	m_maxMessageCount = 3; // that's enough, (Update button may be used hundreds of times)
	m_messageCount = 0;

    // whm 15Sep2018 added below - used in OnOK() handler to restore a 1 ref count
    // ref string that gets deleted to the phrasebox.
    this->m_originalPhraseBoxContent.Empty();
    this->m_bBoxContent_present_but_deleted = FALSE;
    // wx Note: The dialog fonts for the list boxes are set by calls to
    // SetFontAndDirectionalityForDialogControl() in the LoadDataForPage() function. That
    // is necessary because in the wx version there are different pointers to the dialog
    // controls for each page in the tabbed notebook, and LoadDataForPage() is where those
    // pointers are assigned.

	wxASSERT(m_pKBEditorNotebook != NULL);

    // MFC Note: New feature for version 2.2.1 requested by Gene Casad. A user selection
    // will take the first CSourcePhrase instance in the selection and open the KB editor
    // on the appropriate page and with that source key selected, but if the key is not in
    // the KB the selection will default to the first item of the list. The selection in
    // the main window can be anywhere, not necessarily at the phrasebox location.
    //
    // WX version Note: New feature for version 4.1.0. In addition to being able to look up
    // the KB entry of a selected source phrase at any location, Roland Fumey requested
    // that the KB Editor look up the source phrase that is being displayed in the
    // phrasebox at its current location when the KB Editor is invoked. This lookup should
    // be made whether or not the source phrase at that active location is selected or not.
    // The only gotcha happens in the case that the source phrase at the current location
    // had a reference count of 1 before the phrasebox landed or was placed at the current
    // location - in which case the self-editing feature of the KB will have removed the
    // entry from the KB even though the user sees it in the phrasebox's current location,
    // and the KB Editor would not ordinarily be able to find it. Clearly, it would be more
    // intuitive for the user to be able to look up the source phrase that is currently
    // being displayed in the phrasebox.
    //
    // In order to effect a successful lookup in this case we need to temporarily add the
    // source phrase in the phrasebox back to the KB if it has just disappeared due to a
    // reference count having decremented to zero prior to invoking the KB Editor.

    // If any source phrase is selected, determine how many words are in the selection and
    // what key to use in the lookup. The m_nWordsSelected and m_TheSelectedKey members
    // need to be set regardless of whether the app is in active Glossing mode or not.
    int selectionCount = gpApp->m_selection.GetCount();
	if (gpApp->m_selectionLine == 0 && selectionCount > 0)
	{
		// we have a selection
		CCellList::Node* cpos = gpApp->m_selection.GetFirst();
		CCell* pCell = (CCell*)cpos->GetData();
		CSourcePhrase* pSrcPhrase = pCell->GetPile()->GetSrcPhrase();
		m_nWordsSelected = pSrcPhrase->m_nSrcWords;
		m_TheSelectedKey = pSrcPhrase->m_key;

        // If the Pile at this selection is the same as the active location's Pile we need
        // to call StoreBeforeProceeding() and set the bKBEntryTemporarilyAddedForLookup
        // flag here too as in the "else if" block below.
		if (gpApp->m_pActivePile != NULL && pCell->GetPile() == gpApp->m_pActivePile)
		{
			gpApp->GetView()->StoreBeforeProceeding(gpApp->m_pActivePile->GetSrcPhrase());
			bKBEntryTemporarilyAddedForLookup = TRUE;
            // whm 15Sep2018 added below - used in caller View's OnToolsKBEditor after the PlaceBox()
            // call (which empties the phrasebox content), to restore phrasebox 
            // content of a 1 ref count ref string that gets deleted here in KBEditor.
            this->m_originalPhraseBoxContent = gpApp->m_pTargetBox->GetTextCtrl()->GetValue();
		}
		gpApp->GetView()->RemoveSelection(); // to be safe, on return from the KB editor
											 // we want predictability
	}
	else if (gpApp->m_pActivePile != NULL)
	{
        // No selection is active, but we have a valid active location, so use the source
        // phrase at the active location for lookup.
		m_nWordsSelected = gpApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords;
		m_TheSelectedKey = gpApp->m_pActivePile->GetSrcPhrase()->m_key;

        // Determine if there is a KB entry for the source phrase at his active location.
        // If not, then the ref count probably was decremented to zero prior to our arrival
        // here, so we should temporarily add it back to the KB so that the user will be
        // able to successfully look up that entry. The View's StoreBeforeProceeding()
        // function does this.
		gpApp->GetView()->StoreBeforeProceeding(gpApp->m_pActivePile->GetSrcPhrase());
		bKBEntryTemporarilyAddedForLookup = TRUE;
        // whm 15Sep2018 added below - used in caller View's OnToolsKBEditor after the PlaceBox()
        // call (which empties the phrasebox content), to restore phrasebox 
        // content of a 1 ref count ref string that gets deleted here in KBEditor.
        this->m_originalPhraseBoxContent = gpApp->m_pTargetBox->GetTextCtrl()->GetValue();
    }
	else
	{
        // No selection is active and there is no valid active location (which can happen,
        // for example, after finishing adaptation at end of the document), so set suitable
        // defaults.
		m_nWordsSelected = -1;
		m_TheSelectedKey = _T("");
	}
    // Determine which tab page needs to be pre-selected.
    //
    // The m_nCurPage is set as follows depending on what the circumstances are when the 
	// KB Editor is invoked:
    // 1. When there was a source phrase selection, the m_nCurPage should be one less than
    // the number of words in the source phrase selection.
    // 2. When there was no source phrase selection, the m_nCurPage should be one less than
    // the number of words in the phrase box at the active location.
    // 3. Or if m_nWordsSelected is -1, if user selected a source phrase before calling
    // the KBEditor, start by initializing the tab page having the correct number of words.

	if (gbIsGlossing)
	{
		// Make it show  "Glosses Knowledge Base" at top right, rather than "Adaptations
		// Knowledge Base" (Localizable)
		wxString glossesKBLabel = _("Glosses Knowledge Base");
		m_pStaticWhichKB->SetLabel(glossesKBLabel);
	}

	if (m_nWordsSelected != -1)
	{
		// the m_nCurPage is one less than m_nWordsSelected.
		m_nCurPage = m_nWordsSelected-1;
	}
	else
	{
		m_nCurPage = m_pKBEditorNotebook->GetSelection();
	}

	// BEW added 16Jan13 so as to set it here just once; don't get it from m_pStaticCount
	// wxStatic label object here, because it isn't available; so just set it from a
	// literal string instead
	//m_entryCountStr = m_pStaticCount->GetLabel();
	m_entryCountStr = _T("Number of Entries: %d");

	if (m_nCurPage != -1)
	{
		LoadDataForPage(m_nCurPage,0);
	}

	// make the combobox have nothing in it
	m_pComboOldSearches->Clear();
}

// BEW 22Jun10, changes needed for support of kbVersion 2's m_bDeleted flag (we must allow
// manual addition of a deleted translationStr to 'undelete' it)
bool CKBEditor::AddRefString(CTargetUnit* pTargetUnit, wxString& translationStr)
// remember, translationStr could legally be empty
{
	// check for a matching CRefString, if there is no match,
	// then add a new one, otherwise warn user of the match & return
	// BEW 22Jun10, don't give the warning if what is being added matches a deleted
	// CRefString instance's m_translation string, instead, just undelete that CRefString
	// instance (no need to check for manual addition of "<Not In KB>" string, as the
	// caller has that job and has already done the check)
	wxASSERT(pTargetUnit != NULL);
	bool bMatched = FALSE;
	CRefString* pRefString = new CRefString(pTargetUnit); // creates its metadata too
	pRefString->m_refCount = 1; // set the count, assuming this will be stored (it may not be)
	pRefString->m_translation = translationStr; // set its translation string

	TranslationsList::Node* pos = pTargetUnit->m_pTranslations->GetFirst();
	TranslationsList::Node* savePos; // in case we need to undelete (see below)
	while (pos != NULL)
	{
		CRefString* pRefStr = (CRefString*)pos->GetData();
		savePos = pos; // preserve old location

		pos = pos->GetNext();
		wxASSERT(pRefStr != NULL);

		// wx TODO: Compare this to changes made in ChooseTranslation dialog
		// does it match?
		if ((pRefStr->m_translation.IsEmpty() && translationStr.IsEmpty()) ||
			(*pRefStr == *pRefString))
		{
			// we got a match, or both are empty, so warn user its a copy of an existing
			// translation and return; or we matched an already 'deleted' one, in which
			// case we undelete the deleted one and throw away the new one we created, and
			// we have to reposition the undeleted one at the end of the list because the
			// caller will append the translationStr to the end of the list box in the dlg
			bMatched = TRUE;
			if (pRefStr->GetDeletedFlag())
			{
				// it matches a deleted CRefString instance, so undelete it, etc
				pRefStr->SetDeletedFlag(FALSE);
				wxString emptyStr = _T("");
				pRefStr->GetRefStringMetadata()->SetDeletedDateTime(emptyStr);
				pRefStr->GetRefStringMetadata()->SetModifiedDateTime(emptyStr);
				pRefStr->GetRefStringMetadata()->SetCreationDateTime(GetDateTimeNow());
				pRefStr->m_refCount = 1; // don't leave it zero - that's illegal for a
										 // CRefString instance which is valid
				// set pCurRefString to pRefStr, as the caller will use pCurRefString
				// to set the client data for the list box string when we return
				pCurRefString = pRefStr;
				// throw away the new one, we won't need it
				pRefString->DeleteRefString();

                // now reposition the undeleted element to the end of the CTargetUnit's
                // list because the caller will append the new string to the end of the
                // list box's list, so their positions need to match
				bool bOK = TRUE;
				bOK = pTargetUnit->m_pTranslations->DeleteNode(savePos);
				if (!bOK)
				{
					// unlikely to happen, use an English warning
					wxMessageBox(_T("KBEditor.cpp, AddRefString() did not repostion the undeleted element to the end of the list - probably you should save, shut down and relaunch the application"));
				}
				savePos = pTargetUnit->m_pTranslations->Append(pRefStr);
				return TRUE;
			}
			else
			{
                // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
                pApp->m_bUserDlgOrMessageRequested = TRUE;
                wxMessageBox(_(
"Sorry, the translation you are attempting to associate with the current source phrase already exists."),
				_T(""),wxICON_INFORMATION | wxOK);
				pRefString->DeleteRefString(); // don't need this one
				return FALSE;
			}
		}
	}
	// if we get here with bMatched == FALSE, then there was no match, so we must add
	// the new pRefString to the targetUnit
	if (!bMatched)
	{
		pTargetUnit->m_pTranslations->Append(pRefString);
		pCurRefString = pRefString; // caller will use this LHS member variable
	}
	return TRUE;
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CKBEditor::OnOK(wxCommandEvent& event)
{
	if (bKBEntryTemporarilyAddedForLookup)
	{
		//int nWords;
        // RemoveRefString below will internally calc nWords - 1 to get map page so we must
        // increment the zero based page index here, for it to come out right within
        // RemoveRefString.
		//nWords = m_pKBEditorNotebook->GetSelection() + 1;
		wxString emptyStr = _T("");
		if (gbIsGlossing)
			gpApp->m_pGlossingKB->GetAndRemoveRefString(gpApp->m_pActivePile->GetSrcPhrase(),
												emptyStr, useGlossOrAdaptationForLookup);
		else
			gpApp->m_pKB->GetAndRemoveRefString(gpApp->m_pActivePile->GetSrcPhrase(),
												emptyStr, useGlossOrAdaptationForLookup);
        // whm 15Sep2018 added. If the phrasebox was not empty, and the refString of the target
        // text there had a ref count of 1 on entry, the above GetAndRemoveRefString() call would
        // remove it from the KB, setting its m_bDeleted member to TRUE. Here within the KBEditor's
        // OnOK() handler, we need to detect this situation and set a member bool value
        // m_bBoxContent_present_but_deleted to TRUE to inform the caller of this situation
        // so the caller (View's OnToolsKbEditor) can retain the original text in the phrasebox 
        // and set the m_pTargetBox->m_bAbandonable to FALSE - so that if the user moves the 
        // box elsewhere without doing any typing or clicking in the box, the original entry 
        // gets retained. 
        // Moreover, if the user actually clicked the "Remove" button in the KBEditor to remove 
        // this same original phrasebox content, the user has to manually delete the entry from 
        // the phrasebox before moving on, to truly keep that original content deleted in the KB 
        // at this location.
        CRefString* pRefStr = NULL;
        KB_Entry rsEntry = pApp->m_pKB->GetRefString(pApp->m_pActivePile->GetSrcPhrase()->m_nSrcWords,
            pApp->m_pActivePile->GetSrcPhrase()->m_key, pApp->m_targetPhrase, pRefStr);
        //
        // whm 6Mar2020 testing indicates that the following setting of the m_bBoxContent_present_but_deleted
        // member to TRUE is not needed back in the OnToolsKbEditor() handler in the View, but I'll leave the 
        // following block intact here in case it might be useful at a future time.
        if (pRefStr != NULL && rsEntry == present_but_deleted)
        {
            // Ensure that the m_originalPhraseBoxContent is put back into the phrasebox
            // back in the caller (OnToolsKbEditor) after its PlaceBox() call. See above 
            // comment.
            wxString debugStr = pApp->m_pTargetBox->GetTextCtrl()->GetValue();
            debugStr = debugStr; // to avoid GCC warning
            m_bBoxContent_present_but_deleted = TRUE;
        }
	}

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
    // whm 24Jul2018 added test to only call Clear() on m_arrSearches if its count is > 0,
    // otherwise after adding meanings to a source phrase and clicking OK, the Clear() call would
    // crash due to calling delete on a bad pointer.
    if (gpApp->m_arrSearches.GetCount() > 0)
	    gpApp->m_arrSearches.Clear(); // but leave m_arrOldSearches intact until project is exitted

	if (m_bRemindUserToDoAConsistencyCheck)
	{
		// give yes/no message, and set app boolean member m_bForceFullConsistencyCheck if
		// the user responds with a Yes button click (to have consistency check started
		// after the KBEditor dialog goes away)
		MessageForConsistencyCheck();
	}
}

void CKBEditor::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	if (bKBEntryTemporarilyAddedForLookup)
	{
		//int nWords;
        // RemoveRefString below will internally calc nWords - 1 to get map page so we must
        // increment the zero based page index here, for it to come out right within
        // RemoveRefString.
		//nWords = m_pKBEditorNotebook->GetSelection() + 1;
		wxString emptyStr = _T("");
		if (gbIsGlossing)
			gpApp->m_pGlossingKB->GetAndRemoveRefString(gpApp->m_pActivePile->GetSrcPhrase(),
												emptyStr, useGlossOrAdaptationForLookup);
		else
			gpApp->m_pKB->GetAndRemoveRefString(gpApp->m_pActivePile->GetSrcPhrase(),
												emptyStr, useGlossOrAdaptationForLookup);
	}
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event);
    // whm 24Jul2018 added test to only call Clear() on m_arrSearches if its count is > 0,
    // otherwise after adding meanings to a source phrase and clicking Cancel, the Clear() 
    // call might crash here as it can in the OnOK() handler above, due to calling delete 
    // on a bad pointer.
    if (gpApp->m_arrSearches.GetCount() > 0)
        gpApp->m_arrSearches.Clear(); // but leave m_arrOldSearches intact until project is exitted

    if (m_bRemindUserToDoAConsistencyCheck)
    {
        // give yes/no message, and set app boolean member m_bForceFullConsistencyCheck if
        // the user responds with a Yes button click (to have consistency check started
        // after the KBEditor dialog goes away)
        MessageForConsistencyCheck();
    }
}

// other class methods

// BEW 22Jun10, changes needed for support of kbVersion 2's m_bDeleted flag
// BEW 12Jan11, removed the wxASSERT which assumes the target text list will have at least
// one showable entry - that won't be the case if the user deletes all the list's entries,
// which would otherwise produce an unwanted trip of the assert because no error has been
// produced by such a deletion, it's an allowed user operation
// BEW bug fix 27May13, old code erased pTU within the loop if it's m_translations wxList
// member was empty. But erasure invalidates the iterator, leading to a crash. So instead
// I collect source text strings for bad entries, don't enter those to the in-memory KB
// and after the loop use the stored entries to find and delete the bad entries. (To test
// this, go to the xml and in a page, remove the <RS ... /RS> element for a few entries.
// And then in the KB Editor try entering that page. App will crash if the fix does not
// work.)
void CKBEditor::LoadDataForPage(int pageNumSel,int nStartingSelection)
{
    // In the wx version wxDesigner has created identical sets of controls for each page
    // (nbPage) in our wxNotebook. We need to associate the pointers with the correct
    // controls here within LoadDataForPage() which is called initially and for each tab
    // page selected. The pointers to controls will differ for each page, hence they need
    // to be reassociated for each page. Note the nbPage-> prefix on FindWindowById(). I
    // chosen not to use Validators here because of the complications of having pointers to
    // controls differ on each page of the notebook; instead we manually transfer data
    // between dialog controls and their variables.

    // set the page in wxNotebook
    //
    // whm changed 20Jan09. Previously I wrongly used SetSelection() which "generates page
    // changing events" and is now "deprecated". This LoadDataForPage() was being called
    // twice since SetSelection() was triggering OnTabSelChange() which also calls this
    // LoadDataForPage. This double action was causing doubled data to be inserted in the
    // KB Editor's list box(es) and potentially other problems. The solution is to use
    // ChangeSelection() instead of SetSelection() at this point.
    // ChangeSelection() does not trigger page changeing events. See:
    // http://docs.wxwidgets.org/stable/wx_eventhandlingoverview.html#progevent
    // for a summary in the docs.
	m_pKBEditorNotebook->ChangeSelection(pageNumSel);
	wxNotebookPage* nbPage = m_pKBEditorNotebook->GetPage(pageNumSel);

	// get pointers to dialog controls on this nbPage:
	// Note: FindWindowById finds the first window with the given ID starting from the
	// top-level frames, etc. We do NOT want this, we want to find the control as
	// a child of nbPage only, so we must use FindWindow, rather than FindWindowById.
	m_pFlagBox = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_SHOW_FLAG);
	wxASSERT(m_pFlagBox != NULL);
	m_pFlagBox->SetBackgroundColour(gpApp->sysColorBtnFace); // read only background color

	m_pTypeSourceBox = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_SRC_KEY);
	wxASSERT(m_pTypeSourceBox != NULL);

	// whm 31Aug2021 modified line below to use the AutoCorrectTextCtrl class which is now
	// used as a custom control in wxDesigner's KBEditorPanelFunc() dialog.
	m_pEditOrAddTranslationBox = (AutoCorrectTextCtrl*)nbPage->FindWindow(IDC_EDIT_EDITORADD);
	wxASSERT(m_pEditOrAddTranslationBox != NULL);

	m_pEditRefCount = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_REF_COUNT);
	wxASSERT(m_pEditRefCount != NULL);
	m_pEditRefCount->SetBackgroundColour(gpApp->sysColorBtnFace); // read only background color

	// whm 31Aug2021 modified line below to use the AutoCorrectTextCtrl class which is now
	// used as a custom control in wxDesigner's KBEditorPanelFunc() dialog.
	m_pEditSearches = (AutoCorrectTextCtrl*)nbPage->FindWindow(ID_TEXTCTRL_SEARCH);
	wxASSERT(m_pEditSearches != NULL);

	m_pStaticCount = (wxStaticText*)nbPage->FindWindow(IDC_STATIC_COUNT);
	wxASSERT(m_pStaticCount != NULL);

	m_pListBoxExistingTranslations = (wxListBox*)nbPage->FindWindow(IDC_LIST_EXISTING_TRANSLATIONS);
	wxASSERT(m_pListBoxExistingTranslations != NULL);

	m_pListBoxKeys = (wxListBox*)nbPage->FindWindow(IDC_LIST_SRC_KEYS);
	wxASSERT(m_pListBoxKeys != NULL);

	m_pComboOldSearches = (wxComboBox*)nbPage->FindWindow(ID_COMBO_OLD_SEARCHES);
	wxASSERT(m_pComboOldSearches != NULL);

	// below are pointers to the buttons on this nbPage page
	m_pBtnUpdate = (wxButton*)nbPage->FindWindow(IDC_BUTTON_UPDATE);
	wxASSERT(m_pBtnUpdate != NULL);
	m_pBtnAdd = (wxButton*)nbPage->FindWindow(IDC_BUTTON_ADD);
	wxASSERT(m_pBtnAdd != NULL);
	m_pBtnRemove = (wxButton*)nbPage->FindWindow(IDC_BUTTON_REMOVE);
	wxASSERT(m_pBtnRemove != NULL);
	m_pBtnRemoveSomeTgtEntries = (wxButton*)nbPage->FindWindow(ID_BUTTON_REMOVE_SOME);
	wxASSERT(m_pBtnRemoveSomeTgtEntries != NULL);
	m_pBtnMoveUp = (wxButton*)nbPage->FindWindow(IDC_BUTTON_MOVE_UP);
	wxASSERT(m_pBtnMoveUp != NULL);
	m_pBtnMoveDown = (wxButton*)nbPage->FindWindow(IDC_BUTTON_MOVE_DOWN);
	wxASSERT(m_pBtnMoveDown != NULL);
	m_pBtnAddNoAdaptation = (wxButton*)nbPage->FindWindow(IDC_ADD_NOTHING);
	wxASSERT(m_pBtnAddNoAdaptation != NULL);
	m_pBtnToggleTheSetting = (wxButton*)nbPage->FindWindow(IDC_BUTTON_FLAG_TOGGLE);
	wxASSERT(m_pBtnToggleTheSetting != NULL);

	m_pBtnGo = (wxButton*)nbPage->FindWindow(ID_BUTTON_GO);
	wxASSERT(m_pBtnGo != NULL);
	m_pBtnEraseAllLines = (wxButton*)nbPage->FindWindow(ID_BUTTON_ERASE_ALL_LINES);
	wxASSERT(m_pBtnEraseAllLines != NULL);

	// most of this was originally in MFC's InitDialog
	wxString s = _("<no adaptation>");

	wxString srcKeyStr;

	wxASSERT(pageNumSel >= 0);
	pMap = pKB->m_pMap[pageNumSel];
	wxASSERT(pMap != NULL);
	m_ON = _("ON"); //IDS_ON // for localization, use string table
	m_OFF = _("OFF"); //IDS_OFF  // ditto

	// Set fonts and directionality for controls in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont,
				m_pTypeSourceBox, NULL, m_pListBoxKeys, NULL, gpApp->m_pDlgSrcFont,
				gpApp->m_bSrcRTL);
	#else
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont,
			m_pTypeSourceBox, NULL, m_pListBoxKeys, NULL, gpApp->m_pDlgSrcFont);
	#endif

	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont,
				m_pEditOrAddTranslationBox, m_pEditSearches, m_pListBoxExistingTranslations,
				NULL, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
		gpApp->SetFontAndDirectionalityForComboBox(gpApp->m_pNavTextFont,
				m_pComboOldSearches, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
		#else
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont,
				m_pEditOrAddTranslationBox, m_pEditSearches, m_pListBoxExistingTranslations,
				NULL, gpApp->m_pDlgTgtFont);
		gpApp->SetFontAndDirectionalityForComboBox(gpApp->m_pNavTextFont,
				m_pComboOldSearches, gpApp->m_pDlgTgtFont);
		#endif
	}
	else
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont,
				m_pEditOrAddTranslationBox, m_pEditSearches, m_pListBoxExistingTranslations,
				NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
		gpApp->SetFontAndDirectionalityForComboBox(gpApp->m_pTargetFont,
				m_pComboOldSearches, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
		#else
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont,
				m_pEditOrAddTranslationBox, m_pEditSearches, m_pListBoxExistingTranslations,
				NULL, gpApp->m_pDlgTgtFont);
		gpApp->SetFontAndDirectionalityForComboBox(gpApp->m_pTargetFont,
				m_pComboOldSearches, gpApp->m_pDlgTgtFont);
		#endif
	}

	// start with an empty src keys listbox
	m_pListBoxKeys->Clear(); // whm added
	// start with an empty translations list box
	m_pListBoxExistingTranslations->Clear();

	// if a page change in the tabbed control was done, and the m_arrSearches array has
	// search strings put there by the OnTabSelChange() function, then restore them
	// to the wxTextCtrl, m_pEditSearches on the new page & then erase the m_arrSearches -
	// we will leave it populated only when the Go button was pressed (to do the searching)
	// now erase the storage in the array; similarly, restore combobox contents, but we
	// won't clear the m_arrOldSearches array (it's done when we leave the project)
	DoRestoreSearchStrings();
	gpApp->m_arrSearches.Empty();
	UpdateComboInEachPage();
	// BEW added 27May13 to properly delete entries with no CRefString instances in them
	// by doing the deletions after the loop is finished
	wxArrayString arrBadEntries; arrBadEntries.clear();

	// get the list of source keys filled
	if (pMap->size() > 0)
	{
		MapKeyStringToTgtUnit::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter)
		{
			srcKeyStr = iter->first;
			pCurTgtUnit = iter->second;
			wxASSERT(pCurTgtUnit != NULL);

            // in case the data is corrupted, (eg. a pCurTgtUnit with an empty refString
            // list) we will check and ignore any such, and also remove them so they cannot
            // make trouble later
			if (pCurTgtUnit->m_pTranslations->IsEmpty())
			{
				// BEW changed 27May13, don't erase, it invalidates the iterator. Store
				// the key for later erasure of its CTargetUnit when the loop is done
				arrBadEntries.Add(srcKeyStr);
				/* removed, 27May13
				pMap->erase(srcKeyStr); // the map now lacks this invalid association
				if (pCurTgtUnit != NULL) // whm 11Jun12 added NULL test
					delete pCurTgtUnit; // and its memory chunk is freed
				*/
				continue;
			}
			int index = 0;
			// BEW 22Jun10, for kbVersion 2 it is possible to have a CTargetUnit instance
			// for which all of its stored CRefString instances are marked as deleted (for
			// example, the user may have manually used the KBEditor to Remove each one);
			// when this is the case, such a CTargetUnit instance should not have it's key
			// listed in the list box - so test for this and handle it appropriately
			int numNotDeleted = pCurTgtUnit->CountNonDeletedRefStringInstances();
			if (numNotDeleted > 0)
			{
				// there is at least one non-deleted element, so put this CTargetUnit
				// instance in the list box
				index = m_pListBoxKeys->Append(srcKeyStr,(void*)pCurTgtUnit);
                wxUnusedVar(index); // whm added 25Jun2015 to avoid gcc "not used" warning
			}
		}
		// BEW 27May13, the loop has finished; remove any bad entries that were skipped over
		if (!arrBadEntries.IsEmpty())
		{
			size_t badCount = arrBadEntries.GetCount();
			size_t i;
			for (i=0; i< badCount; i++)
			{
				MapKeyStringToTgtUnit::iterator iter2; // need a freshly defined iterator per deletion
				wxString aSrcKey = arrBadEntries.Item(i);
				// Look up the CTargetUnit instance for this key
				iter2 = pMap->find(aSrcKey);
				srcKeyStr = iter2->first; // should be same value as is in aSrcKey
				wxASSERT(srcKeyStr == aSrcKey);
				CTargetUnit* pTgtUnit = iter2->second; // the associated ptr to CTargetUnit
				wxASSERT(pTgtUnit != NULL);

				// Remove pTgtUnit from the map which is in memory (at next save, this deletion
				// will go into the on-disk persistent copy of the kb)
				if (pTgtUnit->m_pTranslations->IsEmpty())
				{
					if (pTgtUnit != NULL) // free the mem chunk, don't leak memory
					{
						delete pTgtUnit;
					}
					pMap->erase(aSrcKey); // the map now lacks this invalid association
				}
			}
			arrBadEntries.clear();
		}
	}
	else
	{
		pCurTgtUnit = 0; // no valid pointer set
	}

#if defined(_DEBUG) && defined(DUALS_BUG)
		bool bDoAbaotem = FALSE;
		int counter = 0;
#endif

	// select the first string in the keys listbox by default, if possible; but if
	// m_TheSelectedKey has content, then try to make the selection default
	// to m_TheSelectedKey instead, for rapid access to the desired entry.
    m_currentKey = _T("");
	int nCount = m_pListBoxKeys->GetCount();
	int nTopOfListItem = 0; // initialize (Mike wants some lines above the selection
			// to be visible; I'll give him 3 above, so selection will be 4th line
	if (nCount > 0)
	{
		// check out if we have a desired key to be accessed first
		if (!m_TheSelectedKey.IsEmpty())
		{
           // user wants a certain key selected on entry to the editor, try set it up for
            // him If successful this will change the initial selection passed in the 3rd
            // parameter
			//
            // BEW 9Jun15, changed from a case insensitive search to a case sensitive one,
			// as that is the only way to be consistent with the auto-capitalization feature.
            // Note: The View's OnToolsKBEditor() sets the tab page num before ShowModal()
            // is called.

			m_srcKeyStr = m_TheSelectedKey;
			// BEW 9Jun15, looking at what goes into KBserver, I noticed that no adjustment to
			// initial capital letter is done when gbAutoCaps is TRUE. Fix this.
			bool bNoError = TRUE;
			if (gbAutoCaps)
			{
				bNoError = pApp->GetDocument()->SetCaseParameters(m_srcKeyStr); // for source word or phrase
				if (bNoError)
				{
					m_srcKeyStr = pKB->AutoCapsMakeStorageString(m_srcKeyStr); // might be returned as lower case initial
				}
			}
			
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, m_srcKeyStr, caseSensitive, exactString);
			if (nNewSel == -1) // LB_ERR
			{
				nNewSel = 0; // if not found, default to the first in the list
			}
			m_pListBoxKeys->SetSelection(nNewSel);
			wxString str = m_pListBoxKeys->GetString(nNewSel);
			nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys,str,caseSensitive,exactString);
			wxASSERT(nNewSel != -1);
			pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nNewSel);
			// BEW 4May16, make 3 lines above selection visible, or less if close to the top
			nTopOfListItem = nNewSel - 3;
			// Check we are not out of bounds
			if (nTopOfListItem < 0)
			{
				nTopOfListItem++;
				if (nTopOfListItem < 0)
				{
					nTopOfListItem++;
					if (nTopOfListItem < 0)
					{
						nTopOfListItem++;
					}
				}
			}
		}
		else if (gpApp->m_pActivePile != NULL && nStartingSelection == 0)
		{
			// whm added 13Jan09. No selection has been made, and caller expects a
			// possible zero starting selection, next best thing is to look up the source
			// phrase at the current active location of the phrasebox.
			// get the key for the source phrase at the active location
safe:		wxString srcKey;
			srcKey = gpApp->m_pActivePile->GetSrcPhrase()->m_key;
			// BEW 9Jun15, looking at what goes into KBserver, I noticed that no adjustment to
			// initial capital letter is done when gbAutoCaps is TRUE. Fix this.
			bool bNoError = TRUE;
			if (gbAutoCaps)
			{
				bNoError = pApp->GetDocument()->SetCaseParameters(srcKey); // for source word or phrase
				if (bNoError)
				{
					srcKey = pKB->AutoCapsMakeStorageString(srcKey); // might be returned as lower case initial
				}
			}
			m_srcKeyStr = srcKey;
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, srcKey, caseSensitive, exactString);
			if (nNewSel == -1) // LB_ERR
			{
				nNewSel = 0; // if not found, default to the first in the list
			}
			m_pListBoxKeys->SetSelection(nNewSel);
			wxString str = m_pListBoxKeys->GetString(nNewSel);
			nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys,str,caseSensitive,exactString);
			wxASSERT(nNewSel != -1);
			pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nNewSel);
		}
		else
		{
			m_pListBoxKeys->SetSelection(nStartingSelection);
			wxString str = m_pListBoxKeys->GetString(nStartingSelection);
			// BEW 9Jun15, looking at what goes into KBserver, I noticed that no adjustment to
			// initial capital letter is done when gbAutoCaps is TRUE. Fix this.
			bool bNoError = TRUE;
			if (gbAutoCaps)
			{
				bNoError = pApp->GetDocument()->SetCaseParameters(str); // for source word or phrase
				if (bNoError)
				{
					str = pKB->AutoCapsMakeStorageString(str); // might be returned as lower case initial
				}
			}
			m_srcKeyStr = str;
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, str, caseSensitive, exactString);

			// BEW 30Apr20 if str changes to lower case, and nNewSel is present as a lower case
			// variant, then the user will experience a disturbing jump to a case-adjusted different entry -
			// either before or after, which may have quite a different set of adaptations. So test
			// for the jump (by +/- 1) and if so, then restore to nStartingSelection value
			if ((nNewSel == nStartingSelection - 1) || (nNewSel == nStartingSelection + 1))
			{
				nNewSel = nStartingSelection;
				m_pListBoxKeys->SetSelection(nStartingSelection);
				str = m_pListBoxKeys->GetString(nStartingSelection);
				m_srcKeyStr = str;
			}
			//wxASSERT(nNewSel != -1);
			if (nNewSel != wxNOT_FOUND)
			{
				pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nNewSel);
				// BEW 4May16, make 3 lines above selection visible, or less if close to the top
				nTopOfListItem = nNewSel - 3;
				// Check we are not out of bounds
				if (nTopOfListItem < 0)
				{
					nTopOfListItem++;
					if (nTopOfListItem < 0)
					{
						nTopOfListItem++;
						if (nTopOfListItem < 0)
						{
							nTopOfListItem++;
						}
					}
				}
			}
			else
			{
				// BEW added 30Ap20, because the commented out assert above tripped
				// once when I removed the last of 5 adaptations in the tgt list. I
				// was unable to get the error to repeat, so I've added this safety
				// first block
				int count = m_pListBoxKeys->GetCount();
				if (count > 0)
				{
					nStartingSelection = 0;
					goto safe;
				}
				else
				{
					return;
				}
			}
		}
        m_currentKey = m_pListBoxKeys->GetStringSelection(); // set m_curKey
		m_pListBoxKeys->SetFirstItem(nTopOfListItem);

#if defined(_DEBUG) && defined(DUALS_BUG)
		if (m_currentKey == _T("abaotem"))
		{
			bDoAbaotem = TRUE;
			counter = 0;
			wxLogDebug(_T("\n")); // want a blank line to separate groups
		}
		else
		{
			bDoAbaotem = FALSE;
			counter = 0;
		}
#endif

		// we're not using validators here so fill the textbox, but only if m_srcKeyStr
		// is different from the current contents (avoids a spurious system beep)
		// Also, instead of calling SetValue() we use ChangeValue() here to avoid spurious calling
		// of OnSelChangeListSrcKeys
		// BEW 5May16 don't programmatically put text in this box, it's for user-defined searching
		//if (m_pTypeSourceBox->GetValue() != m_srcKeyStr)
		//{
		//	m_pTypeSourceBox->ChangeValue(m_srcKeyStr);
		//}
		// set members to default state, ready for reuse
		m_nWordsSelected = -1;
		m_TheSelectedKey.Empty();

        // whm modified 3Jan2018 test for bad/NULL pointer
        //// the above could fail, eg. if nothing is in the list box, in which case -1 will be
        ////  put in the pCurTgtUnit variable, so change it to a zero pointer
        //if (pCurTgtUnit <= 0)
        if (pCurTgtUnit == NULL)
		{
			//pCurTgtUnit = 0; // invalid pointer
			m_flagSetting = m_OFF;
			wxMessageBox(_T("Warning: Invalid pointer to current target unit returned."),_T(""), wxICON_EXCLAMATION | wxOK);
		}
		else
		{
			bool bFlagValue = (bool)pCurTgtUnit->m_bAlwaysAsk;
			if (bFlagValue)
				m_flagSetting = m_ON;
			else
				m_flagSetting = m_OFF;
		}

		// we're not using validators here so fill the text ctrl
		m_pFlagBox->SetValue(m_flagSetting);
	}
	// show user how many entries - whm moved here from inside end of above block
	// nCount was set above before if (nCount > 0) block
	//
    // BEW 16Jan13, changed to use two strings, because the specifier string was being
    // loaded from the wxStatic label object, and the result was put back in the wxStatic
    // label object - thereby turning that object into a string without a formatting
    // specifier in it - and then returning to the same page of the KBEditor would crash
    // when wxString::Format() was called with an argument for which a %d specifier no
    // longer exists in the label string
	m_entryCountLabel.Empty();
	m_entryCountLabel = m_entryCountLabel.Format(m_entryCountStr,nCount);
	m_pStaticCount->SetLabel(m_entryCountLabel);

	// fill the translations listbox for the pCurTgtUnit selected
	// BEW 22Jun10, tweaked the code to support both glossing and adapting KBs when trying
	// to set nMatchedRefString, and also put in support for kbVersion 2's m_bDeleted flag
	// - any CRefString instances with that flag set TRUE have to be excluded from being
	// shown to the user in the list; also, the count needs to be done for just the
	// non-deleted instances, so use CKB::CountNonDeletedRefStringInstances(CTargetUnit*)
	if (pCurTgtUnit != NULL)
	{
		int countNonDeleted = 0;
                wxUnusedVar(countNonDeleted); // whm added 25Jun2015 to avoid gcc "unused" warning
		countNonDeleted = pCurTgtUnit->CountNonDeletedRefStringInstances();
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->GetFirst();
		wxASSERT(pos != NULL);
		int nMatchedRefString = -1; // whm added 24Jan09
		int countShowable = 0;
		while (pos != NULL)
		{
			pCurRefString = (CRefString*)pos->GetData();

#if defined(_DEBUG) && defined(DUALS_BUG)
			if (bDoAbaotem)
			{
				counter++;
				wxString flag = pCurRefString->GetDeletedFlag() ? _T("TRUE") : _T("FALSE");
				wxLogDebug(_T("LoadDataForPage(): counter = %d  countNonDeleted = %d  m_translation = %s  m_bDeleted = %s"),
					counter, countNonDeleted,
					pCurRefString->m_translation.c_str(), flag.c_str());
			}
#endif

			pos = pos->GetNext();
			// only put into the list CRefString instances which are not marked as deleted
			if (!pCurRefString->GetDeletedFlag())
			{
				countShowable++; // it's not deleted so count it
				wxString str = pCurRefString->m_translation;
				if (str.IsEmpty())
				{
					str = s; // set "<no adaptation>"
				}

				int anIndex = m_pListBoxExistingTranslations->Append(str,pCurRefString); // whm
														// modified 24Jan09 to use 2nd param
				// If the ref string's m_translation member is the same as the m_adaption or,
				// for a glossingKB, the m)gloss, set nMatchedRefString to the current index
				if (GetGlossingKBFlag(pKB))
				{
					// no punctuation stripping is done with the glossingKB, so just test for
					// a match with m_gloss contents (which could contain puntuation)
					if (gpApp->m_pActivePile != NULL && pCurRefString->m_translation ==
										gpApp->m_pActivePile->GetSrcPhrase()->m_gloss)
					{
						nMatchedRefString = anIndex;
					}
				}
				else
				{
					// its an adaptingKB (use m_adaption, as it has punctuation stripped off,
					// whereas m_targetStr doesn't and is never entered into the KB's maps)
					if (gpApp->m_pActivePile != NULL && pCurRefString->m_translation ==
										gpApp->m_pActivePile->GetSrcPhrase()->m_adaption)
					{
						nMatchedRefString = anIndex;
					}
				}
			} // end TRUE block for test: if (!pCurRefString->m_bDeleted)
		} // end while loop

		//wxASSERT(countShowable > 0 && countShowable == countNonDeleted); // <<-- remove
		// because if the user has removed all the list's entries, this assert would trip
		// even though no error has happened

		// if possible select the matched m_translation in the Ref String
        int listcount = m_pListBoxExistingTranslations->GetCount();
		if (nMatchedRefString != -1)
		{
			// there was a match
			wxASSERT(nMatchedRefString < (int)m_pListBoxExistingTranslations->GetCount());
			m_pListBoxExistingTranslations->SetSelection(nMatchedRefString);
		}
		else
		{
            // select the first translation in the listbox by default -- there will always
            // be at least one - except when the user has just used the Remove button to
            // remove all the entries
			if (listcount > 0)
			{
				m_pListBoxExistingTranslations->SetSelection(0);
			}
		}
		if (listcount > 0)
		{
			wxString str = m_pListBoxExistingTranslations->GetStringSelection();
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,caseSensitive,exactString);
			wxASSERT(nNewSel != -1);
			pCurRefString = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nNewSel);
			wxASSERT(pCurRefString != 0);
			m_refCount = pCurRefString->m_refCount;
			m_refCountStr.Empty();
			m_refCountStr << m_refCount;
			if (m_refCount < 0)
			{
				m_refCount = 0; // an error condition
				m_refCountStr = _T("0");
				wxMessageBox(_T("A value of zero for the reference count means there was an error."),_T(""),wxICON_EXCLAMATION | wxOK);
			}

			// get the selected translation text; if nNewSel is the same value as the default
			// selection of index 0, then m_edTransStr can be got from the top element in the
			// list, but if nNewSel is > 0, we must assign str's contents to it instead
			if (nNewSel > 0)
			{
				m_edTransStr = str;
			}
			else
			{
				m_edTransStr = m_pListBoxExistingTranslations->GetString(0);
			}
		}
		else
		{
			m_edTransStr.Empty();
		}
	}

	//m_pTypeSourceBox->SetSelection(-1,-1); // select all, BEW 5May16 commented it out
	m_pTypeSourceBox->SetFocus();

	m_pEditRefCount->ChangeValue(m_refCountStr);
	m_pEditOrAddTranslationBox->ChangeValue(m_edTransStr);// whm added
	// UpdateButtons(); whm 2May2022 removed. UpdateButtons() is now called in OnIdle, the main/only place where UpdateButtons() needs to be called.
}

/*
void CKBEditor::UpdateButtons()
{
	// enables or disables buttons to reflect valid choices depending on number of
	// items in list and which are selected

	// disable Update, Add, MoveUp, MoveDown, and Toggle buttons initially (may be enabled below)
	//m_pBtnUpdate->Enable(FALSE);
	//m_pBtnAdd->Enable(FALSE);
	//m_pBtnMoveUp->Enable(FALSE);
	//m_pBtnMoveDown->Enable(FALSE);
	// The "Toggle The Setting" should be enabled except for when the translations list is empty
	// as when displaying a tab without any data
	//if (m_pListBoxExistingTranslations->GetCount() == 0)
	//{
	//	if (m_pBtnToggleTheSetting->IsEnabled())
	//		m_pBtnToggleTheSetting->Enable(FALSE);
	//}
	//else
	//{
	//	if (!m_pBtnToggleTheSetting->IsEnabled())
	//		m_pBtnToggleTheSetting->Enable(TRUE);
	//}
	// enable the Add <no adaptation> button initially
	//m_pBtnAddNoAdaptation->Enable(TRUE);
	// Since there should be at least 1 translation in the existing translations box
	// for every source phrase, the Remove button should always be enabled. Whenever the only
	// translation of a given source phrase is removed, the source phrase itself is also removed.
	// The one time the "Remove" button should be disabled is when the list is empty which occurs
	// on tab pages having no items.
	//if (m_pListBoxExistingTranslations->GetCount() == 0)
	//{
	//	if (m_pBtnRemove->IsEnabled())
	//		m_pBtnRemove->Enable(FALSE);
	//}
	//else
	//{
	//	if (!m_pBtnRemove->IsEnabled())
	//		m_pBtnRemove->Enable(TRUE);
	//}
	// The Remove Some Translations button can be called from any panel of the tabbed
	// dialog. It lists tgt <> src and a checkbox, in two user choosable formats, in a
	// child dialog with a large listbox, doing it for ALL target text adaptations (with
	// their source text shown second) in the whole KB - ie. all lengths of phrase; the
	// button is enabled in any panel which has at least a single source text key
	//if (m_pListBoxKeys->GetCount() == 0)
	//{
	//	if (m_pBtnRemoveSomeTgtEntries->IsEnabled())
	//		m_pBtnRemoveSomeTgtEntries->Enable(FALSE);
	//}
	//else
	//{
	//	if (!m_pBtnRemoveSomeTgtEntries->IsEnabled())
	//		m_pBtnRemoveSomeTgtEntries->Enable(TRUE);
	//}
	//// Enable those buttons meeting certain conditions depending on number of items and selection
	//int nSel = m_pListBoxExistingTranslations->GetSelection();
	//wxString selectedTrans;
	//if (m_pListBoxExistingTranslations->GetCount() > 0)
	//	selectedTrans = m_pListBoxExistingTranslations->GetStringSelection();
	//else
	//	selectedTrans.Empty();
	//if (m_pListBoxExistingTranslations->GetCount() > 1 && nSel > 0)
	//{
	//	// The Move Up button is enabled whenever there are more than 1 translations in the list
	//	// and the selected item is not the last one in the list
	//	if (!m_pBtnMoveUp->IsEnabled())
	//		m_pBtnMoveUp->Enable(TRUE);
	//}
	//else
	//{
	//	if (m_pBtnMoveUp->IsEnabled())
	//		m_pBtnMoveUp->Enable(FALSE);
	//}
	//if (m_pListBoxExistingTranslations->GetCount() > 1 && nSel < (int)m_pListBoxExistingTranslations->GetCount()-1)
	//{
	//	// The Move Down button is enabled whenever there are more than 1 translations in the list
	//	// and the selected item is not the first one in the list
	//	if (!m_pBtnMoveDown->IsEnabled())
	//		m_pBtnMoveDown->Enable(TRUE);
	//}
	//else
	//{
	//	if (m_pBtnMoveDown->IsEnabled())
	//		m_pBtnMoveDown->Enable(FALSE);
	//}
	//if (m_pEditOrAddTranslationBox->IsModified() && m_pEditOrAddTranslationBox->GetValue() != m_pListBoxExistingTranslations->GetStringSelection())
	//{
	//	// The Update button is enabled whenever the edit translation text box is dirty
	//	if (!m_pBtnUpdate->IsEnabled())
	//		m_pBtnUpdate->Enable(TRUE);
	//}
	//else
	//{
	//	if (m_pBtnUpdate->IsEnabled())
	//		m_pBtnUpdate->Enable(FALSE);
	//}
	//wxString stringBeingTyped;
	//stringBeingTyped = m_pEditOrAddTranslationBox->GetValue();
	//if (m_pEditOrAddTranslationBox->IsModified() &&  !IsInListBox(m_pListBoxExistingTranslations, stringBeingTyped))
	//{
	//	// The Add button is enabled whenever the edit translation text box is dirty, and
	//	// the current string in the box is not already in the m_pListBoxExistingTranslations list.
	//	if (!m_pBtnAdd->IsEnabled())
	//		m_pBtnAdd->Enable(TRUE);
	//}
	//else
	//{
	//	if (m_pBtnAdd->IsEnabled())
	//		m_pBtnAdd->Enable(FALSE);
	//}
	//if (m_pListBoxExistingTranslations->GetCount() == 0 || IsInListBox(m_pListBoxExistingTranslations, _("<no adaptation>")))
	//{
	//	// the AddNoAdaptation button is always enabled unless "<no adaptation>" is in the
	//	// m_pListBoxExistingTranslations list
	//	if (m_pBtnAddNoAdaptation->IsEnabled())
	//	m_pBtnAddNoAdaptation->Enable(FALSE);
	//}
	//else
	//{
	//	if (!m_pBtnAddNoAdaptation->IsEnabled())
	//		m_pBtnAddNoAdaptation->Enable(TRUE);
	//}
}
*/

bool CKBEditor::IsInListBox(wxListBox* listBox, wxString str)
{
	bool found = FALSE;
	int count = listBox->GetCount();
	if (count == 0 || str.IsEmpty()) return FALSE;
	int ct;
	for (ct = 0; ct < count; ct++)
	{
		if (listBox->GetString(ct) == str)
		{
			found = TRUE;
			break;
		}
	}
	return found;
}

void CKBEditor::OnComboItemSelected(wxCommandEvent& event)
{
	// add combobox item selected to the multiline wxTextCtrl which is for the set of
	// user-defined search strings to be used for searching the KB
	DoRetain();
	wxString strSearch = m_pComboOldSearches->GetStringSelection();
	gpApp->m_arrSearches.Add(strSearch);

	m_pEditSearches->ChangeValue(_T("")); // clear it
	int count = gpApp->m_arrSearches.GetCount();
	wxString eol = _T("\n");
	wxString str;
	int index;
	for (index = 0; index < count; index++)
	{
		str = gpApp->m_arrSearches.Item(index);
		str += eol;
		m_pEditSearches->AppendText(str);
	}
	event.Skip();
}

void CKBEditor::UpdateComboInEachPage()
{
	m_pComboOldSearches->Clear();

	int count = gpApp->m_arrOldSearches.GetCount();
	if (count > 0)
	{
		// fill with the set of items
		for (int index = 0; index < count; index++)
		{
			m_pComboOldSearches->Append(gpApp->m_arrOldSearches.Item(index));
		}
		// select the top
        m_pComboOldSearches->SetSelection(0);
    }
}

/* these two are not needed, as it turns out
// A helper for Move Up and Move Down buttons (required because kbVersion 2 stores deleted
// CRefString instances in the KB, and so the moving up or down over the contents of a
// CTargetUnit instance will have to skip over any deleted CRefString instances in its
// m_pTranslations list; so we'll do the moves and then repopulate the list from scratch
// for each move)
// params:
// pTgtUnit    ->  the current CTargetUnit instance whose contents are displayed in the
//                 KBEditor's m_pTranslations list box
// BEW added 22Jun10, in support of kbVersion 2
void CKBEditor::PopulateTranslationsListBox(CTargetUnit* pTgtUnit)
{
	wxASSERT(pTgtUnit);
	wxString s = _("<no adaptation>");
	int countNonDeleted = pKB->CountNonDeletedRefStringInstances(pTgtUnit);
	TranslationsList::Node* pos = pTgtUnit->m_pTranslations->GetFirst();
	wxASSERT(pos != NULL);
	int countShowable = 0;
	CRefString* pRefStr = NULL;
	int anIndex = -1;
	while (pos != NULL)
	{
		pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		// only put into the list CRefString instances which are not marked as deleted
		if (!pRefStr->GetDeletedFlag())
		{
			countShowable++; // it's not deleted so count it
			wxString str = pRefStr->m_translation;
			if (str.IsEmpty())
			{
				str = s; // set "<no adaptation>"
			}
            // cast to (void*) to ensure pRefStr is not considered owned by this list, so
            // that a later Clear() call will not delete any of the client data
			anIndex = m_pListBoxExistingTranslations->Append(str,(void*)pRefStr);
		} // end TRUE block for test: if (!pCurRefString->m_bDeleted)
	} // end while loop
	wxASSERT(countShowable == countNonDeleted);
	countShowable = countNonDeleted; // avoid a compiler warning in Release build
}

void CKBEditor::EraseTranslationsListBox()
{
	m_pListBoxExistingTranslations->Clear();
}
*/


