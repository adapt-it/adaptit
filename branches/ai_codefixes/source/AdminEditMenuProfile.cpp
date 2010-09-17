/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AdminEditMenuProfile.cpp
/// \author			Bill Martin
/// \date_created	20 August 2010
/// \date_revised	20 August 2010
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
/// \derivation		The CAdminEditMenuProfile class is derived from AIModalDialog.
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
//#include <wx/valtext.h> // for wxTextValidator
#include "Adapt_It.h"
#include "MainFrm.h"
#include "AdminEditMenuProfile.h"
#include "XML.h"

/// This global is defined in Adapt_ItView.cpp.
extern CAdapt_ItApp* m_pApp;

// event handler table
BEGIN_EVENT_TABLE(CAdminEditMenuProfile, AIModalDialog)
	EVT_INIT_DIALOG(CAdminEditMenuProfile::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_OK, CAdminEditMenuProfile::OnOK)
	EVT_NOTEBOOK_PAGE_CHANGED(ID_MENU_EDITOR_NOTEBOOK, CAdminEditMenuProfile::OnNotebookTabChanged)
	EVT_RADIOBOX(ID_RADIOBOX, CAdminEditMenuProfile::OnRadioBoxSelection)
	EVT_CHECKLISTBOX(ID_CHECKLISTBOX_MENU_ITEMS,CAdminEditMenuProfile::OnCheckListBoxToggle)
	EVT_LISTBOX_DCLICK(ID_CHECKLISTBOX_MENU_ITEMS,CAdminEditMenuProfile::OnCheckListBoxDblClick)
END_EVENT_TABLE()

CAdminEditMenuProfile::CAdminEditMenuProfile(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("User Workflow Profiles"),
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
	pRadioBox = (wxRadioBox*)FindWindowById(ID_RADIOBOX);
	wxASSERT(pRadioBox != NULL);
	pCheckListBox = (wxCheckListBox*)FindWindowById(ID_CHECKLISTBOX_MENU_ITEMS);
	wxASSERT(pCheckListBox != NULL);

	// other attribute initializations
}

CAdminEditMenuProfile::~CAdminEditMenuProfile() // destructor
{
	//AI_MenuStructure* tempMenuStructure;
	//UserProfiles* tempUserProfiles;

	// deallocate the memory used by our temporary objects
	if (tempUserProfiles != NULL)
	{
		ProfileItemList::Node* pos;
		int count;
		int item_count = tempUserProfiles->profileItemList.GetCount();
		for(count = 0; count < item_count; count++)
		{
			pos = tempUserProfiles->profileItemList.Item(count);
			UserProfileItem* pItem;
			pItem = pos->GetData();
			pos = pos->GetNext();
			delete pItem;
		}
		delete tempUserProfiles;
	}

	if (tempMenuStructure != NULL)
	{
		MainMenuItemList::Node* mmpos;
		int ct_mm;
		int total_mm = tempMenuStructure->aiMainMenuItems.GetCount();
		for(ct_mm = 0; ct_mm < total_mm; ct_mm++)
		{
			mmpos = tempMenuStructure->aiMainMenuItems.Item(ct_mm);
			AI_MainMenuItem* pmmItem;
			pmmItem = mmpos->GetData();
			wxASSERT(pmmItem != NULL);
			mmpos = mmpos->GetNext();
			
			SubMenuItemList::Node* smpos;
			int ct_sm;
			int total_sm = pmmItem->aiSubMenuItems.GetCount();
			for (ct_sm = 0; ct_sm < total_sm; ct_sm++)
			{
				smpos = pmmItem->aiSubMenuItems.Item(ct_sm);
				AI_SubMenuItem* psmItem;
				psmItem = smpos->GetData();
				wxASSERT(psmItem != NULL);
				smpos = smpos->GetNext();
				delete psmItem;
			}
			delete pmmItem;
		}
		delete tempMenuStructure;
	}
}

