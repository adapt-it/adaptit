/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportSaveAsDlg.cpp
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 June 2012
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General
///			Public License (see license directory)
/// \description	This is the implementation file for the
///			CExportSaveAsDlg class.
/// 			The CExportSaveAsDlg class provides a dialog in which
///			the user can indicate the format of the exported
///			source text (or target text). The dialog also has an
///			"Export/Filter Options" buttons to access that
///			sub-dialog.
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

//(*InternalHeaders(CExportSaveAsDlg)
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
//*)

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/valgen.h> // for wxGenericValidator

#include "Adapt_It.h"
#include "Adapt_ItDoc.h"
#include "ExportSaveAsDlg.h"
#include "ExportOptionsDlg.h"
#include "Adapt_ItView.h"
#include "BookNameDlg.h"

#include "../res/vectorized/pw_48.cpp"
#include "../res/vectorized/text-txt_48.cpp"
#include "../res/vectorized/text-rtf_48.cpp"
#include "../res/vectorized/text-html_48.cpp"

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

//(*IdInit(CExportSaveAsDlg)
const long CExportSaveAsDlg::ID_LBLEXPORTTO = wxNewId();
const long CExportSaveAsDlg::ID_BTNEXPORTTOTXT = wxNewId();
const long CExportSaveAsDlg::ID_BTNEXPORTTORTF = wxNewId();
const long CExportSaveAsDlg::ID_BTNEXPORTTOXHTML = wxNewId();
const long CExportSaveAsDlg::ID_BTNEXPORTTOPATHWAY = wxNewId();
const long CExportSaveAsDlg::ID_PANEL1 = wxNewId();
//*)

BEGIN_EVENT_TABLE(CExportSaveAsDlg,AIModalDialog)
	//(*EventTable(CExportSaveAsDlg)
	EVT_INIT_DIALOG(CExportSaveAsDlg::InitDialog)
	EVT_BUTTON(ID_BTNEXPORTTOTXT, CExportSaveAsDlg::OnbtnExportToTxtClick)
	EVT_BUTTON(ID_BTNEXPORTTORTF, CExportSaveAsDlg::OnbtnExportToRtfClick)
	EVT_BUTTON(ID_BTNEXPORTTOXHTML, CExportSaveAsDlg::OnbtnExportToXhtmlClick)
	EVT_BUTTON(ID_BTNEXPORTTOPATHWAY, CExportSaveAsDlg::OnbtnExportToPathwayClick)
	EVT_BUTTON(wxID_OK, CExportSaveAsDlg::OnOK)
	EVT_BUTTON(ID_BTNFILTEROPTIONS, CExportSaveAsDlg::OnbtnFilterOptionsClick)
	EVT_BUTTON(ID_BUTTON_CHANGE_BOOK_NAME, CExportSaveAsDlg::OnBtnChangeBookName)
	EVT_RADIOBUTTON(ID_RDOFILTEROFF,CExportSaveAsDlg::OnrdoFilterOffSelect)
	EVT_RADIOBUTTON(ID_RDOFILTERON,CExportSaveAsDlg::OnrdoFilterOnSelect)
	//*)
END_EVENT_TABLE()

