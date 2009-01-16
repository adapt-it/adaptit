/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBEditor.cpp
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
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
/// \derivation		The CKBEditor class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in KBEditor.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "KBEditor.h"
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
#include "KBEditor.h" 
#include "KB.h"
#include "TargetUnit.h"
#include "RefString.h"
#include "Pile.h"
#include "Adapt_ItView.h"
#include "Adapt_ItDoc.h"
#include "helpers.h"

// for support of fast access to a selected CSourcePhrase's entry in the KB
extern int gnWordsInPhrase; // use -1 as a flag for disabling the feature
extern wxString gTheSelectedKey; // if multiple keys selected, take only the first

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

/// This global is defined in Adapt_ItView.cpp.
extern bool gbGlossingUsesNavFont;

/// This global is defined in Adapt_It.cpp.
extern int	gnMapLength[10];

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

//const int ID_DUMMY = 22000;

// event handler table
BEGIN_EVENT_TABLE(CKBEditor, AIModalDialog)
	EVT_INIT_DIALOG(CKBEditor::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_NOTEBOOK_PAGE_CHANGED(ID_KB_EDITOR_NOTEBOOK, CKBEditor::OnTabSelChange)
	EVT_LISTBOX(IDC_LIST_SRC_KEYS, CKBEditor::OnSelchangeListSrcKeys)
	EVT_LISTBOX(IDC_LIST_EXISTING_TRANSLATIONS, CKBEditor::OnSelchangeListExistingTranslations)
	EVT_LISTBOX_DCLICK(IDC_LIST_EXISTING_TRANSLATIONS, CKBEditor::OnDblclkListExistingTranslations)
	EVT_TEXT(IDC_EDIT_SRC_KEY, CKBEditor::OnUpdateEditSrcKey)
	EVT_TEXT(IDC_EDIT_EDITORADD, CKBEditor::OnUpdateEditOrAdd)
	EVT_BUTTON(IDC_BUTTON_UPDATE, CKBEditor::OnButtonUpdate)
	EVT_BUTTON(IDC_BUTTON_ADD, CKBEditor::OnButtonAdd)
	EVT_BUTTON(IDC_ADD_NOTHING, CKBEditor::OnAddNoAdaptation)
	EVT_BUTTON(IDC_BUTTON_REMOVE, CKBEditor::OnButtonRemove)
	EVT_BUTTON(IDC_BUTTON_MOVE_UP, CKBEditor::OnButtonMoveUp)
	EVT_BUTTON(IDC_BUTTON_MOVE_DOWN, CKBEditor::OnButtonMoveDown)
	EVT_BUTTON(IDC_BUTTON_FLAG_TOGGLE, CKBEditor::OnButtonFlagToggle)
	//EVT_BUTTON(ID_DUMMY, CKBEditor::OnAccelChangePage) // testing - not working
END_EVENT_TABLE()


CKBEditor::CKBEditor(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Edit or Examine the Knowledge Base"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.

	// wx Note: Since InsertPage also calls SetSelection (which in turn activates our OnTabSelChange
	// handler, we need to initialize some variables before KBEditorDlgFunc is called below.
	// Specifically m_nCurPage and pKB needs to be initialized - so no harm in putting all vars
	// here before the dialog contorls are created via KBEditorDlgFunc.
	m_edTransStr = _T("");
	m_srcKeyStr = _T("");
	m_refCount = 0;
	m_refCountStr = _T("");
	m_flagSetting = _T("");
	m_entryCountStr = _T("");

	// whm: In the following, I've changed the "ON" to "YES" and "OFF" to "NO" because it signifies 
	// the status of "Force Choice For This Item is ___". This makes it possible to distinguish 
	// localizations for the other place in the program where "ON" and "OFF" is used in relation to
	// Book Folder mode, where we have "Book folder mode is ___" where "ON" and "OFF" is appropriate
	// but "YES" or "NO" doesn't fit so well. In the localization a given string can only have one
	// translation that must work for the string literal used everywhere in the program.
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
	m_curKey = _T("");
	m_nWords = 1;
	m_nCurPage = 0; // default to first page (1 Word)
//#ifdef __WXGTK__
//	m_bListBoxBeingCleared = FALSE;
//#endif

	KBEditorDlgFunc(this, TRUE, TRUE);
	// The declaration is: KBEditorDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// pointers to the controls common to each page (most of them) are obtained within
	// the LoadDataForPage() function

	// the following pointer to the KBEditor's wxNotebook control is a single instance; 
	// it can only be associated with a pointer after the KBEditorDlgFunc above call
	m_pKBEditorNotebook = (wxNotebook*)FindWindowById(ID_KB_EDITOR_NOTEBOOK);
	wxASSERT(m_pKBEditorNotebook != NULL);

	// testing - can't get these to work
    //wxAcceleratorEntry entries[10];
    //entries[0].Set(wxACCEL_ALT, (int) '1', ID_DUMMY);
    //entries[1].Set(wxACCEL_ALT, (int) '2', ID_DUMMY);
    //entries[2].Set(wxACCEL_ALT, (int) '3', ID_DUMMY);
    //entries[3].Set(wxACCEL_ALT, (int) '4', ID_DUMMY);
    //entries[4].Set(wxACCEL_ALT, (int) '5', ID_DUMMY);
    //entries[5].Set(wxACCEL_ALT, (int) '6', ID_DUMMY);
    //entries[6].Set(wxACCEL_ALT, (int) '7', ID_DUMMY);
    //entries[7].Set(wxACCEL_ALT, (int) '8', ID_DUMMY);
    //entries[8].Set(wxACCEL_ALT, (int) '9', ID_DUMMY);
    //entries[9].Set(wxACCEL_ALT, (int) 'W', ID_DUMMY);
    //wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    //SetAcceleratorTable(accel);

	// other attribute initializations
}

CKBEditor::~CKBEditor()
{
}

//void CKBEditor::OnAccelChangePage(wxCommandEvent& event)
//{
//	int junk = 1;
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
	// if we get to here user selected a different page
	m_srcKeyStr.Empty(); // we don't want a string from a previous page displayed on tab with no entries
	m_edTransStr.Empty(); // ditto...
	m_refCountStr.Empty(); // ditto...
	m_flagSetting.Empty(); // ditto...
	m_nCurPage = pageNumSelected;

	// Set up new page data by populating list boxes and controls
	m_nWords = pageNumSelected + 1;
	LoadDataForPage(pageNumSelected,0); // start with first item selected for new tab page
}

