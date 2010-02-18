/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTrans.h
/// \author			Erik Brommers
/// \date_created	10 Februuary 2010
/// \date_revised	10 Februuary 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the CNotes class. 
/// The CNotes class contains the notes-related methods and event handlers
/// that were in the CAdapt_ItView class.
/// \derivation		The CNotes class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef NOTES_H
#define NOTES_H

#ifdef	_NOTES

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "FreeTrans.h"
#endif

//////////////////////////////////////////////////////////////////////////////////
/// The CNotes class presents free translation fields to the user. 
/// The functionality in the CNotes class was originally contained in
/// the CAdapt_ItView class.
/// \derivation		The CNotes class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////
class CNotes : public wxEvtHandler
	{
		friend class CLayout;
	public:
		
		CNotes(); // default constructor
		CNotes(CAdapt_ItApp* app); // use this one
		
		virtual ~CNotes();	// destructor
		
		// Utility functions
		CLayout* CNotes::GetLayout();
		CAdapt_ItView*	CNotes::GetView();

		// Items from Adapt_ItView
		bool		CreateNoteAtLocation(SPList* pSrcPhrases, int nLocationSN, wxString& strNote);
		void		CheckAndFixNoteFlagInSpans(SPList* pSrcPhrases, EditRecord* pRec);
		void		DeleteAllNotes();
		bool		DoesTheRestMatch(WordList* pSearchList, wxString& firstWord, wxString& noteStr,
									 int& nStartOffset, int& nEndOffset);
		bool		FindNote(SPList* pList, int nStartLoc, int& nFoundAt, bool bFindForwards = TRUE); // BEW added 29May08
		int			FindNoteSubstring(int nCurrentlyOpenNote_SequNum, WordList*& pStrList, int numWords,
									  int& nStartOffset, int& nEndOffset);
		bool		GetMovedNotesSpan(SPList* pSrcPhrases, EditRecord* pRec, WhichContextEnum context); // BEW added 14Jun08
		bool		IsNoteStoredHere(SPList* pSrcPhrases, int nNoteSN);
		void		JumpBackwardToNote_CoreCode(int nJumpOffSequNum);
		void		JumpForwardToNote_CoreCode(int nJumpOffSequNum);
		void		MoveNote(CSourcePhrase* pFromSrcPhrase,CSourcePhrase* pToSrcPhrase);
		void		MoveToAndOpenFirstNote();
		void		MoveToAndOpenLastNote();
		bool		MoveNoteLocationsLeftwardsOnce(wxArrayInt* pLocationsList, int nLeftBoundSN);
		bool		RestoreNotesAfterSourceTextEdit(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 26May08
		bool		ShiftANoteRightwardsOnce(SPList* pSrcPhrases, int nNoteSN);
		bool		ShiftASeriesOfConsecutiveNotesRightwardsOnce(SPList* pSrcPhrases, int nFirstNoteSN);
		
	public:
		// Items from Adapt_ItView
		// (edb 17 Feb 2010) BUGBUG: these were protected in Adapt_ItView; does it make sense to friend
		// the view class?
		void OnButtonCreateNote(wxCommandEvent& WXUNUSED(event));
		void OnUpdateButtonCreateNote(wxUpdateUIEvent& event);
		void OnUpdateButtonPrevNote(wxUpdateUIEvent& event);
		void OnUpdateButtonNextNote(wxUpdateUIEvent& event);
		void OnButtonDeleteAllNotes(wxCommandEvent& WXUNUSED(event));
		void OnUpdateButtonDeleteAllNotes(wxUpdateUIEvent& event);
		void OnButtonNextNote(wxCommandEvent& WXUNUSED(event));
		void OnButtonPrevNote(wxCommandEvent& WXUNUSED(event));
		void OnEditMoveNoteForward(wxCommandEvent& WXUNUSED(event));
		void OnUpdateEditMoveNoteForward(wxUpdateUIEvent& event);
		void OnEditMoveNoteBackward(wxCommandEvent& WXUNUSED(event));
		void OnUpdateEditMoveNoteBackward(wxUpdateUIEvent& event);
		
	private:
		CAdapt_ItApp*	m_pApp;	// The app owns this
		
		CLayout*		m_pLayout;
		CAdapt_ItView*	m_pView;
				
	};



#endif // _NOTES

#endif // NOTES_H