CExportSaveAsDlg::CExportSaveAsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Export Document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	// This dialog function below is generated in wxDesigner, and defines the controls and sizers
	// for the dialog. The first parameter is the parent which should normally be "this".
	// The second and third parameters should both be TRUE to utilize the sizers and create the right
	// size dialog.
	pExportSaveAsSizer = ExportSaveAsDlgFunc(this, FALSE, TRUE); // second param FALSE enables resize
	// The declaration is: NameFromwxDesignerDlgFunc( wxWindow *parent, bool call_fit, bool set_sizer );
	
	// edb 15June2012: This is a workaround for a difference between wxSmith and wxDesigner 2.20a.
	// wxDesigner doesn't appear to let you add controls to a wxPanel (you can do this in wxSmith);
	// we've created a panel in wxDesigner that we'll manually add the export format buttons to here.
	wxBoxSizer* BoxSizer1;
	wxPanel* Panel1;
	Panel1 = (wxPanel*)FindWindowById(ID_PNLEXPORT);
	Panel1->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	Panel1->SetWindowStyle(wxEXPAND);
	BoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
	lblExportTo = new wxStaticText(Panel1, ID_LBLEXPORTTO, _("Export to:"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_LBLEXPORTTO"));
	BoxSizer1->Add(lblExportTo, 1, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 10);
	btnExportToTxt = new wxBitmapButton(Panel1, ID_BTNEXPORTTOTXT, gpApp->wxGetBitmapFromMemory(text_txt_png), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BTNEXPORTTOTXT"));
	BoxSizer1->Add(btnExportToTxt, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	btnExportToRtf = new wxBitmapButton(Panel1, ID_BTNEXPORTTORTF, gpApp->wxGetBitmapFromMemory(text_rtf_png), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BTNEXPORTTORTF"));
	BoxSizer1->Add(btnExportToRtf, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	btnExportToXhtml = new wxBitmapButton(Panel1, ID_BTNEXPORTTOXHTML, gpApp->wxGetBitmapFromMemory(Text_html_png), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BTNEXPORTTOXHTML"));
	BoxSizer1->Add(btnExportToXhtml, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	btnExportToPathway = new wxBitmapButton(Panel1, ID_BTNEXPORTTOPATHWAY, gpApp->wxGetBitmapFromMemory(pw48_png), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator, _T("ID_BTNEXPORTTOPATHWAY"));
	BoxSizer1->Add(btnExportToPathway, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer1->Add(-1,-1,1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	Panel1->SetSizer(BoxSizer1);
	BoxSizer1->Fit(Panel1);
	BoxSizer1->SetSizeHints(Panel1);
	// end wxDesigner workaround
	
	// initialize UI control pointers
	lblExportTypeDescription = (wxStaticText*)FindWindowById(ID_LBLEXPORTTYPEDESCRIPTION);
	wxASSERT(lblExportTypeDescription != NULL);
	pCheckUsePrefixExportProjNameOnFilename = (wxCheckBox*)FindWindowById(ID_CHKPROJECTNAMEPREFIX);
	wxASSERT(pCheckUsePrefixExportProjNameOnFilename != NULL);
	pCheckUsePrefixExportTypeOnFilename = (wxCheckBox*)FindWindowById(ID_CHKTARGETTEXTPREFIX);
	wxASSERT(pCheckUsePrefixExportTypeOnFilename != NULL);
	pCheckUseSuffixExportDateTimeStamp = (wxCheckBox*)FindWindowById(ID_CHKDATETIMESUFFIX);
	wxASSERT(pCheckUseSuffixExportDateTimeStamp != NULL);
	rdoFilterOff = (wxRadioButton *)FindWindowById(ID_RDOFILTEROFF);
	wxASSERT(rdoFilterOff != NULL);
	rdoFilterOn = (wxRadioButton *)FindWindowById(ID_RDOFILTERON);
	wxASSERT(rdoFilterOn != NULL);
	btnFilterOptions = (wxButton *)FindWindowById(ID_BTNFILTEROPTIONS);
	wxASSERT(btnFilterOptions != NULL);
	pBtnChangeBookName = (wxButton*)FindWindowById(ID_BUTTON_CHANGE_BOOK_NAME);
	wxASSERT(pBtnChangeBookName != NULL);
}

CExportSaveAsDlg::~CExportSaveAsDlg(void)
{
	//(*Destroy(CExportSaveAsDlg)
	//*)
}


void CExportSaveAsDlg::InitDialog(wxInitDialogEvent& WXUNUSED(event)) // InitDialog is method of wxWindow
{
	// Ensure that the arrays used as extern variables are empty before export
	// operations.
	m_exportBareMarkers.Clear();
	m_exportMarkerAndDescriptions.Clear();
	m_exportFilterFlags.Clear();
	m_exportFilterFlagsBeforeEdit.Clear();

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

	// Pathway check
	// (moved to Validate() for Windows / Linux) 
#ifdef __WXMAC__
	// no Pathway on OSX - at least not yet
	btnExportToPathway->Disable();
#endif

    // export format description
    m_enumSaveAsType = ExportSaveAsTXT;
    SetExportTypeDescription(m_enumSaveAsType);

    // filter radio buttons and filter options button
    rdoFilterOff->SetValue(bExportAll);
    rdoFilterOn->SetValue(!bExportAll);
    btnFilterOptions->Enable(!bExportAll); // if there is no filter, disable the filter options button

	// Hide the "Change Book Name..." button by if there is no value in the
	// App's m_CollabBookSelected member. If the m_CollabBookSelected has a value
	// the button will be shown so the user can optionally change the name of the
	// book.
	// whm Note: the App's m_CollabBookSelected member itself should not be changed in
	// the Xhtml/Pathway export code. A different variable should be used if necessary
	// to store the change
	if (gpApp->m_CollabBookSelected.IsEmpty())
	{
		pBtnChangeBookName->Hide();
	}

	btnExportToTxt->SetBackgroundColour(wxColour(189,255,189)); // light pastel green
	btnExportToRtf->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToXhtml->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToPathway->SetBackgroundColour(wxColour(255,255,255)); // white

	// export file naming rules checkboxes
	pCheckUsePrefixExportProjNameOnFilename->SetValue(gpApp->m_bUsePrefixExportProjectNameOnFilename);
	pCheckUsePrefixExportTypeOnFilename->SetValue(gpApp->m_bUsePrefixExportTypeOnFilename);
	pCheckUseSuffixExportDateTimeStamp->SetValue(gpApp->m_bUseSuffixExportDateTimeOnFilename);
	// Note: the caller DoExportSfmText() accesses the above two checkbox values and may
	// enable or disable the checkboxes as appropriate to the exporting context there.
	
	pExportSaveAsSizer->Layout();
	// The bottom of the dialog is likely going to be truncated unless we resize the
	// dialog to fit it. Note: The constructor's call of ExportSaveAsDlgFunc(this, FALSE, TRUE)
	// has its second parameter as FALSE to allow this resize here in InitDialog().
	wxSize dlgSize;
	dlgSize = pExportSaveAsSizer->ComputeFittingWindowSize(this);
	this->SetSize(dlgSize);
	this->CenterOnParent();
}

// Update the lblExportTypeDescription text based on the specified export type
void CExportSaveAsDlg::SetExportTypeDescription(ExportSaveAsType newType)
{
    wxString newString;
    switch (newType)
    {
        case ExportSaveAsRTF:
            newString = _("Export a Rich Text Format file that can be loaded into a Word processor like MS Word. Adapt It will format the file using the Word Scripture Template, so that you can print it in a publishable format.");
            break;
        case ExportSaveAsXHTML:
            newString = _T("Export an XHTML file that can be viewed using a current web browser.");
            break;
        case ExportSaveAsPathway:
            newString = _T("Export through SIL Pathway to produce a file in other formats, such as cell phones or e-book readers.");
            // TODO: check if pathway is installed; if not, notify the user.
            break;
        case ExportSaveAsTXT:
        default:
            newString = _("Export a Standard Format Text file (containing backslash markers for formatting) that can be used as a source text for another Adapt It project, or can be edited in a text editor, or loaded into the Paratext program.");
            break;
    }
    lblExportTypeDescription->SetLabel(newString);
    lblExportTypeDescription->Wrap(425); // wrap if needed
}


/////////////////////////////////////////////////////////////////////////////
// Event Handlers
/////////////////////////////////////////////////////////////////////////////
void CExportSaveAsDlg::OnrdoFilterOnSelect(wxCommandEvent& WXUNUSED(event))
{
    bExportAll = FALSE;
    bExportSelectedMarkersOnly = TRUE;
    btnFilterOptions->Enable();
}

void CExportSaveAsDlg::OnrdoFilterOffSelect(wxCommandEvent& WXUNUSED(event))
{
    bExportAll = TRUE;
    bExportSelectedMarkersOnly = FALSE;
    btnFilterOptions->Disable();
}

void CExportSaveAsDlg::OnbtnFilterOptionsClick(wxCommandEvent& WXUNUSED(event))
{
	// user clicked on Export Filter/Options button
	CExportOptionsDlg dlg(this);
	dlg.Centre();
	if (dlg.ShowModal() == wxID_OK)
	{
		// TODO: If the user has set a filter, change the radio button
		rdoFilterOff->SetValue(bExportAll);
		rdoFilterOn->SetValue(!bExportAll);
		btnFilterOptions->Enable(!bExportAll); // if there is no filter, disable the filter options button
	}
}

// whm 10Aug12 added handler for Change Book Name... button
void CExportSaveAsDlg::OnBtnChangeBookName(wxCommandEvent& WXUNUSED(event))
{
	// user clicked on the Change Book Name... button
	// whm Note: the App's m_CollabBookSelected member itself should not be changed in
	// the Xhtml/Pathway export code. A different variable should be used if necessary
	// to store the change
	// BEW reply to Note: the value is stored in the app's wxString member m_bookName_Current
	gpApp->GetDocument()->DoBookName();
}

bool CExportSaveAsDlg::Validate()
{
	if ((gpApp->PathwayIsInstalled() == false) && (m_enumSaveAsType == ExportSaveAsPathway))
	{
		// Pathway isn't installed, but the user chose it. Disable Pathway, set the current
		// output type to xhtml and tell the user they need to install Pathway.
		wxString aMsg = _T("In order to use the Pathway export option, Pathway must installed on this computer./nPlease download and install Pathway from http://pathway.sil.org/download/latest-sprint-downloads/ and retry this operation.");
		wxMessageBox(aMsg,_T("Pathway Not Installed"),wxICON_HAND | wxOK);
		btnExportToPathway->Disable();
		wxCommandEvent evt;
		OnbtnExportToXhtmlClick(evt);
		return false;
	}

	// If we got here, there should be no problems closing the export dialog and continuing with the export.
	return true;
}


// OnOK() calls wxWindow::Validate, then wxWindow::TransferDataFromWindow.
// If this returns TRUE, the function either calls EndModal(wxID_OK) if the
// dialog is modal, or sets the return value to wxID_OK and calls Show(FALSE)
// if the dialog is modeless.
void CExportSaveAsDlg::OnOK(wxCommandEvent& event)
{
	// save the values to the flags on the App for saving in the basic config file
	gpApp->m_bUsePrefixExportProjectNameOnFilename = pCheckUsePrefixExportProjNameOnFilename->GetValue();
	gpApp->m_bUsePrefixExportTypeOnFilename = pCheckUsePrefixExportTypeOnFilename->GetValue();
	gpApp->m_bUseSuffixExportDateTimeOnFilename = pCheckUseSuffixExportDateTimeStamp->GetValue();

	event.Skip(); //EndModal(wxID_OK); //AIModalDialog::OnOK(event); // not virtual in wxDialog
}

void CExportSaveAsDlg::OnbtnExportToTxtClick(wxCommandEvent& WXUNUSED(event))
{
	// whm added 11Aug12 set background color of selected item to pastel green
	btnExportToTxt->SetBackgroundColour(wxColour(189,255,189)); // light pastel green
	btnExportToRtf->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToXhtml->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToPathway->SetBackgroundColour(wxColour(255,255,255)); // white
    
	m_enumSaveAsType = ExportSaveAsTXT;
    bExportToRTF = false;
    SetExportTypeDescription(m_enumSaveAsType);
}

void CExportSaveAsDlg::OnbtnExportToRtfClick(wxCommandEvent& WXUNUSED(event))
{
	// whm added 11Aug12 set background color of selected item to pastel green
	btnExportToTxt->SetBackgroundColour(wxColour(255,255,255)); // light pastel green
	btnExportToRtf->SetBackgroundColour(wxColour(189,255,189)); // white
	btnExportToXhtml->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToPathway->SetBackgroundColour(wxColour(255,255,255)); // white
    
    m_enumSaveAsType = ExportSaveAsRTF;
    bExportToRTF = true;
    SetExportTypeDescription(m_enumSaveAsType);
}

void CExportSaveAsDlg::OnbtnExportToXhtmlClick(wxCommandEvent& WXUNUSED(event))
{
	// whm added 11Aug12 set background color of selected item to pastel green
	btnExportToTxt->SetBackgroundColour(wxColour(255,255,255)); // light pastel green
	btnExportToRtf->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToXhtml->SetBackgroundColour(wxColour(189,255,189)); // white
	btnExportToPathway->SetBackgroundColour(wxColour(255,255,255)); // white
    
    m_enumSaveAsType = ExportSaveAsXHTML;
    bExportToRTF = false;
    SetExportTypeDescription(m_enumSaveAsType);
}

void CExportSaveAsDlg::OnbtnExportToPathwayClick(wxCommandEvent& WXUNUSED(event))
{
	// whm added 11Aug12 set background color of selected item to pastel green
	btnExportToTxt->SetBackgroundColour(wxColour(255,255,255)); // light pastel green
	btnExportToRtf->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToXhtml->SetBackgroundColour(wxColour(255,255,255)); // white
	btnExportToPathway->SetBackgroundColour(wxColour(189,255,189)); // white
    
    m_enumSaveAsType = ExportSaveAsPathway;
    bExportToRTF = false;
    SetExportTypeDescription(m_enumSaveAsType);
}
