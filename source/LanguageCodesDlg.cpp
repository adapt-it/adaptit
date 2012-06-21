/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			LanguageCodesDlg.cpp
/// \author			Bill Martin
/// \date_created	5 May 2010
/// \date_revised	5 May 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CLanguageCodesDlg class. 
/// The CLanguageCodesDlg class provides a dialog in which the user can enter
/// the ISO639-3 3-letter language codes for the source and target languages.
/// The dialog allows the user to search for the codes by language name.
/// \derivation		The CLanguageCodesDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in LanguageCodesDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "LanguageCodesDlg.h"
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

#include "Adapt_It.h"
#include "LanguageCodesDlg.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern LangInfo langsKnownToWX[];

// event handler table
BEGIN_EVENT_TABLE(CLanguageCodesDlg, AIModalDialog)
	EVT_INIT_DIALOG(CLanguageCodesDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CLanguageCodesDlg::OnOK)
	EVT_BUTTON(ID_BUTTON_FIND_CODE, CLanguageCodesDlg::OnFindCode)
	EVT_BUTTON(ID_BUTTON_FIND_LANGUAGE, CLanguageCodesDlg::OnFindLanguage)
	EVT_BUTTON(ID_BUTTON_USE_SEL_AS_SRC, CLanguageCodesDlg::OnUseSelectedCodeForSrcLanguage)
	EVT_BUTTON(ID_BUTTON_USE_SEL_AS_TGT, CLanguageCodesDlg::OnUseSelectedCodeForTgtLanguage)
	EVT_BUTTON(ID_BUTTON_USE_SEL_AS_GLS, CLanguageCodesDlg::OnUseSelectedCodeForGlsLanguage)
	EVT_LISTBOX(ID_LIST_LANGUAGE_CODES_NAMES, CLanguageCodesDlg::OnSelchangeListboxLanguageCodes)
	EVT_TEXT_ENTER(ID_TEXTCTRL_SEARCH_LANG_NAME, CLanguageCodesDlg::OnEnterInSearchBox)
END_EVENT_TABLE()

CLanguageCodesDlg::CLanguageCodesDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Choose the 3-letter language codes for Source and Target Languages"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	LanguageCodesDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning (retain this line as is)
	pListBox = (wxListBox*)FindWindowById(ID_LIST_LANGUAGE_CODES_NAMES);
	wxASSERT(pListBox != NULL);

	pEditSearchForLangName = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SEARCH_LANG_NAME);
	wxASSERT(pEditSearchForLangName != NULL);

	pEditSourceLangCode = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_SRC_LANG_CODE);
	wxASSERT(pEditSourceLangCode != NULL);

	pEditTargetLangCode = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_TGT_LANG_CODE);
	wxASSERT(pEditTargetLangCode != NULL);

	pEditGlossLangCode = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_GLS_LANG_CODE);
	wxASSERT(pEditGlossLangCode != NULL);

	pBtnFindCode = (wxButton*)FindWindowById(ID_BUTTON_FIND_CODE);
	wxASSERT(pBtnFindCode != NULL);

	pBtnFindLanguage = (wxButton*)FindWindowById(ID_BUTTON_FIND_LANGUAGE);
	wxASSERT(pBtnFindLanguage != NULL);

	pBtnUseSelectionAsSource = (wxButton*)FindWindowById(ID_BUTTON_USE_SEL_AS_SRC);
	wxASSERT(pBtnUseSelectionAsSource != NULL);

	pBtnUseSelectionAsTarget = (wxButton*)FindWindowById(ID_BUTTON_USE_SEL_AS_TGT);
	wxASSERT(pBtnUseSelectionAsTarget != NULL);

	pStaticScrollList = (wxStaticText*)FindWindowById(ID_STATICTEXT_SCROLL_LIST);
	wxASSERT(pStaticScrollList != NULL);

	pStaticSearchForLangName = (wxStaticText*)FindWindowById(ID_STATICTEXT_SEARCH_FOR_LANG_NAME);
	wxASSERT(pStaticSearchForLangName != NULL);
}

CLanguageCodesDlg::~CLanguageCodesDlg() // destructor
{
	
}

void CLanguageCodesDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	m_bISO639ListFileFound = TRUE;

	// Adapt It uses both the 2-letter iso639-1 codes and the 3-letter iso639-3 
	// language codes concatenated together in a single UTF-8 plain text file
	// called iso639-3codes.txt. There are 184 2-letter codes and they are placed
	// first in the iso639-3codes.txt file. The 2-letter codes were reformatted
	// from the table at: http://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
	// The 3-letter codes were downloaded from SIL's site at:
	// http://www.sil.org/iso639-3/download.asp#LNIndex
	// under the section called "Language Names Index" using the "Download ISO 639-3 
	// code set UTF-8" link.
	// 
	// Load the list box from the iso639-3codes.txt file located in the directory
	// where they were installed. This directory is different on each platform but
	// is the same directory where the xml install file AI_USFM.xml and books.xml
	// were placed. That directory is retrieved using the App's 
	// GetDefaultPathForXMLControlFiles() function.
	wxString iso639_3CodesFileName = _T("iso639-3codes.txt");
	wxString pathToLangCodesFile;
	pathToLangCodesFile = gpApp->GetDefaultPathForXMLControlFiles();
	pathToLangCodesFile += gpApp->PathSeparator;
	pathToLangCodesFile += iso639_3CodesFileName;
	pListBox->Clear();
	wxLogNull logNo; // avoid system messages about missing file
	wxFFile f(pathToLangCodesFile); // default mode is "r" or read
	if (!(f.IsOpened() && f.Length() > 0))
	{
		// tell user that the iso639-3codes.txt file cannot be found
		wxString msg = msg.Format(_("Cannot open the %s language codes file at the path:\n   %s\nAdapt It cannot show the list of language names and their codes,\nbut you can enter the 3-letter language codes manually."),iso639_3CodesFileName.c_str(),pathToLangCodesFile.c_str());
		wxMessageBox(msg, _T(""), wxICON_EXCLAMATION | wxOK);
		m_bISO639ListFileFound = FALSE;
	}

	if (!m_bISO639ListFileFound)
	{
		wxString isoFileNotFound;
		isoFileNotFound = isoFileNotFound.Format(_("NO LIST AVAILABLE - %s File Not Found"),iso639_3CodesFileName.c_str());
		pListBox->Append(isoFileNotFound);
		pListBox->Disable(); // disable the list box window
		// also disable other dialog controls that cannot be used when no list is available
		pEditSearchForLangName->Disable();
		pBtnFindCode->Disable();
		pBtnFindLanguage->Disable();
		pBtnUseSelectionAsSource->Disable();
		pBtnUseSelectionAsTarget->Disable();
		pStaticScrollList->Disable();
		pStaticSearchForLangName->Disable();
	}

	if (m_bISO639ListFileFound)
	{
		// read the language codes file
		wxString* pTempStr;
		pTempStr = new wxString;
		bool bSuccessfulRead;
		bSuccessfulRead = f.ReadAll(pTempStr); // ReadAll doesn't require the file's Length
		if(!bSuccessfulRead)
		{
			wxASSERT(FALSE);
			// file may have been truncated
			// TODO: issue warning to user
		}
		// language codes file was read into a wxString buffer so parse it line-by-line into the listbox
		const wxChar* pStr = pTempStr->GetData();
		wxChar* ptr = (wxChar*)pStr;
		// whm Note: regardless of the platform, when reading a text file from disk into a
		// wxString, wxWidgets converts the line endings to a single \n char, hence we can't
		// use the fLen value from above to assign pEnd, but we have to get the new Length()
		// of the resulting string.
		int sLen = pTempStr->Length();
		wxChar* pEnd = ptr+sLen;
		wxString tempLine = _T("");
		const wxString tab5sp = _T("     ");
		int tabCount = 0;
		while (ptr < pEnd && *ptr != '\0')
		{
			if (*ptr == _T('\n') || *ptr == _T('\r'))
			{
				if (!tempLine.IsEmpty())
				{
					pListBox->Append(tempLine);
					tempLine.Empty();
				}
			}
			else if (*ptr == _T('\t'))
			{
				tabCount++;
				if (tabCount == 2)
				{
					// skip chars from second tab to end of line
					// this removes the inverted form of the line item
					while(*ptr != _T('\n') && ptr < pEnd) 
					{
						ptr++;
					}
					if (*ptr == _T('\n'))
						ptr--; // back up one char - the ptr++ at end of while loop will point ptr back at the \n
					tabCount = 0;
				}
				else
				{
					tempLine += tab5sp; // convert tab to 5 spaces in listbox item string
				}
			}
			else
			{
				// add chars other than newlines and tabs to tempLine
				tempLine += *ptr;
			}
			ptr++;
		}
		// catch the last list item if the codes file does not end with a newline char
		if (!tempLine.IsEmpty())
		{
			pListBox->Append(tempLine);
		}
		if (pTempStr != NULL) // whm 11Jun12 added NULL test
			delete pTempStr;
		// remove the first line of the listbox which should contain
		// "Id <tab> Print_Name"
		if (pListBox->GetCount() > 0)
		{
			wxString firstItemStr = pListBox->GetString(0);
			firstItemStr.Trim(FALSE); // trim left end
			firstItemStr.LowerCase();
			if (firstItemStr.Find(_T("id")) == 0)
			{
				pListBox->Delete(0); // remove the Id ... line from the listbox
			}
		}
		m_curSel = 0;
		if (pListBox->GetCount() > 0)
			pListBox->SetSelection(m_curSel,TRUE);
		// if the user had previously designated a source language code and/or a
		// target language code, and/or a gloss language code, enter those into 
		// the appropriate edit boxes as initial/default values
		if (!m_sourceLangCode.IsEmpty())
			pEditSourceLangCode->ChangeValue(m_sourceLangCode);
		if (!m_targetLangCode.IsEmpty())
			pEditTargetLangCode->ChangeValue(m_targetLangCode);
		if (!m_glossLangCode.IsEmpty())
			pEditGlossLangCode->ChangeValue(m_glossLangCode);
	}
}

