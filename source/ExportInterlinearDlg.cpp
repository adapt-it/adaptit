/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportInterlinearDlg.cpp
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CExportInterlinearDlg class. 
/// The CExportInterlinearDlg class provides a dialog in which the user can specify
/// the inclusion or exclusion of various parts of the text or a span of references
/// for the interlinear RTF export. The dialog also has an "Export filter/options" 
/// button to access that sub-dialog.
/// \derivation		The CExportInterlinearDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

// BEW 10Apr10, no changes for support of doc version 5

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ExportInterlinearDlg.h"
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

#include "Adapt_It.h"
#include "ExportInterlinearDlg.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h" 
#include "ExportOptionsDlg.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

//extern wxChar gSFescapechar; // the escape char used for start of a standard format marker

// The following globals defined in ExportOptionsDlg.cpp for use here and in the 
// export routines in the View: DoExportTgt() and OnFileExportSource().
extern wxArrayString m_exportBareMarkers;
extern wxArrayString m_exportMarkerAndDescriptions;
extern wxArrayInt m_exportFilterFlags;
extern wxArrayInt m_exportFilterFlagsBeforeEdit;  // to detect any changes to list of markers for export

extern bool bExportAll;
extern bool bExportSelectedMarkersOnly;
extern bool bPlaceFreeTransInRTFText; 
extern bool bPlaceBackTransInRTFText;
extern bool bPlaceAINotesInRTFText;
extern bool bPlaceFreeTransCheckboxEnabled; // start with all checkboxes enabled (ExportOptionsDlg can change)
extern bool bPlaceBackTransCheckboxEnabled;
extern bool bPlaceAINotesCheckboxEnabled;

extern const wxChar* filterMkr;

bool bExportAsRTFInterlinear; // used as extern in ExportSaveAsDlg
extern bool bExportToRTF;

// event handler table
BEGIN_EVENT_TABLE(CExportInterlinearDlg, AIModalDialog)
	EVT_INIT_DIALOG(CExportInterlinearDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CExportInterlinearDlg::OnOK)
	EVT_RADIOBUTTON(IDC_RADIO_OUTPUT_ALL, CExportInterlinearDlg::OnRadioOutputAll)
	EVT_RADIOBUTTON(IDC_RADIO_OUTPUT_CHAPTER_VERSE_RANGE, CExportInterlinearDlg::OnRadioOutputChapterVerseRange)
	EVT_RADIOBUTTON(IDC_RADIO_OUTPUT_PRELIM, CExportInterlinearDlg::OnRadioOutputPrelim)
	EVT_RADIOBUTTON(IDC_RADIO_OUTPUT_FINAL, CExportInterlinearDlg::OnRadioOutputFinal)
	EVT_BUTTON(IDC_BUTTON_RTF_EXPORT_FILTER_OPTIONS, CExportInterlinearDlg::OnBnClickedButtonRtfExportFilterOptions)
	EVT_CHECKBOX(IDC_CHECK_INCLUDE_NAV_TEXT, CExportInterlinearDlg::OnBnClickedCheckIncludeNavText)
	EVT_CHECKBOX(ID_CHECK_NEW_TABLES_FOR_NEWLINE_MARKERS,CExportInterlinearDlg::OnBnClickedCheckNewTablesForNewlineMarkers)
END_EVENT_TABLE()


