/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminEditMenuProfile.cpp
/// \author			Bill Martin
/// \date_created	20 August 2010
/// \date_revised	11 October 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAdminEditMenuProfile class. 
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
/// \derivation		The CAdminEditMenuProfile class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in AdminEditMenuProfile.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "AdminEditMenuProfile.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/choicdlg.h>
#include "Adapt_It.h"
#include "MainFrm.h"
#include "AdminEditMenuProfile.h"
#include "XML.h"

/// This global is defined in Adapt_ItView.cpp.
extern CAdapt_ItApp* m_pApp;

// event handler table
BEGIN_EVENT_TABLE(CAdminEditMenuProfile, wxDialog)
	EVT_INIT_DIALOG(CAdminEditMenuProfile::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_CANCEL, CAdminEditMenuProfile::OnCancel)
	EVT_BUTTON(wxID_OK, CAdminEditMenuProfile::OnOK)
	EVT_UPDATE_UI(wxID_OK, CAdminEditMenuProfile::OnUpdateButtonOK)
	EVT_BUTTON(ID_BUTTON_RESET_TO_FACTORY, CAdminEditMenuProfile::OnBtnResetToFactory)
	EVT_UPDATE_UI(ID_BUTTON_RESET_TO_FACTORY, CAdminEditMenuProfile::OnUpdateBtnResetToFactory)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_MENU_EDITOR_NOTEBOOK, CAdminEditMenuProfile::OnNotebookTabChanged)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_NONE,CAdminEditMenuProfile::OnRadioNone)
	EVT_RADIOBUTTON(ID_RADIOBUTTON_USE_PROFILE,CAdminEditMenuProfile::OnRadioUseProfile)
	EVT_COMBOBOX(ID_COMBO_PROFILE_ITEMS, CAdminEditMenuProfile::OnComboBoxSelection)
	EVT_CHECKLISTBOX(ID_CHECKLISTBOX_MENU_ITEMS,CAdminEditMenuProfile::OnCheckListBoxToggle)
	EVT_LISTBOX_DCLICK(ID_CHECKLISTBOX_MENU_ITEMS,CAdminEditMenuProfile::OnCheckListBoxDblClick)
END_EVENT_TABLE()

CAdminEditMenuProfile::CAdminEditMenuProfile(wxWindow* parent) // dialog constructor
	: wxDialog(parent, -1, _("User Workflow Profiles"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	bCreatingDlg = TRUE;

	MenuEditorDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bCreatingDlg = FALSE;

	m_pApp = (CAdapt_ItApp*)&wxGetApp();
	wxASSERT(m_pApp != NULL);
	
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);

	pNotebook = (wxNotebook*)FindWindowById(ID_MENU_EDITOR_NOTEBOOK);
	wxASSERT(pNotebook != NULL);
	pButtonResetToFactory = (wxButton*)FindWindowById(ID_BUTTON_RESET_TO_FACTORY);
	wxASSERT(pButtonResetToFactory != NULL);
	pRadioBtnNone = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_NONE);
	wxASSERT(pRadioBtnNone != NULL);
	pRadioBtnUseAProfile = (wxRadioButton*)FindWindowById(ID_RADIOBUTTON_USE_PROFILE);
	wxASSERT(pRadioBtnUseAProfile != NULL);
	pEditProfileDescr = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_PROFILE_DESCRIPTION);
	wxASSERT(pEditProfileDescr != NULL);
	pStaticTextDescr = (wxStaticText*)FindWindowById(ID_TEXT_STATIC_DESCRIPTION);
	wxASSERT(pStaticTextDescr != NULL);

	pComboBox = (wxComboBox*)FindWindowById(ID_COMBO_PROFILE_ITEMS);
	wxASSERT(pComboBox != NULL);
	pCheckListBox = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_MENU_ITEMS);
	wxASSERT(pCheckListBox != NULL);

	tempUserProfiles = (UserProfiles*)NULL;

	// other attribute initializations
}

CAdminEditMenuProfile::~CAdminEditMenuProfile() // destructor
{
	// deallocate the memory used by our temporary objects
	m_pApp->DestroyUserProfiles(tempUserProfiles);
}

