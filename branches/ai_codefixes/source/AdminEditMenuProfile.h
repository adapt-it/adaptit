/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminEditMenuProfile.h
/// \author			Bill Martin
/// \date_created	20 August 2010
/// \date_revised	20 August 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CAdminEditMenuProfile class. 
/// The CAdminEditMenuProfile class allows a program administrator to 
/// simplify a user's interface by only making certain menu items and 
/// other settings available (visible) and other menu items unavailable 
/// (hidden) to the user. A tabbed dialog is created that has one tab for a 
/// "Novice" profile, one for a "Custom 1" profile, and one for a "Custom 2" 
/// profile. Each tab page contains a checklist of interface menu items and 
/// other settings preceded by check boxes. Each profile tab starts with a
/// subset of preselected items, to which the administrator can tweak to 
/// his liking, checking those menu items he wants to be visible in the 
/// interface and un-checking the menu items that are to be hidden. After 
/// adjusting the visibility of the desired menu items for a given profile, 
/// the administrator can select the profile to be used, and the program 
/// will continue to use that profile each time the application is run. 
/// The selection is saved in the basic and project config files, and the 
/// profile information is saved in an external xml control file. 
/// \derivation		The CAdminEditMenuProfile class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef AdminEditMenuProfile_h
#define AdminEditMenuProfile_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AdminEditMenuProfile.h"
#endif

class CAdminEditMenuProfile : public AIModalDialog
{
public:
	CAdminEditMenuProfile(wxWindow* parent); // constructor
	virtual ~CAdminEditMenuProfile(void); // destructor
	wxNotebook* pNotebook;
	wxRadioBox* pRadioBox;
	wxCheckListBox* pCheckListBox;
	wxButton* pOKButton;
	int tempWorkflowProfile;
	AI_MenuStructure* tempMenuStructure;
	UserProfiles* tempUserProfiles;
	wxArrayInt itemsAlwaysChecked;
	bool bChangesMadeToProfiles;
	// other methods

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnNotebookTabChanged(wxNotebookEvent& event);
	void OnRadioBoxSelection(wxCommandEvent& WXUNUSED(event));
	void OnCheckListBoxToggle(wxCommandEvent& event);
	void OnCheckListBoxDblClick(wxCommandEvent& WXUNUSED(event));
	void PopulateListBox(int newTabIndex);
	void CopyMenuStructure(const AI_MenuStructure* pFromMenuStructure, AI_MenuStructure*& pToMenuStructure);
	void CopyUserProfiles(const UserProfiles* pFromUserProfiles, UserProfiles*& pToUserProfiles);
	bool ProfileItemIsSubMenuOfThisMainMenu(UserProfileItem* pUserProfileItem, wxString mmLabel);
	bool SubMenuIsInCurrentAIMenuBar(wxString itemText);
	bool UserProfilesHaveChanged(const UserProfiles* tempUserProfiles, const UserProfiles* appUserProfiles);
	wxString GetTopLevelMenuLabelForThisSubMenuID(wxString IDStr);

private:
	// class attributes
	CAdapt_ItApp* m_pApp;
	bool bCreatingDlg;
	wxString compareLBStr;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AdminEditMenuProfile_h */
