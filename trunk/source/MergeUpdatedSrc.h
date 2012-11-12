/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MergeUpdatedSrc.h
/// \author			Bruce Waters
/// \date_created	12 April 2011
/// \rcs_id $Id$
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is a header file containing some helper functions used 
///                 by Adapt It, for merging updated/edited source text in the form
///                 of a list of CSourcePhrase instances with an earlier list of
///                 adapted CSourcePhrase instances for the same span of source text. 
/////////////////////////////////////////////////////////////////////////////
//
#ifndef mergeUpdatedSrc_h
#define mergeUpdatedSrc_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "MergeUpdatedSrc.h"
#endif

#include <wx/dynarray.h>

class CSourcePhrase;
WX_DECLARE_OBJARRAY(CSourcePhrase*, SPArray);

#ifndef _string_h_loaded
#define _string_h_loaded
#include "string.h"
#endif
#include "Adapt_It.h"

class CBString;
class SPList;	// declared in SourcePhrase.h WX_DECLARE_LIST(CSourcePhrase, SPList); macro 
				// and defined in SourcePhrase.cpp WX_DEFINE_LIST(SPList); macro
class CSourcePhrase;

//struct Matchup {
//	int oldMatchPos; // index in the oldSPArray at which a common word was matched
//	int newMatchPos; // index in the newSPArray at which a common word was matched
//};

enum ProcessHow {
	processStartToEnd, // use this whenever per-milestone isn't appropriate
	processPerMilestone 
};

enum SubspanType {
	beforeSpan, // whatever subspan precedes those which are in common
	commonSpan, // the in common subspan (at can be empty, in which case only the 
				// beforeSpan has content)
	afterSpan   // whatever subspan follows those which are in common (this can be 
				// as large as the remaining CSourcePhrase instances in oldSPArray
				// and newSPArray, when the Subspan is at the right edge in the tree
				// -- that is, the yet-to-be-processed instances)
};

enum WhatYouAre {
	singleton,
	singleton_in_retrans,
	singleton_matches_new_conjoined,
	merger,
	conjoined,
	manual_placeholder,
	placeholder_in_retrans
};

struct Subspan {
	int			oldStartPos;		// index in oldSPArray where CSourcePhrase instances commence
	int			oldEndPos;			// index in oldSPArray where CSourcePhrase instances end (inclusive)
	int			newStartPos;		// index in newSPArray where CSourcePhrase instances commence
	int			newEndPos;			// index in newSPArray where CSourcePhrase instances end (inclusive)
	Subspan*	childSubspans[3];	// a set of beforeSpan, commonSpan & afterSpan Subspan instances on the heap
	SubspanType	spanType;			// an enum with values beforeSpan, commonSpan, afterSpan (this member
									// is redundant, but useful for a sanity check when processing
	bool		bClosedEnd;			// default TRUE, but FALSE for the rightmost afterSpan so that
									// in-common matching can match beyond SPAN_LIMIT instances
};

const int tupleSize = 3;