void CAdminEditMenuProfile::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	bChangesMadeToProfileItems = FALSE;
	bChangeMadeToProfileSelection = FALSE;
	tempWorkflowProfile = m_pApp->m_nWorkflowProfile;
	startingWorkflowProfile = tempWorkflowProfile;
	compareLBStr = _T("      %s   [%s]");
	
	// Deallocate the memory in the App's m_pUserProfiles and m_pAI_MenuStructure
	// These are instances of UserProfiles AI_MenuStructure that were allocated 
	// on the heap. These routines first destroy the internal items that were
	// allocated on the heap, then the top level items.
	m_pApp->DestroyUserProfiles(m_pApp->m_pUserProfiles);

	// Reread the AI_UserProfiles.xml file (this also loads the App's 
	// m_pUserProfiles data structure with latest values stored on disk).
	// Note: the AI_UserProfiles.xml file was read when the app first 
	// ran from OnInit() and the app's menus configured for those values.
	// We need to reread the AI_UserProfiles.xml each time the InitDialog()
	// is called because we change the interface dynamically and need to 
	// reload the data from the xml file in case the administrator previously
	// changed the profile during the current session (which automatically
	// saved any changes to AI_UserProfiles.xml).
	wxString readDataFrom;
	readDataFrom = _T("AI_UserProfiles.xml File");
	bool bReadOK = ReadPROFILES_XML(m_pApp->m_userProfileFileWorkFolderPath);
	if (!bReadOK)
	{
		// XML.cpp issues a Warning that AI_UserProfiles.xml could not be read
		// We'll populate the list boxes with default settings parsed from our
		// default unix-like strings
		readDataFrom = _T("Internal Default Strings (Unable to read AI_UserProfiles.xml)");
		m_pApp->SetupDefaultUserProfiles(m_pApp->m_pUserProfiles);
	}
	else
	{
		// AI_UserProfiles.xml was read successfully
		// Note: GetAndAssignIdValuesToUserProfilesStruct() needs to be called here
		// because ReadPROFILES_XML() does not call it, and it needs to be called at
		// some point after ReadPROFILES_XML().
		m_pApp->GetAndAssignIdValuesToUserProfilesStruct(m_pApp->m_pUserProfiles);
	}
	wxLogDebug(_T("Reading data from the %s"),readDataFrom.c_str());
	
	
	// Now that we've read the data (xml file or string defaults if xml file not present)
	// we can initialize some characteristics of the dialog.
	// First adjust the notebook tabs for number of profiles and labels

	// scan through the pNotebook pages and compare with data in AI_UserProfiles.xml
	int totPages = (int)pNotebook->GetPageCount();
	int totProfileNames = m_pApp->m_pUserProfiles->definedProfileNames.GetCount();
	if (totProfileNames != totPages)
	{
		// the number of pages in the wxDesigner notebook is different from the number
		// of profile names defined in the AI_UserProfiles.xml file. Make the notebook
		// agree with the incoming data from AI_UserProfiles.xml.
		if (totProfileNames > totPages)
		{
			// need to add page(s)
			int numToAdd = totProfileNames - totPages;
			int i;
			for (i = 0; i < numToAdd; i++);
			{
				wxPanel* pPage = new wxPanel(pNotebook, -1);
				MenuEditorPanelFunc(pPage, FALSE);
				pNotebook->AddPage(pPage, _T("AddedPage"));
				wxLogDebug(_T("User Workflow Profile dialog tab page %s added! [Reading data from %s]"),pNotebook->GetPageText(pNotebook->GetPageCount() - 1).c_str(),readDataFrom.c_str());
			}
		}
		else if (totProfileNames < totPages)
		{
			// need to remove page(s)
			int numToRemove = totPages - totProfileNames;
			int i;
			int lastPageIndex = pNotebook->GetPageCount() - 1;
			for (i = 0; i < numToRemove; i++);
			{
				// remove page(s) on the right
				wxLogDebug(_T("User Workflow Profile dialog tab page %s removed! [Reading data from %s]"),pNotebook->GetPageText(lastPageIndex).c_str(),readDataFrom.c_str());
				pNotebook->RemovePage(lastPageIndex);
				lastPageIndex = pNotebook->GetPageCount() - 1;
			}
		}
	}
	// We have now insured that there are the same number of tab pages as profile names in
	// AI_UserProfiles.xml.
	// Now go through and assign the names from the m_pUserProfiles->definedProfileNames array.
	int ct;
	totPages = pNotebook->GetPageCount(); // make sure this is current
	wxString existingTabLabel;
	wxString profileTabLabel;
	for (ct = 0; ct < totPages; ct++)
	{
		//wxNotebookPage* pNBPage; // unused
		existingTabLabel = pNotebook->GetPageText(ct);
		profileTabLabel = m_pApp->m_pUserProfiles->definedProfileNames.Item(ct);
		if (profileTabLabel != existingTabLabel)
		{
			pNotebook->SetPageText(ct,profileTabLabel);
			wxLogDebug(_T("User Workflow Profile dialog tab page changed from %s to %s. [Reading data from %s]"),existingTabLabel.c_str(),profileTabLabel.c_str(),readDataFrom.c_str());
		}
	}

	// To keep track of which profiles an administrator may have edited, set up an array of 
	// bool values to track which profiles have changed
	const int numProfiles = ct;
	bProfileChanged.SetCount(numProfiles,0); // initialize all array elements to zero (0)

	// Select whatever tab the administrator has set if any, first tab if none.
	if (tempWorkflowProfile < 0 || tempWorkflowProfile > (int)pNotebook->GetPageCount())
	{
		// the config file's value for the saved user workflow profile (from 
		// m_pApp->m_nWorkflowProfile) was invalid, so set the temporary value
		// to zero, i.e., the "None" profile.
		tempWorkflowProfile = 0;
		// Not likely to happen so use English message
		wxMessageBox(_T("The user workflow profile value saved in the project configuration file was out of range (%d).\nA value of 0 (= \"None\") will be used instead."),_T(""),wxICON_WARNING);
	}
	// Get the indices for the current workflow profile value
	if (tempWorkflowProfile > 0)
	{
		// The tempWorkflowProfile > 0 so tab selection index is tempWorkflowProfile - 1
		notebookTabIndex = tempWorkflowProfile - 1;
		selectedComboProfileIndex = notebookTabIndex;
		lastNotebookTabIndex = notebookTabIndex;
	}
	else
	{
		// tempWorkflowProfile is 0 ("None"), so just set the notebook tab to the first page Novice (default)
		notebookTabIndex = 0; // open the dialog with the "Novice" tab displaying if "None" selected for user profile
		selectedComboProfileIndex = notebookTabIndex;
		lastNotebookTabIndex = notebookTabIndex;
	}
	// ChangeSelection does not generate page changing events
	pNotebook->ChangeSelection(notebookTabIndex);
	
	// set the wxComboBox to its initial state from the tempWorkflowProfile copied from the config file's value
	// The Combobox index is same as the tab index, but is tempWorkflowProfile
	if (tempWorkflowProfile == 0)
	{
		// set "None" radio button
		pRadioBtnNone->SetValue(TRUE); // does not cause a wxEVT_COMMAND_RADIOBUTTON_SELECTED event to get emitted
		pRadioBtnUseAProfile->SetValue(FALSE); // " "
		// also disable the profile selection controls
		pButtonResetToFactory->Enable(FALSE);
		pEditProfileDescr->Enable(FALSE);
		pStaticTextDescr->Enable(FALSE);
		pComboBox->Enable(FALSE);
	}
	else
	{
		// set the "Use a workflow profile" radio button
		pRadioBtnNone->SetValue(FALSE); // does not cause a wxEVT_COMMAND_RADIOBUTTON_SELECTED event to get emitted
		pRadioBtnUseAProfile->SetValue(TRUE); // " "
		// also enable the profile selection controls
		pButtonResetToFactory->Enable(TRUE);
		pEditProfileDescr->Enable(TRUE);
		pStaticTextDescr->Enable(TRUE);
		pComboBox->Enable(TRUE);
		// set the combo box to the current workflow profile value
		// tempWorkflowProfile here > 0, so set selectedComboProfileIndex to tempWorkflowProfile - 1
		pComboBox->SetSelection(selectedComboProfileIndex); // this does not cause a wxEVT_COMMAND_COMBOBOX_SELECTED event
	}
		

	// create a temporary UserProfiles object for use in AdminEditMenuProfile.
	tempUserProfiles = new UserProfiles;
	// Make a temporary copy tempUserProfiles of the App's m_pUserProfiles and only work with it
	// until OnOK() is called. Only in OnOK() do we then copy the tempUserProfiles data back to
	// the App's m_pUserProfiles.
	CopyUserProfiles(m_pApp->m_pUserProfiles, tempUserProfiles);
	
	if (tempWorkflowProfile > 0)
	{
		// add the descriptionProfileText to the edit box
		pEditProfileDescr->ChangeValue(tempUserProfiles->descriptionProfileTexts.Item(selectedComboProfileIndex));
	}
	// Finally populate the list box corresponding to the notebookTabIndex
	PopulateListBox(notebookTabIndex);
}

// event handling functions

void CAdminEditMenuProfile::OnNotebookTabChanged(wxNotebookEvent& event)
{
	// This OnNotebookTabChanged is triggered during dialog creation because
	// AddPage() is called. We don't want to establish pointers to the controls
	// nor call PopulateListBox() until after the dialog creation is complete.
	if (!bCreatingDlg)
	{
		// Note: Each tab page's wxPanel has a wxCheckListBox instance with 
		// ID_CHECKLISTBOX_MENU_ITEMS as its identifier, so there are duplicate 
		// identifiers within the dialog on the heap (one for each tab panel). 
		// So we must get a handle to the newly selected page from the wxNotebookEvent 
		// passed in, and call FindWindow on it (which only finds a child of the 
		// window we are interested in).
		// The wx docs indicate that wxNotebookEvent::GetSelection() should return 
		// the newly selected page for the event triggered in the 
		// EVT_NOTEBOOK_PAGE_CHANGED macro (which we use here). We avoid using
		// wxNotebook::GetSelection() because the docs indicate that it gets either 
		// the previously or the newly selected page on some platforms, making it 
		// unreliable for our cross-platform needs.
		int eGetSel = event.GetSelection(); // event.GetSelection() gets the index of the new tab
		notebookTabIndex = eGetSel;

		// If no previous changes have been made to the "Custom" profile, present a
		// wxChoice dialog allowing the user to select another profile's visibility
		// settings as a preset for the Custom profile.
		int indexOfCustomProfile;
		indexOfCustomProfile = GetIndexOfProfileFromProfileName(_("Custom"));
		if (indexOfCustomProfile != wxNOT_FOUND 
			&& notebookTabIndex == indexOfCustomProfile
			&& !ThisUserProfilesItemsDifferFromFactory(indexOfCustomProfile))
		{
			// the Custom profile has not been changed from its factory defaults so we will
			// present a wxSingleChoiceDialog with the other (non-Custom) profile names that 
			// the administrator can use to preset the values of the Custom profile. 
			wxArrayString nonCustomProfileNamesArray;
			int ct;
			int numComboItems;
			numComboItems = pNotebook->GetPageCount();
			for (ct = 0; ct < (int)numComboItems; ct++)
			{
				wxString nbPageLabel = pNotebook->GetPageText(ct); // gets label without mnemonics
				if (nbPageLabel != _("Custom"))
				{
					nonCustomProfileNamesArray.Add(nbPageLabel);
				}

			}
			wxString msg;
			msg = _("For the Custom profile, you can copy the settings from one of the following profiles.\nThen continue customizing the profile to your liking:");
			wxSingleChoiceDialog ChooseProfileForCustomPreset(this,msg,_T("Choose a user profile to copy from"),nonCustomProfileNamesArray);
			// preselect the listed profile in the dialog representing the last tab that was 
			// selected before choosing the Custom tab
			ChooseProfileForCustomPreset.SetSelection(lastNotebookTabIndex);
			if (ChooseProfileForCustomPreset.ShowModal() == wxID_OK)
			{
				int userSelectionInt;
				wxString userSelectionStr;
				userSelectionStr = ChooseProfileForCustomPreset.GetStringSelection();
				userSelectionInt = ChooseProfileForCustomPreset.GetSelection();
				// The indices of the profile names in the dialog's list correspond to the
				// indices of the non-Custom profiles in our lists.
				// Preset the checkboxes to the selected profile
				CopyProfileVisibilityValues(userSelectionInt,indexOfCustomProfile);
			}
		}
		// whm Note: Originally I felt it would be good to allow the tab page to be 
		// changed without keeping the combobox selection in sync with it, but the 
		// more I play with it, the more I feel it would be better to keep two in
		// sync as long as the "Use a workflow profile" radio button is selected.
		// When "None" is selected, we allow the two selections to operate out of
		// sync - albeit, the combo box is disabled/grayed out when "None" is 
		// selected.
		// Place this synching here so that the previous combobox selection is still
		// visible behind the dialog.
		if (pRadioBtnUseAProfile->GetValue() == TRUE) // TRUE is selected
		{
			selectedComboProfileIndex = notebookTabIndex;
			pComboBox->SetSelection(selectedComboProfileIndex); // keep combobox in sync with tabs
			tempWorkflowProfile = selectedComboProfileIndex + 1;
		}
		
		PopulateListBox(notebookTabIndex);
		
		// Update lastNotebookTabIndex here just before leaving and not before the possible
		// call of the dialog. Reason: the actual lastNotebookTabIndex may have been needed 
		// above for the dialog.
		lastNotebookTabIndex = notebookTabIndex;  
	}
}
	
