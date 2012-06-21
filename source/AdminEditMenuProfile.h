/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminEditMenuProfile.h
/// \author			Bill Martin
/// \date_created	20 August 2010
/// \date_revised	15 October 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CAdminEditMenuProfile class. 
/// The CAdminEditMenuProfile class allows a program administrator to 
/// simplify a user's interface by only making certain menu items and 
/// other settings available (visible) and other menu items unavailable 
/// (hidden) to the user. A tabbed dialog is created that has tabs for 
/// "Novice", "Experienced", "Skilled" and "Custom" profiles. Each tab 
/// page contains a checklist of interface menu items and other settings 
/// preceded by check boxes. Each profile tab starts with a subset of 
/// preselected items, which the administrator can further modify to 
/// his liking, checking those menu items he wants to be visible in the 
/// interface and un-checking the menu items that are to be hidden. After 
/// adjusting the visibility of the desired menu items for a given profile, 
/// the administrator can select the profile to be used, and the program 
/// will continue to use that profile each time the application is run. 
/// The selection is saved in the basic and project config files, and the 
/// profile information is saved in an external control file under the
/// name AI_UserProfiles.xml. 
/// \derivation		The CAdminEditMenuProfile class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef AdminEditMenuProfile_h
#define AdminEditMenuProfile_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AdminEditMenuProfile.h"
#endif

// whm 14Jun12 modified to use wxDialog for wxWidgets 2.9.x and later; wxScrollingDialog for pre-2.9.x
#if wxCHECK_VERSION(2,9,0)
class CAdminEditMenuProfile : public wxDialog // use wxScrollingDialog instead of AIModalDialog because we use wxUpdateUIEvent
#else
class CAdminEditMenuProfile : public wxScrollingDialog // use wxScrollingDialog instead of AIModalDialog because we use wxUpdateUIEvent
#endif
{
public:
	CAdminEditMenuProfile(wxWindow* parent); // constructor
	virtual ~CAdminEditMenuProfile(void); // destructor
	wxNotebook* pNotebook;
	wxRadioButton* pRadioBtnNone;
	wxRadioButton* pRadioBtnUseAProfile;
	wxButton* pButtonResetToFactory;
	wxTextCtrl* pEditProfileDescr;
	wxStaticText* pStaticTextDescr;
	wxComboBox* pComboBox;
	wxCheckListBox* pCheckListBox;
	wxButton* pOKButton;
	int tempWorkflowProfile; // 0 = "None", 1 = "Novice", 2 = "Experienced", 3 = "Skilled", 4 = "Custom"
	int startingWorkflowProfile;
	int selectedComboProfileIndex; // 0 = "Novice", 1 = "Experienced", 2 = "Skilled", 3 = "Custom"
	int notebookTabIndex; // 0 = "Novice", 1 = "Experienced", 2 = "Skilled", 3 = "Custom"
	int lastNotebookTabIndex;
	UserProfiles* tempUserProfiles;
	wxArrayInt itemsAlwaysChecked;
	bool bChangesMadeToProfileItems;
	bool bChangeMadeToProfileSelection;
	bool bChangeMadeToDescriptionItems;
	wxArrayInt bProfileChanged;
	wxArrayInt bDescriptionChanged;
	wxArrayString factoryDescrArrayStrings;
	wxSizer* pAdminEditMenuProfileDlgSizer;
	// other methods

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnUpdateButtonOK(wxUpdateUIEvent& WXUNUSED(event));
	void OnCancel(wxCommandEvent& WXUNUSED(event));
	void OnBtnResetToFactory(wxCommandEvent& WXUNUSED(event));
	void OnUpdateBtnResetToFactory(wxUpdateUIEvent& WXUNUSED(event));
	void OnNotebookTabChanged(wxNotebookEvent& event);
	void OnComboBoxSelection(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggle(wxCommandEvent& event);
	void OnCheckListBoxDblClick(wxCommandEvent& WXUNUSED(event));
	void OnRadioNone(wxCommandEvent& WXUNUSED(event));
	void OnRadioUseProfile(wxCommandEvent& WXUNUSED(event));
	void OnEditBoxDescrChanged(wxCommandEvent& WXUNUSED(event));
	void PopulateListBox(int newTabIndex);
	void CopyUserProfiles(const UserProfiles* pFromUserProfiles, UserProfiles*& pToUserProfiles);
	bool ProfileItemIsSubMenuOfThisMainMenu(UserProfileItem* pUserProfileItem, wxString mmLabel);
	bool SubMenuIsInCurrentAIMenuBar(wxString itemText);
	bool UserProfileItemsHaveChanged();
	bool UserProfileDescriptionsHaveChanged();
	bool CurrentProfileDescriptionHasChanged();
	bool ThisUserProfilesItemsDifferFromFactory(int profileSelectionIndex);
	bool UserProfileSelectionHasChanged();
	wxString GetNameOfProfileFromProfileValue(int tempWorkflowProfile);
	wxString GetNameOfProfileFromProfileIndex(int profileIndex);
	wxString GetNamesOfEditedProfiles();
	int GetIndexOfProfileFromProfileName(wxString profileName);
	wxArrayString GetArrayOfFactoryDescrStrings();
	void CopyProfileVisibilityValues(int sourceProfileIndex, int destinationProfileIndex);

private:
	// class attributes
	CAdapt_ItApp* m_pApp;
	bool bCreatingDlg;
	wxString compareLBStr;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AdminEditMenuProfile_h */
