/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RemoveSomeTgtEntries.cpp
/// \author			Bruce Waters
/// \date_created	14 October 2013
/// \rcs_id $Id: RemoveSomeTgtEntries.cpp 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the RemoveSomeTgtEntries class. 
/// The RemoveSomeTgtEntries class provides a handler for the "Remove Some Translations..."
/// button in the KB Editor. It provides a wxListCtrl with each line being the target text
/// adaptation, the source text for that adaptation, and a reference count - displayed in 4
/// columns, column 0 being a picture of either an empty checkbox, or a ticked checkbox.
/// Two organising principles are offered by radio buttons, keep the groups of adaptations
/// (or glosses) together according to which source text they are associated with (but
/// alphabetizing within each such group), or simply alphabetizing the target text
/// adaptations (or glosses) altogether (which splits up the source text groupings, but may
/// be easier to use for some situations - such as when working in dialects with very few
/// adaptation variants per source text key). A button is also provided for sending the
/// currently displayed form of the list to a file. The default button closes the dialog
/// and the checked lines are used to define a deletions on the KB entry which are done as
/// soon as the dialog is dismissed. A Cancel button is also provided. The deletions done
/// are standard "pseudo-deletions" - so the items no longer are seen in the KB Editor, but
/// are present in the kb as pseudo-deletions, and so will propagate to other clients if KB
/// Sharing is turned on. This feature was added to help with mass editing when spelling or
/// diacritic changes are many, at a user's request.
/// The wxDesigner resource is Remove_Some_Tgt_Entries_Func
/// \derivation		The RemoveSomeTgtEntries class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "RemoveSomeTgtEntries.h"
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

#include <wx/filename.h>

// other includes
#include "Adapt_It.h"
#include "Adapt_It_wdr.h"
#include "KB.h"
#include "TargetUnit.h"
#include "KBEditor.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"
#include "BString.h"
#include "KbServer.h"
#include <wx/imaglist.h> // for wxImageList
#include "RemoveSomeTgtEntries.h"

extern bool gbIsGlossing;
extern bool gbNoSourceCaseEquivalents;

inline int	CompareNonSrcListRecs(NonSrcListRec* rec1, NonSrcListRec* rec2)
{
	return rec1->nonsrc.Cmp(rec2->nonsrc); // case-sensitive compare
}

inline int	CompareSrcTargetUnits(SrcTgtUnitPair* ptr1, SrcTgtUnitPair* ptr2)
{
	return ptr1->src.Cmp(ptr2->src);
}

// event handler table
BEGIN_EVENT_TABLE(RemoveSomeTgtEntries, AIModalDialog)
	EVT_INIT_DIALOG(RemoveSomeTgtEntries::InitDialog)
	EVT_BUTTON(wxID_OK, RemoveSomeTgtEntries::OnOK)
	EVT_BUTTON(wxID_CANCEL, RemoveSomeTgtEntries::OnCancel)
	EVT_BUTTON(ID_BUTTON_SAVE_ENTRYLIST_TO_FILE, RemoveSomeTgtEntries::OnBtnSaveEntryListToFile)
	EVT_RADIOBUTTON(ID_RADIO_ORGANISE_BY_KEYS, RemoveSomeTgtEntries::OnRadioOrganiseByKeys)
	EVT_RADIOBUTTON(ID_RADIO_SIMPLY_TARGET_ALPHABETICAL, RemoveSomeTgtEntries::OnRadioListTgtAlphabetically)
	EVT_LIST_ITEM_SELECTED(ID_LISTCTRL_BULK_DEL, RemoveSomeTgtEntries::OnCheckboxSelection)
END_EVENT_TABLE()