void CAdminEditMenuProfile::OnComboBoxSelection(wxCommandEvent& WXUNUSED(event))
{
	int newSel = pComboBox->GetSelection();
	if (selectedComboProfileIndex != newSel)
	{
		selectedComboProfileIndex = newSel;
		// We change the notebook tab selection to agree with the combobox selection
		notebookTabIndex = selectedComboProfileIndex;
		// The tempWorkflowProfile is the selectedComboProfileIndex + 1
		tempWorkflowProfile = selectedComboProfileIndex + 1;
		pNotebook->SetSelection(notebookTabIndex);
		// Note the SetSelection() call above generates a page changing event which 
		// triggers the OnNotebookTabChanged() handler which in turn calls
		// PopulateListBox() to update the displayed list items.
		// put the "Description of Selected User Profile" in the edit box
		pEditProfileDescr->ChangeValue(tempUserProfiles->descriptionProfileTexts.Item(selectedComboProfileIndex));
	}
}

void CAdminEditMenuProfile::OnCheckListBoxToggle(wxCommandEvent& event)
{
	// First don't allow the menu category labels in the list be
	// unchecked.
	// TODO: Determine why this doesn't seem to work on the Mac
	int ct;
	int indexOfClickedItem;
	indexOfClickedItem = event.GetInt();
	for (ct = 0; ct < (int)itemsAlwaysChecked.GetCount(); ct++)
	{
		if (indexOfClickedItem == itemsAlwaysChecked.Item(ct))
		{
			pCheckListBox->Check(indexOfClickedItem,true);
		}
	}

	wxString lbItemStr;
	lbItemStr = pCheckListBox->GetString(indexOfClickedItem);

	// Process any user changes to check boxes and store changes
	// in the tempUserProfiles struct on the heap; any changes are
	// only saved if user clicks on OK button invoking the OnOK()
	// handler.
	if (tempUserProfiles != NULL)
	{
		ProfileItemList::Node* posTemp;
		int count;
		int item_count = tempUserProfiles->profileItemList.GetCount();
		wxString tempStr;
		for(count = 0; count < item_count; count++)
		{
			posTemp = tempUserProfiles->profileItemList.Item(count);
			UserProfileItem* pProfileItem;
			pProfileItem = posTemp->GetData();
			tempStr = compareLBStr.Format(compareLBStr,pProfileItem->itemText.c_str(),pProfileItem->itemDescr.c_str());
			if (tempStr == lbItemStr)
			{
				// We are at the pProfileItem matching the list box profile item representation
				// We calculate the index of the usedVisibilityValues array according to the pNotebook
				// tab that is currently selected, i.e., "Novice" is 0, "Custom 1" is 1, "Custom 2" is 2.
				int nIndex;
				bool bItemChecked;
				nIndex = pNotebook->GetSelection();
				// nIndex is also the index into the usedVisibilityValues array
				bItemChecked = pCheckListBox->IsChecked(indexOfClickedItem);
				if (bItemChecked)
				{
					pProfileItem->usedVisibilityValues[nIndex] = _T("1");
				}
				else
				{
					pProfileItem->usedVisibilityValues[nIndex] = _T("0");
				}
			}
		}
	}
}

void CAdminEditMenuProfile::OnCheckListBoxDblClick(wxCommandEvent& WXUNUSED(event))
{
	// TODO: Fix/Eliminate the problem of the listbox (that is scrolled down) 
	// scrolling back to make a selected item visible when when a check box is 
	// double clicked. This behavior doesn't happen in other wxCheckListBoxes 
	// that exist in AI, so there should be some discoverable reason why it 
	// happens in this one.
	// Note: The scroll-back action seems to occur before entry to this DblClick
	// handler.
	;
}
	
void CAdminEditMenuProfile::OnRadioNone(wxCommandEvent& WXUNUSED(event))
{
	// set "None" radio button
	pRadioBtnNone->SetValue(TRUE); // does not cause a wxEVT_COMMAND_RADIOBUTTON_SELECTED event to get emitted
	pRadioBtnUseAProfile->SetValue(FALSE); // " "
	// also disable the profile selection controls
	pButtonResetToFactory->Enable(FALSE);
	pEditProfileDescr->Enable(FALSE);
	pStaticTextDescr->Enable(FALSE);
	pComboBox->Enable(FALSE);
	// Although the user has set the "None" radio button we will leave whatever tab 
	// that was showing to continue to be shown while the "Use a profile" stuff is 
	// disabled. When a notebook tab is selected we adjust the radio buttons and/or 
	// combo box accordingly, but not the reverse.
	// Set the tempWorkflowProfile value here to zero.
	tempWorkflowProfile = 0;
	// We don't change the notebook tab index nor do we change the combobox index.
}
	
void CAdminEditMenuProfile::OnRadioUseProfile(wxCommandEvent& WXUNUSED(event))
{
	// set the "Use a workflow profile" radio button
	pRadioBtnNone->SetValue(FALSE); // does not cause a wxEVT_COMMAND_RADIOBUTTON_SELECTED event to get emitted
	pRadioBtnUseAProfile->SetValue(TRUE); // " "
	// also enable the profile selection controls
	pButtonResetToFactory->Enable(TRUE);
	pEditProfileDescr->Enable(TRUE);
	pStaticTextDescr->Enable(TRUE);
	pComboBox->Enable(TRUE);
	// Use whatever previous value selectedComboProfileIndex had
	pComboBox->SetSelection(selectedComboProfileIndex); // this does not cause a wxEVT_COMMAND_COMBOBOX_SELECTED event
	// Make the corresponding notebook tab appear too, since pComboBox->SetSelection does not cause it to change
	notebookTabIndex = selectedComboProfileIndex;
	pNotebook->SetSelection(notebookTabIndex);
}