CExportInterlinearDlg::CExportInterlinearDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Export RTF Interlinear Document"),
				wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	ExportInterlinearDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );

    // whm 5Mar2019 Note: The ExportInterlinearDlgFunc() now uses wxStdDialogButtonSizer, so there
    // is no longer need to call the ReverseOkCancelButtonsForMac() function below.
	//bool bOK;
	//bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	//bOK = bOK; // avoid warning 
	m_bIncludeNavText = TRUE;
	m_bIncludeSourceText = TRUE;
	m_bIncludeTargetText = TRUE;
	m_bIncludeGlossText = FALSE;
	m_bPortraitOrientation = (gpApp->GetPageOrientation() == wxPORTRAIT);	// FALSE sets portrait radio button in MFC
	m_bDisableRangeButtons = FALSE;
	m_bNewTableForNewLineMarker = TRUE;
	m_bCenterTableForCenteredMarker = TRUE;
	m_nFromChapter = 0;
	m_nFromVerse = 1;
	m_nToChapter = 0;
	m_nToVerse = 1;

	pCheckIncludeNavText = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_NAV_TEXT);
	//pCheckIncludeNavText->SetValidator(wxGenericValidator(&m_bIncludeNavText));

	pCheckIncludeSrcText = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_SOURCE_TEXT);
	//pCheckIncludeSrcText->SetValidator(wxGenericValidator(&m_bIncludeSourceText));

	pCheckIncludeTgtText = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_TARGET_TEXT);
	//pCheckIncludeTgtText->SetValidator(wxGenericValidator(&m_bIncludeTargetText));

	pCheckIncludeGlsText = (wxCheckBox*)FindWindowById(IDC_CHECK_INCLUDE_GLS_TEXT);
	//pCheckIncludeGlsText->SetValidator(wxGenericValidator(&m_bIncludeGlossText));

	pTextCtrlAsStaticExpInterlinear = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_EXP_INTERLINEAR);
	wxASSERT(pTextCtrlAsStaticExpInterlinear != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticExpInterlinear->SetBackgroundColour(backgrndColor);

	pCheckNewTablesForNewLineMarkers = (wxCheckBox*)FindWindowById(ID_CHECK_NEW_TABLES_FOR_NEWLINE_MARKERS);
	//pCheckNewTablesForNewLineMarkers->SetValidator(wxGenericValidator(&m_bNewTableForNewLineMarker));
	
	pCheckCenterTablesForCenteredMarkers = (wxCheckBox*)FindWindowById(ID_CHECK_CENTER_TABLES_FOR_CENTERED_MARKERS);
	//pCheckCenterTablesForCenteredMarkers->SetValidator(wxGenericValidator(&m_bCenterTableForCenteredMarker));

	pRadioUsePortrait = (wxRadioButton*)FindWindowById(IDC_RADIO_PORTRAIT);
	//pRadioUsePortrait->SetValidator(wxGenericValidator(&m_bPortraitOrientation));

	pRadioUseLandscape = (wxRadioButton*)FindWindowById(IDC_RADIO_LANDSCAPE);

	pEditFromChapter = (wxTextCtrl*)FindWindowById(IDC_EDIT_FROM_CHAPTER);
	//pEditFromChapter->SetValidator(wxGenericValidator(&m_nFromChapter));

	pEditFromVerse = (wxTextCtrl*)FindWindowById(IDC_EDIT_FROM_VERSE);
	//pEditFromVerse->SetValidator(wxGenericValidator(&m_nFromVerse));

	pEditToChapter = (wxTextCtrl*)FindWindowById(IDC_EDIT_TO_CHAPTER);
	//pEditToChapter->SetValidator(wxGenericValidator(&m_nToChapter));

	pEditToVerse = (wxTextCtrl*)FindWindowById(IDC_EDIT_TO_VERSE);
	//pEditToVerse->SetValidator(wxGenericValidator(&m_nToVerse));
	
	// whm added 21Feb12
	pCheckUsePrefixExportProjNameOnFilename = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PREFIX_EXPORT_PROJ_NAME);
	wxASSERT(pCheckUsePrefixExportProjNameOnFilename != NULL);
	
	// whm added 9Dec11
	pCheckUsePrefixExportTypeOnFilename = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PREFIX_EXPORT_TYPE);
	wxASSERT(pCheckUsePrefixExportTypeOnFilename != NULL);
	
	pCheckUseSuffixExportDateTimeStamp = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SUFFIX_EXPORT_DATETIME_STAMP);
	wxASSERT(pCheckUseSuffixExportDateTimeStamp != NULL);
	// whm Note: The substitution of export type and state of the checkbox and enabling is done in
	// the DoExportInterlinearRTF() caller.
}

