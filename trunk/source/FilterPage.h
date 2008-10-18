/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FilterPage.h
/// \author			Bill Martin
/// \date_created	18 April 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CFilterPageWiz and CFilterPagePrefs classes. 
/// It also defines a third class CFilterPageCommon which
/// contains the routines common to the above two classes.
/// The CFilterPageWiz class creates a wizard page for the StartWorkingWizard; the 
/// CFilterPagePrefs creates a identical panel as a tabbed page in the 
/// EditPreferencesDlg. Each form allows the user to change the filtering state 
/// of standard format markers used in the current document and/or whole project. 
/// The interface resources for the page/panel are defined in FilterPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// Note: The filter page and the usfm page, although separate tabs/pages in the 
/// preferences property sheet and the startup wizard, are closely related and the
/// setting changes made in one page/tab affects the other page/tab. The dependencies
/// that one page/tab has for the other page/tab, and the need to share their 
/// functionality in wizard pages as well as in property sheet preference tabs somewhat 
/// complicates the classes and their implementation. Another complicating factor is
/// the fact that in the wxWidgets' implementation of wxCheckListBox, client data is 
/// reserved for internal use, and cannot be used to associate the list item with its
/// underlying data representation.
/// \derivation		CFilterPageWiz is derived from wxWizardPage, CFilterPagePrefs from wxPanel, and CFilterPageCommon from wxPanel.
/////////////////////////////////////////////////////////////////////////////

#ifndef FilterPage_h
#define FilterPage_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FilterPage.h"
#endif

class MapSfmToUSFMAnalysisStruct;

/// The CFilterPageCommon class contains data and methods which are 
/// common to the CFilterPageWiz and CFilterPagePrefs classes.
/// \derivation The CFilterPageCommon class is derived from wxPanel
class CFilterPageCommon : public wxPanel
// CFilterPageCommon needs to be derived from wxPanel in order to use built-in 
// functions like FindWindowById()
{
public:
	wxSizer*	pFilterPageSizer;

	wxCheckListBox* pListBoxSFMsDoc; // initialized to point to above in constructor
	// whm Note: The IDD_FILTER_PAGE dialog's m_listBoxSFMsDoc control
	// is a wxCheckListBox, and as such, requires in MFC that the "Owner Draw" property
	// of the dialog's attribute be set from NO to "Fixed" or "Variable" (since 
	// we just have text and it should all be of the same height we use "Fixed"), 
	// and the "Has Strings" property attribute set from FALSE to TRUE.

	// The following arrays store information on all markers used in the Document, 
	wxArrayString m_SfmMarkerAndDescriptionsDoc; // 
	wxArrayString* pSfmMarkerAndDescriptionsDoc; // initialized to point to above in constructor
	
	wxArrayInt m_filterFlagsDoc; // these flag filtering markers
	wxArrayInt* pFilterFlagsDoc; // initialized to point to above in constructor
	
	wxArrayInt m_filterFlagsDocBeforeEdit; // to detect any changes to filter markers of the chosen set
	wxArrayInt* pFilterFlagsDocBeforeEdit; // initialized to point to above in constructor

	// Note: The Doc has CStrings named m_filterMarkersBeforeEdit and m_filterMarkersAfterEdit.
	// The following two are local variables used in the Filtering page and USFM page.
	wxString tempFilterMarkersBeforeEditDoc; // initialized to equal gpApp->gCurrentFilterMarkers in the constructor
	wxString tempFilterMarkersAfterEditDoc; // initialized to equal tempFilterMarkersBeforeEditDoc in the constructor

	wxArrayInt m_userCanSetFilterFlagsDoc; // these flag the userCanSetFilter markers
	wxArrayInt* pUserCanSetFilterFlagsDoc; // initialized to point to above in constructor

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

	wxCheckListBox* pListBoxSFMsFactory; // initialized to point to above in constructor
	// whm Note: The IDD_FILTER_PAGE dialog's m_listBoxSFMsFactory control
	// is a wxCheckListBox, and as such, requires that the "Owner Draw" property
	// of the dialog's attribute be set from NO to "Fixed" or "Variable" (since 
	// we just have text and it should all be of the same height we use "Fixed"), 
	// and the "Has Strings" property attribute set from FALSE to TRUE.

	// The following arrays store information on all markers used in the Factory defaults, 
	wxArrayString m_SfmMarkerAndDescriptionsFactory;
	wxArrayString* pSfmMarkerAndDescriptionsFactory; // initialized to point to above in constructor

	wxArrayInt m_filterFlagsFactory;
	wxArrayInt* pFilterFlagsFactory; // initialized to point to above in constructor
	// Since the Factory sfm list is read only, we don't need to check for editing changes
	//wxArrayInt m_atSfmFlagsBeforeEdit; // to detect any changes to filter markers of the chosen set
	
	wxArrayInt m_userCanSetFilterFlagsFactory; // these flag the userCanSetFilter markers
	wxArrayInt* pUserCanSetFilterFlagsFactory; // initialized to point to above in constructor

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
	
	void DoLbnSelchangeListSfmsDoc();
	void DoCheckListBoxToggleDoc(wxCommandEvent& event);
	void DoBoxClickedIncludeOrFilterOutDoc(int lbItemIndex);
	void DoLbnSelchangeListSfmsProj();
	void DoCheckListBoxToggleProj(wxCommandEvent& event);
	void DoBoxClickedIncludeOrFilterOutProj(int lbItemIndex);
	void DoCheckListBoxToggleFactory(wxCommandEvent& event);

	void DoSetDataAndPointers();
	void DoInit();
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
	void LoadFactorySFMListBox();
	wxString GetSetNameStr(enum SfmSet set);
	wxString GetFilterMkrStrFromFilterArrays(wxArrayString* pSfmMarkerAndDescr, wxArrayInt* pFilterFlags);
	void AddFilterMarkerToString(wxString& filterMkrStr, wxString wholeMarker);
	void RemoveFilterMarkerFromString(wxString& filterMkrStr, wxString wholeMarker);
	void AdjustFilterStateOfUnknownMarkerStr(wxString& unknownMkrStr, wxString wholeMarker, enum UnkMkrFilterSetting filterSetting);
};

