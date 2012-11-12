/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SilConverterSelectDlg.h
/// \author			Bill Martin
/// \date_created	28 April 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CSilConverterSelectDlg class. 
/// The CSilConverterSelectDlg class uses Bob Eaton's ECDriver.dll dynamic library so that 
/// Adapt It (Windows port only) can interface with SILConverters functionality as a cc-like 
/// filter for source phrases entered in the phrase box.
/// \derivation		The CSilConverterSelectDlg class is derived from AIModalDialog.

#ifndef SilConverterSelectDlg_h
#define SilConverterSelectDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SilConverterSelectDlg.h"
#endif

// whm 29Dec09: the following typedef is needed to interface properly with Bob's MFC type code
typedef int BOOL;

class CSilConverterSelectDlg : public AIModalDialog
{
public:
	CSilConverterSelectDlg(
		const wxString&	strConverterName, 
        BOOL			bDirectionForward,
		int				eNormalizeOutput,
		wxWindow* parent); // constructor
	virtual ~CSilConverterSelectDlg(void); // destructor
	//enum { IDD = IDD_DLG_SILCONVERTERS };

    wxString        ConverterName;
    BOOL            DirectionForward;
	wxTextCtrl*		pEditSILConverterName; // IDC-ED_SILCONVERTER_NAME
	wxTextCtrl*		pEditSILConverterInfo; // IDC_ED_SILCONVERTER_INFO (read only)
	wxButton*		pBtnUnload; // IDC_BTN_CLEAR_SILCONVERTER
	int				NormalizeOutput;

	// other methods
    void OnBnClickedBtnClearSilconverter(wxCommandEvent& WXUNUSED(event));

protected:
    wxString m_strConverterDescription;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
    void OnBnClickedBtnSelectSilconverter(wxCommandEvent& event);

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* SilConverterSelectDlg_h */
