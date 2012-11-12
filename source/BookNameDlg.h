/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			BookNameDlg.h
/// \author			Bruce Waters
/// \date_created	7 August 2012
/// \rcs_id $Id$
/// \copyright		2012 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CBookName class. 
/// The CBookName class provides a handler for the "Change Book Name..." menu item. It is
/// also called at other times, not just due to an explicit menu item click - such as when
/// creating a new document, or opening a document which has no book name defined but does
/// have a valid bookID code within it. This handler class lets the user get an appropriate
/// book name defined, given a valid bookID code -- it supports the Paratext list of 123
/// book ids and full book name strings (the latter are localizable). A book name is
/// needed for exports of xhtml or to Pathway, so this dialog provides the functionality
/// for associating a book name with every document which contains a valid bookID.
/// \derivation		The CBookName class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CBookName_h
#define CBookName_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "BookNameDlg.h"
#endif

class CBookName : public AIModalDialog
{
public:
	CBookName(
		wxWindow*	parent,
		wxString*	title,
		wxString*	pstrBookCode, 
		bool		bShowCentered); // constructor
	virtual ~CBookName(); // destructor

	// member variables
	wxString		m_currentBookName; // grabbed from app's m_bookName_Current member, and stored here
	wxString		m_newBookName; // empty, but if not, it will have the newly typed book name or the suggested name
	wxString		m_suggestedBookName; // get from Paratext list of book names, and show in radio button label

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnRadioSuggestedName(wxCommandEvent& WXUNUSED(event));
	void OnRadioInappropriateBookName(wxCommandEvent& WXUNUSED(event));
	void OnRadioUseCurrentBookName(wxCommandEvent& WXUNUSED(event));
	void OnRadioUseDifferentBookName(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	wxSizer*		m_pBookNameDlgSizer;
	wxString		m_checkSuggestionLabelStr; // for the "The suggested bookname, %s, is acceptable" string
	bool			m_bShowItCentered;
	wxString		m_bookCode; // store the passed in book code here
	wxString		m_radioLabelStr; //for the suggested name one

	wxRadioButton*	m_pRadioSuggestedName;
	wxRadioButton*	m_pRadioUseCurrent;
	wxRadioButton*	m_pRadioTypeMyOwn;
	wxRadioButton*	m_pRadioInappropriateName;
	wxTextCtrl*		m_pTextCtrl_CurrentBookName;
	wxTextCtrl*		m_pTextCtrl_TypeNewBookName;

	DECLARE_EVENT_TABLE()
};
#endif /* CBookName_h */
