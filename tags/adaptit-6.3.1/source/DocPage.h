/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DocPage.h
/// \author			Bill Martin
/// \date_created	3 May 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CDocPage class. 
/// The CDocPage class creates a wizard panel that allows the user
/// to either create a new document or select a document to work on 
/// from a list of existing documents.
/// \derivation		The CDocPage class is derived from wxWizardPage.
/////////////////////////////////////////////////////////////////////////////

#ifndef DocPage_h
#define DocPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "DocPage.h"
#endif

/// The CDocPage class creates a wizard panel that allows the user
/// to either create a new document or select a document to work on 
/// from a list of existing documents.
/// \derivation		The CDocPage class is derived from wxWizardPage.
class CDocPage : public wxWizardPage
{
public:
	CDocPage();
	CDocPage(wxWizard* parent); // constructor
	virtual ~CDocPage(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_DOC_PAGE };
   
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxScrolledWindow* m_scrolledWindow;
	
	wxSizer* pDocPageSizer;
	bool m_bForceUTF8;
	wxString m_staticModeStr;
	wxString m_staticFolderStr;
	wxString m_docName;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event)); // needs to be public because it's called from the App
    
	// implement wxWizardPage functions
	void OnWizardFinish(wxWizardEvent& WXUNUSED(event)); // make it public
	void OnCallWizardFinish(wxCommandEvent& WXUNUSED(event)); // since 2.5.3 two handlers cant call same function
    virtual wxWizardPage* GetPrev() const;
    virtual wxWizardPage* GetNext() const;

	void OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListNewdocAndExistingdoc(wxCommandEvent& WXUNUSED(event)); // need this???

protected:
	wxListBox* m_pListBox;

	// whm added 21Apr05
	wxCheckBox* pChangeFixedSpaceToRegular;
	bool bChangeFixedSpaceToRegularSpace;

	void OnSetActive(); // not called by EVT_ACTIVATE //void OnActivate(wxActivateEvent& event); 
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
	void OnWizardPageChanging(wxWizardEvent& event);
	void OnButtonWhatIsDoc(wxCommandEvent& WXUNUSED(event));
	void OnButtonChangeFolder(wxCommandEvent& event);
	void OnCheckForceUtf8(wxCommandEvent& WXUNUSED(event));
	//void OnCheckSaveUsingXML(wxCommandEvent& WXUNUSED(event));

private:
	// other class attributes
	
	wxWizard* m_pParentWizard;

    DECLARE_DYNAMIC_CLASS( CDocPage )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* DocPage_h */