void CKBEditor::OnSelchangeListSrcKeys(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately. Unfortunately,
	// OnSelchangeListSrcKeys is called before the internal listbox data is cleared so GetCount() still 
	// returns the count of items in the list. As a work around, I've added a CKBEditor member called
	// m_bListBoxBeingCleared boolean which is set to TRUE just before calling Clear() on the listbox
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxKeys))
	{
		return;
	}

//#ifdef __WXGTK__
//	if (m_pListBoxKeys->GetCount() == 0 || m_bListBoxBeingCleared)
//	{
//		m_bListBoxBeingCleared = FALSE;
//		return;
//	}
//#else
//	if (m_pListBoxKeys->GetCount() == 0)
//		return;
//#endif

	wxString s;
	// IDS_NO_ADAPTATION
	s = _("<no adaptation>");

	int nSel;
	nSel = m_pListBoxKeys->GetSelection();
	if (nSel == -1) // LB_ERR
	{
		wxMessageBox(_("Keys list box error when getting the current selection"), _T(""), wxICON_EXCLAMATION);
		wxASSERT(FALSE);
		//AfxAbort();
	}
	wxString str;
	str = m_pListBoxKeys->GetStringSelection();
	int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys,str,caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nNewSel); //pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nSel);
	wxASSERT(pCurTgtUnit != NULL);
	m_curKey = str;

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
	m_pListBoxExistingTranslations->Clear();
	if (pCurTgtUnit != NULL)
	{
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->GetFirst();
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			pCurRefString = (CRefString*)pos->GetData();
			pos = pos->GetNext();
			wxString str = pCurRefString->m_translation;
			if (str.IsEmpty())
			{
				str = s;
			}
			m_pListBoxExistingTranslations->Append(str); // m_pListBoxExistingTranslations is not sorted
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,caseSensitive,exactString);
			wxASSERT(nNewSel != -1); // we just added it so it must be there!
			m_pListBoxExistingTranslations->SetClientData(nNewSel,pCurRefString);
		}

		// select the first translation in the listbox by default
		wxASSERT(m_pListBoxExistingTranslations->GetCount() > 0);
		m_pListBoxExistingTranslations->SetSelection(0);
		str = m_pListBoxExistingTranslations->GetStringSelection();
		int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,caseSensitive,exactString);
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
				wxMessageBox(_T("A value of zero for the reference count means there was an error."));
			}

			// get the selected translation text
			m_edTransStr = m_pListBoxExistingTranslations->GetStringSelection();
		}
	}

	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	UpdateButtons();
}

void CKBEditor::OnSelchangeListExistingTranslations(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		return;
	}
	//int nCount;
	//nCount = m_pListBoxExistingTranslations->GetCount();
	//if (nCount == 0)
	//	return;

	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>" 

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	// In wxGTK, when m_pListBoxExistingTranslations->Clear() is called it triggers this OnSelchangeListExistingTranslations
	//	// handler. The following message is of little help to the user even if it were called for a genuine
	//	// problem, so I've commented it out, so the present handler can exit gracefully
	//	//wxMessageBox(_T("Translations list box error when getting the current selection"), 
	//	//	_T(""), wxICON_EXCLAMATION);
	//	//wxASSERT(FALSE);
	//	return; //AfxAbort();
	//}
	wxString str;
	str = m_pListBoxExistingTranslations->GetStringSelection();
	int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,caseSensitive,exactString);
	wxASSERT(nNewSel != -1);
	CRefString* pRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nNewSel);
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
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	UpdateButtons();
}

