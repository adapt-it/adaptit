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

    // whm 11Jul18 this constructor only creates the edit box part of the phrasebox, see App's DoCreatePhraseBox()
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
    bool        m_bCompletedMergeAndMove;
    long        m_nSaveStart; //int m_nSaveStart; // these two are for implementing Undo() for a backspace operation
    long        m_nSaveEnd; //int m_nSaveEnd;                 
    int         m_nCurrentSequNum; /// Contains the current sequence number of the active pile (m_nActiveSequNum) for use by auto-saving.
    int         m_nWordsInPhrase;
    wxString	m_CurKey;
    wxString    m_Translation;
    bool        m_bEmptyAdaptationChosen;

    wxString    m_SaveTargetPhrase;

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
	int			 nDeletedItem_refCount;
	// BEW 5Apr22 retaining the two extra members I add, RemoveRefString() needs them
	wxString     strPreDeletedValue; // BEW added 30Mar22, need to get the pRefString->m_translation value
						// cached, for input to CKB::HandlePseudoDeletion(wxString src, wxString nonSrc)
	wxString     strPreDeletedKey; // a copy of the active pile's pSrcPhrase's m_key value,
						// need for input to CKB::HandlePseudoDeletion(wxString src, wxString nonSrc)

	void InitializeComboLandingParams(); // initialize the above member variables, I'll decline 
										 // using the m_ prefix in their names, as these are very hacky
    wxSize  m_computedPhraseBoxSize; // stores the computed size of the phrasebox's sizer - accounting for its current layout state

    // Some PhraseBox Getters
    wxTextCtrl* GetTextCtrl(); // this gets the wxTextCtrl that was created by the App's DoCreatePhraseBox() function.
    CMyListBox* GetDropDownList(); // this gets the CMyListBox (wxListBox) that was created by the App's CreatePhraseBox() function.
    wxBitmapToggleButton* GetPhraseBoxButton(); // this gets the wxBitmapToggleButton control that was created by the App's CreatePhraseBox() function.

	// Some PhraseBox Setters
    void SetTextCtrl(wxTextCtrl* textCtrl);
    void SetDropDownList(CMyListBox* listBox);
    void SetPhraseBoxButton(wxBitmapToggleButton* listButton);
    // whm 12Jul2018 Note: The handler for the PhraseBox dropdown button is
    // now in CAdapt_ItCanvas::OnTogglePhraseBoxButton()
    void SetButtonBitMapNormal();
    void SetButtonBitMapXDisabled();
    void SetFocusAndSetSelectionAtLanding();