// This function populates the listbox that is on the newTabIndex page of the wxNotebook
void CAdminEditMenuProfile::PopulateListBox(int newTabIndex)
{
	// Note: Each time a wxNotebook tab changes, a different wxPanel appears that
	// contains different actual controls (with different pointers to their
	// instances on the heap) for the wxNotebook and wxComboBox. Therefore, the 
	// pointers to these controls need to be reestablished each time a Notebook 
	// tab is changed (which brings a different panel and controls into view). 
	
	// The pNotebook, pComboBox and pOKButton pointers shouldn't actually change, but 
	// it won't hurt to get them again here
	pNotebook = (wxNotebook*)FindWindowById(ID_MENU_EDITOR_NOTEBOOK);
	wxASSERT(pNotebook != NULL);
	pComboBox = (wxComboBox*)FindWindowById(ID_COMBO_PROFILE_ITEMS);
	wxASSERT(pComboBox != NULL);
	pOKButton = (wxButton*)FindWindowById(wxID_OK);
	wxASSERT(pOKButton != NULL);
	
	// Note: Each tab page's wxPanel has a wxCheckListBox instance with 
	// ID_CHECKLISTBOX_MENU_ITEMS as its identifier, so there are duplicate 
	// identifiers within the dialog on the heap (one for each tab panel). 
	// So we must get a handle to the newly selected page from the wxNotebookEvent 
	// passed in, and call FindWindow on it (which only finds a child of the 
	// window we are interested in).
	// The wx docs indicate that wxNotebookEvent::GetSelection() should return 
	// the newly selected page for the event triggered in the 
	// EVT_NOTEBOOK_PAGE_CHANGED macro (which we use here). We avoid using
	// wxNotebook::GetSelection() because the docs indicate that it gets either 
	// the previously or the newly selected page on some platforms, making it 
	// unreliable for our cross-platform needs.
	//int eGetSel = event.GetSelection(); // 
	pCheckListBox = (wxCheckListBox*)pNotebook->GetPage(newTabIndex)->FindWindow(ID_CHECKLISTBOX_MENU_ITEMS);
	wxASSERT(pCheckListBox != NULL);

	// populate the pCheckListBox with data from m_pAI_MenuStructure
	pCheckListBox->Clear(); // remove any previous items
	itemsAlwaysChecked.Clear(); 

	wxASSERT(m_pApp->m_pAI_MenuStructure != NULL);
	wxASSERT(tempUserProfiles != NULL);
	// Since we have internal strings to use in creation of tempUserProfiles in case the xml file 
	// is not found, and we create m_pAI_MenuStructure from a temporary default menu bar in 
	// SetupDefaultMenuStructure(), the following test should never succeed, but just in case
	// I've coded it anyway. A non-localized English message here is ok.
	if (m_pApp->m_pAI_MenuStructure == NULL || tempUserProfiles == NULL)
	{
		pCheckListBox->Append(_T("[Warning: No User Workflow Profile data is available for display -"));
		pCheckListBox->Append(_T("you cannot use this dialog to change user workflow profiles until"));
		pCheckListBox->Append(_T("a valid AI_UserProfiles.xml file is installed in your work folder]"));
		pCheckListBox->Enable(FALSE); // disable the checklistbox
		pComboBox->Enable(FALSE); // disable the combo box
		pOKButton = (wxButton*)FindWindowById(wxID_OK);
		wxASSERT(pOKButton != NULL);
		pOKButton->Enable(FALSE); // don't allow changes to be made via the OK button - only option is Cancel
		return;
	}

	// handle menu items first in list
	int lbIndexOfInsertion;
	lbIndexOfInsertion = pCheckListBox->Append(_("Adapt It Menu Items:"));
	pCheckListBox->Check(lbIndexOfInsertion);
	itemsAlwaysChecked.Add(lbIndexOfInsertion);
	// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
	
	// to load the listbox with menu items, we scan through our m_pAI_MenuStructure
	// object, and examine the mainMenuLabel sections corresponding to the top level
	// menus ("File" "Edit" "View" "Tools" etc). We list items in which the
	// ProfileItemIsSubMenuOfThisMainMenu() is TRUE, and for which the 
	// adminCanChange attribute is set to _T("1"), and the usedVisibilityValues
	// array for the given profile is also set to _T("1").
	wxString mainMenuLabel = _T("");
	wxString prevMainMenuLabel = _T("");
	wxString tempStr;
	MainMenuItemList::Node* mmNode;
	AI_MainMenuItem* pMainMenuItem;
	int ct;
	int nMainMenuItems = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.GetCount();
	for (ct = 0; ct < nMainMenuItems; ct++)
	{
		mmNode = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.Item(ct);
		pMainMenuItem = mmNode->GetData();
		mainMenuLabel = pMainMenuItem->mainMenuLabel;
		// remove any & in the mainMenuLabel
		wxString mmLabel;
		mmLabel = mainMenuLabel;
		mmLabel = m_pApp->RemoveMenuLabelDecorations(mmLabel); // removes any & chars for comparison

		if (mmLabel != prevMainMenuLabel)
		{
			// this is a new main menu label, add it to the listbox
			lbIndexOfInsertion = pCheckListBox->Append(_T("   \"") + mmLabel + _T("\" Menu"));
			pCheckListBox->Check(lbIndexOfInsertion);
			itemsAlwaysChecked.Add(lbIndexOfInsertion);
			// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
			//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
			//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
		}

		// now scan through tempUserProfiles->profileItemList and load profile items
		// that match this main menu label
		ProfileItemList::Node* piNode;
		UserProfileItem* pUserProfileItem;
		int ct_pi;
		int numItemsLoaded = 0;
		int lbIndx;
		int nProfileItems = tempUserProfiles->profileItemList.GetCount();
		for (ct_pi = 0; ct_pi < nProfileItems; ct_pi++)
		{
			piNode = tempUserProfiles->profileItemList.Item(ct_pi);
			pUserProfileItem = piNode->GetData();
			if (ProfileItemIsSubMenuOfThisMainMenu(pUserProfileItem,mmLabel))
			{
				// we only display in the list box items that the administrator can change
				if (pUserProfileItem->adminCanChange == _T("1"))
				{
					tempStr = compareLBStr.Format(compareLBStr,pUserProfileItem->itemText.c_str(),pUserProfileItem->itemDescr.c_str());
					lbIndx = pCheckListBox->Append(tempStr);
					numItemsLoaded++;
					if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
						pCheckListBox->Check(lbIndx,true);
					else
						pCheckListBox->Check(lbIndx,false);
				}
			}
		}
		if (numItemsLoaded == 0)
		{
			// no items were loaded for this main menu so remove the mainMenuLabel, i.e., for the "&Help" menu
			pCheckListBox->Delete(lbIndexOfInsertion);
		}
		prevMainMenuLabel = mmLabel;
	}

	// Next, handle preferences tab pages in the listbox
	lbIndexOfInsertion = pCheckListBox->Append(_("Adapt It Preferences Tab Pages:"));
	pCheckListBox->Check(lbIndexOfInsertion);
	itemsAlwaysChecked.Add(lbIndexOfInsertion);
	// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
	ProfileItemList::Node* piNode;
	UserProfileItem* pUserProfileItem;
	int lbIndx;
	int numItemsLoaded = 0;
	int nProfItemCount = tempUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = tempUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("preferencesTab"))
		{
			tempStr = compareLBStr.Format(compareLBStr,pUserProfileItem->itemText.c_str(),pUserProfileItem->itemDescr.c_str());
			lbIndx = pCheckListBox->Append(tempStr);
			numItemsLoaded++;
			if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
				pCheckListBox->Check(lbIndx,true);
			else
				pCheckListBox->Check(lbIndx,false);
		}
	}

	// Next, handle modebar items in the listbox
	lbIndexOfInsertion = pCheckListBox->Append(_("Adapt It Modebar Items:"));
	pCheckListBox->Check(lbIndexOfInsertion);
	itemsAlwaysChecked.Add(lbIndexOfInsertion);
	// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
	numItemsLoaded = 0;
	nProfItemCount = tempUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = tempUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("modeBar"))
		{
			tempStr = compareLBStr.Format(compareLBStr,pUserProfileItem->itemText.c_str(),pUserProfileItem->itemDescr.c_str());
			lbIndx = pCheckListBox->Append(tempStr);
			numItemsLoaded++;
			if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
				pCheckListBox->Check(lbIndx,true);
			else
				pCheckListBox->Check(lbIndx,false);
		}
	}
	
	// Next, handle Toolbar items in the listbox
	lbIndexOfInsertion = pCheckListBox->Append(_("Adapt It Toolbar Items:"));
	pCheckListBox->Check(lbIndexOfInsertion);
	itemsAlwaysChecked.Add(lbIndexOfInsertion);
	// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
	numItemsLoaded = 0;
	nProfItemCount = tempUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = tempUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("toolBar"))
		{
			tempStr = compareLBStr.Format(compareLBStr,pUserProfileItem->itemText.c_str(),pUserProfileItem->itemDescr.c_str());
			lbIndx = pCheckListBox->Append(tempStr);
			numItemsLoaded++;
			if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
				pCheckListBox->Check(lbIndx,true);
			else
				pCheckListBox->Check(lbIndx,false);
		}
	}
	
	// Next, handle wizardListItem items in the listbox
	lbIndexOfInsertion = pCheckListBox->Append(_("Adapt It Wizard List Item:"));
	pCheckListBox->Check(lbIndexOfInsertion);
	itemsAlwaysChecked.Add(lbIndexOfInsertion);
	// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
	//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
	numItemsLoaded = 0;
	nProfItemCount = tempUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = tempUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("wizardListItem"))
		{
			tempStr = compareLBStr.Format(compareLBStr,pUserProfileItem->itemText.c_str(),pUserProfileItem->itemDescr.c_str());
			lbIndx = pCheckListBox->Append(tempStr);
			numItemsLoaded++;
			if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
				pCheckListBox->Check(lbIndx,true);
			else
				pCheckListBox->Check(lbIndx,false);
		}
	}
}