void CKBEditor::OnDblclkListExistingTranslations(wxCommandEvent& event) 
{
	OnSelchangeListExistingTranslations(event);
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent that is generated when the contents of the 
///                         KB Dialog Editor's "Type Key To Be Found" edit box changes
/// \remarks
/// Called from: The wxCommandEvent mechanism when the contents of the KB Dialog Editor's 
/// "Type Key To Be Found" edit box changes. If the edit box is empty the handler returns immediately.
/// If the string typed into the edit box is found the that found item is selected in the list and
/// this handler calls OnSelchangeListSrcKeys(). If the string typed so far is not found as a 
/// substring of a list item, a beep is emitted.
// //////////////////////////////////////////////////////////////////////////////////////////
void CKBEditor::OnUpdateEditSrcKey(wxCommandEvent& event) 
{
	// assuming that another char was typed, find the nearest matching key in the list
	m_srcKeyStr = m_pTypeSourceBox->GetValue(); // this is what user has typed into edit box
	// if user deleted last char in the edit box, we don't want to generate a beep or move
	// the selection
	if (m_srcKeyStr.IsEmpty())
		return;
	//m_flagSetting, m_entryCountStr and m_refCountStr don't need to come from window to the variables
	// wx version note: wxListBox::FindString doesn't have a second parameter to find from
	// a certain position in the list. We'll do it differently and use our own function which
	// finds the first item having case insensitive chars the same as what user has typed into 
	// the "key to be found" edit box. For this search, we can return an index of a substring
	// at the beginning of the list item.
	int nSel = gpApp->FindListBoxItem(m_pListBoxKeys, m_srcKeyStr, caseInsensitive, subString);

	if (nSel == -1) // LB_ERR
	{
		::wxBell();
		return;
	}
	m_pListBoxKeys->SetSelection(nSel,TRUE);
	OnSelchangeListSrcKeys(event);
}

// //////////////////////////////////////////////////////////////////////////////////////////
/// \return		nothing
/// \param      event   -> the wxCommandEvent that is generated when the contents of the 
///                         KB Dialog Editor's "Edit or Add a Translation" edit box changes
/// \remarks
/// Called from: The wxCommandEvent mechanism when the contents of the KB Dialog Editor's 
/// "Edit or Add a Translation" edit box changes. This handler simply calls the CKBEditor's
/// UpdateButtons() helper function whenever the edit box changes.
// //////////////////////////////////////////////////////////////////////////////////////////
void CKBEditor::OnUpdateEditOrAdd(wxCommandEvent& WXUNUSED(event)) 
{
	// This OnUpdateEditOrAdd is called every time SetValue() is called which happens
	// anytime the user selects a marker from the list box, even though he makes no changes to
	// the associated text in the control. That is a difference between
	// the EVT_TEXT event macro design and MFC's ON_EN_TEXT macro design. Therefore we need
	// to add the enclosing if block with a IsModified() test to the wx version.
	// Enable the Update button if the text is dirty and the text is different from
	// the selected item in existing translations
	wxString testStrTransBox,testStrListBox;
	testStrTransBox = m_pEditOrAddTranslationBox->GetValue();
	testStrListBox = m_pListBoxExistingTranslations->GetStringSelection();
	UpdateButtons();
}

void CKBEditor::OnButtonUpdate(wxCommandEvent& WXUNUSED(event)) 
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	//if(nSel == -1) // LB_ERR
	//{
	//	::wxBell();
	//	return;
	//}
	wxString oldText;
	oldText = m_pListBoxExistingTranslations->GetStringSelection();
	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>" 

	// ensure that it is not a "<Not In KB>" entry
	if (oldText == _T("<Not In KB>"))
	{
		// IDS_ILLEGAL_EDIT
		wxMessageBox(_("Sorry, editing this kind of entry is not permitted."),_T(""), wxICON_INFORMATION);
		return;
	}

	wxASSERT(pCurTgtUnit != 0);
	wxString newText = _T("");
	newText = m_pEditOrAddTranslationBox->GetValue();

	// IDS_CONSISTENCY_CHECK_NEEDED
	int ok = wxMessageBox(_("Changing the spelling in this editor will leave the instances in the document unchanged. (Do a  Consistency Check later to fix this problem.) Do you wish to go ahead with the spelling change?"),_T(""),wxYES_NO);
	if (ok != wxYES)
		return;

	if (newText.IsEmpty())
	{
		// IDS_REDUCED_TO_NOTHING
		int value = wxMessageBox(_("You have made the translation nothing. This is okay, but is it what you want to do?"),_T(""), wxYES_NO);
		if (value != wxYES)
			return;
	}

	// ensure we are not duplicating a translation already in the list box
	wxString str = _T("");
	int nStrings = m_pListBoxExistingTranslations->GetCount();
	for (int i=0; i < nStrings; i++)
	{
		str = m_pListBoxExistingTranslations->GetString(i);
		if (str == s)
			str = _T("");
		if ((str.IsEmpty() && newText.IsEmpty()) || (str == newText))
		{
			// IDS_TRANSLATION_ALREADY_EXISTS
			wxMessageBox(_("Sorry, the translation you are attempting to associate with the current source phrase already exists."),_T(""),wxICON_INFORMATION);
			return;
		}
	}

	// go ahead and do the update
	wxASSERT(pCurRefString != NULL);
	pCurRefString->m_translation = newText;
	pCurRefString->m_refCount = 1;
	if (newText.IsEmpty())
		newText = s;
	m_pListBoxExistingTranslations->Insert(newText,nSel); //int nLocation = m_pListBoxExistingTranslations->InsertString(nSel,newText);
	// whm comment: MFC's FindStringExact and FindString do NOT do a case sensitive match by default so we'll 
	// use our own more flexible FindListBoxItem method below with caseSensitive and exactString parameters.
	// m_pListBoxExistingTranslations is NOT sorted so the following should work
	int nOldLoc = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, oldText, caseSensitive, exactString);
	wxASSERT(nOldLoc != -1); // LB_ERR;
	m_pListBoxExistingTranslations->Delete(nOldLoc);
	int nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, newText, caseSensitive, exactString);
	m_pListBoxExistingTranslations->SetSelection(nLocation,TRUE);
	m_pListBoxExistingTranslations->SetClientData(nLocation,pCurRefString);
	m_refCount = 1;
	m_refCountStr = _T("1");

	m_edTransStr = newText;
	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);

	UpdateButtons();
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
}

void CKBEditor::OnAddNoAdaptation(wxCommandEvent& event)
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	// error, there must be *something* in the list of translations
	//	::wxBell();
	//	return;
	//}
	wxString oldText;
	oldText = m_pListBoxExistingTranslations->GetStringSelection();

	// ensure that it is not a "<Not In KB>" targetUnit that we are trying to add more to
	if (oldText == _T("<Not In KB>"))
	{
		// can't add to such an entry, it <Not In KB> is present, no other entry is allowed
		::wxBell();
		return;
	}

	wxString newText = _T(""); // this is what the user wants to add
	m_edTransStr = m_pEditOrAddTranslationBox->GetValue();
	m_srcKeyStr = m_pTypeSourceBox->GetValue();
	wxASSERT(pCurTgtUnit != 0);
	bool bOK = AddRefString(pCurTgtUnit,newText); // adds it, provided it is not already there

	if (bOK)
	{
		// don't add to the list if the AddRefString call did not succeed
		wxString s;
		s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>" 
		newText = s; // i.e. "<no adaptation>"
		m_pListBoxExistingTranslations->Append(newText);
		// m_pListBoxExistingTranslations is not sorted, but it is safer to always get an index using FindListBoxItem
		int nFound = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, newText, caseSensitive, exactString);
		if (nFound == -1) // LB_ERR
		{
			wxMessageBox(_("Error warning: Did not find the translation text just inserted!"),_T(""),wxICON_WARNING);
			m_pListBoxExistingTranslations->SetSelection(0,TRUE);
			m_pEditRefCount->SetValue(m_refCountStr);
			m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
			return;
		}
		m_pListBoxExistingTranslations->SetClientData(nFound,pCurRefString);
		m_pListBoxExistingTranslations->SetSelection(nFound,TRUE); //m_pListBoxExistingTranslations->SetCurSel(nFound);
		OnSelchangeListExistingTranslations(event);
		m_pEditRefCount->SetValue(m_refCountStr);
		m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
		UpdateButtons();
	}
}