// event handling functions
	
// whm revised 5Dec11 changed name of handler and modified to only search within the
// code part of the list strings (up to the 5 spaces). This routine does a brute 
// force linear search through the list. On a fast machine if the search
// string is near the end of the list it can take 7 or 8 seconds, longer on a slower
// machine, but this function is likely to be used only rarely when the code for
// a new language is being determined.
void CLanguageCodesDlg::OnFindCode(wxCommandEvent& WXUNUSED(event))
{
	unsigned int count = pListBox->GetCount();
	// get the text in the edit control
	m_searchString = pEditSearchForLangName->GetValue();
	if (m_searchString.IsEmpty() || count < 2)
	{
		::wxBell();
		return;
	}

	m_searchString.LowerCase();

	// for an ordinary search start at the first list position
	// m_curSel is set to 0 in InitDialog()
	int nCurSel = m_curSel;
	nCurSel++; // start search with following item
	unsigned int index;
	bool bFound = FALSE;
	wxString strLabel = _T("");
	// do a little benchmark test of search times in __WXDEBUG__
#ifdef __WXDEBUG__
	wxDateTime dt1 = wxDateTime::Now(),
			   dt2 = wxDateTime::UNow();
#endif
	for (index = nCurSel; index < count; index++)
	{
		//wxCursor(wxCURSOR_WAIT);
		// get the list's label string at index & check for a match; return with the
		// matched item selected; but if not matched, continue to iterate thru the list
		strLabel = pListBox->GetString(index);
		strLabel.LowerCase();
		// whm modified 5Dec11 to only search for codes in the first column (before the 5 spaces)
		const wxString tab5sp = _T("     "); // the tab was replaced by 5 spaces in InitDialog()
		int offset = strLabel.Find(tab5sp);
		wxASSERT(offset != wxNOT_FOUND); // there should be 5 spaces in the string
		strLabel = strLabel.Mid(0,offset); // get substring up to but not including the 5 spaces
		offset = strLabel.Find(m_searchString);
		if (offset == wxNOT_FOUND)
			continue;
		else
		{
			pListBox->SetSelection(index,TRUE);
			m_curSel = index;
			bFound = TRUE;
			break;
		}
	} // end of search loop
	//wxCursor(wxNullCursor);
	if (!bFound)
		::wxBell();

#ifdef __WXDEBUG__
		dt1 = dt2;
		dt2 = wxDateTime::UNow();
		wxLogDebug(_T("Find Code executed in %s ms"), 
			(dt2 - dt1).Format(_T("%l")).c_str());
#endif
}