/*
// Copies the menu structure of a pFromMenuStructure instance to a pToMenuStructure instance, copying
// all of the internal attributes and lists.
// Note: Assumes that pFromMenuStructure and pToMenuStructure were created on the heap before this
// function was called.
void CAdminEditMenuProfile::CopyMenuStructure(const AI_MenuStructure* pFromMenuStructure, AI_MenuStructure*& pToMenuStructure)
{
	wxASSERT(pFromMenuStructure != NULL);
	wxASSERT(pToMenuStructure != NULL);
	int ct;
	int totct;
	totct = pFromMenuStructure->aiMainMenuItems.GetCount();
	for (ct = 0; ct < totct; ct++)
	{
		AI_MainMenuItem* pFromMainMenuItem;
		MainMenuItemList::Node* node;
		node = pFromMenuStructure->aiMainMenuItems.Item(ct);
		pFromMainMenuItem = node->GetData();
		AI_MainMenuItem* pToMainMenuItem;
		pToMainMenuItem = new AI_MainMenuItem;
		wxASSERT(pToMainMenuItem != NULL);
		int i;
		int ict;
		ict = pFromMainMenuItem->aiSubMenuItems.GetCount();
		for (i = 0; i < ict; i++)
		{
			AI_SubMenuItem* pFromSubMenuItem;
			AI_SubMenuItem* pToSubMenuItem;
			SubMenuItemList::Node* subNode;
			subNode = pFromMainMenuItem->aiSubMenuItems.Item(i);
			pFromSubMenuItem = subNode->GetData();
			pToSubMenuItem = new AI_SubMenuItem;
			wxASSERT(pToSubMenuItem != NULL);
			pToSubMenuItem->subMenuHelp = pFromSubMenuItem->subMenuHelp;
			pToSubMenuItem->subMenuIDint = pFromSubMenuItem->subMenuIDint;
			pToSubMenuItem->subMenuKind = pFromSubMenuItem->subMenuKind;
			pToSubMenuItem->subMenuLabel = pFromSubMenuItem->subMenuLabel;
			pToMainMenuItem->mainMenuIDint = pFromMainMenuItem->mainMenuIDint;
			pToMainMenuItem->mainMenuLabel = pFromMainMenuItem->mainMenuLabel;
			pToMainMenuItem->aiSubMenuItems.Append(pToSubMenuItem);
		}
		pToMenuStructure->aiMainMenuItems.Append(pToMainMenuItem);
	}
}
*/

// "Deep" copies the UserProfiles of a pFromUserProfiles instance to a pToUserProfiles instance, copying
// all of the internal attributes and lists.
// Note: Assumes that pFromUserProfiles and pToUserProfiles were created on the heap before this
// function was called and that all member counts are identical
void CAdminEditMenuProfile::CopyUserProfiles(const UserProfiles* pFromUserProfiles, UserProfiles*& pToUserProfiles)
{
	wxASSERT(pFromUserProfiles != NULL);
	wxASSERT(pToUserProfiles != NULL);

	if (pFromUserProfiles != NULL)
	{
		pToUserProfiles->profileVersion = pFromUserProfiles->profileVersion;
		
		int count;
		int totPNamesFrom = (int)pFromUserProfiles->definedProfileNames.GetCount();
		int totPNamesTo = (int)pToUserProfiles->definedProfileNames.GetCount();
		wxASSERT(totPNamesTo <= totPNamesFrom);
		if (totPNamesTo < totPNamesFrom)
		{
			for (count = 0; count < totPNamesFrom - totPNamesTo; count++)
			{
				// insure the pToUserProfiles->definedProfileNames has the same number of array elements
				pToUserProfiles->definedProfileNames.Add(_T("")); // the added string elements will get copied below
			}
		}
		totPNamesFrom = (int)pFromUserProfiles->definedProfileNames.GetCount();
		totPNamesTo = (int)pToUserProfiles->definedProfileNames.GetCount();
		wxASSERT(totPNamesFrom == totPNamesTo);
		// now copy the definedProfileNames elements
		for (count = 0; count < totPNamesFrom; count++)
		{
			pToUserProfiles->definedProfileNames[count] = pFromUserProfiles->definedProfileNames[count];
		}
		totPNamesFrom = (int)pFromUserProfiles->descriptionProfileTexts.GetCount();
		totPNamesTo = (int)pToUserProfiles->descriptionProfileTexts.GetCount();
		wxASSERT(totPNamesTo <= totPNamesFrom);
		if (totPNamesTo < totPNamesFrom)
		{
			for (count = 0; count < totPNamesFrom - totPNamesTo; count ++)
			{
				// insure the pToUserProfiles->descriptionProfileTexts has the same number of array elements
				pToUserProfiles->descriptionProfileTexts.Add(_T("")); // the added string elements will get copied below
			}
		}
		totPNamesFrom = (int)pFromUserProfiles->descriptionProfileTexts.GetCount();
		totPNamesTo = (int)pToUserProfiles->descriptionProfileTexts.GetCount();
		wxASSERT(totPNamesFrom == totPNamesTo);
		for (count = 0; count < totPNamesFrom; count++)
		{
			pToUserProfiles->descriptionProfileTexts[count] = pFromUserProfiles->descriptionProfileTexts[count];
		}
		ProfileItemList::Node* Frompos;
		ProfileItemList::Node* Topos;
		int item_countFrom = pFromUserProfiles->profileItemList.GetCount();
		int item_countTo = pToUserProfiles->profileItemList.GetCount();
		wxASSERT(item_countTo <= item_countFrom);
		if (item_countTo < item_countFrom)
		{
			for (count = 0; count < item_countFrom - item_countTo; count++)
			{
				// add new objects to profileItemList
				UserProfileItem* pToItem;
				pToItem = new UserProfileItem;
				pToItem->adminCanChange = _T("");
				pToItem->itemDescr = _T("");
				pToItem->itemID = _T("");
				pToItem->itemIDint = -1;
				pToItem->itemText = _T("");
				pToItem->itemType = _T("");
				pToItem->usedFactoryValues.Clear();
				pToItem->usedProfileNames.Clear();
				pToItem->usedVisibilityValues.Clear();
				pToUserProfiles->profileItemList.Append(pToItem);
			}
		}
		item_countFrom = (int)pFromUserProfiles->profileItemList.GetCount();
		item_countTo = (int)pToUserProfiles->profileItemList.GetCount();
		wxASSERT(item_countFrom == item_countTo);
		for(count = 0; count < item_countFrom; count++)
		{
			Frompos = pFromUserProfiles->profileItemList.Item(count);
			Topos = pToUserProfiles->profileItemList.Item(count);
			UserProfileItem* pFromItem;
			UserProfileItem* pToItem;
			pFromItem = Frompos->GetData();
			wxASSERT(pFromItem != NULL);
			pToItem = Topos->GetData();
			wxASSERT(pToItem != NULL);
			pToItem->adminCanChange = pFromItem->adminCanChange;
			pToItem->itemDescr = pFromItem->itemDescr;
			pToItem->itemID = pFromItem->itemID;
			pToItem->itemIDint = pFromItem->itemIDint;
			pToItem->itemText = pFromItem->itemText;
			pToItem->itemType = pFromItem->itemType;
			int ct;
			int totCtFrom,totCtTo;
			totCtFrom = (int)pFromItem->usedFactoryValues.GetCount();
			totCtTo = (int)pToItem->usedFactoryValues.GetCount();
			wxASSERT(totCtTo <= totCtFrom);
			for (ct = 0; ct < totCtFrom - totCtTo; ct++)
			{
				pToItem->usedFactoryValues.Add(_T(""));
			}
			totCtFrom = (int)pFromItem->usedFactoryValues.GetCount();
			totCtTo = (int)pToItem->usedFactoryValues.GetCount();
			wxASSERT(totCtFrom == totCtTo);
			for (ct = 0; ct < totCtFrom; ct++)
			{
				pToItem->usedFactoryValues[ct] = pFromItem->usedFactoryValues[ct];
			}
			totCtFrom = (int)pFromItem->usedProfileNames.GetCount();
			totCtTo = (int)pToItem->usedProfileNames.GetCount();
			wxASSERT(totCtTo <= totCtFrom);
			for (ct = 0; ct < totCtFrom - totCtTo; ct++)
			{
				pToItem->usedProfileNames.Add(_T(""));
			}
			totCtFrom = (int)pFromItem->usedProfileNames.GetCount();
			totCtTo = (int)pToItem->usedProfileNames.GetCount();
			wxASSERT(totCtFrom == totCtTo);
			for (ct = 0; ct < totCtFrom; ct++)
			{
				pToItem->usedProfileNames[ct] = pFromItem->usedProfileNames[ct];
			}
			totCtFrom = (int)pFromItem->usedVisibilityValues.GetCount();
			totCtTo = (int)pToItem->usedVisibilityValues.GetCount();
			wxASSERT(totCtTo <= totCtFrom);
			for (ct = 0; ct < totCtFrom - totCtTo; ct++)
			{
				pToItem->usedVisibilityValues.Add(_T(""));
			}
			totCtFrom = (int)pFromItem->usedVisibilityValues.GetCount();
			totCtTo = (int)pToItem->usedVisibilityValues.GetCount();
			wxASSERT(totCtFrom == totCtTo);
			for (ct = 0; ct < totCtFrom; ct++)
			{
				pToItem->usedVisibilityValues[ct] = pFromItem->usedVisibilityValues[ct];
			}
		}
	}
}