// Rethinking (May 16) on the algorithm, and discussing with Bill, we noted that the
// addition or removal of large chunks of source text (more than the limit value) has the
// potential, due to spurious matches, of messing up the merger - throwing text fragments
// to places where they don't belong. E.g, a chapter which has some of the source text in
// some verses empty, and the user in Paratext adds the missing source text of those
// verses, then the attempt to merge that source text to the AI document will have only
// spurious matches to anything in that additional material, and taking the largest is no
// protection at all, since it would still be a spurious match, and the resulting merger
// would move material where it shouldn't go. Fortunately, because of USFM or SFM markup,
// we have chapter and verse and subheading and introduction milestones on which we can
// rely - so we will add code to divide the legacy and incoming (possibly edited) source
// text material, after tokennization into CSourcePhrase instances, into subranges which
// are either bCorrelatesWithNothing = FALSE, or bCorrelatesWithNothing = TRUE. The former
// ranges have to have MergeUpdatedSourceText() done on such a subrange (with arrOld and
// arrNew restricted to the CSourcePhrase instances in the subrange) to handle any possible
// user-editing that may have been done. The latter ranges, because either the source
// exists in arrOld but not in arrNew (a removal is needed), or the source exists in arrNew
// but not in arrOld (an insertion is needed), don't require MergeUpdatedSourceText() be
// done on such as these, but instead, either the arrOld material is skipped (that is, just
// not put into pMergedList), or the arrNew material is inserted (that is, it is appended
// to pMergedList at the appropriate time), because there is nothing to compare it with.
// Such potentially large chunks of material to be either removed or added in are a problem
// to our recursive algorithm because of the potential of spurious matches within them, so
// we have to pre-process the input lists to remove from contention any such large chunks,
// and only do the merging on chapters, verses, subheadings, etc which exist in both arrOld
// and arrNew. Our approach is to divide the lists (converted into arrays of CSourcePhrase
// instances) into a series of structs which map, via integers, into subspans in arrOld and
// arrNew, and which are stored in a flat array of wxArrayPtrVoid type. Each struct handles
// a verse, or introduction, or subheading, or chapter-initial material which is none of
// the previous - one such set of structs for the whole of the legacy AI document having
// it's m_key source text values compared, and another for the (possibly edited) source
// text coming from Paratext to which comparison is made; and then a third array, using a
// different struct, to break up the associated subspans in the former two in those which
// are to be merged recursively, and those which are not. The structs below are defined so
// as to meet the above needs. The wxArrayPtrVoid instances will be local variables within
// encapsulating functions, and so are not defined as globals here. The arrays we generate
// in order to provide a cover of the whole data array are deliberately kept flag (ie. no
// nesting of structs in structs), to simplify the query and Find...() functions we define
// for obtaining the spans we are interested in.
// The above approach means we can apply the top level function to any sized source text
// data - whether a whole book, or a single verse, and not have to worry about what kind of
// editing, additions or removals may have been done in the incoming new source text. (The
// structs here, and the chunking functions which use them, may also have value in
// decomposing for an OXES export of the document.)

// the last 3 chunk types below each contain a single verse, so whatever else may be
// there, each will have a verse or verse range - which facilitates comparison of a
// limited span of text - using a limit value of about 80 characters may be a better idea
// than using a value of 50 as before; and for introduction chunks limit = -1 should be
// used as someone may add an introduction as a single slab of source text - possibly
// hundreds of words. A parallet passage reference (\r) may follow a subheading (\s or
// \s#) # = 1 to 4, but we don't expect one without a preceding subheading - if we get
// such a structure, we'll call it a subheadingPlusVerseChunk anyway, so as to minimize
// the number of distinct chunks to test for.
enum SfmChunkType {
	unknownChunkType, // at initialization time & before arriving at a specific type
	bookInitialChunk, // for data like \id line, \mt main title, secondary title, etc
	introductionChunk, // for introductory material, markers beginning \i.... 
	preFirstChapterChunk, // anything after book intro or introduction chunks which precedes \c 1
	chapterPlusVerseChunk, // for stuff from the start of a chapter, to the end of first verse
	subheadingPlusVerseChunk, // for a subheading plus the content of its following verse
	verseChunk // for a verse (will collect any preceding \p too, since the USFM parser
			   // will put the \p \v nnn into the one m_markers member)
};

// We do a set of the following stored in wxArrayPtrVoid for the whole existing Adapt It
// document chunk. As for "document chunk", we have agreed to only store single-chapter
// documents when in PT collaboration mode, never the whole of a book, and this way we can
// more easily do robust support of whatever the PT user may do in PT and AI, without
// duplication of data - such actions which would result in the unwelcome possibility of
// having the same CSourcePhrase(s) in both a whole-book document and in a single-chapter
// document; we'll avoid this by never storing a whole book document, only single-chapter
// ones.
// We do a separate set in a different wxArrayPtrVoid for the whole of the (possibly
// edited) source text (chapter) chunk.
// We'll do analysis on those two and arrive at a third wxArrayPtrVoid which defines the
// span associations between the two which require recursive merging, versus those which
// don't - the latter are chunks where wholesale removals or wholesale additions are
// required, because the association is with nothing within a subspan of one of the two
// tokenized source text arrays.
struct SfmChunk {
	SfmChunkType		type; // one of the above types
	bool				bContainsText; // default TRUE, set FALSE if the information in the chunk is
									   // absent (such as a \v marker without any text
									   // following the verse number)
	int					startsAt; // index into an SPArray defining where the chunk starts
	int					endsAt; // index into an SPArray defining where the chunk ends
	// the remaining members pertain to verseChunk type, and store info from the verse
	// reference within the Adapt It document
	wxString			strChapter; // chapter number as a string (always set, except for introduction material)
	int					nChapter; // chapter number (always set, except for introduction material)
	wxString			strDelimiter; // delimiter string used in a range, eg. - in 3-5
	wxString			strStartingVerse; // string version of the verse number or first verse number of a range
	int					nStartingVerse;  // decimal digits converted to a number, eg 6 from 6a-8
	wxChar				charStartingVerseSuffix;  // for the a in something like 6a-8, or 9a
	wxString			strEndingVerse; // string version of the verse number or final verse number of a range
	int					nEndingVerse; // the decimal digits converted to a number, eg. 8 from 6a-8
	wxChar				charEndingVerseSuffix; // for the a in something like 15-17a
};

