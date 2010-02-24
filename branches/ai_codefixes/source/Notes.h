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
		CAdapt_ItApp* CNotes::GetApp();

		// Items from Adapt_ItView
		bool		CreateNoteAtLocation(SPList* pSrcPhrases, int nLocationSN, wxString& strNote); // done
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
		// private body functions for the On...() update handlers - these need to be called
		// on the actual instance of CNotes which the app maintains in its m_pNotes member
		void UpdateButtonCreateNote(wxUpdateUIEvent& event, CAdapt_ItApp* pApp);
		void UpdateButtonPrevNote(wxUpdateUIEvent& event, CAdapt_ItApp* pApp);
		void UpdateButtonNextNote(wxUpdateUIEvent& event, CAdapt_ItApp* pApp);
		void UpdateButtonDeleteAllNotes(wxUpdateUIEvent& event, CAdapt_ItApp* pApp);
		void UpdateEditMoveNoteForward(wxUpdateUIEvent& event, CAdapt_ItApp* pApp);
		void UpdateEditMoveNoteBackward(wxUpdateUIEvent& event, CAdapt_ItApp* pApp);

		void ButtonCreateNote(CAdapt_ItApp* pApp);
		void ButtonDeleteAllNotes(CAdapt_ItApp* pApp);
		void ButtonNextNote(CAdapt_ItApp* pApp);
		void ButtonPrevNote(CAdapt_ItApp* pApp);
		void EditMoveNoteForward(CAdapt_ItApp* pApp);
		void EditMoveNoteBackward(CAdapt_ItApp* pApp);

		// private body functions for the other functions moved from the view. These too
		// need to be called from the instance of CNotes which has been instantiated, and
		// if that is not done, any calls within them to the utility functions or directly
		// to m_pApp, m_pView, m_pLayout will give bogus pointers to unallocated memory.
		// These can be named by the same name but with prefix Private added
		bool PrivateCreateNoteAtLocation(SPList* pSrcPhrases, int nLocationSN, wxString& strNote);
		void PrivateCheckAndFixNoteFlagInSpans(SPList* pSrcPhrases, EditRecord* pRec);	


	private:
		CAdapt_ItApp*	m_pApp;	// The app owns this
		
		CLayout*		m_pLayout;
		CAdapt_ItView*	m_pView;
				
	};



#endif // _NOTES

#endif // NOTES_H