void CAdminEditMenuProfile::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	tempWorkflowProfile = m_pApp->m_nWorkflowProfile;
	
	// First, deallocate any memory that was allocated at startup or last invocation
	// of this dialog
	if (m_pApp->m_pAI_MenuStructure != NULL)
	{
		MainMenuItemList::Node* mmpos;
		int ct_mm;
		int total_mm = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.GetCount();
		for(ct_mm = 0; ct_mm < total_mm; ct_mm++)
		{
			mmpos = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.Item(ct_mm);
			AI_MainMenuItem* pmmItem;
			pmmItem = mmpos->GetData();
			wxASSERT(pmmItem != NULL);
			mmpos = mmpos->GetNext();
			
			SubMenuItemList::Node* smpos;
			int ct_sm;
			int total_sm = pmmItem->aiSubMenuItems.GetCount();
			for (ct_sm = 0; ct_sm < total_sm; ct_sm++)
			{
				smpos = pmmItem->aiSubMenuItems.Item(ct_sm);
				AI_SubMenuItem* psmItem;
				psmItem = smpos->GetData();
				wxASSERT(psmItem != NULL);
				smpos = smpos->GetNext();
				delete psmItem;
			}
			delete pmmItem;
		}
		delete m_pApp->m_pAI_MenuStructure;
	}

	// destroy the allocated memory in m_pUserProfiles. This is a single 
	// instance of the UserProfiles struct that was allocated on the heap. Note
	// that m_pUserProfiles also contains a list of pointers in its 
	// profileItemList member that point to UserProfileItem instances on the 
	// heap.
	if (m_pApp->m_pUserProfiles != NULL)
	{
		ProfileItemList::Node* pos;
		int count;
		int item_count = m_pApp->m_pUserProfiles->profileItemList.GetCount();
		for(count = 0; count < item_count; count++)
		{
			pos = m_pApp->m_pUserProfiles->profileItemList.Item(count);
			UserProfileItem* pItem;
			pItem = pos->GetData();
			pos = pos->GetNext();
			delete pItem;
		}
		delete m_pApp->m_pUserProfiles;
	}

	// Select whatever tab the administrator has set if any, first tab if none.
	if (tempWorkflowProfile < 0 || tempWorkflowProfile > (int)pNotebook->GetPageCount())
	{
		// the config file's value for the saved user workflow profile (from 
		// m_pApp->m_nWorkflowProfile) was invalid, so set the temporary value
		// to zero, i.e., the "None" profile.
		tempWorkflowProfile = 0;
		// TODO: warn user about this??
	}
	int tabIndex;
	if (tempWorkflowProfile > 0)
		tabIndex = tempWorkflowProfile - 1;
	else
		tabIndex = 0;
	pNotebook->ChangeSelection(tabIndex); // ChangeSelection does not generate page changing events
	
	// set the wxRadioBox to its initial state from the tempWorkflowProfile copied from the config file's value
	pRadioBox->SetSelection(tempWorkflowProfile); // this does not cause a wxEVT_COMMAND_RADIOBOX_SELECTED event
		
	// Reread the AI_UserProfiles.xml file (this also loads the App's 
	// m_pUserProfiles data structure with latest values stored on disk).
	// Note: the AI_UserProfiles.xml file was read when the app first 
	// ran from OnInit() and the app's menus configured for those values.
	// We need to reread the AI_UserProfiles.xml each time the InitDialog()
	// is called because we change the interface dynamically and need to 
	// reload the data from the xml file in case the administrator previously
	// changed the profile during the current session (which automatically
	// saved any changes to AI_UserProfiles.xml).
	// Note: Reading the AI_UserProfiles.xml file, also repopulates the 
	// m_pAI_MenuStructure with the latest current data.
	bool bReadOK = ReadPROFILES_XML(m_pApp->m_userProfileFileWorkFolderPath);
	if (!bReadOK)
	{
		// XML.cpp issues a Warning that AI_UserProfiles.xml could not be read
		// We'll populate the list boxes with default settings parsed from our
		// default unix-like strings
		m_pApp->SetupDefaultUserProfiles();
		m_pApp->SetupDefaultMenuStructure();
	}
	wxASSERT(m_pApp->m_pAI_MenuStructure != NULL);

	// create a temporary MenuStructure object for use in AdminEditMenuProfile.
	tempMenuStructure = new AI_MenuStructure;
	// create a temporary UserProfiles object for use in AdminEditMenuProfile.
	tempUserProfiles = new UserProfiles;
	// Make a copy of the App's objects
	CopyMenuStructure(m_pApp->m_pAI_MenuStructure, tempMenuStructure);
	CopyUserProfiles(m_pApp->m_pUserProfiles, tempUserProfiles);
	// Finally populate the list box
	PopulateListBox(tabIndex);
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
		int eGetSel = event.GetSelection(); // 
		PopulateListBox(eGetSel);
	}
}
	
