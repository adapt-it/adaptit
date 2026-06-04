/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KbSvrHowGetUrl.h
/// \author			Bruce Waters
/// \date_created	25 January 2016
/// \rcs_id $Id: KbSvrHowGetUrl.h 3028 2016-01-25 11:38:00Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the KbSvrHowGetUrl class.
/// The KbSvrHowGetUrl class provides a dialog for letting the user decide whether 
/// to do service discovery on the local LAN to find a running KBserver, or to 
/// manually type in an ipAddress he knows (e.g. for a KBserver running on the web, or
/// on a different subnet of the LAN, where service discovery would be of no benefit).
/// The dialog consists of a wxRadioBox (two radio buttons), an OK button and a
/// Cancel button. The same controls are also in the KbSharingSetup.h and .cpp
/// files, and the KbSvrHowGetUrl.h and .cpp files are just a cut-down version
/// of that dialog. 
/// This dialog is used in the ProjectPage of the wizard, and for access to the
/// KB Sharing Manager (tabbed dialog), and in the HookUpToExistingAIProject in
/// the CollabUtilities.cpp file. It is not used in KbSharingSetup.cpp because
/// the same controls are already built in to the latter dialog.
/// BEW 27Jul20 keep name, but the class now deals with ipAddress, not a U R L
/// \derivation		The KbSvrHowGetUrl class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef KbSvrHowGetUrl_h
#define KbSvrHowGetUrl_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "KbSvrHowGetUrl.h"
#endif

//#if defined(_KBSERVER)

class CAdapt_ItApp;
// BEW 27Jul20 keep the Url in the name, even though new it's dealing only with an ipAddress
class KbSvrHowGetUrl : public AIModalDialog
{
public:
	// Expose all overloads of InitDialog from the base class (wxWindowBase)
	using wxWindowBase::InitDialog; // whm 6Dec2025 added to avoid gcc warning
	KbSvrHowGetUrl(wxWindow* parent); // constructor

	virtual ~KbSvrHowGetUrl(void); // destructor

	bool m_bUserClickedCancel;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	CAdapt_ItApp* m_pApp;
	wxButton*   m_pBtnOK; //wxID_OK
	wxSpinCtrl* m_pTimeout;
	wxRadioBox*	m_pRadioBoxHow;
	int			m_nRadioBoxSelection;
	int			m_nTimeout;
	DECLARE_EVENT_TABLE()
};

//#endif

#endif /* KbSvrHowGetUrl_h */
