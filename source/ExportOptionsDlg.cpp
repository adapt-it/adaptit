/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ExportOptionsDlg.cpp
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the implementation file for the CExportOptionsDlg class. 
/// The CExportOptionsDlg class provides a sub-dialog accessible either from the 
/// ExportSaveAsDlg dialog, or the ExportInterlinearDlg. It provides options whereby
/// the user can filter certain markers from the exported output; and also indicate
/// whether back translations, free translations, and notes are to be formatted as
/// boxed paragraphs, rows of interlinear tables, balloon comments, or footnotes.
/// \derivation		The CExportOptionsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ExportOptionsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ExportOptionsDlg.h"
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
#include  "wx/checklst.h"

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "ExportOptionsDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// The following globals defined here are for use as extern variables in 
// ExportOptionsDlg.cpp and the export routines in the View.
wxArrayString m_exportBareMarkers;
wxArrayString m_exportMarkerAndDescriptions;
wxArrayInt m_exportFilterFlags;
wxArrayInt m_exportFilterFlagsBeforeEdit; // to detect any changes to list of markers for export

// defaults for the following are set in global space since in ExportSaveAsDlg so we can preserve
// their state if user reopens the ExportOptionsDlg
extern bool bPlaceFreeTransInRTFText; 
extern bool bPlaceBackTransInRTFText;
extern bool bPlaceAINotesInRTFText;
extern bool bExportAll;
extern bool bExportSelectedMarkersOnly;
extern bool bPlaceFreeTransCheckboxEnabled;	// ExportSaveAsDlg initializes all 3 checkboxes to disabled 
										// (which ExportOptionsDlg can change)
extern bool bPlaceBackTransCheckboxEnabled;	// ""
extern bool bPlaceAINotesCheckboxEnabled;	// ""


extern const wxChar* filterMkr;
extern bool bExportToRTF; // global declared in ExportSaveAsDlg.cpp
extern bool bExportAsRTFInterlinear;
extern wxString embeddedWholeMkrs; // whm added 27Nov07

// event handler table
BEGIN_EVENT_TABLE(CExportOptionsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CExportOptionsDlg::InitDialog)// not strictly necessary for dialogs based on wxDialog
	EVT_BUTTON(wxID_CANCEL, CExportOptionsDlg::OnCancel)
	EVT_RADIOBUTTON(IDC_RADIO_EXPORT_SELECTED_MARKERS, CExportOptionsDlg::OnBnClickedRadioExportSelectedMarkers)
	EVT_RADIOBUTTON(IDC_RADIO_EXPORT_ALL, CExportOptionsDlg::OnBnClickedRadioExportAll)
	EVT_LISTBOX(IDC_LIST_SFMS, CExportOptionsDlg::OnLbnSelchangeListSfms)
	EVT_CHECKLISTBOX(IDC_LIST_SFMS, CExportOptionsDlg::OnCheckListBoxToggle)
	EVT_BUTTON(IDC_BUTTON_INCLUDE_IN_EXPORT, CExportOptionsDlg::OnBnClickedButtonIncludeInExport)
	EVT_BUTTON(IDC_BUTTON_EXCLUDE_FROM_EXPORT, CExportOptionsDlg::OnBnClickedButtonFilterOutFromExport)
	EVT_BUTTON(IDC_BUTTON_UNDO, CExportOptionsDlg::OnBnClickedButtonUndo)
	EVT_CHECKBOX(IDC_CHECK_PLACE_FREE_TRANS, CExportOptionsDlg::OnBnClickedCheckPlaceFreeTrans)
	EVT_CHECKBOX(IDC_CHECK_PLACE_BACK_TRANS, CExportOptionsDlg::OnBnClickedCheckPlaceBackTrans)
	EVT_CHECKBOX(IDC_CHECK_PLACE_AI_NOTES, CExportOptionsDlg::OnBnClickedCheckPlaceAiNotes)
	EVT_CHECKBOX(IDC_CHECK_INTERLINEAR_PLACE_AI_NOTES, CExportOptionsDlg::OnBnClickedCheckInterlinearPlaceAiNotes)
	EVT_CHECKBOX(IDC_CHECK_INTERLINEAR_PLACE_BACK_TRANS, CExportOptionsDlg::OnBnClickedCheckInterlinearPlaceBackTrans)
	EVT_CHECKBOX(IDC_CHECK_INTERLINEAR_PLACE_FREE_TRANS, CExportOptionsDlg::OnBnClickedCheckInterlinearPlaceFreeTrans)
END_EVENT_TABLE()