// Determines if the menu item represented by the input pUserProfileItem belongs
// as a subMenu within the input top level mainMenuLabel of the AI menu structure
// according to the current information stored in the m_pAI_MenuStructure object.
// Used in PopulateListBox().
bool CAdminEditMenuProfile::ProfileItemIsSubMenuOfThisMainMenu(UserProfileItem* pUserProfileItem, wxString mmLabel)
{
	// We scan the lists of aiMainMenuItems and aiSubMenuItems until we locate the 
	// corresponding subMenuID, and determine if the aiMainMenuItems' mainMenuLabel 
	// matches our input parameter. If a match is found break out and return TRUE, 
	// otherwise return FALSE.
	
	// do a reality check
	wxASSERT(pUserProfileItem != NULL);
	wxASSERT(m_pApp->m_pAI_MenuStructure != NULL);
	if (pUserProfileItem == NULL || m_pApp->m_pAI_MenuStructure == NULL)
	{
		return FALSE;
	}
	int profileItemIDint;
	profileItemIDint = pUserProfileItem->itemIDint;

	
	wxString menuLabel;
	MainMenuItemList::Node* mmNode;
	AI_MainMenuItem* pMainMenuItem;
	int ct;
	int nMainMenuItems = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.GetCount();
	for (ct = 0; ct < nMainMenuItems; ct++)
	{
		mmNode = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.Item(ct);
		pMainMenuItem = mmNode->GetData();
		menuLabel = pMainMenuItem->mainMenuLabel;
		menuLabel = m_pApp->RemoveMenuLabelDecorations(menuLabel); // removes any & chars for comparison
		SubMenuItemList::Node* smNode;
		AI_SubMenuItem* pSubMenuItem;
		int ct_sm;
		int nSubMenuItems = pMainMenuItem->aiSubMenuItems.GetCount();
		for (ct_sm = 0; ct_sm < nSubMenuItems; ct_sm++)
		{
			smNode = pMainMenuItem->aiSubMenuItems.Item(ct_sm);
			pSubMenuItem = smNode->GetData();
			if (pSubMenuItem->subMenuIDint == profileItemIDint && menuLabel == mmLabel)
			{
				return TRUE;
			}
		}
	}
	// if we get here we did not find a match
	return FALSE;
}

// Determines if the sub menu with itemText is currently a menu item within
// Adapt It's current menu bar. Currently Unused but may be handy later.
bool CAdminEditMenuProfile::SubMenuIsInCurrentAIMenuBar(wxString itemText)
{
	wxMenuBar* pMenuBar;
	pMenuBar = m_pApp->GetMainFrame()->m_pMenuBar;
	wxString mainMenuText;
	int menuCount = pMenuBar->GetMenuCount();
	int menuIndex;
	int ct;
	for (ct = 0; ct < menuCount; ct++)
	{
		mainMenuText = pMenuBar->GetMenuLabel(ct); // includes accelerator chars
		menuIndex = pMenuBar->FindMenuItem(mainMenuText,itemText);
		if (!wxNOT_FOUND)
		{
			return TRUE;
		}
	}

	return FALSE;
}


// Used in OnOK().
bool CAdminEditMenuProfile::UserProfileItemsHaveChanged(const UserProfiles* tempUserProfiles, const UserProfiles* appUserProfiles)
{
	bool bChanged = FALSE;
	wxASSERT(tempUserProfiles != NULL);
	wxASSERT(appUserProfiles != NULL);
	wxASSERT(tempUserProfiles->definedProfileNames.GetCount() == appUserProfiles->definedProfileNames.GetCount());
	wxASSERT(tempUserProfiles->descriptionProfileTexts.GetCount() == appUserProfiles->descriptionProfileTexts.GetCount());
	wxASSERT(tempUserProfiles->profileItemList.GetCount() == appUserProfiles->profileItemList.GetCount());
	wxASSERT(tempUserProfiles->profileVersion == appUserProfiles->profileVersion);

	// We only need to check for changes between tempUserProfiles and appUserProfiles, checking for changes
	// in their usedVisibilityValues arrays.
	if (tempUserProfiles != NULL && appUserProfiles != NULL)
	{
		ProfileItemList::Node* posTemp;
		ProfileItemList::Node* posApp;
		int count;
		int item_count = appUserProfiles->profileItemList.GetCount();
		for(count = 0; count < item_count; count++)
		{
			posTemp = tempUserProfiles->profileItemList.Item(count);
			posApp = appUserProfiles->profileItemList.Item(count);
			UserProfileItem* pTempItem;
			UserProfileItem* pAppItem;
			pTempItem = posTemp->GetData();
			pAppItem = posApp->GetData();
			int ct;
			int totCt;
			totCt = pTempItem->usedVisibilityValues.GetCount();
			for (ct = 0; ct < totCt; ct++)
			{
				if (pTempItem->usedVisibilityValues.Item(ct) != pAppItem->usedVisibilityValues.Item(ct))
				{
					bChanged = TRUE;
					bProfileChanged[ct] = 1;
					// don't break here because we want the bProfileChanged array to account
					// for any and all profiles that may have been edited/changed 
				}
			}
		}
	}
	return bChanged;
}