protected:
	bool CheckPhraseBoxDoesNotLandWithinRetranslation(CAdapt_ItView* pView, CPile* pNextEmptyPile,
							CPile* pCurPile); // BEW added 24Mar09, to simplify MoveToNextPile()
	void DealWithUnsuccessfulStore(CAdapt_ItApp* pApp, CAdapt_ItView* pView, CPile* pNextEmptyPile);
	// BEW added DealWithUnsuccessfulStore() 24Mar09, to simplify MoveToNextPile()
	bool DoStore_NormalOrTransliterateModes(CAdapt_ItApp* pApp, CAdapt_ItDoc* pDoc, CAdapt_ItView* pView,
							CPile* pCurPile, bool bIsTransliterateMode = FALSE);
	// BEW added DoStore_NormalOrTransliterateModes() 24Mar09, to simplify MoveToNextPile()
    // whm 22Feb2018 removed bool m_bCancelAndSelect parameter and logic
    void HandleUnsuccessfulLookup_InAutoAdaptMode_AsBestWeCan(CAdapt_ItApp* pApp, CAdapt_ItView* pView,
							CPile* pNewPile, bool& bWantSelect);
	// BEW added 24Mar09, to simplify MoveToNextPile()
    // whm 22Feb2018 removed bool m_bCancelAndSelect parameter and logic
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
public:
	void JumpForward(CAdapt_ItView* pView);

	// BEW 31Oct22 public: was here
	bool DoStore_ForPlacePhraseBox(CAdapt_ItApp* pApp, wxString& targetPhrase);	// added 3Apr09
	CLayout* GetLayout();

	// whm 11Nov2022 added this convenience function for phrasebox width sizing. 
	// Only the .x coord text extent is returned of the str input string, based on
	// the main frame's canvas. The width extent of the App's m_pNavTextFont is returned
	// if gbIsGlossing && gbGlossingUsesNavFont, otherwise the width extent of the App's
	// m_pTargetFont is returned.
	int GetTextExtentWidth(wxString str);

	int boxContentPixelExtentAtLanding;

	// whm 11Nov2022 added the following oldPhraseBoxWidthCached and 
	// oldBoxContentPixelExtentCached values for caching old phrasebox width 
	// and old phrasebox content widths.
	// These values are set just before the OnPhraseBoxChanged() function returns. 
	// It is set at that time and used to detect the spaceRemainingInBoxAtThisEdit 
	// value within the GetBestTabSettingFromArrayForBoxResize() function, which 
	// is called earlier in OnPhraseBoxChanged(). 
	// This oldPhraseBoxWidthCached value is used in determining the need for 
	// contracting the size of the phrasebox after an edit occurs that causes an 
	// such an increase in whitespace at the end of the phrasebox contents, that a 
	// phrasebox contraction should occur (if within constraints).
	int oldPhraseBoxWidthCached; 
	int oldBoxContentPixelExtentCached;

	// whm 11Nov2022 added the following array of integer values, to keep track of the
	// potential tab positions for expanding and/or contracting the phrasebox. The int
	// values in the array represent potential pixel positions that function as tab stops
	// along any potential string content within the phrasebox. The pixel tab positions
	// within the int array are pre-set to pixel positions that are multiples of the
	// "slop distance" away from (left and right of) the initial starting at the end of 
	// the phrasebox string content at phrasebox landing.
	// The first arrayTabPositionInPixels item is normally 0 regardless of the length of
	// the string content of the phrasebox at landing and regardless of the slop value that
	// is determined from the View page's setting for the slop.
	// If the phrasebox has an empty content at landing, all the tab stops within the array
	// will be multiples of the slop pixel value, starting at 0, each succeeding value being
	// a positive int of the slop above the previous tab stop in the array.
	// If the phrasebox has initial string content at landing, the insertion point is at
	// the end of the string. We make that insertion point's position the reference point
	// for tab positions both preceeding that point and following that point. So, the 
	// second element of the array is likely to be a value which is less that a slop 
	// distance in pixels from the 0 first element, in particular when the extent of the
	// initial content string is not exactly a slop multiple above 0. Third and all 
	// remaining array items will be exactly a slop multiple apart in their pixel values.
	// 
	// The arrayTabPositionsInPixels is cleared/emptied, and its int values are set at
	// phrasebox landing within the CLayout::PlaceBox() function. The values for tab 
	// positions stay constant within the array while the phrasebox remains at the same
	// location established at the PlaceBox() call.
	// Hence, whenever the user mades an edit that changes the pixel length of the content
	// within the phrasebox resulting in that pixel length crossing a pre-set pixel tab 
	// position, a resize of the phrasebox will be triggered within the 
	// CPhraseBox::OnPhraseBoxChanged() function. A single edit by the user, such as a
	// paste of a lengthy portion of text, or the selection and deletion of a significant
	// portion of text can result in the phrasebox text's pixel extent crossing more than
	// one pixel tab stop for that single edit action. In that case the phrasebox resize 
	// may make the resulting phrasebox size grow or shrink by more than one pixel tab 
	// (slop) amount. For normal typing and backspace key presses, the OnPhraseBoxChanged()
	// function is called at each key press, and only phrasebox size changes would be 
	// triggered in those common editing scenarios at the point that the text extent of
	// the resulting string content within the phrasebox crosses a pixel tab setting
	// in the in array arrayTabPositionsInPixels.
	wxArrayInt arrayTabPositionsInPixels;
	int GetBestTabSettingFromArrayForBoxResize(int initialPixelTabPosition, int boxContentPixelExtentAtThisEdit);

    // whm 10Jan2018 added members below to implement the dropdown phrasebox functionality
    void SetupDropDownPhraseBoxForThisLocation();
	void PopulateDropDownList(CTargetUnit* pTU, int& selectionIndex, int& indexOfNoAdaptation);
	//void RepopulateDropDownList(CTargetUnit* pTU, int& selectionIndex, int& indexOfNoAdaptation); // BEW removed 5Apr22
	bool RestoreDeletedRefCount_1_ItemToDropDown();

	bool IsPhraseBoxVisibleInClientWindow();

    void ClearDropDownList();
    void CloseDropDown();
    void PopupDropDownList();
    void HidePhraseBox();
    void SetSizeAndHeightOfDropDownList(int width);
    wxString GetListItemAdjustedforPhraseBox(bool bSetEmptyAdaptationChosen);
    bool bUp_DownArrowKeyPressed; // initialized to FALSE at each location - at end of Layout's PlaceBox().

	// whm 16Jul2018 added to implement undo of phrasebox changes via Esc key
	// whm 11Nov2022 expanded use of the following initialPhraseBoxContentsOnLanding variable
	// to be the baseline for phrasebox content in refactored phrasebox resizing code within
	// the OnPhraseBoxChanged() function.
    wxString initialPhraseBoxContentsOnLanding; 
	// The following are tab positions in pixels for controlling phrasebox expansion and contraction.
	// The following value is the phrasebox's edit box width in pixels at PlaceBox() landing
	int initialPixelTabPositionOnLanding;
	// The following is the phrasebox's content extent after the current edit
	int boxContentPixelExtentAtThisEdit;
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
	// BEW added 23Apr15
	void ChangeValue(const wxString& value); // will replace all ZWSP with / if app->m_bFwdSlashDelimiter is TRUE

	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
    void OnSysKeyUp(wxKeyEvent& event); // whm 3Jul2018 moved to public 
	void OnLButtonUp(wxMouseEvent& event); // whm 2Jun2018 moved to public access for use in App's FilterEvent()
	void OnLButtonDown(wxMouseEvent& event); // whm 2Jun2018 moved to public access for use in App's FilterEvent()
	// whm 11Nov2022 made the following OnEditUndo a normal function; it no longer has an EVT_MENU entry
	// in the CPhraseBox's event table. It is only called by the CAdapt_ItView::OnEditUndo() menu event handler.
	void OnEditUndo();
	void OnPhraseBoxChanged(wxCommandEvent& WXUNUSED(event));

    // whm 12Apr2019 The events for the handlers below are actually caught in
    // CAdapt_ItCanvas and are now handled there.
    //void OnTogglePhraseBoxButton(wxCommandEvent& event);
    //void OnListBoxItemSelected(wxCommandEvent& event);

	int indexOfNoAdaptation;

private:
    // The private pointers of the components that make up the PhraseBox:
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

