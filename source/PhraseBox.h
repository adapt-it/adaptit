/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PhraseBox.h
/// \author			Bill Martin
/// \date_created	11 February 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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
class CAdapt_ItApp;
class CAdapt_ItDoc;
class CAdapt_ItView;
class CPile;
class CTargetUnit;
class CKB;
class CLayout;

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
		m_textColor = wxColour(0,0,0); // default to black
		m_bMergeWasDone = FALSE;
		m_bCancelAndSelectButtonPressed = FALSE;
	}
	virtual ~CPhraseBox(void);

// Attributes
public:
	wxColour	m_textColor;
	//CPile*		m_pActivePile; // refactored BEW 23Mar09, removed this copy, view's one is enough
	bool		m_bAbandonable;
	wxString	m_backspaceUndoStr;
	bool		m_bMergeWasDone; // whm moved here from within OnChar()

protected:
	bool CheckPhraseBoxDoesNotLandWithinRetranslation(CAdapt_ItView* pView, CPile* pNextEmptyPile, 
							CPile* pCurPile); // BEW added 24Mar09, to simplify MoveToNextPile()
	void DealWithUnsuccessfulStore(CAdapt_ItApp* pApp, CAdapt_ItView* pView, CPile* pNextEmptyPile);
							// BEW added DealWithUnsuccessfulStore() 24Mar09, to simplify MoveToNextPile()
	//bool DoStore_NormalOrTransliterateModes(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc, CAdapt_ItView* pView,
	//						CPile* pCurPile, CPile* pNextEmptyPile, bool bIsTransliterateMode = FALSE);
	bool DoStore_NormalOrTransliterateModes(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc, CAdapt_ItView* pView,
							CPile* pCurPile, bool bIsTransliterateMode = FALSE);
							// BEW added DoStore_NormalOrTransliterateModes() 24Mar09, to simplify MoveToNextPile()
	void HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
							CPile* pNewPile, bool m_bCancelAndSelect, bool& bWantSelect); 
							// BEW added 24Mar09, to simplify MoveToNextPile()
	void HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
							CPile* pNewPile, bool m_bCancelAndSelect, bool& bWantSelect); 
							// BEW added 24Mar09, to simplify MoveToNextPile()
	void MakeCopyOrSetNothing(CAdapt_ItApp* pApp, CAdapt_ItView* pView, CPile* pNewPile, bool& bWantSelect);
							// BEW added MakeCopyOrSetNothing() 24Mar09,  to simplify MoveToNextPile()
	bool MoveToNextPile(CPile* pCurPile);
	bool MoveToNextPile_InTransliterationMode(CPile* pCurPile); // BEW added 24Mar09
							// to simplify the syntax for MoveToNextPile()
	bool MoveToPrevPile(CPile* pCurPile);
	bool MoveToImmedNextPile(CPile* pCurPile);
	bool IsActiveLocWithinSelection(const CAdapt_ItView* WXUNUSED(pView), const CPile* pActivePile);
	void JumpForward(CAdapt_ItView* pView);

public:
	void DoCancelAndSelect(CAdapt_ItView* pView, CPile* pPile);
	bool DoStore_ForPlacePhraseBox(CAdapt_ItApp* pApp, wxString& targetPhrase);	// added 3Apr09
	CLayout* GetLayout();
	void FixBox(CAdapt_ItView* pView, wxString& thePhrase, bool bWasMadeDirty, wxSize& textExtent,
							int nSelector); // BEW made public on 14Mar11, now called in view's OnDraw()
	bool LookAhead(CPile* pNewPile);
	int	 BuildPhrases(wxString phrases[10],int nActiveSequNum, SPList* pSourcePhrases);
	bool OnePass(CAdapt_ItView *pView);
	bool ChooseTranslation(bool bHideCancelAndSelectButton = FALSE);
	bool LookUpSrcWord(CPile* pNewPile);
	//SPList::Node* GetSrcPhrasePos(int nSequNum, SPList* pSourcePhrases);
	void SetModify(bool modify);
	bool GetModify();
	bool GetCancelAndSelectFlag(); // accessor for private bool m_bCancelAndSelectButtonPressed getting
	void ChangeCancelAndSelectFlag(bool bValue); // accessor to change private bool m_bCancelAndSelectButtonPressed
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
    // BEW added 26Mar10, for doc version 5, ChooseTranslation() and DoCancelAndSelect()
    // use this, the former sets it if the relevant button is pressed, the latter uses it
    // snf then clears it
	bool m_bCancelAndSelectButtonPressed;

	DECLARE_DYNAMIC_CLASS(CPhraseBox)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to 
	// declare that the objects of this class should be dynamically 
	// creatable from run-time type information. 
	// MFC uses DECLARE_DYNCREATE(CPhraseBox)

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PhraseBox_h */
