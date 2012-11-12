/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Notes.h
/// \author			Erik Brommers
/// \date_created	10 Februuary 2010
/// \rcs_id $Id$
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

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Notes.h"
#endif

/// wxList declaration and partial implementation of the WordList class being
/// a list of pointers to wxString objects
WX_DECLARE_LIST(wxString, WordList); // see list definition macro in .cpp file


//////////////////////////////////////////////////////////////////////////////////
/// The CNotes class contains notes-related functionality originally found in
/// the CAdapt_ItView class.
/// \derivation		The CNotes class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////
class CNotes : public wxEvtHandler
	{
		//friend class CLayout; //BEW I don't think we need to make friends with CLayout
	public:
		
		CNotes(); // default constructor
		CNotes(CAdapt_ItApp* app); // use this one
		
		virtual ~CNotes();	// destructor
		

		// Items from Adapt_ItView
		bool	CreateNoteAtLocation(SPList* pSrcPhrases, int nLocationSN, wxString& strNote);
		void	CheckAndFixNoteFlagInSpans(SPList* pSrcPhrases, EditRecord* pRec);
		void	DeleteAllNotes();
		bool	DoesTheRestMatch(WordList* pSearchList, wxString& firstWord, wxString& noteStr,
									 int& nStartOffset, int& nEndOffset);
		int		FindNoteSubstring(int nCurrentlyOpenNote_SequNum, WordList*& pSearchList, int numWords,
									  int& nStartOffset, int& nEndOffset);
		bool	GetMovedNotesSpan(SPList* pSrcPhrases, EditRecord* pRec, WhichContextEnum context); // BEW added 14Jun08
		void	JumpBackwardToNote_CoreCode(int nJumpOffSequNum);
		void	JumpForwardToNote_CoreCode(int nJumpOffSequNum);
		void	MoveNote(CSourcePhrase* pFromSrcPhrase,CSourcePhrase* pToSrcPhrase);
		void	MoveToAndOpenFirstNote();
		void	MoveToAndOpenLastNote();
		bool	RestoreNotesAfterSourceTextEdit(SPList* pSrcPhrases, EditRecord* pRec); // BEW added 26May08
		bool	ShiftANoteRightwardsOnce(SPList* pSrcPhrases, int nNoteSN);
		bool	ShiftASeriesOfConsecutiveNotesRightwardsOnce(SPList* pSrcPhrases, int nFirstNoteSN);

		// the following have no internal calls to functions or values on the app, layout
		// or view class
		bool	FindNote(SPList* pList, int nStartLoc, int& nFoundAt, bool bFindForwards = TRUE); // BEW added 29May08
		bool	IsNoteStoredHere(SPList* pSrcPhrases, int nNoteSN);
		bool	MoveNoteLocationsLeftwardsOnce(wxArrayInt* pLocationsList, int nLeftBoundSN);
		bool	BunchUpUnsqueezedLocationsLeftwardsFromEndByOnePlace(int nStartOfEditSpan, 
									int nEditSpanCount, wxArrayInt* pUnsqueezedArr, 
									wxArrayInt* pSqueezedArr, int WXUNUSED(nRightBound));		
	public:
		// Items from Adapt_ItView
		// (edb 17 Feb 2010) BUGBUG: these were protected in Adapt_ItView; does it make sense to friend
		// the view class? No real need.
		void OnButtonCreateNote(wxCommandEvent& WXUNUSED(event));
		void OnButtonDeleteAllNotes(wxCommandEvent& WXUNUSED(event));
		void OnButtonNextNote(wxCommandEvent& WXUNUSED(event));
		void OnButtonPrevNote(wxCommandEvent& WXUNUSED(event));
		void OnEditMoveNoteForward(wxCommandEvent& WXUNUSED(event));
		void OnEditMoveNoteBackward(wxCommandEvent& WXUNUSED(event));
		// update handlers...
		void OnUpdateButtonCreateNote(wxUpdateUIEvent& event);
		void OnUpdateButtonPrevNote(wxUpdateUIEvent& event);
		void OnUpdateButtonNextNote(wxUpdateUIEvent& event);
		void OnUpdateButtonDeleteAllNotes(wxUpdateUIEvent& event);
		void OnUpdateEditMoveNoteForward(wxUpdateUIEvent& event);
		void OnUpdateEditMoveNoteBackward(wxUpdateUIEvent& event);

	private:
		CAdapt_ItApp*	m_pApp;	// The app owns this
		CLayout*		m_pLayout;
		CAdapt_ItView*	m_pView;

		DECLARE_EVENT_TABLE()
	};

#endif // NOTES_H