void CAdminEditMenuProfile::OnRadioBoxSelection(wxCommandEvent& WXUNUSED(event))
{
	tempWorkflowProfile = pRadioBox->GetSelection();
	// if user selects a workflow profile radio button that is on a different tab 
	// page than is currently displaying, change to that tab page.
	if (tempWorkflowProfile == 0)
		pNotebook->SetSelection(0);
	else
	{
		// when tempWorkflowProfile > 0
		pNotebook->SetSelection(tempWorkflowProfile - 1);
	}
}

void CAdminEditMenuProfile::OnCheckListBoxToggle(wxCommandEvent& event)
{
	// First don't allow the menu category labels in the list be
	// unchecked
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

	// TODO: process any user changes to check boxes
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


void CAdminEditMenuProfile::PopulateListBox(int newTabIndex)
{
	// Note: Each time a wxNotebook tab changes, a different wxPanel appears that
	// contains different actual controls (with different pointers to their
	// instances on the heap) for the wxNotebook and wxRadioBox. Therefore, the 
	// pointers to these controls need to be reestablished each time a Notebook 
	// tab is changed (which brings a different panel and controls into view). 
	
	// The pNotebook and pRadioBox pointers shouldn't actually change, but 
	// it won't hurt to get them again here
	pNotebook = (wxNotebook*)FindWindowById(ID_MENU_EDITOR_NOTEBOOK);
	wxASSERT(pNotebook != NULL);
	pRadioBox = (wxRadioBox*)FindWindowById(ID_RADIOBOX);
	wxASSERT(pRadioBox != NULL);
	
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
	wxASSERT(m_pApp->m_pUserProfiles != NULL);
	if (m_pApp->m_pAI_MenuStructure == NULL || m_pApp->m_pUserProfiles == NULL)
	{
		pCheckListBox->Append(_("[Warning: No User Workflow Profile data is available for display -"));
		pCheckListBox->Append(_("you cannot use this dialog to change user workflow profiles until"));
		pCheckListBox->Append(_("a valid AI_UserProfiles.xml file is installed in your work folder]"));
		pCheckListBox->Enable(FALSE); // disable the checklistbox
		int count;
		count = pRadioBox->GetCount();
		int ct;
		for (ct = 0; ct < count; ct++)
		{
			pRadioBox->Enable(ct,FALSE); // disable the radio box and radio buttons
		}
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
	// menus ("File" "Edit" "View" "Tools" etc). We look for a match between the 
	// m_pUserProfile's <MENU>'s itemID field and the m_pAI_MenuStructure's 
	// <SUB_MENU>'s subMenuID field. Where the ID's match we list the <MENU>'s string
	// values in the listbox.
	wxString mainMenuLabel = _T("");
	wxString prevMainMenuLabel = _T("");
	MainMenuItemList::Node* mmNode;
	AI_MainMenuItem* pMainMenuItem;
	int ct;
	int nMainMenuItems = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.GetCount();
	for (ct = 0; ct < nMainMenuItems; ct++)
	{
		mmNode = m_pApp->m_pAI_MenuStructure->aiMainMenuItems.Item(ct);
		pMainMenuItem = mmNode->GetData();
		mainMenuLabel = pMainMenuItem->mainMenuLabel;

		if (mainMenuLabel != prevMainMenuLabel)
		{
			// this is a new main menu label, add it to the listbox
			lbIndexOfInsertion = pCheckListBox->Append(_T("   \"") + mainMenuLabel + _T("\" Menu"));
			pCheckListBox->Check(lbIndexOfInsertion);
			itemsAlwaysChecked.Add(lbIndexOfInsertion);
			// TODO: Determine why g++ reports GetItem is not a member of wxCheckListBox
			//pCheckListBox->GetItem(lbIndexOfInsertion)->SetBackgroundColour(*wxBLACK); //(m_pApp->sysColorBtnFace);
			//pCheckListBox->GetItem(lbIndexOfInsertion)->SetTextColour(*wxWHITE);
		}

		// now scan through the App's m_pUserProfiles->profileItemList and load profile items
		// that match this main menu label
		ProfileItemList::Node* piNode;
		UserProfileItem* pUserProfileItem;
		int ct_pi;
		int numItemsLoaded = 0;
		int lbIndx;
		int nProfileItems = m_pApp->m_pUserProfiles->profileItemList.GetCount();
		for (ct_pi = 0; ct_pi < nProfileItems; ct_pi++)
		{
			piNode = m_pApp->m_pUserProfiles->profileItemList.Item(ct_pi);
			pUserProfileItem = piNode->GetData();
			if (ProfileItemIsSubMenuOfThisMainMenu(pUserProfileItem,mainMenuLabel))
			{
				lbIndx = pCheckListBox->Append(_T("      ") + pUserProfileItem->itemText + _T("   [") + pUserProfileItem->itemDescr + _T("]"));
				numItemsLoaded++;
				if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
					pCheckListBox->Check(lbIndx,true);
				else
					pCheckListBox->Check(lbIndx,false);
			}
		}
		if (numItemsLoaded == 0)
		{
			// no items were loaded for this main menu so remove the mainMenuLabel, i.e., for the "&Help" menu
			pCheckListBox->Delete(lbIndexOfInsertion);
		}
		prevMainMenuLabel = mainMenuLabel;
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
	int nProfItemCount = m_pApp->m_pUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = m_pApp->m_pUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("preferencesTab"))
		{
			lbIndx = pCheckListBox->Append(_T("      ") + pUserProfileItem->itemText + _T("   [") + pUserProfileItem->itemDescr + _T("]"));
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
	nProfItemCount = m_pApp->m_pUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = m_pApp->m_pUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("modeBar"))
		{
			lbIndx = pCheckListBox->Append(_T("      ") + pUserProfileItem->itemText + _T("   [") + pUserProfileItem->itemDescr + _T("]"));
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
	nProfItemCount = m_pApp->m_pUserProfiles->profileItemList.GetCount();
	for (ct = 0; ct < nProfItemCount; ct++)
	{
		piNode = m_pApp->m_pUserProfiles->profileItemList.Item(ct);
		pUserProfileItem = piNode->GetData();
		if (pUserProfileItem->itemType == _T("wizardListItem"))
		{
			lbIndx = pCheckListBox->Append(_T("      ") + pUserProfileItem->itemText + _T("   [") + pUserProfileItem->itemDescr + _T("]"));
			numItemsLoaded++;
			if (pUserProfileItem->usedVisibilityValues.Item(newTabIndex) == _T("1"))
				pCheckListBox->Check(lbIndx,true);
			else
				pCheckListBox->Check(lbIndx,false);
		}
	}
}

// Copies the menu structure of a pFromMenuStructure instance to a pToMenuStructure instance, copying
// all of the internal attributes and lists.
// Note: Assumes that pFromMenuStructure and pToMenuStructure were created on the heap before this
// function was called.
void CAdminEditMenuProfile::CopyMenuStructure(AI_MenuStructure* pFromMenuStructure, AI_MenuStructure* pToMenuStructure)
{
	wxASSERT(pFromMenuStructure != NULL);
	wxASSERT(pToMenuStructure != NULL);
	int ct;
	int totct;
	totct = pFromMenuStructure->aiMainMenuItems.GetCount();
	for (ct = 0; ct < totct; ct++)
	{
		AI_MainMenuItem* pFromMainMenuItem;
		AI_MainMenuItem* pToMainMenuItem;
		MainMenuItemList::Node* node;
		node = pFromMenuStructure->aiMainMenuItems.Item(ct);
		pFromMainMenuItem = node->GetData();
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
			pToSubMenuItem->subMenuID = pFromSubMenuItem->subMenuID;
			pToSubMenuItem->subMenuKind = pFromSubMenuItem->subMenuKind;
			pToSubMenuItem->subMenuLabel = pFromSubMenuItem->subMenuLabel;
			pToMainMenuItem = new AI_MainMenuItem;
			wxASSERT(pToMainMenuItem != NULL);
			pToMainMenuItem->mainMenuLabel = pFromMainMenuItem->mainMenuLabel;
			pToMainMenuItem->aiSubMenuItems.Append(pFromSubMenuItem);
		}
		pToMenuStructure->aiMainMenuItems.Append(pFromMainMenuItem);
	}
}

// Copies the UserProfiles of a pFromUserProfiles instance to a pToUserProfiles instance, copying
// all of the internal attributes and lists.
// Note: Assumes that pFromUserProfiles and pToUserProfiles were created on the heap before this
// function was called.
void CAdminEditMenuProfile::CopyUserProfiles(UserProfiles* pFromUserProfiles, UserProfiles* pToUserProfiles)
{
	wxASSERT(pFromUserProfiles != NULL);
	wxASSERT(pToUserProfiles != NULL);
	if (pFromUserProfiles != NULL)
	{
		pToUserProfiles->profileVersion = pFromUserProfiles->profileVersion;
		
		int count;
		int totPNames = pFromUserProfiles->definedProfileNames.GetCount();
		for (count = 0; count < totPNames; count++)
		{
			pToUserProfiles->definedProfileNames.Add(pFromUserProfiles->definedProfileNames.Item(count));
		}
		
		ProfileItemList::Node* pos;
		int item_count = pFromUserProfiles->profileItemList.GetCount();
		for(count = 0; count < item_count; count++)
		{
			pos = pFromUserProfiles->profileItemList.Item(count);
			UserProfileItem* pFromItem;
			UserProfileItem* pToItem;
			pFromItem = pos->GetData();
			pToItem = new UserProfileItem;
			wxASSERT(pToItem != NULL);
			pToItem->adminCanChange = pFromItem->adminCanChange;
			pToItem->itemDescr = pFromItem->itemDescr;
			pToItem->itemID = pFromItem->itemID;
			pToItem->itemText = pFromItem->itemText;
			pToItem->itemType = pFromItem->itemType;
			int ct;
			int totCt;
			totCt = pFromItem->usedFactoryValues.GetCount();
			for (ct = 0; ct < totCt; ct++)
			{
				pToItem->usedFactoryValues.Add(pFromItem->usedFactoryValues.Item(ct));
			}
			totCt = pFromItem->usedProfileNames.GetCount();
			for (ct = 0; ct < totCt; ct++)
			{
				pToItem->usedProfileNames.Add(pFromItem->usedProfileNames.Item(ct));
			}
			totCt = pFromItem->usedVisibilityValues.GetCount();
			for (ct = 0; ct < totCt; ct++)
			{
				pToItem->usedVisibilityValues.Add(pFromItem->usedVisibilityValues.Item(ct));
			}
			pToUserProfiles->profileItemList.Append(pToItem);
		}
	}
}

// Determines if the menu item represented by the input pUserProfileItem belongs
// as a subMenu within the input top level mainMenuLabel of the AI menu structure
// according to the current information stored in the m_pAI_MenuStructure object.
bool CAdminEditMenuProfile::ProfileItemIsSubMenuOfThisMainMenu(UserProfileItem* pUserProfileItem, wxString mainMenuLabel)
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
	wxString profileItemID;
	profileItemID = pUserProfileItem->itemID;

	
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
		SubMenuItemList::Node* smNode;
		AI_SubMenuItem* pSubMenuItem;
		int ct_sm;
		int nSubMenuItems = pMainMenuItem->aiSubMenuItems.GetCount();
		for (ct_sm = 0; ct_sm < nSubMenuItems; ct_sm++)
		{
			smNode = pMainMenuItem->aiSubMenuItems.Item(ct_sm);
			pSubMenuItem = smNode->GetData();
			if (pSubMenuItem->subMenuID == profileItemID && menuLabel == mainMenuLabel)
			{
				return TRUE;
			}
		}
	}
	// if we get here we did not find a match
	return FALSE;
}

