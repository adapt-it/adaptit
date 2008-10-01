/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			OutputFilenameDlg.h
/// \author			Bill Martin
/// \date_created	3 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the COutputFilenameDlg class. 
/// The COutputFilenameDlg class works together with the GetOutputFilenameDlgFunc()
/// dialog which was created and is maintained by wxDesigner. Together they 
/// implement the dialog used for getting a suitable name for the source data 
/// (title only, no extension).
/// \derivation		The COutputFilenameDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef OutputFilenameDlg_h
#define OutputFilenameDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "OutputFilenameDlg.h"
#endif

/// The COutputFilenameDlg class works together with the GetOutputFilenameDlgFunc()
/// dialog which was created and is maintained by wxDesigner. Together they 
/// implement the dialog used for getting a suitable name for the source data 
/// (title only, no extension).
/// \derivation		The COutputFilenameDlg class is derived from AIModalDialog.
class COutputFilenameDlg : public AIModalDialog
{
// Construction
public:
	COutputFilenameDlg(wxWindow* parent); // constructor

// Dialog Data
	//enum { IDD = IDD_GET_FILENAME }; // MFC IDD
	wxString	m_strFilename;
	wxTextCtrl* pEdit;
	wxStaticText* pStaticTextInvalidCharacters;

// Implementation
protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

#endif // OutputFilenameDlg_h
