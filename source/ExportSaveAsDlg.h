/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ExportSaveAsDlg.h
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 June 2012 (edb rework for xhtml and pathway options)
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General
///			Public License (see license directory)
/// \description	This is the header file for the CExportSaveAsDlg class.
/// 			The CExportSaveAsDlg class provides a dialog in which
///			the user can indicate the format of the exported text.
///			Current options are text, RTF, xhtml and Pathway
///			(which is just the Pathway program run on an xhtml output).
/// 			The dialog also has an "Export/Filter Options" button
///			to access that sub-dialog.
/// \derivation		The CExportSaveAsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ExportSaveAsDlg_H
#define ExportSaveAsDlg_H

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ExportSaveAsDlg.h"
#endif

//(*Headers(ExportSaveAsDlg)
#include <wx/bmpbuttn.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/panel.h>
#include <wx/stattext.h>
//*)

enum ExportSaveAsType
{
	ExportSaveAsTXT,
	ExportSaveAsRTF,
	ExportSaveAsXHTML,
	ExportSaveAsPathway // technically, save as xhtml and launch Pathway. :)
};


/// The CExportSaveAsDlg class provides a dialog in which the user can indicate
/// the format of exported source text (or target text). The dialog also has an
/// "Export/Filter Options" button to access that sub-dialog.
/// \derivation		The CExportSaveAsDlg class is derived from AIModalDialog.
class CExportSaveAsDlg: public AIModalDialog
{
public:

	CExportSaveAsDlg(wxWindow* parent);
	virtual ~CExportSaveAsDlg();

	//(*Declarations(ExportSaveAsDlg)
	wxPanel* Panel1;
	wxStaticText* lblExportTo;
	wxBitmapButton* btnExportToPathway;
	wxBitmapButton* btnExportToXhtml;
	wxBitmapButton* btnExportToTxt;
	wxBitmapButton* btnExportToRtf;
	wxStaticText* lblExportTypeDescription;
	wxRadioButton* rdoFilterOff;
	wxRadioButton* rdoFilterOn;
	wxButton* btnFilterOptions;
	wxCheckBox* pCheckUsePrefixExportTypeOnFilename;
	wxCheckBox* pCheckUseSuffixExportDateTimeStamp;
	wxCheckBox* pCheckUsePrefixExportProjNameOnFilename;
	//*)

	ExportType exportType;

	inline ExportSaveAsType GetSaveAsType()
	{
		return m_enumSaveAsType;
	}

protected:

	//(*Identifiers(CExportSaveAsDlg)
	static const long ID_LBLEXPORTTO;
	static const long ID_BTNEXPORTTOTXT;
	static const long ID_BTNEXPORTTORTF;
	static const long ID_BTNEXPORTTOXHTML;
	static const long ID_BTNEXPORTTOPATHWAY;
	static const long ID_PANEL1;
	static const long ID_LBLEXPORTTYPEDESCRIPTION;
	static const long ID_RDOFILTEROFF;
	static const long ID_RDOFILTERON;
	static const long ID_BTNFILTEROPTIONS;
	static const long ID_CHKPROJECTNAMEPREFIX;
	static const long ID_CHKTARGETTEXTPREFIX;
	static const long ID_CHKDATETIMESUFFIX;
	//*)

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:

	//(*Handlers(CExportSaveAsDlg)
	void OnchkDateTimeSuffixClick(wxCommandEvent& event);
	void OnchkProjectNamePrefixClick(wxCommandEvent& event);
	void OnchkTargetTextPrefixClick(wxCommandEvent& event);
	void OnrdoFilterOnSelect(wxCommandEvent& event);
	void OnrdoFilterOffSelect(wxCommandEvent& event);
	void OnbtnFilterOptionsClick(wxCommandEvent& event);
	void OnbtnExportToTxtClick(wxCommandEvent& event);
	void OnbtnExportToRtfClick(wxCommandEvent& event);
	void OnbtnExportToXhtmlClick(wxCommandEvent& event);
	void OnbtnExportToPathwayClick(wxCommandEvent& event);
	//*)

	ExportSaveAsType m_enumSaveAsType;

	void SetExportTypeDescription(ExportSaveAsType newType);

	DECLARE_EVENT_TABLE()
};

#endif