struct ChunkInfo {
	int				associatedSfmChunk; // which SfmChunk spawned the data to be stored herein
	int				verse;
	int				chapter;
	SfmChunkType	type;
	bool			bIsComplex; // TRUE if verse num is a range, or something like 6b,
								// or is a gap in the chapter/verses; set to FALSE if 
								// just a simple number
	int				indexRef; // index to CSourcePhrase in arrOld or arrNew
};

// Our approach to the analysed range of SfmChunk types for arrOld and arrNew is to
// process them after creating the arrays of chunks, matching up the longest possible sets
// of corresponding simple verses from each array. Whenever we get to a gap, or a range not
// matched EXACTLY in it's verse and chapter reference information, or a verse number which
// isn't simple (such as 6b) which is not matched EXACTLY in the other array, then we look
// ahead (as far as the ends of the arrays of SfmChunk instances) for a safe halt location
// -- defined as a matchup of simple verse numbers (but same chapter and chunk type of
// course) that is, just simple verse numbers, like 7 or 12 etc. We try to find these
// matching values of such numbers as close as possible to the start of such arrays. Then
// we count the words in that more-complex pairing of old and new CSourcePhrase instances,
// and call MergeRecursively() with a limit value equal to the larger of the two word
// counts we got (one each, from arrOld and arrNew) for the more-complex paired subspans --
// that way we can be sure that the limit value is large enough to be a cover for all the
// words involed, so that the recursive merge will succeed (and not stop short). For
// non-complex paired spans, which could be quite long if the user has not edited anything
// for a large stretch of the source text, we just call MergeRecursively() with a default
// small limit value, SPAN_LIMIT (currently defined as 50). The idea behind using the large
// dynamically defined limit value for unmatched gaps, or verse ranges, etc, is so that we
// compromise some processing time wasted for the convenience of not analysing what
// actually happens in the more-complex section - the possibilities for the latter are too
// numberous to bother with. We just instead use a large limit value to be sure that the
// code's detecting the largest possible in-common span actually finds the largest when
// starting. Only that makes the processing safe in the context of ranges, gaps, large
// chunks of inserted source text, and so forth.

// Typical scripture data involves some chapterPlusVerseChunk instances interspersed with
// lots of verseChunk instances and in the latter there will generally be a few
// subheadingPlusVerseChunk instances. However, although these are typed differently, the
// range of verses spanned is consecutive because these types are stored in the same order
// in which they occur in the SPArray span. So, for example, a chunk association which is
// mergeable may be, perhaps, chapter 2 verse 1 up to chapter 5 verse 3, because in the
// new source text we want to merge we find that this range of verses and chapters is also
// present (there is some discontinuity at either end, obviously, otherwise we'd have a
// longer matchup), and the additional condition that for every chapter:verse combination
// in the legacy instances there is the same chapter:verse combination in the new
// instances. These two conditions, the lack of discontinuities within, and the presence of
// matched milestones within, make it safe to call MergeRecursively() on that paired
// extent of subranges of arrOld and arrNew that are involved in the association with just
// a small value for limit (such as 50).

bool	AnalyseChapterVerseRef(wxString& strChapVerse, wxString& strChapter, int& nChapter, 
						wxString& strDelimiter, wxString& strStartingVerse, int& nStartingVerse,
						wxChar& charStartingVerseSuffix, wxString& strEndingVerse,
						int& nEndingVerse, wxChar& charEndingVerseSuffix);
bool	AnalyseSPArrayChunks(SPArray* pInputArray, wxArrayPtrVoid* pChunkSpecifiers,
							 int& countOfChapters, int& countOfVerses);
