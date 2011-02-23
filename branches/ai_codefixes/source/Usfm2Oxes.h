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
// synonym is more meaningful when making a test where including or excluding the \rem marker
// in the test is what is in focus
enum RemarkMarker {
	excludeRemarkFromTest = 0,
	includeRemarkInTest,
	ignoreRemarkWhenParsing = 0,
	haltAtRemarkWhenParsing
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

// when parsing a section for it's paragraph chunks, the first paragraph chunk is special
// because that is the one which will have sectionHeader info (although some paragraphs
// might lack a section header - of what SF markup typically calls a 'subheading')
enum WhichParagraph {
	parsingFirstParagraph,
	parsingNonFirstParagraph
};

// section part names - names for productions at the first level within a section - this
// can be added to, but the Dhao example from Jim Albright indicates that the following are
// sibling productions as immediately children of <section>: sectionHead,
// parallelPassageHead, p  [ie. paragraph], and l [ie. line -- for USFM's poetry elements,
// such as\q \q2 \m etc].
// So our approach to parsing sections needs to be able to store disparate objects - we'll
// handle this by having one object with a sectionPartType member, and it will be an enumeration
// which uses the relevant ones of the following values...
enum ChunkType {
	titleChunkType,
	introductionChunkType,
	sectionChunkType,
	sectionPartChunkType, // it will have subtype taken from the next 6 enum values,
		// the first 5 of which are small chunks and won't need much further decomposition
		// but the last, the paragraphChunkType will typically be large and require an
		// array of aiGroup structs for further decomposition; the first four kinds, if
		// any or all occur, they occur at the top of the section - so are parsed for
		// first in a loop; when no more are found, break out of the loop and enter a
		// second loop to parse for poetry and paragraph chunks
	majorOrSeriesChunkType, // for \ms \ms1 \qa [we'll treat \ms2 or \ms3 as \ms1]
	rangeOrPsalmChunkType, // for \mr or \d (these, if they occur, follow \ms or \qa)
	normalOrMinorChunkType, // for \s \s1 \s2 [minor, and we'll treat \s3 or \s4 as \s2]
	parallelPassageHeadChunkType, // for \r
	poetryChunkType, // for \q \q1 \q2 etc
	paragraphChunkType, // for \p \m and other paragraph types
	groupChunkType
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
    // for storing (1) zero, one or several \rem remarks fields, (2) free translation, (3)
    // one or more embedded notes following the free translation, (4) one or more SF
    // markers preceding the text, (5) the text; the information for a single aiGroup is
    // typically deemed complete when the document end is reached or one of the following
    // is true: another \rem field, or free translation occurs, a note which follows the
    // text occurs, a marker which halts parsing is encountered (such as \c \v, \p, \q,
    // \q#, \s, \s# etc)
    wxArrayString arrRemarks; // stores zero, one or several content strings from zero, one
								// or several \rem fields from the USFM markup
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

struct SectionInfo; // forward reference

struct SectionPart
{
	bool bChunkExists;
	ChunkType sectionPartType; // an enum value
	SectionInfo* pSectionInfo; // my owner in the hierarchy of structs
	wxArrayString arrSkipMarkers; // skip over and these but retain in the chunk (eg. \c)
	wxArrayString arrPossibleMarkers; // what's in here depends on sectionPartName
	wxString strChunk; // either material comprising a sectionHead, or parallelPassageHead,
					   // or Poetry of level 1, 2, 3 etc, or some kind of Paragraph
					   // (usually \p or \m)
	AIGroupArray aiGroupArray; // only paragraph chunks are likely to need more than one aiGroup				   
};

	WX_DECLARE_OBJARRAY(SectionPart*, AISectionPartArray); // store pointers, and manage 
													   // the heap ourselves

