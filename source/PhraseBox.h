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
/// BEW 23Apr15 Beware, support for / as a whitespace delimiter for word breaking was
/// added as a user-chooseable option. When enabled, there is conversion to and from
/// ZWSP and / to the opposite character. Done in an override of ChangeValue() 
/////////////////////////////////////////////////////////////////////////////

#ifndef PhraseBox_h
#define PhraseBox_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PhraseBox.h"
#endif

//#include <wx/combobox.h>
//#include <wx/combo.h>
#include <wx/odcombo.h>

#include "SourcePhrase.h" // needed for

// forward declarations
class CAdapt_ItApp;
class CAdapt_ItDoc;
class CAdapt_ItView;
class CPile;
class CTargetUnit;
class CKB;
class CLayout;
class CMyListBox;

/// The CPhraseBox class governs the behavior of the phrase or
/// target box where the user enters and/or edits translations while adapting text.
/// \derivation		The PhraseBox class derives from the wxTextCtrl class.
class CPhraseBox : public wxTextCtrl
{
public:
	CPhraseBox(void); // wx needs the explicit constructor here

    // whm 11Jul18 this constructor uses the wxDesigner resource PhraseBoxDropDownFunc()
    CPhraseBox(
        wxWindow *parent, 
        wxWindowID id, 
        const wxString &value,
        const wxPoint &pos, 
        const wxSize &size, 
        int style = 0);

	virtual ~CPhraseBox(void);

// Attributes
public:
	wxColour	m_textColor;
	bool		m_bAbandonable;
	wxString	m_backspaceUndoStr;
	bool		m_bCurrentCopySrcPunctuationFlag; // BEW added 20May16 (preserve m_bCopySourcePunctuation
					// value so it can be restored within DoStore_ForPlacePhraseBox() )
    bool        m_bRetainBoxContents;
    bool        m_bBoxTextByCopyOnly;
    bool        m_bTunnellingOut;
    bool        m_bSavedTargetStringWithPunctInReviewingMode;
    wxString    m_StrSavedTargetStringWithPunctInReviewingMode;
    bool        m_bNoAdaptationRemovalRequested;
    bool        m_bCameToEnd;
    bool        m_bTemporarilySuspendAltBKSP;
    bool        m_bSuppressStoreForAltBackspaceKeypress;
    bool        m_bSuppressMergeInMoveToNextPile; 
    //bool        m_bMovingToPreviousPile; // whm 24Feb2018 initialized but unused so removed 
    bool        m_bCompletedMergeAndMove;
    long        m_nSaveStart; //int m_nSaveStart; // these two are for implementing Undo() for a backspace operation
    long        m_nSaveEnd; //int m_nSaveEnd;                 
    int         m_nCurrentSequNum; /// Contains the current sequence number of the active pile (m_nActiveSequNum) for use by auto-saving.
    int         m_nWordsInPhrase;
    wxString	m_CurKey;
    wxString    m_Translation;
    bool        m_bEmptyAdaptationChosen;


    wxString    m_SaveTargetPhrase;
    //CTargetUnit* pTargetUnitFromChooseTrans; // whm 24Feb2018 moved to the App

	// BEW 7May18 Added members for saving the to-be-removed CRefString, it's owning pTU (pointer to
	// CTargetUnit, and the list index at which the to-be-removed CRefString currently lives (before
	// the removal takes place due to the phrasebox landing at that location). Setting these values
	// will be done from places within the KB lookup function. The app will use these variables for
	// keeping what's saved here for display in the combobox dropdown list, while from the KB the
	// relevant CRefString has been removed. PlaceBox() is called at the end of PlacePhraseBox()
	// and so we will have the CRefString reinstated before PlaceBox()'s list gets dropped down
	int          nSaveComboBoxListIndex;
	wxString	 strSaveListEntry;
	bool		 bRemovedAdaptionReadyForInserting; // into the combo box's dropdown list - at its former location
	void InitializeComboLandingParams(); // initialize the above member variables, I'll decline 
										 // using the m_ prefix in their names, as these are very hacky
    wxSize  m_computedPhraseBoxSize; // stores the computed size of the phrasebox's sizer - accounting for its current layout state

