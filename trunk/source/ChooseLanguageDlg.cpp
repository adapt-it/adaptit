/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseLanguageDlg.cpp
/// \author			Bill Martin
/// \date_created	15 July 2008
/// \date_revised	5 August 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CChooseLanguageDlg class. 
/// The CChooseLanguageDlg class provides a dialog in which the user can change
/// the interface language used by Adapt It. The dialog also allows the user to
/// browse to a different path (from that expected) to find the interface
/// language folders containing the <appName>.mo localization files.
/// \derivation		The CChooseLanguageDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ChooseLanguageDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ChooseLanguageDlg.h"
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
//#include <wx/valtext.h> // for wxTextValidator
#include <wx/dir.h>
//#include <wx/dirdlg.h>

#include "Adapt_It.h"
#include "ChooseLanguageDlg.h"
#include "BString.h"
#include "helpers.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

/// This global is defined in Adapt_It.cpp.
extern LangInfo langsKnownToWX[];

// event handler table
BEGIN_EVENT_TABLE(CChooseLanguageDlg, AIModalDialog)
	EVT_INIT_DIALOG(CChooseLanguageDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CChooseLanguageDlg::OnOK)
	//EVT_BUTTON(IDC_BTN_BROWSE_PATH, CChooseLanguageDlg::OnBrowseForPath) // whm removed 8Dec11
	EVT_LISTBOX(ID_LIST_UI_LANGUAGES, CChooseLanguageDlg::OnSelchangeListboxLanguages)
END_EVENT_TABLE()

CChooseLanguageDlg::CChooseLanguageDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Select your preferred language for Adapt It's menus and other messages"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ChooseLanguageDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);

	wxColor backgrndColor = this->GetBackgroundColour();
	
	pListBox = (wxListBox*)FindWindowById(ID_LIST_UI_LANGUAGES);
	wxASSERT(pListBox != NULL);

	pEditLocalizationPath = (wxTextCtrl*)FindWindowById(IDC_LOCALIZATION_PATH);
	wxASSERT(pEditLocalizationPath != NULL);
	pEditLocalizationPath->SetBackgroundColour(backgrndColor);

	//pBtnBrowse = (wxButton*)FindWindowById(IDC_BTN_BROWSE_PATH);
	//wxASSERT(pBtnBrowse != NULL);

	pEditAsStaticShortName = (wxTextCtrl*)FindWindowById(ID_TEXT_AS_STATIC_SHORT_LANG_NAME);
	wxASSERT(pEditAsStaticShortName != NULL);
	pEditAsStaticShortName->SetBackgroundColour(backgrndColor);

	pEditAsStaticLongName = (wxTextCtrl*)FindWindowById(ID_TEXT_AS_STATIC_LONG_LANG_NAME);
	wxASSERT(pEditAsStaticLongName != NULL);
	pEditAsStaticLongName->SetBackgroundColour(backgrndColor);
}

CChooseLanguageDlg::~CChooseLanguageDlg() // destructor
{
	
}

void CChooseLanguageDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// whm 8Dec11 modified. The localization folder path is determined on each run of the
	// application. Therefore it need not be restored from what is stored in the
	// currLocalizationInfo struct, nor ever restored from being saved in the Adapt_It_WX.ini
	// file. This is particularly true for the Linux version whose paths to certain auxiliary
	// files such as the localization files can vary depending on the install prefix (usually
	// "/usr/share/locale/<lang>/LC_MESSAGES/" or "/usr/local/share/locale/<lang>/LC_MESSAGES/")
	// depending on the value of m_PathPrefix.
	// Therefore I've modified the code below to always assign the current value returned by 
	// the App's GetDefaultPathForLocalizationSubDirectories() to pathToLocalizationFolders .
	// Note: This modification has also been done for 
	
	m_bChangeMade = FALSE;
	// If a localization folder path is stored in m_pConfig, and it is a valid path,
	// use it to display the list of language choices, otherwise use the standard 
	// default path.
	//if (!gpApp->currLocalizationInfo.curr_localizationPath.IsEmpty()
	//	&& ::wxDirExists(gpApp->currLocalizationInfo.curr_localizationPath))
	//{
	//	pathToLocalizationFolders = gpApp->currLocalizationInfo.curr_localizationPath;
	//}
	//else
	//{
	// Get the default path that should contain the localization subdirectories for this platform.
	//wxString pathToLocalizationFolders;
	pathToLocalizationFolders = gpApp->GetDefaultPathForLocalizationSubDirectories();
	//}
	// put localization path in its edit box
	pEditLocalizationPath->SetValue(pathToLocalizationFolders);
	
	// Determine if the data stored in m_pConfig indicates that a user_defined_language_n was last used
	// or not. If so, ensure that the language was added to the current m_pLocale.
	if (gpApp->currLocalizationInfo.curr_UI_Language > wxLANGUAGE_USER_DEFINED)
	{
		m_bCurrentLanguageUserDefined = TRUE;
		
		// has the language already been added to this locale?
		const wxLanguageInfo *info = wxLocale::GetLanguageInfo(gpApp->currLocalizationInfo.curr_UI_Language);
		if (!info)
		{
			wxLanguageInfo langInfo;
			// User gave a common language name, so we call AddLanguage() to add language to the
			// database of wxWidgets' known languages
			langInfo.Language = gpApp->currLocalizationInfo.curr_UI_Language;
			langInfo.CanonicalName = gpApp->currLocalizationInfo.curr_shortName;
#ifdef __WIN32__
			// Win32 language identifiers
			langInfo.WinLang = 0;			// We don't know the Windows LANG_xxxx value so enter zero
			langInfo.WinSublang = 0;		// We don't know the Windows SUBLANG_xxxx value so enter zero 
#endif
			langInfo.Description = gpApp->currLocalizationInfo.curr_fullName;
			// Note: There are no examples of AddLanguage in poEdit sources or Audacity sources
			gpApp->m_pLocale->AddLanguage(langInfo);
		}
	}
	else
	{
		m_bCurrentLanguageUserDefined = FALSE;
	}

	// Get all the possible installed localizations, i.e., there is no point in presenting
	// choices to the user that do not exist as part of the Adapt It installation on his 
	// computer. If pathToLocalizationFolders is empty, after executing the code below
	// subDirList will only have one item listed in it (the default system language).
	subDirList.Clear();
	pListBox->Clear();
	gpApp->GetListOfSubDirectories(pathToLocalizationFolders,subDirList);
	
    // Get the system language name. We will use it as the first item in the list - provided 
    // the system language is either English (or some form en_XX), or if it is the same as one 
    // of the existing Adapt It localizations on the local computer.
    // If the system language is neither English (or some form of it), nor one of the existing
    // Adapt It localizations, we will default to listing English as the first item in the list.
    // This check needs to be made every time the ChooseLanguageDlg is displayed (or forced to
    // be displayed), because of the possibility that a user may have done one of the following
    // since the last use of Adapt It:
    //    1. Deleted a localization from his/her computer that was previously in use
    //    2. Changed the registry/hidden settings file manually
    //    3. Changed the system's default language
	wxString fullDefaultLangNameListItem = langsKnownToWX[0].fullName;
		
	int sysLanguage = wxLocale::GetSystemLanguage();
	const wxLanguageInfo *infoSys = wxLocale::GetLanguageInfo(sysLanguage);		
	/*
		currLocalizationInfo.curr_UI_Language = sysLanguage;
		currLocalizationInfo.curr_shortName = info->CanonicalName;
		currLocalizationInfo.curr_fullName = info->Description;
		currLocalizationInfo.curr_localizationPath = GetDefaultPathForLocalizationSubDirectories();
	*/
	// Determine if the system default language has an existing localization
	wxString shortDefaultLangName;
	if (infoSys != NULL)
	{
		shortDefaultLangName = infoSys->CanonicalName; //langsKnownToWX[0].shortName;
	}
	else 
	{
		shortDefaultLangName.Empty();
	}
	//wxString fullDefaultLangName;
	//wxLanguage sysLangFullName = gpApp->GetLanguageFromDirStr(shortDefaultLangName,fullDefaultLangName);
	// If the system default language has an existing localization we'll put it first in the list.
	// If the system default's canonical name is some form of en_XX we put "English [en_XX]" first in the list, otherwise
	// we'll just put "English" as first language in the list.
	
	if (gpApp->PathHas_mo_LocalizationFile(pathToLocalizationFolders, shortDefaultLangName) && gpApp->m_languageInfo != NULL)
	{
		// the shortDefaultLangName has an Adapt It localization
		fullDefaultLangNameListItem = fullDefaultLangNameListItem.Format(fullDefaultLangNameListItem, gpApp->m_languageInfo->Description.c_str());
	}
	else if (shortDefaultLangName.Length() > 2 && shortDefaultLangName.Mid(0,3) == _T("en_"))
	{
		// the shortDefaultLangName is of the form en_XX
		fullDefaultLangNameListItem = _T("English");
		fullDefaultLangNameListItem += _T(" [")+shortDefaultLangName+_T("]");
	}
	else
	{
		// the shortDefaultLangName does not have a localization and is not some form of en_XX so we'll
		// just list "English" as first item in the list
		fullDefaultLangNameListItem = _T("English");
	}
		
	//wxLogNull logNo;	// eliminates any spurious messages from the system while reading read-only folders/files

	// Ignore subdirectories in subDirList that do not contain .mo file(s) within the
	// subdirectory. Add the localization subdirectories that do have <appName>.mo file(s) 
	// for those localizations which the host OS and/or C run time environment say are 
	// available. Note: After the "for" loop below we will insert the default language from 
	// the fullDefaultLangNameListItem variable above as first list item at the beginning 
	// of pListBox.
	wxString appName;
	appName = gpApp->GetAppName();
	wxString dirPath;
	int ct;
	// For each subDirList item (canonical name) add the full name of any language that has a
	// localization on the machine - saving its canonical name as ClientData in the pListBox item.
	for (ct = 0; ct < (int)subDirList.GetCount(); ct++)
	{
		dirPath = pathToLocalizationFolders + gpApp->PathSeparator + subDirList.Item(ct);
		if (wxDir::Exists(dirPath + gpApp->PathSeparator + _T("LC_MESSAGES")))
			dirPath = dirPath + gpApp->PathSeparator + _T("LC_MESSAGES");
		wxDir dPath(dirPath);
		wxLogDebug(_T("dirPath = %s appName = %s.mo"),dirPath.c_str(),appName.c_str());
		if (dPath.Open(dirPath) && dPath.HasFiles(_T("*.mo")) && dPath.HasFiles(appName + _T(".mo")))
		{
			wxLogDebug(_T("   HasFiles TRUE"));
			// The subdir has an <appName>.mo file, so see if we recognize the
			// language from the dirStr. If so we then determine if the language
			// is available according to our host OS and/or C run time environment.
			// If it is available we insert its full language name into the list.
			// If the OS and/or C run time can't identify the language (i.e., returns
			// wxLANGUAGE_UNKNOWN), we investigate the following possibilities in order:
			// 1. Snoop in the .mo binary file and look for a string directly following
			// "X-Poedit_Language: " character sequence. If specified, the language 
			// specified within the .mo file will be composed of all of the subsequent 
			// characters up to the next \n (0a) in the binary file.
			// 2. If 1 above fails to produce a valid string (which might be the case if the
			// .mo file was compiled by a program other than Poedit or the translator
			// forgot to enter the language name into the Poedit setup), we check to see if
			// we got a user defined language name already stored in the registry/hidden
			// settings file associated with this subdir name (it would be stored in one 
			// of the user_defined_language_n entries (where n is a numerical sequence 
			// from 0..8).
			// 3. If 1 and 2 above fail, we just label the entry " [Contains Unknown or 
			// New Localization]", which if chosen by the user, we query the user for
			// a language name in this case.
			wxLanguage lang;
			wxString fullLangName;
			lang = gpApp->GetLanguageFromDirStr(subDirList.Item(ct),fullLangName);
			if (lang != wxLANGUAGE_UNKNOWN)
			{
// Disabled the IsAvailable(lang) test below because it seems to return FALSE all the time on wxGTK
// The test is not really needed, because AddCatalog() will institute the interface localization
// even if IsAvailable() returns FALSE.
#if 0 //#if wxCHECK_VERSION(2, 7, 1) 
				// We recognize the language, check if it is available for a locale
				if (wxLocale::IsAvailable(lang)) // IsAvailable is new since wxWidgets version 2.7.1
				{
#endif
					// Save the dirStr from subDirList as listbox client data along with the list item.
					// We append the scanned subdir data into the list box. Then at the end (see below)
					// we will insert the default (English) as the first item in the list using the
					// wxListBox::Insert() method. 
					pListBox->Append(fullLangName,&subDirList.Item(ct)); //pListBox->Insert(fullLangName,ct,&subDirList.Item(ct));
#if 0 //#if wxCHECK_VERSION(2, 7, 1)
				}
				else
				{
					// The language is NOT available for a locale, so we'll
					// indicate that on the list item. Then, if the user chooses this item
					// we can advise him further about it.
					fullLangName += _T(" [NOT AVAILABLE IN CURRENT LOCALE]");
					pListBox->Insert(fullLangName,ct,&subDirList.Item(ct));
				}
#endif
			}
			else
			{
				// The list item data represents a language that wxWidgets doesn't know about.
				// It may be that we already got a name for this language from the user previously so
				// we should check first and use that instead of "[Contains Unknown or New Localization]".
				// If it really is a new localization, the user may choose to add this unknown language 
				// once it is selected from the list, in which case we query him for the language name 
				// Description and store it in wxFileConfig. See above for three ways to go from here.
				// 
				bool bFound = FALSE;
				wxString str = _T("");
				wxFile f;
				if (f.Exists(dirPath + gpApp->PathSeparator + appName + _T(".mo")) && f.Open(dirPath + gpApp->PathSeparator + appName + _T(".mo"),wxFile::read))
				{
					long fileLen; 
					fileLen = f.Length(); // get length of file in bytes.
					char* pBuff = new char[fileLen + 1]; // create on the heap just in case it is mistakenly a huge file
					memset(pBuff,0,fileLen + 1);
					int nReadBytes;
					nReadBytes = f.Read(pBuff,fileLen);
					// we were able to open the file, so now read through the file until we have read successive
					// bytes that together read "X-Poedit-Language: "
					char strToFind[] = "X-Poedit-Language: "; // length is 19 bytes plus null char = 20
					char* ptr;
					wxString languageFound;
					int pos, len, outcome;
					len = strlen(strToFind); // strlen gets length of strToFind not including the terminating null
					ptr = pBuff;
					// scan through the buffer with ptr and use memcmp to compare strToFind with the
					// portion of the buffer starting at ptr
					for (pos = 0; pos < fileLen - 20; pos++) // stop 20 bytes short of end of buffer
					{
						outcome = memcmp(strToFind, ptr+pos, len);
						// outcome is 0 if strToFind is equal to the sequence of len chars starting at ptr+pos
						if (outcome == 0)
						{
							bFound = TRUE;
							break;
						}
					}
					if (bFound)
					{
						// the "X-Poedit-Language: " string was found in the mo file, so now copy bytes from
						// the end of "X-Poedit-Language: " to the following \n character
						int ct = 0;
						while (pos < fileLen && *(ptr+pos+len+ct) != '\n')
						{
							str += *(ptr+pos+len+ct);
							ct++;
						}
					}
					f.Close();
					delete[] pBuff;
				}
				if (bFound)
				{
					// Do reality check. If str is longer than say 100 bytes, truncate it at 100 bytes
					// since a longer string won't show in the list box, and besides if the language name
					// is anywhere 100 bytes long, we probably have found something in the binary mo file
					// that is not in reality a language name.
					if (str.Length() > 100)
						str.Left(100);
					wxString tempStr(str);
					fullLangName = tempStr;
				}
				else if (gpApp->currLocalizationInfo.curr_shortName == subDirList.Item(ct))
				{
					fullLangName = gpApp->currLocalizationInfo.curr_fullName;
				}
				else
				{
					fullLangName = subDirList.Item(ct) + _T(" [Contains Unknown or New Localization]");
				}
				// append the fullLangName and associated subDir data to the list
				pListBox->Append(fullLangName,&subDirList.Item(ct));
			}
		}

	}
	// Here we insert the default (English) as the first item in the list (position 0) using the
	// wxListBox::Insert() method. 
	// whm: Check to see if the current system default language is NOT English. If that is the
	// case we need to ensure that en English is also a valid selection in the listbox. We also need
	// to remove any "default" language if there is no actual localization for it on the local machine.
	// A user whose computer was set to Dutch as its default system language, could not select English
	// because it was not offered as a choice, and selecting Dutch would not work either since although
	// the listbox offered Dutch as the "default" language, there was not actually a Dutch localization
	// of Adapt It existing on the machine. But, if the current system default language is French, we
	// want French to be the default selected choice because we do have a localization in French.
	if (fullDefaultLangNameListItem == _T("English") || gpApp->m_languageInfo == NULL)
		defaultDirStr = _T("en");
	else
		defaultDirStr = gpApp->m_languageInfo->CanonicalName;
	pListBox->Insert(fullDefaultLangNameListItem,0,&defaultDirStr); // the default language (1st item in list) saves its Canonical name as client data

	// we added the system language name to the list above so there must be at least one item in the list
	wxASSERT(pListBox->GetCount() > 0);
	// Highlight the first item (the default) unless a previously selected interface language was made,
	// in which case we want to continue using the previously selected interface regardless of the
	// currently set default system language.
	wxString str;
	wxLanguage lang(wxLANGUAGE_UNKNOWN);
	if (gpApp->currLocalizationInfo.curr_UI_Language == wxLANGUAGE_DEFAULT 
		|| gpApp->currLocalizationInfo.curr_UI_Language == wxLANGUAGE_UNKNOWN)
	{
		pListBox->SetSelection(0);
		// the first item in list is highlighted which is the system default language
		lang = (wxLanguage)wxLocale::GetSystemLanguage();
		str = wxLocale::GetLanguageName(lang);
	}
	else
	{
		// find the list item matching our current interface language
		int ct;
		for (ct = 0; ct < (int)pListBox->GetCount(); ct++)
		{
			wxString dirStr = *(wxString*)pListBox->GetClientData(ct);
			wxString xxshortName = gpApp->currLocalizationInfo.curr_shortName;
			// consider it a match if dirStr is two letters and it matches the first two letters of the
			// curr_shortName when the curr_shortName is of the form xx_XX.
			if (dirStr.Find(_T('_')) == -1 // if dirStr has no _ in name, i.e., is of the xx form instead of xx_XX
				&& xxshortName.Length() > 2 // and if curr_shortName > 2 chars
				&& xxshortName.Find(_T('_')) != -1) // and if curr_shortName has at least one '_' within it
			{
				// trim off the _XX... part so we can compare the xx major language parts
				xxshortName = xxshortName.Left(xxshortName.Find(_T('_')));
			}
			if (dirStr == xxshortName)
			{
				// we found the current language name
				pListBox->SetSelection(ct);
				str = pListBox->GetString(ct);
				wxString subStr = _("(Use system default language)");
				if (str.Find(subStr) != -1)
				{
					// strip off the substring and square brackets from the default language name
					str = str.Mid(subStr.Length());
					str.Trim(FALSE);
					str.Trim(TRUE);
					wxASSERT(str.GetChar(0) == _T('['));
					wxASSERT(str.GetChar(str.Length()-1) == _T(']'));
					str.Remove(0,1);
					str.RemoveLast(1);
				}
				break;
			}
		}
	}
	// update the static text showing the short and full language names of the selected item
	
	// whm added the following sanity check 10Feb09 after report from Patrick Rietveld of a crash due
	// to a residual Dutch entry getting into his registry before Adapt It WX knew how to handle
	// situations in which the system language is other than English (i.e., Dutch), but there is no
	// localization available for that language. In such cases we may not have a valid selection on
	// wxListBox, but ListBoxPassesSanityCheck() below fixes the situation.
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
		return;

	int nSel;
	nSel = pListBox->GetSelection();
	wxASSERT(nSel != -1);
	wxString shortName,longName;
	longName = pListBox->GetStringSelection();
	shortName = *(wxString*)pListBox->GetClientData(nSel);
	pEditAsStaticShortName->SetValue(shortName);
	pEditAsStaticLongName->SetValue(longName);
	/*
	const wxLanguageInfo *info = wxLocale::FindLanguageInfo(str);
	if(info != NULL)
	{
		pEditAsStaticShortName->SetValue(info->CanonicalName);
		pEditAsStaticLongName->SetValue(info->Description);
	}
	else if (m_bCurrentLanguageUserDefined)
	{
		pEditAsStaticShortName->SetValue(gpApp->currLocalizationInfo.curr_shortName);
		pEditAsStaticLongName->SetValue(gpApp->currLocalizationInfo.curr_fullName);
	}
	else
	{
		pEditAsStaticShortName->SetValue(_T(""));
		pEditAsStaticLongName->SetValue(_T(""));
	}
	*/
	m_fullNameOnEntry = longName;
	// end of scope for wxLogNull
}