bool	AreSizesDisparate(SPArray& arrOld, SPArray& arrNew, SfmChunk* pOldChunk, SfmChunk* pNewChunk);
bool	AreInventoriesDisparate(int oldOnesCount, int newOnesCount);
bool	AreSfmChunksWithSameRef(SfmChunk* pOldChunk, SfmChunk* pNewChunk);
void	ReplaceSavedOriginalSrcPhrases(CSourcePhrase* pMergedSP, wxArrayPtrVoid* pArrayNew);
void	CopySubArray(SPArray& arr, int fromIndex, int toIndex, SPArray& subArray);
void	CopyToList(SPArray& arr, int fromIndex, int toIndex, SPList* pList);
int		CountWords(SPArray* pArray, wxArrayPtrVoid* pChunksArray, int firstChunk, int lastChunk);
bool	DoesChunkContainSourceText(SPArray* pArray, int startsAt, int endsAt);
bool	DoUSFMandPunctuationAlterations(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan);
void	EraseAdaptationsFromRetranslationTruncations(SPList* pMergedList);
bool	FindClosestSafeMatchup(SPArray& arrOld, SPArray& arrNew, wxArrayPtrVoid* pOldChunks, 
							wxArrayPtrVoid* pNewChunks, int oldStartChunk, int newStartChunk, 
							int& oldMatchedChunk, int& newMatchedChunk, bool& bDisparateSizes);
int		FindNextInArray(wxString& word, SPArray& arr, int startFrom, int endAt, wxString& phrase); 
wxString FindVerseReference(SPArray* arrP, int startFrom, int endAt);
bool	GetAllCommonSubspansFromOneParentSpan(SPArray& arrOld, SPArray& arrNew, Subspan* pParentSubspan, 
				wxArrayString* pUniqueCommonWordsArray, wxArrayPtrVoid* pSubspansArray, 
				wxArrayInt* pWidthsArray, bool bClosedEnd);
bool	GetBookInitialChunk(SPArray* arrP, int& startsAt, int& endsAt);
bool	GetChapterPlusVerseChunk(SPArray* arrP, int& startsAt, int& endsAt);
bool	GetPreFirstChapterChunk(SPArray* arrP, int& startsAt, int& endsAt);
bool	GetMaxInSyncChunksPairing(SPArray& arrOld, SPArray& arrNew, wxArrayPtrVoid* pOldChunksArray,
						wxArrayPtrVoid* pNewChunksArray, int oldStartChunk, int newStartChunk, 
						int& oldEndChunk, int& newEndChunk, bool& bDisparateSizes, 
						wxArrayInt& arrPairingOld);
bool	GetIntroductionChunk(SPArray* arrP, int& startsAt, int& endsAt);
bool	GetSubheadingPlusVerseChunk(SPArray* arrP, int& startsAt, int& endsAt);
bool	GetVerseChunk(SPArray* arrP, int& startsAt, int& endsAt);
int		GetKeysAsAString_KeepDuplicates(SPArray& arr, Subspan* pSubspan, bool bShowOld, 
										wxString& keysStr, int limit);
Subspan* GetMaxInCommonSubspan(SPArray& arrOld, SPArray& arrNew, Subspan* pParentSubspan, int limit);
bool	GetNextMatchup(wxString& word, SPArray& arrOld, SPArray& arrNew, int oldStartAt, int newStartAt,
					   int oldStartFrom, int oldEndAt, int newStartFrom, int newEndAt, int& oldMatchedStart, 
					   int& oldMatchedEnd, int & newMatchedStart, int& newMatchedEnd, int& oldLastIndex,
					   int& newLastIndex);
bool	GetNextCommonSpan(wxString& word, SPArray& arrOld, SPArray& arrNew, int oldStartAt, 
					   int newStartAt, int oldStartFrom, int oldEndAt, int newStartFrom, int newEndAt, 
					   int& oldMatchedStart, int& oldMatchedEnd, int & newMatchedStart, 
					   int& newMatchedEnd, int& oldLastIndex, int& newLastIndex, bool bClosedEnd, 
					   wxArrayPtrVoid* pCommonSpans, wxArrayInt* pWidthsArray);
int		GetNextNonemptyMarkers(SPArray* pArray, int& startFrom, bool& bReachedEndOfArray);
bool	GetNextSimpleVerseChunkInfo(wxArrayPtrVoid* pChunkInfos, int startFrom, int& foundAt);
int		GetUniqueOldSrcKeysAsAString(SPArray& arr, Subspan* pSubspan, wxString& oldSrcKeysStr, int limit);
int		GetWordsInCommon(SPArray& arr, Subspan* pSubspan, wxString& uniqueKeysStr, wxArrayString& strArray,
						 int limit);