void CKBEditor::OnButtonAdd(wxCommandEvent& event) 
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	::wxBell();
	//	return;
	//}
	wxString oldText;
	oldText = m_pListBoxExistingTranslations->GetStringSelection();

	// ensure that it is not a "<Not In KB>" targetUnit that we are trying to add more to
	if (oldText == _T("<Not In KB>"))
	{
		::wxBell();
		return;
	}

	wxString newText = _T("");
	newText = m_pEditOrAddTranslationBox->GetValue();
	if (newText.IsEmpty())
	{
		// IDS_NOT_NOTHING
		int value = wxMessageBox(_("You adding a translation which is nothing. That is okay, but is it what you want to do?"),_T(""), wxYES_NO);
		if (value == wxNO)
			return;
	}
	m_edTransStr = m_pEditOrAddTranslationBox->GetValue();
	m_srcKeyStr = m_pTypeSourceBox->GetValue();
	wxASSERT(pCurTgtUnit != 0);
	bool bOK = AddRefString(pCurTgtUnit,newText);

	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>" 

	// if it was added successfully, show it in the listbox & select it
	if (bOK)
	{
		if (newText.IsEmpty())
			newText = s; // i.e. "<no adaptation>"
		m_pListBoxExistingTranslations->Append(newText);
		// m_pListBoxExistingTranslations is not sorted, but it is safer to always get an index using FindListBoxItem
		int nFound = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, newText, caseSensitive, exactString);
		if (nFound == -1) // LB_ERR
		{
			wxMessageBox(_("Error warning: Did not find the translation text just inserted!"),_T(""),wxICON_WARNING);
			m_pListBoxExistingTranslations->SetSelection(0,TRUE);
			m_pEditRefCount->SetValue(m_refCountStr);
			m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
			return;
		}
		m_pListBoxExistingTranslations->SetClientData(nFound,pCurRefString);
		m_pListBoxExistingTranslations->SetSelection(nFound,TRUE);
		OnSelchangeListExistingTranslations(event);
		m_pEditRefCount->SetValue(m_refCountStr);
		m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
		m_pEditOrAddTranslationBox->DiscardEdits(); // resets the internal "modified" flag to no longer "dirty"
		UpdateButtons();
		gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
	}
}

void CKBEditor::OnButtonRemove(wxCommandEvent& WXUNUSED(event)) 
{
	// this button must remove the selected translation from the KB, which means that
	// user must be shown a child dialog or message to the effect that there are m_refCount instances
	// of that translation in this and previous document files which will then not agree with
	// the state of the knowledge base, and the user is then to be urged to do a Verify operation
	// on each of the existing document files to get the KB and those files in synch with each other.
	
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		::wxBell();
		return;
	}
	
	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>" 
	wxString message;
	int	nPreviousReferences = 0;

	// get the index of the selected translation string (this will be same index for the CRefString
	// stored in pCurTgtUnit)
	int nTransSel = 0;
	nTransSel = m_pListBoxExistingTranslations->GetSelection();
	//if (nTransSel == -1) // LB_ERR
	//{
	//	::wxBell();
	//	return;
	//}

	// get the selected string
	wxString str;
	str = m_pListBoxExistingTranslations->GetStringSelection(); // the list box translation string being deleted
	wxString str2 = str;
	
	if (str == s) // ie. if contents of str is "<no adaptation>"
		str = _T(""); // for comparison's with what is stored in the KB, it must be empty

	// don't allow an attempt to remove <Not In KB>
	if (str == _T("<Not In KB>"))
	{
		// IDS_ILLEGAL_REMOVE
		wxMessageBox(_("Sorry, you can only remove this kind of entry by putting the phrase box at the relevant location in the document, and then click the  \"Save To Knowledge Base\" checkbox back to ON."),_T(""),wxICON_INFORMATION);
		return;
	}

	// find the corresponding CRefString instance in the knowledge base, and set the
	// nPreviousReferences variable for use in the message box; if user hits Yes
	// then go ahead and do the removals
	wxASSERT(pCurTgtUnit != NULL);
	TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->Item(nTransSel);
	wxASSERT(pos != NULL); // it must be there!
	CRefString* pRefString = (CRefString*)pos->GetData();
	wxASSERT(pRefString != NULL);

	// do checks that we have the right reference string instance; we must allow for 
	// equality of two empty strings to be considered to evaluate to TRUE
	if (str.IsEmpty() && pRefString->m_translation.IsEmpty())
		goto a;
	if (str != pRefString->m_translation)
	{
		// message can be in English, it's never likely to occur
		wxMessageBox(_T("Remove button error: Knowledge bases's adaptation text does not match that selected in the list box\n"),
			_T(""), wxICON_EXCLAMATION);
	}

	if (pRefString != (CRefString*)m_pListBoxExistingTranslations->GetClientData(nTransSel))
	{
		// message can be in English, it's never likely to occur
		wxMessageBox(_T("Remove button error: pRefString data pointer stored at list box selection does not match that at same location on the target unit\n"),
			_T(""), wxICON_EXCLAMATION);
	}

	// get the ref count, use it to warn user about how many previous references this will mess up