// event handling functions

/*
// whm removed 8Dec11
void CChooseLanguageDlg::OnBrowseForPath(wxCommandEvent& WXUNUSED(event))
{
	// code for "Browse" button
	wxString savePath, selectedPath;
	savePath = gpApp->currLocalizationInfo.curr_localizationPath;
	wxDirDialog dirDlg(this, _("Browse to the folder that contains interface localizations for Adapt It"),
		gpApp->currLocalizationInfo.curr_localizationPath,wxDD_DIR_MUST_EXIST, wxDefaultPosition, wxSize(450,-1)); // dialog size is ignored under Windows
	if (dirDlg.ShowModal() == wxID_OK)
	{
		selectedPath = dirDlg.GetPath();

		// Check to ensure that localization folder(s)/file(s) exist at the chosen path
		if (gpApp->PathHas_mo_LocalizationFile(selectedPath,_T(""))) // _T("") for param 2 means that any named subfolder is OK
		{
			gpApp->currLocalizationInfo.curr_localizationPath = selectedPath;
			gpApp->m_localizationInstallPath = selectedPath; // save it here too
			gpApp->m_pLocale->AddCatalogLookupPathPrefix(selectedPath);
			// call InitDialog() to find and display the localizations at the new path selected
			wxInitDialogEvent ievent;
			InitDialog(ievent);
		}
		else
		{
			wxString msg;
			msg = msg.Format(_("Sorry, there are no localization folders or files at the following location you chose:\n%s"),selectedPath.c_str());
			wxMessageBox(msg,_T(""), wxICON_WARNING);
		}
	}
}
*/