    // Some PhraseBox Getters
    wxTextCtrl* GetTextCtrl(); // this gets the wxTextCtrl that has been created by the PhraseBoxDropDownFunc() in wxDesigner
    CMyListBox* GetDropDownList(); // this gets the wxListBox that has been created by the PhraseBoxDropDownFunc() in wxDesigner
    wxBitmapToggleButton* GetPhraseBoxButton(); // this gets the wxButton control that has been created by the PhraseBoxDropDownFunc() in wxDesigner
    // Some PhraseBox Setters
    void SetTextCtrl(wxTextCtrl* textCtrl);
    void SetDropDownList(CMyListBox* listBox);
    void SetPhraseBoxButton(wxBitmapToggleButton* listButton);
    // whm 12Jul2018 Note: The handler for the PhraseBox dropdown button is
    // now in CAdapt_ItCanvas::OnTogglePhraseBoxButton()
    void SetButtonBitMapNormal();
    void SetButtonBitMapXDisabled();

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
    // whm 22Feb2018 removed bool m_bCancelAndSelect parameter and logic
    //void HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
    //                      CPile* pNewPile, bool m_bCancelAndSelect, bool& bWantSelect);
    void HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
							CPile* pNewPile, bool& bWantSelect);
	// BEW added 24Mar09, to simplify MoveToNextPile()
    // whm 22Feb2018 removed bool m_bCancelAndSelect parameter and logic
    //void HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
    //                      CPile* pNewPile, bool m_bCancelAndSelect, bool& bWantSelect);
	void HandleUnsuccessfulLookup_InSingleStepMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
							CPile* pNewPile, bool& bWantSelect);
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
	bool DoStore_ForPlacePhraseBox(CAdapt_ItApp* pApp, wxString& targetPhrase);	// added 3Apr09
	CLayout* GetLayout();

	bool UpdatePhraseBoxWidth_Expanding(wxString inStr); // BEW addedd 30Jul18 the returned bool 
				// sets CMainFrame::m_bUpdatePhraseBoxWidth to TRUE, which OnIdle() then traps, 
				// to call a handler to effect the width update; or if no update is needed, 
				// returns FALSE. Pass in the character just typed as a wxString, or the 
				// string being pasted into the phrasebox control from the clipboard
	bool UpdatePhraseBoxWidth_Contracting(wxString inStr); // BEW addedd 30Jul18 the returned bool 
				// sets CMainFrame::m_bUpdatePhraseBoxWidth to TRUE, which OnIdle() then traps, 
				// to call a handler to effect the width update; or if no update is needed, 
				// returns FALSE. Pass in the character just typed as a wxString, or the 
				// string being pasted into the phrasebox control from the clipboard


	void FixBox(CAdapt_ItView* pView, wxString& thePhrase, bool bWasMadeDirty, wxSize& textExtent,
							int nSelector); // BEW made public on 14Mar11, now called in view's OnDraw()

    // whm 10Jan2018 added members below to implement the dropdown phrasebox functionality
    void SetupDropDownPhraseBoxForThisLocation();
    void PopulateDropDownList(CTargetUnit* pTU, int& selectionIndex, bool& bNoAdaptationFlagPresent, int& indexOfNoAdaptatio);

    void ClearDropDownList();
    void CloseDropDown();
    void PopupDropDownList();
    void HidePhraseBox();
    void SetSizeAndHeightOfDropDownList(int width);
    wxString GetListItemAdjustedforPhraseBox(bool bSetEmptyAdaptationChosen);
    bool bUp_DownArrowKeyPressed; // initialized to FALSE at each location - at end of Layout's PlaceBox().
    wxString initialPhraseBoxContentsOnLanding; // whm 16Jul2018 added to implement undo of phrasebox changes via Esc key

    // The following members are used to present a dropdown arrow or a rose pink X for the control's button:
    //wxBitmap dropbutton_hover; // (xpm_dropbutton_hover);
    //wxBitmap dropbutton_pressed; // (xpm_dropbutton_pressed);
    wxBitmap bmp_dropbutton_normal; // (xpm_dropbutton_normal);
    //wxBitmap dropbutton_disabled; // (xpm_dropbutton_disabled);
    wxBitmap bmp_dropbutton_X; // (xpm_dropbutton_X);
    //char * xpm_dropbutton_hover;
    //char * xpm_dropbutton_pressed;
    char * xpm_dropbutton_normal;
    //char * xpm_dropbutton_disabled;
    char * xpm_dropbutton_X;


	bool LookAhead(CPile* pNewPile);
	int	 BuildPhrases(wxString phrases[10],int nActiveSequNum, SPList* pSourcePhrases);
	bool OnePass(CAdapt_ItView *pView);
	bool LookUpSrcWord(CPile* pNewPile);
	void SetModify(bool modify);
	bool GetModify();

	// Generated message map functions
	void RemoveFinalSpaces(CPhraseBox* pBox,wxString* pStr);
	void RemoveFinalSpaces(wxString& rStr); // overload of the public function, BEW added 30Apr08
	void RestorePhraseBoxAtDocEndSafely(CAdapt_ItApp* pApp, CAdapt_ItView *pView);
//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15
	void ChangeValue(const wxString& value); // will replace all ZWSP with / if app->m_bFwdSlashDelimiter is TRUE
//#endif
protected:
    //wxCoord OnMeasureItem(size_t item) const; // whm 12Jul2018 removed - no longer using wxOwnerDrawnComboBox
public:
	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
    void OnSysKeyUp(wxKeyEvent& event); // whm 3Jul2018 moved to public 
	void OnLButtonUp(wxMouseEvent& event); // whm 2Jun2018 moved to public access for use in App's FilterEvent()
	void OnLButtonDown(wxMouseEvent& event); // whm 2Jun2018 moved to public access for use in App's FilterEvent()
	void OnEditUndo(wxCommandEvent& WXUNUSED(event));
	void OnPhraseBoxChanged(wxCommandEvent& WXUNUSED(event));

    // whm 12Jul2018 The events for the handlers below are actually caught in
    // CAdapt_ItCanvas. The handlers there of the same name simply call these
    // handlers below to do the actual handling of the events. Hence the handlers
    // below do not have a presence in the CPhraseBox event table at the beginning
    // of PhraseBox.cpp.
    void OnTogglePhraseBoxButton(wxCommandEvent& event);
    void OnListBoxItemSelected(wxCommandEvent& event);


private:

    wxTextCtrl* m_pTextCtrl;
    CMyListBox* m_pDropDownList;
    wxBitmapToggleButton* m_pPhraseBoxButton;

	DECLARE_DYNAMIC_CLASS(CPhraseBox)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to
	// declare that the objects of this class should be dynamically
	// creatable from run-time type information.
	// MFC uses DECLARE_DYNCREATE(CPhraseBox)

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PhraseBox_h */

