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
		// BEW 11Oct10, next is no longer called anywhere, so commented out
		//CSourcePhrase*	ReDoInsertNullSrcPhrase(
		//					SPList*				pList,
		//					SPList::Node*&		insertPos,
		//					bool				bForRetranslation = FALSE);
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
	private:

		// a utility for setting or clearing the bFollowingMarkers boolean (although
		// strictly speaking more than markers are involved, it's really about deciding if
		// the starting location for something should be tranferred to a right-associated
		// placeholder)
		bool IsRightAssociationTransferPossible(CSourcePhrase* pSrcPhrase);

	protected:
		// event handlers
		void OnButtonRemoveNullSrcPhrase(wxCommandEvent& WXUNUSED(event));
		void OnUpdateButtonRemoveNullSrcPhrase(wxUpdateUIEvent& event);
		void OnButtonNullSrc(wxCommandEvent& WXUNUSED(event));
		void OnUpdateButtonNullSrc(wxUpdateUIEvent& event);
		
	private:
		CAdapt_ItApp*	m_pApp;	// The app owns this
		CLayout*		m_pLayout;
		CAdapt_ItView*	m_pView;
		
		DECLARE_EVENT_TABLE()
	};

#endif // _PLACEHOLDER_H