    // For SectionPart structs, which can be one of 4 variant types, we have to set up
    // (using Initialize() the markers which define each type - and so for efficiency we
    // want to set them up only once each. We'll define a (private)
    // SectionPartTemplateArray which will have 4 SectionType instances (actually, pointers
    // to SectionType) - we need only 4, provided we don't find another type of subChunk of
    // a section other than the 4 currently observed, ie. sectionHead, parallelPassageHead,
    // Poetry, Paragraph -- but we can add extra easily if necessary. Then each time we
    // create a SectionPart instance on the heap, we'll clone it using the copy constructor
    // from the relevant one in this template array. Since each arrPossibleMarkers is a
    // wxArrayString, and the latter has an overloaded operator= defined, we'll get a copy
    // of the marker set put into each instance we create from the templated instance - we
    // only then need to set the instance's partName member, and we can then use the newly
    // created instance for delineating the next chunk being parsed. The
    // ParseSectionIntoSectionParts() function will input a section and do the parsing into
    // an object array of SectionPart* pointers - the protocol for doing the parsing will
    // need to be (1) try for a sectionHead parse first, if it succeeds, try for (2) a
    // parallelPassageHead parse, then try for (3) a Poetry parse, and if none of those
    // produced a span of parsed text, try for (4) a Paragraph parse.
	WX_DECLARE_OBJARRAY(SectionPart*, SectionPartTemplateArray);


struct SectionInfo
{
	wxString strChunk;
	bool bChapterStartsAtSectionStart; // default should be FALSE
	bool bChapterEndsAtSectionEnd; // default should be FALSE
    // a section can span a change of chapters (eg. Mark 8 to Mark 9), and so if both the
    // above booleans are false it is possible that bChapterChangesMidSection can be TRUE,
    // in which case strChapterNumAtSectionStart and strChapterNumAtSectionEnd will contain
    // different values; it's even theoretically possible that the chapter number will
	// change mid section and also at its end or start or both, though I doubt this would
	// ever occur.
	wxString strChapterNumAtSectionStart; // default should be empty string
	wxString strChapterNumAtSectionEnd; // default should  be empty string
	bool bChapterChangesMidSection; // default should be FALSE

    // store the initial info of a section, and the set of SectionPart chunks (sectionHead,
    // parallelPassageHead, Poetry, or Paragraph -- the SectionPart's partName member
    // distinguishes between them), and each such has a pointer pSectionInfo which points
    // at the owning SectionInfo struct; this is the next level of the parsing hierarchy
    // defined over the canonical information which some people call the 'section
    // hierarchy')
	AISectionPartArray sectionPartArray;
};

WX_DECLARE_OBJARRAY(SectionInfo*, AISectionInfoArray); // store pointers, and manage 
													   // the heap ourselves


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
	wxArrayString arrSkipMarkers; // skip over and these but retain in the chunk (unused)
	wxArrayString arrPossibleMarkers;
	AIGroupArray aiGroupArray; // stores aiGroup struct pointers
};

struct IntroductionInfo
{
	// currently supporting: the full range of introduction markers
	bool bChunkExists;
	wxString strChunk;
	wxArrayString arrSkipMarkers; // skip over and these but retain in the chunk (unused)
	wxArrayString arrPossibleMarkers;
	AIGroupArray aiGroupArray; // stores aiGroup struct pointers
};

struct CanonInfo
{
	// Use an AISectionArray to store all the SectionInfo structs which chunk the
	// cannonical material into a series of sections.
	// Supports \s \s1 \s2 \s3 \ms \ms1 \ms2 \ms3 as as section defining markers, but we
	// include code in our sectioning parser to ensure that a chapter \c marker preceding
	// any of these is included within that section's chunk; the sectioning markers are
	// space delimited in the string m_sectioningMkrs

	wxString strChunk;
	AISectionInfoArray arrSections;
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

	// setup functions & shutdown functions & initializing functions...
	
	void Initialize();
	void SetupOxesTags(); // *** TODO *** finish this off later
	void InitializeSectionInfo(SectionInfo* pSectionInfo);
	void ClearAIGroupArray(AIGroupArray& rGrpArray);
	void ClearTitleInfo();
	void ClearIntroInfo();
	void ClearCanonInfo();
	void ClearSectionInfo(SectionInfo* pSectionInfo);
	void ClearNoteDetails(NoteDetailsArray& rNoteDetailsArray);
	void ClearAISectionInfoArray(AISectionInfoArray& arrSections);
	void ClearAISectionPartArray(AISectionPartArray& arrSectionParts);
	void ClearAISectionPart(SectionPart* pSectionPart);

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
	bool IsNormalSectionMkr(wxString& buffer); // TRUE if at start of buffer, \s or \s1 
											   // or \s2 or \s3 is present
	bool IsSpecialTextStyleMkr(wxChar* pChar);
	//bool IsEmbeddedWholeMarker(wxChar* pChar);
	bool IsOneOf(wxString& str, wxArrayString* pArray, CustomMarkersFT inclOrExclFreeTrans, 
			CustomMarkersN inclOrExclNote, RemarkMarker inclOrExclRemark); // eg. check for SFMkr in array
	bool IsWhiteSpace(wxChar& ch);
	//bool IsMkrAllowedPrecedingParagraphs(wxString& wholeMkr); // tests against the marker <<-- unneeded
		// set stored (space delimited) in m_allowedPreParagraphMkrs (see Initialize())
	//bool IsAHaltingMarker_PreFirstParagraph(wxString& wholeMkr); // tests against the <<-- unneeded
		// marker set stored in m_haltingMrks_PreFirstParagraph (see Initialize())
	bool IsANoteMarker(wxString& wholeMkr);
	bool IsAFreeMarker(wxString& wholeMkr);

// ---------------------------------------------------------------

