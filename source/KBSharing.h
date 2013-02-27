/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBSharing.h
/// \author			Bruce Waters
/// \date_created	14 January 2013
/// \rcs_id $Id: KBSharing.h 3025 2013-01-14 18:18:00Z jmarsden6@gmail.com $
/// \copyright		2013 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KBSharing class.
/// The KBSharing class provides a dialog for the turning on or off KB Sharing, and for
/// controlling the non-automatic functionalities within the KB sharing feature.
/// \derivation		The KBSharing class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KBSharing_h
#define KBSharing_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KBSharing.h"
#endif

#if defined(_KBSERVER)

class KBSharing : public AIModalDialog
{
public:
	KBSharing(wxWindow* parent); // constructor
	virtual ~KBSharing(void); // destructor

	// other methods
	wxButton*		m_pBtnGetAll;
	wxRadioBox*		m_pRadioBox;
	wxSpinCtrl*		m_pSpinReceiving;
	//wxSpinCtrl*		m_pSpinSending;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	void OnBtnGetAll(wxCommandEvent& WXUNUSED(event));
	void OnBtnChangedSince(wxCommandEvent& WXUNUSED(event));
	void OnBtnSendAll(wxCommandEvent& WXUNUSED(event));
	void OnRadioOnOff(wxCommandEvent& WXUNUSED(event));
	void OnSpinCtrlReceiving(wxSpinEvent& WXUNUSED(event));
	//void OnSpinCtrlSending(wxSpinEvent& WXUNUSED(event));

private:
	CAdapt_ItApp* m_pApp;
	int	m_nRadioBoxSelection;
	int receiveInterval; //(minutes)
	int oldReceiveInterval; //(minutes) to compare, to see if user changed

	DECLARE_EVENT_TABLE()
};

#endif

#endif /* KBSharing_h */