CExportOptionsDlg::CExportOptionsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Export Filter/Options"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pExportOptionsSizer = ExportOptionsDlgFunc(this, TRUE, TRUE);
	// The declaration is: ExportOptionsDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// get pointers to our controls
	pRadioExportAll = (wxRadioButton*)FindWindowById(IDC_RADIO_EXPORT_ALL);
	wxASSERT(pRadioExportAll != NULL);
	pRadioSelectedMarkersOnly = (wxRadioButton*)FindWindowById(IDC_RADIO_EXPORT_SELECTED_MARKERS);
	wxASSERT(pRadioSelectedMarkersOnly != NULL);
	pButtonIncludeInExport = (wxButton*)FindWindowById(IDC_BUTTON_INCLUDE_IN_EXPORT);
	wxASSERT(pButtonIncludeInExport != NULL);
	pButtonFilterOutFromExport = (wxButton*)FindWindowById(IDC_BUTTON_EXCLUDE_FROM_EXPORT);
	wxASSERT(pButtonFilterOutFromExport != NULL);
	pButtonUndo = (wxButton*)FindWindowById(IDC_BUTTON_UNDO);
	wxASSERT(pButtonUndo);
	pStaticSpecialNote = (wxStaticText*)FindWindowById(IDC_STATIC_SPECIAL_NOTE);
	wxASSERT(pStaticSpecialNote);
	m_pListBoxSFMs = (wxCheckListBox*)FindWindowById(IDC_LIST_SFMS);
	wxASSERT(m_pListBoxSFMs);

	// controls below added for version 3
	pCheckPlaceFreeTrans = (wxCheckBox*)FindWindowById(IDC_CHECK_PLACE_FREE_TRANS);
	wxASSERT(pCheckPlaceFreeTrans);
	pCheckPlaceBackTrans = (wxCheckBox*)FindWindowById(IDC_CHECK_PLACE_BACK_TRANS);
	wxASSERT(pCheckPlaceBackTrans);
	pCheckPlaceAINotes = (wxCheckBox*)FindWindowById(IDC_CHECK_PLACE_AI_NOTES);
	wxASSERT(pCheckPlaceAINotes);
	pCheckPlaceFreeTransInterlinear = (wxCheckBox*)FindWindowById(IDC_CHECK_INTERLINEAR_PLACE_FREE_TRANS);
	wxASSERT(pCheckPlaceFreeTransInterlinear);
	pCheckPlaceBackTransInterlinear = (wxCheckBox*)FindWindowById(IDC_CHECK_INTERLINEAR_PLACE_BACK_TRANS);
	wxASSERT(pCheckPlaceBackTransInterlinear);
	pCheckPlaceAINotesInterlinear = (wxCheckBox*)FindWindowById(IDC_CHECK_INTERLINEAR_PLACE_AI_NOTES);
	wxASSERT(pCheckPlaceAINotesInterlinear);

	pTextCtrlAsStaticExportOptions = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_EXPORT_OPTIONS);
	wxASSERT(pTextCtrlAsStaticExportOptions != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticExportOptions->SetBackgroundColour(backgrndColor);

	// TODO: should the following be moved to global space?
	// whm added 27Nov07 the \id marker to the list that should not be
	// excludable from exports. In the MFC version these markers are grayed
	// out; in the wx version they are simply omitted from the excludable list.
	// I've modified the list to use embeddedWholeMkrs plus \id, \c and \v which
	// are in addition to the embedded markers.
	excludedWholeMkrs = _T("\\id \\c \\v ") + embeddedWholeMkrs; // + _T("\\c \\v \\fr \\fk \\fq \\fqa \\ft \\fdc \\fv \\fm \\xo \\xt \\xk \\xq \\xdc ");

}

CExportOptionsDlg::~CExportOptionsDlg() // destructor
{
	
}

void CExportOptionsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class

	// Note on behavior of ExportOptionsDlg, and scope of changes, effect of OK, Undo and Cancel:
	// InitDialog is called each time the user accesses the ExportOptionsDlg which may be more than
	// once while the ExportSaveAsDlg dialog is open. The m_exportFilterFlagsBeforeEdit array is set
	// inititially (with all flags set to FALSE) just once when the parent ExportSaveAsDlg dialog is 
	// first opened. It does not change as long as the ExportSaveAsDlg dialog is open. The similar
	// array called m_exportFilterFlags keeps a record of any changes the user makes to the export
	// filter settings, which may be more than one visit to the ExportOptionsDlg dialog before starting
	// the export. If the user dismisses the ExportOptionsDlg dialog with OK, any changes to the export 
	// filter settings are recorded in m_exportFilterFlags. Hence, on successive accesses of the 
	// ExportOptionsDlg the export filter state of markers should be preserved, so that the list box
	// always initially reflects the current flag values stored in m_exportFilterFlags. Any changes
	// the user makes during the current session of the ExportOptionsDlg are copied to m_exportFilterFlags 
	// if the user presses OK. If the user types the "Undo" button, only current session changes made to 
	// the filter state of markers in the list box are undone, i.e., they revert back to the values in 
	// m_exportFilterFlags. 
	// The "Undo" button only effects the filter markers in the list box, not the check boxes 
	// at the bottom of the dialog effecting back translations, free translations, and Notes placement
	// (these are only activated for RTF exports). Any time the user types "Cancel" it rolls back
	// all export filter changes made during the current session of the ExportSaveAsDlg dialog.

	// set initial state of controls
	m_pListBoxSFMs->Enable(FALSE);
	pButtonIncludeInExport->Enable(FALSE);
	pButtonFilterOutFromExport->Enable(FALSE);
	pTextCtrlAsStaticExportOptions->Enable(FALSE);
	pRadioExportAll->SetValue(bExportAll);
	pRadioSelectedMarkersOnly->SetValue(bExportSelectedMarkersOnly);
	pButtonUndo->Enable(FALSE);

	// BEW added 1Feb08 to increase the item height from default 16, to 17 to allow for
	// items with Times New Roman, 11 point size, for Azeri. Comment out after producing? (I could leave it)
	// whm: I don't think this is needed in the wx version
	//m_pListBoxSFMs->SetItemHeight(0,17); // to handle 11 point font


	// set initial variable states

	bFilterListChangesMade = FALSE;

	// In ExportOptionsDlg we show the Interlinear related check boxes only when the Export Options
	// dialog is called from ExportInterlinearDlg
	if (bExportAsRTFInterlinear)
	{
		pCheckPlaceFreeTransInterlinear->Show(TRUE);
		pCheckPlaceBackTransInterlinear->Show(TRUE);
		pCheckPlaceAINotesInterlinear->Show(TRUE);
		pCheckPlaceFreeTrans->Show(FALSE);
		pCheckPlaceBackTrans->Show(FALSE);
		pCheckPlaceAINotes->Show(FALSE);
	}
	else
	{
		pCheckPlaceFreeTransInterlinear->Show(FALSE);
		pCheckPlaceBackTransInterlinear->Show(FALSE);
		pCheckPlaceAINotesInterlinear->Show(FALSE);
		pCheckPlaceFreeTrans->Show(TRUE);
		pCheckPlaceBackTrans->Show(TRUE);
		pCheckPlaceAINotes->Show(TRUE);
	}

	// set checkbox control defaults for free trans, back trans and AINotes
	if (bExportToRTF)
	{
		if (bExportAsRTFInterlinear)
		{
			// Enable and set appropriate state of the Interlinear related checkboxes
			pCheckPlaceFreeTransInterlinear->Enable(bPlaceFreeTransCheckboxEnabled);
			pCheckPlaceFreeTransInterlinear->SetValue(bPlaceFreeTransInRTFText);
			pCheckPlaceBackTransInterlinear->Enable(bPlaceBackTransCheckboxEnabled);
			pCheckPlaceBackTransInterlinear->SetValue(bPlaceBackTransInRTFText);
			pCheckPlaceAINotesInterlinear->Enable(bPlaceAINotesCheckboxEnabled);
			pCheckPlaceAINotesInterlinear->SetValue(bPlaceAINotesInRTFText);
		}
		else
		{
			// Enable and set appropriate state of the NON-Interlinear related checkboxes
			pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
			pCheckPlaceFreeTrans->SetValue(bPlaceFreeTransInRTFText);
			pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
			pCheckPlaceBackTrans->SetValue(bPlaceBackTransInRTFText);
			pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
			pCheckPlaceAINotes->SetValue(bPlaceAINotesInRTFText);
		}
		pStaticSpecialNote->Enable(TRUE);
	}
	else
	{
		// in SFM export mode disable all checkboxes and static text at bottom
		pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
		pCheckPlaceFreeTrans->SetValue(bPlaceFreeTransInRTFText);
		pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
		pCheckPlaceBackTrans->SetValue(bPlaceBackTransInRTFText);
		pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
		pCheckPlaceAINotes->SetValue(bPlaceAINotesInRTFText);
		pStaticSpecialNote->Enable(FALSE);
	}

	// make list box enabled along with "selected markers" radio button
	m_pListBoxSFMs->Enable(bExportSelectedMarkersOnly);
	if (bExportSelectedMarkersOnly && m_pListBoxSFMs->GetCount() > 0)
	{
		m_pListBoxSFMs->SetFocus();
		m_pListBoxSFMs->SetSelection(0);
	}

	// Note: InitDialog() here gets called each time the export options dialog is called
	// while the ExportSaveAsDlg is being displayed. The user may assess the export options
	// dialog several times while the ExportSaveAsDlg is open; each time it comes it, it
	// should preserve any filtering changes that were made and accepted by a click on the
	// Export options OK button. Therefore we should not here copy the array values from
	// m_exportFilterFlagsBeforeEdit to m_exportFilterFlags and the following line should
	// remain commented out.
	//m_exportFilterFlags.Copy(m_exportFilterFlagsBeforeEdit);

	// load the list box with sfms used in the current doc
	LoadActiveSFMListBox(); // sets all flags to FALSE
	
	// Set flags of list box items according to flags stored within m_exportFilterFlags
	TransferFilterFlagValuesBetweenCheckListBoxAndArray(ToControls);

	if (m_pListBoxSFMs->IsEnabled())
		m_pListBoxSFMs->SetFocus();
	
	pExportOptionsSizer->Layout(); // resizes the dialog
}

//The function either calls EndModal(wxID_CANCEL) if the dialog is modal, or sets the 
//return value to wxID_CANCEL and calls Show(false) if the dialog is modeless.
void CExportOptionsDlg::OnCancel(wxCommandEvent& event)
{

	// User clicked cancel so undo any changes to the list box data structures
	OnBnClickedButtonUndo(event); // this also sets bFilterListChangesMade to FALSE;

	// the following bools should be set same as they are in ExportSaveAsDlg's InitDialog
	bExportAll = TRUE; // start with export of all markers radio button selected in Export Options
	bExportSelectedMarkersOnly = FALSE;
	bPlaceFreeTransInRTFText = TRUE;
	bPlaceBackTransInRTFText = FALSE;
	bPlaceAINotesInRTFText = TRUE; // start with AI Notes in balloon text modified 17Jan06
	bPlaceFreeTransCheckboxEnabled = FALSE; // start with all checkboxes disabled (ExportOptionsDlg can change)
	bPlaceBackTransCheckboxEnabled = FALSE;
	bPlaceAINotesCheckboxEnabled = FALSE;
	
	EndModal(wxID_CANCEL); //wxDialog::OnCancel(event); // need to call EndModal here
}