	// chunking functions for USFM...

	// Chunker has the knowledge of Adapt It's notes, free translations, and PT \rem
	// remarks field, these associate with a <trGroup> (which in our parser, is realized
	// when chunking as an aiGroup) and precede the text of the group within the input
	// data file, so our chunker has to look ahead all the time to work out where the
	// chunk ends; returns the input string minus the chunk just delineated. Returns TRUE
	// in the bOnlySkippedMaterial if the only material identified and scanned over was
	// material stipulated (in pChunkStruct's arrSkipMarkers array member) as to be
	// skipped (the caller will need to use this fact to avoid assuming that a certain
	// type of chunk was identified and scanned over) Chunker() is good only for chunks
	// where the possible markers in the chunk are unique to the chunk, plus any or all of
	// \rem, \free, and \note.
	int  Chunker(wxString* pInputBuffer, enum ChunkType chunkType, void* pChunkStruct,
		bool& bOnlySkippedMaterial);
	// the following ones use Chunker()
	wxString* GetTitleInfoChunk(wxString* pInputBuffer);
	wxString* GetIntroInfoChunk(wxString* pInputBuffer);
	// for poetry chunking
	int PoetryChunker(wxString* pInputBuffer,  enum ChunkType chunkType, 
					   void* pChunkStruct, bool& bOnlySkippedMaterial, bool& bBlankLine);
	// for paragraph chunking
	int ParagraphChunker(wxString* pInputBuffer,  enum ChunkType chunkType, 
					   void* pChunkStruct, bool& bOnlySkippedMaterial);

// ---------------------------------------------------------------

	// USFM parsing functions...
	
	// a parser which parses over inline (ie. formatting) USFM markers until next halting
	// SF marker is encountered
	int ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, 
					wxString& endMkr, wxString& dataStr, bool& bEmbeddedSpan,  
					CustomMarkersFT inclOrExclFreeTrans, CustomMarkersN inclOrExclNote,
					RemarkMarker inclOrExclRemarks);
	// a parser which takes the TitleInfo chunk and parses it for it's array of aiGroup
	// structs
	void ParseTitleInfoForAIGroupStructs(); 
	// a parser which takes the IntroInfo chunk and parses it for it's array of aiGroup
	// structs
	void ParseIntroInfoForAIGroupStructs();
	// a parser which takes the m_pCanonInfo->strChunk data and parses it into a series of
	// SectionInfo structs - each of the latter stores the section data in its own
	// strChunk member; return FALSE if the canon is empty of verses, else TRUE
	bool ParseCanonIntoSections(CanonInfo* pCanonInfo);
    // a chunker which scans over all the SectionInfo chunks, to divide each into its
    // component parts - SectionPart chunks
	bool ParseSectionsIntoSectionParts(AISectionInfoArray* pSectionsArray);
    // the chunker which chunks an individual SectionInfo to one or more SectionPart
    // chunks, those being chunks for paragraphs, and any pre-paragraph fields such as \ms
    // \s \c \mr \r or poetry etc
	bool ParseSingleSectionIntoSectionParts(SectionInfo* pSectionInfo);
	// a function to back-parse over any \rem, \free, \note which precedes the match
	// location, returning the number of characters back-parsed over
	int BackParseOverNoteFreeRem(wxString& buff, int nMatchLocation);


// ---------------------------------------------------------------

	// other implementation functions...
	
	void ExtractNotesAndStoreText(aiGroup* pCurAIGroup, wxString& theText);
	//void PopulateIntroductionPossibleMarkers(wxArrayString& arrMkrs); <<-- superceded, & can be removed
	void PopulatePossibleMarkers(wxString& strMarkers, enum ChunkType chunkType, 
									void* pChunkStruct);
	void PopulateSkipMarkers(wxString& strMarkers, enum ChunkType chunkType, 
									void* pChunkStruct);
	void WarnNoEndmarkerFound(wxString endMkr, wxString content);
	void WarnAtBufferEndWithoutFindingEndmarker(wxString endMkr);
	void SetNoteDetails(NoteDetails* pDetails, int beginOffset, wxString& dataStr);
	wxString GetChapterNumber(wxString& subStr); // subStr will have white space
				// followed by a chapter number string followed by probably a newline,
				// and we want to just extract a copy of the chapter number without 
				// changing anything in the subStr string


// ---------------------------------------------------------------

private:

// ----------------------DATA STORAGE----------------------------

