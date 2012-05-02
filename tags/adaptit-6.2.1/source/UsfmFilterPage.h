/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UsfmFilterPage.h
/// \author			Bill Martin
/// \date_created	6 October 2010
/// \date_revised	6 October 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CUsfmFilterPageWiz and 
/// CUsfmFilterPagePrefs classes. It also defines a third class CUsfmFilterPageCommon which
/// contains the routines common to the above two classes.
/// The CUsfmFilterPageWiz class creates a wizard page for the StartWorkingWizard; the 
/// CUsfmFilterPagePrefs creates a identical panel as a tabbed page in the 
/// EditPreferencesDlg. Each form allows the user to change the USFM set and/or the
/// filtering state of standard format markers used in the current document and/or the
/// whole project. The interface resources for the page/panel are defined in 
/// UsfmFilterPageFunc() which was developed in and is maintained by wxDesigner.
/// \derivation		CUsfmFilterPageWiz is derived from wxWizardPage, 
/// CUsfmFilterPagePrefs from wxPanel, and CUsfmFilterPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef UsfmFilterPage_h
#define UsfmFilterPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "UsfmFilterPage.h"
#endif

#include "Adapt_It.h"

class MapSfmToUSFMAnalysisStruct;

/// The CUsfmFilterPageCommon class contains data and methods which are 
/// common to the CUsfmFilterPageWiz and CUsfmFilterPagePrefs classes.
/// \derivation The CUsfmFilterPageCommon class is derived from wxPanel
class CUsfmFilterPageCommon : public wxPanel
// CUsfmFilterPageCommon needs to be derived from wxPanel in order to use built-in 
// functions like FindWindowById()
{
public:
	wxSizer*	pUsfmFilterPageSizer;

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
	bool bWarningShownAlready; // set TRUE when project setting's warning is all
			// that was wanted, because the forced change of the document's setting
			// as well does not require a repeat of the warning, so this flag allows
			// for suppressing the second showing of the warning

	wxCheckListBox* pListBoxSFMsDoc; // initialized to point to above in constructor

	// The following arrays store information on all markers used in the Document, 
	wxArrayString m_SfmMarkerAndDescriptionsDoc; // 
	wxArrayString* pSfmMarkerAndDescriptionsDoc; // initialized to point to above in constructor
	
	wxArrayInt m_filterFlagsDoc; // these flag filtering markers
	wxArrayInt* pFilterFlagsDoc; // initialized to point to above in constructor
	
	wxArrayInt m_filterFlagsDocBeforeEdit; // to detect any changes to filter markers of the chosen set
	wxArrayInt* pFilterFlagsDocBeforeEdit; // initialized to point to above in constructor

	// Note: The Doc has CStrings named m_filterMarkersBeforeEdit and m_filterMarkersAfterEdit.
	// The following two are local variables used in the Usfm Filtering page.
	wxString tempFilterMarkersBeforeEditDoc; // initialized to equal gpApp->gCurrentFilterMarkers in the constructor
	wxString tempFilterMarkersAfterEditDoc; // initialized to equal tempFilterMarkersBeforeEditDoc in the constructor

	wxArrayInt m_userCanSetFilterFlagsDoc; // these flag the userCanSetFilter markers
	wxArrayInt* pUserCanSetFilterFlagsDoc; // initialized to point to above in constructor

	wxRadioButton* pRadioUsfmOnly;
	wxRadioButton* pRadioPngOnly;
	wxRadioButton* pRadioUsfmAndPng;

	wxRadioButton* pRadioUsfmOnlyProj;
	wxRadioButton* pRadioPngOnlyProj;
	wxRadioButton* pRadioUsfmAndPngProj;

	wxCheckBox* pChangeFixedSpaceToRegular;
	
	wxCheckListBox* pListBoxSFMsProj; // initialized to point to above in constructor
	// whm Note: The IDD_FILTER_PAGE dialog's m_listBoxSFMsProj control
	// is a wxCheckListBox, and as such, requires that the "Owner Draw" property
	// of the dialog's attribute be set from NO to "Fixed" or "Variable" (since 
	// we just have text and it should all be of the same height we use "Fixed"), 
	// and the "Has Strings" property attribute set from FALSE to TRUE.
	
	// The following arrays store information on all markers used in the Project defaults, 
	wxArrayString m_SfmMarkerAndDescriptionsProj;
	wxArrayString* pSfmMarkerAndDescriptionsProj; // initialized to point to above in constructor

	wxArrayInt m_filterFlagsProj;
	wxArrayInt* pFilterFlagsProj; // initialized to point to above in constructor

	wxArrayInt m_filterFlagsProjBeforeEdit; // to detect any changes to filter markers of the chosen set
	wxArrayInt* pFilterFlagsProjBeforeEdit; // initialized to point to above in constructor

	wxString tempFilterMarkersBeforeEditProj; // initialized to equal gpApp->gProjectFilterMarkersForConfig in the constructor
	wxString tempFilterMarkersAfterEditProj; // initialized to equal tempFilterMarkersBeforeEditProj in the constructor

	wxArrayInt m_userCanSetFilterFlagsProj; // these flag the userCanSetFilter markers
	wxArrayInt* pUserCanSetFilterFlagsProj; // initialized to point to above in constructor

	// The following arrays store information on all markers used in the Factory defaults, 
	wxArrayString m_SfmMarkerAndDescriptionsFactory;
	wxArrayString* pSfmMarkerAndDescriptionsFactory; // initialized to point to above in constructor

	wxArrayInt m_filterFlagsFactory;
	wxArrayInt* pFilterFlagsFactory; // initialized to point to above in constructor
	// Since the Factory sfm list is read only, we don't need to check for editing changes
	//wxArrayInt m_atSfmFlagsBeforeEdit; // to detect any changes to filter markers of the chosen set
	
	//wxArrayInt m_userCanSetFilterFlagsFactory; // these flag the userCanSetFilter markers
	//wxArrayInt* pUserCanSetFilterFlagsFactory; // initialized to point to above in constructor

	// Note: The following are a local temporary set of the variables of the same name on the App.
	// These local variables are for displaying the GUI state of unknown markers. When the user 
	// accepts any changes to the filtering state of unknown markers, these variables are copied to
	// the variables on the App.
	wxArrayString m_unknownMarkers; // array of unknown whole markers - no end markers stored here
	wxArrayInt m_filterFlagsUnkMkrs;// these flag the filtering state of unknown markers
	wxString m_currentUnknownMarkersStr;	// whm added 31May05. Stores the current active list of any unknown
									// whole markers in a space delimited string. This string is stored in 
									// the doc's Buffer member (5th field).

	//bool bShowAll;
	bool bFirstWarningGiven; // Used to only give filter warning once per Preferences session
	bool bDocFilterMarkersChanged; // for the Doc filter list Undo button enabling
	bool bProjectFilterMarkersChanged; // for the Project filter list Undo button enabling
	bool bOnInitDlgHasExecuted;

	//// Note: The WX version uses a single button for "Filter Out" and "Include for Adapting" 
	//// in the doc and project; and just changes the label.
	//// The MFC version used two separate dedicated buttons, one on top of the other with the 
	//// separate labels, the appropriate button being shown or hidden as needed. 
	wxTextCtrl* pTextCtrlAsStaticFilterPage;
	wxStaticText* pStaticNoDocsInThisProj;

	void DoSetDataAndPointers();
	void DoInit();
	
	void DoLbnSelchangeListSfmsDoc();
	void DoCheckListBoxToggleDoc(wxCommandEvent& event);
	void DoBoxClickedIncludeOrFilterOutDoc(int lbItemIndex);
	void DoLbnSelchangeListSfmsProj();
	void DoCheckListBoxToggleProj(wxCommandEvent& event);
	void DoBoxClickedIncludeOrFilterOutProj(int lbItemIndex);
	
	void DoBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseBothSetsDoc(wxCommandEvent& WXUNUSED(event));
	void DoBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void DoBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void DoBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void DoBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& WXUNUSED(event));
	
	void AddUnknownMarkersToDocArrays();
	void SetupFilterPageArrays(MapSfmToUSFMAnalysisStruct* pMap,
		wxString filterMkrStr,
		enum UseStringOrMap useStrOrMap,
		wxArrayString* pSfmMarkerAndDescr,
		wxArrayInt* pFilterFlags,
		wxArrayInt* pUserCanSetFlags);
	void LoadListBoxFromArrays(wxArrayString* pSfmMarkerAndDescr, 
		wxArrayInt* pFilterFlags, wxArrayInt* pUserCanSetFlags, wxCheckListBox* pListBox);
	void LoadDocSFMListBox(enum ListBoxProcess lbProcess);
	void LoadProjSFMListBox(enum ListBoxProcess lbProcess);
	wxString GetSetNameStr(enum SfmSet set);
	wxString GetFilterMkrStrFromFilterArrays(wxArrayString* pSfmMarkerAndDescr, wxArrayInt* pFilterFlags);
	void AddFilterMarkerToString(wxString& filterMkrStr, wxString wholeMarker);
	void RemoveFilterMarkerFromString(wxString& filterMkrStr, wxString wholeMarker);
	void AdjustFilterStateOfUnknownMarkerStr(wxString& unknownMkrStr, wxString wholeMarker, enum UnkMkrFilterSetting filterSetting);

