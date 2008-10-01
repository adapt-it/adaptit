/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			SetDelay.h
/// \author			Bill Martin
/// \date_created	14 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CSetDelay class. 
/// The CSetDelay class provides a dialog that allows the user to set a time delay
/// to slow down the automatic insertions of adaptations. This can be helpful to some
/// who want to read what is being inserted as it is being inserted.
/// \derivation		The CSetDelay class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef SetDelay_h
#define SetDelay_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SetDelay.h"
#endif

/// The CSetDelay class provides a dialog that allows the user to set a time delay
/// to slow down the automatic insertions of adaptations. This can be helpful to some
/// who want to read what is being inserted as it is being inserted.
/// \derivation		The CSetDelay class is derived from AIModalDialog.
class CSetDelay : public AIModalDialog
{
public:
	CSetDelay(wxWindow* parent); // constructor
	virtual ~CSetDelay(void); // destructor
	// other methods
	//enum { IDD = IDD_DELAY_DLG };

	wxTextCtrl* m_pDelayBox;
	int m_nDelay;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* SetDelay_h */
