/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			OpenExistingProjectDlg.h
/// \author			Bill Martin
/// \date_created	20 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for COpenExistingProjectDlg class. 
/// The COpenExistingProjectDlg class works together with the 
/// OpenExistingProjectDlgFunc() dialog-defining function. Together they implement
/// Adapt It's "Access An Existing Adaptation Project" Dialog, which is called from
/// the OpenExistingAdaptionProject() and AccessOtherAdaptionProject() functions 
/// (which is in turn called from OnAdvancedTransformAdaptationsIntoGlosses) in the App.
/// \derivation		The COpenExistingProjectDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef OpenExistingProjectDlg_h
#define OpenExistingProjectDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "OpenExistingProjectDlg.h"
#endif

/// The COpenExistingProjectDlg class works together with the 
/// OpenExistingProjectDlgFunc() dialog-defining function. Together they implement
/// Adapt It's "Access An Existing Adaptation Project" Dialog, which is called from
/// the OpenExistingAdaptionProject() and AccessOtherAdaptionProject() functions 
/// (which is in turn called from OnAdvancedTransformAdaptationsIntoGlosses) in the App.
/// \derivation		The COpenExistingProjectDlg class is derived from AIModalDialog.
class COpenExistingProjectDlg : public AIModalDialog
{
// Construction
public:
	COpenExistingProjectDlg(wxWindow* parent);

// Implementation
public:
	wxString	m_projectName; // folder name for the chosen adaption project
protected:
	// message map functions
	void OnOK(wxCommandEvent& event);
	void OnSelchangeListboxAdaptions(wxCommandEvent& WXUNUSED(event));
	void OnDblclkListboxAdaptions(wxCommandEvent& WXUNUSED(event));
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	DECLARE_EVENT_TABLE()
};

#endif // OpenExistingProjectDlg_h
