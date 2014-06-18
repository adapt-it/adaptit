/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SplitDialog.h
/// \author			Jonathan Field; modified by Bill Martin for the WX version
/// \date_created	15 May 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the SplitDialog class.
/// The SplitDialog class handles the interface between the user and the
/// SplitDialog dialog which is designed to enable the user to split Adapt
/// It documents into smaller unit files.
/// The SplitDialog class is derived from AIModalDialog (DialogBase in MFC).
/// whm Note: SplitDialog was created for the MFC version by Jonathan Field. 
/// Jonathan based the SplitDialog class on a base class he created called 
/// DialogBase (which in turn is based on CDialog). Using such a base dialog 
/// with its special handlers is a good idea, and could have saved some 
/// repetition in other AI dialogs, but complicates things at this late stage 
/// for the wxWidgets version, since his DialogBase is very MFC centric. 
/// Therefore, rather than create a wxWidgets wxDialogBase class, I've 
/// implemented SplitDialog and its methods without dependency on a wxDialogBase 
/// class.
/// \derivation		SplitDialog is derived from AIModalDialog and the supporting Chapter class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef SplitDialog_h
#define SplitDialog_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SplitDialog.h"
#endif

// forward reference

/// The Chapter class is a supporting class for the SplitDialog class.
/// \derivation The Chapter class is derived from wxObject.
class Chapter : public wxObject
{
public:
	int Number;
	wxString NumberString;
	wxString FileName;
	wxString FilePath;
	SPList *SourcePhrases;
};

/// wxList declaration and partial implementation of the ChList class being
/// a list of pointers to Chapter objects
WX_DECLARE_LIST(Chapter, ChList); // see list definition macro in .cpp file

/// The SplitDialog class handles the interface between the user and the
/// SplitDialog dialog which is designed to enable the user to split Adapt
/// It documents into smaller unit files.
/// whm Note: SplitDialog was created for the MFC version by Jonathan Field. 
/// Jonathan based the SplitDialog class on a base class he created called 
/// DialogBase (which in turn is based on CDialog). Using such a base dialog 
/// with its special handlers is a good idea, and could have saved some 
/// repetition in other AI dialogs, but complicates things at this late stage 
/// for the wxWidgets version, since his DialogBase is very MFC centric. 
/// Therefore, rather than create a wxWidgets wxDialogBase class, I've 
/// implemented SplitDialog and its methods without dependency on a wxDialogBase 
/// class.
/// \derivation		SplitDialog is derived from AIModalDialog (DialogBase in the MFC version).
class CSplitDialog : public AIModalDialog
{
public:
	//enum { IDD = IDD_UNITS_DLG };
	CSplitDialog();
	CSplitDialog(wxWindow* parent); // constructor
	virtual ~CSplitDialog(void); // destructor // whm make all destructors virtual

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void ListFiles();
	bool GoToNextChapter_Interactive();
	void SplitAtPhraseBoxLocation_Interactive();
	void SplitIntoChapters_Interactive();

public:

	wxRadioButton* pSplitAtPhraseBox;
	wxRadioButton* pSplitAtNextChapter;
	wxRadioButton* pSplitIntoChapters;
	wxButton* pLocateNextChapter;
	wxTextCtrl* pFileName1;
	wxTextCtrl* pFileName2;
	wxListBox* pFileList;
	wxStaticText* pFileListLabel;
	wxStaticText* pSplittingWait;
	wxStaticText* pFileName1Label;
	wxStaticText* pFileName2Label;
	wxSizer* pSplitDialogSizer;

	void OnBnClickedButtonNextChapter(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedButtonSplitNow(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioPhraseboxLocation(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioChapterSfmarker(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioDivideIntoChapters(wxCommandEvent& WXUNUSED(event));
	void RadioButtonsChanged();
	bool SplitAtPhraseBox_IsChecked();
	bool SplitAtNextChapter_IsChecked();
	bool SplitIntoChapters_IsChecked();
	bool CurrentDocSpansMoreThanOneChapter();
	ChList *DoSplitIntoChapters(wxString WorkingFolderPath,wxString FileNameBase,
								SPList *SourcePhrases,int *cChapterDigits);
	// BEW addition 08Nov05
	int	GetListItem(wxListBox* pFileList, wxString& s);
	bool IsRadioButtonSelected(int ID);

	DECLARE_EVENT_TABLE()
};
#endif /* SplitDialog_h */