CExportInterlinearDlg::~CExportInterlinearDlg() // destructor
{
	
}

void CExportInterlinearDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	if (m_bIncludeGlossText)
	{
		pCheckIncludeGlsText->SetValue(TRUE);
		pCheckIncludeGlsText->Enable(TRUE);
	}
	else
	{
		pCheckIncludeGlsText->SetValue(FALSE);
		pCheckIncludeGlsText->Enable(FALSE);
	}

	if (m_bPortraitOrientation)
	{
		pRadioUsePortrait->SetValue(TRUE);
		pRadioUseLandscape->SetValue(FALSE);
	}
	else
	{
		pRadioUsePortrait->SetValue(FALSE);
		pRadioUseLandscape->SetValue(TRUE);
	}

	pCheckNewTablesForNewLineMarkers->SetValue(TRUE);
	pCheckCenterTablesForCenteredMarkers->SetValue(TRUE);
	pCheckCenterTablesForCenteredMarkers->Enable(TRUE);

	// Initialize Output Range radio buttons - set check on "All" others unchecked
	m_bOutputAll = TRUE;
	wxRadioButton* pButtonOutputAll = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_ALL);
	pButtonOutputAll->SetValue(TRUE);

	m_bOutputPrelim = FALSE;
	wxRadioButton* pButtonOutputPrelim = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_PRELIM);
	pButtonOutputPrelim->SetValue(FALSE);

	m_bOutputFinal = FALSE;
	wxRadioButton* pButtonOutputFinal = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_FINAL);
	pButtonOutputFinal->SetValue(FALSE);

	m_bOutputCVRange = FALSE;
	wxRadioButton* pButtonOutputCVRange = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_CHAPTER_VERSE_RANGE);
	pButtonOutputCVRange->SetValue(FALSE);

	if (m_bDisableRangeButtons)
	{
		wxRadioButton* pButtonOutputAll = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_ALL);
		pButtonOutputAll->Enable(TRUE);

		wxRadioButton* pButtonOutputPrelim = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_PRELIM);
		pButtonOutputPrelim->Enable(FALSE);

		wxRadioButton* pButtonOutputFinal = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_FINAL);
		pButtonOutputFinal->Enable(FALSE);

		wxRadioButton* pButtonOutputCVRange = (wxRadioButton*)FindWindowById(IDC_RADIO_OUTPUT_CHAPTER_VERSE_RANGE);
		pButtonOutputCVRange->Enable(FALSE);
	}

	// Ensure that the arrays used as extern variables are empty before export
	// operations.
	m_exportBareMarkers.Clear();
	m_exportMarkerAndDescriptions.Clear();
	m_exportFilterFlags.Clear();
	m_exportFilterFlagsBeforeEdit.Clear();

	bExportAsRTFInterlinear = TRUE; // set the global
	bExportToRTF = TRUE; // set the extern global
	bExportAll = TRUE; // start with export of all markers radio button selected in Export Options
	bExportSelectedMarkersOnly = FALSE;
	bPlaceFreeTransInRTFText = TRUE; // changed to TRUE; in v 3.0.1 after table row code added
	bPlaceBackTransInRTFText = FALSE;
	bPlaceAINotesInRTFText = FALSE;
	bPlaceFreeTransCheckboxEnabled = TRUE; // changed to TRUE; in v 3.0.1 after table row code added
	bPlaceBackTransCheckboxEnabled = TRUE; // changed to TRUE; in v 3.0.1 after table row code added
	bPlaceAINotesCheckboxEnabled = TRUE;

	gpApp->GetDocument()->GetMarkerInventoryFromCurrentDoc();

	//TransferDataToWindow(); // whm removed 21Nov11
	// whm added below 21Nov11 in place of TransferDataToWindow() above:
	pCheckIncludeNavText->SetValue(m_bIncludeNavText);
	pCheckIncludeSrcText->SetValue(m_bIncludeSourceText);
	pCheckIncludeTgtText->SetValue(m_bIncludeTargetText);
	pCheckIncludeGlsText->SetValue(m_bIncludeGlossText);
	pCheckNewTablesForNewLineMarkers->SetValue(m_bNewTableForNewLineMarker);
	pCheckCenterTablesForCenteredMarkers->SetValue(m_bCenterTableForCenteredMarker);
	pRadioUsePortrait->SetValue(m_bPortraitOrientation);
	wxString tempStr;
	tempStr.Empty();
	tempStr << m_nFromChapter;
	pEditFromChapter->ChangeValue(tempStr);
	tempStr.Empty();
	tempStr << m_nFromVerse;
	pEditFromVerse->ChangeValue(tempStr);
	tempStr.Empty();
	tempStr << m_nToChapter;
	pEditToChapter->ChangeValue(tempStr);
	tempStr.Empty();
	tempStr << m_nToVerse;
	pEditToVerse->ChangeValue(tempStr);
	
	// initialize the values of the checkboxes from the App's values
	pCheckUsePrefixExportProjNameOnFilename->SetValue(gpApp->m_bUsePrefixExportProjectNameOnFilename);
	pCheckUsePrefixExportTypeOnFilename->SetValue(gpApp->m_bUsePrefixExportTypeOnFilename);
	pCheckUseSuffixExportDateTimeStamp->SetValue(gpApp->m_bUseSuffixExportDateTimeOnFilename);
	// Note: the caller DoExportInterlinearRTF() accesses the above two checkbox values and may
	// enable or disable the checkboxes as appropriate to the exporting context there.

}

