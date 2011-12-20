/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportSaveAsDlg.cpp
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CExportSaveAsDlg class. 
/// The CExportSaveAsDlg class provides a dialog in which the user can indicate wither
/// the export of the source text (or target text) is to be in sfm format or RTF format.
/// The dialog also has an "Export/Filter Options" buttons to access that sub-dialog.
/// \derivation		The CExportSaveAsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in ExportSaveAsDlg.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "ExportSaveAsDlg.h"
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
#include "Adapt_ItDoc.h"
#include "ExportSaveAsDlg.h"
#include "ExportOptionsDlg.h"
#include "Adapt_ItView.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// The following globals defined in ExportOptionsDlg.cpp for use here and in the 
// export routines in the View: DoExportTgt() and OnFileExportSource().
extern wxArrayString m_exportBareMarkers;
extern wxArrayString m_exportMarkerAndDescriptions;
extern wxArrayInt m_exportFilterFlags;
extern wxArrayInt m_exportFilterFlagsBeforeEdit;  // to detect any changes to list of markers for export

// Variables below used as extern in ExportOptionsDlg
bool bExportAll;
bool bExportSelectedMarkersOnly;
bool bPlaceFreeTransInRTFText; 
bool bPlaceBackTransInRTFText;
bool bPlaceAINotesInRTFText;
bool bPlaceFreeTransCheckboxEnabled;
bool bPlaceBackTransCheckboxEnabled;
bool bPlaceAINotesCheckboxEnabled;

extern const wxChar* filterMkr;

extern bool bExportAsRTFInterlinear; // global used as extern in ExportOptionsDlg.cpp
bool bExportToRTF;

// event handler table
BEGIN_EVENT_TABLE(CExportSaveAsDlg, AIModalDialog)
	EVT_INIT_DIALOG(CExportSaveAsDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CExportSaveAsDlg::OnOK)
	EVT_RADIOBUTTON(IDC_RADIO_EXPORT_AS_SFM, CExportSaveAsDlg::OnBnClickedRadioExportAsSfm)
	EVT_RADIOBUTTON(IDC_RADIO_EXPORT_AS_RTF, CExportSaveAsDlg::OnBnClickedRadioExportAsRtf)
	EVT_BUTTON(IDC_BUTTON_EXPORT_FILTER_OPTIONS, CExportSaveAsDlg::OnBnClickedButtonExportFilterOptions)
END_EVENT_TABLE()


CExportSaveAsDlg::CExportSaveAsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Export Document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pExportSaveAsSizer = ExportSaveAsDlgFunc(this, TRUE, TRUE);
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	bool bOK;
	bOK = gpApp->ReverseOkCancelButtonsForMac(this);
	
	pExportAsSfm = (wxRadioButton*)FindWindowById(IDC_RADIO_EXPORT_AS_SFM);
	wxASSERT(pExportAsSfm != NULL);
	pExportAsSfm->SetValue(TRUE);
	
	pExportAsRTF = (wxRadioButton*)FindWindowById(IDC_RADIO_EXPORT_AS_RTF);
	wxASSERT(pExportAsRTF != NULL);
	pExportAsRTF->SetValue(FALSE);

	pTextCtrlAsStaticExpSaveAs1 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_EXPORT_SAVE_AS_1);
	wxASSERT(pTextCtrlAsStaticExpSaveAs1 != NULL);
	wxColor backgrndColor = this->GetBackgroundColour();
	pTextCtrlAsStaticExpSaveAs1->SetBackgroundColour(backgrndColor);

	pTextCtrlAsStaticExpSaveAs2 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_EXPORT_SAVE_AS_2);
	wxASSERT(pTextCtrlAsStaticExpSaveAs2 != NULL);
	pTextCtrlAsStaticExpSaveAs2->SetBackgroundColour(backgrndColor);

	pTextCtrlAsStaticExpSaveAs3 = (wxTextCtrl*)FindWindowById(ID_TEXTCTRL_AS_STATIC_EXPORT_SAVE_AS_3);
	wxASSERT(pTextCtrlAsStaticExpSaveAs3 != NULL);
	pTextCtrlAsStaticExpSaveAs3->SetBackgroundColour(backgrndColor);

	pStaticTitle = (wxStaticText*)FindWindowById(IDC_STATIC_TITLE);
	wxASSERT(pStaticTitle != NULL);

	// whm added 9Dec11
	pCheckUsePrefixExportTypeOnFilename = (wxCheckBox*)FindWindowById(ID_CHECKBOX_PREFIX_EXPORT_TYPE);
	wxASSERT(pCheckUsePrefixExportTypeOnFilename != NULL);
	
	pCheckUseSuffixExportDateTimeStamp = (wxCheckBox*)FindWindowById(ID_CHECKBOX_SUFFIX_EXPORT_DATETIME_STAMP);
	wxASSERT(pCheckUseSuffixExportDateTimeStamp != NULL);
	// whm Note: The substitution of export type and state of the checkbox and enabling is done in
	// the DoExportSfmText() caller.

	m_ExportToRTF = FALSE;
}

CExportSaveAsDlg::~CExportSaveAsDlg() // destructor
{
	
}

void CExportSaveAsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	//InitDialog() is not virtual, no call needed to a base class
	
	// whm 9Dec11 moved the code for initializing the pointers to controls 
	// to the constructor because the caller needs to access one or more of
	// them, after instantiating the CExportSaveAsDlg - and simply instantiating
	// the dialog does not call this InitDialog().
	
	// Ensure that the arrays used as extern variables are empty before export
	// operations.
	m_exportBareMarkers.Clear();
	m_exportMarkerAndDescriptions.Clear();
	m_exportFilterFlags.Clear();
	m_exportFilterFlagsBeforeEdit.Clear();

	m_ExportToRTF = FALSE;
	bExportToRTF = FALSE;
	bExportAsRTFInterlinear = FALSE;
	
	// Note: If the initial state of any of the following bools is changed, be sure to also change
	// their re-initializations in ExportOptionsDlg's OnCancel() method.
	bExportAll = TRUE; // start with export of all markers radio button selected in Export Options
	bExportSelectedMarkersOnly = FALSE;
	bPlaceFreeTransInRTFText = TRUE;
	bPlaceBackTransInRTFText = FALSE;
	bPlaceAINotesInRTFText = TRUE; // start with AI Notes in balloon text modified 17Jan06
	bPlaceFreeTransCheckboxEnabled = FALSE; // start with all checkboxes disabled (ExportOptionsDlg can change)
	bPlaceBackTransCheckboxEnabled = FALSE;
	bPlaceAINotesCheckboxEnabled = FALSE;

	gpApp->GetDocument()->GetMarkerInventoryFromCurrentDoc(); // populates export arrays with current doc's markers

	// BEW added 25Nov05, to ensure m_exportFilterFlags gets set to a same sized array as
	// m_exportFilterFlagsBeforeEdit, and with all stored values FALSE (ie. don't filter)
	// so that if the user elects not to invoke the filter options dialog via the button for
	// that purpose, we won't have m_exportFilterFlags left with zero size, leading to a crash
	//m_exportFilterFlags.Copy(m_exportFilterFlagsBeforeEdit);
	// whm Note: No, GetMarkerInventoryFromCurrentDoc() above ensures that m_exportFilterFlags
	// and m_exportFilterFlagsBeforeEdit are the same size and all values for both arrays are
	// set to FALSE. Therefore, the code added below on 25Nov05 is not needed
	//int ct;
	//m_exportFilterFlags.Clear();
	//for (ct = 0; ct < (int)m_exportFilterFlagsBeforeEdit.GetCount(); ct++)
	//{
	//	m_exportFilterFlags.Add(m_exportFilterFlagsBeforeEdit.Item(ct));
	//}
	
	// initialize the values of the checkboxes from the App's values
	pCheckUsePrefixExportTypeOnFilename->SetValue(gpApp->m_bUsePrefixExportTypeOnFilename);
	pCheckUseSuffixExportDateTimeStamp->SetValue(gpApp->m_bUseSuffixExportDateTimeOnFilename);
	// Note: the caller DoExportSfmText() accesses the above two checkbox values and may
	// enable or disable the checkboxes as appropriate to the exporting context there.

	pExportSaveAsSizer->Layout();
}

// event handling functions

void CExportSaveAsDlg::OnBnClickedRadioExportAsSfm(wxCommandEvent& WXUNUSED(event))
{
	// user clicked Export as Sfm radio button
	wxRadioButton* pExportAsRTF = (wxRadioButton*)FindWindowById(IDC_RADIO_EXPORT_AS_RTF);
	pExportAsRTF->SetValue(FALSE);
	m_ExportToRTF = FALSE;
	bExportToRTF = FALSE;
	bExportAsRTFInterlinear = FALSE; // global used as extern in ExportOptionsDlg.cpp
	bPlaceFreeTransCheckboxEnabled = FALSE; // start with all checkboxes enabled (ExportOptionsDlg can change)
	bPlaceBackTransCheckboxEnabled = FALSE;
	bPlaceAINotesCheckboxEnabled = FALSE;
}

void CExportSaveAsDlg::OnBnClickedRadioExportAsRtf(wxCommandEvent& WXUNUSED(event))
{
	// user clicked Export as RTF radio button
	wxRadioButton* pExportAsSfm = (wxRadioButton*)FindWindowById(IDC_RADIO_EXPORT_AS_SFM);
	pExportAsSfm->SetValue(FALSE);
	m_ExportToRTF = TRUE;
	bExportToRTF = TRUE;
	bExportAsRTFInterlinear = FALSE; // global used as extern in ExportOptionsDlg.cpp
	bPlaceFreeTransCheckboxEnabled = TRUE; 
	bPlaceBackTransCheckboxEnabled = TRUE;
	bPlaceAINotesCheckboxEnabled = TRUE;
}

void CExportSaveAsDlg::OnBnClickedButtonExportFilterOptions(wxCommandEvent& WXUNUSED(event))
{


	// user clicked on Export Filter/Options button
	CExportOptionsDlg dlg(this);
	dlg.Centre();
	if (dlg.ShowModal() != wxID_OK)
	{
		// Note: 16Nov05 We don't want to reset the filter flags upon a cancel
		// from the Export Options dialog, but should be done rather from the
		// ExportInterlinearDlg, that way a user can cancel the secondary dialog
		// and go back to check his settings afterwards.
		// User cancelled, and so no export options were changed.
		// So, ensure that all the flags in m_exportFilterFlags are set 
		// back to their default values in m_exportFilterFlagsBeforeEdit,
		// which will signal to the caller that no changes were made.
		//m_exportFilterFlags.Copy(m_exportFilterFlagsBeforeEdit);
	}
}

// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CExportSaveAsDlg::OnOK(wxCommandEvent& event) 
{
	// save the values to the flags on the App for saving in the basic config file
	gpApp->m_bUsePrefixExportTypeOnFilename = pCheckUsePrefixExportTypeOnFilename->GetValue();
	gpApp->m_bUseSuffixExportDateTimeOnFilename = pCheckUseSuffixExportDateTimeStamp->GetValue();
	
	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}


// other class methods

