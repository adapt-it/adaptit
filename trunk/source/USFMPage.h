/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			USFMPage.h
/// \author			Bill Martin
/// \date_created	18 April 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CUSFMPageWiz, CUSFMPagePrefs and CUSFMPageCommon classes. 
/// The CUSFMPageWiz class creates a wizard page and the CUSFMPagePrefs class creates a
/// tab panel for the preferences notebook. A third class CUSFMPageCommon contains the
/// routines that are common to both of the other two classes. Together they allow the user
/// to change the sfm set used for the current document and/or whole project.
/// The interface resources for the page/panel are defined in USFMPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CUSFMPageWiz is derived from wxWizardPage, CUSFMPagePrefs from wxPanel, and CUSFMPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef USFMPage_h
#define USFMPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "USFMPage.h"
#endif

#include "Adapt_It.h"

class CFilterPageWiz;
class CFilterPagePrefs;

/// The CUSFMPageCommon class contains data and methods which are 
/// common to the CUSFMPageWiz and CUSFMPagePrefs classes.
/// \derivation The CUSFMPageCommon class is derived from wxPanel
class CUSFMPageCommon : public wxPanel
{
public:
	wxSizer*	pUSFMPageSizer;

	enum SfmSet tempSfmSetBeforeEditDoc; // to detect any changes to sfm set
	enum SfmSet tempSfmSetAfterEditDoc; // holds the sfm set selected by user until user
							// selects OK. If user cancels, no changes are
							// made to the global gpApp->gCurrentSfmSet. If user
							// selects OK, the caller must assign gpApp->gCurrentSfmSet
							// the value of tempSfmSetAfterEditDoc.
	enum SfmSet tempSfmSetBeforeEditProj; // to detect any changes to Project sfm set
	enum SfmSet tempSfmSetAfterEditProj; // holds the Project sfm set selected by user until user
							// selects OK. If user cancels, no changes are
							// made to the global gpApp->gProjectSfmSetForConfig. If user
							// selects OK, the caller must assign gpApp->gProjectSfmSetForConfig
							// the value of tempSfmSetAfterEditProj.
	// Note: Factory default sfm set is UsfmOnly and can't be changed,
	// so we don't need before and after edit variables for it

	bool bSFMsetChanged;

	bool bChangeFixedSpaceToRegularBeforeEdit;
	bool bChangeFixedSpaceToRegularSpace;
	bool bShowFixedSpaceCheckBox;	// set FALSE in constructor but caller should 
									// set this before calling DoModal()
	bool bDocSfmSetChanged; // for the Doc sfm set Undo button enabling
	bool bProjectSfmSetChanged; // for the Project sfm set Undo button enabling
	bool bOnInitDlgHasExecuted;

	wxRadioButton* pRadioUsfmOnly;
	wxRadioButton* pRadioPngOnly;
	wxRadioButton* pRadioUsfmAndPng;
	//wxButton* pButtonUndoDocChanges;

	wxRadioButton* pRadioUsfmOnlyProj;
	wxRadioButton* pRadioPngOnlyProj;
	wxRadioButton* pRadioUsfmAndPngProj;

	wxRadioButton* pRadioUsfmOnlyFactory;
	wxRadioButton* pRadioPngOnlyFactory;
	wxRadioButton* pRadioUsfmAndPngFactory;

	wxCheckBox* pChangeFixedSpaceToRegular;

	wxTextCtrl* pTextCtrlStaticTextUSFMPage;

	void DoSetDataAndPointers();
	void DoInit();

	void DoBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseBothSetsDoc(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void DoBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void DoBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void DoBnClickedRadioUseUbsSetOnlyFactory(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseBothSetsFactory(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& WXUNUSED(event));

protected:
	void UpdateButtons();
};

/// The CUSFMPageWiz class creates a wizard page for the Startup Wizard 
/// which allows the user to change the sfm set used for the project during 
/// initial setup.
/// The interface resources for the page/panel are defined in USFMPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CUSFMPageWiz is derived from wxWizardPage.
class CUSFMPageWiz : public wxWizardPage
{
public:
	CUSFMPageWiz();
	CUSFMPageWiz(wxWizard* parent); // constructor
	virtual ~CUSFMPageWiz(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD = IDD_USFM_PAGE };
   
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CUSFMPageCommon class for use in CUSFMPageWiz
	CUSFMPageCommon usfmPgCommon;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	// implement wxWizardPage functions
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
	void OnWizardPageChanging(wxWizardEvent& event);
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

	void OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyFactory(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsFactory(wxCommandEvent& event);
	void OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event);

private:
	// class attributes
	
    DECLARE_DYNAMIC_CLASS( CUSFMPageWiz )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// The CUSFMPagePrefs class creates a page for the Usfm tab in the Edit Preferences property sheet
/// which allows the user to change the sfm set used for the document and/or project. 
/// The interface resources for the page/panel are defined in USFMPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CUSFMPagePrefs is derived from wxPanel.
class CUSFMPagePrefs : public wxPanel
{
public:
	CUSFMPagePrefs();
	CUSFMPagePrefs(wxWindow* parent); // constructor
	virtual ~CUSFMPagePrefs(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD = IDD_USFM_PAGE };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CUSFMPageCommon class for use in CUSFMPagePrefs
	CUSFMPageCommon usfmPgCommon;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	
	void OnOK(wxCommandEvent& WXUNUSED(event)); 

	void OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyFactory(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioUseSilpngSetOnlyFactory(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsFactory(wxCommandEvent& event);
	void OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event);

private:
	// class attributes
	
    DECLARE_DYNAMIC_CLASS( CUSFMPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* USFMPage_h */