// whm revised 5Dec11 changed name of handler and modified to only search within the
// language part of the list strings (following the 5 spaces). This routine does
// a brute force linear search through the list. On a fast machine if the search
// string is near the end of the list it can take 7 or 8 seconds, longer on a slower
// machine, but this function is likely to be used only rarely when the code for
// a new language is being determined.
void CLanguageCodesDlg::OnFindLanguage(wxCommandEvent& WXUNUSED(event))
{
	unsigned int count = pListBox->GetCount();
	// get the text in the edit control
	m_searchString = pEditSearchForLangName->GetValue();
	if (m_searchString.IsEmpty() || count < 2)
	{
		::wxBell();
		return;
	}

	m_searchString.LowerCase();

	// for an ordinary search start at the first list position
	// m_curSel is set to 0 in InitDialog()
	int nCurSel = m_curSel;
	nCurSel++; // start search with following item
	unsigned int index;
	bool bFound = FALSE;
	wxString strLabel = _T("");
	// do a little benchmark test of search times in __WXDEBUG__
#ifdef __WXDEBUG__
	wxDateTime dt1 = wxDateTime::Now(),
			   dt2 = wxDateTime::UNow();
#endif
	for (index = nCurSel; index < count; index++)
	{
		//wxCursor(wxCURSOR_WAIT);
		// get the list's label string at index & check for a match; return with the
		// matched item selected; but if not matched, continue to iterate thru the list
		strLabel = pListBox->GetString(index);
		strLabel.LowerCase();
		// whm modified 5Dec11 to only search for codes in the first column (before the 5 spaces)
		const wxString tab5sp = _T("     "); // the tab was replaced by 5 spaces in InitDialog()
		int offset = strLabel.Find(tab5sp);
		wxASSERT(offset != wxNOT_FOUND); // there should be 5 spaces in the string
		strLabel = strLabel.Mid(offset+tab5sp.Length()); // get substring up to but not including the tab
		offset = strLabel.Find(m_searchString);
		if (offset == wxNOT_FOUND)
			continue;
		else
		{
			pListBox->SetSelection(index,TRUE);
			m_curSel = index;
			bFound = TRUE;
			break;
		}
	} // end of search loop
	//wxCursor(wxNullCursor);
	if (!bFound)
		::wxBell();

#ifdef __WXDEBUG__
		dt1 = dt2;
		dt2 = wxDateTime::UNow();
		wxLogDebug(_T("Find Language executed in %s ms"), 
			(dt2 - dt1).Format(_T("%l")).c_str());
#endif
}


void CLanguageCodesDlg::OnEnterInSearchBox(wxCommandEvent& WXUNUSED(event))
{
	unsigned int count = pListBox->GetCount();
	// get the text in the edit control
	m_searchString = pEditSearchForLangName->GetValue();
	if (m_searchString.IsEmpty() || pListBox->GetCount() < 2)
	{
		::wxBell();
		return;
	}

	m_searchString.LowerCase();

	// for an ordinary search start at the first list position
	int nCurSel = 0;
	unsigned int index;
	bool bFound = FALSE;
	wxString strLabel = _T("");

	// do a little benchmark test of search times in __WXDEBUG__
#ifdef __WXDEBUG__
	wxDateTime dt1 = wxDateTime::Now(),
			   dt2 = wxDateTime::UNow();
#endif
	for (index = nCurSel; index < count; index++)
	{
		//wxCursor(wxCURSOR_WAIT);
		// get the list's label string at index & check for a match; return with the
		// matched item selected; but if not matched, continue to iterate thru the list
		strLabel = pListBox->GetString(index);
		strLabel.LowerCase();
		int offset = strLabel.Find(m_searchString);
		if (offset == wxNOT_FOUND)
			continue;
		else
		{
			pListBox->SetSelection(index,TRUE);
			m_curSel = index;
			bFound = TRUE;
			break;
		}
	} // end of search loop
	//wxCursor(wxNullCursor);
	if (!bFound)
		::wxBell();

#ifdef __WXDEBUG__
		dt1 = dt2;
		dt2 = wxDateTime::UNow();
		wxLogDebug(_T("Search executed in %s ms"), 
			(dt2 - dt1).Format(_T("%l")).c_str());
#endif

}

void CLanguageCodesDlg::OnSelchangeListboxLanguageCodes(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (pListBox->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
		return;

	m_curSel = pListBox->GetSelection();
	if (m_curSel != wxNOT_FOUND)
	{
		// enable appropriate buttons
		pBtnUseSelectionAsSource->Enable();
		pBtnUseSelectionAsTarget->Enable();
	}
}

void CLanguageCodesDlg::OnUseSelectedCodeForSrcLanguage(wxCommandEvent& WXUNUSED(event))
{
	pEditSourceLangCode->ChangeValue(Get3LetterCodeFromLBItem());	
}

void CLanguageCodesDlg::OnUseSelectedCodeForTgtLanguage(wxCommandEvent& WXUNUSED(event))
{
	pEditTargetLangCode->ChangeValue(Get3LetterCodeFromLBItem());	
}

void CLanguageCodesDlg::OnUseSelectedCodeForGlsLanguage(wxCommandEvent& WXUNUSED(event))
{
	pEditGlossLangCode->ChangeValue(Get3LetterCodeFromLBItem());	
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CLanguageCodesDlg::OnOK(wxCommandEvent& event) 
{
	m_sourceLangCode = pEditSourceLangCode->GetValue();
	m_targetLangCode = pEditTargetLangCode->GetValue();
	m_glossLangCode = pEditGlossLangCode->GetValue();
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

wxString CLanguageCodesDlg::Get3LetterCodeFromLBItem()
{
	m_curSel = pListBox->GetSelection();
	wxASSERT(m_curSel != wxNOT_FOUND);
	wxString tempStr = pListBox->GetString(m_curSel);
	return tempStr.Mid(0,3);
}

// other class methods

