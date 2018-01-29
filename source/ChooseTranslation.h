/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ChooseTranslation.h
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \rcs_id $Id$
/// \copyright		2018 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CChooseTranslation and the CChooseTranslationDropDown classes. 
/// The CChooseTranslation class provides a dialog in which the user can choose 
/// either an existing translation, or enter a new translation for a given source phrase.
/// The CChooseTranslationDropDown class provides a dropdown wxComboBox that appear in lieu of
/// the CChooseTranslation dialog when called from the CPhraseBox's ChooseTranslation() function.
/// \derivation		The CChooseTranslation class is derived from AIModalDialog, and the CChooseTranslationDropDown
/// class is derived from wxComboBox.
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

// whm added 10Jan2018 to support quick selection of a translation equivalent.
// The CChooseTranslationDropDown is a combobox that appears non-modally - instead 
// of the Choose Translation dialog. It initially has its list open containing the 
// list of translation equivalents - the same string equivalents that would appear 
// in the Choose Translation dialog.
// When shown, the dropdown always tracks the position and size of the phrasebox.
// Since the edit box of the wxComboBox is functioning similarly to the phrasebox
// we'll ensure that this box gets aligned along the bottom edge of the existing 
// phrasebox and it will get the initial focus rather than the phrasebox. Hence,
// the user can use arrow keys and Enter (or Tab?) to select a list item, or simply
// start typing to enter a new translation. When typing a new translation, the 
// characters entered are echoed in the phrasebox (similarly to the way the characters
// are echoed on the free translation line when text is entered in the compose bar in
// free translation mode).
// If the user clicks in the actual phrasebox while the dropdown is open, the dropdown
// closes its list but remains available under the existing phrasebox and the focus
// simply shifts to the phrasebox where a user can could edit there in the usual way.
// If the user clicks elsewhere to reposition the phrasebox to a different location,
// the dropdown also closes and is hidden. Each time the dropdown is shown it re-
// populates its list of translation equivalences. 
class CChooseTranslationDropDown : public wxOwnerDrawnComboBox
{
    friend class CChooseTranslation;
public:
    CChooseTranslationDropDown(void); // default constructor
    
    CChooseTranslationDropDown(wxWindow * parent,
        wxWindowID id,
        const wxString & value,
        const wxPoint & pos,
        const wxSize & size,
        const wxArrayString & choices,
        long style = 0);

    virtual ~CChooseTranslationDropDown(void); // destructor

    bool bDropDownIsPoppedOpen;

    void PopulateDropDownList();
    void SizeAndPositionDropDownBox(void);
    void FocusShowAndPopup(bool bScrolling);

    wxWindow *GetControl() { return this; }

protected:
    void OnComboItemSelected(wxCommandEvent& event);
    void OnComboTextChanged(wxCommandEvent& WXUNUSED(event));
    void OnComboProcessEnterKeyPress(wxCommandEvent& WXUNUSED(event));
#if wxVERSION_NUMBER < 2900
    ;
#else
    void OnComboProcessDropDownListOpen(wxCommandEvent& WXUNUSED(event)); // these last two not apparently in wx 2.8.12 - we're not using them anyway
    void OnComboProcessDropDownListCloseUp(wxCommandEvent& WXUNUSED(event));
#endif
    void OnKeyUp(wxKeyEvent& event);
    wxCoord OnMeasureItem(size_t item) const;


private:

    DECLARE_DYNAMIC_CLASS(CChooseTranslationDropDown)
    // DECLARE_DYNAMIC_CLASS() is used inside a class declaration to
    // declare that the objects of this class should be dynamically
    // creatable from run-time type information.

    DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
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
	CKB* m_pKB;
	int  m_nWordsInPhrase;
	MapKeyStringToTgtUnit* m_pMap;
	void PopulateList(CTargetUnit* pTU, int selectionIndex, enum SelectionWanted doSel);

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ChooseTranslation_h */