void CExportOptionsDlg::OnBnClickedRadioExportAll(wxCommandEvent& event)
{
	// user clicked on Export all markers radio button
	// First Undo any changes made to export filtering state
	OnBnClickedButtonUndo(event);
	// disable selected markers list, static text, and Filter Out, Include In Export, Undo buttons
	m_pListBoxSFMs->Enable(FALSE);
	pButtonIncludeInExport->Enable(FALSE);
	pButtonFilterOutFromExport->Enable(FALSE);
	pTextCtrlAsStaticExportOptions->Enable(FALSE);
	pButtonUndo->Enable(FALSE);
	// only enable these below if we're doing RTF export
	if (bExportToRTF)
	{
		if (bExportAsRTFInterlinear)
		{
			bPlaceFreeTransCheckboxEnabled = TRUE; // made TRUE; in v3.0.1 once table row added
			bPlaceBackTransCheckboxEnabled = TRUE; // made TRUE; " " "
			bPlaceAINotesCheckboxEnabled = TRUE;
			pCheckPlaceFreeTransInterlinear->Enable(bPlaceFreeTransCheckboxEnabled);
			pCheckPlaceBackTransInterlinear->Enable(bPlaceBackTransCheckboxEnabled);
			pCheckPlaceAINotesInterlinear->Enable(bPlaceAINotesCheckboxEnabled);
		}
		else
		{
			bPlaceFreeTransCheckboxEnabled = TRUE;
			bPlaceBackTransCheckboxEnabled = TRUE;
			bPlaceAINotesCheckboxEnabled = TRUE;
			pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
			pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
			pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
		}
		pStaticSpecialNote->Enable(TRUE);
	}
	bExportAll = TRUE;
	bExportSelectedMarkersOnly = FALSE;
	// User wants to export all which was the beginning default, so copy the
	// flags to m_exportFilterFlags as they were in m_exportFilterFlagsBeforeEdit
	// (effectively this makes all flags in m_exportFilterFlags FALSE, i.e., no
	// markers are filtered out of the export)
	//m_exportFilterFlags.Copy(m_exportFilterFlagsBeforeEdit);
	m_exportFilterFlags.Clear();
	int ct;
	for (ct = 0; ct < (int)m_exportFilterFlagsBeforeEdit.GetCount(); ct++)
	{
		// copy all items from m_unknownMarkers into saveUnknownMarkers
		m_exportFilterFlags.Add(m_exportFilterFlagsBeforeEdit.Item(ct));
	}
}

void CExportOptionsDlg::OnBnClickedRadioExportSelectedMarkers(wxCommandEvent& WXUNUSED(event))
{
	// user clicked on Export Selected Markers Only
	// enable selected markers list
	m_pListBoxSFMs->Enable(TRUE);
	pButtonIncludeInExport->Enable(FALSE); // disabled until user selects list item
	pButtonFilterOutFromExport->Enable(FALSE); // disabled until user selects list item
	pTextCtrlAsStaticExportOptions->Enable(TRUE);
	bExportAll = FALSE;
	bExportSelectedMarkersOnly = TRUE;
	m_pListBoxSFMs->SetFocus();
	wxASSERT(m_pListBoxSFMs->GetCount() > 0);
	m_pListBoxSFMs->SetSelection(0);
	wxString selectedString = m_pListBoxSFMs->GetStringSelection();
	int newIndex = gpApp->FindArrayString(selectedString,&m_exportMarkerAndDescriptions); // get index into m_atSfmFlags
	bool bFilter;
	if (m_exportFilterFlags.Item(newIndex) == 0)
		bFilter = FALSE;
	else
		bFilter = TRUE;
	pButtonFilterOutFromExport->Enable(bFilter != TRUE); // same logic as filterPage buttons
	pButtonIncludeInExport->Enable(bFilter == TRUE); // same logic as filterPage buttons
}

void CExportOptionsDlg::OnLbnSelchangeListSfms(wxCommandEvent& WXUNUSED(event))
{
	// wx note: Under Linux/GTK ...Selchanged... listbox events can be triggered after a call to Clear()
	// so we must check to see if the listbox contains no items and if so return immediately
	if (m_pListBoxSFMs->GetCount() == 0)
		return;

	// Enable "Filter Out" or "Include in Export" button depending
	// on the state of a selected item
	int nSel;
	nSel = m_pListBoxSFMs->GetSelection();
	if (nSel != -1) //LB_ERR
	{
		// get the current state of the selected item
		if (nSel < (int)m_exportFilterFlags.GetCount())
		{
			wxString selectedString = m_pListBoxSFMs->GetStringSelection();
			int newIndex = gpApp->FindArrayString(selectedString,&m_exportMarkerAndDescriptions); // get index into m_exportFilterFlags
			bool bFilter;
			if (m_exportFilterFlags.Item(newIndex) == 0)
				bFilter = FALSE;
			else
				bFilter = TRUE;
			pButtonFilterOutFromExport->Enable(bFilter != TRUE); // same logic as filterPage buttons
			pButtonIncludeInExport->Enable(bFilter == TRUE); // same logic as filterPage buttons
		}
	}
	m_pListBoxSFMs->SetSelection(nSel);
	pButtonUndo->Enable(bFilterListChangesMade);
}

