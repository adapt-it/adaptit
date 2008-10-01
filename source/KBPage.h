/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			KBPage.h
/// \author			Bill Martin
/// \date_created	17 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CKBPage class. 
/// The CKBPage class creates a wxPanel that allows the 
/// user to define knowledge base backup options and to reenter the 
/// source and target language names should they become corrupted. 
/// The panel becomes a "Backups and KB" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the BackupsAndKBPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CKBPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBPage_h
#define KBPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBPage.h"
#endif

/// The CKBPage class creates a wxPanel that allows the 
/// user to define knowledge base backup options and to reenter the 
/// source and target language names should they become corrupted. 
/// The panel becomes a "Backups and KB" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the BackupsAndKBPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CKBPage class is derived from wxPanel.
class CKBPage : public wxPanel
{
public:
	CKBPage();
	CKBPage(wxWindow* parent); // constructor
	virtual ~CKBPage(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_KB_PAGE };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxSizer*	pKBPageSizer;
	wxCheckBox* m_pCheckDisableAutoBkups;
	wxCheckBox* m_pCheckBkupWhenClosing;
	wxCheckBox* m_pCheckLegacySourceTextCopy;
	wxTextCtrl*	m_pEditSrcName;
	wxTextCtrl*	m_pEditTgtName;
	wxTextCtrl* pTextCtrlAsStaticTextBackupsKB;
	wxRadioButton* pRadioAdaptBeforeGloss;
	wxRadioButton* pRadioGlossBeforeAdapt;
	bool		tempDisableAutoKBBackups;
	bool		tempAdaptBeforeGloss; // was m_bAdaptBeforeGloss in MFC version
	bool		tempNotLegacySourceTextCopy; // was m_bNotLegacySourceTextCopy in MFC version
	bool		tempBackupDocument;
	wxString	tempSrcName;
	wxString	tempTgtName;
	wxString strSaveSrcName;
	wxString strSaveTgtName;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event)); 

	void OnCheckKbBackup(wxCommandEvent& WXUNUSED(event));
	void OnCheckBakupDoc(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioAdaptBeforeGloss(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioGlossBeforeAdapt(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CKBPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* KBPage_h */
