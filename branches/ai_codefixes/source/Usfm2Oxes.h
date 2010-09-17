/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Usfm2Oxes.h
/// \author			Bruce Waters
/// \date_created	2 Sept 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the Usfm2Oxes class. 
/// The USFM2Oxes class takes an in-memory plain text (U)SFM marked up text, chunks it
/// according to criteria relevant to oxes support, and then builds oxes xml from each
/// chunk in the appropriate order of chunks. The chunks are stored on structs. 
/// Initially oxes version 1 is supported, later it will also support version 2 when the
/// latter standard is finalized. (Deuterocannon books are not supported explicitly.)
/// \derivation		The Usfm2Oxes class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#ifndef Usfm2Oxes_h
#define Usfm2Oxes_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Usfm2Oxes.h"
#endif

#include <wx/dynarray.h>

// forward declarations
class wxObject;
class CAdapt_ItApp;
class CBString;

enum CustomMarkers {
	excludeCustomMarkersFromTest = 0,
	includeCustomMarkersInTest,
	ignoreCustomMarkersWhenParsing = 0,
	haltAtCustomMarkersWhenParsing
}; // use this if a function always either deals with both the \free and \note markers
   // as halting markers, or always excludes dealing with both as halting markers, but when
   // one or the other may be wanted to be included in tests, use the structs below instead
   // in the function signatures

// values are 0 or 1, synonyms are used because one value is meaningful when parsing, the
// synonym is more meaningful when making a test where including or excluding the marker
// in the test is what is in focus
enum CustomMarkersFT {
	excludeFreeTransFromTest = 0,
	includeFreeTransInTest,
	ignoreFreeTransWhenParsing = 0,
	haltAtFreeTransWhenParsing
};

// values are 0 or 1, synonyms are used because one value is meaningful when parsing, the
// synonym is more meaningful when making a test where including or excluding the marker
// in the test is what is in focus
enum CustomMarkersN {
	excludeNoteFromTest = 0,
	includeNoteInTest,
	ignoreNoteWhenParsing = 0,
	haltAtNoteWhenParsing
};

struct NoteDetails
{
	// this is enough to make an OXES annotation, for the datetimes, we will use the
	// current datetime at the time the OXES file is created (currently, I'm giving OXES
	// the local date and time -- but if the TE crew want otherwise, this can be changed
	wxString usfmMarker; // store "rem" here, always
	wxString noteText;
	int beginOffset;
	int endOffset;
	wxString wordsInSpan; // either a single word or a phrase -- the USFM export for oxes
						  // purposes sends the needed info wrapped as @#nnn:phrase#@
						  // stored at the start of the note string, it has the number of
						  // characters in the adaptation phrase which we use to get a
						  // correct endOffset value, and the phrase itself, which we
						  // store in its wordsInSpan member
};

WX_DECLARE_OBJARRAY(NoteDetails*,NoteDetailsArray); // store pointers, and manage the heap ourselves


struct aiGroup
{
	// for storing (1) free translation, (2) one or more embedded notes following the free
	// translation, (3) one or more SF markers preceding the text, (4) the text; the
	// information for a single aiGroup is typically deemed complete when the document end
	// is reached or one of the following is true: another free translation occurs,
	// a note which follows the text occurs, a marker which halts parsing is encountered
	// (such as \c \v, \p, \q, \q#, \s, \s# etc)
	wxString freeTransStr;
	NoteDetailsArray arrNoteDetails; // stores NoteDetail struct pointers
	wxArrayString arrMarkers; // for \c and \v, the chapter or verse number is stored separately
	wxString chapterNumber;
	wxString verseNumber;
	wxString verseNumberBridgeStr; // to store things like "1-2a"
	wxString textStr;
	wxString usfmBareMarker; // store the main marker for this group here, backslash removed
	bool bHasInlineMarker; // TRUE when things like x-ref, \wj, \k, \it etc formatting stuff
						   // are within the contents of the textStr member
};

WX_DECLARE_OBJARRAY(aiGroup*,AIGroupArray); // store pointers, and manage the heap ourselves

struct TitleInfo
{
	// currently supporting: id h h1 h2 h3 mt mt1 mt2 mt3, and SFM PNG: st
	// 
	// markers in USFM which could be here but our OXES export code is not yet supporting:
	// rem (paratext remark) ide (encoding specification) sts (status code, 1 = first
	// draft, 2 = team draft, 3 = reviewed draft, 4 = clean text), toc1 (long 'table of
	// contents' text), toc2 (short 'table of contents' text), toc3 (book abbreviation)
	// mte (major title at ending) 
	bool bChunkExists;
	wxString strChunk;
	wxArrayString arrPossibleMarkers;
	AIGroupArray aiGroupArray; // stores aiGroup struct pointers
};

struct IntroductionInfo
{
	// currently supporting: the full range of introduction markers
	bool bChunkExists;
	wxString strChunk;
	wxArrayString arrPossibleMarkers;
	AIGroupArray aiGroupArray; // stores aiGroup struct pointers
};


class Usfm2Oxes : public wxEvtHandler
{

// -----------------CREATORS, DESTRUCTOR, MAIN FUNCTIONS-----------

public:

	Usfm2Oxes(); // default constructor
	Usfm2Oxes(CAdapt_ItApp* app); // use this one

	virtual ~Usfm2Oxes();// destructor

	// DoOxesExport() is the public function called to get the job done
	wxString DoOxesExport(wxString& buff);
	void SetOXESVersionNumber(int versionNum);
	void SetBookID(wxString& aValidBookCode);

private:

// -----------------PRIVATE FUNCTIONS---------------------------

	// setup functions & shutdown functions...
	
	void Initialize();
	void ClearAIGroupArray(AIGroupArray& rGrpArray);
	void ClearTitleInfo();
	void ClearIntroInfo();
	void ClearNoteDetails(NoteDetailsArray& rNoteDetailsArray);
	void SetupOxesTags();

// ---------------------------------------------------------------

	// coding utilities & debugging support...
	
	void DisplayAIGroupStructContents(TitleInfo* pTitleInfo);
	void DisplayAIGroupStructContentsIntro(IntroductionInfo* pIntroInfo);
	// a utility to convert Unicode markers to ASCII (actually UTF-8, but all markers fall
	// within the ASCII range of UTF-8) -- function is unused so far
	CBString toUTF8(wxString& str);

// ---------------------------------------------------------------

	// query functions...
	
	bool IsAHaltingMarker(wxChar* pChar, CustomMarkersFT inclOrExclFreeTrans, 
			CustomMarkersN inclOrExclNote); // used to ensure a halt when parsing a marker's content
	// overloaded version when the start of the buffer may be a halting marker, or
	// something else - such as no marker, or an inline marker
	bool IsAHaltingMarker(wxString& buff, CustomMarkersFT inclOrExclFreeTrans, 
			CustomMarkersN inclOrExclNote); // used to ensure a halt when parsing a marker's content
	bool IsAVerseBridge(wxString& data);
	bool IsSpecialTextStyleMkr(wxChar* pChar);
	//bool IsEmbeddedWholeMarker(wxChar* pChar);
	bool IsOneOf(wxString& str, wxArrayString& array, CustomMarkersFT inclOrExclFreeTrans, 
			CustomMarkersN inclOrExclNote); // eg. check for SFMkr in array

// ---------------------------------------------------------------

	// chunking functions for SFM or USFM...
	
	wxString* GetTitleInfoChunk(wxString* pInputBuffer);
	wxString* GetIntroInfoChunk(wxString* pInputBuffer);

// ---------------------------------------------------------------

	// (U)SFM parsing functions...
	
	// a parser which parses over inline (ie. formatting) USFM markers until next halting
	// SF marker is encountered
	int ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, 
					wxString& endMkr, wxString& dataStr, bool& bEmbeddedSpan,  
					CustomMarkersFT inclOrExclFreeTrans, CustomMarkersN inclOrExclNote);
	// a parser which takes the TitleInfo chunk and parses it for it's array of aiGroup
	// structs
	void ParseTitleInfoForAIGroupStructs(); 
	// a parser which takes the IntroInfo chunk and parses it for it's array of aiGroup
	// structs
	void ParseIntroInfoForAIGroupStructs(); 


// ---------------------------------------------------------------

	// other implementation functions...
	
	void ExtractNotesAndStoreText(aiGroup* pCurAIGroup, wxString& theText);
	void PopulateIntroductionPossibleMarkers(wxArrayString& arrMkrs);
	void WarnNoEndmarkerFound(wxString endMkr, wxString content);
	void WarnAtBufferEndWithoutFindingEndmarker(wxString endMkr);
	void SetNoteDetails(NoteDetails* pDetails, int beginOffset, wxString& dataStr);


// ---------------------------------------------------------------

private:

// ----------------------DATA STORAGE----------------------------

	CAdapt_ItApp*	m_pApp;	// The app owns this
	wxString m_bookID; // store the 3-letter book code (eg. LUK, GEN, 1CO, etc) here
	int m_version; // initially = 1, later can be 1 or 2
	wxString m_haltingMarkers; // populated in Initialize()
	wxString m_introHaltingMarkers; // for introductory material; populated in Initialize(), these retain digits for level
	wxString m_haltingMarkers_IntroOnly; // populated in Initialize(), these lack digits for level
	wxString m_introInlineBeginMarkers; // populated in Initialize()
	wxString m_introInlineEndMarkers; // populated in Initialize()
	wxString m_specialMkrs; // special markers not listed in the global 
							// charFormatMkrs & populated in class creator
	wxString m_specialEndMkrs; // special markers not listed in the global 
							   // charFormatMkrs & populated in class creator
	wxString* m_pBuffer; // ptr to the (ex-class) buffer containing the (U)SFM text
	bool m_bContainsFreeTrans;
	bool m_bContainsNotes;
	wxString m_freeMkr; // "\free"
	wxString m_freeEndMkr; // "\free*"
	wxString m_noteMkr; // "\note"
	wxString m_noteEndMkr; // "\note*"

	TitleInfo* m_pTitleInfo; // struct for storage for TitleInfo part of document
	IntroductionInfo* m_pIntroInfo; // struct for storage of Introduction material

// ---------------------------------------------------------------

	DECLARE_EVENT_TABLE()
};

#endif /* Usfm2Oxes_h */