bool CAdminEditMenuProfile::ThisUserProfilesItemsDifferFromFactory(int profileSelectionIndex)
{
	bool bChanged = FALSE;
	wxASSERT(tempUserProfiles != NULL);

	// We only need to check for differences in usedVisibilityValues compared 
	// with usedFactoryValues for the given profileSelectionIndex.
	if (tempUserProfiles != NULL)
	{
		ProfileItemList::Node* posTemp;
		int count;
		int item_count = tempUserProfiles->profileItemList.GetCount();
		for(count = 0; count < item_count; count++)
		{
			posTemp = tempUserProfiles->profileItemList.Item(count);
			UserProfileItem* pTempItem;
			pTempItem = posTemp->GetData();
			if (pTempItem->usedFactoryValues.Item(profileSelectionIndex) != pTempItem->usedVisibilityValues.Item(profileSelectionIndex))
			{
				bChanged = TRUE;
				break;
			}
		}
	}
	return bChanged;
}


bool CAdminEditMenuProfile::UserProfileSelectionHasChanged()
{
	if (tempWorkflowProfile != m_pApp->m_nWorkflowProfile)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// Reset the itemVisibility values for the currently selected profile 
// back to their factory default values.
void CAdminEditMenuProfile::OnBtnResetToFactory(wxCommandEvent& WXUNUSED(event))
{
	// Scan through the tempUserProfiles and, for the currently selected 
	// profile, change the UserProfileItem's userVisibility values back to
	// their usedFactoryValues.
	ProfileItemList::Node* piNode;
	UserProfileItem* pUserProfileItem;
	int ct_pi;
	int nProfileItems = tempUserProfiles->profileItemList.GetCount();
	for (ct_pi = 0; ct_pi < nProfileItems; ct_pi++)
	{
		piNode = tempUserProfiles->profileItemList.Item(ct_pi);
		pUserProfileItem = piNode->GetData();
		int tot;
		int ct;
		tot = pUserProfileItem->usedVisibilityValues.GetCount();
		wxASSERT(tot == (int)pUserProfileItem->usedFactoryValues.GetCount());
		// assign the usedFactoryValues values back to the usedVisibilityValues
		for (ct = 0; ct < tot; ct++)
		{
			if (pUserProfileItem->usedVisibilityValues[ct] != pUserProfileItem->usedFactoryValues[ct])
			{
				pUserProfileItem->usedVisibilityValues[ct] = pUserProfileItem->usedFactoryValues[ct];
				// also note which profile changed in this process
				bProfileChanged[ct] = 1;
			}
		}
	}
	// reload the listbox
	PopulateListBox(notebookTabIndex);
}

void CAdminEditMenuProfile::OnUpdateBtnResetToFactory(wxUpdateUIEvent& WXUNUSED(event))
{
	wxASSERT(pButtonResetToFactory != NULL);
	wxASSERT(pRadioBtnUseAProfile != NULL);

	if (pRadioBtnUseAProfile->GetValue() == TRUE)
	{
		if (ThisUserProfilesItemsDifferFromFactory(selectedComboProfileIndex))
		{
			if (pButtonResetToFactory != NULL && !pButtonResetToFactory->IsEnabled())
			{
				pButtonResetToFactory->Enable(TRUE);
			}
		}
		else
		{
			if (pButtonResetToFactory != NULL && pButtonResetToFactory->IsEnabled())
			{
				pButtonResetToFactory->Enable(FALSE);
			}
		}
	}
	else
	{
		if (pButtonResetToFactory != NULL)
		{
			pButtonResetToFactory->Enable(FALSE);
		}
	}
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CAdminEditMenuProfile::OnOK(wxCommandEvent& event) 
{
	// save the previous work profile name
	wxString previousProfileName;
	previousProfileName = GetNameOfProfileFromProfileValue(startingWorkflowProfile); // returns _("None) when tempWorkflowProfile == 0
	
	// Check the radio buttons for the final value to assign to the 
	// m_pApp->m_nWorkflowProfile member. The tempWorkflowProfile should
	// already be set, but getting the values from the UI gets what the
	// user sees in case we've not properly tracked tempWorkflowProfile.
	if (pRadioBtnNone->GetValue() == TRUE)
	{
		tempWorkflowProfile = 0;
	}
	else
	{
		// get the combo box value selection
		int sel;
		sel = pComboBox->GetSelection();
		tempWorkflowProfile = sel + 1; // the saved profile is one greater than the combobox selection
	}

	// get the new work profile name
	wxString newProfileName;
	newProfileName = GetNameOfProfileFromProfileValue(tempWorkflowProfile); // returns _("None) when tempWorkflowProfile == 0

	// update the App member for config file updating on next save
	if (UserProfileSelectionHasChanged())
	{
		bChangeMadeToProfileSelection = TRUE;
		m_pApp->m_nWorkflowProfile = tempWorkflowProfile;
	}
	if (UserProfileItemsHaveChanged(tempUserProfiles, m_pApp->m_pUserProfiles))
	{
		bChangesMadeToProfileItems = TRUE;
		// Copy any changes made in the user workflow profiles to the App's
		// member.
		// Notify the user if changes were made in a profile other than the
		// profile that is selected here on OnOK().
		for (int i=0; i < (int)bProfileChanged.GetCount(); i++)
		{
			wxLogDebug(_T("bProfileChanged array element %d = %d"),i,bProfileChanged.Item(i));
		}
		wxString editedProfilesStr;
		editedProfilesStr = GetNamesOfEditedProfiles();
		wxString msg1 = _("You made changes in the following work profile(s):");
		wxString msg2a = _("However, you did not make any changes in the currently selected user profile which is %s.");
		wxString msg2b = _("However, you changed the user profile selection from %s to %s.");
		wxString msg2c = _("However, the user profile selection remains %s.");
		wxString msg3 = _("Are you sure you want to use the %s work profile?");
		wxString msg4 = _("Click \"Yes\" to use the %s profile (and also save the other profile's changes).\nClick \"No\" to continue editing in the User Workflow Profile dialog.");
		wxString msg; // the final composed string
		int response = -1;
		if (tempWorkflowProfile == 0)
		{
			msg = msg1; // _("You made changes in the following work profile(s):");
			msg += _T("\n   %s\n");
			// the "None" profile is selected
			if (startingWorkflowProfile != tempWorkflowProfile)
			{
				// The user switch from some other profile to "None" in this session and
				// he also made changes to some other profile items - make query!
				msg += msg2b; // _("However, you changed the user profile selection from (%s) to (%s).");
				msg += _T("\n");
				msg += msg3; // _("Are you sure you want to use the %s work profile?");
				msg += _T("\n");
				msg += msg4; // _("Click \"Yes\" to use the %s profile (and save the other profile's changes).\nClick \"No\" to continue editing in the User Workflow Profile dialog.");
				msg = msg.Format(msg,editedProfilesStr.c_str(),previousProfileName.c_str(),newProfileName.c_str(),newProfileName.c_str(),newProfileName.c_str());
			}
			else
			{
				// the selection remains "None", but user made changes to another profile's items
				// - make query!
				msg += msg2c; // _("However, the user profile selection remains %s.");
				msg += _T("\n");
				msg += msg3; // _("Are you sure you want to use the %s work profile?");
				msg += _T("\n");
				msg += msg4; // _("Click \"Yes\" to use the %s profile (and save the other profile's changes).\nClick \"No\" to continue editing in the User Workflow Profile dialog.");
				msg = msg.Format(msg,editedProfilesStr.c_str(),previousProfileName.c_str(),previousProfileName.c_str(),previousProfileName.c_str()); // in this case oldProfileName == newProfileName
			}
			response = wxMessageBox(msg,_T(""),wxICON_QUESTION | wxYES_NO);
		}
		else if (bProfileChanged.Item(tempWorkflowProfile - 1) != 1) // when tempWorkflowProfile is other than 0, the profile index is 1 less
		{
			// Something other than "None" is currently selected as profile and changes were 
			// detected in one or more of those other profiles. The user may have intended to 
			// select one of those other profiles before clicking OK, but we aren't sure, so 
			// we should - make query!
			msg = msg1; // _("You made changes in the following work profile(s):");
			msg += _T("\n   %s\n");
			msg += msg2a; // _("However, you did not make any changes in the currently selected user profile which is %s.");
			msg += _T("\n");
			msg += msg3; //_("Are you sure you want to use the %s work profile?");
			msg += _T("\n");
			msg += msg4; // _("Click \"Yes\" to use the %s profile (and save the other profile's changes).\nClick \"No\" to continue editing in the User Workflow Profile dialog.");
			msg = msg.Format(msg,editedProfilesStr.c_str(),newProfileName.c_str(),newProfileName.c_str(),newProfileName.c_str());
			response = wxMessageBox(msg,_T(""),wxICON_QUESTION | wxYES_NO);
		}
		else
		{
			// This block is entered if at least some of the changes were made to the same work profile 
			// that is currently selected - we don't warn the user in such cases. Any changes that were
			// also made to other profiles will be silently saved.
			;
		}
		if (response == wxNO || response == wxCANCEL)
		{
			// abort the OK handler and go back to the dialog
			return;
		}
		// Note: Making the interface change is actually done in the caller in 
		// ConfigureInterfaceForUserProfile(). 
		// Changes to the external AI_UserProfiles.xml file are also done in the 
		// caller - in the App's OnEditUserMenuSettingsProfiles().
		
		// if we get here the user responded "Yes", so update the App's members
		CopyUserProfiles(tempUserProfiles, m_pApp->m_pUserProfiles);
	}
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}
	
void CAdminEditMenuProfile::OnUpdateButtonOK(wxUpdateUIEvent& WXUNUSED(event))
{
	wxASSERT(pOKButton != NULL);
	if (tempWorkflowProfile != startingWorkflowProfile || UserProfileItemsHaveChanged(tempUserProfiles, m_pApp->m_pUserProfiles))
	{
		if (pOKButton != NULL && pOKButton->GetLabel() != _("Save Changes"))
		{
			pOKButton->SetLabel(_("Save Changes"));
			pOKButton->Refresh();
		}
	}
	else
	{
		if (pOKButton != NULL && pOKButton->GetLabel() != _("OK"))
		{
			pOKButton->SetLabel(_("OK"));
			pOKButton->Refresh();
		}
	}
}

void CAdminEditMenuProfile::OnCancel(wxCommandEvent& WXUNUSED(event)) 
{
	// If changes were made ask user to verify it is OK to loose those
	// changes. If not abort Cancel.
	bool bSelChanged = UserProfileSelectionHasChanged();
	bool bItemsChanged = UserProfileItemsHaveChanged(tempUserProfiles, m_pApp->m_pUserProfiles);
	if (bSelChanged || bItemsChanged)
	{
		wxString editedProfilesStr;
		editedProfilesStr = GetNamesOfEditedProfiles();
		wxString msg,msg2;
		int response = -1;
		msg2 = _("\nIf you Cancel now those changes will be lost. Cancel anyway?");
		if (bSelChanged && bItemsChanged)
		{
			msg = _("You changed the user workflow profile selection and one or more items in the following profile(s):");
			msg += _T("\n   %s");
			msg += msg2;
			msg = msg.Format(msg,editedProfilesStr.c_str());
			response = wxMessageBox(msg,_T(""),wxYES_NO | wxICON_WARNING);
		}
		else if (bSelChanged)
		{
			msg = _("You changed the user workflow profile selection from %s to %s.");
			msg += msg2;
			msg = msg.Format(msg,GetNameOfProfileFromProfileValue(startingWorkflowProfile).c_str(),
				GetNameOfProfileFromProfileValue(tempWorkflowProfile).c_str());
			response = wxMessageBox(msg,_T(""),wxYES_NO | wxICON_WARNING);
		}
		else if (bItemsChanged)
		{
			msg = _("You changed one or more items in the following profile(s):");
			msg += _T("\n   %s");
			msg += msg2;
			msg = msg.Format(msg,editedProfilesStr.c_str());
			response = wxMessageBox(msg,_T(""),wxYES_NO | wxICON_WARNING);
		}
		if (response == wxNO)
		{
			return; // don't Cancel
		}
	}
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event); // need to call EndModal here
}

wxString CAdminEditMenuProfile::GetNameOfProfileFromProfileValue(int tempWorkflowProfile)
{
	int profileIndex = 0;
	if (tempWorkflowProfile == 0)
		return _("None");
	else
		profileIndex = tempWorkflowProfile - 1;
	// if we get here user has selected one of the user workflow profiles
	int totNames;
	totNames = tempUserProfiles->definedProfileNames.GetCount();
	wxASSERT(profileIndex >= 0 && profileIndex < totNames);
	wxString str = tempUserProfiles->definedProfileNames.Item(profileIndex);
	return str;
}

wxString CAdminEditMenuProfile::GetNameOfProfileFromProfileIndex(int profileIndex)
{
	int totNames;
	totNames = (int)tempUserProfiles->definedProfileNames.GetCount();
	wxASSERT(profileIndex >= 0 && profileIndex < totNames);
	wxString str = tempUserProfiles->definedProfileNames.Item(profileIndex);
	return str;
}

int CAdminEditMenuProfile::GetIndexOfProfileFromProfileName(wxString profileName)
{
	int ct;
	int totNames;
	totNames = (int)tempUserProfiles->definedProfileNames.GetCount();
	for (ct = 0; ct < totNames; ct++)
	{
		if (tempUserProfiles->definedProfileNames.Item(ct) == profileName)
		{
			return ct;
		}
	}
	return -1;
}

wxString CAdminEditMenuProfile::GetNamesOfEditedProfiles()
{
	wxString str;
	int totNames = tempUserProfiles->definedProfileNames.GetCount();
	int ct;
	for (ct = 0; ct < totNames; ct++)
	{
		if (bProfileChanged[ct] == 1)
		{
			if (str.IsEmpty())
				str = GetNameOfProfileFromProfileIndex(ct);
			else
				str = str + _T(", ") + GetNameOfProfileFromProfileIndex(ct);
		}
	}
	return str;
}

void CAdminEditMenuProfile::CopyProfileVisibilityValues(int sourceProfileIndex, int destinationProfileIndex)
{
	// Scan through the tempUserProfiles and, for the currently selected 
	// profile, change the UserProfileItem's userVisibility values back to
	// their usedFactoryValues.
	ProfileItemList::Node* piNode;
	UserProfileItem* pUserProfileItem;
	int ct_pi;
	int nProfileItems = tempUserProfiles->profileItemList.GetCount();
	for (ct_pi = 0; ct_pi < nProfileItems; ct_pi++)
	{
		piNode = tempUserProfiles->profileItemList.Item(ct_pi);
		pUserProfileItem = piNode->GetData();
		int tot;
		tot = pUserProfileItem->usedVisibilityValues.GetCount();
		wxASSERT(tot == (int)pUserProfileItem->usedFactoryValues.GetCount());
		// copy the sourceProfileIndex values to the destinationProfileIndex values
		// TODO: Why doesn't the following assignement work???
		wxString destName;
		destName = pUserProfileItem->usedProfileNames[destinationProfileIndex];
		wxString srcName;
		srcName = pUserProfileItem->usedProfileNames[sourceProfileIndex];
		
		wxLogDebug(_T("Before Dest Visibility of %s = %s; Before Src Visibility of %s = %s"),destName.c_str(),
			pUserProfileItem->usedVisibilityValues[destinationProfileIndex].c_str(),
			srcName.c_str(),pUserProfileItem->usedVisibilityValues[sourceProfileIndex].c_str());
		
		pUserProfileItem->usedVisibilityValues[destinationProfileIndex] = pUserProfileItem->usedVisibilityValues[sourceProfileIndex];
		
		wxLogDebug(_T("After Dest Visibility of %s = %s; After Src Visibility of %s = %s"),destName.c_str(),
			pUserProfileItem->usedVisibilityValues[destinationProfileIndex].c_str(),
			srcName.c_str(),pUserProfileItem->usedVisibilityValues[sourceProfileIndex].c_str());
	}
}

// other class methods

