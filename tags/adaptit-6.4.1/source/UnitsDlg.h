/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			UnitsDlg.h
/// \author			Bill Martin
/// \date_created	22 May 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CUnitsDlg class. 
/// The CUnitsDlg class presents the user with a dialog with a choice between
/// Inches and Centimeters, for use primarily in page setup and printing.
/// \derivation		The CUnitsDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef UnitsDlg_h
#define UnitsDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "UnitsDlg.h"
#endif

// forward reference
class CAdapt_ItView;

/// The CUnitsDlg class presents the user with a dialog with a choice between
/// Inches and Centimeters, for use primarily in page setup and printing.
/// \derivation		The CUnitsDlg class is derived from AIModalDialog.
class CUnitsDlg : public AIModalDialog
{
public:
	//enum { IDD = IDD_UNITS_DLG };
	CUnitsDlg(wxWindow* parent); // constructor
	virtual ~CUnitsDlg(void); // destructor // whm make all destructors virtual

	CAdapt_ItView* pView;

protected:
	bool tempUseInches;
	wxRadioButton* m_pRadioUseInches;
	wxRadioButton* m_pRadioUseCentimeters;

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& WXUNUSED(event)); 
	
	void OnRadioUseInches(wxCommandEvent& WXUNUSED(event));
	void OnRadioUseCentimeters(wxCommandEvent& WXUNUSED(event));

private:

	// MFC uses DECLARE_MESSAGE_MAP()
	DECLARE_EVENT_TABLE() 
};
#endif /* UnitsDlg_h */