RemoveSomeTgtEntries::RemoveSomeTgtEntries(
		wxWindow* parent) : AIModalDialog(parent, -1, _("Remove Some Target Text Entries"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	m_pRemoveSomeSizer = Bulk_Delete_Pairs_Func(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	m_pKBEditorDlg = (CKBEditor*)parent; // we'll want access to the parent dlg
	m_pKB = m_pKBEditorDlg->pKB; // the parent knows what type of KB it is displaying (ie. adapting or glossing)
	m_bIsGlossingKB = m_pKB->IsThisAGlossingKB();
	m_pApp = m_pKBEditorDlg->pApp;
	bool bOK;
	bOK = m_pApp->ReverseOkCancelButtonsForMac(this);
	bOK = bOK; // avoid warning

	CentreOnParent();

	// Construct the needed arrays
	m_pOneGroupArray = new SortedNonSrcListRecsArray(CompareNonSrcListRecs);
	m_pUngroupedTgtSortedArray = new SortedNonSrcListRecsArray(CompareNonSrcListRecs);
	m_pGroupsArray = new NonSrcListRecsArray;
	m_pOneTUUnsortedArray = new NonSrcListRecsArray;
	m_pSortedSrcTgtUnitPairsArray = new SortedSrcTgtUnitsArray(CompareSrcTargetUnits);
}

RemoveSomeTgtEntries::~RemoveSomeTgtEntries() // destructor
{
	m_pCheckList->ClearAll();

	pIconImages->RemoveAll();
	if (pIconImages != NULL)
		delete pIconImages;

	// Delete the array entries, emptying the arrays, finally free the array objects' memory
	if (!m_pOneGroupArray->empty())
	{
		WX_CLEAR_ARRAY(*m_pOneGroupArray);
	}
	delete m_pOneGroupArray;

	if (!m_pUngroupedTgtSortedArray->empty())
	{
		WX_CLEAR_ARRAY(*m_pUngroupedTgtSortedArray);
	}
	delete m_pUngroupedTgtSortedArray;

	if (!m_pGroupsArray->empty())
	{
		WX_CLEAR_ARRAY(*m_pGroupsArray);
	}
	delete m_pGroupsArray;

	if (!m_pSortedSrcTgtUnitPairsArray->empty())
	{
		WX_CLEAR_ARRAY(*m_pSortedSrcTgtUnitPairsArray);
	}
	delete m_pSortedSrcTgtUnitPairsArray;

	if (!m_pOneTUUnsortedArray->empty())
	{
		WX_CLEAR_ARRAY(*m_pOneTUUnsortedArray);
	}
	delete m_pOneTUUnsortedArray;
}

void RemoveSomeTgtEntries::InitDialog(wxInitDialogEvent& WXUNUSED(event))
{
	m_pCheckList = (wxListView*)FindWindowById(ID_LISTCTRL_BULK_DEL);
	wxASSERT(m_pCheckList);

	pIconImages = new wxImageList(16,14,TRUE,2); // 0 is ID_BITMAP_CHBOX_OFF & 1 is ID_BITMAP_CHBOX_ON
	// Obtain the checkbox off, and checkbox on, bitmap images which are displayed at start
	// of the line; in the order off & then on. (default is to show the first, so that's
	// what we want)
	wxBitmap checkOFF = BulkKbDeleteFunc(0);
	wxBitmap checkON = BulkKbDeleteFunc(1);
	// Add them to the wxImageList object, for param2, the mask, we use the default
	// wxNullBitmap; we will ignore the returned icon index value (an int)
	pIconImages->Add(checkOFF);
	pIconImages->Add(checkON);
	// Associate the image list with the wxListCtrl instance
	m_pCheckList->SetImageList(pIconImages, wxIMAGE_LIST_SMALL);

	m_no_adaptation = _("<no adaptation>");
	m_no_gloss = _("<no gloss>");

// ***** LEGACY stuff is below, some can be deleted later

	// Use these for quick and dirty approximate lining up of the second field
	// BEW 30Oct13, retain this commented out. If MakeListLine() were refactored to use the
	// approximate lining up of the early approach that used wxCheckListBox for the saving
	// to file, then we'd need to uncomment out these spaces definitions
/*
	m_spaces[0]=_T("");
	m_spaces[1]=_T(" ");
	m_spaces[2]=_T("  ");
	m_spaces[3]=_T("   ");
	m_spaces[4]=_T("   ");
	m_spaces[5]=_T("    ");
	m_spaces[6]=_T("     ");
	m_spaces[7]=_T("      ");
	m_spaces[8]=_T("       ");
	m_spaces[9]=_T("        ");
	m_spaces[10]=_T("         ");
	m_spaces[11]=_T("          ");
	m_spaces[12]=_T("           ");
	m_spaces[13]=_T("            ");
	m_spaces[14]=_T("             ");
	m_spaces[15]=_T("              ");
	m_spaces[16]=_T("               ");
	m_spaces[17]=_T("                ");
	m_spaces[18]=_T("                 ");
	m_spaces[19]=_T("                  ");
	m_spaces[20]=_T("                   ");
	m_spaces[21]=_T("                    ");
	m_spaces[22]=_T("                     ");
	m_spaces[23]=_T("                      ");
	m_spaces[24]=_T("                       ");
	m_spaces[25]=_T("                        ");
	m_spaces[26]=_T("                         ");
	m_spaces[27]=_T("                          ");
	m_spaces[28]=_T("                           ");
	m_spaces[29]=_T("                            ");
	m_spaces[30]=_T("                             ");
	m_spaces[31]=_T("                              ");
	m_spaces[32]=_T("                               ");
	m_spaces[33]=_T("                                ");
	m_spaces[34]=_T("                                 ");
	m_spaces[35]=_T("                                  ");
	m_spaces[36]=_T("                                   ");
	m_spaces[37]=_T("                                    ");
	m_spaces[38]=_T("                                     "); // needed, since field2at
								// is 38, and if nonsrc is empty, this element is accessed
	m_nOtherLine = 0; // initialize
	m_nStoredCheckValue = 0; // initialize
	m_pSelectedRec = NULL; // initialize
*/

	// set up pointers to the buttons etc (I started with the wxDesigner resource having
	// the left radio button clicked, but Ross Jones wants the right button to be the
	// default one set on when the dialog opens. I changed the wxDesigner resource , but I
	// can't get the right one on to find its way into the AI code. So further below I'll
	// force the correct default setting by using the control pointers and setting manually)
	m_pCheckList = (wxListView*)FindWindowById(ID_LISTCTRL_BULK_DEL);
	m_pRadioOrganiseByKeys = (wxRadioButton*)FindWindowById(ID_RADIO_ORGANISE_BY_KEYS);
	m_pRadioListTgtAlphabetically = (wxRadioButton*)FindWindowById(ID_RADIO_SIMPLY_TARGET_ALPHABETICAL);
	this->Centre(wxHORIZONTAL);
	wxString listRadioButtonLabelGloss = _("List in alphabetic order of the gloss text");
	// Depending on which type of CKB the parent tabbed dialog is displaying, set the
	// second radio button's label accordingly - the setting stays in force until this
	// dialog is closed or cancelled
	// The default when opening is to use a label appropriate for an adapting KB, and so
	// we only need change the label if we find we are not working with an adapting KB
	if (m_bIsGlossingKB)
	{
		m_pRadioListTgtAlphabetically->SetLabel(listRadioButtonLabelGloss);
	}
	// Ross Jones thinks users would prefer the default at opening would be to see the
	// whole as tgt (or gloss) first - so the right radio button should be on
	//m_bBySrcGroups = TRUE; // this view by default when first opened
	//m_bCurrentValue = TRUE; // of m_bBySrcGroups when a radio button is clicked
	m_bBySrcGroups = FALSE; // this view by default when first opened
	m_bCurrentValue = FALSE; // of m_bBySrcGroups when a radio button is clicked
	// Force the radio buttons to agree with the new default setting (shouldn't need to do
	// this, but I can't get wxDesigner to update the .wdr file to have the left button
	// turned off, so do it here)
	m_pRadioOrganiseByKeys->SetValue(FALSE);
	m_pRadioListTgtAlphabetically->SetValue(TRUE);

	m_totalKBEntries = GetTotalKBEntries(m_pKB); // how many lines to show in the views of the list

	// Initialize the arrays used for tracking which lines have ticked checkboxes. Note,
	// the views differ, so a give line in one view will certainly NOT be at the same line
	// index in the other - we need to keep track of both arrays so that if the user
	// changes the view, we can automatically restore the indices to what they should be.
	// THE NUMBER OF ENTRIES DOES NOT CHANGE, FOR EITHER VIEW, AND EACH VIEW HAS THE SAME
	// LINE COUNT IN THE LIST
	m_leftCheckedArray.clear();
	m_leftCheckedArray.Alloc(m_totalKBEntries);
	m_leftCheckedArray.Add(0, m_totalKBEntries); // starts out with nothing checked
	m_rightCheckedArray.clear();
	m_rightCheckedArray.Alloc(m_totalKBEntries);
	m_rightCheckedArray.Add(0, m_totalKBEntries); // starts out with nothing checked

	// m_pOneGroupArray is used in a loop repetitively, so start it off empty for safety.
	// Thereafter, DO NOT Clear() it, or call WX_CLEAR_ARRAY() on it -- that does the same
	// as Clear(), it empties the array but also frees the memory pointed to by its items
	// - and the struct points it holds will be copied to another array to be managed by
	// that one. So just do the clear on the array which finally gets to manage the struct
	// pointers - that will be m_pGroupsArray
	WX_CLEAR_ARRAY(*m_pOneGroupArray);

	/* retain in case later wanted for MakeListLine()
	// Define what each list line will look like ([0] for left radio button, [1] for right)
	m_firstSpaces[0] = _T("                        "); // 24 spaces
	m_secondSpaces[0] = _T("      "); // 6 spaces
	m_firstSpaces[1] = _T("      "); // 6 spaces
	m_secondSpaces[1] = _T("            "); // 12 spaces
	*/
	m_output = _T(""); // Empty() this each time before it is used
	m_ref_many = _T("%d"); // for making the string form of the reference count
	m_ref_once = _T("1");
									   
	// Get the CTargetUnit instances' pointers, each with associated source text key, into
	// the m_pSortedSrcTgtUnitPairsArray. This array is sorted by the src text key. We can
	// build everything we need off this one, without having to go to the KB again
	PopulateSrcTargetUnitPairsArray(m_pKB, m_pSortedSrcTgtUnitPairsArray);

	// Make the calls to set up the array for the left radio button's view
	m_bBySrcGroups = TRUE; // temporarily, so that MakeListLine() uses the correct code block
	PopulateGroupsArray(m_pSortedSrcTgtUnitPairsArray, m_pGroupsArray);
	MakeLinesArray(m_pGroupsArray, m_linesArray);
	m_bBySrcGroups = FALSE; // restore default value (as per Ross Jones suggestion)

	// Make the calls to set up the array for the right radio button's view
	//m_bBySrcGroups = FALSE; // temporarily, so that MakeListLine() uses the correct code block
	PopulateTargetSortedArray(m_pSortedSrcTgtUnitPairsArray, m_pUngroupedTgtSortedArray);
	MakeLinesForSortedArray(m_pUngroupedTgtSortedArray, m_linesArrayTgtSorted);
	//m_bBySrcGroups = TRUE; // restore default value

	// Default, load the list from the array for the left radio button
	//PopulateListAsGroups(m_pGroupsArray); // <- my 6.5.1 choice, for 6.5.2 and onwards,
											//default to the right radio button
	PopulateListAsTgtSorted(m_pUngroupedTgtSortedArray); // Ross Jones says this is best default

	// Size the list to suit the hardware, make it as large as is reasonably 
	// possible vertically
	m_pRemoveSomeSizer->ComputeFittingClientSize(this);
	screenHeight = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
	
	this->SetSize(0,1,570,screenHeight - 61); // sets the dialog about 62 pixels
											  // less long as the screen is high
	wxSize mySize = m_LIST_CTRL_SIZER->GetSize();
	m_LIST_CTRL_SIZER->SetItemMinSize(m_pCheckList,mySize.x,screenHeight - 215); // sets
				// the height of the wxListCtrl in the dialog to about 150 pixels less
				// than its window's height - to allow for the buttons & message at the
				// dialog's bottom
	this->CentreOnParent();
}

void RemoveSomeTgtEntries::OnOK(wxCommandEvent& event)
{
	if (m_bIsGlossingKB)
	{
		// Ours is a local glossing KB
#if defined(_KBSERVER)
		if (m_pApp->m_bIsGlossingKBServerProject)
		{
			// This project is one for sharing gloss entries to a remote kbserver,
			// and entry to this handler would be prevented if sharing was disabled, so
			// no need to test for the latter here
			KbServer* pKbServer = m_pApp->GetKbServer(2);
			wxASSERT(pKbServer);

			// Test here for the app's m_arrSourcesForPseudoDeletion or
			// m_arrTargetsForPseudoDeletion not yet Empty. A background thread may be
			// doing bulk pseudo-deletions from an earlier invocation of the
			// RemoveSomeTgtEntries handler, and we must not try to use those arrays for
			// another invocation before that thread finishes - at it's finish it will
			// empty the arrays
			if (!m_pApp->m_arrSourcesForPseudoDeletion.IsEmpty() ||
				!m_pApp->m_arrTargetsForPseudoDeletion.IsEmpty())
			{
				// a previous bulk removal is still in operation, so tell the user to wait
				// a few minutes then try again
				wxString msg = _("A previous bulk removal is still running in the background./nWait a few minutes for it to complete, then try again.\nIf the previous bulk removal involved many entries, you may have to wait and retry more than once.");
				wxMessageBox(msg, _("Operation not completed"), wxICON_INFORMATION | wxOK);
				return; // stay in the dialog
			}

// **** TODO **** finish glossing kbserver support when I know whether or not Jonathan will give me
// php loops at the server end, to speed up bulk deleting
		} // end of TRUE block for test: if (m_pApp->m_bIsGlossingKBServerProject)
#endif
		// We can go ahead with the present bulk removal
		GetPhrasePairsForBulkRemoval(&m_leftCheckedArray, m_pGroupsArray, 
			&m_pApp->m_arrSourcesForPseudoDeletion, &m_pApp->m_arrTargetsForPseudoDeletion);

		// Start the background kbserver deletions first, then do the local KB
		// deletions after that thread has been fired off
#if defined(_KBSERVER)

		// ***********************  TODO  the thread for the kbserver deletions **********************
		
#endif
		// Now the local KB deletions
		DoLocalBulkKbPseudoDeletions(m_bIsGlossingKB);
	}
	else
	{
		// Ours is an local adapting KB
#if defined(_KBSERVER)
		if (m_pApp->m_bIsKBServerProject)
		{
			// This project is one for sharing adaptation entries to a remote kbserver,
			// and entry to this handler would be prevented if sharing was disabled, so
			// no need to test for the latter here
			KbServer* pKbServer = m_pApp->GetKbServer(1);
			wxASSERT(pKbServer);

			// Test here for the app's m_arrSourcesForPseudoDeletion or
			// m_arrTargetsForPseudoDeletion not yet Empty. A background thread may be
			// doing bulk pseudo-deletions from an earlier invocation of the
			// RemoveSomeTgtEntries handler, and we must not try to use those arrays for
			// another invocation before that thread finishes - at it's finish it will
			// empty the arrays
			if (!m_pApp->m_arrSourcesForPseudoDeletion.IsEmpty() ||
				!m_pApp->m_arrTargetsForPseudoDeletion.IsEmpty())
			{
				// a previous bulk removal is still in operation, so tell the user to wait
				// a few minutes then try again
				wxString msg = _("A previous bulk removal is still running in the background./nWait a few minutes for it to complete, then try again.\nIf the previous bulk removal involved many entries, you may have to wait and retry more than once.");
				wxMessageBox(msg, _("Operation not completed"), wxICON_INFORMATION | wxOK);
				return; // stay in the dialog
			}

// **** TODO **** finish kbserver support when I know whether or not Jonathan will give me
// php loops at the server end, to speed up bulk deleting
		} // end of TRUE block for test: if (m_pApp->m_bIsKBServerProject)
#endif
		// We can go ahead with the present bulk removal
		GetPhrasePairsForBulkRemoval(&m_leftCheckedArray, m_pGroupsArray, 
			&m_pApp->m_arrSourcesForPseudoDeletion, &m_pApp->m_arrTargetsForPseudoDeletion);

		// Test it. Comment out next lines when test is no longer wanted
#if defined(_DEBUG)
/*		{ // <<- limit scope to just this for loop
		size_t count = m_pApp->m_arrSourcesForPseudoDeletion.size();
		size_t i;
		for (i = 0; i < count; ++i)
		{
			wxLogDebug(_T("Pseudo-delete this [src,tgt] pair: [ %s , %s ]"),
				m_pApp->m_arrSourcesForPseudoDeletion.Item(i).c_str(),
				m_pApp->m_arrTargetsForPseudoDeletion.Item(i).c_str());
		}
		} // <<- end of scope limitation 
*/
#endif
		// Start the background kbserver deletions first, then do the local KB
		// deletions after that thread has been fired off

#if defined(_KBSERVER)

		// ***********************  TODO  the thread for the kbserver deletions **********************
		
#endif
		// Now the local KB deletions
		DoLocalBulkKbPseudoDeletions(m_bIsGlossingKB);
	} // end of else block for test: if (m_bIsGlossingKB)

	// Clear the arrays
	m_pApp->m_arrSourcesForPseudoDeletion.clear();
	m_pApp->m_arrTargetsForPseudoDeletion.clear();
	event.Skip();
}

void RemoveSomeTgtEntries::OnCancel(wxCommandEvent& event)
{
	m_pApp->m_pDlgTgtFont->SetPointSize(12); // retore to normal 12 point
	event.Skip();
}

void RemoveSomeTgtEntries::DoLocalBulkKbPseudoDeletions(bool bIsGlossingKB)
{
	CKB* pKB = NULL;
	if (bIsGlossingKB)
	{
		pKB = m_pApp->m_pGlossingKB; // the glossing KB's pointer
	}
	else
	{
		pKB = m_pApp->m_pKB; // the adapting KB's pointer
	}
	size_t count = m_pApp->m_arrSourcesForPseudoDeletion.size();
	size_t i;
	for (i = 0; i < count; ++i)
	{
		// Get the next source phrase / target phrase pair to be pseudo-deleted
		wxString src = m_pApp->m_arrSourcesForPseudoDeletion.Item(i);
		wxString tgt = m_pApp->m_arrTargetsForPseudoDeletion.Item(i);

		// Get the CTargetUnit instance which stores this pair
		int numSrcWords = CountSpaceDelimitedWords(src); // this is a helper.cpp function

		// The following GetTargetUnit() call does an AutoCapsLookup() and so, if
		// "Jones/Jones" was the src/tgt pair to be deleted, the src "Jones" would be
		// changed to "jones" if Automatic Capitalizing is currently turned on, and so the
		// pair requested for deletion would not get deleted - because all upper case KB
		// entries are ignored in AutoCapsLookup() calls. If the src/tgt pair was
		// lower-case initial for the src text, the autocaps lookup would not change the
		// key, it would be equivalent to a normal lookup. So to be sure that the user can
		// delete entries with upper-case-initial src phrase without having to turn
		// autocapitalization off beforehand (which users would never remember to do), we
		// need to force a normal lookup to be done. We already have a global boolean
		// gbNoSourceCaseEquivalents which, if TRUE, forces a normal lookup to be done
		// (it's designed for languages where there is no upper/lower case distinction).
		// So we don't have to do anything fancy, just temporarily set this boolean TRUE
		// before the GetTargetUnit() call, and restore it's former value on return.
		bool bSaveFlag = gbNoSourceCaseEquivalents;
		gbNoSourceCaseEquivalents = TRUE;

		CTargetUnit* pTU =  pKB->GetTargetUnit(numSrcWords, src); // does an AutoCapsLookup()

		gbNoSourceCaseEquivalents = bSaveFlag;
		if (pTU == NULL)
		{
			continue; // if we can't find it, skip deleting this one (shouldn't ever happen)
		}
		else
		{
			// pTU points the CTargetUnit instance we want, so next we get the
			// CRefString instance which stores the adaption (or gloss if in
			// glossing mode)
			CRefString* pRefString = NULL;
			pRefString = pTU->FindRefStringUndeleted(tgt);
			if (pRefString == NULL)
			{
				// no match, so it's not an undeleted one in CTargetUnit instance, so
				// skip trying to delete this one
				continue;
			}
			else
			{
				// We have the required CRefString instance, so now pseudo-delete it
				pRefString->SetDeletedFlag(TRUE);
				pRefString->GetRefStringMetadata()->SetDeletedDateTime(GetDateTimeNow());
				pRefString->m_refCount = 0;
			}
		} // end of else block for test: if (pTU == NULL)
	} // end of loop: for (i = 0; i < count; ++i)
}

void RemoveSomeTgtEntries::OnRadioOrganiseByKeys(wxCommandEvent& WXUNUSED(event))
{
	// Get the flag's current value first, in case the user clicks on the button when it
	// is already selected -- we use this to check for a no-operation, we don't want to
	// setup the list again if it is already set up correctly
	m_bCurrentValue = m_bBySrcGroups;
	m_pRadioOrganiseByKeys->SetValue(TRUE);
	m_bBySrcGroups = TRUE;
	m_pRadioListTgtAlphabetically->SetValue(FALSE);
	if (m_bBySrcGroups != m_bCurrentValue)
	{
		// The user is genuinely changing to this view from the other, so we need to setup
		// the list again with this button's view
		m_pCheckList->ClearAll(); // Clear all items and columns
		PopulateListAsGroups(m_pGroupsArray);

		// Get the ticks restored to the appropriate checkboxes
		SetCheckboxes(m_bBySrcGroups);
	}
}

void RemoveSomeTgtEntries::OnRadioListTgtAlphabetically(wxCommandEvent& WXUNUSED(event))
{
	// Get the flag's current value first, in case the user clicks on the button when it
	// is already selected -- we use this to check for a no-operation, we don't want to
	// setup the list again if it is already set up correctly
	m_bCurrentValue = m_bBySrcGroups;
	m_pRadioOrganiseByKeys->SetValue(FALSE);
	m_pRadioListTgtAlphabetically->SetValue(TRUE);
	m_bBySrcGroups = FALSE;
	if (m_bBySrcGroups != m_bCurrentValue)
	{
		// The user is genuinely changing to this view from the other, so we need to setup
		// the list again with this button's view
		m_pCheckList->ClearAll(); // Clear all items and columns
		PopulateListAsTgtSorted(m_pUngroupedTgtSortedArray);

		// Get the ticks restored to the appropriate checkboxes
		SetCheckboxes(m_bBySrcGroups);
	}
}

// This button handler is modelled after OnFileExportKb() from Adapt_It.cpp, it supports
// navigation protection, filename modification option (none, or add datetime stamp), and
// informs the user where the file got saved. We make use of whatever KB export options
// are currently in force in the application, and save to the folder _KB_INPUTS_OUTPUTS
// unless allowed options permit saving elsewhere and the user chooses to do so
void RemoveSomeTgtEntries::OnBtnSaveEntryListToFile(wxCommandEvent& WXUNUSED(event))
{
	wxString msg = _("Do you want the date and time appended to the filename?");
	wxString caption = _("Choose dated or undated filename");
	bool bAddDatetime = FALSE;
	int answer = wxMessageBox(msg, caption, wxYES_NO | wxCANCEL);
	if (answer == wxCANCEL)
	{
		return;
	}
	else if (answer == wxYES)
	{
		bAddDatetime = TRUE;
	}
	if (bAddDatetime)
		m_pApp->LogUserAction(_T("OnBtnSaveEntryListToFile() for KB requested, filename datetime append wanted"));
	else
		m_pApp->LogUserAction(_T("OnBtnSaveEntryListToFile() for KB requested, filename datetime append not wanted"));

	// Calculate the appropriate KB export's dictFilename and defaultDir.
    // Note: In the App's SetupDirectories() function the m_curProjectName is
    // constructed as: m_sourceName + _T(" to ") + m_targetName + _T(" adaptations"),
    // so the "to" and "adaptations" parts are non-localized, i.e., we can depend on
    // them being constant in project names. All KB exports use a default name based
    // on m_curProjectName without " adaptations" part.
	wxString dictFilename;
	dictFilename = m_pApp->m_curProjectName; // m_curProjectName is of the form "x to y adaptations"
	int offset = dictFilename.Find(_T(' '),TRUE); // TRUE - find from right end
	dictFilename = dictFilename.Mid(0,offset); // remove "adaptations" or Tok Pisin equivalent
	// The base dictFilename is now in the form of "x to y"

	wxString defaultDir; defaultDir.Empty();
	wxString glossStr;
	bool bBypassFileDialog_ProtectedNavigation = FALSE;
	// Check whether navigation protection is in effect for _KB_INPUTS_OUTPUTS,
	// and whether the App's m_lastKbOutputPath is empty or has a valid path,
	// and set the defaultDir for the export accordingly.
	if (m_pApp->m_bProtectKbInputsAndOutputsFolder)
	{
		// Navigation protection is ON, so set the flag to bypass the wxFileDialog
		// and force the use of the special protected folder for the export.
		bBypassFileDialog_ProtectedNavigation = TRUE;
		defaultDir = m_pApp->m_kbInputsAndOutputsFolderPath; // <projectPath>\_KB_INPUTS_OUTPUTS
	}
	else if (m_pApp->m_lastKbOutputPath.IsEmpty() ||
		(!m_pApp->m_lastKbOutputPath.IsEmpty() && !::wxDirExists(m_pApp->m_lastKbOutputPath)))
	{
		// Navigation protection is OFF so we set the flag to allow the wxFileDialog
		// to appear. But the m_lastKbOutputPath is either empty or, if not empty,
		// it points to an invalid path, so we initialize the defaultDir to point to
		// the special protected folder, even though Navigation protection is not ON.
		// In this case, the user could point the export path elsewhere using the
		// wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = m_pApp->m_kbInputsAndOutputsFolderPath;
	}
	else
	{
		// Navigation protection is OFF and we have a valid path in m_lastKbOutputPath,
		// so we initialize the defaultDir to point to the m_lastKbOutputPath for the
		// location of the export. The user could still point the export path elsewhere
		// in the wxFileDialog that will appear.
		bBypassFileDialog_ProtectedNavigation = FALSE;
		defaultDir = m_pApp->m_lastKbOutputPath;
	}
	// Add "dictionary records" and "glossing" (if gbIsGlossing) to the dictFilename
	dictFilename += _T(' ');
	if (!m_bIsGlossingKB)
	{
		// It's not data from a glossing KB
		dictFilename += _("adaptations_KB");
	}
	else
	{
		// It's data from a glossing KB
		dictFilename += _("glossing_KB");
	}
	// Add the default extension for text data exports
	dictFilename += _T(".txt"); // the extension is not localizable
	
	wxString exportPath; // put the final path in here
	wxString uniqueFilenameAndPath;
	wxString suffix = _T("_contents_");
    // Prepare a unique filename and path from the dictFilename. This unique filename and
    // path is used when the export is nav protected or when the user has chosen that a
    // date-time stamp is to be suffixed to the export filename -- which ensures that any
    // existing earlier exports are not overwritten
	uniqueFilenameAndPath = GetUniqueIncrementedFileName(
			dictFilename,incrementViaDate_TimeStamp,TRUE,2,suffix); // TRUE - always modify
	if (bAddDatetime)
	{
		// Use the unique path for exportPath
		dictFilename = uniqueFilenameAndPath;
	}

	// Allow the wxFileDialog only when the export is not protected from navigation
	if (!bBypassFileDialog_ProtectedNavigation)
	{
		// get a file dialog
		wxString filter = _("Plain text export (*.txt)|*.txt|All Files (*.*)|*.*||");
		wxFileDialog fileDlg(
			(wxWindow*)m_pApp->GetMainFrame(), // MainFrame is parent window for file dialog
			_("Filename for export of knowledge base contents"),
			defaultDir,	// empty string causes it to use the current working directory (set above)
			dictFilename,	// default filename
			filter,
			wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		fileDlg.Centre();

		// make the dialog visible
		if (fileDlg.ShowModal() != wxID_OK)
		{
			m_pApp->LogUserAction(_T("Cancelled from OnBtnSaveEntryListToFile() from within wxFileDialog()"));
			return; // user cancelled
		}

		// get the user's desired path and file name
		exportPath = fileDlg.GetPath();

		// whm 5Aug11 note: When nav protection is OFF, we allow the user to
		// determine the dictFilename's extension for SFM exports (default is .txt)
		// but we need to force the extension to be .lift for LIFT exports even if
		// the user has typed a different extension
		wxString path, fname, ext;
		wxFileName::SplitPath(exportPath, &path, &fname, &ext);
	}
	else
	{
		// While nav protection is ON, the user doesn't see a wxFileDialog but we set
		// the path and filename automatically
		// determine exportPath to the _KB_INPUTS_OUTPUTS folder using the dictFilename
		exportPath = m_pApp->m_kbInputsAndOutputsFolderPath + m_pApp->PathSeparator + dictFilename;
	}

	wxFile f;
	if( !f.Open(exportPath, wxFile::write))
	{
		// Failure is unlikely, an English message will do
		wxMessageBox(_T("Unable to open file descriptor for knowledge base entry list export"),
		_T("File descriptor open attempt failed"), wxICON_EXCLAMATION | wxOK);
		m_pApp->LogUserAction(_T("Unable to open file descriptor for knowledge base entry list export"));
		return; // return since it is not a fatal error
	}
    // We set the app's m_lastKbOutputPath variable with the path part of the exportPath
    // just used. We do this even when navigation protection is on, so that the special
    // folders would be the initial path suggested if the administrator were to switch
    // Navigation Protection OFF.
	wxString path, fname, ext;
	wxFileName::SplitPath(exportPath, &path, &fname, &ext);

	// Do the export, and update the m_last...Path variables for the project config file
	DoKBExportOfListedEntries(&f, m_bBySrcGroups);
	// update m_lastKbOutputPath
	m_pApp->m_lastKbOutputPath = path;
	
	// close the file
	f.Close();

	// Report the completion of the export to the user.
	// Note: For protected navigation situations AI determines the actual
	// filename that is used for the export, and the export itself is
	// automatically saved in the appropriate outputs folder. Especially
	// in these situations where the user has no opportunity to provide a
	// file name nor navigate to a random path, we should inform the user
	// of the successful completion of the export, and indicate the file
	// name that was used and its outputs folder name and location.
	wxFileName fn(exportPath);
	wxString fileNameAndExtOnly = fn.GetFullName();
	msg.Empty();
	msg = msg.Format(_("The saved file was named:\n\n%s\n\nIt was saved at the following path:\n\n%s"),fileNameAndExtOnly.c_str(),exportPath.c_str());
	wxMessageBox(msg,_("Listed entries saved"),wxICON_INFORMATION | wxOK);
	m_pApp->LogUserAction(_T("KB list entries save was successful"));
}

size_t RemoveSomeTgtEntries::GetTotalTargetUnits(CKB* pKB)
{
	size_t numMapsInUse = pKB->m_nMaxWords; // <= 10
	size_t index;
	size_t total = 0;
	for (index = 0; index < numMapsInUse; index++)
	{
		MapKeyStringToTgtUnit* pMap = pKB->m_pMap[index];
		total += pMap->size();
	}
	return total;
}

size_t RemoveSomeTgtEntries::GetTotalKBEntries(CKB* pKB)
{
	CTargetUnit* pTU;
	size_t numMapsInUse = pKB->m_nMaxWords; // <= 10
	size_t index;
	size_t total = 0;
	size_t subtotal = 0;
	for (index = 0; index < numMapsInUse; index++)
	{
		MapKeyStringToTgtUnit* pMap = pKB->m_pMap[index];
		MapKeyStringToTgtUnit::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter)
		{
			 // ignore src text keys here, but if we wanted them, it's iter->first;
			pTU = iter->second;
			subtotal = pTU->CountNonDeletedRefStringInstances();
			total += subtotal;
		}
	}
	return total;
}

// This creates SrcTgtUnitPair structs on the heap (they store src key, and ptr to
// CTargetUnit) and loads their pointers in to this SORTED array. It's sorted by the src
// key. With this array we can use it to populate the other ones.
void RemoveSomeTgtEntries::PopulateSrcTargetUnitPairsArray(CKB* pKB,  SortedSrcTgtUnitsArray* pArray)
{
	// Start with an empty array
	WX_CLEAR_ARRAY(*pArray);
	// Find out how may CTargetUnit instances there are, we'll pre-allocate the array space
	// we need to save time
	size_t totalTargetUnits = GetTotalTargetUnits(pKB);
	SrcTgtUnitPair* pSrcTUPair = NULL; // create these on the heap as we iterate through the maps
	pArray->Alloc(totalTargetUnits); // the Boko to Shanga KB has 10,001 of these, so far
	size_t numMapsInUse = pKB->m_nMaxWords; // <= 10
	size_t index;
	for (index = 0; index < numMapsInUse; index++)
	{
		MapKeyStringToTgtUnit* pMap = pKB->m_pMap[index];
		MapKeyStringToTgtUnit::iterator iter;
		for (iter = pMap->begin(); iter != pMap->end(); ++iter)
		{
			pSrcTUPair = new SrcTgtUnitPair;
			pSrcTUPair->src = iter->first;
			pSrcTUPair->pTU = iter->second;
			pArray->Add(pSrcTUPair);
		}
	}
}

// This is used in a loop which iterates over all SrcTgtrUnitPair struct pointers. Each
// passed in pPair has a wxString src member, and a CTargetUnit* member. The latter stores
// a list, m_translations which have one CRefString for each target (or gloss if in
// glossing mode) word or phrase associated with that src text key. What we do here is get
// all the word or phrase instances, and the reference count, create on the heap a
// NonSrcListRec for each, and store the key, adaptation (or gloss), and ref count in it.
// These are added to the passed in array (ie. their pointers are added) and they are in
// sorted order according to the target text string (or gloss text string).
// (This then means we can later copy the pointers to an unsorted array quickly,
// maintaining sorted order, and from those construct the lines for the dialog's listbox,
// again, maintaining sorted order. This function is used for the left radio button - we
// want to keep identical src text together, but otherwise have alphabetical order - in
// both source keys and within each group, the adaptations (or glosses) sorted too)
void RemoveSomeTgtEntries::PopulateOneTargetUnitGroupArray(SrcTgtUnitPair* pPair, SortedNonSrcListRecsArray* pArray)
{
	// Make sure the array of struct pointers is empty but don't free their memory because
	// another array from the previous call will need to manage those pointers copied to
	// it. Here we just call Empty() 
	pArray->Empty(); 
	m_src = pPair->src; // m_src is a scratch member variable for wxString
	CTargetUnit* pTU = pPair->pTU;
	wxString notInKBStr = _T("<Not In KB>");
	TranslationsList::Node* pos = 0;
	CRefString* pRefString = NULL;
	pos = pTU->m_pTranslations->GetFirst();
	while (pos != 0)
	{
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pos = pos->GetNext(); // prepare for possibility of yet another
		m_nonsrc = pRefString->m_translation;
		if (pRefString->GetDeletedFlag())
		{
			continue; // don't want any pseudo-deleted ones
		}
		if (m_nonsrc == notInKBStr)
		{
			continue; // we don't want these either
		}
		// Anything left is wanted
		NonSrcListRec* pStruct = new NonSrcListRec;
		// Populate its members
		pStruct->nonsrc = m_nonsrc; // this is what the compare function uses
									// for sorting the struct in the array
		pStruct->src = m_src; // this is a constant string, for any give CTargetUnit instance
		pStruct->numrefs = pRefString->m_refCount;
		// Store it in the sorted array
		pArray->Add(pStruct);
	}
}

void RemoveSomeTgtEntries::PopulateOneTargetUnitUnsortedArray(SrcTgtUnitPair* pPair, NonSrcListRecsArray* pArray)
{
	// Make sure the array of struct pointers is empty but don't free their memory because
	// another array from the previous call will need to manage those pointers copied to
	// it. Here we just call Empty() 
	pArray->Empty(); 
	m_src = pPair->src; // m_src is a scratch member variable for wxString
	CTargetUnit* pTU = pPair->pTU;
	wxString notInKBStr = _T("<Not In KB>");
	TranslationsList::Node* pos = 0;
	CRefString* pRefString = NULL;
	pos = pTU->m_pTranslations->GetFirst();
	while (pos != 0)
	{
		pRefString = (CRefString*)pos->GetData();
		wxASSERT(pRefString != NULL);
		pos = pos->GetNext(); // prepare for possibility of yet another
		m_nonsrc = pRefString->m_translation;
		if (pRefString->GetDeletedFlag())
		{
			continue; // don't want any pseudo-deleted ones
		}
		if (m_nonsrc == notInKBStr)
		{
			continue; // we don't want these either
		}
		// Anything left is wanted
		NonSrcListRec* pStruct = new NonSrcListRec;
		// Populate its members
		pStruct->nonsrc = m_nonsrc; // this is what the compare function uses
									// for sorting the struct in the array
		pStruct->src = m_src; // this is a constant string, for any give CTargetUnit instance
		pStruct->numrefs = pRefString->m_refCount;
		// Store it in the sorted array
		pArray->Add(pStruct);
	}
}

// This is called only once, and stays unchanged. If the user flips the radio buttons, the
// list is reconstituted from a wxArrayString built from this (which also stays unchanged)
void RemoveSomeTgtEntries::PopulateGroupsArray(SortedSrcTgtUnitsArray* pSortedTUArray, 
											   NonSrcListRecsArray* pArray)
{
	WX_CLEAR_ARRAY(*pArray); // make sure it starts off empty & all memory freed
	size_t count = pSortedTUArray->size();
	// Pre-allocate space to save time
	size_t totalArraySize = GetTotalKBEntries(m_pKB);
	pArray->Alloc(totalArraySize); // could be many thousands, or several tens of thousands
	size_t i;
	for (i = 0; i < count; i++)
	{
		SrcTgtUnitPair* pMySrcTgtUnitPair = pSortedTUArray->Item(i);
        // In the next call, m_pOneGroupArray is not cleared beforehand, because it is
        // cleared within as the first thing done - though this redundant here; this call
        // takes the single struct passed in as first param, and converts it into one or
        // more NonSrcListRec structs each created on the heap and which are then added, in
        // sorted order by the nonsource (ie. target, or gloss) text into the
        // m_pOneGroupArray. The latter is not sorted, it doesn't need to be because the
        // data is loaded in pre-sorted order. After generating these structs, we will
        // append them to pArray (which also is not sorted, because the source key ordering
        // was done earlier)
		PopulateOneTargetUnitGroupArray(pMySrcTgtUnitPair, m_pOneGroupArray);
        // New we need to append the new NonSrcListRec structs from m_pOneGroupArray to
        // what is in pArray already, so that we can empty m_pOneGroupArray (BEWARE, the
        // appending does shallow copies - ie, of the pointers only, so DO NOT delete the
        // contents of m_pOneGroupArray, ie, don't use WX_CLEAR_ARRAY() or Clear(), but
        // rather, just call Empty() which leaves the objects alive - necessary because
        // pArray has to manage those pointers from now on)
		WX_APPEND_ARRAY(*pArray, *m_pOneGroupArray);
		m_pOneGroupArray->Empty(); // removes the stored pointers (m_pOneGroupArray only gets
								// as many entries as the user has adaptation variants in
								// the pTU, and so its size is usually less than 2 or 3
								// dozen items - hence we need not call Alloc(), default
								// size will do
	}
	// At this point, the whole KB of src keys and their associated CTargetUnit stored
	// adaptation strings (or glosses) is represented as a partly ordered array of 
	// SrcTgtUnitPair struct pointers - each storing nonsrc, src, and refCount. From these
	// we can construct the wxString instances we'll load into the wxListCtrl
}

// This is called only once, and stays unchanged. If the user flips the radio buttons, the
// list is reconstituted from a wxArrayString built from this (which also stays unchanged)
void RemoveSomeTgtEntries::PopulateTargetSortedArray(SortedSrcTgtUnitsArray* pSortedTUArray,
													 SortedNonSrcListRecsArray* pArray)
{
	WX_CLEAR_ARRAY(*pArray); // make sure it starts off empty & all memory freed
	size_t count = pSortedTUArray->size();
	// Pre-allocate space to save time
	size_t totalArraySize = GetTotalKBEntries(m_pKB);
	pArray->Alloc(totalArraySize); // could be many thousands, or several tens of thousands
	size_t i;
	for (i = 0; i < count; i++)
	{
		SrcTgtUnitPair* pMySrcTgtUnitPair = pSortedTUArray->Item(i);
        // In the next call, m_pOneTUUnsortedArray is not cleared beforehand, because it is
        // cleared within as the first thing done - though this redundant here; this call
        // takes the single struct passed in as first param, and converts it into one or
        // more NonSrcListRec structs each created on the heap and which are then added, in
        // unsorted order into m_pOneTUUnsortedArray. The latter is not sorted, it doesn't
        // need to be because the data will be sorted when entered into pArray. After
        // generating these structs, we will append them to pArray (which is sorted).
		PopulateOneTargetUnitUnsortedArray(pMySrcTgtUnitPair, m_pOneTUUnsortedArray);
        // New we need to append the new NonSrcListRec structs from m_pOneGroupArray to
        // what is in pArray already, so that we can empty m_pOneGroupArray (BEWARE, the
        // appending does shallow copies - ie, of the pointers only, so DO NOT delete the
        // contents of m_pOneGroupArray, ie, don't use WX_CLEAR_ARRAY() or Clear(), but
        // rather, just call Empty() which leaves the objects alive - necessary because
        // pArray has to manage those pointers from now on)
		// WX_APPEND_ARRAY(*pArray, *m_pOneTUUnsortedArray); <- doesn't work if param1 is
		// a sorted array, so do it with a loop
		size_t index;
		size_t count = m_pOneTUUnsortedArray->size();
		NonSrcListRec* pRec = NULL;
		for (index = 0; index < count; ++index)
		{
			pRec = m_pOneTUUnsortedArray->Item(index);
			pArray->Add(pRec); // sorted by nonsrc member of the struct
		}
		m_pOneTUUnsortedArray->Empty(); // removes the stored pointers (m_pOneTUUnsortedArray
                // only gets as many entries as the user has adaptation variants in the
                // pTU, and so its size is usually less than 2 or 3 dozen items - hence we
                // need not call Alloc(), default size will do
	}
    // At this point, the whole KB of target text adaptations (or glosses) and their
    // associated stored source text strings with them. These are in the sorted array of
    // SrcTgtUnitPair struct pointers - each storing nonsrc, src, and refCount. From these
    // we can construct the wxString instances we'll load into the wxListCtrl
}

wxString RemoveSomeTgtEntries::MakeListLine(NonSrcListRec* pStruct)
{
    // BEW changed 30Oct13 to pad with exactly 10 spaces each time padding is done - the
    // reason is that using variable numbers of spaces the lining up is only approximate,
    // but if I use 10 spaces, while lining up is worse, it nevertheless makes it easy for
    // grep or a global search and replace to change things if someone wants to massage the
    // file's data. I'll leave my early code here, commented out, in case I change my mind
	// later on. I'll also add [Source] and 3 spaces before the source text, so it's
	// clear which is source and which is non-source. (The latter label will be localizable.)
	wxString pad = _T("          ");
	wxString srcLabel = _("[Source]   ");

	m_output.Empty();
	wxString mynonsrc = pStruct->nonsrc; 
	//int srcLength = pStruct->src.Len(); // characters
	int nonsrcLength = mynonsrc.Len(); // characters
	if (nonsrcLength == 0)
	{
		// Adaptation or gloss was empty, so use <no adapation> or <no gloss> strings
		if (m_bIsGlossingKB)
		{
			mynonsrc = m_no_gloss;
		}
		else
		{
			mynonsrc = m_no_adaptation;
		}
		nonsrcLength = mynonsrc.Len(); // get the revised length, locally only
	}
	//int field2at = 38; // characters in from left of checklistbox
	//int diff = 0;
	//int refSpace1 = 12; // use with grouped view (left radio button)
	//int refSpace2 = 6; // use with simple tgt alphabetized view (right radio button)

	if (m_bBySrcGroups)
	{
		m_output = srcLabel;
		m_output += pStruct->src; // source text first
		m_output += pad;
		/*
		if (srcLength < field2at)
		{
			//  Pad out to field2at location
			diff = field2at - srcLength;
			m_output += m_spaces[diff];
		}
		else
		{
			// give an extra 4 spaces
			m_output += m_spaces[4];
		}
		*/
		m_output += mynonsrc; // target text next
		m_output += pad;
		// Ref counts last
		//m_output += m_spaces[refSpace1]; // small gap (wider makes diacritics less likely to overlap entry above)
		if (pStruct->numrefs > 1)
		{
			wxString endStr;
			endStr = endStr.Format(m_ref_many, pStruct->numrefs); 
			m_output += endStr; // the ref count number in parentheses
		}
		else
		{
			m_output += m_ref_once;
		}
	}
	else
	{
		/*
		// Make adjustments to the measurements
		if (nonsrcLength < field2at)
		{
			// Get a pad value for diff, using old field2at value
			diff = field2at - nonsrcLength;
		}
		else
		{
			// give an extra 4 spaces
			diff =4;
		}
		wxString endStr;
		endStr = endStr.Format(m_ref_many, pStruct->numrefs);
		//int endStrLen = endStr.Len();
		//int extraSpaces = refSpace2 + endStrLen;
		*/
		// the simple alphabetized list format, tgt text first
		m_output = mynonsrc; // target text first
		m_output += pad;
		/*
		m_output += m_spaces[refSpace2];
		// Next, the ref count (shown closer to the tgt or gloss text than in other view)		
		m_output += endStr; // the ref count number in parentheses
		// That adds extraSpaces spaces, so bump file2at value by extraSpaces to get the
		// correct amount of padding
		//field2at += extraSpaces;
		m_output += m_spaces[diff];
		*/
		m_output += srcLabel;
		m_output += pStruct->src; // source text second
		m_output += pad;
		// Ref counts last
		if (pStruct->numrefs > 1)
		{
			wxString endStr;
			endStr = endStr.Format(m_ref_many, pStruct->numrefs); 
			m_output += endStr; // the ref count number in parentheses
		}
		else
		{
			m_output += m_ref_once;
		}
	}
	return m_output;
}

void RemoveSomeTgtEntries::MakeLinesArray(NonSrcListRecsArray* pGroupsArray, wxArrayString& rLinesArray)
{
	rLinesArray.clear();
	size_t count = pGroupsArray->size();
	size_t i;
	for (i = 0; i < count; ++i)
	{
		NonSrcListRec* pStruct = pGroupsArray->Item(i);
		wxString lineStr = MakeListLine(pStruct);
		rLinesArray.Add(lineStr);
	}
}

void RemoveSomeTgtEntries::MakeLinesForSortedArray(SortedNonSrcListRecsArray* pTargetSortedArray, wxArrayString& rLinesArray)
{
	rLinesArray.clear();
	size_t count = pTargetSortedArray->size();
	size_t i;
	for (i = 0; i < count; ++i)
	{
		NonSrcListRec* pStruct = pTargetSortedArray->Item(i);
		wxString lineStr = MakeListLine(pStruct);
		rLinesArray.Add(lineStr);
	}
}

// bBySrcGroups selects which of the two TrackingArray members, m_leftCheckedArray or
// m_rightCheckedArray, we store the choice in. And that also tells us which of the two is
// the "other" tracking array in which we need to search to find the matching item - it
// will be at a different index in the latter.
// nSelection is the index of the line in the wxListCtrl which shows the checkbox icon
// ticked, or empty.
// This index value also uniquely and reliably indexes the NonSrcListRec struct whose
// values gave rise to the text in the clicked line of the wxListCtrl - we'll use that
// struct in our search for the "other" tracking array's matching index - by searching in
// the other array for the matching struct - this is a simple search because the struct is
// pointed at by both arrays, and so we only need search for pointer identity.
// nChoice must have only values 0 or 1 as it represents the boolean returned by
// IsChecked(). The "other" array's parallel array of size_t item with the matching index
// then is then given the new value for nChoice - this is so that if the user switches
// views using the radio buttons, the tracking array will have the right values for the
// corresponding lines at their different line indices.
// So, in summary, StoreChoice stores the user's choice in both arrays storage structures
// even though only one view is on display at any time - the "other" view's arrays still
// get the choice too
void RemoveSomeTgtEntries::StoreChoice(bool bBySrcGroups, long myline, int nChoice)
{
	// These are the NonSrcListRec* arrays, left radio button's one, then right's
	//NonSrcListRecsArray*	     m_pGroupsArray; // it's partially sorted by src text key
	//SortedNonSrcListRecsArray* m_pUngroupedTgtSortedArray;
	if (bBySrcGroups)
	{
		m_leftCheckedArray[myline] = nChoice;
		// Get the struct pointer for this line
		m_pSelectedRec = m_pGroupsArray->Item(myline);
		// Search for this pointer in the "other" array
     
        // The former .Index(m_pSelectedRec) call here was not 100% reliable - with a phrase
        // of two words, both with diacritics, but two different tgt phrases differing by
        // just a versus a grave at the end of the last word, the wrong phrase got matched.
        // This was unacceptable. 99.99% accuracy won't do - it would result in some wanted
        // entries getting wrongly deleted if the user switched the radio buttons - so that
        // the slightly wrong matchups list of ticked checkboxs got used for the KB
        // deletions. So I rolled my own - it scans linearly and requires match of src,
        // nonsrc and reference count - and did not produce the errors that .Index() did
		m_nOtherLine = SearchInSortedArray(m_pUngroupedTgtSortedArray, m_pSelectedRec);
		// Now set the choice in the parallel tracking array of int
		if (m_nOtherLine != wxNOT_FOUND)
		{
			m_rightCheckedArray[m_nOtherLine] = nChoice;
		}
		else
		{
			// We don't want a search failure to bog the user down with reading a message
			// and cancelling it, just beep to give feedback there was a problem (a
			// failure means that a pair the user wanted pseudo-deleted won't be - that's
			// not a significant problem for continuing the adaptation work)
			wxBell();
		}
	}
	else
	{
		m_rightCheckedArray[myline] = nChoice;
		// Get the struct pointer for this line
		m_pSelectedRec = m_pUngroupedTgtSortedArray->Item(myline);
		// Search for this pointer in the "other" array
		
        // The former .Index(m_pSelectedRec) call here was not 100% reliable - with a phrase
        // of two words, both with diacritics, but two different tgt phrases differing by
        // just a versus a grave at the end of the last word, the wrong phrase got matched.
        // This was unacceptable. 99.99% accuracy won't do - it would result in some wanted
        // entries getting wrongly deleted if the user switched the radio buttons - so that
        // the slightly wrong matchups list of ticked checkboxs got used for the KB
        // deletions. So I rolled my own - it scans linearly and requires match of src,
        // nonsrc and reference count - and it works reliably
		m_nOtherLine = SearchInUnsortedArray(m_pGroupsArray, m_pSelectedRec);
		// Now set the choice in the parallel tracking array of int
		if (m_nOtherLine != wxNOT_FOUND)
		{
			m_leftCheckedArray[m_nOtherLine] = nChoice;
		}
		else
		{
			// We don't want a search failure to bog the user down with reading a message
			// and cancelling it, just beep to give feedback there was a problem (a
			// failure means that a pair the user wanted pseudo-deleted won't be - that's
			// not a significant problem for continuing the adaptation work)
			wxBell();
		}
	}
}

// replaces unreliable .Index() call
long  RemoveSomeTgtEntries::SearchInSortedArray(SortedNonSrcListRecsArray* pArray, NonSrcListRec* pFindThis)
{
    // pArray is the array of structs sorted top to bottom by the nonsrc
    // (ie. usually target text) string
	long count = pArray->size();
	long i;
	NonSrcListRec* pItem = NULL;
	for (i = 0; i < count; i++)
	{
		pItem = pArray->Item(i);
		if ((pItem->nonsrc == pFindThis->nonsrc) &&
			(pItem->src == pFindThis->src) &&
			(pItem->numrefs == pFindThis->numrefs))
		{
			// It's matched, including the reference count
			return i;
		}
	}
	// Failure, return -1
	return (long)wxNOT_FOUND;
}

long  RemoveSomeTgtEntries::SearchInUnsortedArray(NonSrcListRecsArray* pArray, NonSrcListRec* pFindThis)
{
	// pArray is the array of structs partially sorted by the source text, with tgt text
	// sorted within each such group - that is, source text shown first in the wxListCtrl
	long count = pArray->size();
	long i;
	NonSrcListRec* pItem = NULL;
	for (i = 0; i < count; i++)
	{
		pItem = pArray->Item(i);
		if ((pItem->nonsrc == pFindThis->nonsrc) &&
			(pItem->src == pFindThis->src) &&
			(pItem->numrefs == pFindThis->numrefs))
		{
			// It's matched, including the reference count
			return i;
		}
	}
	// Failure, return -1
	return (long)wxNOT_FOUND;
	// The left and right arrays are in sync acording to which checkbox was clicked,
	// despite the lines being different in each 
/* 
    // Identical code but with the lines below "almost" works. What happened? Similar
    // strings with just diacritic difference can be wrongly matched leading to wrong
    // search result in the tgt simple alphabetized list; so abandon .Index() and roll my
    // own -- as done above
		...		
		// Search for this pointer in the "other" array
		m_nOtherLine = m_pUngroupedTgtSortedArray->Index(m_pSelectedRec); // binary chop
		...
	}
	else
	{
		...
		m_nOtherLine = m_pGroupsArray->Index(m_pSelectedRec,FALSE); // linear search
		...
	}
*/
}

// Accessors   
NonSrcListRecsArray* RemoveSomeTgtEntries::GetGroupedArray()
{
	return m_pGroupsArray;
}
SortedNonSrcListRecsArray* RemoveSomeTgtEntries::GetSortedArray()
{
	return m_pUngroupedTgtSortedArray;
}

TrackingArray* RemoveSomeTgtEntries::GetLeftTrackingArray()
{
	return &m_leftCheckedArray;
}

TrackingArray* RemoveSomeTgtEntries::GetRightTrackingArray()
{
	return &m_rightCheckedArray;
}

// This function is called AFTER a radiobutton click to swap to the other view from
// whichever one of the two is current. So the active arrays will be the ones just brought
// into effect by the switch. The just-repopulated listctrl will have all pseudocheckboxes
// showing the emppty box icon. This present function will take the array of int relevant
// to this list, and use it to re-insert at the correct lines the ticked checkbox icons.
// The user is then ready to do more checking or unchecking etc
void RemoveSomeTgtEntries::SetCheckboxes(bool bBySrcGroups)
{
	long i; // index
	int nChoice; // 0 or 1
	long count; // number of array elements (same for either view)
	TrackingArray* pActiveArray = NULL;
	wxListItem item; // we fill it with an item's contents as we scan over the lines
	if (bBySrcGroups)
	{
		// The left radio button, "organised by groups" has just been made active
		pActiveArray = &m_leftCheckedArray;
		count = (long)pActiveArray->size();
		for (i = 0L; i < count; ++i)
		{
			nChoice = (*pActiveArray)[i];
			if (nChoice == 1)
			{
				m_pCheckList->SetItemColumnImage(i, 0, tickedCheckbox);
			}
		}
	}
	else
	{
        // The right radio button, for the list organised top to bottom as an
        // alphabetization of the target text (or gloss if in glossing mode), has just been
        // made active
		pActiveArray = &m_rightCheckedArray;
		count = (unsigned int)pActiveArray->size();
		for (i = 0L; i < count; ++i)
		{
			nChoice = (*pActiveArray)[i];
			if (nChoice == 1)
			{
				m_pCheckList->SetItemColumnImage(i, 0, tickedCheckbox);
			}
		}
	}
}

// We could use the arrays for either the left radio box, or the right, since they are the
// same length and have the same data, only differing in the order of the displayed lines
// and the order of the subfields in each line. We choose to use the arrays for the left
// radio button for our calculations of which src/tgt phrase pairs to remove from the kb,
// and if we are in a shared KB project, also from the remote kbserver (the latter using a
// background thread which unfortunately has to work as a pair per transmission, so if there
// is significant netword latency, the thread may take a long time to complete its work)
void RemoveSomeTgtEntries::GetPhrasePairsForBulkRemoval(TrackingArray* pCheckboxTickedArray, 
		NonSrcListRecsArray* pArray, wxArrayString* pSrcPhrases, wxArrayString* pTgtPhrases)
{
	size_t count = pArray->size();
	size_t nLine;
	int nValue; // will be onlyl 0 (false) or 1 (true)
	NonSrcListRec* pRecord = NULL;
	// We scan for which lines have a ticked checkbox, get the index for each such line,
	// and that gives us the NonSrcListRec elements in pArray which have the source and
	// target phrase strings (also the reference count, but we don't need that) for the kb
	// entries which are to be pseudo-deleted (and removed from kbserver if sharing is on)
	for (nLine = 0; nLine < count; nLine++)
	{
		nValue = pCheckboxTickedArray->Item(nLine); 
		if (nValue == 1)
		{
			pRecord = pArray->Item(nLine);
			pSrcPhrases->Add(pRecord->src);
			pTgtPhrases->Add(pRecord->nonsrc);
		}
	}
}

void RemoveSomeTgtEntries::DoKBExportOfListedEntries(wxFile* pFile, bool bBySrcGroups)
{
	// In unicode app, need a bom otherwise OO Writer etc won't recognise it is getting utf8
	const char* utf8BOM = "\xEF\xBB\xBF\x00";
	CBString bomStr(utf8BOM);
	wxString outputStr;
	outputStr.Empty();
	// If unicode, write the bom to file
#if defined (_UNICODE)
	pFile->Write((char*)bomStr,3);
#endif
	if (bBySrcGroups)
	{
		// This option for when the left radiobutton is on -- the data is grouped
		// alphabetically by source text key, the adaptations are alphabetical within each
		// such group but not across those group boundaries
		size_t count = m_linesArray.size();
		size_t index;
		for (index = 0; index < count; index++)
		{
			outputStr += m_linesArray.Item(index);
			outputStr += m_pApp->m_eolStr;
			pFile->Write(outputStr,wxConvUTF8);
			outputStr.Empty();
		}
	}
	else
	{
		// This option for when the right radiobutton is on -- the data is not grouped
		// alphabetically by source text key, instead, the target text adaptations (or
		// glosses if this is data from a glossing KB) come first and are alphabetically
		// ordered from top to bottom without taking account of the source text spelling
		size_t count = m_linesArrayTgtSorted.size();
		size_t index;
		for (index = 0; index < count; index++)
		{
			outputStr += m_linesArrayTgtSorted.Item(index);
			outputStr += m_pApp->m_eolStr;
			pFile->Write(outputStr,wxConvUTF8);
			outputStr.Empty();
		}
	}
}

// To get checkbox on/off behaviour, requires a few things:
// 1. Don't have a Deselection event trapped -- it unselects anything just selected if a
// selection is made elsewhere; we want our icon changes to 'stick' when other lines are
// clicked.
// 2. OnCheckboxSelection() is always called, whether selecting a non-selected ListItem or
// clicking an already selected one - so use this one event to do it all.
// 3. Use the icon offsets (0 or 1) for the two images as if they are flags.
// 4. Every time this handler is entered, change the current icon to the other state. And,
// at before leaving the handler, programmatically clear the selection. (The selection
// mechanism is a nuisance, if the icon is clicked for a selected line, no selection event
// is fired, and so the handler is not called. This, if allowed to happen, would confuse
// users because they would try to unset a checkbox click that was a mistake by clicking
// it again, and nothing would happen. Solution is to always destroy the selection, then
// next click on a ticked checkbox will work, because the ListItem was set to 'unselected'
// programmatically in the handler.)
// 5. An if-else block, based on the icon offset, is used to change the icon to the other
// one each time the item is clicked. That's it. To the user, it behaves like
// wxCheckListBox, except that we don't show ticked lines with background blue selection.
void RemoveSomeTgtEntries::OnCheckboxSelection(wxListEvent& event)
{
	event.Skip();

	long anIndex = event.GetIndex();
	int anImage = event.GetImage();
	long selectionMask = 0;
	selectionMask |= wxLIST_STATE_SELECTED;
	if (anImage == emptyCheckbox) // both 0
	{
		// It's a click to select, so check that, and if true then change to the ticked
		// checkbox icon; kill the selection too, so that on exit a click on the icon will work
		m_pCheckList->SetItemColumnImage(anIndex, 0, tickedCheckbox);
		m_pCheckList->SetItemState(anIndex,(long)0,selectionMask); // no selection
		StoreChoice(m_bBySrcGroups, anIndex, 1); // stores 1 in appropriate lines of the tracking
											// arrays, m_leftCheckedArray and m_rightCheckedArray
	}
	else // the clicked icon must be the one with offset tickedCheckbox, so user is turning it off
	{
		m_pCheckList->SetItemColumnImage(anIndex, 0, emptyCheckbox);
		m_pCheckList->SetItemState(anIndex,(long)0,selectionMask); // no selection
		StoreChoice(m_bBySrcGroups, anIndex, 0); // stores 1 in appropriate lines of the tracking
											// arrays, m_leftCheckedArray and m_rightCheckedArray
	}
}

void RemoveSomeTgtEntries::PopulateListAsGroups(NonSrcListRecsArray* pGroupsArray)
{
	size_t lineCount = pGroupsArray->size();
	size_t n; // loop index
	wxListItem col[4]; // need four columns, icon, src, tgt, refCount (in that order for
					   // the grouped view), but for the right radio button which is ordered
					   // by the target text from top to bottom, a different populating 
					   // function will do it as icon, tgt, src, refCount
	// start with an empty list
	m_pCheckList->ClearAll();

	// Don't clear pGroupsArray, the stored pointers must stay valid until the
	// RemoveSomeTgtEntries instance is destroyed
	NonSrcListRec* pRec = NULL;
	wxString numStr = _T("");

	// Based on what Mike does in DVCSLogDlg...
	col[0].SetId(0);
	col[0].SetText(_T(" "));
	col[0].SetWidth(22);
	m_pCheckList->InsertColumn(0, col[0]);

	col[1].SetId(1);
	col[1].SetText(_("Source Text"));
	col[1].SetWidth(220);
	m_pCheckList->InsertColumn(1, col[1]);

	col[2].SetId(2);
	if (m_bIsGlossingKB)
	{
		col[2].SetText(_("Gloss"));
	}
	else
	{
		col[2].SetText(_("Translation"));
	}
	col[2].SetWidth(220);
	m_pCheckList->InsertColumn(2, col[2]);

	col[3].SetId(3);
	col[3].SetText(_("Occurs"));
	col[3].SetWidth(64);
	m_pCheckList->InsertColumn(3, col[3]);

    for (n = 0; n < lineCount; n++)
    {
        wxListItem item;
        item.SetId(n);
        m_pCheckList->InsertItem(item);

		pRec = pGroupsArray->Item(n); // stores structs of form: wxString src, 
									  // wxString nonsrc, int numrefs
		// Set the column contents, column zero always has the checkbox icon									  
		m_pCheckList->SetItem(n, 0, _T(""), (int)emptyCheckbox); // sets the unticked checkbox icon in column 0 
		m_pCheckList->SetItem(n, 1, pRec->src); // sets the source text in column 1
		m_pCheckList->SetItemFont(n, *m_pApp->m_pDlgTgtFont); // use target text, 2 pt size, for each line - safest bet

		if (pRec->nonsrc.IsEmpty())
		{
			if (m_bIsGlossingKB)
			{
				m_pCheckList->SetItem(n, 2, m_no_gloss);
			}
			else
			{
				m_pCheckList->SetItem(n, 2, m_no_adaptation);
			}
		}
		else
		{
			m_pCheckList->SetItem(n, 2, pRec->nonsrc); // sets the non-source text in 
								// column 2 (adaption, or gloss if in glossing mode)
		}
		numStr << pRec->numrefs; // convert int to a wxString
		m_pCheckList->SetItem(n, 3, numStr); // puts the reference count in column 3
		numStr.Empty(); // must be emptied after every << operator's use, otherwise next iteration appends
   }
}

void RemoveSomeTgtEntries::PopulateListAsTgtSorted(SortedNonSrcListRecsArray* pNonSrcArray)
{
	size_t lineCount = pNonSrcArray->size();
	size_t n; // loop index
	wxListItem col[4]; // need four columns, icon, src, tgt, refCount (in that order for
					   // the grouped view), but for the right radio button which is ordered
					   // by the target text from top to bottom, a different populating 
					   // function will do it as icon, tgt, src, refCount
	// start with an empty list
	m_pCheckList->ClearAll();

	// Don't clear pGroupsArray, the stored pointers must stay valid until the
	// RemoveSomeTgtEntries instance is destroyed
	NonSrcListRec* pRec = NULL;
	wxString numStr = _T("");

	// Based on what Mike does in DVCSLogDlg...
	col[0].SetId(0);
	col[0].SetText(_T(" "));
	col[0].SetWidth(22);
	m_pCheckList->InsertColumn(0, col[0]);

	col[1].SetId(1);
	if (m_bIsGlossingKB)
	{
		col[1].SetText(_("Gloss"));
	}
	else
	{
		col[1].SetText(_("Translation"));
	}
	col[1].SetWidth(220);
	m_pCheckList->InsertColumn(1, col[1]);

	col[2].SetId(2);
	col[2].SetText(_("Source Text"));
	col[2].SetWidth(220);
	m_pCheckList->InsertColumn(2, col[2]);

	col[3].SetId(3);
	col[3].SetText(_("Occurs"));
	col[3].SetWidth(64);
	m_pCheckList->InsertColumn(3, col[3]);

    for (n = 0; n < lineCount; n++)
    {
        wxListItem item;
        item.SetId(n);
        m_pCheckList->InsertItem(item);

		pRec = pNonSrcArray->Item(n); // stores structs of form: wxString src, 
									  // wxString nonsrc, int numrefs
		// Set the column contents, column zero always has the checkbox icon									  
		m_pCheckList->SetItem(n, 0, _T(""), (int)emptyCheckbox); // sets the unticked checkbox icon in column 0 
		m_pCheckList->SetItemFont(n, *m_pApp->m_pDlgTgtFont); // use target text, 12 pt size, for each line - safest bet

		// Column 1 will be target text, of if in glossing mode, Gloss text
		if (pRec->nonsrc.IsEmpty())
		{
			if (m_bIsGlossingKB)
			{
				m_pCheckList->SetItem(n, 1, m_no_gloss);
			}
			else
			{
				m_pCheckList->SetItem(n, 1, m_no_adaptation);
			}
		}
		else
		{
			m_pCheckList->SetItem(n, 1, pRec->nonsrc);
		}
		// In this view, source text occurs after the target or gloss text
		m_pCheckList->SetItem(n, 2, pRec->src); // sets the source text in column 2
		// Put ref count last -- it's the count for how many target (or gloss) refs for
		// the given source text of the pair, not the count of how many times the src text
		// occurs
		numStr << pRec->numrefs; // convert int to a wxString
		m_pCheckList->SetItem(n, 3, numStr); // puts the reference count in column 3
		numStr.Empty(); // must be emptied after every << operator's use, otherwise next iteration appends
   }
}