	CAdapt_ItApp*	m_pApp;	// The app owns this
	wxString m_bookID; // store the 3-letter book code (eg. LUK, GEN, 1CO, etc) here
	int m_version; // initially = 1, later can be 1 or 2

	// fast-access strings (use these to also populate arrPossibleMarkers wxArrayString
	// members of various structs defined for chunking purposes, using the
	// PopulatePossibleMarkers() function)
	wxString m_titleMkrs;
	wxString m_haltingMarkers; // populated in Initialize()
	wxString m_introHaltingMarkers; // for introductory material; populated in Initialize(), these retain digits for level
	wxString m_haltingMarkers_IntroOnly; // populated in Initialize(), these lack digits for level
	wxString m_introInlineBeginMarkers; // populated in Initialize()
	wxString m_introInlineEndMarkers; // populated in Initialize()
	wxString m_specialMkrs; // special markers not listed in the global 
							// charFormatMkrs & populated in class creator
	wxString m_specialEndMkrs; // special markers not listed in the global 
							   // charFormatMkrs & populated in class creator
	wxString m_sectioningMkrs; // these divide the canonical part into sections,
							   // no endmarkers are involved for these;
							   // populated in Initialize()
	wxString m_chapterMkrs; // for the USFM set of chapter markers
	wxString m_paragraphMkrs; // any paragraph markers, \p or \m etc
	wxString m_poetryMkrs; // these are for what Oxes calls <l> 'line' tags, level is
						   // defined by an level="1" attribute, etc -- we'll support
						   // levels 1 to 3, which should be plenty (most Bibles have
						   // only two levels of indent)
	wxString m_majorOrSeriesMkrs; // there's only a few, based on \ms & \qa
	wxString m_rangeOrPsalmMkrs; // only \mr and \d
	wxString m_normalOrMinorMkrs; // \s \s1 and for minor, \s2, any other like \s3 or 
								 // \s4 we will treat as \s2
	wxString m_parallelPassageHeadMkrs; // there's only one, \r

	//wxString m_allowedPreParagraphMkrs; // markers like \ms \mr \c \s \r and  <<-- not needed, remove later
		// \ms# or \s# which can occur in a section but preceding any paragraphs
	//wxString m_allowedPreParagraphMkrs_NonFirstParagraph; // just \c and \nb
	//wxString m_haltingMrks_PreFirstParagraph; // any marker parsed which is not
		// in m_allowedPreParagraphMkrs should then be in m_haltingMkrs_PreFirstParagraph,
		// but if that is not the case, then put that marker and whatever content
		// follows it into SectionHeaderOrParagraph.unexpectedMarkers, to be shown
		// to the user in the view, but not included in the Oxes export

	wxString* m_pBuffer; // ptr to the (ex-class) buffer containing the (U)SFM text
	bool m_bContainsFreeTrans;
	bool m_bContainsNotes;
	wxString m_freeMkr; // "\\free"
	wxString m_freeEndMkr; // "\\free*"
	wxString m_noteMkr; // "\\note"
	wxString m_noteEndMkr; // "\\note*"
	wxString m_chapterMkr; // "\\c"
	wxString m_verseMkr; // "\\v"
	wxString m_remarkMkr; // "\\rem"
	wxString backslash; // "\\"
	wxString m_majorSectionMkr; // any marker beginning with "\\ms"


	TitleInfo* m_pTitleInfo; // struct for storage for TitleInfo part of document
	IntroductionInfo* m_pIntroInfo; // struct for storage of Introduction material
	CanonInfo* m_pCanonInfo; // struct for storing the canon data in wxString, and 
							 // its parsing into SectionInfo chunks in AISectionInfoArray
	SectionPartTemplateArray m_arrSectionPartTemplate; // stores 6 initialized SectionPart* instances

// ---------------------------------------------------------------

	DECLARE_EVENT_TABLE()
};