a:	nPreviousReferences = pRefString->m_refCount;

	// warn user about the consequences, allow him to get cold feet & back out
	// IDS_VERIFY_NEEDED
	message = message.Format(_("Removing: \"%s\", will make %d occurrences of it in the document files inconsistent with\nthe knowledge base. (You can fix that later by using the Consistency Check command.)\nDo you want to go ahead and remove it?"),
		str2.c_str(),nPreviousReferences);
	int nResult = wxMessageBox(message, _T(""), wxYES_NO | wxICON_WARNING);
	if (!(nResult == wxYES))
	{
		return; // user backed out
	}

	// Autocapitalization might be ON, and if so and if the user has clicked a translation
	// starting with an upper case character (put there earlier when auto caps was OFF), then
	// the call below to AutoCapsLookup( ) would, if not prevented from doing so, change the
	// upper case to lower case at the start of the key string, and then the lookup could find
	// the wrong entry, or find no entry. The former would leave the data in an invalid state,
	// and eventually lead to a crash. The solution is to use a global flag to alert AutoCapsLookup
	// when the caller was the remove button (same applies in handler for Remove button in the
	// Choose Translation dialog), and use the flag to jump the capitalization smarts and instead
	// just do a lookup on the key as supplied to the function.
	gbCallerIsRemoveButton = TRUE;

	// user hit the Yes button, so go ahead; remove the string from the list and then
	// make the first element in the list the new selection - provided there is one left
	m_pListBoxExistingTranslations->Delete(nTransSel);
	int count = m_pListBoxExistingTranslations->GetCount();
	wxASSERT(count >= 0);
	if (count > 0)
	{
		str = m_pListBoxExistingTranslations->GetString(0);
		int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,str,caseSensitive,exactString);
		wxASSERT(nNewSel != -1);
		pCurRefString = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nNewSel);
		m_refCount = pCurRefString->m_refCount;
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
		m_edTransStr = m_pListBoxExistingTranslations->GetString(nNewSel);
		m_pListBoxExistingTranslations->SetSelection(nNewSel);
	}
	else
	{
		// list is now empty
		pCurRefString = NULL;
		m_refCount = 0;
		m_refCountStr = _T("0");
		m_edTransStr = _T("");
		m_pListBoxExistingTranslations->Clear();
	}

	// remove the corresponding CRefString instance from the knowledge base
	wxASSERT(pRefString != NULL);
	delete pRefString;
	pCurTgtUnit->m_pTranslations->DeleteNode(pos);

	// we'll also need the index of the source word/phrase selection. If it turns out that
	// we delete all of a source word/phrase's translations, and need to also delete the 
	// source word from its list box we have its selection; if there are translation left
	// after deleting the current one, nNewKeySel should remain at the same selection.
	int nNewKeySel = m_pListBoxKeys->GetSelection();
	// are we deleting the last item in the translations list box? If so, we have to delete the
	// targetUnit as well (can't have one with no refStrings stored in it), and get rid of its
	// key string in the m_listBoxKeys list box, etc.
	if (count == 0)
	{
		wxASSERT(!m_curKey.IsEmpty());
		// whm note: I've revised the following which is parallel to the code in ChooseTranslation
		wxString s1 = m_curKey;
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
			nRemoved = pMap->erase(s1); // also remove it from the map
		}
		if (nRemoved == 0)
		{
			// have a second shot using the unmodified string curKey
			nRemoved = pMap->erase(m_curKey);// also remove it from the map
		}
		wxASSERT(nRemoved == 1);
		TUList::Node* pos = pKB->m_pTargetUnits->Find(pCurTgtUnit);
		wxASSERT(pos != NULL);
		pKB->m_pTargetUnits->DeleteNode(pos);
		delete pCurTgtUnit; // ensure no memory leak
		pCurTgtUnit = (CTargetUnit*)NULL;

		// now fix up the m_listBoxKeys entry, and update the rest of the dialog (we'll have to 
		// make the default selected key be the first, so everything will change) 
		// MFC Note: 
		// FindStringExact does only a caseless find, so it could find the wrong entry if two 
		// entries differ only by case, so we must also do a CSrtring compare, which is case 
		// sensitive, to ensure we get the right entry. We also start with index = -1, 
		// so we search from the list start.
		// 
		// wx note: FindString also only does a caseless find, and we don't have a second
		// parameter in FindString to allow for a search from a certain position, so we'll
		// use our own FindListBoxItem using a case sensitive search.
		
		// get the selected item from the Source Phrase list, as we need to delete it since
		// all of its translations have been removed
		wxString spStr = m_pListBoxKeys->GetStringSelection();
		nNewKeySel = gpApp->FindListBoxItem(m_pListBoxKeys, spStr, caseSensitive, exactString);
		
		// normally we expect that we would exit the above for loop with the exact string
		wxASSERT(nNewKeySel != -1);
		// now delete the found item from the listbox
		int keysCount;
		keysCount = m_pListBoxKeys->GetCount();
		wxASSERT(nNewKeySel < keysCount);
		m_pListBoxKeys->Delete(nNewKeySel);
		// set nNewKeySel to be the previous item in listbox unless it already is the first item
		if (nNewKeySel > 0)
			nNewKeySel--;
	}
	LoadDataForPage(m_nCurPage,nNewKeySel);
	m_pTypeSourceBox->SetSelection(0,0); // sets selection to beginning of type source edit box following MFC
	m_pTypeSourceBox->SetFocus();

	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	gbCallerIsRemoveButton = FALSE; // reestablish the safe default
	UpdateButtons();
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
}

void CKBEditor::OnButtonMoveUp(wxCommandEvent& WXUNUSED(event)) 
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		wxMessageBox(_("Translations list box error when getting the current selection"),
																		_T(""), wxICON_EXCLAMATION);
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	wxMessageBox(_("Translations list box error when getting the current selection"),
	//																	_T(""), wxICON_EXCLAMATION);
	//	return;
	//}
	int nOldSel = nSel; // save old selection index

	// change the order of the string in the list box
	int count;
	count = pCurTgtUnit->m_pTranslations->GetCount();
	wxASSERT(nSel < count);
	if (nSel > 0)
	{
		nSel--;
		wxString tempStr;
		tempStr = m_pListBoxExistingTranslations->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,caseSensitive,exactString); // whm added
		wxASSERT(nLocation != -1); // LB_ERR;
		CRefString* pRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nLocation);
		m_pListBoxExistingTranslations->Delete(nLocation);
		m_pListBoxExistingTranslations->Insert(tempStr,nSel);
		// m_pListBoxExistingTranslations is not sorted
		nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,caseSensitive,exactString); // whm added
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
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->Item(nOldSel);
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pCurTgtUnit->m_pTranslations->DeleteNode(pos); 
		pos = pCurTgtUnit->m_pTranslations->Item(nSel);
		wxASSERT(pos != NULL);
		// Note: wxList::Insert places the item before the given item and the inserted item then
		// has the insertPos node position.
		TranslationsList::Node* newPos = pCurTgtUnit->m_pTranslations->Insert(pos,pRefString);
		if (newPos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T("Error: MoveUp button failed to reinsert the translation being moved\n"),
																			_T(""), wxICON_EXCLAMATION);
			wxASSERT(FALSE);
			//AfxAbort();
		}
	}

	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	UpdateButtons();
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
}

void CKBEditor::OnButtonMoveDown(wxCommandEvent& WXUNUSED(event)) 
{
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)m_pListBoxExistingTranslations))
	{
		wxMessageBox(_("Translations list box error when getting the current selection"), 
																			_T(""), wxICON_EXCLAMATION);
		return;
	}

	int nSel;
	nSel = m_pListBoxExistingTranslations->GetSelection();
	//if (nSel == -1) // LB_ERR
	//{
	//	wxMessageBox(_("Translations list box error when getting the current selection"), 
	//																		_T(""), wxICON_EXCLAMATION);
	//	return;
	//}
	int nOldSel = nSel; // save old selection index

	// change the order of the string in the list box
	int count = pCurTgtUnit->m_pTranslations->GetCount();
	wxASSERT(nSel < count);
	if (nSel < count-1)
	{
		nSel++;
		wxString tempStr;
		tempStr = m_pListBoxExistingTranslations->GetString(nOldSel);
		int nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,caseSensitive,exactString); // whm added
		wxASSERT(nLocation != -1); // LB_ERR;
		CRefString* pRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nLocation);
		m_pListBoxExistingTranslations->Delete(nLocation);
		m_pListBoxExistingTranslations->Insert(tempStr,nSel);
		// m_pListBoxExistingTranslations is not sorted
		nLocation = gpApp->FindListBoxItem(m_pListBoxExistingTranslations,tempStr,caseSensitive,exactString); // whm added
		wxASSERT(nLocation != -1); // LB_ERR;
		m_pListBoxExistingTranslations->SetSelection(nSel);
		m_pListBoxExistingTranslations->SetClientData(nLocation,pRefStr);
		pRefStr = (CRefString*)m_pListBoxExistingTranslations->GetClientData(nLocation);
		m_refCount = pRefStr->m_refCount;
		m_refCountStr.Empty();
		m_refCountStr << m_refCount;
	}
	else
		return; // impossible to move the list element of the list further down!

	// now change the order of the CRefString in pCurTargetUnit to match the new order
	CRefString* pRefString;
	if (nSel > nOldSel)
	{
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->Item(nOldSel);
		wxASSERT(pos != NULL);
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pCurTgtUnit->m_pTranslations->DeleteNode(pos);
		pos = pCurTgtUnit->m_pTranslations->Item(nOldSel);
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
			pCurTgtUnit->m_pTranslations->Append(pRefString);
		else
			pCurTgtUnit->m_pTranslations->Insert(posNextHigher,pRefString);
		newPos = pCurTgtUnit->m_pTranslations->Find(pRefString);
		if (newPos == NULL)
		{
			// a rough & ready error message, unlikely to ever be called
			wxMessageBox(_T(
				"Error: MoveDown button failed to reinsert the translation being moved"),
				_T(""), wxICON_EXCLAMATION);
			wxASSERT(FALSE);
			//AfxAbort();
		}
	}

	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);
	UpdateButtons();
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
}