int		GetWordsInCommon(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, wxArrayString& strArray,
						 int limit); // overload which encapsulates the GetUniqueSrcKeysAsAString() call
void	InitializeUsfmMkrs();
void	InitializeSubspan(Subspan* pSubspan, SubspanType spanType, int oldStartPos, 
						  int oldEndPos, int newStartPos, int newEndPos, bool bClosedEnd = TRUE);
bool	IsLeftAssociatedPlaceholder(CSourcePhrase* pSrcPhrase);
bool	IsMatchupWithinAnyStoredSpanPair(int oldPosStart, int oldPosEnd, int newPosStart, 
						int newPosEnd, wxArrayPtrVoid* pSubspansArray);
bool	IsMergerAMatch(SPArray& arrOld, SPArray& arrNew, int oldLoc, int newFirstLoc);
bool	IsChunkEmpty(SPArray arr, int indexStart, int indexEnd);
bool	IsRightAssociatedPlaceholder(CSourcePhrase* pSrcPhrase);
void	MergeOldAndNew(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, SPList* pMergedList);
void	MergeRecursively(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList, int limit, 
						int initialSequNum, wxArrayPtrVoid* pChunksOld, 
						wxArrayPtrVoid* pChunksNew, enum ProcessHow howToProcess,
						wxArrayInt& arrPairingOld,
						SPArray& arrOldFull, SPArray& arrNewFull,
						int oldChunkIndexKickoff, int newChunkIndexKickoff);
void	MergeUpdatedSourceText(SPList& oldList, SPList& newList, SPList* pMergedList, int limit);
void	MergeUpdatedSrcTextCore(SPArray& oldArray, SPArray& newArray, SPList* pMergedList, int limit);
void	RecursiveTupleProcessor(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList,
						int limit, Subspan* tuple[]); // the array size is always 3, so 
													  // we don't need a parameter for it
void	InitializeNonVerseChunk(SfmChunk* pStruct);
int		PutBeginMarkersIntoArray(CSourcePhrase* pSrcPhrase, wxArrayString* pArray);
int		PutEndMarkersIntoArray(CSourcePhrase* pSrcPhrase, wxArrayString* pArray);
void	ReplaceMedialPunctuationAndMarkersInMerger(CSourcePhrase* pMergedSP, wxArrayPtrVoid* pArrayNew,
						wxString& parentPrevPunct, wxString& parentFollPunct, wxString& strFromMedials);
void	RemoveAll(SPArray* pSPArray);
bool	SetupChildTuple(SPArray& arrOld, SPArray& arrNew, Subspan* pParentSubspan, Subspan* tuple[],
						int limit);
void	SetEndIndices(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, int limit); // overload
bool	TransferForFixedSpaceConjoinedPair(SPArray& arrOld, SPArray& arrNew, int oldIndex, int newIndex,  
						Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding);
void	TransferFollowingMembers(CSourcePhrase* pFrom, CSourcePhrase* pTo, 
						bool bFlagsToo, bool bClearAfterwards);
bool	TransferPunctsAndMarkersToMerger(SPArray& arrOld, SPArray& arrNew, int oldIndex, int newIndex,
						Subspan* pSubspan, int& oldDoneToIncluding, int & newDoneToIncluding);
void	TransferPunctsAndMarkersOnly(CSourcePhrase* pFrom, CSourcePhrase* pTo, bool bClearAfterwards);
void	TransferPrecedingMembers(CSourcePhrase* pFrom, CSourcePhrase* pTo, bool bAICustomMkrsAlso, 
						 bool bFlagsToo, bool bClearAfterwards);
bool	TransferToManualPlaceholder(SPArray& arrOld, SPArray& arrNew, int oldIndex, int newIndex, 
				Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding);
bool	TransferToPlaceholderInRetranslation(SPArray& arrOld, SPArray& arrNew, int oldIndex, 
		int newIndex, Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding);
bool	TransferToSingleton(SPArray& arrOld, SPArray& arrNew, int oldIndex, int newIndex, 
						 Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding);
WhatYouAre	WhatKindAreYou(CSourcePhrase* pSrcPhrase, CSourcePhrase* pNewSrcPhrase);
bool	WidenLeftwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount);
bool	WidenRightwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount);
void	DeleteSfmChunkArray(wxArrayPtrVoid* pChunksArray);

#endif	// mergeUpdatedSrc_h