void CChooseLanguageDlg::OnSelchangeListboxLanguages(wxCommandEvent& WXUNUSED(event)) 
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	//if (pListBox->GetCount() == 0)
	if (!ListBoxPassesSanityCheck((wxControlWithItems*)pListBox))
		return;

	int nSel;
	nSel = pListBox->GetSelection();
	//if (nSel == -1)
	//{
	//	// In wxGTK, when pListBox->Clear() is called it triggers this OnSelchangeListExistingTranslations
	//	// handler. The following message is of little help to the user even if it were called for a genuine
	//	// problem, so I've commented it out, so the present handler can exit gracefully
	//	//wxMessageBox(_("List box error when getting the current selection"), 
	//	//	_T(""), wxICON_ERROR);
	//	//wxASSERT(FALSE);
	//	return;
	//}
	
	// update the static text showing the short and full language names of the selected item
	wxASSERT(nSel != -1);
	wxString shortName,longName;
	longName = pListBox->GetStringSelection();
	shortName = *(wxString*)pListBox->GetClientData(nSel);
	pEditAsStaticShortName->SetValue(shortName);
	pEditAsStaticLongName->SetValue(longName);
	/*
	wxLanguage lang;
	wxString commonName;
	if (nSel == 0)
	{
		// user clicked on the first item in list which is the system default language
		lang = (wxLanguage)wxLocale::GetSystemLanguage();
		commonName = wxLocale::GetLanguageName(lang);
	}
	else
	{
		commonName = pListBox->GetString(nSel);
	}
	
	const wxLanguageInfo *info = wxLocale::FindLanguageInfo(commonName);
	if(info != NULL)
	{
		pEditAsStaticShortName->SetValue(info->CanonicalName);
		pEditAsStaticLongName->SetValue(info->Description);
	}
	else if (m_bCurrentLanguageUserDefined && commonName == gpApp->currLocalizationInfo.curr_fullName)
	{
		pEditAsStaticShortName->SetValue(gpApp->currLocalizationInfo.curr_shortName);
		pEditAsStaticLongName->SetValue(gpApp->currLocalizationInfo.curr_fullName);
	}
	else if (commonName != _T(" [Contains Unknown or New Localization]"))
	{
		wxString dirStr = *(wxString*)pListBox->GetClientData(nSel);
		pEditAsStaticShortName->SetValue(dirStr);
		pEditAsStaticLongName->SetValue(commonName);
	}
	else
	{
		pEditAsStaticShortName->SetValue(_T("[UNKNOWN]"));
		pEditAsStaticLongName->SetValue(_T("[UNKNOWN]"));
	}
	*/
	
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CChooseLanguageDlg::OnOK(wxCommandEvent& event) 
{
	// The user has clicked OK so we need to handle some special cases:
	// 1. the list choice was marked " [NOT AVAILABLE IN CURRENT LOCALE]" or, // removed 1.
	// 2. the list choise was marked " [Contains Unknown or New Localization]".
	// After handling the special cases above, we can ensure that the global
	// currLocalizationInfo struct's members are filled appropriately for the
	// user's chosen language, and that m_pConfig is updated by calling
	// SaveCurrentUILanguageInfoToConfig().
	int nSel;
	nSel = pListBox->GetSelection();
	if (nSel == -1) // LB_ERR is #define -1
	{
		// There is no valid selection, so we set the CurrLocalizationInfo
		// struct's members to use wxLANGUAGE_DEFAULT and associated data defaults.
		gpApp->currLocalizationInfo.curr_UI_Language = wxLANGUAGE_DEFAULT;
		gpApp->currLocalizationInfo.curr_shortName = _T("default");
		gpApp->currLocalizationInfo.curr_fullName = _T("system default language");
		gpApp->currLocalizationInfo.curr_localizationPath = gpApp->GetDefaultPathForLocalizationSubDirectories();
		//gpApp->m_localizationInstallPath = gpApp->currLocalizationInfo.curr_localizationPath; // save it here too
		return; // go back to the dialog in case the user wants to select a different language
	}
	m_strCurrentLanguage = pListBox->GetString(nSel);
	// also get the associated client data into dirStr
	wxString dirStr = *(wxString*)pListBox->GetClientData(nSel);

	//int posn;
	//posn = m_strCurrentLanguage.Find(_T(" [NOT AVAILABLE IN CURRENT LOCALE]"));
	//if (posn != -1)
	//{
	//	m_strCurrentLanguage = m_strCurrentLanguage.Mid(0,posn);
	//	wxString msg;
	//	msg = msg.Format(_(" The %s localization is not available on this computer. You may need to enable the language in your operating system. Adapt It will continue to use its default language interface."),m_strCurrentLanguage.c_str());
	//	wxMessageBox(msg,_T(""),wxICON_WARNING);
	//	// highlight the default list choice since the current choice didn't work
	//	pListBox->SetSelection(0);
	//	return; // go back to the dialog in case the user wants to select a different language
	//}

	// Is the selection a language that is unknown to wxWidgets?
	// We can determine if a language (using either the short name or the common/full name) is
	// "unknown" to wxWidgets by using the wxLocale::FindLanugageInfo() static method. Since any
	// user defined languages will have been added to the wxLanguage database in the App's OnInit(),
	// those also should be findable via FindLanguageInfo(). If FindLanguageInfo() returns
	// NULL we consider the language to be "unknown" to wxWidgets.
	// An "unknown" language here can be expressed in two ways:
	// 1. InitDialog() found the language name in the .mo binary file in which case
	//    the list box entry shows the language name, but does NOT have the  
	//    " [Contains Unknown or New Localization]" suffixed to the name in the list.
	// 2. InitDialog() found an .mo localization file but could not determine a language name
	//    by reading the binary file, and the dirStr did not match any known Canonical (short)
	//    name
	// Below we first determine if the language is unknown. If it is a known language we just update
	// the App's currLocalizationInfo struct and return from OnOK(). If it is an unknown language,
	// we deal with it depending on how it is expressed (see above).
	wxString tempFullLangName;
	const wxLanguageInfo *info = wxLocale::FindLanguageInfo(dirStr);
	if (info)
	{
		// The language represented by the dirStr (Canonical name) was found in the language database
		// so process and fill out the language info struct used by the App's ChooseInterfaceLanguage().
		// Note: FindLanguageInfo(dirStr) should also find the default language (1st item in listbox)
		// when selected, since dirStr was given the default language's CanonicalName.
		gpApp->currLocalizationInfo.curr_UI_Language = info->Language;
		gpApp->currLocalizationInfo.curr_shortName = info->CanonicalName;
		gpApp->currLocalizationInfo.curr_fullName = info->Description;

		// Here we only want to call SaveUserDefinedLanugage... if the language really is a user
		// defined one, i.e., its wxLanguage value is > wxLANGUAGE_USER_DEFINED
		if (info->Language > wxLANGUAGE_USER_DEFINED)
		{
			// Call the App's SaveUserDefinedLanguageInfoStringToConfig() to update the path part of the
			// information saved there (in case the user has changed it).
			gpApp->SaveUserDefinedLanguageInfoStringToConfig(gpApp->currLocalizationInfo.curr_UI_Language, 
														gpApp->currLocalizationInfo.curr_shortName, 
														gpApp->currLocalizationInfo.curr_fullName, 
														pEditLocalizationPath->GetValue());
		}
		// whm 8Dec11 removed the path manipulations below
		//gpApp->currLocalizationInfo.curr_localizationPath = pEditLocalizationPath->GetValue();
		//gpApp->m_localizationInstallPath = gpApp->currLocalizationInfo.curr_localizationPath; // save it here too
	}
	else
	{
		// The selection represents an "unknown" language to wxWidgets, which means that Adapt It has not
		// registered it as a user_defined_language_n in the registry/settings. See the App's OnInit()
		// where languages previously registered in the registry/hidden settings file are added to the 
		// locale.
		wxLanguageInfo langInfo;
		wxString commonName;
		// If the string stored in m_strCurrentLanguage does not have the value " [Contains Unknown or New
		// Localization]", it will have the full language name which we can use here for commonName
		if (m_strCurrentLanguage != _T(" [Contains Unknown or New Localization]"))
			commonName = m_strCurrentLanguage;
		else
		{
			wxString caption;
			caption = _("Enter Language Name");
			wxString prompt;
			prompt = prompt.Format(_("Please enter the common language name for the %s localization: "),m_strCurrentLanguage.c_str());
			commonName = ::wxGetTextFromUser(prompt,caption);
		}
		if (!commonName.IsEmpty())
		{
			int new_wxLangCode;
			gpApp->SaveUserDefinedLanguageInfoStringToConfig(new_wxLangCode, dirStr, commonName, pEditLocalizationPath->GetValue());
			// SaveUserDefinedLanguageInfoStringToConfig() above returns in new_wxLangCode, the next
			// consecutive new wxLangauge code available for this new language. The minimum value will
			// be wxLANGUAGE_USER_DEFINED + 1 if no previous "new" languages have been added on this
			// computer.
			// Note: Currently wxLANGUAGE_USER_DEFINED has the value 229 and the wx docs say that any
			// user defined language must have a value greater than 229. And since the maximum value for 
			// new_wxLangCode is 255 (the max for a wxLanguage enum), we could only at most add 26 
			// user defined languages for the current computer. I've arbitrarily limited the number
			// of user defined languages stored in the registry/hidden settings file to 9, since I 
			// think it highly unlikely that any one user would ever define even 9 new localization 
			// languages that are unknown to the wxWidgets wxLanguage list, much less 26! If a user 
			// defined more than 9, the only downside is that his computer wouldn't remember the 
			// earliest one(s), and he would have to be queried again for the language name - no big 
			// deal if it ever happened.
			//
			// Use the new_wxLangCode returned from above and call InitializeLanguageLocale()
			CurrLocalizationInfo currLocInfo; // first use a local copy of the struct until we see if InitializeLanguageLocale() below succeeds.

			currLocInfo.curr_UI_Language = new_wxLangCode;	// we are now using a user defined language
			currLocInfo.curr_shortName = dirStr;			// the dirStr of the subfolder where the .mo file was located
			currLocInfo.curr_fullName = commonName;			// the name supplied by the user for this language localization
			//currLocInfo.curr_localizationPath = pEditLocalizationPath->GetValue(); // the path shown in the path edit box
			
			// InitializeLanguageLocale() below deletes any exising wxLocale object and creates a new wxLocale
			// for the currently selected language (using the non-default wxLocale constructor).
			// The currLocalizationInfo struct was initialized with the interface language of choice above, so
			// now we can create the wxLocale object
			// whm 8Dec11 modified below to use the App's m_localizationInstallPath
			//if (gpApp->InitializeLanguageLocale(currLocInfo.curr_shortName, currLocInfo.curr_fullName, currLocInfo.curr_localizationPath))
			if (gpApp->InitializeLanguageLocale(currLocInfo.curr_shortName, currLocInfo.curr_fullName, gpApp->m_localizationInstallPath))
			{
				// Local initialization succeeded, so set the global currLocalizationInfo struct
				gpApp->currLocalizationInfo.curr_UI_Language = currLocInfo.curr_UI_Language;	// we are now using a user defined language
				gpApp->currLocalizationInfo.curr_shortName = currLocInfo.curr_shortName;		// the dirStr of the subfolder where the .mo file was located
				gpApp->currLocalizationInfo.curr_fullName = currLocInfo.curr_fullName;			// the name supplied by the user for this language localization
				//gpApp->currLocalizationInfo.curr_localizationPath = currLocInfo.curr_localizationPath; // the path shown in the path edit box
				//gpApp->m_localizationInstallPath = gpApp->currLocalizationInfo.curr_localizationPath; // save it here too
				
				// Update list to show user entered language name (curr_fullName). This name
				// should also be loaded subsequently when tpi (curr_shortName) is found as a directory name.
				pListBox->SetString(nSel, gpApp->currLocalizationInfo.curr_fullName);
				
				// User gave a common language name, so we call AddLanguage() to add this language to the
				// database of wxWidgets' known languages.
				langInfo.Language = new_wxLangCode;
				langInfo.CanonicalName = m_strCurrentLanguage;
	#ifdef __WIN32__
				// Win32 language identifiers
				langInfo.WinLang = 0;			// We don't know the Windows LANG_xxxx value so enter zero
				langInfo.WinSublang = 0;		// We don't know the Windows SUBLANG_xxxx value so enter zero 
	#endif
				langInfo.Description = commonName;
				// Note: There are no examples of AddLanguage in poEdit sources or Audacity sources
				gpApp->m_pLocale->AddLanguage(langInfo);
			}
			else
			{
				wxString msg = _T("Initialization of the new locale and use of localization files for %s failed.");
				msg = msg.Format(msg,currLocInfo.curr_fullName.c_str());
				wxMessageBox(msg,_T(""),wxICON_WARNING);

				// remove the user defined language from the registry/hidden settings file by calling the App's
				// RemoveUserDefinedanguageInfoStringFromConfig()
				gpApp->RemoveUserDefinedLanguageInfoStringFromConfig(dirStr, commonName);
			}
		}
		else
		{
			// No response was entered, so we assume the user decided to abort the current selection as
			// a choice.
			return; // go back to the dialog in case the user wants to select a different language
		}
	}

	if (m_fullNameOnEntry != gpApp->currLocalizationInfo.curr_fullName)
	{
		m_bChangeMade = TRUE;
	}
	// update edit boxes with current localization data
	pEditLocalizationPath->SetValue(gpApp->currLocalizationInfo.curr_localizationPath);
	pEditAsStaticShortName->SetValue(gpApp->currLocalizationInfo.curr_shortName);
	pEditAsStaticLongName->SetValue(gpApp->currLocalizationInfo.curr_fullName);

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