// Determines if the sub menu with itemText is currently a menu item within
// Adapt It's current menu bar
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


// Gets the label of the top level AI menu where the submenu having IDStr is located
// according to the current information stored in the m_pAI_MenuStructure object.
wxString CAdminEditMenuProfile::GetTopLevelMenuLabelForThisSubMenuID(wxString IDStr)
{
	wxString nullStr = _T("");
	// do a reality check
	wxASSERT(m_pApp->m_pAI_MenuStructure != NULL);
	if (m_pApp->m_pAI_MenuStructure == NULL)
	{
		return nullStr;
	}
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
		SubMenuItemList::Node* smNode;
		AI_SubMenuItem* pSubMenuItem;
		int ct_sm;
		int nSubMenuItems = pMainMenuItem->aiSubMenuItems.GetCount();
		for (ct_sm = 0; ct_sm < nSubMenuItems; ct_sm++)
		{
			smNode = pMainMenuItem->aiSubMenuItems.Item(ct_sm);
			pSubMenuItem = smNode->GetData();
			if (pSubMenuItem->subMenuID == IDStr)
			{
				return menuLabel;
			}
		}
	}
	return nullStr;
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CAdminEditMenuProfile::OnOK(wxCommandEvent& event) 
{
	// update the App member for config file updating on next save
	if (tempWorkflowProfile != m_pApp->m_nWorkflowProfile)
	{
		m_pApp->m_nWorkflowProfile = tempWorkflowProfile;
		// make the doc dirty so it will prompt for saving
	}
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

