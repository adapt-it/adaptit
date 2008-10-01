/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			WhichFilesDlg.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CWhichFilesDlg class. 
/// The CWhichFilesDlg class presents the user with a 
/// dialog in which various files can be selected or deselected and by moving them 
/// right or left into different lists (the will be used list, and the will not be
/// used list), by pressing a large check mark button (to include), or a large X 
/// button (to exclude). The interface resources for the CWhichFilesDlg are defined 
/// in WhichFilesDlgFunc() which was developed and is maintained by wxDesigner.
/// \derivation		The CWhichFilesDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef WhichFilesDlg_h
#define WhichFilesDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "WhichFilesDlg.h"
#endif

/// The CWhichFilesDlg class presents the user with a 
/// dialog in which various files can be selected or deselected and by moving them 
/// right or left into different lists (the will be used list, and the will not be
/// used list), by pressing a large check mark button (to include), or a large X 
/// button (to exclude). The interface resources for the CWhichFilesDlg are defined 
/// in WhichFilesDlgFunc() which was developed and is maintained by wxDesigner.
/// \derivation		The CWhichFilesDlg class is derived from AIModalDialog.
class CWhichFilesDlg : public AIModalDialog
{
public:
	CWhichFilesDlg(wxWindow* parent); // constructor
	virtual ~CWhichFilesDlg(void); // destructor // whm make all destructors virtual
	// other methods

	//enum { IDD = IDD_WHICH_FILES };
	wxSizer* pWhichFilesDlgSizer;
	wxListBox*	m_pListBoxRejected;
	wxListBox*	m_pListBoxAccepted;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnSelchangeListRejected(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListAccepted(wxCommandEvent& WXUNUSED(event));
	void OnButtonReject(wxCommandEvent& WXUNUSED(event));
	void OnButtonAccept(wxCommandEvent& WXUNUSED(event));

	wxBitmapButton	m_rejectBtn;
	wxBitmapButton	m_acceptBtn;


private:
	// class attributes
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* WhichFilesDlg_h */