void CExportOptionsDlg::OnCheckListBoxToggle(wxCommandEvent& event)
{
	// Note: the checkbox that gets toggled, may not be the item currently selected in the
	// listbox, since the user can click directly on a checkbox without first selecting the
	// listbox line containing the checkbox. The code below automatically selects the line
	// if the checkbox is directly clicked on to check or uncheck it.
	// The Linux (GTK) and MAC ports do not yet implement the background coloring of individual items
	// of a wxCheckListBox so we cannot yet represent "invalid" choices in the list by a gray 
	// background under GTK or the MAC. Possible design solutions: 
	// 1. Don't list any markers in the list that we don't want the user to select 
	// 2. List all markers, but the GUI gives a beep or message to the user if he tries to 
	//    check the box of an "invalid" choice.
	// 3. Implement MFC like behavior only for Windows port, but behavior 1 or 2 for the
	//    Linux and Mac ports.
	// A naive user would probably be less confused with solution 1 (don't list any invalid markers),
	// so I've implemented that design solution in LoadActiveSFMListBox().
	int nItem = event.GetInt();
	wxString itemString = m_pListBoxSFMs->GetString(nItem);
	if (m_pListBoxSFMs->IsChecked(nItem))
	{
		int newIndex = gpApp->FindArrayString(itemString,&m_exportMarkerAndDescriptions); // index into m_asSfmFlags
		wxASSERT(newIndex < (int)m_exportFilterFlags.GetCount());
		wxASSERT(m_exportFilterFlags[newIndex] == FALSE);
		// also set the flags to indicate the change
		m_exportFilterFlags[newIndex] = TRUE;

		// if the user filters out \free, \bt or \note we need to disable the
		// corresponding checkboxes in the dialog, but only if we're doing RTF export
		if (bExportToRTF)
		{
			if (m_exportBareMarkers.Item(newIndex) == _T("free"))
			{
				// whm added 29Nov07 we also need to set the flag bPlaceFreeTransInRTFText to FALSE
				// here since the \free material will be filtered out of exports
				bPlaceFreeTransInRTFText = FALSE;
				bPlaceFreeTransCheckboxEnabled = FALSE;
				if (bExportAsRTFInterlinear)
					pCheckPlaceFreeTransInterlinear->Enable(bPlaceFreeTransCheckboxEnabled);
				else
					pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
			}
			if (m_exportBareMarkers.Item(newIndex).Find(_T("bt")) != -1) // any \bt...
			{
				// whm added 29Nov07 we also need to set the flag bPlaceBackTransInRTFText to FALSE
				// here since the \bt material will be filtered out of exports
				bPlaceBackTransInRTFText = FALSE;
				bPlaceBackTransCheckboxEnabled = FALSE;
				if (bExportAsRTFInterlinear)
					pCheckPlaceBackTransInterlinear->Enable(bPlaceBackTransCheckboxEnabled);
				else
					pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
			}
			if (m_exportBareMarkers.Item(newIndex) == _T("note"))
			{
				// whm comment 29Nov07: AI Notes never get placed into a table row, therefore
				// there won't be any empty row to be concerned with if they are filtered from
				// exports
				bPlaceAINotesCheckboxEnabled = FALSE;
				if (bExportAsRTFInterlinear)
					pCheckPlaceAINotesInterlinear->Enable(bPlaceAINotesCheckboxEnabled);
				else
					pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
			}
		}

		// the Filter Out and Include in Export buttons need to reverse
		// their previous enabled state
		pButtonFilterOutFromExport->Enable(FALSE); // TODO: remove the "Filter Out" button ???
		pButtonIncludeInExport->Enable(TRUE); // TODO: remove the "Include in Export" button ???
		pButtonUndo->Enable(TRUE);
		m_pListBoxSFMs->SetFocus();
		m_pListBoxSFMs->SetSelection(nItem);
		bFilterListChangesMade = TRUE;
	}
	else
	{
		int newIndex = gpApp->FindArrayString(itemString,&m_exportMarkerAndDescriptions); // index into m_asSfmFlags
		wxASSERT(newIndex < (int)m_exportFilterFlags.GetCount());
		wxASSERT(m_exportFilterFlags[newIndex] == TRUE);
		// also set the flags to indicate the change
		m_exportFilterFlags[newIndex] = FALSE;
		// the Filter Out and Include in Export buttons need to reverse
		// their previous enabled state

		// if the user includes out \free, \bt or \note we need to enable the
		// corresponding checkboxes in the dialog, but only if we're doing RTF export
		if (bExportToRTF)
		{
			if (m_exportBareMarkers.Item(newIndex) == _T("free"))
			{
				// whm added 29Nov07 we also need to set the flag bPlaceFreeTransInRTFText to TRUE
				// here since the \free material will be included in exports
				bPlaceFreeTransInRTFText = TRUE;
				bPlaceFreeTransCheckboxEnabled = TRUE;
				if (bExportAsRTFInterlinear)
					pCheckPlaceFreeTransInterlinear->Enable(bPlaceFreeTransCheckboxEnabled);
				else
					pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
			}
			if (m_exportBareMarkers.Item(newIndex).Find(_T("bt")) != -1) // any \bt...
			{
				// whm added 29Nov07 we also need to set the flag bPlaceBackTransInRTFText to TRUE
				// here since the \bt material will be included in exports
				bPlaceBackTransInRTFText = TRUE;
				bPlaceBackTransCheckboxEnabled = TRUE;
				if (bExportAsRTFInterlinear)
					pCheckPlaceBackTransInterlinear->Enable(bPlaceBackTransCheckboxEnabled);
				else
					pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
			}
			if (m_exportBareMarkers.Item(newIndex) == _T("note"))
			{
				// whm comment 29Nov07: AI Notes never get placed into a table row, therefore
				// there won't be any empty row to be concerned with if they are filtered from
				// exports
				bPlaceAINotesCheckboxEnabled = TRUE;
				if (bExportAsRTFInterlinear)
					pCheckPlaceAINotesInterlinear->Enable(bPlaceAINotesCheckboxEnabled);
				else
					pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
			}
		}

		pButtonFilterOutFromExport->Enable(TRUE);
		pButtonIncludeInExport->Enable(FALSE);
		pButtonUndo->Enable(TRUE);
		m_pListBoxSFMs->SetFocus();
		m_pListBoxSFMs->SetSelection(nItem); // make the selection highlight line of box just clicked
		bFilterListChangesMade = TRUE;
	}
}