/// The CFilterPageWiz class creates a wizard page for the Startup Wizard 
/// which allows the user to change the filtered material settings used for the project during 
/// initial setup.
/// The interface resources for the page/panel are defined in FilterPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CFilterPageWiz is derived from wxWizardPage.
class CFilterPageWiz : public wxWizardPage
{
public:
	CFilterPageWiz();
	CFilterPageWiz(wxWizard* parent); // constructor
	virtual ~CFilterPageWiz(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD = IDD_FILTER_PAGE };
   
	/// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

	wxScrolledWindow* m_scrolledWindow;
	
	/// an instance of the CFilterPageCommon class for use in CFilterPageWiz
	CFilterPageCommon filterPgCommon;
	
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
	void OnLbnSelchangeListSfmsFactory(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleFactory(wxCommandEvent& event);
	
private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CFilterPageWiz )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

/// The CFilterPagePrefs class creates a page for the Filtering tab in the Edit Preferences property sheet
/// which allows the user to change the sfm set used for the document and/or project. 
/// The interface resources for the page/panel are defined in FilterPageFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		CFilterPagePrefs is derived from wxPanel.
class CFilterPagePrefs : public wxPanel
{
public:
	CFilterPagePrefs();
	CFilterPagePrefs(wxWindow* parent); // constructor
	virtual ~CFilterPagePrefs(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD = IDD_FILTER_PAGE };
   
	/// Creation
    bool Create( wxWindow* parent );

    /// Creates the controls and sizers
    void CreateControls();

	/// an instance of the CFilterPageCommon class for use in CFilterPagePrefs
	CFilterPageCommon filterPgCommon;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

    // implement functions unique to CFilterPagePrefs
	void OnOK(wxCommandEvent& WXUNUSED(event)); 

	void OnLbnSelchangeListSfmsDoc(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleDoc(wxCommandEvent& event);
	void OnBnClickedIncludeOrFilterOutDoc(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListSfmsProj(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleProj(wxCommandEvent& event);
	void OnBnClickedIncludeOrFilterOutProj(wxCommandEvent& WXUNUSED(event));
	void OnLbnSelchangeListSfmsFactory(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggleFactory(wxCommandEvent& event);

private:
	// class attributes
	
	// other class attributes

    DECLARE_DYNAMIC_CLASS( CFilterPagePrefs )
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* FilterPage_h */
