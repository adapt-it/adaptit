/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ChooseTranslation.h
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CChooseTranslation class. 
/// The CChooseTranslation class provides a dialog in which the user can choose 
/// either an existing translation, or enter a new translation for a given source phrase.
/// \derivation		The CChooseTranslation class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ChooseTranslation_h
#define ChooseTranslation_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ChooseTranslation.h"
#endif

#include "MyListBox.h"

class CAdapt_ItView;

/// The CChooseTranslation class provides a dialog in which the user can choose 
/// either an existing translation, or enter a new translation for a given source phrase.
/// \derivation		The CChooseTranslation class is derived from AIModalDialog.
class CChooseTranslation : public AIModalDialog
{
public:
	CChooseTranslation(wxWindow* parent); // constructor
	virtual ~CChooseTranslation(void); // destructor // whm make all destructors virtual

	//enum { IDD = IDD_DIALOG_CHOOSE_ADAPTATION };
	wxSizer*	pChooseTransSizer;
	CMyListBox*	m_pMyListBox;
	wxTextCtrl*	m_pSourcePhraseBox;
	wxTextCtrl*	m_pNewTranslationBox;
	wxTextCtrl* m_pEditReferences;
	int			m_refCount;
	wxString	m_refCountStr; // need wxString for validating wxTextCtrl
	wxString	m_chosenTranslation;
	bool		m_bEmptyAdaptationChosen;
	bool		m_bCancelAndSelect;
	bool		m_bHideCancelAndSelectButton;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

	void OnButtonMoveUp(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonMoveUp(wxUpdateUIEvent& event);
	void OnButtonMoveDown(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonMoveDown(wxUpdateUIEvent& event);
	virtual void OnCancel(wxCommandEvent& WXUNUSED(event));
	void OnSelchangeListboxTranslations(wxCommandEvent& WXUNUSED(event));
	void OnDblclkListboxTranslations(wxCommandEvent& WXUNUSED(event));
	void OnButtonRemove(wxCommandEvent& WXUNUSED(event));
	void OnUpdateButtonRemove(wxUpdateUIEvent& event);
	void OnButtonCancelAsk(wxCommandEvent& WXUNUSED(event));
	void OnButtonCancelAndSelect(wxCommandEvent& event);
public:
	void OnKeyDown(wxKeyEvent& event);

private:

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ChooseTranslation_h */