void CExportOptionsDlg::OnBnClickedButtonFilterOutFromExport(wxCommandEvent& WXUNUSED(event))
{
	// We can assume that the list item's check box is unchecked
	// when this function is called. 
	// This OnBnClickedButtonFilterOutFromExport() method adds a check in the 
	// checkbox of the selected item in the m_listBoxSFMs, but does nothing 
	// to the global USFMAnalysis structs which retain their current attributes 
	// throughout the export process. Only the array of filter flags m_exportFilterFlags 
	// and the corresponding array of markers and descriptions called
	// m_exportBareMarkers and m_exportMarkerAndDescriptions are changed during 
	// processing in this ExportOptionsDlg class.
	// If the user cancels the containing dialog, no changes will be made to the
	// global data structures.
	// The calling routines (ExportSaveAsDlg which is in turn called by either 
	// OnFileExportSource or DoExportTgt in the View) should check for any changes 
	// by storing the contents of m_exportFilterFlags before edit and comparing that with 
	// m_exportFilterFlags after the user dismisses the dialog with the OK button. 
	// If the comparison shows that changes were made the caller should then
	// make the required changes in the actual export routines.
	int curSel = m_pListBoxSFMs->GetSelection();
	wxString selectedString = m_pListBoxSFMs->GetStringSelection();
	int newIndex = gpApp->FindArrayString(selectedString,&m_exportMarkerAndDescriptions); // index into m_asSfmFlags
	wxASSERT(newIndex < (int)m_exportFilterFlags.GetCount());
	wxASSERT(m_exportFilterFlags[newIndex] == FALSE);
	m_pListBoxSFMs->Check(curSel, TRUE);
	// also set the flags to indicate the change
	m_exportFilterFlags[newIndex] = TRUE;

	// if the user filters out \free, \bt or \note we need to disable the
	// corresponding checkboxes in the dialog, but only if we're doing RTF export
	if (bExportToRTF)
	{
		if (m_exportBareMarkers.Item(newIndex) == _T("free"))
		{
			// whm added 29Nov07 we also need to set the flag bPlaceFreeTransInRTFText to FALSE
			// here since the \free material will be filtered out of exports
			bPlaceFreeTransInRTFText = FALSE;
			bPlaceFreeTransCheckboxEnabled = FALSE;
			if (bExportAsRTFInterlinear)
				pCheckPlaceFreeTransInterlinear->Enable(bPlaceFreeTransCheckboxEnabled);
			else
				pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
		}
		if (m_exportBareMarkers.Item(newIndex).Find(_T("bt")) != -1) // any \bt...
		{
			// whm added 29Nov07 we also need to set the flag bPlaceBackTransInRTFText to FALSE
			// here since the \bt material will be filtered out of exports
			bPlaceBackTransInRTFText = FALSE;
			bPlaceBackTransCheckboxEnabled = FALSE;
			if (bExportAsRTFInterlinear)
				pCheckPlaceBackTransInterlinear->Enable(bPlaceBackTransCheckboxEnabled);
			else
				pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
		}
		if (m_exportBareMarkers.Item(newIndex) == _T("note"))
		{
			// whm comment 29Nov07: AI Notes never get placed into a table row, therefore
			// there won't be any empty row to be concerned with if they are filtered from
			// exports
			bPlaceAINotesCheckboxEnabled = FALSE;
			if (bExportAsRTFInterlinear)
				pCheckPlaceAINotesInterlinear->Enable(bPlaceAINotesCheckboxEnabled);
			else
				pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
		}
	}

	// the Filter Out and Include in Export buttons need to reverse
	// their previous enabled state
	pButtonFilterOutFromExport->Enable(FALSE);
	pButtonIncludeInExport->Enable(TRUE);
	pButtonUndo->Enable(TRUE);
	m_pListBoxSFMs->SetFocus();
	m_pListBoxSFMs->SetSelection(curSel);
	bFilterListChangesMade = TRUE;
}

void CExportOptionsDlg::OnBnClickedButtonIncludeInExport(wxCommandEvent& WXUNUSED(event))
{
	// We can assume that the list item's check box is checked
	// when this function is called. 
	// This OnBnClickedButtonIncludeInExport() method removes the check in the 
	// checkbox of the selected item in the m_listBoxSFMs, but does nothing 
	// to the global USFMAnalysis structs which retain their current attributes 
	// throughout the export process. Only the array of filter flags m_exportFilterFlags 
	// and the corresponding array of markers and descriptions called
	// m_exportBareMarkers and m_exportMarkerAndDescriptions are changed during 
	// processing in this ExportOptionsDlg class.
	// If the user cancels the containing dialog, no changes will be made to the
	// global data structures.
	// The calling routines (ExportSaveAsDlg which is in turn called by either 
	// OnFileExportSource or DoExportTgt in the View) should check for any changes 
	// by storing the contents of m_exportFilterFlags before edit and comparing that with 
	// m_exportFilterFlags after the user dismisses the dialog with the OK button. 
	// If the comparison shows that changes were made the caller should then
	// make the required changes in the actual export routines.
	int curSel = m_pListBoxSFMs->GetSelection();
	wxString selectedString = m_pListBoxSFMs->GetStringSelection();

	int newIndex = gpApp->FindArrayString(selectedString,&m_exportMarkerAndDescriptions); // index into m_asSfmFlags
	wxASSERT(newIndex < (int)m_exportFilterFlags.GetCount());
	wxASSERT(m_exportFilterFlags[newIndex] == TRUE);
	m_pListBoxSFMs->Check(curSel, FALSE);
	// also set the flags to indicate the change
	m_exportFilterFlags[newIndex] = FALSE;
	// the Filter Out and Include in Export buttons need to reverse
	// their previous enabled state

	// if the user includes out \free, \bt or \note we need to enable the
	// corresponding checkboxes in the dialog, but only if we're doing RTF export
	if (bExportToRTF)
	{
		if (m_exportBareMarkers.Item(newIndex) == _T("free"))
		{
			// whm added 29Nov07 we also need to set the flag bPlaceFreeTransInRTFText to TRUE
			// here since the \free material will be included in exports
			bPlaceFreeTransInRTFText = TRUE;
			bPlaceFreeTransCheckboxEnabled = TRUE;
			if (bExportAsRTFInterlinear)
				pCheckPlaceFreeTransInterlinear->Enable(bPlaceFreeTransCheckboxEnabled);
			else
				pCheckPlaceFreeTrans->Enable(bPlaceFreeTransCheckboxEnabled);
		}
		if (m_exportBareMarkers.Item(newIndex).Find(_T("bt")) != -1) // any \bt...
		{
			// whm added 29Nov07 we also need to set the flag bPlaceBackTransInRTFText to TRUE
			// here since the \bt material will be included in exports
			bPlaceBackTransInRTFText = TRUE;
			bPlaceBackTransCheckboxEnabled = TRUE;
			if (bExportAsRTFInterlinear)
				pCheckPlaceBackTransInterlinear->Enable(bPlaceBackTransCheckboxEnabled);
			else
				pCheckPlaceBackTrans->Enable(bPlaceBackTransCheckboxEnabled);
		}
		if (m_exportBareMarkers.Item(newIndex) == _T("note"))
		{
			// whm comment 29Nov07: AI Notes never get placed into a table row, therefore
			// there won't be any empty row to be concerned with if they are filtered from
			// exports
			bPlaceAINotesCheckboxEnabled = TRUE;
			if (bExportAsRTFInterlinear)
				pCheckPlaceAINotesInterlinear->Enable(bPlaceAINotesCheckboxEnabled);
			else
				pCheckPlaceAINotes->Enable(bPlaceAINotesCheckboxEnabled);
		}
	}

	pButtonFilterOutFromExport->Enable(TRUE);
	pButtonIncludeInExport->Enable(FALSE);
	pButtonUndo->Enable(TRUE);
	m_pListBoxSFMs->SetFocus();
	m_pListBoxSFMs->SetSelection(curSel);
	bFilterListChangesMade = TRUE;
}

