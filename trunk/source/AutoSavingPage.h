/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			AutoSavingPage.h
/// \author			Bill Martin
/// \date_created	18 August 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CAutoSavingPage class. 
/// The CAutoSavingPage class creates a wxPanel that allows the 
/// user to define the various document and knowledge base saving parameters. 
/// The panel becomes a "Auto-Saving" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the AutoSavingPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CAutoSavingPage class is derived from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef AutoSavingPage_h
#define AutoSavingPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AutoSavingPage.h"
#endif

/// The CAutoSavingPage class creates a wxPanel that allows the 
/// user to define the various document and knowledge base saving parameters. 
/// The panel becomes a "Auto-Saving" tab of the EditPreferencesDlg.
/// The interface resources are loaded by means of the AutoSavingPageFunc()
/// function which was developed and is maintained by wxDesigner.
/// \derivation		The CAutoSavingPage class is derived from wxPanel.
class CAutoSavingPage : public wxPanel
{
public:
	CAutoSavingPage();
	CAutoSavingPage(wxWindow* parent); // constructor
	virtual ~CAutoSavingPage(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_AUTO_SAVE_PAGE };
	
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxSizer* pAutoSavingPageSizer;

	bool	tempNoAutoSave;
	int		tempKBMinutes;
	int		tempMinutes;
	int		tempMoves;
	int		tempSeconds;
	bool	tempIsDocTimeButton;

	wxCheckBox* m_pCheckNoAutoSave;
	wxRadioButton* m_pRadioByMinutes;
	wxRadioButton* m_pRadioByMoves;
	wxTextCtrl* m_pEditMinutes;
	wxTextCtrl*	m_pEditMoves;
	wxTextCtrl* m_pEditKBMinutes;
	wxTextCtrl* m_pTextCtrlAsStatic;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event)); 
	
	void OnButtonHighlightColor();
	void OnRadioByMinutes(wxCommandEvent& WXUNUSED(event));
	void OnRadioByMoves(wxCommandEvent& WXUNUSED(event));
	void OnCheckNoAutoSave(wxCommandEvent& WXUNUSED(event));
	void EnableAll(bool bEnable = TRUE);

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CAutoSavingPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AutoSavingPage_h */
