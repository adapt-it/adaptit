/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MergeUpdatedSrc.h
/// \author			Bruce Waters
/// \date_created	12 April 2011
/// \date_revised	
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
    #pragma interface "helpers.h"
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

enum SubspanType {
	beforeSpan, // whatever subspan precedes those which are in common
	commonSpan, // the in common subspan (at can be empty, in which case only the 
				// beforeSpan has content)
	afterSpan   // whatever subspan follows those which are in common (this can be 
				// as large as the remaining CSourcePhrase instances in oldSPArray
				// and newSPArray, when the Subspan is at the right edge in the tree
				// -- that is, the yet-to-be-processed instances)
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

void	GetAllCommonSubspansFromOneParentSpan(SPArray& arrOld, SPArray& arrNew, Subspan* pParentSubspan, 
				wxArrayString* pUniqueCommonWordsArray, wxArrayPtrVoid* pSubspansArray, 
				wxArrayInt* pWidthsArray, bool bClosedEnd);
int		FindNextInArray(wxString& word, SPArray& arr, int startFrom, int endAt, wxString& phrase); 
int		GetKeysAsAString_KeepDuplicates(SPArray& arr, Subspan* pSubspan, bool bShowOld, 
										wxString& keysStr, int limit);
int		GetMaxInCommonSubspan(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, bool bClosedEnd, wxArrayString* pWords,
				Subspan* pMaxInCommonSubspan);
bool	GetNextMatchup(wxString& word, SPArray& arrOld, SPArray& arrNew, int oldStartAt, int newStartAt,
					   int oldStartFrom, int oldEndAt, int newStartFrom, int newEndAt, int& oldMatchedStart, 
					   int& oldMatchedEnd, int & newMatchedStart, int& newMatchedEnd, int& oldLastIndex,
					   int& newLastIndex);
int		GetUniqueOldSrcKeysAsAString(SPArray& arr, Subspan* pSubspan, wxString& oldSrcKeysStr, int limit);
int		GetWordsInCommon(SPArray& arr, Subspan* pSubspan, wxString& uniqueKeysStr, wxArrayString& strArray,
						 int limit);
//int		GetWordsInCommon(SPArray& arrOld, int oldStartAt, SPArray& arrNew, int newStartAt, 
//						 wxArrayString& strArray, int limit); // overload which
							// encapsulates the GetUniqueSrcKeysAsAString() call
int		GetWordsInCommon(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, wxArrayString& strArray,
						 int limit); // overload which
						// encapsulates the GetUniqueSrcKeysAsAString() call
void	InitializeSubspan(Subspan* pSubspan, SubspanType spanType, int oldStartPos, 
						  int newStartPos, int oldEndPos, int newEndPos, bool bClosedEnd = TRUE);
bool	IsLeftAssociatedPlaceholder(CSourcePhrase* pSrcPhrase);
bool	IsRightAssociatedPlaceholder(CSourcePhrase* pSrcPhrase);
bool	IsMatchupWithinAnyStoredSpanPair(int oldPos, int newPos, wxArrayPtrVoid* pSubspansArray);
bool	IsMergerAMatch(SPArray& arrOld, SPArray& arrNew, int oldLoc, int newFirstLoc);
void	MergeUpdatedSourceText(SPList& oldList, SPList& newList, SPList* pMergedList,
							   int limit = SPAN_LIMIT);
void	RecursiveTupleProcessor(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList,
						int limit, Subspan* tuple[]); // the array size is always 3, so 
													  // we don't need a parameter for it
bool	WidenLeftwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount);
bool	WidenRightwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int& oldEndAt,
				int newStartAt, int& newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount, bool bClosedEnd);
void	WidenMatchup(SPArray& arrOld, SPArray& arrNew,  int oldStartPos, int newStartPos, 
				int oldEndPos, int newEndPos, int oldMatchLoc, int newMatchLoc, 
				bool bClosedEnd, Subspan* pSubspan);

#endif	// mergeUpdatedSrc_h
