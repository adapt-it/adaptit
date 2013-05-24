/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ProjectPage.h
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CProjectPage class. 
/// The CProjectPage class creates a panel that is used in the Edit Preferenced property sheet. 
/// The CProjectPage class allows the user to choose an existing project to work on or 
/// a <New Project>.
/// The interface resources for the CProjectPage are defined in ProjectPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// The CProjectPage class is derived from wxWizardPage, rather than wxWizardPageSimple
/// in order to utilize its GetPrev() and GetNext() handlers as a means to facilitate
/// an 8-page wizard when the user selects <New Project>.
/// \derivation		CProjectPage is derived from the wxWizardPage class.
/////////////////////////////////////////////////////////////////////////////

#ifndef ProjectPage_h
#define ProjectPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ProjectPage.h"
#endif

/// The CProjectPage class creates a panel that is used in the Edit Preferenced property sheet. 
/// The CProjectPage class allows the user to choose an existing project to work on or 
/// a <New Project>.
/// The interface resources for the CProjectPage are defined in ProjectPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// The CProjectPage class is derived from wxWizardPage, rather than wxWizardPageSimple
/// in order to utilize its GetPrev() and GetNext() handlers as a means to facilitate
/// an 8-page wizard when the user selects <New Project>.
/// \derivation		CProjectPage is derived from the wxWizardPage class.
class CProjectPage : public wxWizardPage
{
public:
	CProjectPage();
	CProjectPage(wxWizard* parent); // constructor
	virtual ~CProjectPage(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_PROJECT_PAGE };

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxScrolledWindow* m_scrolledWindow;
	
	wxSizer* pProjectPageSizer;

	wxString m_projectName;
	wxListBox* m_pListBox;
	int m_curLBSelection;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event)); // needs to be public because it's called from the App
    
	// implement wxWizardPage functions
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
	void OnWizardPageChanging(wxWizardEvent& event);
	void OnWizardPageChanged(wxWizardEvent& event);
	void OnCallWizardNext(wxCommandEvent& WXUNUSED(event));
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

protected:
	void OnLBSelectItem(wxCommandEvent& WXUNUSED(event));
	void OnButtonWhatIsProject(wxCommandEvent& WXUNUSED(event));

private:
	// other class attributes
	// BEW added 20May13, for the msg box and text control for the username to be typed in
	wxTextCtrl* pUsernameMsgTextCtrl;
	wxTextCtrl* pUsernameTextCtrl;
	wxTextCtrl* pInformalUsernameTextCtrl;
	wxString usernameMsgTitle;
	wxString usernameMsg;
	wxString usernameInformalMsgTitle;
	wxString usernameInformalMsg;

    DECLARE_DYNAMIC_CLASS( CProjectPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ProjectPage_h */