protected:
	void UpdateButtons();
};

/// The CUsfmFilterPageWiz class creates a wizard page for the Startup Wizard 
/// which allows the user to change the USFM set and/or filtered material settings 
/// used for the project during initial setup.
/// The interface resources for the page/panel are defined in UsfmFilterPageFunc() 
/// which was developed in and is maintained by wxDesigner.
/// \derivation		CUsfmFilterPageWiz is derived from wxWizardPage.
class CUsfmFilterPageWiz : public wxWizardPage
{
public:
	CUsfmFilterPageWiz();
	CUsfmFilterPageWiz(wxWizard* parent); // constructor
	virtual ~CUsfmFilterPageWiz(void); // destructor // whm make all destructors virtual
	
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxScrolledWindow* m_scrolledWindow;
	
	/// an instance of the CUsfmFilterPageCommon class for use in CUsfmFilterPageWiz
	CUsfmFilterPageCommon usfm_filterPgCommon;
	
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	// implement functions unique to wxWizardPage
	void OnWizardCancel(wxWizardEvent& WXUNUSED(event));
	void OnWizardPageChanging(wxWizardEvent& event);
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

	void OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleDoc(wxCommandEvent& event);
	void OnBnClickedIncludeOrFilterOutDoc(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleProj(wxCommandEvent& event);
	void OnBnClickedIncludeOrFilterOutProj(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event);


private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CUsfmFilterPageWiz )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// The CUsfmFilterPagePrefs class creates a page for the USFM and Filtering tab in 
/// the Edit Preferences property sheet which allows the user to change the sfm set 
/// and/or filter settings used for the document and/or project. 
/// The interface resources for the page/panel are defined in UsfmFilterPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CUsfmFilterPagePrefs is derived from wxPanel.
class CUsfmFilterPagePrefs : public wxPanel
{
public:
	CUsfmFilterPagePrefs();
	CUsfmFilterPagePrefs(wxWindow* parent); // constructor
	virtual ~CUsfmFilterPagePrefs(void); // destructor // whm make all destructors virtual
	
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CUsfmFilterPageCommon class for use in CUsfmFilterPagePrefs
	CUsfmFilterPageCommon usfm_filterPgCommon;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

    // implement functions unique to CUsfmFilterPagePrefs
	void OnOK(wxCommandEvent& WXUNUSED(event)); 

	void OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleDoc(wxCommandEvent& event);
	void OnBnClickedIncludeOrFilterOutDoc(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleProj(wxCommandEvent& event);
	void OnBnClickedIncludeOrFilterOutProj(wxCommandEvent& WXUNUSED(event));

	void OnBnClickedRadioUseUbsSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsDoc(wxCommandEvent& event);
	void OnBnClickedRadioUseUbsSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseSilpngSetOnlyProj(wxCommandEvent& event);
	void OnBnClickedRadioUseBothSetsProj(wxCommandEvent& event);
	void OnBnClickedCheckChangeFixedSpacesToRegularSpaces(wxCommandEvent& event);


private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CUsfmFilterPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* UsfmFilterPage_h */