void CExportOptionsDlg::LoadActiveSFMListBox()
{
	// LoadActiveSFMListBox fills the CCheckListBox with the sfms that are 
	// currently active in the document. These were collected in the caller 
	// (from the Doc's source phrase m_markers members) and currently exist 
	// in the CStringArrays m_exportBareMarkers and m_exportMarkerAndDescriptions, 
	// and have corresponding export flags in the CUIntArrays m_exportFilterFlags 
	// and m_exportFilterFlagsBeforeEdit. 
	// The currently filtered sfms are listed with [FILTERED] prefixed to the 
	// description. Unknown markers are listed with [UNKNOWN MARKER] as their 
	// description. We list all markers that are used in the document, and if the 
	// user excludes things illogically, then the output will reflect that.
	
	int nMarkerIndex = 0;
	wxString lbStr;
	// Make a wxString with all markers to be grayed out. These include all the embedded
	// content markers for footnotes, endnotes and crossrefs, as well as chapter, verse
	// number, etc.
	// wx version: Since we cannot grey out individual listbox items on Linux (GTK) or the
	// MAC, we'll not put the embedded
	wxString wholeMkr;
	m_pListBoxSFMs->Clear(); // clears all items out of list

	int count;
	for (count = 0; count < (int)m_exportMarkerAndDescriptions.GetCount(); count++)
	{
		// since m_listBoxSFMs is sorted, the order that the sfms (lbStr)
		// appear in the list box to the user will not be the same order 
		// we process them here. Therefore, we'll keep track of our 
		// processing order in the SetItemData field. This number becomes
		// an index into the m_exportBareMarkers array, the 
		// m_exportMarkerAndDescriptions array, and the m_exportFilterFlags array, 
		// the latter keeps track of the current filtering state of the marker.
		// whm Note: compare with FilterPage's LoadListBoxFromArrays() which handles the
		// disabling and graying out of items in the list box
		lbStr = m_exportMarkerAndDescriptions[count];
		wxString wholeMkrSp = _T('\\') + m_exportBareMarkers.Item(count) + _T(' ');
		// wx version: do not include in the checklistbox any markers in the excludeWholeMkrs string
		if (excludedWholeMkrs.Find(wholeMkrSp) == -1)
		{
			if (m_pListBoxSFMs->FindString(lbStr) < 0)
			{
				// the list entry doesn't already exist so add it
				// add lbStr to the CCheckListBox, if it doesn't already exist, and set item data
				m_pListBoxSFMs->Append(lbStr);
				int newIndex = m_pListBoxSFMs->FindString(lbStr);
				wxASSERT(newIndex != -1); // LB_ERR
				if (newIndex != -1) // LB_ERR
				{
					m_pListBoxSFMs->Check(newIndex,FALSE); // export defaults to nothing filtered out
					// Can't use ItemData with wxCheckListBox as it is reserved for internal operations
				}
				nMarkerIndex++;
			}
		}
	}

	m_pListBoxSFMs->SetFocus();
	// Make sure that initially first item is selected in m_listBoxSFMs
	if (m_pListBoxSFMs->GetCount() > 0)
		m_pListBoxSFMs->SetSelection(0);
}

void CExportOptionsDlg::OnBnClickedButtonUndo(wxCommandEvent& WXUNUSED(event))
{
	// The Undo button resets the check boxes to the state they were in
	// when the Export Options Dialog was called.
	// Reset the export flags by copying again from the m_exportFilterFlagsBeforeEdit generated in 
	// ExportSaveAsDlg's GetMarkerInventoryFromCurrentDoc() routine. All flags start
	// as FALSE (i.e., don't filter out any, rather export all markers and associated text).
	//m_exportFilterFlags.Copy(m_exportFilterFlagsBeforeEdit);
	m_exportFilterFlags.Clear();
	int ct;
	for (ct = 0; ct < (int)m_exportFilterFlagsBeforeEdit.GetCount(); ct++)
	{
		m_exportFilterFlags.Add(m_exportFilterFlagsBeforeEdit.Item(ct));
	}

	// load the list box with sfms used in the current doc
	LoadActiveSFMListBox();

	m_pListBoxSFMs->SetFocus();
	if (m_pListBoxSFMs->GetCount() > 0)
		m_pListBoxSFMs->SetSelection(0);
	else
		return; // nothing else to do
	wxString selectedString = m_pListBoxSFMs->GetStringSelection();
	int newIndex = gpApp->FindArrayString(selectedString,&m_exportMarkerAndDescriptions); // get index into m_atSfmFlags
	bool bFilter;
	if (m_exportFilterFlags.Item(newIndex) == 0)
		bFilter = FALSE;
	else
		bFilter = TRUE;
	pButtonFilterOutFromExport->Enable(bFilter != TRUE); // same logic as filterPage buttons
	pButtonIncludeInExport->Enable(bFilter == TRUE); // same logic as filterPage buttons
	bFilterListChangesMade = FALSE;
	pButtonUndo->Enable(FALSE);
}