// event handling functions

void CExportInterlinearDlg::OnRadioOutputAll(wxCommandEvent& WXUNUSED(event))
{
	m_bOutputAll = TRUE;
	m_bOutputCVRange = FALSE;
	m_bOutputPrelim = FALSE;
	m_bOutputFinal = FALSE;
	// make sure the Ch:Vs edit boxes are disabled
	pEditFromChapter->Enable(FALSE);
	pEditFromVerse->Enable(FALSE);
	pEditToChapter->Enable(FALSE);
	pEditToVerse->Enable(FALSE);
}

void CExportInterlinearDlg::OnRadioOutputChapterVerseRange(wxCommandEvent& WXUNUSED(event))
{
	m_bOutputAll = FALSE;
	m_bOutputCVRange = TRUE;
	m_bOutputPrelim = FALSE;
	m_bOutputFinal = FALSE;
	// enable the Ch:Vs edit boxes
	pEditFromChapter->Enable(TRUE);
	pEditFromVerse->Enable(TRUE);
	pEditToChapter->Enable(TRUE);
	pEditToVerse->Enable(TRUE);
}


void CExportInterlinearDlg::OnRadioOutputPrelim(wxCommandEvent& WXUNUSED(event))
{
	m_bOutputAll = FALSE;
	m_bOutputCVRange = FALSE;
	m_bOutputPrelim = TRUE;
	m_bOutputFinal = FALSE;
	// make sure the Ch:Vs edit boxes are disabled
	pEditFromChapter->Enable(FALSE);
	pEditFromVerse->Enable(FALSE);
	pEditToChapter->Enable(FALSE);
	pEditToVerse->Enable(FALSE);
}

void CExportInterlinearDlg::OnRadioOutputFinal(wxCommandEvent& WXUNUSED(event))
{
	m_bOutputAll = FALSE;
	m_bOutputCVRange = FALSE;
	m_bOutputPrelim = FALSE;
	m_bOutputFinal = TRUE;
	// make sure the Ch:Vs edit boxes are disabled
	pEditFromChapter->Enable(FALSE);
	pEditFromVerse->Enable(FALSE);
	pEditToChapter->Enable(FALSE);
	pEditToVerse->Enable(FALSE);
}

