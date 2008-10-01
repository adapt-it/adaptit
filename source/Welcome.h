/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Welcome.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CWelcome class. 
/// The CWelcome class provides an opening splash screen to welcome a new user.
/// The screen has a checkbox to allow the user to turn it off for subsequent
/// runs of the application.
/// \derivation		The CWelcome class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef Welcome_h
#define Welcome_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Welcome.h"
#endif

/// The CWelcome class provides an opening splash screen to welcome a new user.
/// The screen has a checkbox to allow the user to turn it off for subsequent
/// runs of the application.
/// \derivation		The CWelcome class is derived from AIModalDialog.
class CWelcome : public AIModalDialog
{
public:
	CWelcome(wxWindow* parent); // constructor
	virtual ~CWelcome(void); // destructor // whm make all destructors virtual
	// other methods
	//enum { IDD = IDD_WELCOME };
	bool m_bSuppressWelcome;
	void OnCheckNolongerShow(wxCommandEvent& WXUNUSED(event));

protected:
	CAdapt_ItView* pMyParent;
	wxCheckBox* pCheckBtn;
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* Welcome_h */
