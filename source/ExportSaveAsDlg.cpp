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
const long CExportSaveAsDlg::ID_LBLEXPORTTYPEDESCRIPTION = wxNewId();
const long CExportSaveAsDlg::ID_RDOFILTEROFF = wxNewId();
const long CExportSaveAsDlg::ID_RDOFILTERON = wxNewId();
const long CExportSaveAsDlg::ID_BTNFILTEROPTIONS = wxNewId();
const long CExportSaveAsDlg::ID_CHKPROJECTNAMEPREFIX = wxNewId();
const long CExportSaveAsDlg::ID_CHKTARGETTEXTPREFIX = wxNewId();
const long CExportSaveAsDlg::ID_CHKDATETIMESUFFIX = wxNewId();
//*)

BEGIN_EVENT_TABLE(CExportSaveAsDlg,wxDialog)
	//(*EventTable(CExportSaveAsDlg)
	EVT_INIT_DIALOG(CExportSaveAsDlg::InitDialog)
	EVT_BUTTON(wxID_OK, CExportSaveAsDlg::OnOK)
	//*)
END_EVENT_TABLE()

CExportSaveAsDlg::CExportSaveAsDlg(wxWindow* parent) // dialog constructor
	: AIModalDialog(parent, -1, _("Export Document"),
		wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	//(*Initialize(CExportSaveAsDlg)
	wxStaticBoxSizer* StaticBoxSizer2;
	wxFlexGridSizer* FlexGridSizer1;
	wxBoxSizer* BoxSizer3;
	wxStaticBoxSizer* StaticBoxSizer3;
	wxBoxSizer* BoxSizer1;
	wxStdDialogButtonSizer* StdDialogButtonSizer1;
	wxStaticBoxSizer* StaticBoxSizer1;

	FlexGridSizer1 = new wxFlexGridSizer(4, 1, 0, 0);
	Panel1 = new wxPanel(this, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("ID_PANEL1"));
	Panel1->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	BoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
	lblExportTo = new wxStaticText(Panel1, ID_LBLEXPORTTO, _("Export to:"), wxDefaultPosition, wxDefaultSize, 0, _T("ID_LBLEXPORTTO"));
	BoxSizer1->Add(lblExportTo, 1, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
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
	FlexGridSizer1->Add(Panel1, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	StaticBoxSizer1 = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Description"));
	lblExportTypeDescription = new wxStaticText(this, ID_LBLEXPORTTYPEDESCRIPTION, _("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean posuere dictum sem id elementum. Mauris lobortis, sapien nec iaculis condimentum, turpis risus posuere turpis, vel condimentum magna libero at lectus."), wxDefaultPosition, wxSize(425,75), 0, _T("ID_LBLEXPORTTYPEDESCRIPTION"));
	StaticBoxSizer1->Add(lblExportTypeDescription, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FlexGridSizer1->Add(StaticBoxSizer1, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	StaticBoxSizer3 = new wxStaticBoxSizer(wxVERTICAL, this, _("Export Filters"));
	rdoFilterOff = new wxRadioButton(this, ID_RDOFILTEROFF, _("Export all markers and text"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP, wxDefaultValidator, _T("ID_RDOFILTEROFF"));
	StaticBoxSizer3->Add(rdoFilterOff, 1, wxTOP|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	BoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
	rdoFilterOn = new wxRadioButton(this, ID_RDOFILTERON, _("Filter out selected markers and text:"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_RDOFILTERON"));
	BoxSizer3->Add(rdoFilterOn, 2, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	btnFilterOptions = new wxButton(this, ID_BTNFILTEROPTIONS, _("&Options..."), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_BTNFILTEROPTIONS"));
	BoxSizer3->Add(btnFilterOptions, 1, wxBOTTOM|wxLEFT|wxRIGHT|wxSHAPED|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	StaticBoxSizer3->Add(BoxSizer3, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
	FlexGridSizer1->Add(StaticBoxSizer3, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	StaticBoxSizer2 = new wxStaticBoxSizer(wxVERTICAL, this, _("Export filename"));
	pCheckUsePrefixExportProjNameOnFilename = new wxCheckBox(this, ID_CHKPROJECTNAMEPREFIX, _("Use project &name prefix \"%s\" on export filename"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHKPROJECTNAMEPREFIX"));
	pCheckUsePrefixExportProjNameOnFilename->SetValue(false);
	StaticBoxSizer2->Add(pCheckUsePrefixExportProjNameOnFilename, 1, wxTOP|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	pCheckUsePrefixExportTypeOnFilename = new wxCheckBox(this, ID_CHKTARGETTEXTPREFIX, _("Use export &type prefix \"%s\" on export filename"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHKTARGETTEXTPREFIX"));
	pCheckUsePrefixExportTypeOnFilename->SetValue(false);
	StaticBoxSizer2->Add(pCheckUsePrefixExportTypeOnFilename, 1, wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	pCheckUseSuffixExportDateTimeStamp = new wxCheckBox(this, ID_CHKDATETIMESUFFIX, _("Use &date-time suffix on export filename"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHKDATETIMESUFFIX"));
	pCheckUseSuffixExportDateTimeStamp->SetValue(false);
	StaticBoxSizer2->Add(pCheckUseSuffixExportDateTimeStamp, 1, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	FlexGridSizer1->Add(StaticBoxSizer2, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	StdDialogButtonSizer1 = new wxStdDialogButtonSizer();
	StdDialogButtonSizer1->AddButton(new wxButton(this, wxID_OK, wxEmptyString));
	StdDialogButtonSizer1->AddButton(new wxButton(this, wxID_CANCEL, wxEmptyString));
	StdDialogButtonSizer1->Realize();
	FlexGridSizer1->Add(StdDialogButtonSizer1, 1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	SetSizer(FlexGridSizer1);
	FlexGridSizer1->Fit(this);
	FlexGridSizer1->SetSizeHints(this);

	Connect(ID_BTNEXPORTTOTXT,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnbtnExportToTxtClick);
	Connect(ID_BTNEXPORTTORTF,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnbtnExportToRtfClick);
	Connect(ID_BTNEXPORTTOXHTML,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnbtnExportToXhtmlClick);
	Connect(ID_BTNEXPORTTOPATHWAY,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnbtnExportToPathwayClick);
	Connect(ID_RDOFILTEROFF,wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&CExportSaveAsDlg::OnrdoFilterOffSelect);
	Connect(ID_RDOFILTERON,wxEVT_COMMAND_RADIOBUTTON_SELECTED,(wxObjectEventFunction)&CExportSaveAsDlg::OnrdoFilterOnSelect);
	Connect(ID_BTNFILTEROPTIONS,wxEVT_COMMAND_BUTTON_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnbtnFilterOptionsClick);
	Connect(ID_CHKPROJECTNAMEPREFIX,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnchkProjectNamePrefixClick);
	Connect(ID_CHKTARGETTEXTPREFIX,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnchkTargetTextPrefixClick);
	Connect(ID_CHKDATETIMESUFFIX,wxEVT_COMMAND_CHECKBOX_CLICKED,(wxObjectEventFunction)&CExportSaveAsDlg::OnchkDateTimeSuffixClick);
	//*)
}

CExportSaveAsDlg::~CExportSaveAsDlg()
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
	// TODO: if pathway is not installed, disable the button(?)
	// (actually, I'm not sure about that. How do we tell people that PW is not installed?)

    // export format description
    m_enumSaveAsType = ExportSaveAsTXT;
    SetExportTypeDescription(m_enumSaveAsType);

    // filter radio buttons and filter options button
    rdoFilterOff->SetValue(bExportAll);
    rdoFilterOn->SetValue(!bExportAll);
    btnFilterOptions->Enable(!bExportAll); // if there is no filter, disable the filter options button

	// export file naming rules checkboxes
	pCheckUsePrefixExportProjNameOnFilename->SetValue(gpApp->m_bUsePrefixExportProjectNameOnFilename);
	pCheckUsePrefixExportTypeOnFilename->SetValue(gpApp->m_bUsePrefixExportTypeOnFilename);
	pCheckUseSuffixExportDateTimeStamp->SetValue(gpApp->m_bUseSuffixExportDateTimeOnFilename);
	// Note: the caller DoExportSfmText() accesses the above two checkbox values and may
	// enable or disable the checkboxes as appropriate to the exporting context there.
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

void CExportSaveAsDlg::OnchkDateTimeSuffixClick(wxCommandEvent& event)
{
}

void CExportSaveAsDlg::OnchkProjectNamePrefixClick(wxCommandEvent& event)
{
}

void CExportSaveAsDlg::OnchkTargetTextPrefixClick(wxCommandEvent& event)
{
}

void CExportSaveAsDlg::OnrdoFilterOnSelect(wxCommandEvent& event)
{
    bExportAll = FALSE;
    bExportSelectedMarkersOnly = TRUE;
    btnFilterOptions->Enable();
}

void CExportSaveAsDlg::OnrdoFilterOffSelect(wxCommandEvent& event)
{
    bExportAll = TRUE;
    bExportSelectedMarkersOnly = FALSE;
    btnFilterOptions->Disable();
}

void CExportSaveAsDlg::OnbtnFilterOptionsClick(wxCommandEvent& event)
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


void CExportSaveAsDlg::OnbtnExportToTxtClick(wxCommandEvent& event)
{
    m_enumSaveAsType = ExportSaveAsTXT;
    bExportToRTF = false;
    SetExportTypeDescription(m_enumSaveAsType);
}

void CExportSaveAsDlg::OnbtnExportToRtfClick(wxCommandEvent& event)
{
    m_enumSaveAsType = ExportSaveAsRTF;
    bExportToRTF = true;
    SetExportTypeDescription(m_enumSaveAsType);
}

void CExportSaveAsDlg::OnbtnExportToXhtmlClick(wxCommandEvent& event)
{
    m_enumSaveAsType = ExportSaveAsXHTML;
    bExportToRTF = false;
    SetExportTypeDescription(m_enumSaveAsType);
}

void CExportSaveAsDlg::OnbtnExportToPathwayClick(wxCommandEvent& event)
{
    m_enumSaveAsType = ExportSaveAsPathway;
    bExportToRTF = false;
    SetExportTypeDescription(m_enumSaveAsType);
}