void CExportInterlinearDlg::OnBnClickedButtonRtfExportFilterOptions(wxCommandEvent& WXUNUSED(event))
{
	// user clicked on Export Filter/Options button
	CExportOptionsDlg dlg(this);
	dlg.Centre();
	if (dlg.ShowModal() != wxID_OK)
	{
		// Note: 16Nov05 We don't want to reset the filter flags upon a cancel
		// from the Export Options dialog, but should be done rather from the
		// ExportSaveAsDlg, that way a user can cancel the secondary dialog
		// and go back to check his settings afterwards.
		// User cancelled, and so no export options were changed.
		// So, ensure that all the flags in m_exportFilterFlags are set 
		// back to their default values in m_exportFilterFlagsBeforeEdit,
		// which will signal to the caller that no changes were made.
		//m_exportFilterFlags.Copy(m_exportFilterFlagsBeforeEdit);
	}
}

void CExportInterlinearDlg::OnBnClickedCheckIncludeNavText(wxCommandEvent& WXUNUSED(event))
{
	if (pCheckIncludeNavText->GetValue() == FALSE)
	{
		// "Warning: The Navigation text row is necessary to display Adapt It Notes and
		// any free translation or back translation when formatted as footnotes. If you omit
		// the Navigation text row, these note items will not be included in the RTF output."
		//IDS_WARN_IF_NO_NAV_TEXT
        // whm 15May2020 added below to supress phrasebox run-on due to handling of ENTER in CPhraseBox::OnKeyUp()
        gpApp->m_bUserDlgOrMessageRequested = TRUE;
        wxMessageBox(_("Warning: The Navigation text row is necessary to display Adapt It Notes and any Free translation or Back translation when they are formatted as footnotes. If you omit the Navigation text row, these note items will not be included in the RTF output."),_T(""), wxICON_EXCLAMATION | wxOK);
	}
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CExportInterlinearDlg::OnOK(wxCommandEvent& event) 
{
	//TransferDataFromWindow(); // whm removed 21Nov11
	// whm added below 21Nov11 in place of TransferDataFromWindow() above:
	m_bIncludeNavText = pCheckIncludeNavText->GetValue();
	m_bIncludeSourceText = pCheckIncludeSrcText->GetValue();
	m_bIncludeTargetText = pCheckIncludeTgtText->GetValue();
	m_bIncludeGlossText = pCheckIncludeGlsText->GetValue();
	m_bNewTableForNewLineMarker = pCheckNewTablesForNewLineMarkers->GetValue();
	m_bCenterTableForCenteredMarker = pCheckCenterTablesForCenteredMarkers->GetValue();
	m_bPortraitOrientation = pRadioUsePortrait->GetValue();
	wxString tempStr;
	tempStr = pEditFromChapter->GetValue();
	m_nFromChapter = wxAtoi(tempStr);
	tempStr = pEditFromVerse->GetValue();
	m_nFromVerse = wxAtoi(tempStr);
	tempStr = pEditToChapter->GetValue();
	m_nToChapter = wxAtoi(tempStr);
	tempStr = pEditToVerse->GetValue();
	m_nToVerse = wxAtoi(tempStr);
	
	// save the values to the flags on the App for saving in the basic config file
	gpApp->m_bUsePrefixExportProjectNameOnFilename = pCheckUsePrefixExportProjNameOnFilename->GetValue();
	gpApp->m_bUsePrefixExportTypeOnFilename = pCheckUsePrefixExportTypeOnFilename->GetValue();
	gpApp->m_bUseSuffixExportDateTimeOnFilename = pCheckUseSuffixExportDateTimeStamp->GetValue();
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


void CExportInterlinearDlg::OnBnClickedCheckNewTablesForNewlineMarkers(wxCommandEvent& WXUNUSED(event))
{
	bool bVal = pCheckNewTablesForNewLineMarkers->GetValue();
	// enable or disable the second checkbox according to the checked state of the first box
	if (bVal == FALSE)
		pCheckCenterTablesForCenteredMarkers->SetValue(FALSE);
	pCheckCenterTablesForCenteredMarkers->Enable(bVal);
}

