/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			UnpackWarningDlg.h
/// \author			Bill Martin
/// \date_created	20 July 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CUnpackWarningDlg class. 
/// The CUnpackWarningDlg class provides a dialog that issues a warning to the user
/// that the document was packed by the Unicode version of the program and now the
/// non-Unicode version is attempting to unpack it, or vs versa.
/// \derivation		The CUnpackWarningDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef UnpackWarningDlg_h
#define UnpackWarningDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "UnpackWarningDlg.h"
#endif

/// The CUnpackWarningDlg class provides a dialog that issues a warning to the user
/// that the document was packed by the Unicode version of the program and now the
/// non-Unicode version is attempting to unpack it, or vs versa.
/// \derivation		The CUnpackWarningDlg class is derived from AIModalDialog.
class CUnpackWarningDlg : public AIModalDialog
{
public:
	CUnpackWarningDlg(wxWindow* parent); // constructor
	virtual ~CUnpackWarningDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_AI_AIUNICODE_MISMATCH };
	wxSizer* pUnpackDlgSizer;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* UnpackWarningDlg_h */
