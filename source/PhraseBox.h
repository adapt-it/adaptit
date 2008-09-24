/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			PhraseBox.h
/// \author			Bill Martin
/// \date_created	11 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CPhraseBox class. 
/// The CPhraseBox class governs the behavior of the phrase or
/// target box where the user enters and/or edits translations while adapting text.
/// \derivation		The PhraseBox class derives from the wxTextCtrl class.
/////////////////////////////////////////////////////////////////////////////

#ifndef PhraseBox_h
#define PhraseBox_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PhraseBox.h"
#endif

#include "SourcePhrase.h" // needed for 

// forward declarations
class CAdapt_ItView;
class CPile;
class CTargetUnit;
class CKB;

/// The CPhraseBox class governs the behavior of the phrase or
/// target box where the user enters and/or edits translations while adapting text.
/// \derivation		The PhraseBox class derives from the wxTextCtrl class.
class CPhraseBox : public wxTextCtrl
{
public:
	CPhraseBox(void); // wx needs the explicit constructor here
	CPhraseBox(wxWindow *parent, wxWindowID id, const wxString &value,
				const wxPoint &pos, const wxSize &size, int style = 0)
				: wxTextCtrl(parent, id, value, pos, size, style)
	{
	}
	virtual ~CPhraseBox(void);

// Attributes
public:
	wxColour	m_textColor;
	CPile*		m_pActivePile;
	bool		m_bAbandonable;
	wxString	m_backspaceUndoStr;
	bool		m_bMergeWasDone; // whm moved here from within OnChar()

protected:
	bool MoveToNextPile(CAdapt_ItView* pView, CPile* pCurPile);
	bool MoveToPrevPile(CAdapt_ItView* pView, CPile* pCurPile);
	bool MoveToImmedNextPile(CAdapt_ItView* pView, CPile* pCurPile);
	bool IsActiveLocWithinSelection(const CAdapt_ItView* WXUNUSED(pView), const CPile* pActivePile);
	void FixBox(CAdapt_ItView* pView, wxString& thePhrase, bool bWasMadeDirty, wxSize& textExtent,
					int nSelector);
	void JumpForward(CAdapt_ItView* pView);

public:
	void DoCancelAndSelect(CAdapt_ItView* pView, CPile* pPile);
	bool LookAhead(CAdapt_ItView* pAppView, CPile* pNewPile);
	bool FindMatchInKB(CKB* pKB, int numWords, wxString srcPhrase, CTargetUnit*& pTargetUnit);
	int	 BuildPhrases(wxString phrases[10],int nActiveSequNum, SPList* pSourcePhrases);
	bool OnePass(CAdapt_ItView *pView);
	bool ChooseTranslation(bool bHideCancelAndSelectButton = FALSE);
	bool LookUpSrcWord(CAdapt_ItView* pAppView, CPile* pNewPile);
	//SPList::Node* GetSrcPhrasePos(int nSequNum, SPList* pSourcePhrases);
	void SetModify(bool modify);
	bool GetModify();

	// Generated message map functions
protected:
	void OnChar(wxKeyEvent& event);
	void OnSysKeyUp(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnLButtonDown(wxMouseEvent& event);
	void OnLButtonUp(wxMouseEvent& event);

public:
	void OnEditUndo(wxCommandEvent& WXUNUSED(event));
	void OnPhraseBoxChanged(wxCommandEvent& WXUNUSED(event));

private:

	DECLARE_DYNAMIC_CLASS(CPhraseBox)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to 
	// declare that the objects of this class should be dynamically 
	// creatable from run-time type information. 
	// MFC uses DECLARE_DYNCREATE(CPhraseBox)

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PhraseBox_h */
