/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CreateNewAIProjForCollab.h
/// \author			Bill Martin
/// \date_created	23 February 2012
/// \rcs_id $Id$
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CCreateNewAIProjForCollab class. 
/// The CCreateNewAIProjForCollab class implements a simple dialog that allows the user to
/// enter the source language and target language names that are to be used for a new Adapt It
/// project.
/// \derivation		The CCreateNewAIProjForCollab class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CreateNewAIProjForCollab_h
#define CreateNewAIProjForCollab_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CreateNewAIProjForCollab.h"
#endif

class CCreateNewAIProjForCollab : public AIModalDialog
{
public:
	CCreateNewAIProjForCollab(wxWindow* parent); // constructor
	virtual ~CCreateNewAIProjForCollab(void); // destructor
	// other methods
	
	wxStaticText* pStaticTextTopInfoLine1;
	wxStaticText* pStaticTextTopInfoLine2;
	wxTextCtrl* pTextCtrlSrcLangName;
	wxTextCtrl* pTextCtrlSrcLangCode;
	wxTextCtrl* pTextCtrlTgtLangName;
	wxTextCtrl* pTextCtrlTgtLangCode;
	wxTextCtrl* pTextCtrlNewAIProjName;
	wxButton* pBtnLookupCodes;
	wxSizer* pCreateNewAIProjForCollabSizer;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnEnChangeSrcLangName(wxCommandEvent& WXUNUSED(event));
	void OnEnChangeTgtLangName(wxCommandEvent& WXUNUSED(event));
	void OnBtnLookupCodes(wxCommandEvent& WXUNUSED(event));

private:
	CAdapt_ItApp* m_pApp;
	// class attributes
	// wxString m_stringVariable;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* CreateNewAIProjForCollab_h */
