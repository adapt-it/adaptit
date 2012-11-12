/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseConsistencyCheckTypeDlg.h
/// \author			Bill Martin
/// \date_created	11 July 2006
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CChooseConsistencyCheckTypeDlg class. 
/// The CChooseConsistencyCheckTypeDlg class puts up a dialog for the user to indicate
/// whether the consistency check should be done on only the current document or also on
/// other documents in the current project.
/// \derivation		The CChooseConsistencyCheckTypeDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ChooseConsistencyCheckTypeDlg_h
#define ChooseConsistencyCheckTypeDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ChooseConsistencyCheckTypeDlg.h"
#endif

/// The CChooseConsistencyCheckTypeDlg class puts up a dialog for the user to indicate
/// whether the consistency check should be done on only the current document or also on
/// other documents in the current project.
/// \derivation		The CChooseConsistencyCheckTypeDlg class is derived from AIModalDialog.
class CChooseConsistencyCheckTypeDlg : public AIModalDialog
{
public:
	CChooseConsistencyCheckTypeDlg(wxWindow* parent); // constructor
	virtual ~CChooseConsistencyCheckTypeDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_CONSISTENCY_CHECK_TYPE };

	void OnBnClickedRadioCheckOpenDocOnly(wxCommandEvent& WXUNUSED(event));
	void OnBnClickedRadioCheckSelectedDocs(wxCommandEvent& WXUNUSED(event));
	bool m_bCheckOpenDocOnly;

	wxTextCtrl* pTextCtrlAsStaticChooseConsChkType;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

private:
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ChooseConsistencyCheckTypeDlg_h */
