/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseLanguageDlg.h
/// \author			Bill Martin
/// \date_created	15 July 2008
/// \date_revised	15 July 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CChooseLanguageDlg class. 
/// The CChooseLanguageDlg class provides a dialog in which the user can change
/// the interface language used by Adapt It. The dialog also allows the user to
/// browse to a different path (from that expected) to find the interface
/// language folders containing the <appName>.mo localization files.
/// \derivation		The CChooseLanguageDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ChooseLanguageDlg_h
#define ChooseLanguageDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ChooseLanguageDlg.h"
#endif

class CChooseLanguageDlg : public AIModalDialog
{
public:
	CChooseLanguageDlg(wxWindow* parent); // constructor
	virtual ~CChooseLanguageDlg(void); // destructor
	// other methods
	wxArrayString subDirList;
	wxString defaultDirStr;
	wxString pathToLocalizationFolders;
	wxString m_pathToLocalizationFolders;
	wxString m_strCurrentLanguage;
	wxString m_fullNameOnEntry;
	bool m_bCurrentLanguageUserDefined;
	bool m_bChangeMade;
	//int m_nLanguageChoice;

	wxListBox* pListBox;
	wxTextCtrl* pEditLocalizationPath;
	wxTextCtrl* pEditAsStaticShortName;
	wxTextCtrl* pEditAsStaticLongName;
	//wxButton* pBtnBrowse;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	
	//void OnBrowseForPath(wxCommandEvent& WXUNUSED(event)); // whm removed 8Dec11
	void OnSelchangeListboxLanguages(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ChooseLanguageDlg_h */