void CKBEditor::OnButtonFlagToggle(wxCommandEvent& WXUNUSED(event)) 
{
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
	gpApp->GetDocument()->Modify(TRUE); // whm added addition should make save button enabled
}

void CKBEditor::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	int pageNumSelected;
	// if user selected a source phrase before calling the KBEditor, start by initializing
	// the tab page having the correct number of words
	if (gnWordsInPhrase != -1)
	{
		pageNumSelected = gnWordsInPhrase-1;
	}
	// whm added 13Jan09. If no selection has been made, next best thing is to look up the source
	// phrase at the current active location of the phrasebox.
	else if (gpApp->m_pActivePile != NULL)
	{
		int nWords;
		nWords = gpApp->m_pActivePile->m_pSrcPhrase->m_nSrcWords;
		if (nWords != -1)
			pageNumSelected = nWords-1;
		else
			pageNumSelected = m_pKBEditorNotebook->GetSelection();
	}
	else
	{
		pageNumSelected = m_pKBEditorNotebook->GetSelection();
	}
	if (pageNumSelected != -1)
	{
		m_nWords = pageNumSelected + 1;
		LoadDataForPage(pageNumSelected,0);	
		// In LoadDataForPage above, the parameters may be changed in
		// LoadDataForPage if gnWordsInPhrase and gTheSelectedKey have content.
	}
}


bool CKBEditor::AddRefString(CTargetUnit* pTargetUnit, wxString& translationStr)
// remember, translationStr could legally be empty
{
	// check for a matching CRefString, if there is no match,
	// then add a new one, otherwise warn user of the match & return
	wxASSERT(pTargetUnit != NULL); 
	bool bMatched = FALSE;
	CRefString* pRefString = new CRefString(pTargetUnit);
	pRefString->m_refCount = 1; // set the count, assuming this will be stored (it may not be)
	pRefString->m_translation = translationStr; // set its translation string

	TranslationsList::Node* pos = pTargetUnit->m_pTranslations->GetFirst();
	while (pos != NULL)
	{
		CRefString* pRefStr = (CRefString*)pos->GetData();
		pos = pos->GetNext();
		wxASSERT(pRefStr != NULL);

		// wx TODO: Compare this to changes made in ChooseTranslation dialog
		// does it match?
		if ((pRefStr->m_translation.IsEmpty() && translationStr.IsEmpty()) ||	
			(*pRefStr == *pRefString))
		{
			// we got a match, or both are empty, so warn user its a copy of an existing 
			// translation and return
			bMatched = TRUE;
			// IDS_TRANSLATION_ALREADY_EXISTS
			wxMessageBox(_("Sorry, the translation you are attempting to associate with the current source phrase already exists."),_T(""),wxICON_INFORMATION);
			delete pRefString; // don't need this one
			return FALSE;
		}
	}
	// if we get here with bMatched == FALSE, then there was no match, so we must add
	// the new pRefString to the targetUnit
	if (!bMatched)
	{
		pTargetUnit->m_pTranslations->Append(pRefString);
		pCurRefString = pRefString;
	}
	return TRUE;
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CKBEditor::OnOK(wxCommandEvent& event) // unused in CKBEditor
{
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

void CKBEditor::LoadDataForPage(int pageNumSel,int nStartingSelection)
{
	// In the wx vesion wxDesigner has created identical sets of controls for each
	// page (nbPage) in our wxNotebook. We need to associate the pointers with the correct
	// controls here within LoadDataForPage() which is called initially and for each
	// tab page selected. The pointers to controls will differ for each page, hence
	// they need to be reassociated for each page. Note the nbPage-> pefix on 
	// FindWindowById(). I chosen not to use Validators here because of the complications
	// of having pointers to controls differ on each page of the notebook; instead we
	// manually transfer data between dialog controls and their variables.
	//wxColour sysColorBtnFace = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	
	// set the page in wxNotebook
	m_pKBEditorNotebook->SetSelection(pageNumSel);
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

	m_pEditOrAddTranslationBox = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_EDITORADD);
	wxASSERT(m_pEditOrAddTranslationBox != NULL);

	m_pEditRefCount = (wxTextCtrl*)nbPage->FindWindow(IDC_EDIT_REF_COUNT);
	wxASSERT(m_pEditRefCount != NULL);
	m_pEditRefCount->SetBackgroundColour(gpApp->sysColorBtnFace); // read only background color

	m_pStaticCount = (wxStaticText*)nbPage->FindWindow(IDC_STATIC_COUNT);
	wxASSERT(m_pStaticCount != NULL);

	m_pListBoxExistingTranslations = (wxListBox*)nbPage->FindWindow(IDC_LIST_EXISTING_TRANSLATIONS);
	wxASSERT(m_pListBoxExistingTranslations != NULL);

	m_pListBoxKeys = (wxListBox*)nbPage->FindWindow(IDC_LIST_SRC_KEYS);
	wxASSERT(m_pListBoxKeys != NULL);

	// below are pointers to the buttons on this nbPage page
	m_pBtnUpdate = (wxButton*)nbPage->FindWindow(IDC_BUTTON_UPDATE);
	wxASSERT(m_pBtnUpdate != NULL);
	m_pBtnAdd = (wxButton*)nbPage->FindWindow(IDC_BUTTON_ADD);
	wxASSERT(m_pBtnAdd != NULL);
	m_pBtnRemove = (wxButton*)nbPage->FindWindow(IDC_BUTTON_REMOVE);
	wxASSERT(m_pBtnRemove != NULL);
	m_pBtnMoveUp = (wxButton*)nbPage->FindWindow(IDC_BUTTON_MOVE_UP);
	wxASSERT(m_pBtnMoveUp != NULL);
	m_pBtnMoveDown = (wxButton*)nbPage->FindWindow(IDC_BUTTON_MOVE_DOWN);
	wxASSERT(m_pBtnMoveDown != NULL);
	m_pBtnAddNoAdaptation = (wxButton*)nbPage->FindWindow(IDC_ADD_NOTHING);
	wxASSERT(m_pBtnAddNoAdaptation != NULL);
	m_pBtnToggleTheSetting = (wxButton*)nbPage->FindWindow(IDC_BUTTON_FLAG_TOGGLE);
	wxASSERT(m_pBtnToggleTheSetting != NULL);

	// most of this was originally in MFC's InitDialog
	wxString s;
	s = _("<no adaptation>"); //IDS_NO_ADAPTATION // that is, "<no adaptation>" 

	wxString srcKeyStr;
	
	wxASSERT(m_nWords > 0); // set when initializing the CPropertySheet wrapper
	pMap = pKB->m_pMap[m_nWords-1]; 
	wxASSERT(pMap != NULL);
	m_ON = _("ON"); //IDS_ON // for localization, use string table
	m_OFF = _("OFF"); //IDS_OFF  // ditto

	// Set fonts and directionality for controls in the dialog
	#ifdef _RTL_FLAGS
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pTypeSourceBox, NULL,
								m_pListBoxKeys, NULL, gpApp->m_pDlgSrcFont, gpApp->m_bSrcRTL);
	#else 
	gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pSourceFont, m_pTypeSourceBox, NULL, 
								m_pListBoxKeys, NULL, gpApp->m_pDlgSrcFont);
	#endif

	if (gbIsGlossing && gbGlossingUsesNavFont)
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditOrAddTranslationBox, NULL,
									m_pListBoxExistingTranslations, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bNavTextRTL);
		#else 
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pNavTextFont, m_pEditOrAddTranslationBox, NULL, 
									m_pListBoxExistingTranslations, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}
	else
	{
		#ifdef _RTL_FLAGS
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditOrAddTranslationBox, NULL,
									m_pListBoxExistingTranslations, NULL, gpApp->m_pDlgTgtFont, gpApp->m_bTgtRTL);
		#else 
		gpApp->SetFontAndDirectionalityForDialogControl(gpApp->m_pTargetFont, m_pEditOrAddTranslationBox, NULL, 
									m_pListBoxExistingTranslations, NULL, gpApp->m_pDlgTgtFont);
		#endif
	}