/* first attempt, abandoned & unfinished

struct SectionInfo; // forward reference
struct SectionHeaderOrParagraph
{
    // next set of wxStrings stores the info which can occur in a section at its start but
    // which does not belong within a paragraph chunk; by giving each possibility its own
    // storage, it becomes easier to contruct the correct order of Oxes elements later on;
    // most instances of SectionHeaderOrParagraph within a section do not have any of these
    // 13 strings with data in them, except the first instance may have one or more of
    // these strings with content (we assume \r and \mr markers' content strings never are
    // free translated nor contain notes)
	SectionInfo* pSectionInfo; // owning struct
	bool bHasHeaderInfo; // set TRUE when this is the first paragraph and it contains some 
						 // or a lot of header information
	wxString chapterNumAtStart; // chapter marker \c is in this struct,
		// before the first paragraph starts; store its number here, else leave it empty
	wxString majorHeader_ms; // store \ms content here
	wxString majorHeader_ms1; // store \ms1 content here
	wxString majorHeader_ms2; // store \ms2 content here
	wxString majorHeader_ms3; // store \ms3 content here
	AIGroupArray majorHeaderGroupArray; // there could be free translations and notes
										 // segmenting the content string into bits, so
										 // parse to the level of aiGroup instances
	wxString sectionHeader_s; // store \s content here
	wxString sectionHeader_s1; // store \s1 content here
	wxString sectionHeader_s2; // store \s2 content here
	wxString sectionHeader_s3; // store \s3 content here
	AIGroupArray sectionHeaderGroupArray; // there could be free translations and notes
										 // segmenting the content string into bits, so
										 // parse to the level of aiGroup instances
	wxString descriptiveTitle_d; // store \d content here
	wxString majorSectionRef_mr; // store \mr content here
	wxString parallelRef_r; // store \r content here 
	wxString unexpectedMarkers; // store any markers not allowed for this initial stuff,
								// but which are not halting markers for this initial
								// stuff (store their marker with the content, space
								// delimited) -- anything in here should be shown to the
								// user as "unhandled", and not included in the Oxes file
	wxString paragraphOpeningMkr; // store whatever marker begins the paragraph, usually
								  // this is \p, but other possibilities such as \m or \pc
								  // or one of the \q markers will require a type attribute
								  // to be set in the Oxes production, and so we have to
								  // store the marker here so as to test it later on
	// anything not for storage above, should belong to a paragraph chunk, and except for
	// an embedded change of chapter (which, if it occurs, really should be followed by
	// \nb "no break" marker and then would come a verse marker..., so we don't need any
	// other storage here, except for the array which stores the section's set of
	// SectionHeaderOrParagraph instances, called SectHdrOrParaArray. Dividing into
	// paragraphs consumes whatever paragraph type of marker causes the division (there
	// are several possibilities, not just \p only); a complication is that Adapt It's
	// export will show any free translation, or note on the first word, preceding the
	// paragraph marker - so we have to store these temporarily and put them into the
	// chunk at its start once the new paragraph chunk commences
	wxString chapterNumInParaChunk; // chapter marker \c is in this struct,
		// within a paragraph chunk; store its number here, else leave it empty
	bool bNoParagraphBreak_MkrEncountered; // default FALSE, set TRUE if \nb is after \c
		// and when that happens, do not close off the paragraph at the chapter boundary
	wxString paragraphChunk; // the paragraph data, with paragraph marker removed
};

WX_DECLARE_OBJARRAY(SectionHeaderOrParagraph*,AISectHdrOrParaArray); // store pointers, 
													  // and manage the heap ourselves

	void ClearAISectHdrOrParaArray(AISectHdrOrParaArray& rParagraphArray);

	void ParseSectionsIntoParagraphs(CanonInfo* pCanonInfo); // level 2 of the hierarchy
	void ParseOneSection(SectionInfo* pSection); // ParseSectionsIntoParagraphs() calls 
				// this in a loop over all the sections in the document's canonical text
	// ParseHeaderInfoOfSection() is for parsing through header information at the start
	// of the first paragraph of a section
	void ParseHeaderInfoAndFirstParagraph(wxString* pSectionText, 
		SectionHeaderOrParagraph* pCurPara, bool* pbReachedEndOfHdrMaterial,
		bool* pbMatchedParagraphMkrAtEnd, wxString* pStr_wholeMkrAtOpening);
    // ParseNonFirstParagraph() is for parsing a section into a series of
    // non-first-paragraph paragraph chunks - these contain no header information, but may
    // involve a change of chapter, or a non-breaking chapter change which causes the
    // section to continue into the next chapter until a section header is encountered
    // there
	void ParseNonFirstParagraph(wxString* pSectionText, 
		SectionHeaderOrParagraph* pCurPara, bool* pbHandledChapterChange,
		bool* pbMatchedParagraphMkrAtEnd, wxString* pStr_wholeMkrAtOpening,
		bool* bMatchedNoBreakMkr);
	// pass in and parse *pSectionText, bleed of parsed data, storing it in the
	// *pAccumulatedParagraphData string
	void ParseParagraphData(wxString* pSectionText, wxString* pAccumulatedParagraphData,
							SectionHeaderOrParagraph* pCurPara);


*/

#endif /* Usfm2Oxes_h */