void CExportOptionsDlg::OnBnClickedCheckPlaceFreeTrans(wxCommandEvent& WXUNUSED(event))
{
	if (bPlaceFreeTransInRTFText)
	{
		bPlaceFreeTransInRTFText = FALSE;
		pCheckPlaceFreeTrans->SetValue(bPlaceFreeTransInRTFText);
	}
	else
	{
		bPlaceFreeTransInRTFText = TRUE;
		pCheckPlaceFreeTrans->SetValue(bPlaceFreeTransInRTFText);
	}
}

void CExportOptionsDlg::OnBnClickedCheckPlaceBackTrans(wxCommandEvent& WXUNUSED(event))
{
	if (bPlaceBackTransInRTFText)
	{
		bPlaceBackTransInRTFText = FALSE;
		pCheckPlaceBackTrans->SetValue(bPlaceBackTransInRTFText);
	}
	else
	{
		bPlaceBackTransInRTFText = TRUE;
		pCheckPlaceBackTrans->SetValue(bPlaceBackTransInRTFText);
	}
}

void CExportOptionsDlg::OnBnClickedCheckPlaceAiNotes(wxCommandEvent& WXUNUSED(event))
{
	if (bPlaceAINotesInRTFText)
	{
		bPlaceAINotesInRTFText = FALSE;
		pCheckPlaceAINotes->SetValue(bPlaceAINotesInRTFText);
	}
	else
	{
		bPlaceAINotesInRTFText = TRUE;
		pCheckPlaceAINotes->SetValue(bPlaceAINotesInRTFText);
	}
}

void CExportOptionsDlg::OnBnClickedCheckInterlinearPlaceFreeTrans(wxCommandEvent& WXUNUSED(event))
{
	if (bPlaceFreeTransInRTFText)
	{
		bPlaceFreeTransInRTFText = FALSE;
		pCheckPlaceFreeTransInterlinear->SetValue(bPlaceFreeTransInRTFText);
	}
	else
	{
		bPlaceFreeTransInRTFText = TRUE;
		pCheckPlaceFreeTransInterlinear->SetValue(bPlaceFreeTransInRTFText);
	}
}

void CExportOptionsDlg::OnBnClickedCheckInterlinearPlaceBackTrans(wxCommandEvent& WXUNUSED(event))
{
	if (bPlaceBackTransInRTFText)
	{
		bPlaceBackTransInRTFText = FALSE;
		pCheckPlaceBackTransInterlinear->SetValue(bPlaceBackTransInRTFText);
	}
	else
	{
		bPlaceBackTransInRTFText = TRUE;
		pCheckPlaceBackTransInterlinear->SetValue(bPlaceBackTransInRTFText);
	}
}

void CExportOptionsDlg::OnBnClickedCheckInterlinearPlaceAiNotes(wxCommandEvent& WXUNUSED(event))
{
	if (bPlaceAINotesInRTFText)
	{
		bPlaceAINotesInRTFText = FALSE;
		pCheckPlaceAINotesInterlinear->SetValue(bPlaceAINotesInRTFText);
	}
	else
	{
		bPlaceAINotesInRTFText = TRUE;
		pCheckPlaceAINotesInterlinear->SetValue(bPlaceAINotesInRTFText);
	}
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CExportOptionsDlg::OnOK(wxCommandEvent& event) 
{
	TransferFilterFlagValuesBetweenCheckListBoxAndArray(FromControls);
	
	event.Skip(); //EndModal(wxID_OK); //wxDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods
void CExportOptionsDlg::TransferFilterFlagValuesBetweenCheckListBoxAndArray(enum TransferDirection transferDirection)
{
	int nMarkerIndex;
	int nItem;
	if(m_pListBoxSFMs->GetCount() > 0)
	{
		for(nMarkerIndex = 0; nMarkerIndex < (int)m_exportMarkerAndDescriptions.GetCount(); nMarkerIndex++)
		{
			wxString wholeMkrSp = _T('\\') + m_exportBareMarkers.Item(nMarkerIndex) + _T(' ');
			// wx version: do not include in the checklistbox any markers in the excludeWholeMkrs string
			if (excludedWholeMkrs.Find(wholeMkrSp) == -1)
			{
				nItem = m_pListBoxSFMs->FindString(m_exportMarkerAndDescriptions[nMarkerIndex]);
				wxASSERT(nItem != -1); //LB_ERR
				if (transferDirection == FromControls)
				{
					int flag;
					if (m_pListBoxSFMs->IsChecked(nItem))
						flag = 1;
					else
						flag = 0;
					m_exportFilterFlags[nMarkerIndex] = flag;
				}
				else // when transferDirection == ToControls
				{
					bool check;
					if (m_exportFilterFlags[nMarkerIndex] == 0)
						check = FALSE;
					else
						check = TRUE;
					m_pListBoxSFMs->Check(nItem, check);
				}
			}
		}
	}
}