//#ifdef __WXGTK__
//	m_bListBoxBeingCleared = TRUE;	// a work around so OnSelchangedListSrcKeys will know that we
//									// are calling Clear() below (on wxGTK OnSechangedListSrcKeys is
//									// triggered by the call to Clear(), but in OnSelchangedListSrcKeys
//									// m_pListBoxKeys->GetCount() still returns the count before clearing
//									// the listbox
//#endif

	// start with an empty src keys listbox
	m_pListBoxKeys->Clear(); // whm added
	// start with an empty translations list box
	m_pListBoxExistingTranslations->Clear();

	// get the list of source keys filled
	if (pMap->size() > 0)
	{
		MapKeyStringToTgtUnit::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter)
		{
			srcKeyStr = iter->first;
			pCurTgtUnit = iter->second;
			wxASSERT(pCurTgtUnit != NULL);

			// in case the data is corrupted, (eg. a pCurTgtUnit with an empty refString list)
			// we will check and ignore any such, and also remove them so they cannot make trouble
			// later
			if (pCurTgtUnit->m_pTranslations->IsEmpty())
			{
				pMap->erase(srcKeyStr); // the map now lacks this invalid association
				TUList::Node* pos = pKB->m_pTargetUnits->Find(pCurTgtUnit);
				wxASSERT(pos != NULL);
				pKB->m_pTargetUnits->DeleteNode(pos); // its CTargetUnit ptr is now gone from list
				delete pCurTgtUnit; // and its memory chunk is freed
				continue;
			}
			int index;
			index = m_pListBoxKeys->Append(srcKeyStr,pCurTgtUnit); // whm modified 10Nov08 to use 2nd param
			// In wx the index returned from wxListBox::Append on a sorted list cannot be used reliably
			// for SetClientData; instead we must find the exact string's index in the sorted list
			// whm update 10Nov08: In this case we can use the Append function that takes the second
			// parameter for the client data. For KBs with hundrends of key string items, using
			// FindListBoxItem takes too long and causes a considerable amount of time to populate the
			// m_pListBoxKeys list box.
			//int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, srcKeyStr, caseSensitive, exactString);
			//wxASSERT(nNewSel != -1); // we just added it so it must be there!
			//m_pListBoxKeys->SetClientData(nNewSel,pCurTgtUnit);
		}
	}
	else
	{
		pCurTgtUnit = 0; // no valid pointer set
	}

	// select the first string in the keys listbox by default, if possible; but if the globals
	// gnWordsInPhrase and gTheSelectedKey have content, then try to make the selection default
	// to gnTheSelectedKey instead, for rapid access to the desired entry.
	m_curKey = _T("");
	int nCount = m_pListBoxKeys->GetCount();
	if (nCount > 0)
	{
		// check out if we have a desired key to be accessed first
		if (gnWordsInPhrase > 0 && !gTheSelectedKey.IsEmpty())
		{
			// user wants a certain key selected on entry to the editor, try set it up for him
			// If successful this will change the initial selection passed in the 3rd parameter
			//
			// whm revision for wx: We'll do a case insensitive find of the desired word or
			// phrase since that would be better than failing because of an entry existing but
			// not being found because of a mere case difference.
			// Note: The View's OnToolsKBEditor() sets the tab page num before ShowModal() is
			// called.

			//nLastSel = ; // setting nLastSel to gnWordsInPhrase is an error!
			m_srcKeyStr = gTheSelectedKey;
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, m_srcKeyStr, caseInsensitive, subString);
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
		// whm added 13Jan09. If no selection has been made, next best thing is to look up the source
		// phrase at the current active location of the phrasebox.
		else if (gpApp->m_pActivePile != NULL)
		{
			// get the key for the source phrase at the active location
			wxString srcKey;
			srcKey = gpApp->m_pActivePile->m_pSrcPhrase->m_key;
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys, srcKey, caseInsensitive, subString);
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
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxKeys,str,caseSensitive,exactString);
			wxASSERT(nNewSel != -1);
			pCurTgtUnit = (CTargetUnit*)m_pListBoxKeys->GetClientData(nNewSel);
		}
		m_curKey = m_pListBoxKeys->GetStringSelection(); // set m_curKey

		// we're not using validators here so fill the textbox, but only if m_srcKeyStr
		// is different from the current contents (avoids a spurious system beep)
		// Also, instead of calling SetValue() we use ChangeValue() here to avoid spurious calling
		// of OnSelChangeListSrcKeys
		if (m_pTypeSourceBox->GetValue() != m_srcKeyStr)
			m_pTypeSourceBox->ChangeValue(m_srcKeyStr);

		// clear the globals, ready for reuse
		gnWordsInPhrase = -1;
		gTheSelectedKey.Empty();

		// the above could fail, eg. if nothing is in the list box, in which case -1 will be 
		//  put in the pCurTgtUnit variable, so change it to a zero pointer
		if (pCurTgtUnit <= 0)
		{
			pCurTgtUnit = 0; // invalid pointer
			m_flagSetting = m_OFF;
			wxMessageBox(_T("Warning: Invalid pointer to current target unit returned."),_T(""), wxICON_WARNING);
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
	nCount = m_pListBoxKeys->GetCount();
	//IDS_ENTRY_COUNT
	m_entryCountStr = m_pStaticCount->GetLabel();
	m_entryCountStr = m_entryCountStr.Format(m_entryCountStr,nCount);
	m_pStaticCount->SetLabel(m_entryCountStr);

	// fill the translations listbox for the pCurTgtUnit selected
	if (pCurTgtUnit != NULL)
	{
		TranslationsList::Node* pos = pCurTgtUnit->m_pTranslations->GetFirst();
		wxASSERT(pos != NULL);
		while (pos != NULL)
		{
			pCurRefString = (CRefString*)pos->GetData();
			pos = pos->GetNext();
			wxString str = pCurRefString->m_translation;
			if (str.IsEmpty())
			{
				str = s;
			}
			m_pListBoxExistingTranslations->Append(str);
			// m_pListBoxExistingTranslations is NOT a sorted list but it is safest to use FindListBoxItem before
			// we call SetClientData()
			int nNewSel = gpApp->FindListBoxItem(m_pListBoxExistingTranslations, str, caseSensitive, exactString);
			wxASSERT(nNewSel != -1); // we just added it so it must be there!
			m_pListBoxExistingTranslations->SetClientData(nNewSel,pCurRefString);
		}

		// select the first translation in the listbox by default -- there will always be at least one
		wxASSERT(m_pListBoxExistingTranslations->GetCount() > 0);
		m_pListBoxExistingTranslations->SetSelection(0);
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
			wxMessageBox(_T("A value of zero for the reference count means there was an error."),_T(""),wxICON_WARNING);
		}

		// get the selected translation text
		m_edTransStr = m_pListBoxExistingTranslations->GetString(0);
	}

	m_pTypeSourceBox->SetSelection(-1,-1); //(0,0); // MFC intends to select the whole contents
	m_pTypeSourceBox->SetFocus();
		
	m_pEditRefCount->SetValue(m_refCountStr);
	m_pEditOrAddTranslationBox->SetValue(m_edTransStr);// whm added
	UpdateButtons();
}
 
