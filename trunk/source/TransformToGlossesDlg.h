/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			TransformToGlossesDlg.h
/// \author			Bill Martin
/// \date_created	29 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CTransformToGlossesDlg class. 
/// The CTransformToGlossesDlg class is a dialog with a "Yes" and "No" buttons to verify
/// from the user that it is Ok to discard any adaptations from the current document in
/// order to create a glossing KB from the former adaptations KB. A "Yes" would then call
/// up the OpenExistingProjectDlg.
/// \derivation		The CTransformToGlossesDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef TransformToGlossesDlg_h
#define TransformToGlossesDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "TransformToGlossesDlg.h"
#endif

/// The CTransformToGlossesDlg class is a dialog with a "Yes" and "No" buttons to verify
/// from the user that it is Ok to discard any adaptations from the current document in
/// order to create a glossing KB from the former adaptations KB. A "Yes" would then call
/// up the OpenExistingProjectDlg.
/// \derivation		The CTransformToGlossesDlg class is derived from AIModalDialog.
class CTransformToGlossesDlg : public AIModalDialog
{
public:
	CTransformToGlossesDlg(wxWindow* parent); // constructor
	virtual ~CTransformToGlossesDlg(void); // destructor // whm make all destructors virtual
	// other methods
	//enum { IDD = IDD_TRANSFORM_TO_GLOSSES };
	
	wxSizer* pTransformToGlossesDlgSizer;
	
	wxTextCtrl* pTextCtrlStatic1;
	wxTextCtrl* pTextCtrlStatic2;
	wxTextCtrl* pTextCtrlStatic3;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* TransformToGlossesDlg_h */
