/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Placeholder.h
/// \author			Erik Brommers
/// \date_created	02 April 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CPlaceholder class. 
/// The CPlaceholder class contains methods for working with placeholder 
/// elements within the translated text.
/// \derivation		The CPlaceholder class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#ifndef PLACEHOLDER_H
#define PLACEHOLDER_H

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Placeholder.h"
#endif
/*
// The following CPlaceholderInsertDlg class was not needed after making the
// insertion of placeholders directional to left/right of selection/phrasebox
// a helper class for CPlaceholder
//////////////////////////////////////////////////////////////////////////////////
/// The CPlaceholderInsertDlg class is a helper class for CPlaceholder. It defines
/// a small dialog that may interact with the user to determine whether a placeholder
/// should be inserted BEFORE text that follows, or AFTER text that precedes.
/// \derivation		The CPlaceholderInsertDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////
class CPlaceholderInsertDlg : public AIModalDialog
{
public:
    CPlaceholderInsertDlg(wxWindow* parent); // constructor
    virtual ~CPlaceholderInsertDlg(void); // destructor

    void OnButtonYes(wxCommandEvent& event);
    void OnButtonNo(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyDownChar(wxKeyEvent& event);
    wxButton* pYesBtn;
    wxButton* pNoBtn;
    CAdapt_ItApp* pApp;
protected:
    void InitDialog(wxInitDialogEvent& WXUNUSED(event));
    void OnOK(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnButtonCancel(wxCommandEvent& WXUNUSED(event));

private:
    DECLARE_EVENT_TABLE()
};
*/
//////////////////////////////////////////////////////////////////////////////////
/// The CPlaceholder class contains methods for working with placeholder 
/// elements within the translated text.
/// \derivation		The CPlaceholder class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////
class CPlaceholder : public wxEvtHandler
	{
	public:
		
		CPlaceholder(); // default constructor
		CPlaceholder(CAdapt_ItApp* app); // use this one
		
		virtual ~CPlaceholder();	// destructor
		
		// methods
		void		InsertNullSrcPhraseBefore();
		void		InsertNullSrcPhraseAfter();
		void		InsertNullSourcePhrase(
							CAdapt_ItDoc*		pDoc,
							CPile*				pInsertLocPile,
							const int			nCount,
							bool				bRestoreTargetBox = TRUE,
							bool				bForRetranslation = FALSE,
							bool bInsertBefore = TRUE);
		void		RemoveNullSourcePhrase(
							CPile*				pInsertLocPile, 
							const				int nCount);
		void		RemoveNullSrcPhraseFromLists(
							SPList*&			pList,
							SPList*&			pSrcPhrases,
							int&				nCount,
							int&				nEndSequNum,
							bool				bActiveLocAfterSelection,
							int&				nSaveActiveSequNum);
		bool		RemovePlaceholdersFromSublist(SPList*& pSublist);
		bool		NeedsTransferBackwards(CSourcePhrase* pPlaceholderSrcPhrase);
		bool		NeedsTransferForwards(CSourcePhrase* pPlaceholderSrcPhrase);
		bool		IsPlaceholderInSublist(SPList* pSublist);
		CSourcePhrase* CreateBasicPlaceholder(); // creates a new placeholder on the
							// heap, and sets the expected flags - the caller then
							// needs to fill out other members relevant to the situation

		// BEW added 11Oct10 for better support of docV5 within OnButtonRetranslation()
		void		UntransferTransferredMarkersAndPuncts(
							SPList*				pSrcPhraseList,
							CSourcePhrase*		pSrcPhrase);
		void OnButtonNullSrc(wxCommandEvent& event); // whm 1Jul2018 moved to public whm 20Mar2020 removed WXUNUSED
		void OnButtonRemoveNullSrcPhrase(wxCommandEvent& WXUNUSED(event)); // whm 1Jul2018 moved to public
	private:

		// a utility for setting or clearing the bFollowingMarkers boolean (although
		// strictly speaking more than markers are involved, it's really about deciding if
		// the starting location for something should be tranferred to a right-associated
		// placeholder)
		bool IsRightAssociationTransferPossible(CSourcePhrase* pSrcPhrase);

	public:
	// ****************** Refactoring for the two-button placeholder insertions, March2020
#if defined (_PHRefactor)

	void DoInsertPlaceholder(CAdapt_ItDoc* pDoc, // needed here & there
		CPile* pInsertLocPile,	// ptr to the pSrcPhrase which is the current active location, before
								// or after which the placeholder is to be inserted
		const int nCount,		// how many placeholders to sequentially insert
		bool bRestoreTargetBox,	// TRUE if restoration wanted, FALSE if not; no default
		bool bForRetranslation,	// TRUE if in a Retranslation, 1 or more may need appending, no default
		bool bInsertBefore,		// TRUE to insert before pInsertLocPile, FALSE to insert after pInsertLocPile, no default
		bool bAssociateLeftwards // TRUE to associate with left located text, FALSE to associate with right
								// located text, no default. However, if bInsertBefore is TRUE we will
								// hard code bAssocateLeftwards to FALSE. If bInsertBefore is FALSE we will
								// hard code bAssociateLeftwards to TRUE. The parameter is included in
								// the signature in case sometime we with to vary the behaviour.
		);

	void OnButtonNullSrcLeft(wxCommandEvent& event);  // handler when the left insert button 
																// is clicked on the command bar
	void OnButtonNullSrcRight(wxCommandEvent& event); // handler when the right insert button 
																// is clicked on the command bar






#endif
	// ***************** End refactoring for the two-button placeholder insertions

	protected:
		// event handlers
		void OnUpdateButtonRemoveNullSrcPhrase(wxUpdateUIEvent& event);
		void OnUpdateButtonNullSrc(wxUpdateUIEvent& event);
		
	private:
		CAdapt_ItApp*	m_pApp;	// The app owns this
		CLayout*		m_pLayout;
		CAdapt_ItView*	m_pView;
        bool m_bDummyAddedTemporarily;
		
		DECLARE_EVENT_TABLE()
	};

#endif // _PLACEHOLDER_H