void CKBEditor::UpdateButtons()
{
	// enables or disables buttons to reflect valid choices depending on number of
	// items in list and which are selected

	// disable Update, Add, MoveUp, MoveDown, and Toggle buttons initially (may be enabled below)
	m_pBtnUpdate->Enable(FALSE);
	m_pBtnAdd->Enable(FALSE);
	m_pBtnMoveUp->Enable(FALSE);
	m_pBtnMoveDown->Enable(FALSE);
	// The "Toggle The Setting" should be enabled except for when the translations list is empty
	// as when displaying a tab without any data
	if (m_pListBoxExistingTranslations->GetCount() == 0)
		m_pBtnToggleTheSetting->Enable(FALSE);
	else
		m_pBtnToggleTheSetting->Enable(TRUE);
	// enable the Add <no adaptation> button initially
	m_pBtnAddNoAdaptation->Enable(TRUE);
	// since there should be at least 1 translation in the existing translations box
	// for every source phrase, the Remove button should always be enabled. Whenever the only
	// translation of a given source phrase is removed, the source phrase itself is also removed.
	// The one time the "Remove" button should be disabled is when the list is empty which occurs
	// on tab pages having no items.
	if (m_pListBoxExistingTranslations->GetCount() == 0)
		m_pBtnRemove->Enable(FALSE);
	else
		m_pBtnRemove->Enable(TRUE);
	// enable those buttons meeting certain conditions depending on number of items and selection
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
		m_pBtnMoveUp->Enable(TRUE);
	}
	if (m_pListBoxExistingTranslations->GetCount() > 1 && nSel < (int)m_pListBoxExistingTranslations->GetCount()-1)
	{
		// The Move Down button is enabled whenever there are more than 1 translations in the list
		// and the selected item is not the first one in the list
		m_pBtnMoveDown->Enable(TRUE);
	}
	if (m_pEditOrAddTranslationBox->IsModified() && m_pEditOrAddTranslationBox->GetValue() != m_pListBoxExistingTranslations->GetStringSelection())
	{
		// The Update button is enabled whenever the edit translation text box is dirty
		m_pBtnUpdate->Enable(TRUE);
	}
	wxString stringBeingTyped;
	stringBeingTyped = m_pEditOrAddTranslationBox->GetValue();
	if (m_pEditOrAddTranslationBox->IsModified() &&  !IsInListBox(m_pListBoxExistingTranslations, stringBeingTyped))
	{
		// The Add button is enabled whenever the edit translation text box is dirty, and
		// the current string in the box is not already in the m_pListBoxExistingTranslations list.
		m_pBtnAdd->Enable(TRUE);
	}
	if (m_pListBoxExistingTranslations->GetCount() == 0 || IsInListBox(m_pListBoxExistingTranslations, _("<no adaptation>")))
	{
		// the AddNoAdaptation button is always enabled unless "<no adaptation>" is in the
		// m_pListBoxExistingTranslations list
		m_pBtnAddNoAdaptation->Enable(FALSE);
	}
}

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
