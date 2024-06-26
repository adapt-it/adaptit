/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseTranslation.h
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \rcs_id $Id$
/// \copyright		2018 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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

#include <wx/combobox.h>
#include <wx/combo.h>
#include <wx/odcombo.h>
#include "MyListBox.h"

class MapKeyStringToTgtUnit;; 
class CAdapt_ItView;

enum SelectionWanted
{
    No,
    Yes
};

/// The CChooseTranslation class provides a dialog in which the user can choose 
/// either an existing translation, or enter a new translation for a given source phrase.
/// \derivation		The CChooseTranslation class is derived from AIModalDialog.
/// BEW 2July10, this class has been updated to support kbVersion 2
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
    // whm 17Jul2018 removed check box to auto-open dropdown on arrival at location with multiple translations
    // This checkbox was mainly added as a temporary option due to problems with the wxOwnerDrawnComboBox 
    // derived control - now fixed.
    //wxCheckBox* m_pCheckAutoOpenPhraseboxOnLanding;
	int			m_refCount;
	wxString	m_refCountStr; // need wxString for validating wxTextCtrl
	wxString	m_chosenTranslation;
	bool		m_bEmptyAdaptationChosen;
	//bool		m_bCancelAndSelect; // whm 24Feb2018 removed - there is no longer any Cancel and Select functionality
    bool		m_bHideCancelAndSelectButton;
    bool        m_bTempUseChooseTransDropDown;  // whm 10Jan2018 added

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
	//void OnButtonCancelAndSelect(wxCommandEvent& event); // whm 24Feb2018 removed - not needed with dropdown integrated in CPhraseBox class
public:
	//void OnKeyDown(wxKeyEvent& event); - not needed with dropdown integrated in CPhraseBox class

private:
	CKB* m_pKB;
	int  m_nWordsInPhrase;
	MapKeyStringToTgtUnit* m_pMap;
	void PopulateList(CTargetUnit* pTU, int selectionIndex, enum SelectionWanted doSel);
	bool m_bRemovingEntryFromList;

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ChooseTranslation_h */
