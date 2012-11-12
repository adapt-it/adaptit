/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MergeUpdatedSrc.cpp
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

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "MergeUpdatedSrc.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include "Adapt_It.h"
#include "FreeTrans.h"
#include "SourcePhrase.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "Adapt_ItView.h"
#include "AdaptitConstants.h"
#include "MergeUpdatedSrc.h"

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(SPArray);

// **** for debugging ****
// To turn on wxLogDebug() calls, uncomment out one or more of next lines.
// To see the old and new arrays of CSourcePhrase instances with wxLogDebug,
// uncomment out the #define ShowConversionItems just preceding the helpers.cpp function
// void ConvertSPList2SPArray(SPList* pList, SPArray* pArray), at about line 8772

/* If you don't know what you need to look at for debugging, turn on the first five below...
#define myLogDebugCalls       // probably the most useful one overall, and for counting spans & their deletions
#define myMilestoneDebugCalls // useful for outer loop and MergeRecursively() calls
#define LOOPINDEX             // this one gives the loop indices at the start of each iteration
#define MERGE_Recursively     // use this one and the next to look at the tuple processing
#define _RECURSE_			  // gives useful information when recursion takes place in merging matched spans
//#define LEFTRIGHT			  // displays results of extending in-common matches to left or right
							  //(this one has limited usefulness, only use if extending issues are your focus)
*/
/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;

// marker strings needed in this module and populated in InitializeUsfmMkrs()
wxString titleMkrs;
wxString introductionMkrs;
wxString chapterMkrs;
wxString verseMkrs;
wxString normalOrMinorMkrs;
wxString rangeOrPsalmMkrs;
wxString majorOrSeriesMkrs;
wxString parallelPassageHeadMkrs;

#if defined(_DEBUG) && defined(myLogDebugCalls)
// for debugging memory leaks
int countBeforeSpans = 0;
int countCommonSpans = 0;
int countAfterSpans = 0;
int countBeforeSpanDeletions = 0;
int countCommonSpanDeletions = 0;
int countAfterSpanDeletions = 0;
#endif


void InitializeUsfmMkrs()
{
	titleMkrs = _T("\\id \\ide \\h \\h1 \\h2 \\h3 \\mt \\mt1 \\mt2 \\mt3 ");
	introductionMkrs = _T("\\imt \\imt1 \\imt2 \\imt3 \\imte \\is \\is1 \\is2 \\is3 \\ip \\ipi \\ipq \\ipr \\iq \\iq1 \\iq2 \\iq3 \\im \\imi \\imq \\io \\io1 \\io2 \\io3 \\iot \\iex \\ib \\ili \\ili1 \\ili2 \\ie ");
	chapterMkrs = _T("\\c \\cl "); // \ca \ca* \p & \cd omitted, they follow
								   // \c so aren't needed for chunking
	verseMkrs = _T("\\v \\vn "); // \va \va* \vp \vp* omitted, they follow \v
				// and so are not needed for chunking; \vn is a non-standard
				// 'verse number' marker that some people use
	normalOrMinorMkrs = _T("\\s \\s1 \\s2 \\s3 \\s4 ");
	majorOrSeriesMkrs = _T("\\ms \\ms1 \\qa \\ms2 \\ms3 ");
	parallelPassageHeadMkrs = _T("\\r ");
	rangeOrPsalmMkrs = _T("\\mr \\d ");

	// I've got memory leaks -- all are 36 byte blocks, so get some struct sizes...
	//int size_of_Subspan = sizeof(Subspan); // yep, this has size of 36 bytes
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// \return                    the word count
/// \param  arr             -> the arrOld from which we extract the m_key member
///                            from each CSourcePhrase instance
/// \param  pSubspan        -> the subspan in arrOld we are dealing with, from it we need the
///                            bClosedEnd value, and also the oldEndPos value - we use the
///                            latter when bClosedEnd is TRUE (to keep from scanning past
///                            the end of the span which must not happen when the flag is TRUE)
/// \param  oldSrcKeysStr   <- pass back the string in the form "word1 word2 word3 ... wordn"
///                            (the words are keys and so have any beginning and ending
///                            puntuation stripped off)
/// \param  limit           -> how many consecutive CSourcePhrase instances to deal with,
///                            to limit the checking so as to apply to just that many
///                            (default is SPAN_LIMIT, currently set to 80) use -1 if no
///                            limit is wanted & so all are to be checked. If the
///                            array does not have the limit amount of instances, all are
///                            checked.
/// \remarks
/// The returned string is to be used as a "fast access string", that is, to quickly
/// determine if a given word is in the string by calling .Find(_T("someword")) on it and
/// checking for a wxNOT_FOUND to indicate it is not in the string, otherwise it is there.
/// The returned string is filled from the CSourcePhrase instances starting from the
/// beginning of the array arrOld, and proceding to the end. This implies that words early
/// in the string are likely to be also early in the arrNew array of CSourcePhrase
/// instances built from the updated/edited source text (the editing probably having been
/// done in Paratext at an earlier time, but not necessarily done in Paratext), and this
/// fact should make our approach within the MergeUpdatedSourceText() function a little
/// more efficient than would be the case if the order was random.
///
/// Note 1: some CSourcePhrase instances encountered may be mergers, and so will contain
/// two or more words. We have to break these into individual word tokens and check for
/// duplicates on each such token. We ignore ellipses (...) for placeholders and don't put
/// them into the return string. Any pseudo-merger conjoined pairs are decomposed into two
/// individual words.
/// Note 2: pSubspan passed in may have one of the subspans with indices (-1,-1) because
/// the subspan in the relevant array is empty, so we need to protect from using such
/// bogus index values as real indices for lookup
/////////////////////////////////////////////////////////////////////////////////////////////
int GetUniqueOldSrcKeysAsAString(SPArray& arr, Subspan* pSubspan, wxString& oldSrcKeysStr, int limit)
{
	int count = arr.GetCount();
	int activeSpanCount;
	oldSrcKeysStr.Empty();
	int wordCount = 0;
	int index;
	wxString space = _T(' ');
	wxString ellipsis = _T("...");
	wxArrayString arrMerged;
	long mergeCount = 0;
	int howMany;
	int nStartAt;
	if (pSubspan->bClosedEnd)
	{
		// we must not display more content than up to and including the oldEndAt
		// index; & beware if -1 index values (they occur as paired -1 start & end values,
		// so if the start is not -1, neither is the end)
		nStartAt = pSubspan->oldStartPos;
		if (nStartAt == -1)
		{
			// the subspan in arrOld is empty, so there cannot be any matchups
			return 0;
		}
		activeSpanCount = pSubspan->oldEndPos - nStartAt + 1;
		if (activeSpanCount == 0)
			return 0;
		howMany = activeSpanCount;
	}
	else
	{
		// how many words we display is limited only by the limit value, and if the
		// latter is -1, it is not limited at all
		nStartAt = pSubspan->oldStartPos;
		if (nStartAt == -1)
		{
			// the subspan in arrOld is empty, so there cannot be any matchups
			return 0;
		}
		activeSpanCount = count - nStartAt; // a default (max) value
		if (activeSpanCount == 0)
			return 0;
        // limit the span of CSourcePhrase instances to be checked to the limit value:
        // possible values passed in are -1, or SPAN_LIMIT (ie. 80), or an explicit
        // value. If limit is -1 or those remaining from nStartAt onwards is positive
        // but fewer than limit remain from nStartAt onwards, then just use the
        // remainder value instead; if SPAN_LIMIT or an explicit value is passed in,
        // use it - but shorten it appropriately if the value would otherwise result in
        // a bounds error.
		if (limit == -1 || (activeSpanCount > 0 && activeSpanCount < limit))
		{
			howMany = activeSpanCount;
		}
		else
		{
			howMany = limit;
		}
	}
	index = nStartAt;
	int aSrcPhraseCounter = 0;
	while (aSrcPhraseCounter < howMany)
	{
		CSourcePhrase* pSrcPhrase = arr.Item(index);
		wxASSERT(pSrcPhrase != NULL);
		wxString aKey = pSrcPhrase->m_key;
		aSrcPhraseCounter++;
		index++;

		if (aKey == ellipsis)
		{
			continue; // skip over any ... ellipses (placeholders)
		}
		if (pSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger -- so get the word tokens and deal with each individually
			wxString aWordToken;
			arrMerged.Clear();
			// handle any pseudo-mergers too
			bool bHasFixedSpace = IsFixedSpaceSymbolWithin(pSrcPhrase);
			if (bHasFixedSpace)
			{
				// it's a conjoining - separate the two words
				SPList::Node* pos = pSrcPhrase->m_pSavedWords->GetFirst();
				int index;
				for (index = 0; index < 2; index++)
				{
					CSourcePhrase* pSP = pos->GetData();
					pos = pos->GetNext();
					aWordToken = pSP->m_key;
					int offset = oldSrcKeysStr.Find(aWordToken);
					if (offset == wxNOT_FOUND)
					{
						oldSrcKeysStr += space;
						oldSrcKeysStr += aWordToken;
						wordCount++; // count it
					}
				}
			}
			else
			{
				// not a conjoining, it's just a normal merger
				mergeCount = SmartTokenize(space, aKey, arrMerged, FALSE); // FALSE means ignore empty strings
				if (mergeCount > 0)
				{
					int indx;
					for (indx = 0; indx < (int)mergeCount; indx++)
					{
						aWordToken = arrMerged.Item(indx);
						int offset = oldSrcKeysStr.Find(aWordToken);
						if (offset == wxNOT_FOUND)
						{
							oldSrcKeysStr += space;
							oldSrcKeysStr += aWordToken;
							wordCount++; // count it
						}
					}
				}
			}
		}
		else
		{
			// it's not a merger
			int offset = oldSrcKeysStr.Find(aKey);
			if (offset == wxNOT_FOUND)
			{
				oldSrcKeysStr += space;
				oldSrcKeysStr += aKey;
				wordCount++; // count it
			}
		}
	} // end of for loop for scanning a span (or all)
	oldSrcKeysStr.Trim(FALSE); //  trim off initial space
	return wordCount;
}

// Like GetUniqueOldSrcKeysAsAString() except removing duplicates is not done, the m_key words
// (some will be merged phrases) are gathered in natural storage order (that is, from left
// to right), so it's correct for LTR or RTL reading scripts; ellipses (placeholders), if
// present, are not retained; nStartAt is the index for the CSourcePhrase instance at
// which to start getting; return the count of words gotten
// pSubspan - the Subspan we are logging
// bShowOld - TRUE shows words from the arrOld array, using the 'old' indices; FALSE from the
//             arrNew array, using the 'new' indices
// keysStr - return the derived string here
// limit - whatever value is used for limiting the span; -1 (no limit), a +ve value,
// limits to that value except when pSubspan has bClosedEnd set TRUE, in which case the
// span end index (ie. either oldEndAt or newEndAT) is used depending on which array we
// are displaying
int GetKeysAsAString_KeepDuplicates(SPArray& arr, Subspan* pSubspan, bool bShowOld, wxString& keysStr, int limit)
{
	int count = arr.GetCount();
	int activeSpanCount;
	keysStr.Empty();
	int wordCount = 0;
	int index;
	wxString space = _T(' ');
	wxString ellipsis = _T("...");
	wxArrayString arrMerged;
	long mergeCount = 0;
	int howMany;
	int nStartAt;
	if (bShowOld)
	{
		if (pSubspan->bClosedEnd)
		{
			// we must not display more content than up to and including the oldEndAt
			// index
			nStartAt = pSubspan->oldStartPos;
			activeSpanCount = pSubspan->oldEndPos - nStartAt + 1;
			if (activeSpanCount == 0)
				return 0;
			howMany = activeSpanCount;
		}
		else
		{
			// how many words we display is limited only by the limit value, and if the
			// latter is -1, it is not limited at all
			nStartAt = pSubspan->oldStartPos;
			activeSpanCount = count - nStartAt; // a default (max) value
			if (activeSpanCount == 0)
				return 0;
            // limit the span of CSourcePhrase instances to be checked to the limit value:
            // possible values passed in are -1, or SPAN_LIMIT (ie. 80), or an explicit
            // value. If limit is -1 or those remaining from nStartAt onwards is positive
            // but fewer than limit remain from nStartAt onwards, then just use the
            // remainder value instead; if SPAN_LIMIT or an explicit value is passed in,
            // use it - but shorten it appropriately if the value would otherwise result in
            // a bounds error.
			if (limit == -1 || (activeSpanCount > 0 && activeSpanCount < limit))
			{
				howMany = activeSpanCount;
			}
			else
			{
				howMany = limit;
			}
		}
	}
	else
	{
		// we are showing words from the CSourcePhrase::m_key members in arrNew
		if (pSubspan->bClosedEnd)
		{
			// we must not display more content than up to and including the oldEndAt
			// index
			nStartAt = pSubspan->newStartPos;
			activeSpanCount = pSubspan->newEndPos - nStartAt + 1;
			if (activeSpanCount == 0)
				return 0;
			howMany = activeSpanCount;
		}
		else
		{
			// how many words we display is limited only by the limit value, and if the
			// latter is -1, it is not limited at all
			nStartAt = pSubspan->newStartPos;
			activeSpanCount = count - nStartAt; // a default (max) value
			if (activeSpanCount == 0)
				return 0;
            // limit the span of CSourcePhrase instances to be checked to the limit value:
            // possible values passed in are -1, or SPAN_LIMIT (ie. 80), or an explicit
            // value. If limit is -1 or those remaining from nStartAt onwards is positive
            // but fewer than limit remain from nStartAt onwards, then just use the
            // remainder value instead; if SPAN_LIMIT or an explicit value is passed in,
            // use it - but shorten it appropriately if the value would otherwise result in
            // a bounds error.
			if (limit == -1 || (activeSpanCount > 0 && activeSpanCount < limit))
			{
				howMany = activeSpanCount;
			}
			else
			{
				howMany = limit;
			}
		}
	} // end of else block for test: if (bShowOld)
	if (nStartAt < 0 || nStartAt > count - 1)
		return 0; // otherwise there would be a bounds error

	// now that nStartAt and howMany are set, construct the string for output and count
	// the words in it
	for (index = nStartAt; index < howMany; index++)
	{
		CSourcePhrase* pSrcPhrase = arr.Item(index);
		wxASSERT(pSrcPhrase != NULL);
		wxString aKey = pSrcPhrase->m_key;
		if (aKey == ellipsis)
			continue; // skip over any ... ellipses (placeholders)
		if (pSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger -- so get the word tokens and deal with each individually,
			// because they have to be counted
			wxString aWordToken;
			arrMerged.Clear();
			mergeCount = SmartTokenize(space, aKey, arrMerged, FALSE); // FALSE means ignore empty strings
			if (mergeCount > 0)
			{
				int indx;
				for (indx = 0; indx < (int)mergeCount; indx++)
				{
					aWordToken = arrMerged.Item(indx);
					keysStr += space;
					keysStr += aWordToken;
					wordCount++; // count it
				}
			}
		}
		else
		{
			// it's not a merger
			keysStr += space;
			keysStr += aKey;
			wordCount++; // count it
		}
	} // end of for loop for scanning a span (or all)
	keysStr.Trim(FALSE); //  trim off initial space
	return wordCount;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                    the count of the words which are in common
/// \param  arr             -> the arrNew from which we extract the m_key member from
///                            each CSourcePhrase instance (the words are keys and so
///                            have any beginning and ending puntuation stripped off)
/// \param  pSubspan        -> from this we get the starting index for the search
///                            (newStartPos), and the bClosedEnd value which determines
///                            how we calculate the howMany value which determines how
///                            many new CSourcePhrase instances we check when looking for
///                            in-common words with the associated subspan in arrOld
/// \param  uniqueKeysStr   -> the string of unique old m_key values, in the form "word1
///                            word2 word3 ... wordn", with which the keys from arrNew
///                            are compared
/// \param  strArray        <- pass in an empty string array, return the "words in common"
///                            in it, one word per item - mergers in the CSourcePhrases
///                            obtained from the arr parameter have to be tokenized to
///                            individual word tokens before storage in strArray
/// \param  limit           -> how many consecutive CSourcePhrase instances to deal with,
///                            to limit the checking so as to apply to just that many
///                            (default is SPAN_LIMIT, currently set to 80) use -1 if no
///                            limit is wanted & so all are to be checked. If the
///                            array does not have the limit amount of instances, all are
///                            checked.
/// \remarks
/// The returned array, strArray, contains all of the unique individual words which are in
/// both the old passed in uniqueKeysStr and in the source text keys (with mergers reduced
/// to the individual words for storage in strArray) - collected from the first (or all)
/// of the words in arrNew's CSourcePhrase instances -- exactly how many are used depends
/// on the 4th param, limit, which if absent defaults to SPAN_LIMIT = 80 (CSourcePhrase
/// instances) - but the whole arr if limit is passed in as -1, or if the arr count is
/// less than the limit value.
///
/// Note 1: Some CSourcePhrase instances encountered may be mergers, and so will contain
/// two or more words. We have to break these into individual word tokens and check for
/// duplicates on each such token. We ignore ellipses (...) for placeholders - and there
/// shouldn't be any in the arr array anyway, since what is in arr will be the
/// just-tokenized CSourcePhrase instances arising from calling TokenizeText() on the
/// just-edited (outside of Adapt It) source text USFM plain text string.
///
/// Note 2: Beware, empty subspans within a Subspan instance will manifest as starting and
/// ending index pairs with values (-1,-1), so protect from using these as real index values
////////////////////////////////////////////////////////////////////////////////////////
int GetWordsInCommon(SPArray& arrNew, Subspan* pSubspan, wxString& uniqueKeysStr, wxArrayString& strArray, int limit)
{
	strArray.Clear();
	int count = arrNew.GetCount();
	int activeSpanCount;
	int nStartAt;
	int howMany;
	int index;
	wxString space = _T(' ');
	wxString ellipsis = _T("...");
	wxArrayString arrMerged;
	long mergeCount = 0;

	if (pSubspan->bClosedEnd)
	{
		// we must not access more content than up to and including the newEndPos
		// index
		nStartAt = pSubspan->newStartPos;
		if (nStartAt == -1)
		{
			// the arrNew subspan is empty - so no words are in common
			return 0;
		}
		activeSpanCount = pSubspan->newEndPos - nStartAt + 1;
		if (activeSpanCount == 0)
			return 0;
		howMany = activeSpanCount;
	}
	else
	{
        // how many CSourcePhrase instandes we access is limited only by the limit value,
        // and if the latter is -1, it is not limited at all
		nStartAt = pSubspan->newStartPos;
		if (nStartAt == -1)
		{
			// the arrNew subspan is empty - so no words are in common
			return 0;
		}
		activeSpanCount = count - nStartAt; // a default (max) value
		if (activeSpanCount == 0)
			return 0;
        // limit the span of CSourcePhrase instances to be checked to the limit value:
        // possible values passed in are -1, or SPAN_LIMIT (ie. 80), or an explicit
        // value. If limit is -1 or those remaining from nStartAt onwards is positive
        // but fewer than limit remain from nStartAt onwards, then just use the
        // remainder value instead; if SPAN_LIMIT or an explicit value is passed in,
        // use it - but shorten it appropriately if the value would otherwise result in
        // a bounds error.
		if (limit == -1 || (activeSpanCount > 0 && activeSpanCount < limit))
		{
			howMany = activeSpanCount;
		}
		else
		{
			howMany = limit;
		}
	}
	if (nStartAt > count - 1)
		return 0; // otherwise there would be a bounds error

	int aSrcPhraseCounter = 0;
	index = nStartAt;
	while (aSrcPhraseCounter < howMany)
	{
		CSourcePhrase* pSrcPhrase = arrNew.Item(index);
		wxASSERT(pSrcPhrase != NULL);
		wxString aKey = pSrcPhrase->m_key;
		index++;
		aSrcPhraseCounter++;
		if (aKey == ellipsis)
			continue; // skip over any ... ellipses (placeholders)
		if (pSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger or a fixedspace pseudo-merger -- so get the word tokens
			// and deal with each individually
			wxString aWordToken;
			arrMerged.Clear();
			bool bHasFixedSpace = IsFixedSpaceSymbolWithin(pSrcPhrase);
			if (bHasFixedSpace)
			{
				// it's a pair of conjoined (by fixed space, ~ symbol) CSourcePhrase
				// instances
				SPList::Node* pos = pSrcPhrase->m_pSavedWords->GetFirst();
				int indx;
				for (indx = 0; indx < 2; indx++)
				{
					CSourcePhrase* pSP = pos->GetData();
					pos = pos->GetNext();
					aWordToken = pSP->m_key;
					int offset = uniqueKeysStr.Find(aWordToken);
					if (offset != wxNOT_FOUND)
					{
                        // it's a candidate for inclusion in strArray, but we must check
                        // first that it is not already present in the array, and only if
                        // it isn't do we add it
						int indexMatched = strArray.Index(aWordToken);
						if (indexMatched == wxNOT_FOUND)
						{
							strArray.Add(aWordToken);
						}
					}
				}
			}
			else
			{
				mergeCount = SmartTokenize(space, aKey, arrMerged, FALSE); // FALSE means ignore empty strings
				if (mergeCount > 0)
				{
					int indx;
					for (indx = 0; indx < (int)mergeCount; indx++)
					{
						aWordToken = arrMerged.Item(indx);
						int offset = uniqueKeysStr.Find(aWordToken);
						if (offset != wxNOT_FOUND)
						{
							// it's a candidate for inclusion in strArray, but we must check
							// first that it is not already present in the array, and only if
							// it isn't do we add it
							int indexMatched = strArray.Index(aWordToken);
							if (indexMatched == wxNOT_FOUND)
							{
								strArray.Add(aWordToken);
							}
						}
					}
				}
			}
		}
		else
		{
			// it's not a merger
			int offset = uniqueKeysStr.Find(aKey);
			if (offset != wxNOT_FOUND)
			{
				// it's a candidate for inclusion in strArray, but we must check
				// first that it is not already present in the array, and only if
				// it isn't do we add it
				int indexMatched = strArray.Index(aKey);
				if (indexMatched == wxNOT_FOUND)
				{
					strArray.Add(aKey);
				}
			}
		}
	} // end of while loop for scanning a span (or all if limit == -1)
	return strArray.GetCount();
}

// overloaded version, which takes both arrays as params etc; returns a count of the
// number of words which are in common; they are stored in order of occurrence in strArray
// which returns them to the caller (pSubspan may have empty subspan with (-1,-1) values,
// but each of the called functions tests for this and if one such happens, zero is
// returned because nothing can be in common with an empty span)
int	GetWordsInCommon(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, wxArrayString& strArray, int limit)
{
	strArray.Clear();
    // turn the array of old CSourcePhrase instances into a fast access string made up of
    // the unique m_key words (note, if the instances contain mergers, phrases will be
    // obtained, but that's fine - they will have spaces between the words as required
    // here)
    wxString oldUniqueSrcWords;
	// there will be no placeholder ellipses in the returned string oldUniqueSrcWords
	int oldUniqueWordsCount = GetUniqueOldSrcKeysAsAString(arrOld, pSubspan, oldUniqueSrcWords, limit);
	// the return value can be zero of course, but it would be rare (e.g. a single source
	// text's CSourcePhrase instance is in the arrOld's subspan here, and that
	// CSourcePhrase instance just happens to be a placeholder)
#if defined(_DEBUG) && defined(myLogDebugCalls)
	if (oldUniqueWordsCount > 0)
		wxLogDebug(_T("oldUniqueWordsCount = %d ,  oldUniqueSrcWords:  %s"), oldUniqueWordsCount, oldUniqueSrcWords.c_str());
	else
		wxLogDebug(_T("oldUniqueWordsCount = 0 ,  oldUniqueSrcWords:  <empty string>"));
#endif
	if (oldUniqueWordsCount == 0)
		return 0;
	// compare the initial words from the arrNew array of CSourcePhrase instances with
	// those in oldUniqueSrcWords
	int commonsCount = GetWordsInCommon(arrNew, pSubspan, oldUniqueSrcWords, strArray, limit);
#if defined(_DEBUG) && defined(myLogDebugCalls)
/*
	if (commonsCount > 0)
	{
		int i = 0;
		wxString word[12];
		int nLines = commonsCount / 12;
		if (commonsCount % 12 > 0)
			nLines++;
		wxLogDebug(_T("  commonsCount = %d     Words in common:"), commonsCount);
		int index = 0;
		for (i = 0; i < nLines; i++)
		{
			for (index = 0; index < 12; index++)
			{
				int index2 = i*12 + index;
				if (index2 < commonsCount)
				{
					word[index] = strArray.Item(index2);
				}
				else
				{
					word[index] = _T(' ');
				}
			}
			wxLogDebug(_T("   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s"),
				word[0].c_str(), word[1].c_str(), word[2].c_str(), word[3].c_str(), word[4].c_str(),
				word[5].c_str(), word[6].c_str(), word[7].c_str(), word[8].c_str(), word[9].c_str(),
				word[10].c_str(), word[11].c_str());
		}
	}
	else
	{
		wxLogDebug(_T("  commonsCount = 0     Words in common:  <empty string>"));
	}
*/
#endif

	return commonsCount;
}

void InitializeSubspan(Subspan* pSubspan, SubspanType spanType, int oldStartPos,
						int oldEndPos, int newStartPos, int newEndPos, bool bClosedEnd)
{
	pSubspan->childSubspans[0] = NULL;
	pSubspan->childSubspans[1] = NULL;
	pSubspan->childSubspans[2] = NULL;
	pSubspan->oldStartPos = oldStartPos;
	pSubspan->newStartPos = newStartPos;
	pSubspan->oldEndPos = oldEndPos;
	pSubspan->newEndPos = newEndPos;
	pSubspan->spanType = spanType;
	pSubspan->bClosedEnd = bClosedEnd; // caller must always set this
		// explicitly FALSE for any rightmost one which abuts the
		// as-yet-unprocessed lying-to-the-right CSourcePhrase instances
		// in the array pair (the indices for the abuttal location won't
		// necessarily be identical in arrNew & arrOld)
}

/////////////////////////////////////////////////////////////////////////////////////////////
/// \return             the index at or after startFrom at which the matched CSourcePhrase is
///                     found; or wxNOT_FOUND if no match was made
/// \param  word    ->  the word (no punctuation either end) which is searched for in the
///                     arr containing CSourcePhrase instances to find one with a matching m_key
///                     containing it
/// \param  arr     ->  the array of CSourcePhrase instances being searched, from
///                     startFrom index and onwards
/// \param  startFrom -> index in arr at which to start looking for a match
/// \param  endAt   ->  index (inclusive) at which to cease looking for a match
/// \param  phrase  <-  when the m_key matched contains only a single word, phrase is
///                     returned empty; but when m_key contains more than one word (any of
///                     them could have therefore been matched), the whole m_key phrase is
///                     returned here
/// \remarks
/// Get the index for whichever CSourcePhrase instance in arr contains the searched for
/// word, starting the search from the index value startFrom, and searching as far as
/// endAt (inclusing that index as a possible match location). Return the whole m_key if the
/// latter is within a matched CSourcePhrase which is a merger, returning it in the phrase
/// parameter ( so that the caller can check if the phrase occurs in the other array).
/// We have startFrom and endAt bounds because we want to limit our algorithms to a short
/// span in which we have a certainty of there being at least one word in common - the
/// default value of SPAN_LIMIT (set to 80) is suitably large without generating heaps of
/// possibilities which all have to be checked.
/////////////////////////////////////////////////////////////////////////////////////////////
int FindNextInArray(wxString& word, SPArray& arr, int startFrom, int endAt, wxString& phrase)
{
	int i;
	for (i = startFrom; i <= endAt; i++)
	{
		CSourcePhrase* pSrcPhrase = arr.Item(i);
		if (pSrcPhrase->m_nSrcWords == 1)
		{
			// make sure all the word matches, not just part of it
			if (pSrcPhrase->m_key == word)
			{
				phrase.Empty();
				return i;
			}
		}
		else
		{
			// must be a merger, or a fixedspsace pseudo-merger
			phrase = pSrcPhrase->m_key;
			if (IsFixedSpaceSymbolWithin(phrase))
			{
				int offset2 = phrase.Find(_T("~"));
				wxString word1 = phrase.Left(offset2);
				wxString word2 = phrase.Mid(offset2 + 1);
				if (word == word1)
				{
					return i;
				}
				if (word == word2)
				{
					return i;
				}
			}
			else
			{
				// it's a normal merger - get a list of the words in the merger and see if
				// word matches any of them
				wxArrayString arrStr;
				wxString delim = _T(' ');
				long aCount = SmartTokenize(delim, pSrcPhrase->m_key, arrStr, FALSE);
				int index;
				for (index = 0; index < (int)aCount; index++)
				{
					if (word == arrStr.Item(index))
					{
						return i;
					}
				}
			}
		}
	}
	// no match made
	phrase.Empty();
	return wxNOT_FOUND;
}

// Empties the array without deleting any of the objects pointed at (the ptrs to these are
// copies anyway, so we don't want their memory freed) The wxWidgets documentation
// suggests that the pointed at CSourcePhrase instances would be deleted by the Empty()
// call, but testing reveals it isn't the case.
void RemoveAll(SPArray* pSPArray)
{
	pSPArray->Empty();
}

void MergeUpdatedSourceText(SPList& oldList, SPList& newList, SPList* pMergedList, int limit)
{
	// turn the lists into arrays of CSourcePhrase*; note, we are using arrays to manage
	// the same pointers as the SPLists do, so don't in this function try to delete any of
	// the oldList or newList contents, nor what's in the arrays
	SPArray arrOld;
	SPArray arrNew;
	ConvertSPList2SPArray(&oldList, &arrOld);
	int oldSPCount = oldList.GetCount();
	if (oldSPCount == 0)
		return;
	ConvertSPList2SPArray(&newList, &arrNew);
	int newSPCount = newList.GetCount();
	if (newSPCount == 0)
		return;

	// do the merger of the two arrays
	MergeUpdatedSrcTextCore(arrOld, arrNew, pMergedList, limit);
}

// This is the guts of the recursive merging algorithm - it relies on the limit value
// being as large as or larger than the biggest group of new CSourcePhrase instances added
// by the user's editing to any one place in the old (probably exported) source text; if
// that condition is violated, it will not return all the data. I use a value of 80 for
// limit, but for potentially large blocks of new material, -1 should be used & be
// prepared for things to slow down! It's an N squared algorithm.
void MergeUpdatedSrcTextCore(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList, int limit)
{
	int nStartingSequNum;
	int oldSPCount = arrOld.GetCount();
	if (oldSPCount == 0)
		return;
	int newSPCount = arrNew.GetCount();
	if (newSPCount == 0)
		return;
	bool bAreDispirateSizedInventories = AreInventoriesDisparate(oldSPCount, newSPCount);

#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("\n\nMergeUpdatedSrcTextCore() passed in: arrOld CSourcePhrase count = %d   arrNew CSourcePhrase count = %d"),
		oldSPCount, newSPCount);
#endif
	int defaultLimit = limit; // this is the default miminum value for the limit parameter;
			// internally, for big chunks added, a small value is dangerous, so we'll use
			// bigger values in certain situations so as to be sure we span the material
			// with the limit value; but for subspans of matched verses, the default value
			// will suffice
	int dynamicLimit = defaultLimit; // use LHS as our variable limit value, initialize
									 // to the SPAN_LIMIT value (of 80)
    // Note: we impose a limit on maximum span size, to keep our algorithms from getting
    // bogged down by having to handle too much data in any one iteration. The limit
    // parameter specifies what to do.
    // If limit is -1 then bogging down potential is to be ignored, and we'll always
    // take the largest span possible. If limit is not explicitly specified in the call,
    // then default to SPAN_LIMIT (currently set in AdaptitConstants.h to 80 -- big enough
    // to embrace an average verse's amount of words); if some explicit value is given, use
	// that or a suitably smaller value if close to an array end.
	//
    // However, when defining an embedded tuple, the beforeSpan will end at the start of
    // the commonSpan, the commonSpan will end at the start of the afterSpan, and the
    // afterSpan must ALWAYS be set to end at the end of oldSPArray for the old data, and
    // newSPArray for the new data - even if the latter index values are thousands bigger
    // than SPAN_LIMIT, because recursion will cut up such long spans into shorter Subspan
    // pieces, until ultimately the whole of both oldSPArray and newSPArray have a cover
    // defined over them - thereby processing the whole input data (the two arrays) to
    // define the single merged output array. Recursion processor function proceeds across
    // a tuple from left to right, processing each Subspan (the central commonSpan is
    // terminal and never therefore needs to have a child tuple defined for it), and
    // whenever a child tuple is generated within the beforeSpan or afterSpan, the
    // RecusiveTupleProcessor() function must be called on that tuple immediately. When the
    // initial call of the function ultimately finishes, all the data will have been
    // processed. After the terminal nodes have been processed, the parent deletes the just
    // finished node - so tuple deletion is involved automatically.

	InitializeUsfmMkrs();
	nStartingSequNum = (arrOld.Item(0))->m_nSequNumber; // store this, to get the
									// sequence numbers right at the end of the process

    // Analyse the arrOld and arrNew arrays in order to chunk the data appropriately...
    // remember to delete these local wxArrayPtrVoid arrays' contents from the heap before
    // returning, etc
    bool bHandleDispirateSizedLastOldChunk = FALSE; // BEW added 25Oct12
    bool bSuccessful_Old;
	bool bSuccessful_New;
	wxArrayPtrVoid* pChunksOld = new wxArrayPtrVoid;
	wxArrayPtrVoid* pChunksNew = new wxArrayPtrVoid;
	int oldCountOfChapters = 0;
	int oldCountOfVerses = 0;
	int newCountOfChapters = 0;
	int newCountOfVerses = 0;
	// setup the SfmChunk arrays
	bSuccessful_Old =  AnalyseSPArrayChunks(&arrOld, pChunksOld, oldCountOfChapters, oldCountOfVerses); // analyse arrOld
	bSuccessful_New =  AnalyseSPArrayChunks(&arrNew, pChunksNew, newCountOfChapters, newCountOfVerses); // analyse arrNew
	int countOldChunks = pChunksOld->GetCount();
	int countNewChunks = pChunksNew->GetCount();
	int oldMaxIndex = countOldChunks - 1;
	int newMaxIndex = countNewChunks - 1;
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("MergeUpdatedSrcTextCore()  countOldChunks = %d   countNewChunks = %d"),
		countOldChunks, countNewChunks);
#endif
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("MergeUpdatedSrcTextCore()  oldCountOfChapters = %d   oldCountOfVerses = %d  newCountOfChapters = %d  newCountOfVerses = %d"),
			oldCountOfChapters, oldCountOfVerses,  newCountOfChapters,  newCountOfVerses);
#endif
	bool bDispirateSizedChunkInventories = AreInventoriesDisparate(countOldChunks, countNewChunks);
	bool bDispirateSizedChaptersInventory = AreInventoriesDisparate(oldCountOfChapters, newCountOfChapters);
	bool bDispirateSizedVersesInventory = AreInventoriesDisparate(oldCountOfVerses, newCountOfVerses);

    // BEW added 17Aug12, these will hold subranges of matched pairs of subarrays of the
    // contents of arrOld and arrNew, which we populated based on our analysis of the
    // arrOld and arrNew arrays done just above, to support remembering individual SfmChunk
    // pairings. To values from the one arrPairingOld array we added 'kick off' offsets -
    // one for the arrOld array, the other for the arrNew array, for a given matched
    // milestone pair. So the one index value yields from each array the SfmChunk index
    // which constitutes a successfully matched milestoned pair within the
    // GetMaxInSyncChunksPairing() function; MergeRecursively() uses these arrays to
    // determine spans of old and new CSourcePhrase instances in paired spans, for
    // recursive merging on a per-milestone basis, rather than merging all the milestones
    // as a superchunk from start to end (we do this because the latter is dangerous for a
    // SPAN_LIMIT value of 80 when the user may add hundreds of new words at the end of
    // source text in Paratext - only a loop doing per-milestone recursive mergers is safe
    // in such a circumstance - otherwise data at the end of the supergroup could be lost)
	SPArray subArrOld;
	SPArray subArrNew;
	wxArrayInt arrPairingOld; // only need one such, index differences for what's in subArrOld
							  // and what's in subArrNew can be accomodated for by different
							  // offsets, so I've removed arrPairingNew
	//wxArrayInt arrPairingNew;

	// get a wxLogDebug() display of the chunks and their types and ranges
#if defined(_DEBUG) && defined( myMilestoneDebugCalls)
	wxString unStr = _T("unknownChunkType");
	wxString biStr = _T("bookInitialChunk");
	wxString inStr = _T("introductionChunk");
	wxString preFChStr = _T("preFirstChapterChunk");
	wxString cvStr = _T("chapterPlusVerseChunk");
	wxString svStr = _T("subheadingPlusVerseChunk");
	wxString vsStr = _T("verseChunk");
	int count = pChunksOld->GetCount();
	int index;
	int counter = -1;
	SfmChunk* pChunk = NULL;
	wxLogDebug(_T("\n\n****  OLD array: SfmChunk instances  ****"));
	for (index = 0; index < count; index++)
	{
		pChunk = (SfmChunk*)pChunksOld->Item(index);
		counter++;
		wxString typeStr;
		switch (pChunk->type)
		{
		case unknownChunkType:
			typeStr = unStr;
			break;
		case bookInitialChunk:
			typeStr = biStr;
			break;
		case introductionChunk:
			typeStr = inStr;
			break;
		case preFirstChapterChunk:
			typeStr = preFChStr;
			break;
		case chapterPlusVerseChunk:
			typeStr = cvStr;
			break;
		case subheadingPlusVerseChunk:
			typeStr = svStr;
			break;
		case verseChunk:
			typeStr = vsStr;
			break;
		}
		wxLogDebug(_T("  %d  type: %s  [ start , end ] =  [ %d , %d ]   bContainsText: %d   chapter:  %s  verse_start:  %s  verse_end  %s"),
		counter, typeStr.c_str(), pChunk->startsAt, pChunk->endsAt, (int)pChunk->bContainsText,
		pChunk->strChapter.c_str(), pChunk->strStartingVerse.c_str(), pChunk->strEndingVerse.c_str());
	}
	wxLogDebug(_T("\n\n****  NEW array: SfmChunk instances  ****"));
	count = pChunksNew->GetCount();
	counter = -1;
	for (index = 0; index < count; index++)
	{
		pChunk = (SfmChunk*)pChunksNew->Item(index);
		counter++;
		wxString typeStr;
		switch (pChunk->type)
		{
		case unknownChunkType:
			typeStr = unStr;
			break;
		case bookInitialChunk:
			typeStr = biStr;
			break;
		case introductionChunk:
			typeStr = inStr;
			break;
		case preFirstChapterChunk:
			typeStr = preFChStr;
			break;
		case chapterPlusVerseChunk:
			typeStr = cvStr;
			break;
		case subheadingPlusVerseChunk:
			typeStr = svStr;
			break;
		case verseChunk:
			typeStr = vsStr;
			break;
		}
		wxLogDebug(_T("  %d  type: %s  [ start , end ] =  [ %d , %d ]   bContainsText: %d   chapter:  %s  verse_start:  %s  verse_end  %s"),
		counter, typeStr.c_str(), pChunk->startsAt, pChunk->endsAt, (int)pChunk->bContainsText,
		pChunk->strChapter.c_str(), pChunk->strStartingVerse.c_str(), pChunk->strEndingVerse.c_str());
	}
#endif
	// BEW 25Oct12, augmented the tests for entry to this block, to cover difficult
	// situations robustly...
    // If there was no SFM or USFM data, need a limit = -1 merger, plus a progress bar In
    // fact, there are quite a few conditions where just merging all the old stuff with all
    // the new stuff is best done as two large superchunks -- slower, but it gets
    // everything right. Usually these situations are when the new data is approximately
    // the same as the old data (that is, the file sizes are not dispirate), but chunking
    // goes differently for some reason, such as:
	// a) no \c nor \v markers are in the old data, but the user has edited them in to the
	// new data;
	// b) the old data has only partial markup, so that \c and/or \v markers are
	// significantly fewer in the old data than in the new data (or vise versa).
    // There is a very small risk however, if the old data had extra material, say some
    // adapted introduction which isn't in the new material, then that block of adapted
    // introduction material would be lost. This is a pretty unlikely scenario however,
    // because intro material is typically only put in after verses and chapters are marked
    // up - and in that circumstance we'd not be processing the data as two large
    // superchunks.
	if (	(!bSuccessful_Old && !bSuccessful_New) || // no \c or \v markers in either, OR
			(!bAreDispirateSizedInventories && bDispirateSizedChunkInventories) || // similar CSourcePhrase inventories
																				   // but dissimilar sized chunk inventories
            (!bAreDispirateSizedInventories && // not dissimilar in their inventories of CSourcePhrase instances, AND
			(bDispirateSizedVersesInventory || bDispirateSizedChaptersInventory))  // are dissimilar in either \v
		)																		   // inventories or \c inventories
	{
        // Do a "two superchunks" merger. This will be inefficient and slow because
        // all unique words in arrOld must be checked for all matchups with all unique
        // words in arrNew, an order of N squared operation, where N = total words (but if
        // one or the other array is markedly shorter, it isn't so bad)
		// (When processStartToEnd is the enum value, rather than processPerMilestone, all
		// param after the nStartingSequNum param are ignored.)
        int oldChunkIndex = -1;
		int newChunkIndex = -1;
		MergeRecursively(arrOld, arrNew, pMergedList, -1, nStartingSequNum,
				pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
				arrOld, arrNew, oldChunkIndex, newChunkIndex);
		// Updating m_nSequNumber values is done internally

        // we are done with the temporary array's of SfmChunk instances, so remove them
        // from the heap; there shouldn't be any, but just in case...
		DeleteSfmChunkArray(pChunksOld);
		DeleteSfmChunkArray(pChunksNew);
		return;
	} // end of TRUE block for test: if (!bSuccessful_Old && !bSuccessful_New)

	// when both don't fail, there are SFMs or USFMs in the data, and so we must scan
	// through pChunksOld and pChunksNew, looking for stretches where the milestones are
	// in sync, and for where there is some sort of dislocation. For the former we call
	// MergeRecursively() with a small limit value (equal to SPAN_LIMIT, currently 80),
	// but for the dislocation parts (could be a mismatched gap, mismatched verse range,
	// etc) we work out the maximum number of words involved and call MergeRecursively()
	// with a limit value equal to that number of words, just on those paired subspans. We
	// proceed with this left to right processing until all the inputs are merged - that
	// is, arrOld and arrNew.

	// start with bookInitialChunk, if present
	SfmChunk* pOldChunk = NULL; // a scratch variable
	SfmChunk* pNewChunk = NULL; // a scratch variable
	int oldChunkIndex = 0;
	int newChunkIndex = 0;
	int oldLastChunkIndex = wxNOT_FOUND; // index in pChunksOld of last processed SfmChunk
	int newLastChunkIndex = wxNOT_FOUND; // index in pChunksNew of last processed SfmChunk

	if (countOldChunks > 0 && countNewChunks > 0)
	{
		// we have chunks that potentially can be matched up to each other
		pOldChunk = (SfmChunk*)pChunksOld->Item(oldChunkIndex);
		pNewChunk = (SfmChunk*)pChunksNew->Item(newChunkIndex);
		if (pOldChunk->type == bookInitialChunk && pNewChunk->type == bookInitialChunk)
		{
			// both have bookInitialChunk, so merge these with the default value of limit
			CopySubArray(arrOld, pOldChunk->startsAt, pOldChunk->endsAt, subArrOld);
			CopySubArray(arrNew, pNewChunk->startsAt, pNewChunk->endsAt, subArrNew);
			// MergeRecursively() appends to pMergedList and updates sequ numbers, and
			// copies changed punctuation and/or USFMs for material "in common", etc
			MergeRecursively(subArrOld, subArrNew, pMergedList, defaultLimit, nStartingSequNum,
				pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
				arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() 2 book-initial chunks: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pOldChunk->endsAt, pNewChunk->startsAt, pNewChunk->endsAt, defaultLimit, nStartingSequNum);
#endif
			RemoveAll(&subArrOld);
			RemoveAll(&subArrNew);
			oldLastChunkIndex = oldChunkIndex;
			newLastChunkIndex = newChunkIndex;
		}
		else
		{
			// If pChunksOld has no bookInitialChunk, and pChunksNew does, the simply copy
			// the CSourcePhrase instances from the arrNew's bookInitialChunk to
			// pMergedList. On the other hand, if the other way round, then the user wants
			// the bookInitialChunk in arrOld to be removed, so simply refrain from
			// copying it to pMergedList; if both have no such chunk, do nothing
			if (pNewChunk->type == bookInitialChunk)
			{
				// copy to pMergedList, no recursive merge is required
				CopyToList(arrNew, pNewChunk->startsAt, pNewChunk->endsAt, pMergedList);
				newLastChunkIndex = newChunkIndex;
				oldLastChunkIndex = wxNOT_FOUND; // -1

				// update sequence numbers using initialSequNum
				if (!pMergedList->IsEmpty())
				{
					// when MergeRecursively() isn't used, we need to force the update;
					// when it is use, it is done within that function call, on the whole
					// pMergedList, always using the nStartingSequNum value to start from
					gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
				}
			} // the other two options can be safely ignored (ie. neither array has
			  // bookInitialChunk or only arrOld has it)
		} // end of else block for test:
		  // if (pOldChunk->type == bookInitialChunk && pNewChunk->type == bookInitialChunk)

		// next, handle any introduction information, if present
		oldChunkIndex = oldLastChunkIndex + 1;
		newChunkIndex = newLastChunkIndex + 1;
		if (oldChunkIndex < countOldChunks && newChunkIndex < countNewChunks)
		{
			// we still have chunks that potentially can be matched up to each other
			pOldChunk = (SfmChunk*)pChunksOld->Item(oldChunkIndex);
			pNewChunk = (SfmChunk*)pChunksNew->Item(newChunkIndex);
			if (pOldChunk->type == introductionChunk && pNewChunk->type == introductionChunk)
			{
				// both arrays have introductionChunk, and this is not verse-milestoned,
				// so there may be removal or addition of a very large amount of
				// introduction data - so we have to use a limit value based on the length
				// of the subarrays -- this is accomplished by passing in -1 for limit
				CopySubArray(arrOld, pOldChunk->startsAt, pOldChunk->endsAt, subArrOld);
				CopySubArray(arrNew, pNewChunk->startsAt, pNewChunk->endsAt, subArrNew);
				// MergeRecursively() appends to pMergedList and updates sequ numbers, and
				// copies changed punctuation and/or USFMs for material "in common", etc
				MergeRecursively(subArrOld, subArrNew, pMergedList, -1, nStartingSequNum,
						pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() 2 introduction chunks: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pOldChunk->endsAt, pNewChunk->startsAt, pNewChunk->endsAt, -1, nStartingSequNum);
#endif
				RemoveAll(&subArrOld);
				RemoveAll(&subArrNew);
				oldLastChunkIndex = oldChunkIndex;
				newLastChunkIndex = newChunkIndex;
			}
			else
			{
				// one of, or both of, the array's introductionChunk instances are absent;
				// if arrOld's is absent, but arrNew's is not, then just copy the
				// introduction material straight to pMergedList; if arrNew's is absent,
				// but arrOld's is not, then just ignore the introductionChunk in arrOld
				// because it isn't any longer wanted; if both are absent, do nothing
				// (except get the oldLastChunkIndex and newLastChunkIndex set ready for
				// the next test - which should be milestoned material)
				if (pNewChunk->type == introductionChunk)
				{
					// copy to pMergedList, no recursive merge is required
					CopyToList(arrNew, pNewChunk->startsAt, pNewChunk->endsAt, pMergedList);
					newLastChunkIndex = newChunkIndex;
					// oldLastChunkIndex hasn't changed, so don't reset it

					// update sequence numbers using initialSequNum
					if (!pMergedList->IsEmpty())
					{
						// when MergeRecursively() isn't used, we need to force the update;
						// when it is use, it is done within that function call, on the whole
						// pMergedList, always using the nStartingSequNum value to start from
						gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
					}
				} // the other two options can be safely ignored (ie. neither array has
				  // introductionChunk or only arrOld has it)
			}
		} // end of TRUE block for test:
		  // if (oldChunkIndex < countOldChunks && newChunkIndex < countNewChunks)

		// next, handle any preFirstChapterChunk information, if present
		oldChunkIndex = oldLastChunkIndex + 1;
		newChunkIndex = newLastChunkIndex + 1;
		if (oldChunkIndex < countOldChunks && newChunkIndex < countNewChunks)
		{
			// we still have chunks that potentially can be matched up to each other
			pOldChunk = (SfmChunk*)pChunksOld->Item(oldChunkIndex);
			pNewChunk = (SfmChunk*)pChunksNew->Item(newChunkIndex);
			if (pOldChunk->type == preFirstChapterChunk && pNewChunk->type == preFirstChapterChunk)
			{
				// both arrays have a preFirstChapterChunk, and this is not verse-milestoned,
				// so there may be removal or addition of a very large amount of
				// introduction data - so we have to use a limit value based on the length
				// of the subarrays -- this is accomplished by passing in -1 for limit
				CopySubArray(arrOld, pOldChunk->startsAt, pOldChunk->endsAt, subArrOld);
				CopySubArray(arrNew, pNewChunk->startsAt, pNewChunk->endsAt, subArrNew);
				// MergeRecursively() appends to pMergedList and updates sequ numbers, and
				// copies changed punctuation and/or USFMs for material "in common", etc
				MergeRecursively(subArrOld, subArrNew, pMergedList, -1, nStartingSequNum,
						pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() a preFirstChapterChunk: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pOldChunk->endsAt, pNewChunk->startsAt, pNewChunk->endsAt, -1, nStartingSequNum);
#endif
				RemoveAll(&subArrOld);
				RemoveAll(&subArrNew);
				oldLastChunkIndex = oldChunkIndex;
				newLastChunkIndex = newChunkIndex;
			}
			else
			{
                // One of, or both of, the array's preFirstChapterChunk instances are
                // absent. If arrOld's is absent, but arrNew's is present, then just copy
                // the introduction material straight to pMergedList. If arrNew's is
                // absent, but arrOld's is present, then try not to loose the arrOld
                // material unless it really should be removed. It the new chunk is a
                // chapter and verse chunk, the it's likely that the user added a missing
                // chapter marker when doing the external edit, and so we want to keep any
                // subheading adaptations etc. The way to do this is to do no merge here,
                // but instead take the next "old" chunk and give it the present old
                // chunk's starting index number, so that a later matchup will incorporate
                // the material which can't be handled here. For any other situatin, assume
                // this preliminary material is no longer wanted. So do nothing (except get
                // the oldLastChunkIndex and newLastChunkIndex set ready for the next test
                // - which should be milestoned material)
				if (pNewChunk->type == preFirstChapterChunk)
				{
					// copy to pMergedList, no recursive merge is required
					CopyToList(arrNew, pNewChunk->startsAt, pNewChunk->endsAt, pMergedList);
					newLastChunkIndex = newChunkIndex;
					// oldLastChunkIndex hasn't changed, so don't reset it

					// update sequence numbers using initialSequNum
					if (!pMergedList->IsEmpty())
					{
						// when MergeRecursively() isn't used, we need to force the update;
						// when it is use, it is done within that function call, on the whole
						// pMergedList, always using the nStartingSequNum value to start from
						gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
					}
				}
				else if ( oldChunkIndex < oldMaxIndex && (
					(pNewChunk->type == chapterPlusVerseChunk) ||
					(pNewChunk->type == subheadingPlusVerseChunk) ||
					(pNewChunk->type == verseChunk)))
				{
					SfmChunk* pNextOldChunk = (SfmChunk*)pChunksOld->Item(oldChunkIndex + 1);
					pNextOldChunk->startsAt = pOldChunk->startsAt;
				}
				// the other option can be safely ignored (ie. neither array has
				// preFirstChapterChunk)
			}
		} // end of TRUE block for test:
		  // if (oldChunkIndex < countOldChunks && newChunkIndex < countNewChunks)

		// next, handle any milestoned material - this is done in a loop
		oldChunkIndex = oldLastChunkIndex + 1;
		newChunkIndex = newLastChunkIndex + 1;
		SfmChunk* pEndChunkOld = NULL;
		SfmChunk* pEndChunkNew = NULL;
		bool bInitialRefsSame =  TRUE;
		bool bPairedOK = FALSE;
		bool bDisparateSizes = FALSE; // gets set TRUE if a matched pair of chunks differ
									  // in word counts by more than a factor of 1.5
		bool bDoingToTheEnd = FALSE; // we can come to a messy non-pairing section
                  // with bGotALaterPairing (see code near loop end) either TRUE or FALSE
                  // -- if the latter is TRUE then there are pairings after the messy bit;
                  // if FALSE, then the messy bit extends to the doc ends in both old and
                  // new arrays - and in this latter case, we'll want to break from the
                  // loop after we have done the merger. However, there is some
                  // safety-first code after the loop that ensures that any unprocessed
                  // final new material doesn't get omitted (if it needed to be copied then
                  // it would be copied to the end of the merged list) - but since we can
                  // come to that code from two prior states, we must suppress such copying
                  // if the messy bit merger was done to the very end of the arrays already
                  // (otherwise we'd copy that final data twice). The bDoingToTheEnd flag
                  // we will set TRUE if bFinished was FALSE and bGodALaterPairing was
                  // FALSE, that will get us safely out of the loop with the help of a few
                  // carefully placed tests at the loop end below.
		while(oldChunkIndex < countOldChunks && newChunkIndex < countNewChunks)
		{
/*
#if defined(_DEBUG) && defined(LOOPINDEX)
			wxLogDebug(_T("while loop indices: oldChunkIndex %d   newChunkIndex %d"),oldChunkIndex,newChunkIndex);
			if (newChunkIndex >= 22)
			{
				int break_here = 1;
			}
#endif
*/
			// we still have chunks that potentially can be matched up to each other
			pOldChunk = (SfmChunk*)pChunksOld->Item(oldChunkIndex);
			pNewChunk = (SfmChunk*)pChunksNew->Item(newChunkIndex);
			bInitialRefsSame = AreSfmChunksWithSameRef(pOldChunk, pNewChunk);
			bPairedOK = FALSE; // initialize
			if (bInitialRefsSame)
			{
                // Collect successive paired chunks into a superchunk, and process the
                // superchunk with a single MergeRecursively() call, with limit set to
                // defaultLimit ( = SPAN_LIMIT = 80, see AdaptitConstants.h); FALSE is
                // returned if a pairing failure occurs prior to reaching the end of one or
                // both of the arrays, or a pairing of two chunks is done but the word
                // counts within those two chunks are disparate; if pairing gets to the end
                // of one or both arrays, it returns TRUE
				bPairedOK = GetMaxInSyncChunksPairing(arrOld, arrNew, pChunksOld, pChunksNew,
								oldChunkIndex, newChunkIndex, oldLastChunkIndex,
								newLastChunkIndex, bDisparateSizes, arrPairingOld);
			} // end of TRUE block for test: if (bInitialRefsSame)

			// BEW added 25Oct12. This block is to handle the situation when the old source
			// text has only negligable or partial USFM marker content, and the user has
			// exported the source text, added fuller or full USFM content, and then
			// imported the marker-rich edited source text back into the document. We want
			// this process to preserve many of the original adaptations (and
			// placeholders) as possible, since it's likely that only markers, or mainly
			// markers, are the sum total of the external editing changes. In this
			// scenario, it's likely that the final chunk of the old text will be very
			// large - being the span from where partial earlier markup finished to the
			// end of the earlier form of the document; and the matching chunk may be very
			// small, typically just a verse. The legacy code would just match the verse
			// content with adaptations retention, and then the loop would be exited and
			// the whole of the remaining newly edited source text would be added as a
			// copy operation - thereby throwing all the adaptations etc for that
			// material. The approach I'll take here is to test for this kind of scenario,
			// and when the size of the old LAST chunk is disproportionately large to the
			// matched new chunk, call MergeRecursively() with the old and new spans being
			// everything from the presently unmerged old and new material up to the very
			// end of each CSourcePhrase array - the internal one (arrOld), and the one
			// for the edited material just imported (arrNew). For this, the limit value
			// should be -1, so that milestones are ignored and the final (large) chunks
			// are merged as wholes. Then control should exit the loop, and the
			// MergeUpdatedSrcTextCore() function should exit after any final housekeeping
			// is done.
			if ( // bInitialRefsSame may also be TRUE, but we won't assume so and so won't test for it
				!bPairedOK &&
				bDisparateSizes &&
				(countOldChunks > 0) &&
				(oldLastChunkIndex != wxNOT_FOUND) &&
				(oldLastChunkIndex == oldMaxIndex)
				)
			{
				// set the following flag, and delay processing until the matched
				// milestoned material, if any, has been handled - then test the flag for
				// TRUE just before loop end, and if true then exit the loop and finish
				// the super-chunk merger outside the loop
				bHandleDispirateSizedLastOldChunk = TRUE;
			}

			if (bPairedOK && bInitialRefsSame)
			{
				// didn't come to a halting location, so must have reached end of one
				// or both arrays -- check it out
				if (oldLastChunkIndex == oldMaxIndex && newLastChunkIndex == newMaxIndex)
				{
					// reached the end of both SfmChunk arrays simultaneously, so we
					// can just recursively process the rest without residue
					pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex);
					pEndChunkNew = (SfmChunk*)pChunksNew->Item(newLastChunkIndex);

					CopySubArray(arrOld, pOldChunk->startsAt, pEndChunkOld->endsAt, subArrOld);
					CopySubArray(arrNew, pNewChunk->startsAt, pEndChunkNew->endsAt, subArrNew);
					// MergeRecursively() appends to pMergedList and updates sequ numbers, and
					// copies changed punctuation and/or USFMs for material "in common",
					// etc; for in-sync milestoned chunk pairings, use the (small) default
					// value of limit
					MergeRecursively(subArrOld, subArrNew, pMergedList, defaultLimit, nStartingSequNum,
						pChunksOld, pChunksNew, processPerMilestone, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() milestoned, paired: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pEndChunkOld->endsAt, pNewChunk->startsAt, pEndChunkNew->endsAt, defaultLimit, nStartingSequNum);
#endif
					RemoveAll(&subArrOld);
					RemoveAll(&subArrNew);
					// m_nSequNumber values have been made up-to-date already, for
					// pMergedList
					break;
				}
				else
				{
					// Got to one or the other array's end, but not simultaneously to
					// the end of both... If we got to the end of the arrNew material,
					// than the unpaired arrOld material beyond is to be deleted,
					// because the user doesn't want it any more - so just don't copy
					// it's CSourcePhrase instances to pMergedList. On the other hand,
					// if we got to the end of arrOld's material, then the user has
					// added more source text at the end of the previous source text,
					// and we have to now copy ALL of the remaining CSourcePhrase
					// instances in arrNew to pMergedList - no recursion is needed;
					// and we must explicitly do m_nSequNumber updating here as well.
					// But prior to all that, we must recursively merge whatever stuff
					// was successfully paired to form a super-chunk.
					pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex);
					pEndChunkNew = (SfmChunk*)pChunksNew->Item(newLastChunkIndex);

					CopySubArray(arrOld, pOldChunk->startsAt, pEndChunkOld->endsAt, subArrOld);
					CopySubArray(arrNew, pNewChunk->startsAt, pEndChunkNew->endsAt, subArrNew);
                    // MergeRecursively() appends to pMergedList and updates sequ
                    // numbers, and copies changed punctuation and/or USFMs for
                    // material "in common", etc; for in-sync milestoned chunk
                    // pairings, use the (small) default value of limit
					MergeRecursively(subArrOld, subArrNew, pMergedList, defaultLimit, nStartingSequNum,
						pChunksOld, pChunksNew, processPerMilestone, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() milestoned, not paired, a super-chunk: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pEndChunkOld->endsAt, pNewChunk->startsAt, pEndChunkNew->endsAt, defaultLimit, nStartingSequNum);
#endif
					RemoveAll(&subArrOld);
					RemoveAll(&subArrNew);

					// check for the only situation which requires we do more, as
					// explained above
					if (newLastChunkIndex < newMaxIndex)
					{
						// there is more arrNew material to be added to pMergedList
						pNewChunk = (SfmChunk*)pChunksNew->Item(newLastChunkIndex + 1);
						pEndChunkNew = (SfmChunk*)pChunksNew->Item(newMaxIndex);
						CopyToList(arrNew, pNewChunk->startsAt, pEndChunkNew->endsAt, pMergedList);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("CopyToList() milestoned, newLastChunkIndex < newMaxIndex: NEW [ %d : %d] copied"),
				pNewChunk->startsAt, pEndChunkNew->endsAt);
#endif
						if (!pMergedList->IsEmpty())
						{
                            // when MergeRecursively() isn't used, we need to force the
                            // update; when it is used, it is done within that function
                            // call, on the whole pMergedList, always using the
                            // nStartingSequNum value to start from
							gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
						}
                        // BEW added 26Sep12, we have to make sure the block of code after
                        // the loop isn't entered, because we've completed the merge now;
                        // we can do this simply by resetting newLastChunkIndex to
                        // newMaxIndex
						newLastChunkIndex = newMaxIndex;
					}
					break;

				} // end of else block for test:
				  // if (oldLastChunkIndex == oldMaxIndex && newLastChunkIndex == newMaxIndex)

			} // end of TRUE block for test: if (bPairedOK && bInitialRefsSame)
			else
			{
				// Came to a halt-forcing location, so process the superchunk
				// collected up to that point (oldLastChunkIndex and newLastChunkIndex
				// are returned pointing at the previous locations - that is, the last
				// successful pairing before the halt location was found). A chunk
				// pair which have disparate sizes would be unsafe to process with a
				// small limit value, so we treat such a pair as causing a halt too -
				// and since such a halt could occur before any aggregation has
				// happened, we must check for this and only merge the aggregated
				// material provided there is such material already aggegated. If the
				// pair are disparate in size, the indices returned point to those
				// two, rather than to the instances preceding them as for other halt
				// situations)
				bool bDoMyEndTweak = FALSE;
				// The following complex 3-part test has the following meaning: if one of
				// the conditions is TRUE, then there is at least one (but possibly more,
				// even hundreds) paired chunk which were successfully matched
				// simple-verse chunks, and so should be processed recursively with the
				// minimal limit value defaultLimit (the value passed in, which equals
				// SPAN_LIMIT and the latter is set at 80 in AdaptitConstants.h).
				// After that successfully paired lot of chunks there will be a location
				// where the pairing attempt failed - either due to disparate sizes in the
				// pair being considered, or because the verse referencing etc is messy
				// and not a simple single-verse matchup. In the next block we want to
				// deal with the successfully paired stuff,
				// The conditions have the following individual implications for a
				// TRUE result:
                // 1st: bInitialRefsSame is TRUE -- the first chunks considered for pairing
                // turned out to be pairable simple-verse chunks, and therefore any halting
                // location for aggregating would have to be later in the chunk arrays, so
                // at least the first pair need a recursive merge with minimal limit value.
				// But a FALSE value means there was nothing successfully aggregated and
				// so this block has to be skipped.
				// 2nd: the last pair considered for a match were not disparate sized, and
				// so that pair at least would qualify - so there is something
				// successfully aggregated and hence this block will need to handle that
				// much. (But there may have been a disparately sized pair encountered
				// later, so we'll need to handle that pair AFTER this block is finished.)
				// 3rd: The initial pairing succeeded, and so there is something to be
				// done here, and the last pairing was disparately sized, so we have
				// something aggregated successfully only provided the oldLastChunkIndex
				// and newLastChunkIndex are greater than the indices started from - if
				// not so, the disparate sized chunk pair would be at the same values as
				// oldChunkIndex and newChunkIndex and there would be nothing successfully
				// aggreagated for us to handle in the next block.
				if ( bInitialRefsSame && (!bDisparateSizes ||
					(bDisparateSizes && (oldChunkIndex < oldLastChunkIndex && newChunkIndex < newLastChunkIndex)))
					)
				{
					// there is non-empty "successfully paired aggregated material" to process first
					if (bDisparateSizes)
					{
						// the end of the "successfully paired" chunks is the chunks
						// preceding those which oldLastChunkIndex and
						// newLastChunkIndex point at (because GetMaxInSyncChunksPairing()
                        // for a return due to disparate sizes returns not the previous
                        // indices to the matchup, but the indices of the matchup
                        // itself, so the 'normal' stuff ends at the previous indices
                        // to those returned)
						// The following tweak is a bit of a kludge. If there is a
						// disparate sized pairing at the end of the arrays, the
						// exclusion of that pairing can lead to a less than
						// satisfactory result if the final verses were re-arranged in
						// their order (yes, I know, who'd ever do that?) but with
						// this tweak we can include the final pair and get a better
						// result (more 'good' adaptations get retained)
						if (oldLastChunkIndex == oldMaxIndex && newLastChunkIndex == newMaxIndex)
						{
							bDoMyEndTweak = TRUE;
						}
						if (bDoMyEndTweak)
						{
							pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex);
							pEndChunkNew = (SfmChunk*)pChunksNew->Item(newLastChunkIndex);
						}
						else
						{
							pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex - 1);
							pEndChunkNew = (SfmChunk*)pChunksNew->Item(newLastChunkIndex - 1);
						}
					}
					else
					{
						// the end of the "successfully paired" chunks is the chunks
						// which oldLastChunkIndex and newLastChunkIndex point at
						pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex);
						pEndChunkNew = (SfmChunk*)pChunksNew->Item(newLastChunkIndex);
					}

					CopySubArray(arrOld, pOldChunk->startsAt, pEndChunkOld->endsAt, subArrOld);
					CopySubArray(arrNew, pNewChunk->startsAt, pEndChunkNew->endsAt, subArrNew);
					// MergeRecursively() appends to pMergedList and updates sequ
					// numbers, and copies changed punctuation and/or USFMs for
					// material "in common", etc; for in-sync milestoned chunk
					// pairings, use the (small) default value of limit
					MergeRecursively(subArrOld, subArrNew, pMergedList, defaultLimit, nStartingSequNum,
						pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() milestoned, disparate super-chunk -- earlier: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pEndChunkOld->endsAt, pNewChunk->startsAt, pEndChunkNew->endsAt, defaultLimit, nStartingSequNum);
#endif
					RemoveAll(&subArrOld);
					RemoveAll(&subArrNew);
				} // end of TRUE block for complex 3-part test:
				  // if ( bInitialRefsSame || (!bDisparateSizes ||
				  // (bDisparateSizes && (oldChunkIndex < oldLastChunkIndex && newChunkIndex < newLastChunkIndex)))

				// if bDisparateSizes is TRUE, we have to merge that pair which have
				// the disparate word count sizes now, using a limit value of -1 to
				// ensure all the material involved is used for determining the
				// largest possible 'in-common' span - then augment the loop indices
				// and iterate (bDisparateSizes TRUE can ONLY happen for a simple-verse
				// matchup which otherwise would have succeeded except for the fact that
				// the sizes of the arrOld and arrNew chunks are too different to be
				// safely recursively merged with a minimal value of limit)
				// This block also handles the case where the first pair tried on a new
				// iteration did not succeed because they were disparately sized - and
				// hence there was nothing successfully aggregated (if the latter was the
				// case, that stuff would have been processed in the block above)
				if (bDisparateSizes)
				{
					// starting and ending chunks are the same one, so use
					// pEndChunkOld and pEndChunkNew for startsAt and endsAt values
					pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex);
					pEndChunkNew = (SfmChunk*)pChunksNew->Item(newLastChunkIndex);
					CopySubArray(arrOld, pEndChunkOld->startsAt, pEndChunkOld->endsAt, subArrOld);
					CopySubArray(arrNew, pEndChunkNew->startsAt, pEndChunkNew->endsAt, subArrNew);
					MergeRecursively(subArrOld, subArrNew, pMergedList, -1, nStartingSequNum,
						pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() milestoned, disparate super-chunk -- at end: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pEndChunkOld->startsAt, pEndChunkOld->endsAt, pEndChunkNew->startsAt, pEndChunkNew->endsAt, -1, nStartingSequNum);
#endif
					RemoveAll(&subArrOld);
					RemoveAll(&subArrNew);
					oldChunkIndex = oldLastChunkIndex + 1;
					newChunkIndex = newLastChunkIndex + 1;
					bDisparateSizes = FALSE; // not needed, but harmless
					if (!bDoMyEndTweak)
					{
						continue;
					}
					else
					{
						break; // out of the loop
					}
				}

                // Any recursive merging of material which comprised successful pairings
                // has now been done, including any any pair which are disparate in size;
                // so here below we handle the stuff which lies ahead of the successful
                // pairings, that is,the SfmChunks which were not paired, and find the
                // closest later matchup of a single-verse pair (if any) -- note, this
                // 'closest safe matchup' attempt may return TRUE but also because of a
                // disparately sized pair was found which are a simple-verse matched pair
                // (it returns TRUE in such a case, but the bDisparateSizes parameter will
                // also be returned as TRUE, whereas it would otherwise come back as FALSE)
				oldChunkIndex = oldLastChunkIndex + 1; // first chunk of the arrOld's messy data
				newChunkIndex = newLastChunkIndex + 1; // first chunk of the arrNew's messy data
				int oldMatchedChunk; // for returning closest paired old chunk, or -1 if none
				int newMatchedChunk; // for returning closest paired new chunk, or -1 if none
				bool bGotALaterPairing = FindClosestSafeMatchup(arrOld, arrNew, pChunksOld,
											pChunksNew, oldChunkIndex, newChunkIndex,
											oldMatchedChunk, newMatchedChunk, bDisparateSizes);
                // Note: if bDisparateSizes is returned TRUE, the oldMatchedChunk and
                // newMatchedChunk index the pair which have the disparate size, and
                // that pair are to be given special treatment here and not used as the
                // kick-off locations for the loop iteration - instead, the index
                // values one greater than each should be used for kick-off at
                // iteration. However, for FALSE returned in bDisparateSizes, the
                // indices will still point at the matched pair, but that matched pair
                // SHOULDn't be included in the processing of the messy material at the
                // halt, because they are to be the later kick-off locations when the
				// loop iterates. We subsume the disparate sizes = TRUE situation into
				// the other cases where a dynamicLimit value needs to be worked out,
				// and we include the disparate sized chunks in the material to be
				// handled by that recursive merge
				bool bFinished = FALSE; // initialize value
				bDoingToTheEnd = FALSE; // initialize value
				if (bGotALaterPairing)
				{
					// the oldMatchedChunk and newMatchedChunk values can be relied
					// upon; don't include these locations in the merger to be done
					// here, instead they will be the kick-off locations for when the
					// aggregation of pairings takes off again on next iteration of
					// the loop; but note: if adding a verse(s), e.g. 4 between 3 & 5,
					// oldMatchedChunk will return same index as oldChunkIndex passed
					// in, and if removing a verse(s), newChunkIndex and newMatchedChunk
					// will be identical indices. In these situations, we don't need
					// any recursion, just detect them and do the direct copy from
					// arrNew to pMergedList, or in the case of deleting one or more
					// verses, just don't copy anything to pMergedList; and kick-off
					// for next iteration will be oldMatchedChunk and newMatchedChunk
					// values. (These comments apply equally to 3 more blocks below.)
					// Note, check for bDisparateSizes TRUE, and if so, we have to
					// process that particular pair with a limit value of -1, and the
					// oldMatchedChunk and newMatchedChunk values are the disparate
					// sized pair, and won't be used subsequently as the kick-off
					// location, but rather then next location in each array will be
					// used for that purpose.)
					if (oldChunkIndex == oldMatchedChunk && newChunkIndex < newMatchedChunk)
					{
						if (bDisparateSizes)
						{
                            // the chunk sizes are disparate, so the matchup cannot be
                            // a subsequent kickoff location; but there are one or more
                            // arrNew verses to be inserted here, and no recursion is
                            // needed for that, but recursion is needed for the
                            // disparate chunks - with a large limit value too
							CopyToList(arrNew, ((SfmChunk*)pChunksNew->Item(newChunkIndex))->startsAt,
								((SfmChunk*)pChunksNew->Item(newMatchedChunk - 1))->endsAt, pMergedList);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("CopyToList() disparate sizes, oldChunkIndex == oldMatchedChunk && newChunkIndex < newMatchedChunk: NEW [ %d : %d]"),
				((SfmChunk*)pChunksNew->Item(newChunkIndex))->startsAt,
				((SfmChunk*)pChunksNew->Item(newMatchedChunk - 1))->endsAt);
#endif
							// update sequence numbers using initialSequNum
							if (!pMergedList->IsEmpty())
							{
								// when MergeRecursively() isn't used, we need to force the update;
								// when it is use, it is done within that function call, on the whole
								// pMergedList, always using the nStartingSequNum value to start from
								gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
							}
							// now prepare for the if (!bFinished) code block below
							int oldCountOfWords =
								CountWords(&arrOld, pChunksOld, oldMatchedChunk, oldMatchedChunk);
							int newCountOfWords =
								CountWords(&arrNew, pChunksNew, newMatchedChunk, newMatchedChunk);
							dynamicLimit = wxMax(oldCountOfWords, newCountOfWords); // ensure a cover
							// obtain the start and end chunks in arrOld and arrNew for each subspan
							pOldChunk = (SfmChunk*)pChunksOld->Item(oldMatchedChunk);
							pNewChunk = (SfmChunk*)pChunksNew->Item(newMatchedChunk);
							pEndChunkOld = pOldChunk;
							pEndChunkNew = pNewChunk;
							bFinished = FALSE;
						} // end of TRUE block for test: if (bDisparateSizes)
						else
						{
                            // the chunk sizes are not disparate, so the matchup can be
                            // a subsequent kickoff location; but there are one or more
                            // arrNew verses to be inserted here, and no recursion is
                            // needed
							CopyToList(arrNew, ((SfmChunk*)pChunksNew->Item(newChunkIndex))->startsAt,
								((SfmChunk*)pChunksNew->Item(newMatchedChunk - 1))->endsAt, pMergedList);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("CopyToList() NOT disparate sizes, oldChunkIndex == oldMatchedChunk && newChunkIndex < newMatchedChunk: NEW [ %d : %d]"),
				((SfmChunk*)pChunksNew->Item(newChunkIndex))->startsAt,
				((SfmChunk*)pChunksNew->Item(newMatchedChunk - 1))->endsAt);
#endif
							// update sequence numbers using initialSequNum
							if (!pMergedList->IsEmpty())
							{
								// when MergeRecursively() isn't used, we need to force the update;
								// when it is use, it is done within that function call, on the whole
								// pMergedList, always using the nStartingSequNum value to start from
								gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
							}
							bFinished = TRUE; // suppresses the MergeRecursively() call below
						} // end of else block for test: if (bDisparateSizes)
					}
					else if (oldChunkIndex < oldMatchedChunk && newChunkIndex == newMatchedChunk)
					{
						// there are one or more arrOld verses to be removed, no
						// recursion is needed; but if oldMatchedChunk and
						// newMatchedChunk are disparately sized, we need to do a
						// merger below with a value for limit which is large - so
						// check and set up for that if warranted
						if (bDisparateSizes)
						{
							int oldCountOfWords =
								CountWords(&arrOld, pChunksOld, oldMatchedChunk, oldMatchedChunk);
							int newCountOfWords =
								CountWords(&arrNew, pChunksNew, newMatchedChunk, newMatchedChunk);
							dynamicLimit = wxMax(oldCountOfWords, newCountOfWords); // ensure a cover
							// obtain the start and end chunks in arrOld and arrNew for each subspan
							pOldChunk = (SfmChunk*)pChunksOld->Item(oldMatchedChunk);
							pNewChunk = (SfmChunk*)pChunksNew->Item(newMatchedChunk);
							pEndChunkOld = pOldChunk;
							pEndChunkNew = pNewChunk;
#if defined(_DEBUG) && defined(MERGE_Recursively)
							wxLogDebug(_T("Removing some OLD: disparate sizes, oldChunkIndex < oldMatchedChunk && newChunkIndex == newMatchedChunk: old word count %d  new word count %d, uses max"),
								oldCountOfWords, newCountOfWords);
#endif
							bFinished = FALSE; // redundant, but it documents what's happening
						}
						else
						{
							// nothing to be done, the old stuff will get deleted
							bFinished = TRUE; // suppresses the MergeRecursively() call below
						}
					}
					else if (bDisparateSizes &&
							(oldChunkIndex == oldMatchedChunk && newChunkIndex == newMatchedChunk))
					{
                        // we get here if there has as yet been no aggegation of
                        // successfully paired instances, but the first pairing was of
                        // a disparately sized pair, -- merging this pair with a small
                        // limit value would be in danger of leaving some material
                        // unprocessed - which would cause some data loss or premature
                        // exit without dealing with all the data; so for this scenario
                        // we use a limit value of -1 on just the pair of chunks with
                        // the disparate sizes (other situations were dealt with above)
						int oldCountOfWords =
							CountWords(&arrOld, pChunksOld, oldMatchedChunk, oldMatchedChunk);
						int newCountOfWords =
							CountWords(&arrNew, pChunksNew, newMatchedChunk, newMatchedChunk);
						dynamicLimit = wxMax(oldCountOfWords, newCountOfWords); // ensure a cover
						// obtain the start and end chunks in arrOld and arrNew for each subspan
						pOldChunk = (SfmChunk*)pChunksOld->Item(oldMatchedChunk);
						pNewChunk = (SfmChunk*)pChunksNew->Item(newMatchedChunk);
						pEndChunkOld = pOldChunk;
						pEndChunkNew = pNewChunk;
#if defined(_DEBUG) && defined(MERGE_Recursively)
							wxLogDebug(_T("no aggregation yet: but disparate sizes, oldChunkIndex == oldMatchedChunk && newChunkIndex == newMatchedChunk: old word count %d  new word count %d, uses max"),
								oldCountOfWords, newCountOfWords);
#endif
						bFinished = FALSE;
					}
					else if (!bDisparateSizes &&
							(oldChunkIndex == oldMatchedChunk && newChunkIndex == newMatchedChunk))
					{
						// shouldn't happen
						bFinished = TRUE;
					}
					else
					{
						// recursion is needed for a merger, there aren't any
						// disparate sized chunks, just a messy section of
						// non-pairings to deal with -- so get the chunks pointed at &
						// work count done for setting dynamicLimit
						int oldCountOfWords =
							CountWords(&arrOld, pChunksOld, oldChunkIndex, oldMatchedChunk - 1);
						int newCountOfWords =
							CountWords(&arrNew, pChunksNew, newChunkIndex, newMatchedChunk - 1);
						dynamicLimit = wxMax(oldCountOfWords, newCountOfWords); // ensure a cover
						// obtain the start and end chunks in arrOld and arrNew for each subspan
						pOldChunk = (SfmChunk*)pChunksOld->Item(oldChunkIndex);
						pNewChunk = (SfmChunk*)pChunksNew->Item(newChunkIndex);
						pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldMatchedChunk - 1);
						pEndChunkNew = (SfmChunk*)pChunksNew->Item(newMatchedChunk - 1);
#if defined(_DEBUG) && defined(MERGE_Recursively)
							wxLogDebug(_T("recursion needed: NOT disparate sizes, a mess of unpaired chunks to aggregate & merge: old word count %d  new word count %d, uses max"),
								oldCountOfWords, newCountOfWords);
#endif
						bFinished = FALSE;
					}
				} // end of TRUE block for test: if (bGotALaterPairing)
				else
				{
					// got to the end of the arrays without finding a pairing, (and
					// this also means that bDisparateSizes was returned FALSE) so
					// oldMatchedChunk and newMatchedChunk index values are useless
					// (being wxNOT_FOUND) So use oldMaxIndex and newMaxIndex to
					// process the end material; and no accomodation needs to be made
					// in the code here for bDisparateSizes
					int oldCountOfWords =
						CountWords(&arrOld, pChunksOld, oldLastChunkIndex + 1, oldMaxIndex);
					int newCountOfWords =
						CountWords(&arrNew, pChunksNew, newLastChunkIndex + 1, newMaxIndex);
					dynamicLimit = wxMax(oldCountOfWords, newCountOfWords); // ensure a cover
					// obtain the start and end chunks in arrOld and arrNew for each subspan
					pOldChunk = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex + 1);
					pNewChunk = (SfmChunk*)pChunksNew->Item(newLastChunkIndex + 1);
					pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldMaxIndex);
					pEndChunkNew = (SfmChunk*)pChunksNew->Item(newMaxIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
							wxLogDebug(_T("at end of the arrays without a pairing: old word count %d  new word count %d, uses max"),
								oldCountOfWords, newCountOfWords);
#endif
					bFinished = FALSE;
					bDoingToTheEnd = TRUE;
				} // end of else block for test: if (bGotALaterPairing)

				// do now any processing not done in the blocks above, but prepared
				// for in those blocks and passing bFinished = FALSE to here
				if (!bFinished)
				{
					// make the subarrays to pass in
					CopySubArray(arrOld, pOldChunk->startsAt, pEndChunkOld->endsAt, subArrOld);
					CopySubArray(arrNew, pNewChunk->startsAt, pEndChunkNew->endsAt, subArrNew);
					// MergeRecursively() with the word-count-based limit value, which
					// ensures safety (a word count is potentially larger than a
					// CSourcePhrase count, because of the possibility of mergers)
					MergeRecursively(subArrOld, subArrNew, pMergedList, dynamicLimit, nStartingSequNum,
						pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
						arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() the messy aggregates: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pEndChunkOld->endsAt, pNewChunk->startsAt, pEndChunkNew->endsAt, dynamicLimit, nStartingSequNum);
#endif
					RemoveAll(&subArrOld);
					RemoveAll(&subArrNew);

					if (bDoingToTheEnd)
					{
						// because of the safety-first check after exiting the loop, namely
						// the line [ newChunkIndex = newLastChunkIndex + 1; ] further below,
						// we need a correct value for newLastChunkIndex here, so that when we
						// break from the loop, the code following the loop will behave
						// correctly
						newLastChunkIndex = newMaxIndex;
					}
				}
				// update the iterators & iterate (the kick-off point depends on
				// whether bDisparateSizes was TRUE or FALSE; if TRUE, it is one
				// chunk further along in both chunk arrays)
				if (bDisparateSizes)
				{
					// set the index values for next iteration to be one greater than
					// otherwise we would, since we've just merged the disparately
					// sized pair at oldMatchedChunk and newMatchedChunk
					oldChunkIndex = oldMatchedChunk + 1;
					newChunkIndex = newMatchedChunk + 1;
#if defined(_DEBUG) && defined(MERGE_Recursively)
					wxLogDebug(_T("Kick-off indices for next iteration:  Disparate Sizes: oldChunkIndex %d  newChunkIndex %d  Loop Iterates Now"),
					oldChunkIndex, newChunkIndex);
#endif
				}
				else if (!bDoingToTheEnd)
				{
					// normal situation, oldMatchedChunk and newMatchedChunk are to be
					// the kick-off location for next iteration -- we do this block only if
					// at a preceding block we've not processed to the end of the old and
					// new arrays, that's what !bDoingToTheEnd tells us
					oldChunkIndex = oldMatchedChunk;
					newChunkIndex = newMatchedChunk;
#if defined(_DEBUG) && defined(MERGE_Recursively)
					wxLogDebug(_T("Kick-off indices for next iteration:  Normal Sizes: oldChunkIndex %d  newChunkIndex %d  Loop Iterates Now"),
					oldChunkIndex, newChunkIndex);
#endif
				}
			} // end of else block for test: if (bPairedOK && bInitialRefsSame)
			// structure ok to bracket above

			if (bDoingToTheEnd)
			{
				break; // break out of the loop
			}
			if (bHandleDispirateSizedLastOldChunk)
			{
				// this flag being true is also cause for exiting the loop and processing
				// in a once-only block below; we need to here set newLastChunkIndex to its
				// maximum value to prevent a spurious copy being done below outside the loop
				newLastChunkIndex = newMaxIndex; // this is needed!
				oldLastChunkIndex = oldMaxIndex; // not needed, but documents where
													 // processing has finished to
			}
		} // end of loop: while (oldIndex < countOldChunks && newIndex < countNewChunks)

		// Since the above loop can exit with one of the two arrays still with data, we
		// have to check and do any end-processing here. If arrNew was fully handled, and
		// arrNew still has some chunks, then the user's edits want those old chunks
		// removed - so we just don't copy them to pMergedList. The other possibility is
		// that arrOld was fully processed, and arrNew has more chunks - that means the
		// user has added new material that he wants included, and so a simple deep copy
		// of them to pMergedList would need to be done here.
		newChunkIndex = newLastChunkIndex + 1; // since newLastChunkIndex was last instance
											   // copied to pMergeList
		// BEW added next test and true block on 25Oct12
		if (bHandleDispirateSizedLastOldChunk)
		{
			// Because bDisparateSizes is TRUE, the oldLastChunkIndex and
			// newLastChunkIndex values returned are the chunk indices for
			// the parts which are yet to be merged; so use this fact to
			// set the needed subarrays for the final superchunk merge
			pOldChunk = (SfmChunk*)pChunksOld->Item(oldLastChunkIndex); // starts here
			pNewChunk = (SfmChunk*)pChunksNew->Item(newLastChunkIndex); // starts here
			pEndChunkOld = (SfmChunk*)pChunksOld->Item(oldMaxIndex); // ends at end of this one
			pEndChunkNew = (SfmChunk*)pChunksNew->Item(newMaxIndex); // ends at end of this one
			// now set up the content of subArrOld and subArrNew
			CopySubArray(arrOld, pOldChunk->startsAt, pEndChunkOld->endsAt, subArrOld);
			CopySubArray(arrNew, pNewChunk->startsAt, pEndChunkNew->endsAt, subArrNew);
			// Merge the superchunks. Fot the processStartToEnd enum value, params
			// after the nStartingSequNum param are not used, and what's passed in are
			// just fillers that are ignored.
			MergeRecursively(subArrOld, subArrNew, pMergedList, defaultLimit, nStartingSequNum,
				pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
				arrOld, arrNew, oldChunkIndex, newChunkIndex);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("MergeRecursively() last old chunk & disparate sized, take all remaining in both old & new \nas super-chunk -- earlier: OLD [ %d : %d] NEW [ %d : %d] limit=%d  starting sequnum = %d"),
				pOldChunk->startsAt, pEndChunkOld->endsAt, pNewChunk->startsAt, pEndChunkNew->endsAt, -1, nStartingSequNum);
#endif
			RemoveAll(&subArrOld);
			RemoveAll(&subArrNew);
		}
		else if (newChunkIndex <= newMaxIndex)
		{
			// copy the rest from arrNew to pMergedList
			CopyToList(arrNew, ((SfmChunk*)pChunksNew->Item(newChunkIndex))->startsAt,
				((SfmChunk*)pChunksNew->Item(newMaxIndex))->endsAt, pMergedList);
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("CopyToList() AFTER LOOP, newChunkIndex <= newMaxIndex: NEW [ %d : %d] where newChunkIndex was %d"),
				((SfmChunk*)pChunksNew->Item(newChunkIndex))->startsAt,
				((SfmChunk*)pChunksNew->Item(newMaxIndex))->endsAt, newChunkIndex);
#endif
			// update sequence numbers using initialSequNum
			if (!pMergedList->IsEmpty())
			{
				// when MergeRecursively() isn't used, we need to force the update;
				// when it is use, it is done within that function call, on the whole
				// pMergedList, always using the nStartingSequNum value to start from
				gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
			}
		}

	} // end of TRUE block for test: if (countOldChunks > 0 && countNewChunks > 0)
	else
	{
		// One or both of the SfmChunk arrays is empty. The case where the source text has
		// no SFMs or USFMs, and still when edited it doesn't have any, is handled above.
		// What we need to deal with here is:
		// (1) arrOld has data, but no SFMs; but arrNew has milestoned data (a situation
		// unlikely to occur though - but it would occur if the user had scripture without
		// SFMs which he'd adapted and then decided to insert verse and chapter and other
		// SFM markup outside of Adapt It in the source text as exported from the
		// document, and then he imports the edited stuff -- hmmm, this isn't all that
		// unlikely, and we'll want to support it robustly)
		// (2)arrOld is empty (no CSourcePhrase instances), but arrNew is not (The
		// OnUpdateImportEditedSourceText() handler disables the menu item, so we don't
		// expect it. However, using MergeUpdatedSrcTextCore() in Paratext collaboration
		// mode bypasses such a check, and so we must handle it - we'll just copy all the
		// arrNew CSourcePhrase instances to pMergedList, and they become the (unadapted)
		// document)
		// (3) arrOld is milestoned, but arrNew isn't but has some text without SFM
		// markup (we could disallow this combination from making any changes,
		// otherwise it would wipe out all the original milestoned data; however, it may
		// happen that the user wants to remove USFMs from the document but retain
		// adaptations made, so we'll allow it)
		//
		// The way to handle (2) doesn't require recursion, just a copy operation. The
		// others, and any other unlikely combinations are best handled by simply calling
		// MergeRecursively() with a limit value of -1, which gives the safest merge in a
		// non- or partly-milestoned situation
#if defined(_DEBUG) && defined(MERGE_Recursively)
			wxLogDebug(_T("else block, AFTER LOOP, special cases 1 23 or 3  -- we don't expect to enter either of these two blocks"));
#endif
		if (arrOld.IsEmpty())
		{
			CopyToList(arrNew, 0, arrNew.GetCount() - 1, pMergedList);

			// update sequence numbers using initialSequNum
			if (!pMergedList->IsEmpty())
			{
				gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
			}
		}
		else
		{
            // this will be very inefficient and slow because all unique words in arrOld
            // must be checked for all matchups with all unique words in arrNew, an order
            // of N squared operation, where N = total words (but if one or the other array
            // is markedly shorter, it isn't so bad)
			MergeRecursively(arrOld, arrNew, pMergedList, -1, nStartingSequNum,
				pChunksOld, pChunksNew, processStartToEnd, arrPairingOld,
				arrOld, arrNew, oldChunkIndex, newChunkIndex);
			// Updating m_nSequNumber values is done internally
		}
	} // end of else block for test: if (countOldChunks > 0 && countNewChunks > 0)

	// we are done with the temporary array's of SfmChunk instances,
	// so remove them from the heap
	DeleteSfmChunkArray(pChunksOld);
	DeleteSfmChunkArray(pChunksNew);
}

void DeleteSfmChunkArray(wxArrayPtrVoid* pChunksArray)
{
	int iter;
	int countStructs = pChunksArray->GetCount();
	if (countStructs > 0)
	{
		for (iter = 0; iter < countStructs; iter++)
		{
			SfmChunk* pChunk = (SfmChunk*)pChunksArray->Item(iter);
			if (pChunk != NULL) // whm 11Jun12 added NULL test
				delete pChunk;
		}
	}
	if (pChunksArray != NULL) // whm 11Jun12 added NULL test
		delete pChunksArray;
}

// Return TRUE only when one or both of the word counts exceeds SPAN_LIMIT, AND the ratio
// of max count to min count exceeds 1.5; ratios exceeding 1.5 are safe if the span is
// small enough to fall within the bounds defined by the default limit = SPAN_LIMIT (this
// prevents returning TRUE when, say, the new verse is 21 words and the old was 7 - such
// differences are easily handled in the recursion and safely, provided they fall within
// the limit value)
bool AreSizesDisparate(SPArray& arrOld, SPArray& arrNew, SfmChunk* pOldChunk, SfmChunk* pNewChunk)
{
	wxArrayPtrVoid tempOld;
	wxArrayPtrVoid tempNew;
	tempOld.Add(pOldChunk);
	tempNew.Add(pNewChunk);
	// params 3 and 4 in the CountWords() signature are firstChunk's index, and
	// lastChunk's index, respectively
	int oldWordsInChunk = CountWords(&arrOld, &tempOld, 0, 0); // only one SfmChunk is in pTempOld
	int newWordsInChunk = CountWords(&arrNew, &tempNew, 0, 0); // only one SfmChunk is in pTempNew

//	if (oldWordsInChunk <= (int)SPAN_LIMIT && newWordsInChunk <= (INT)SPAN_LIMIT) <-(INT) gave problems with GCC 4
	if (oldWordsInChunk <= (int)SPAN_LIMIT && newWordsInChunk <= (int)SPAN_LIMIT)
	{
		// if SPAN_LIMIT covers all the words in either array, no problem, even if one
		// array's word count is zero
		return FALSE;
	}
	int maxWords = wxMax(oldWordsInChunk,newWordsInChunk);
	int minWords = wxMin(oldWordsInChunk,newWordsInChunk);
	// the two constants are defined in AdaptitConstants.h as 3 for numerator and 2 for denominator
	int second = (int)DISPARATE_SIZES_NUMERATOR * minWords;
	int first = (int)DISPARATE_SIZES_DENOMINATOR * maxWords;
	if (first >  second)
	{
#if defined(_DEBUG) && defined(myLogDebugCalls)
		wxLogDebug(_T("AreSizesDisparate() is TRUE for pOldChunk %s:%s  OLD chunk: [starts,ends] = [ %d , %d ]  NEW chunk: [ %d , %d ] "),
			pOldChunk->strChapter.c_str(), pOldChunk->strStartingVerse.c_str(), pOldChunk->startsAt, pOldChunk->endsAt, pNewChunk->startsAt, pNewChunk->endsAt);
#endif
		return TRUE;
	}
	else
	{
//#ifdef myMilestoneDebugCalls
#if defined(_DEBUG) && defined(myLogDebugCalls)
		wxLogDebug(_T("AreSizesDisparate() is FALSE, matched simple-verse chunks: for pOldChunk %s:%s  OLD = [ %d , %d ]  NEW = [ %d , %d ] "),
			pOldChunk->strChapter.c_str(), pOldChunk->strStartingVerse.c_str(), pOldChunk->startsAt, pOldChunk->endsAt, pNewChunk->startsAt, pNewChunk->endsAt);
#endif
	}
	return FALSE;
}

/// \return         TRUE if safe matchup is determined (oldMatchedChunk and
///                 newMatchedChunk values can then be relied upon); FALSE if no matchup
///                 was able to be made
/// \param  arrOld          ->  nonedited (old) array of CSourcePhrase instances (such as document)
/// \param  arrNew         ->   edited (new) array of CSourcePhrase instances just imported
/// \param  pOldChunks      ->  array of SfmChunk structs indexing into arrOld, determined by analysis
/// \param  pNewChunks      ->  array of SfmChunk structs indexing into arrNew, determined by analysis
/// \param  oldStartChunk   ->  index to the first SfmChunk to be examined when looking
///                             ahead for a simple verse instance which can be matched to
///                             one with the same simple verse, within pNewChunks
/// \param  newStartChunk   ->  index to the first SfmChunk to be examined in pNewChunks,
///                             for the matchup with one in pOldChunks
/// \param  oldMatchedChunk <-  ref to index for the matched SfmChunk pertaining to
///                             arrOld, -1 if there was no matchup made
/// \param  newMatchedChunk <-  ref to index for the matched SfmChunk pertaining to
///                             arrNew, -1 if there was no matchup made
/// \param  bDisparateSizes <-  ref to boolean, TRUE if a matching pair of chunks differ
///                             in word counts by more than a factor of 1.5, else FALSE
/// If the SfmChunks in pOldChunks at oldStartChunk have verse numbers like 5-6a 6b 7 8 9 ...
/// and those in pNewChunks at newStartChunk have numbers like 6 7 8-10 11 12 ...
/// Then the hunt for the closes simple verse matchup would yield 7 for the above data.
/// At where oldStartChunk points, it may be an SfmChunk after a gap in the verses, or one
/// which is a range, or a non-simple number like 4b. We don't want to mess with gaps and
/// non-simple verse references chunks, but just subsume them into a larger chunk which we
/// can deal with as a whole by taking a larger limit value to pass into
/// MergeRecursively(). So we assume that arrOld having a simple verse 7 chunk, and arrNew
/// having also a simple verse 7 chunk, that those chunks are a valid match and therefore
/// define the end of the stuff we want to subsume into a larger chunk and ignore it's
/// internal details. Of course, the caller may have halted processing to make this test
/// call near a chapter end, so we take into account that a chapter may change and
/// anything else relevant, but the general idea is still the same. (We don't want to do
/// the 'ultimate convenience' of taking a limit value equal to the total words in the
/// whole of arrOld or arrNew, as while that would avoid this testing, it would give a
/// large limit value, and that would in turn result in processing bogging down, as the
/// merge algorithm is order N squared, N = number of words being considered.)
/// When setting up the internal arrOldChunkInfos and arrNewChunkInfos, we go from the
/// starting locations in pOldChunks and pNewChunks, and process ALL SfmChunk instances in
/// each array up to the end of pOldChunks and pNewChunks. This is the only safe thing to
/// do, because we can't prescribe that somewhere in the middle of the source text the
/// user may add a sufficient number of chapters and verses previously not in the source
/// text that the number of words involved won't exceed any arbitrary presumed-safe
/// default value.
bool FindClosestSafeMatchup(SPArray& arrOld, SPArray& arrNew, wxArrayPtrVoid* pOldChunks,
							wxArrayPtrVoid* pNewChunks, int oldStartChunk, int newStartChunk,
							int& oldMatchedChunk, int& newMatchedChunk, bool& bDisparateSizes)
{
	int oldCountOfChunks = pOldChunks->GetCount();
	int newCountOfChunks = pNewChunks->GetCount();
	ChunkInfo* pOldChunkInfo = NULL;
	ChunkInfo* pNewChunkInfo = NULL;
	wxArrayPtrVoid arrOldChunkInfos;
	wxArrayPtrVoid arrNewChunkInfos;
	int oldIndex;
	for (oldIndex = oldStartChunk; oldIndex < oldCountOfChunks; oldIndex++)
	{
		SfmChunk* pOldChunk = (SfmChunk*)pOldChunks->Item(oldIndex);
		pOldChunkInfo = new ChunkInfo;
		pOldChunkInfo->type = pOldChunk->type;
		pOldChunkInfo->associatedSfmChunk = oldIndex;
		pOldChunkInfo->verse = pOldChunk->nStartingVerse;
		pOldChunkInfo->chapter = pOldChunk->nChapter;
		pOldChunkInfo->indexRef = pOldChunk->startsAt; // indexes into arrOld
		pOldChunkInfo->bIsComplex = FALSE; // default, but check and make TRUE if required by the
										   // tests in the following lines
		if (pOldChunk->nStartingVerse != pOldChunk->nEndingVerse ||
			pOldChunk->charStartingVerseSuffix != _T('\0') ||
			pOldChunk->charEndingVerseSuffix != _T('\0'))
		{
			pOldChunkInfo->bIsComplex = TRUE;
		}
		// now store this struct
		arrOldChunkInfos.Add(pOldChunkInfo);
	}
	// do the same for the arrNew array's subarray
	int newIndex;
	for (newIndex = newStartChunk; newIndex < newCountOfChunks; newIndex++)
	{
		SfmChunk* pNewChunk = (SfmChunk*)pNewChunks->Item(newIndex);
		pNewChunkInfo = new ChunkInfo;
		pNewChunkInfo->type = pNewChunk->type;
		pNewChunkInfo->associatedSfmChunk = newIndex;
		pNewChunkInfo->verse = pNewChunk->nStartingVerse;
		pNewChunkInfo->chapter = pNewChunk->nChapter;
		pNewChunkInfo->indexRef = pNewChunk->startsAt; // indexes into arrNew
		pNewChunkInfo->bIsComplex = FALSE; // default, but check and make TRUE if required by the
										   // tests in the following lines
		if (pNewChunk->nStartingVerse != pNewChunk->nEndingVerse ||
			pNewChunk->charStartingVerseSuffix != _T('\0') ||
			pNewChunk->charEndingVerseSuffix != _T('\0'))
		{
			pNewChunkInfo->bIsComplex = TRUE;
		}
		// now store this struct
		arrNewChunkInfos.Add(pNewChunkInfo);
	}
	// arrOldChunkInfos and arrNewChunkInfos are populated, so check for the closest safe
	// matchup; oldIndex and newIndex can now be re-used to index into arrOldChunkInfos
	// and arrNewChunkInfos respectively
	int countOldInfoStructs = arrOldChunkInfos.GetCount();
	int countNewInfoStructs = arrNewChunkInfos.GetCount();
	// re-use iterators oldIndex and newIndex in the do loops below. Each must start from 0.
	oldIndex = 0;
	newIndex = 0;
	int oldFoundAt = wxNOT_FOUND;
	int newFoundAt = wxNOT_FOUND;
	bool bOldFoundOne = FALSE;
	bool bNewFoundOne = FALSE;
	bool bMatched = FALSE;
	bDisparateSizes = FALSE;
	// outer loop, ranges over the SfmChunks pertaining to arrOld
	do {

		bOldFoundOne = GetNextSimpleVerseChunkInfo(&arrOldChunkInfos, oldIndex, oldFoundAt);
		if (bOldFoundOne)
		{
			// search in the SfmChunks pertaining to arrNew for a matchup
			bool bGotAMatch = FALSE;
			do {
				bNewFoundOne = GetNextSimpleVerseChunkInfo(&arrNewChunkInfos, newIndex, newFoundAt);
				if (bNewFoundOne)
				{
					// we found an SfmChunk from the new material which is a simple-verse chunk,
					// so check out whether or not we have a matchup
					ChunkInfo* pOldChunk = (ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt);
					ChunkInfo* pNewChunk = (ChunkInfo*)arrNewChunkInfos.Item(newFoundAt);
					// we only call this function on data which is chapter/verse
					// milestoned, so ensure it
					wxASSERT(pOldChunk->type != bookInitialChunk && pOldChunk->type != introductionChunk);
					wxASSERT(pNewChunk->type != bookInitialChunk && pNewChunk->type != introductionChunk);

					// are we beyond the outer loop's reference? If so, no point in looking further
					if (pNewChunk->chapter > pOldChunk->chapter)
					{
						break; // out of inner loop
					}
					if (pNewChunk->verse > pOldChunk->verse)
					{
						break; // out of inner loop
					}

					// set bGotAMatch if we achieved a valid matchup
					if (pNewChunk->chapter == pOldChunk->chapter)
					{
						if (pNewChunk->verse == pOldChunk->verse)
						{
							// we have a valid matchup
							newIndex = newFoundAt;
							bGotAMatch = TRUE;
							break; // out of inner loop
						}
						else
						{
							// no matchup
							newIndex = newFoundAt;
							newIndex++;
							continue; // iterate inner loop
						}
					}
					else
					{
						// diff chapters, so can't match up
						newIndex = newFoundAt;
						newIndex++;
						continue; // iterate inner loop
					}
				}
				else
				{
					// we didn't find a new simple-verse chunk in the chunks pertaining to arrNew,
					// so iterate the outer loop and try again
					break; // out of inner loop
				}
			} while (newIndex < countNewInfoStructs &&  newFoundAt != wxNOT_FOUND);

			// if there was no matchup done, try a later SfmChunk within the old set
			if (!bGotAMatch)
			{
				if (oldFoundAt == -1)
				{
					break;
				}
				oldIndex = oldFoundAt;
				oldIndex++;
				newIndex = 0;
				continue; // outer loop will iterate and inner loop will start over,
						  // or the outer loop will end and we've not found a match
			}
			else
			{
				// we got a match, newIndex is already set to newFoundAt
				oldIndex = oldFoundAt;
				bMatched = TRUE;
                // We got a matchup, so we have a valid halt location to return (we
                // calculate the return value here, since we needed to do so while the
                // ChunkInfo instances still exist; because 0 in arrNewChunkInfo's is not
                // the 0th SfmChunk in the array of SfmChunk instances, likewise for
                // arrNewChunkInfo. The indices we want to return are stored in
                // ChunkInfo::associatedSfmChunk)
				oldMatchedChunk = ((ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt))->associatedSfmChunk;
				newMatchedChunk = ((ChunkInfo*)arrNewChunkInfos.Item(newFoundAt))->associatedSfmChunk;
				SfmChunk* anOldChunkPtr = (SfmChunk*)pOldChunks->Item(oldMatchedChunk);
				SfmChunk* aNewChunkPtr = (SfmChunk*)pNewChunks->Item(newMatchedChunk);
				bDisparateSizes = AreSizesDisparate(arrOld, arrNew, anOldChunkPtr, aNewChunkPtr);
#if defined(_DEBUG) && defined(myMilestoneDebugCalls)
				wxLogDebug(_T("FindClosestSafeMatchup() Matched: OLD SfmChunk %d  chap %d verse %d , NEW SfmChunk %d  chap %d verse %d , in arrOld at %d , & in arrNew at %d  bIsComplex %d , bDisparateSizes %d"),
					((ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt))->associatedSfmChunk,
					((ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt))->chapter,
					((ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt))->verse,
					((ChunkInfo*)arrNewChunkInfos.Item(newFoundAt))->associatedSfmChunk,
					((ChunkInfo*)arrNewChunkInfos.Item(newFoundAt))->chapter,
					((ChunkInfo*)arrNewChunkInfos.Item(newFoundAt))->verse,
					((ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt))->indexRef,
					((ChunkInfo*)arrNewChunkInfos.Item(newFoundAt))->indexRef,
					((ChunkInfo*)arrOldChunkInfos.Item(oldFoundAt))->bIsComplex,
					(int)bDisparateSizes);
#endif
				break;
			}
		} // end of TRUE block for test: if (bOldFoundOne)
		else
		{
			// there weren't any more in the arrOld array of SfmChunk instances that were
			// simple-verse ones, so no matchup of same is possible for what's in arrNew
			// chunks
			break;
		}
	} while (oldIndex < countOldInfoStructs &&  oldFoundAt != wxNOT_FOUND);

	// delete the ChunkInfo instances from the heap
	for (oldIndex = 0; oldIndex < countOldInfoStructs; oldIndex++)
	{
		ChunkInfo* pInfo = (ChunkInfo*)arrOldChunkInfos.Item(oldIndex);
		if (pInfo != NULL) // whm 11Jun12 added NULL test
			delete pInfo;
	}
	for (newIndex = 0; newIndex < countNewInfoStructs; newIndex++)
	{
		ChunkInfo* pInfo = (ChunkInfo*)arrNewChunkInfos.Item(newIndex);
		if (pInfo != NULL) // whm 11Jun12 added NULL test
			delete pInfo;
	}
	// determine what to tell the caller
	if (!bMatched)
	{
		// no matchup, return wxNOT_FOUND values
		oldMatchedChunk = wxNOT_FOUND;
		newMatchedChunk = wxNOT_FOUND;
		return FALSE;
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return         TRUE if one or more in-sync matchups are determined (oldEndChunk and
///                 newEndChunk values can then be relied upon); FALSE if no in-sync matchups
///                 were able to be made, or if their is a valid pairing but the pair are
///                 disparate in size (word counts comparison -- see the bDisparateSizes
///                 parameter for more detail)
/// \param  arrOld          ->  ref to old CSourcePhrase instances array
/// \param  arrNew          ->  ref to new CSourcePhrase instances array
/// \param  pOldChunks      ->  array of SfmChunk structs indexing into arrOld, determined by analysis
/// \param  pNewChunks      ->  array of SfmChunk structs indexing into arrNew, determined by analysis
/// \param  oldStartChunk   ->  index to the first SfmChunk to be examined when examining
///                             instances ahead for verse ref data in-sync
/// \param  newStartChunk   ->  index to the first SfmChunk to be examined in pNewChunks
/// \param  oldEndChunk     <-  ref to index for the last matched in-sync pair of SfmChunk
///                             instances prior to which a dislocation in the syncing was
///                             detected (-1 if no aggregation happened)
/// \param  newEndChunk     <-  ref to index for the matched last in-sync SfmChunk instance
///                             which is in-sync with the one defined by oldEndChunk (-1 if
///                             no aggregation happened)
/// \param  bDisparateSizes <-  return FALSE if the 1.5 times the size of the smaller number
///                             of words is less than or equal to the larger number of
///                             words; otherwise return TRUE -- the caller in the latter
///                             case will use a limit value of -1 to process the pair with
///                             disparate sizes safely
/// \param arrPairingOld    <-  stores the index of the pOldChunk array's matched SfmChunk
///                             struct of a successful pairing (the new text's paired
///                             chunk can be had by supplying an offset in functions which
///                             make use of the contents of arrPairingOld)
/// This is the function which gathers in-sync chunks into a super-chunk in which the same
/// chapters and verses are present - paired. That is, if pOldChunks has a chapter 2 verse
/// 3 chunk and pNewChunks has a chapter 2 verse 3 chunk, and there were pairings up to
/// that point, then those two chunks are considered paired and the next index values are
/// tested for pairing -- this pairing to aggregate a super-chunk only halts when either
/// the end of the arrays is reached, or in-syncness is violated for some reason - for
/// example, one of the arrays may have a gap in the verse sequence, or a range of verses
/// like 5-6a might be in one array but a matching range not in the other array, or 8a may
/// be in one but 8 in the other. We do this because calling MergeRecursively() with a
/// small limit value is safe provided the pairings are in-sync throughout the sequence of
/// paired chunks.
///
/// (That doesn't mean there are no gaps, it does mean that any gaps that are present have
/// the same verse ref info preceding the gap in each array, and the same verse ref info
/// following the gap in each array. For instance if in arrOld there is a gap after 2:6 and
/// the next chunk starts at 3:2, then in arrNew there would be the same gap - that is,
/// before its gap 2:6 exists and after its gap 3:2 exists. Nor does it mean there are no
/// verse ranges; it just means that if one array has a verse range (eg. 4-6a) then the
/// other array has the same verse range (4-6a) at the expected pairing location.) So long
/// as the pairings keep in sync, aggregation can continue and should continue.
/// Note: oldStartChunk and newStartChunk are indices which must point to a valid pairing
/// established in the caller before entry. Iteration just tests successive potential
/// pairings using AreSfmChunksWithSameRef() until the latter returns FALSE, and returns
/// the indices for the last successful pairing in oldEndChunk and newEndChunk.
///
/// BEW 16Aug12 Here's how I plan to tackle a problem identified by Kim Blewett
/// (17July2012) in an email. She had added several verses, more than SPAN_LIMIT's value of
/// extra words, at the end of the source text in Paratext - and the Adapt It messed up the
/// merger of the new source text. I'll define a new array, arrPairingOld, of integers
/// which work in to defined the sequence of paired milestones; offsets to the values at
/// any one index account for differences in chunk indices between a pair of matched
/// chunks, so only one such array is needed. Each successful milestoned SfmChunk pairing
/// will have the relevant struct's index into either pOldChunksArray or pNewChunksArray at
/// the same index in each array - once offsets are added to account for absolute index
/// differences (for example, in arrOld, chunk at index 15 may be paired with chunk at
/// index 17 in arrNew). MergeRecursively() will then be able, within a loop, to get each
/// milestoned pairing and do the recursive merging for that particular pair of chunks.
/// This will avoid any problems which otherwise would occur because SPAN_LIMIT is not big
/// enough to ensure that a large group of added words at the end of the source text do not
/// get some chopped off and lost.
//////////////////////////////////////////////////////////////////////////////////////
bool GetMaxInSyncChunksPairing(SPArray& arrOld, SPArray& arrNew, wxArrayPtrVoid* pOldChunksArray,
						wxArrayPtrVoid* pNewChunksArray, int oldStartChunk, int newStartChunk,
						int& oldEndChunk, int& newEndChunk, bool& bDisparateSizes,
						wxArrayInt& arrPairingOld)
{
	arrPairingOld.Clear();
	//arrPairingNew.Clear();

	int oldIndex = oldStartChunk;
	int newIndex = newStartChunk;
	int oldChunkCount = pOldChunksArray->GetCount();
	int newChunkCount = pNewChunksArray->GetCount();
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("GetMaxInSyncChunksPairing() before loop, oldChunkCount %d   newChunkCount %d"),
		oldChunkCount, newChunkCount);
#endif
	bool bTheyAreTheSameRef = FALSE;
	bDisparateSizes = FALSE;
	SfmChunk* pOldChunk = (SfmChunk*)pOldChunksArray->Item(oldIndex);
	SfmChunk* pNewChunk = (SfmChunk*)pNewChunksArray->Item(newIndex);
	while (oldIndex < oldChunkCount && newIndex < newChunkCount)
	{
		bTheyAreTheSameRef = AreSfmChunksWithSameRef(pOldChunk, pNewChunk);
		bDisparateSizes = AreSizesDisparate(arrOld, arrNew, pOldChunk, pNewChunk);
		if (!bTheyAreTheSameRef)
		{
			// halt the accumulation, the caller needs to merge what we've collected so
			// far, and then do a special collection of the not-in-sync material etc
			oldEndChunk = oldIndex - 1; // points to part of the last matched pair
			newEndChunk = newIndex - 1; // points to the other part of the last matched pair
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
			wxLogDebug(_T("GetMaxInSyncChunksPairing() in loop: last matched pairs, oldEndChunk index: %d newEndChunk index: %d  Returning FALSE"),
			oldEndChunk, newEndChunk);
#endif
			return FALSE;
		}
		else if (bDisparateSizes)
		{
			// they are the same reference, but they are disparately sized chunks, we have
			// to return FALSE if this is the case to force a halt so these can be given
			// special handling (ie. a limit of -1 used). In this special case, we return
			// not the previous old and new indices, but the current ones - because the
			// pair with disparate sizes possibly may be the first pair tested and we
			// don't want to return index values that assume previous chunks exist
			oldEndChunk = oldIndex;
			newEndChunk = newIndex;
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
			wxLogDebug(_T("GetMaxInSyncChunksPairing() in loop: DISPARATE SIZES (current indices), oldEndChunk index: %d newEndChunk index: %d  Returning FALSE"),
			oldEndChunk, newEndChunk);
#endif
			return FALSE;
		}
		else
		{
			// they are the same reference and not disparate in size, so iterate to test
			// the next pair...
			// BEW 17Aug12, but first add the indices for their chunks to arrPairingOld
			// and arrPairingNew, respectively -- MergeRecursively will use these
			arrPairingOld.Add(oldIndex);
			//arrPairingNew.Add(newIndex);

#if defined( myMilestoneDebugCalls) && defined( _DEBUG)
			// log the pair we've just accepted
			SfmChunk* pAnOldChunk = (SfmChunk*)pOldChunksArray->Item(oldIndex);
			SfmChunk* pANewChunk = (SfmChunk*)pNewChunksArray->Item(newIndex);
			wxLogDebug(_T("GetMaxInSyncChunksPairing() TRUE, (non-disparate-sized pair) accepted pair: pOldChunk %s:%s  OLD = [ %d , %d ]  NEW = [ %d , %d ] "),
				pAnOldChunk->strChapter.c_str(), pAnOldChunk->strStartingVerse.c_str(), pAnOldChunk->startsAt, pAnOldChunk->endsAt, pANewChunk->startsAt, pANewChunk->endsAt);
#endif

			oldIndex++;
			newIndex++;
			if (oldIndex >= oldChunkCount)
			{
				// return TRUE, and with the last paired SfmChunk instances' indices set
				oldEndChunk = oldIndex - 1;
				newEndChunk = newIndex - 1;
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
				wxLogDebug(_T("GetMaxInSyncChunksPairing() ELSE block, returns TRUE (oldIndex >= oldChunkCount) last pairing: oldEndChunk index: %d newEndChunk index: %d"),
			oldEndChunk, newEndChunk);
#endif
				return TRUE;
			}
			if (newIndex >= newChunkCount)
			{
				// return TRUE, and with the last paired SfmChunk instances' indices set
				oldEndChunk = oldIndex - 1;
				newEndChunk = newIndex - 1;
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
				wxLogDebug(_T("GetMaxInSyncChunksPairing() ELSE block, returns TRUE (newIndex >= newChunkCount) last pairing: oldEndChunk index: %d newEndChunk index: %d"),
			oldEndChunk, newEndChunk);
#endif
				return TRUE;
			}
			// if control reaches here, neither index is at the end of its array, so the
			// chunks can be accessed for testing on next iteration
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
			wxLogDebug(_T("GetMaxInSyncChunksPairing() 'neither index is at end of its array', oldIndex: %d  newIndex index: %d  will iterate"),
			oldIndex, newIndex);
#endif
			// prepare for next iteration by getting pointers to the potentially next
			// successfully matchable pair, then iterate
			pOldChunk = (SfmChunk*)pOldChunksArray->Item(oldIndex);
			pNewChunk = (SfmChunk*)pNewChunksArray->Item(newIndex);
		}
	} // end of loop: while (oldIndex < oldChunkCount && newIndex < newChunkCount)

	// control should never get to here, but just in case
	oldEndChunk = oldIndex - 1;
	newEndChunk = newIndex - 1;
#if defined(myMilestoneDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("GetMaxInSyncChunksPairing() end 'control should not get here' returns TRUE, oldEndChunk index: %d  newEndChunk index: %d"),
			oldIndex, newIndex);
#endif
	return TRUE;
}


// Return TRUE if a ChunkInfo with bIsComplex = FALSE is found, and return in foundAt the
// index which points to it in pChunkInfos array; if none is found before the end of
// pChunkInfos is reached, return FALSE (in which case, foundAt will be returned as
// wxNOT_FOUND). The value passed in, startFrom, should be the first ChunkInfo instance not
// as yet checked, and it must not be a value which exceeds the bound of pChunkInfos
bool GetNextSimpleVerseChunkInfo(wxArrayPtrVoid* pChunkInfos, int startFrom, int& foundAt)
{
	int count = pChunkInfos->GetCount();
	wxASSERT(startFrom < count);
	foundAt = wxNOT_FOUND;
	int index;
	for (index = startFrom; index < count; index++)
	{
		ChunkInfo* pInfo = (ChunkInfo*)pChunkInfos->Item(index);
		if(pInfo->type != bookInitialChunk && pInfo->type != introductionChunk && !pInfo->bIsComplex)
		{
			foundAt = index;
			return TRUE;
		}
	}
	// if control reaches here, we didn't find a simple-verse ChunkInfo in the array
	return FALSE;
}

// return TRUE if the verse reference (and chapter) information in each chunk is the same,
// FALSE if not. For example, both have 12-14a, or both have 9, or both have 14b, etc, but
// dissimilar types also return FALSE regardless of what the verse and chapter info is,
// for example, one is introductionChunk the other is chapterPlusVerseChunk; or one is
// subheadinglusVerseChunk and the other is verseChunk, etc
// BEW changed 26Sep12, because the user may have old data like this:
// \c 2
// \v 12 <words>
// and the new data may be the fuller chapter or whole book, and look like:
// \c 2  ... other fields, like \p
// \v 1 .. other verses....
// \v 12 <words>
// and so just testing for identity in the two chunks' types gives chapterPlussVerseChunk for the old chunk,
// but the matching verse 12 in the new is a verseChunk, and therefore a FALSE result, and
// returning FALSE as a consequence would mess up what essentially is a valid pairing. The
// two verses match - and we can leave it to the code which updates the sfms to remove the
// old \c 2 and put a new \c 2 earlier at the m_markers of the first word of verse 1 in
// chapter 2. So I'm making the test smarter, and putting it AFTER the tests for matching
// verse numbers and suffix part differences. We need the above situation to return TRUE.
bool AreSfmChunksWithSameRef(SfmChunk* pOldChunk, SfmChunk* pNewChunk)
{
	//if (pOldChunk->type != pNewChunk->type)
	//	return FALSE;
	if (pOldChunk->strStartingVerse != pNewChunk->strStartingVerse)
		return FALSE;
	else if (pOldChunk->strEndingVerse != pNewChunk->strEndingVerse)
		return FALSE;
	else if (pOldChunk->charStartingVerseSuffix != _T('\0') && pNewChunk->charStartingVerseSuffix != _T('\0')
			&& (pOldChunk->charStartingVerseSuffix != pNewChunk->charStartingVerseSuffix))
		return FALSE;
	else if (pOldChunk->charStartingVerseSuffix != _T('\0') && pNewChunk->charStartingVerseSuffix == _T('\0'))
		return FALSE;
	else if (pOldChunk->charStartingVerseSuffix == _T('\0') && pNewChunk->charStartingVerseSuffix != _T('\0'))
		return FALSE;
	else if (pOldChunk->charEndingVerseSuffix != _T('\0') && pNewChunk->charEndingVerseSuffix != _T('\0')
			&& (pOldChunk->charEndingVerseSuffix != pNewChunk->charEndingVerseSuffix))
		return FALSE;
	else if (pOldChunk->charEndingVerseSuffix != _T('\0') && pNewChunk->charEndingVerseSuffix == _T('\0'))
		return FALSE;
	else if (pOldChunk->charEndingVerseSuffix == _T('\0') && pNewChunk->charEndingVerseSuffix != _T('\0'))
	{
		return FALSE;
	}
	// now check the chunk types - disparate types should return FALSE, except that we
	// must allow for fluidity in where the chapter number may occur; so one may be a
	// chapterPlusVerseChunk and the other may be a verseChunk - and if so, we'll allow
	// such combinations as legitimate
	if (
		 (pOldChunk->type == chapterPlusVerseChunk && pNewChunk->type == verseChunk)
		 ||
		 (pOldChunk->type == verseChunk && pNewChunk->type == chapterPlusVerseChunk)
		)
	{
		// those two are allowable as valid matchups, so return TRUE
		return TRUE;
	}
	else if (
		 (pOldChunk->type == subheadingPlusVerseChunk && pNewChunk->type == verseChunk)
		 ||
		 (pOldChunk->type == verseChunk && pNewChunk->type == subheadingPlusVerseChunk)
		)
	{
		// those two also are allowable as valid matchups, so return TRUE
		return TRUE;
	}
	else if (pOldChunk->type != pNewChunk->type)
	{
		// other mismatch combinations are not valid as "same reference" matchups
		return FALSE;
	}
	// if control gets to here, the two chunks are the same type
	return TRUE;
}

// Count the words in the series of CSourcePhrase instances of pArray represented by the
// startsAt value in the SfmChunk* pointed at by firstChunk, and the endsAt value in the
// one pointed at by lastChunk, indexing into the contents of pChunksArray. If there is only one
// chunk involved, firstChunk and lastChunk will be the same integer value. The code
// accumulates the values of the m_nSrcWords member of each CSourcePhrase in the range.
int CountWords(SPArray* pArray, wxArrayPtrVoid* pChunksArray, int firstChunk, int lastChunk)
{
	int count = 0;
	int chunkCount = pChunksArray->GetCount(); chunkCount = chunkCount; // avoid compiler warning
	int index = firstChunk;
	SfmChunk* pChunk = (SfmChunk*)pChunksArray->Item(index);
	int startAt = pChunk->startsAt;
	wxASSERT(lastChunk < chunkCount);
	pChunk = (SfmChunk*)pChunksArray->Item(lastChunk);
	int endAt = pChunk->endsAt;
	// repurpose index to iterate over the CSourcePhrase instances in pArray from startAt
	// to endAt, inclusive
	for (index = startAt; index <= endAt; index++)
	{
		count += pArray->Item(index)->m_nSrcWords;
	}
	return count;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return         TRUE if one or more in-sync matchups are determined (oldEndChunk and
///                 newEndChunk values can then be relied upon); FALSE if no in-sync matchups
///                 were able to be made, or if there is a valid pairing but the pair are
///                 disparate in size (word counts comparison -- see the bDisparateSizes
///                 parameter for more detail)
/// \param  arrOld          ->  ref to old CSourcePhrase instances array (ie. the Adapt It
///                             original source text, or in collaboration mode, the source
///                             text of the Adapt It collaborating document)
/// \param  arrNew          ->  ref to new CSourcePhrase instances array (ie. the edited
///                             source text being imported, in colloboration mode, this will
///                             be coming from the Paratext or Bibledit source language project)
/// \param  pMergedList     <-  the SPList of CSourcePhrase instances resulting from the merger
/// \param  limit           ->  the number of CSourcePhrase instances to consider for a
///                             limited-width span, usually of value SPAN_LIMIT (currently 80)
///                             but can be larger, or -1 (the latter selects largest word count)
/// \param  initialSequNum  ->  needed to supply the kick off value for the UpdateSequNumbers()
///                             call after CSourcePhrase inventory in pMergedList has been changed
///  **** The next 9 were added at 17Aug12 refactoring, to make the algorithm failsafe if
///  a lot of words were manually added at the end of the edited source text ****
/// \param  pChunksOld      ->  the array of SfmChunk structs indexing into arrOld
/// \param  pChunksNew      ->  the array of SfmChunk structs indexing into arrNew
/// \param  howToProcess    ->  enum value; the legacy code will be used when processStartToEnd
///                             is the value passed in, if processPerMilestone is passed in
///                             then internally we loop across all the successfully paired
///                             milestone chunks (typically verse chunks) and do the merger
///                             recursively on each such chunk pair. This ensures safety.
/// \param  arrPairingsOld  ->  array of integer indices which define one member of a
///                             successfully matched SfmChunk pair
/// \param  arrPairingsNew  ->  array of integer indices, acting in parallel to the one immediately
///                             above, which defines the other SfmChunk of the matched pair
/// \param  arrOldFull      ->  the full 'old' SPArray (not a subarray drawn from it,
///                             because the latter is what the arrOld param is)
/// \param  arrNewFull      ->  the full 'new' SPArray (not a subarray drawn from it,
///                             because the latter is what the arrNew param is)
/// \param  oldChunkIndexKickoff -> the index for the first chunk in the old (full) array
///                             which as yet is not processed
/// \param  newChunkIndexKickoff -> the index for the first chunk in the new (full) array
///                             which as yet is not processed
/// \remarks
/// This is the function which does the recursive merging of two subspans of CSourcePhrase
/// instances. It may be called often in the process of merging all the data.
/// Note: Merging milestoned groups (such as verse chunks) will normally not destroy
/// stored retranslations or free translations - but it is possible that either or both
/// could be corrupted if the extent of one or the other extends across a milestone
/// boundary. It's rather unlikely that a retranslation or free translation will cross a
/// verse boundary, but we must check for the possibility. The function will, after the
/// mergers are done, examine the pMergedList list for corrupted retranslations and/or
/// corrupted free translations - any such are then removed from the document (the user
/// would be required to reconstitute them manually using the normal GUI tools after the
/// data is all imported).
/// Note: the oldChunkIndexKickoff and newChunkIndexKickoff are indices into the full
/// arrays in the caller - they are each one more than the indices for the last chunks of
/// those that have already been processed. Since the GetMaxInSyncChunksPairing() starts
/// with an empty arrPairingsOld and arrPairingsNew, the latter arrays will loop using an
/// index which commences at 0. So we have to be careful when using the loop index in this
/// function, we would make a mistake if we use it without an offset to allow for what has
/// already been processed. Instead, we must add appropriate offsets if indexing into
/// arrOldFull and arrNewFull, and the offsets are oldChunkIndexKickoff and
/// newChunkIndexKickoff, respectively -- but this only applies when howToProcess was
/// passed in as processPerMilestone; for the other enum value, pre-processing done in the
/// caller makes all the needed adjustments before MergeRecursively is entered.
//////////////////////////////////////////////////////////////////////////////////////
void MergeRecursively(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList, int limit,
		int initialSequNum,  wxArrayPtrVoid* pChunksOld, wxArrayPtrVoid* pChunksNew,
		enum ProcessHow howToProcess, wxArrayInt& arrPairingsOld, SPArray& arrOldFull,
		SPArray& arrNewFull, int oldChunkIndexKickoff, int newChunkIndexKickoff)
{
	// set up the top level tuple, beforeSpan and commonSpan will be empty (that is, the
	// tuple[0] and tuple[1] struct pointers will be NULL. Tuple[3] will have the whole
	// extent of both oldSPArray and newSPArray.
#if defined(_DEBUG) && defined(_RECURSE_)
	wxLogDebug(_T("\n###  ENTERED & at start of MergeRecursively: howToProcess = %d (0 is 'processStartToEnd') oldChunkIndexKickoff = %d, newChunkIndexKickoff = %d,limit=%d"),
				howToProcess, oldChunkIndexKickoff, newChunkIndexKickoff, limit);
#endif
	// get a pointer to the CFreeTrans module so we can use it's functions that support
	// the Import Edited Source Text feature, and PT collaboration feature
	CFreeTrans* pFreeTrans = gpApp->GetFreeTrans();
	bool bContainsFreeTranslations = FALSE;
	bContainsFreeTranslations = pFreeTrans->IsFreeTransInArray(&arrOld); // if TRUE we will
			// later call a free-translation-malformations-fixing function after the
			// merger is done, to erase free translations corrupted by CSourcePhrase
			// replacements from arrNew
	// BEW added support for a per-milestone loop, to be used when there is a sequence of
	// successfully matched SfmChunk pairs; do the legacy way in the first block, then the
	// new code in a block further below
	if (howToProcess == processStartToEnd)
	{
        // arrOld and arrNew passed in are typically subarray's of longer arrays in the
        // caller, because arrOld and arrNew params are set by CopySubArray() calls in the
        // caller, extracting the subarrays from the caller's arrOld and arrNew which
        // contain the full set of old and new CSourcePhrase instances, respectively
		int oldSPCount = arrOld.GetCount();
		int newSPCount = arrNew.GetCount();

		Subspan* tuple[3]; // an array of three pointer-to-Subspan
		tuple[0] = NULL;
		tuple[1] = NULL;
		Subspan* pSubspan = new Subspan;
#if defined (myLogDebugCalls) && defined (_DEBUG)
		countAfterSpans++;
#endif
		tuple[2] = pSubspan;
		// initialize tuple[2] to store an open-ended Subspan pointer spanning the whole
		// extents of arrOld and arrNew -- the subsequent SetEndIndices() call will limit the
		// value, provided limit is not -1
		// In this call: FALSE is bClosedEnd, i.e. it's an open-ended afterSpan
		InitializeSubspan(pSubspan, afterSpan, 0, oldSPCount - 1, 0, newSPCount - 1, FALSE);
		// update the end indices to more reasonable values, the above could bog processing down
		SetEndIndices(arrOld, arrNew, pSubspan, limit); // SPAN_LIMIT is currently 80

		// Pass the top level tuple to the RecursiveTupleProcessor() function. When this next
		// call finally returns after being recursively called possibly very many times, the
		// merging is complete
		RecursiveTupleProcessor(arrOld, arrNew, pMergedList, limit, tuple);
	}
	else
	{
		// process on a per-milestoned pairing basis - it's safer if the user added a lot
		// of extra words in a big group at one location in the source text
		int pairingsCount = arrPairingsOld.GetCount();
#if defined(_DEBUG) && defined (myLogDebugCalls)
		{
		int anIndex;
		// list the pairings
		for (anIndex = 0; anIndex < pairingsCount; anIndex++)
		{
		int indexOldStart;
		int indexOldEnd;
		int indexNewStart;
		int indexNewEnd;
		SfmChunk* pPairedChunkOld;
		SfmChunk* pPairedChunkNew;
		pPairedChunkOld = (SfmChunk*)pChunksOld->Item(anIndex + oldChunkIndexKickoff);
		indexOldStart = pPairedChunkOld->startsAt;
		indexOldEnd   = pPairedChunkOld->endsAt;
		pPairedChunkNew = (SfmChunk*)pChunksNew->Item(anIndex + newChunkIndexKickoff);
		indexNewStart = pPairedChunkNew->startsAt;
		indexNewEnd   = pPairedChunkNew->endsAt;
			wxLogDebug(_T("\nPairings List: index = %d    OLD [ %d : %d ] <-> NEW [ %d : %d ]  (new) Verse = %d"),
				anIndex, indexOldStart, indexOldEnd, indexNewStart, indexNewEnd, pPairedChunkNew->nStartingVerse);
		}
		}
#endif
		int indexOldStart;
		int indexOldEnd;
		int indexNewStart;
		int indexNewEnd;
		SfmChunk* pPairedChunkOld;
		SfmChunk* pPairedChunkNew;
		SPArray arrSubOld; // will contain a subarray drawn from arrOldFull
		SPArray arrSubNew; // will contain a subarray drawn from arrNewFull
		// use pChunk = (SfmChunk*)pChunksOld->Item(index);
		int pairingsIndex;
		for (pairingsIndex = 0; pairingsIndex < pairingsCount; pairingsIndex++)
		{
			// get the chunk of old CSourcePhrase instances
			pPairedChunkOld = (SfmChunk*)pChunksOld->Item(pairingsIndex + oldChunkIndexKickoff);
			indexOldStart = pPairedChunkOld->startsAt;
			indexOldEnd   = pPairedChunkOld->endsAt;
			CopySubArray(arrOldFull, indexOldStart, indexOldEnd, arrSubOld);

			// do the same for the new CSourcePhrase instances
			pPairedChunkNew = (SfmChunk*)pChunksNew->Item(pairingsIndex + newChunkIndexKickoff);
			indexNewStart = pPairedChunkNew->startsAt;
			indexNewEnd   = pPairedChunkNew->endsAt;
			CopySubArray(arrNewFull, indexNewStart, indexNewEnd, arrSubNew);
			// arrSubOld and arrSubNew are now populated ready for merging or copying

			int oldSPCount = arrSubOld.GetCount();
			int newSPCount = arrSubNew.GetCount();

			bool bOldChunkEmpty = IsChunkEmpty(arrOldFull, indexOldStart, indexOldEnd);
			bool bNewChunkEmpty = IsChunkEmpty(arrNewFull, indexNewStart, indexNewEnd);
			if (bOldChunkEmpty && bNewChunkEmpty)
			{
				// probably an empty CSourcePhrase instance with \v and verse number in
				// m_markers, from the old array's data, and same for the new data;
				// just keep the old one
#if defined (myLogDebugCalls) && defined (_DEBUG)
				wxLogDebug(_T("\n*** pairingIndex = %d  In MergeRecursively() per-Milestone; pairings LOOP: OLD [ %d : %d ] NEW [ %d : %d ]  (new) Verse = %d DO NOTHING, both empty"),
				pairingsIndex, indexOldStart, indexOldEnd, indexNewStart, indexNewEnd, pPairedChunkNew->nStartingVerse);
#endif
				wxASSERT(indexOldStart == indexOldEnd);
				CopyToList(arrOldFull, indexOldStart, indexOldEnd, pMergedList); // makes deep copy
			}
			else if (bOldChunkEmpty)
			{
                // the CSourcePhrase instance from the old data's array is an empty one,
                // but the equivalent from the newly edited data's array is not empty
#if defined (myLogDebugCalls) && defined (_DEBUG)
				wxLogDebug(_T("\n*** pairingIndex = %d  In MergeRecursively() per-Milestone; pairings LOOP: OLD [ %d : %d ] NEW [ %d : %d ]  (new) Verse = %d\n REPLACE EMPTY OLD with NEW content, %d words"),
				pairingsIndex, indexOldStart, indexOldEnd, indexNewStart, indexNewEnd, pPairedChunkNew->nStartingVerse, newSPCount);
#endif
				CopyToList(arrNewFull, indexNewStart, indexNewEnd, pMergedList); // makes deep copies
			}
			else if (bNewChunkEmpty)
			{
                // the CSourcePhrase instance from the newly edited data's array is an
                // empty one, but the equivalent from the old data's array is not empty
                // so just copy the new CSourcePhrase instance to pMergedList, effectively
                // removing the old array's chunk in the process
#if defined (myLogDebugCalls) && defined (_DEBUG)
				wxLogDebug(_T("\n*** pairingIndex = %d  In MergeRecursively() per-Milestone; pairings LOOP: OLD [ %d : %d ] NEW [ %d : %d ]  (new) Verse = %d\n REMOVE OLD %d SrcPhrases, REPLACE with 1 empty NEW"),
				pairingsIndex, indexOldStart, indexOldEnd, indexNewStart, indexNewEnd, pPairedChunkNew->nStartingVerse, oldSPCount);
#endif
				wxASSERT(indexNewStart == indexNewEnd);
				CopyToList(arrNewFull, indexNewStart, indexNewEnd, pMergedList); // makes a deep copy
			}
			else
			{
				// neither are empty, we should be safe for a recursive merger
#if defined (myLogDebugCalls) && defined (_DEBUG)
				wxLogDebug(_T("\n*** pairingIndex = %d  In MergeRecursively() per-Milestone; pairings LOOP: OLD [ %d : %d ] NEW [ %d : %d ]  (new) Verse = %d\n RECURSING..."),
				pairingsIndex, indexOldStart, indexOldEnd, indexNewStart, indexNewEnd, pPairedChunkNew->nStartingVerse);
#endif
				Subspan* tuple[3]; // an array of three pointer-to-Subspan
				tuple[0] = NULL;
				tuple[1] = NULL;
				Subspan* pSubspan = new Subspan;
#if defined (myLogDebugCalls) && defined (_DEBUG)
				countAfterSpans++;
#endif
				tuple[2] = pSubspan;

#if defined(_DEBUG) && defined(_RECURSE_)
				if (pairingsIndex == 2) // this correspondes to chapter 1 verse 3, which has 23 old and 24 new words
				{
				//	int break_here = 1;
				}
#endif
				// initialize tuple[2] to store an open-ended Subspan pointer spanning the whole
				// extents of arrSubOld and arrSubNew
				// In this call: FALSE is bClosedEnd, i.e. it's an open-ended afterSpan
				InitializeSubspan(pSubspan, afterSpan, 0, oldSPCount - 1, 0, newSPCount - 1, FALSE);

				// Pass the top level tuple to the RecursiveTupleProcessor() function. When this next
				// call finally returns after being recursively called possibly a few times, the
				// merging for this one milestoned chunk is complete
				RecursiveTupleProcessor(arrSubOld, arrSubNew, pMergedList, limit, tuple);
			}
		} // end of loop: for (pairingsIndex = 0; pairingsIndex < pairingsCount; pairingsIndex++)

	} // end of else block for test: if (howToProcess == processStartToEnd)

	// Clean up...

#if defined(_DEBUG) && defined(_RECURSE_)
	wxLogDebug(_T("in MergeRecursively() RecursiveTupleProcessor() after top level call, returned: pMergedList->GetCount() = %d ; initial sequnum = %d"),
		pMergedList->GetCount(), initialSequNum);
#endif

	// Get the CSourcePhrase instances appropriately numbered in the temporary
	// list which is to be returned to the caller
	if (!pMergedList->IsEmpty())
	{
		gpApp->GetDocument()->UpdateSequNumbers(initialSequNum, pMergedList);

        // Call an adjustment helper function which looks for retranslation spans that were
        // partially truncated (at their beginning and/or at their ending), and removes the
        // adaptations in the remainder fragments of such retranslation trucated spans,
        // fixing the relevant flags, and if placeholders need to be removed from the end
        // of a retranslation, does any transfers of ending puncts and/or inline markers
        // back to the last non-placeholder instance
		EraseAdaptationsFromRetranslationTruncations(pMergedList);
	}
#if defined(_DEBUG) && defined(MERGE_Recursively)
	wxLogDebug(_T("in MergeRecursively() after EraseAdaptationsFromRetranslationTruncations(pMergedList): pMergedList->GetCount() = %d"),
		pMergedList->GetCount());
#endif
	// clear any corrupted free translations
	if (bContainsFreeTranslations)
	{
		// need to turn pMergedList into an SPArray for the next job, then throw the
		// copied pointers away, since pMergeList's contents will have been updated and
		// the copies in SPArray are no longer needed
		SPArray arrMerged;
		ConvertSPList2SPArray(pMergedList, &arrMerged);
		pFreeTrans->EraseMalformedFreeTransSections(&arrMerged);
		RemoveAll(&arrMerged); // internally calls Detach() in a loop
	}
#if defined(_DEBUG) && defined(MERGE_Recursively)
	wxLogDebug(_T("in MergeRecursively() after EraseMalformedFreeTransSections(&arrMerged): pMergedList->GetCount() = %d"),
		pMergedList->GetCount());
#endif
}

// Return TRUE if the milestoned chunk is empty. Typically a milestoned chunk is a verse
// chunk, but we won't insist on it being a verse chunk. It will be a candidate for being
// empty if the starting index and ending index are the same (ie. the one CSourcePhrase is
// all there is in the chunk), AND, its m_key value is an empty string. In this
// circumstance, we must not try a recursive merger because the algorithm breaks down. We
// instead copy the new material if the empty chunk is old array data, or replace the old chunk
// with an empty new CSourcePhrase (typically with \v num in m_markers) if the empty chunk
// is new array data (the replacements, of course, are done elsewhere, not here)
bool IsChunkEmpty(SPArray arr, int indexStart, int indexEnd)
{
	if (indexEnd > indexStart)
	{
		return FALSE;
	}
	CSourcePhrase* pSrcPhrase = arr.Item(indexStart);
	if (pSrcPhrase->m_key.IsEmpty())
	{
		return TRUE;
	}
	return FALSE;
}

// look for retranslations that have lost content at either end -- remove the adaptations
// within them and reset the flags to make them normal CSourcePhrase instances (the
// adaptations must go, because it can't be assured that they correspond to the source
// text above them)
void EraseAdaptationsFromRetranslationTruncations(SPList* pMergedList)
{
	int count = pMergedList->GetCount();
	if (count < 2)
		return;
	int index;
	CSourcePhrase* pSrcPhrase = NULL;
	CSourcePhrase* pPrevSrcPhrase = NULL;
	CRetranslation* pRetranslation = gpApp->GetRetranslation();
	// we assume that a chopped off retranslation does not commence the document
	SPList::Node* pos = pMergedList->GetFirst();
	pPrevSrcPhrase = pos->GetData();
	pos = pos->GetNext();
	for (index = 1; pos != NULL && index < count; index++)
	{
		pSrcPhrase = pos->GetData();
        // check the pair pPrevSrcPhrase and pSrcPhrase; if the former has m_bRetranslation
        // FALSE and the latter has it TRUE and with m_bBeginRetranslation FALSE, then we
        // have a retranslation with its start chopped off
		if (!pPrevSrcPhrase->m_bRetranslation &&
			(pSrcPhrase->m_bRetranslation && !pSrcPhrase->m_bBeginRetranslation))
		{
            // clear out the bad stuff until we get to a non-retranslation instance, or the
            // start of an immediately following (valid) retranslation...fix it, by making
            // it a "valid" retranslation and then removing it's retranslation status
            // (which also clears out its target text words), our algorithm automatically
            // handles any auto-inserted padding placeholders & removes them, & and fixes
            // the doc
			SPList::Node* posLocal = pos;
			CSourcePhrase* pSP = posLocal->GetData(); // same as pSrcPhrase
			CSourcePhrase* pKickoffSP = pSP;
			pSP->m_bBeginRetranslation = TRUE; // give it a valid start flag
			SPList::Node* posEnd = pos;
			int first = pMergedList->IndexOf(pSP);
			while (posLocal != NULL &&  pSP->m_bRetranslation
				&& (pSP->m_bRetranslation && !pSP->m_bEndRetranslation)
				&& !(pSP != pKickoffSP &&  pSP->m_bRetranslation && pSP->m_bBeginRetranslation))
			{
				posEnd = posLocal;
                // it "ends" when a new retranslation starts at location different from the
                // kickoff one, or when a non-retranslation instance is found, or when a
                // retranslation one is found which also has m_bEndRetranslation already
                // set TRUE; then set m_bEndRetranslation when it isn't already set
				posLocal = posLocal->GetNext();
				pSP = posLocal->GetData(); // get the CSourcePhrase instance
			}
			wxASSERT(posEnd != NULL);
			// the loop exit could be because we found a valid final CSourcePhrase instance
			// which is in the retranslation and having m_bEndRetranslation TRUE already,
			// or because we don't find a valid end and go one instance further - so check
			// and process accordingly
			int last;
			if (pSP->m_bEndRetranslation)
			{
				// it has a valid end already, so do nothing except set the last value
				last = pMergedList->IndexOf(pSP);
			}
			else
			{
				// we didn't find a valid end, so the previous location is the true end,
				// get that location's CSourcePhrase instance and set it's end flag and
				// calculate the variable last's value
				CSourcePhrase* pSP_AtEnd = posEnd->GetData();
				pSP_AtEnd->m_bEndRetranslation = TRUE;
				last = pMergedList->IndexOf(pSP_AtEnd);
			}

			// now do RemoveRetranslation() on the now-valid short retranslation fragment
			wxString strAdaptationToThrowAway;
			pRetranslation->RemoveRetranslation(pMergedList,first,last,strAdaptationToThrowAway);
			strAdaptationToThrowAway.Empty();
			gpApp->GetDocument()->UpdateSequNumbers(0,pMergedList);

            // CSourcePhrase instances may have been removed (e.g. placeholders following a
            // partial retranslation), so get an updated count value before iterating
			count = pMergedList->GetCount();
			// restart the iterations from a safe location - the pos value corresponding
			// to the value stored in first is suitable; & pPrevSrcPhrase still points at
			// the instance stored at the previous position to that one
			index = first;
			pos = pMergedList->Item(first);
		}
		else
        // deal with retranslations which have their end chopped off; because the caller
        // always keeps any auto-inserted placeholders belonging to the retranslation
        // together with the retranslation, a 'chopped off end' will always have chopped
        // off any such placeholders as well as at least one of the final non-placeholder
        // retranslation CSourcePhrase instances in the retranslation
		if ( (pPrevSrcPhrase->m_bRetranslation && !pPrevSrcPhrase->m_bEndRetranslation) &&
		(!pSrcPhrase->m_bRetranslation ||
		(pSrcPhrase->m_bRetranslation && pSrcPhrase->m_bBeginRetranslation)))
		{
			// The above condition for detecting a chopped off end is: (1) previous
			// CSourcePhrase must have m_bRetranslation flag set, but the
			// m_bEndRetranslation flag not set (it would be set if it was not a chopped
			// off one) and (2) the following CSourcePhrase instance must be not part of the
			// retranslation which means that it must either have it's m_bRetranslation
			// flag FALSE, or be the start of a new retranslation - which means that the
			// latter flag would be TRUE and also it's m_bBeginRetranslation flag would be
			// TRUE as well.

			// Clear out the bad stuff starting at pPrevSrcPhrase and going backwards
			// until we get to a non-retranslation instance or the end of a (valid)
			// retranslation; pSP will start off being pPrevSrcPhrase, and we iterate to
			// get the pSP which is at the start of the retranslation (we can't be certain
			// the user's edits didn't also chop off the start of the retranslation, but
			// that will have been handled beforehand (above), so probably the initiap pSP
			// will have m_bBeginRetranslation set TRUE, but we won't count on it being so)
			SPList::Node* posLocal = pos;
			SPList::Node* posAfter = pos;
			posLocal = posLocal->GetPrevious(); // get the one on which pPrevSrcPhrase is stored
			CSourcePhrase* pSP = posLocal->GetData(); // same as pPrevSrcPhrase
			pSP->m_bEndRetranslation = TRUE; // give it a valid end flag
			CSourcePhrase* pSP_AtStart = pSP; // it could be the location both starts and ends the
											  // retranslation, i.e. there is only one in it
			CSourcePhrase* pKickOffSP = pSP; // don't check for a m_bEndRetranslation on an instance
											 // where pKickOffSP and pSP are the same CSourcePhrase!
			//SPList::Node* posStart = posLocal;
			int last = pMergedList->IndexOf(pSP);
			while ( posLocal != NULL  && (pSP->m_bRetranslation && !pSP->m_bBeginRetranslation)
				 && pSP->m_bRetranslation
				 && !(pSP != pKickOffSP && pSP->m_bRetranslation && pSP->m_bEndRetranslation) )
			{
                // the above condition means: keep moving pSP backwards so long as there is
                // a CSourcePhrase existing at the earlier location, and provided we don't
                // encounter the beginning of the retranslation being backed over, and
                // provided we are backing over a retranslation, and provided we are at a
                // different retranslation CSourcePhrase instance than the initial kickoff
                // one - and that different one is the end of a previous (valid)
                // retranslation which immediately precedes the one we are removing
				//posStart = posLocal;
				posAfter = posLocal; // preserve a value "after" position, in case the it
									 // happens to be the start of the pMergedList, so we
									 // don't lose that location
				posLocal = posLocal->GetPrevious();
				if (posLocal != NULL)
				{
					pSP = posLocal->GetData();
				}
			} // end of loop
			// we must test several possible exit conditions
			int first = last; // initialize to last's value
			if (posLocal == NULL)
			{
				// posAfter is at the start of pMergedList, & it must be the start of the
				// retranslation
				pSP_AtStart = posAfter->GetData();
				pSP_AtStart->m_bBeginRetranslation = TRUE;
				first = 0;
			}
			else // posLocal is not NULL on exit from the loop
			{
				// first, check if the retranslation we are fixing already has a valid
				// beginning, and if so, set first to the index for that location
				if (pSP->m_bBeginRetranslation)
				{
					// we've a valid start at pSP's location
					first = pMergedList->IndexOf(pSP);
				}
				else
				{
					// the starting CSourcePhrase must be the one which follows pSP, and
					// it will need it's flag to be set as well as first's value calculated
					pSP_AtStart = posAfter->GetData();
					pSP_AtStart->m_bBeginRetranslation = TRUE;
					first = pMergedList->IndexOf(pSP_AtStart);
				}
			}
			// now remove the adaptations and clear relevant flags, etc
			wxString strAdaptationToThrowAway;
			pRetranslation->RemoveRetranslation(pMergedList,first,last,strAdaptationToThrowAway);
			strAdaptationToThrowAway.Empty();
			gpApp->GetDocument()->UpdateSequNumbers(0,pMergedList);

			// in the unlikely event that a CSourcePhrase got removed, update count, and
			// continue looping from the location where pSrcPhrase is
			count = pMergedList->GetCount();
			pos = pMergedList->Item(first);
		}
		// iterate in the outer loop which scans over all of pMergedList
		pPrevSrcPhrase = pSrcPhrase;
		pos = pos->GetNext();
	} // end of for loop: for (index = 1; pos != NULL && index < count; index++)
}

// Checks for an indicator of left-association: returns TRUE if one is found. Otherwise
// returns FALSE. An indicator of left-association would be one of the following:
// m_follPunct is non-empty, m_follOuterPunct is non-empty, m_endMarkers is non-empty,
// m_inlineBindingEndMarkers is non-empty, m_inlineNonbindingEndMarkers is non-empty, a
// boolean indicating the end of a span is TRUE, such as m_bEndRetranslation,
// m_bFootnoteEnd, or m_bEndFreeTrans
bool IsLeftAssociatedPlaceholder(CSourcePhrase* pSrcPhrase)
{
	if (!pSrcPhrase->m_follPunct.IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetEndMarkers().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
		return TRUE;
	if (pSrcPhrase->m_bEndRetranslation)
		return TRUE;
	if (pSrcPhrase->m_bFootnoteEnd)
		return TRUE;
	if (pSrcPhrase->m_bEndFreeTrans)
		return TRUE;
	return FALSE; // none apply, so it is not left-associated
}

// Transfers data from certain fields between CSourcePhrase instances, bClearAfterwards, if
// TRUE, will clear the 'from' field after its data is transferred, if FALSE, the data is
// left in place. Transfer of values of m_bFootnoteEnd, m_bEndRetranlation, and
// m_bEndFreeTrans is governed by the bFlagsToo value, transfer of these values is done if
// bFlagsToo is TRUE, skipped if FALSE, and the flags are cleared to FALSE after transfer
// provided both bFlagsToo and bClearAfterwards are TRUE.
// This function is used in two main contexts: transfer of information from or to a
// placeholder to a CSourcePhrase before or after - this is when the bool parameters will
// all be passed as TRUE; secondly, in the import edited source text feature, to transfer
// punctuation and markers (but not flags) from the incoming (edited) source text's
// unchanged CSourcePhrase instances to the existing matching CSourcePhrase instances - so
// as to ensure any user edits of the UFSM structure or punctuation are not missed in the
// merge process.
void TransferFollowingMembers(CSourcePhrase* pFrom, CSourcePhrase* pTo, bool bFlagsToo,
							  bool bClearAfterwards)
{
	wxString empty = _T("");
	pTo->m_follPunct = pFrom->m_follPunct;
	if (!pFrom->m_follPunct.IsEmpty() && bClearAfterwards)
	{
		pFrom->m_follPunct.Empty();
	}

	pTo->SetFollowingOuterPunct(pFrom->GetFollowingOuterPunct());
	if (!pFrom->GetFollowingOuterPunct().IsEmpty() && bClearAfterwards)
	{
		pFrom->SetFollowingOuterPunct(empty);
	}

	pTo->SetEndMarkers(pFrom->GetEndMarkers());
	if (!pFrom->GetEndMarkers().IsEmpty() && bClearAfterwards)
	{
		pFrom->SetEndMarkers(empty);
	}

	pTo->SetInlineBindingEndMarkers(pFrom->GetInlineBindingEndMarkers());
	if (!pFrom->GetInlineBindingEndMarkers().IsEmpty() && bClearAfterwards)
	{
		pFrom->SetInlineBindingEndMarkers(empty);
	}

	pTo->SetInlineNonbindingEndMarkers(pFrom->GetInlineNonbindingEndMarkers());
	if (!pFrom->GetInlineNonbindingEndMarkers().IsEmpty() && bClearAfterwards)
	{
		pFrom->SetInlineNonbindingEndMarkers(empty);
	}

	if (pFrom->m_bFootnoteEnd && bFlagsToo)
	{
		pTo->m_bFootnoteEnd = pFrom->m_bFootnoteEnd;
		if (bClearAfterwards)
		{
			pFrom->m_bFootnoteEnd = FALSE;
		}
	}
	if (pFrom->m_bEndRetranslation && bFlagsToo)
	{
		pTo->m_bEndRetranslation = pFrom->m_bEndRetranslation;
		if (bClearAfterwards)
		{
			pFrom->m_bEndRetranslation = FALSE;
		}
	}
	if (pFrom->m_bEndFreeTrans && bFlagsToo)
	{
		pTo->m_bEndFreeTrans = pFrom->m_bEndFreeTrans;
		if (bClearAfterwards)
		{
			pFrom->m_bEndFreeTrans = FALSE;
		}
	}
}

// Checks for an indicator of right-association: returns TRUE if one is found. Otherwise
// returns FALSE. An indicator of right-association would be one of the following:
// m_precPunct is non-empty, m_markers is non-empty, m_collectedBackTrans is non-empty,
// m_filteredInfo is non-empty, m_inlineBindingMarkers is non-empty,
// m_inlineNonbindingMarkers is non-empty, a boolean indicating the start of a span is
// TRUE, such as m_bBeginRetranslation, m_bFootnote, m_bStartFreeTrans, or m_bFirstOfType
bool IsRightAssociatedPlaceholder(CSourcePhrase* pSrcPhrase)
{
	if (!pSrcPhrase->m_precPunct.IsEmpty())
		return TRUE;
	if (!pSrcPhrase->m_markers.IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetCollectedBackTrans().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetFreeTrans().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetFilteredInfo().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
		return TRUE;
	if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
		return TRUE;
	if (pSrcPhrase->m_bBeginRetranslation)
		return TRUE;
	if (pSrcPhrase->m_bFootnote)
		return TRUE;
	if (pSrcPhrase->m_bStartFreeTrans)
		return TRUE;
	if (pSrcPhrase->m_bFirstOfType)
		return TRUE;
	return FALSE; // none apply, so it is not right-associated
}

// Transfers data from certain fields between CSourcePhrase instances, bClearAfterwards, if
// TRUE, will clear the 'from' field after its data is transferred, if FALSE, the data is
// left in place. Transfer of values of m_bFootnoteEnd, m_bEndRetranlation, and
// m_bEndFreeTrans is governed by the bFlagsToo value, transfer of these values is done if
// bFlagsToo is TRUE, skipped if FALSE, and the flags are cleared to FALSE after transfer
// provided both bFlagsToo and bClearAfterwards are TRUE; the Adapt It custom fields for
// storage of filtered info m_filteredInfo, collected back translations (\bt), and free
// translations (\free) are transferred also if bAICustomMkrsAlso is TRUE (and the fields
// cleared after transfer if bClearAfterwards is TRUE), otherwise FALSE leaves them
// untransferred.
// This function is used in two main contexts: transfer of information from or to a
// placeholder to a CSourcePhrase before or after - this is when the bool parameters will
// all be passed as TRUE; secondly, in the import edited source text feature, to transfer
// punctuation and markers (but not most flags) from the incoming (edited) source text's
// unchanged CSourcePhrase instances to the existing matching CSourcePhrase instances - so
// as to ensure any user edits of the UFSM structure or punctuation are not missed in the
// merge process.
void TransferPrecedingMembers(CSourcePhrase* pFrom, CSourcePhrase* pTo,
			bool bAICustomMkrsAlso, bool bFlagsToo, bool bClearAfterwards)
{
	wxString empty = _T("");

	pTo->m_precPunct = pFrom->m_precPunct;
	if (!pFrom->m_precPunct.IsEmpty() && bClearAfterwards)
	{
		pFrom->m_precPunct.Empty();
	}

	pTo->m_markers = pFrom->m_markers;
	if (!pFrom->m_markers.IsEmpty() && bClearAfterwards)
	{
		pFrom->m_markers.Empty();
	}

	pTo->SetInlineBindingMarkers(pFrom->GetInlineBindingMarkers());
	if (!pFrom->GetInlineBindingMarkers().IsEmpty() && bClearAfterwards)
	{
		pFrom->SetInlineBindingMarkers(empty);
	}

	pTo->SetInlineNonbindingMarkers(pFrom->GetInlineNonbindingMarkers());
	if (!pFrom->GetInlineNonbindingMarkers().IsEmpty() && bClearAfterwards)
	{
		pFrom->SetInlineNonbindingMarkers(empty);
	}

	if (bAICustomMkrsAlso)
	{
		pTo->SetCollectedBackTrans(pFrom->GetCollectedBackTrans());
		if (!pFrom->GetCollectedBackTrans().IsEmpty() && bClearAfterwards)
		{
			pFrom->SetCollectedBackTrans(empty);
		}

		pTo->SetFreeTrans(pFrom->GetFreeTrans());
		if (!pFrom->GetFreeTrans().IsEmpty() && bClearAfterwards)
		{
			pFrom->SetFreeTrans(empty);
		}

		pTo->SetFilteredInfo(pFrom->GetFilteredInfo());
		if (!pFrom->GetFilteredInfo().IsEmpty() && bClearAfterwards)
		{
			pFrom->SetFilteredInfo(empty);
		}
	}
	if (pFrom->m_bFootnote && bFlagsToo)
	{
		pTo->m_bFootnote = pFrom->m_bFootnote;
		if (bClearAfterwards)
		{
			pFrom->m_bFootnote = FALSE;
		}
	}
	if (pFrom->m_bFirstOfType && bFlagsToo)
	{
		pTo->m_bFirstOfType = pFrom->m_bFirstOfType;
		if (bClearAfterwards)
		{
			pFrom->m_bFirstOfType = FALSE;
		}
	}
	if (pFrom->m_bBeginRetranslation && bFlagsToo)
	{
		pTo->m_bBeginRetranslation = pFrom->m_bBeginRetranslation;
		if (bClearAfterwards)
		{
			pFrom->m_bBeginRetranslation = FALSE;
		}
	}
	if (pFrom->m_bStartFreeTrans && bFlagsToo)
	{
		pTo->m_bStartFreeTrans = pFrom->m_bStartFreeTrans;
		if (bClearAfterwards)
		{
			pFrom->m_bStartFreeTrans = FALSE;
		}
	}
}

// Transfers the contents of all punctuation and marker fields in pFrom to pTo, but does
// not do anything with any most of of the flags, and optionally can empty the puncts and marker
// fields data is transferred from, after the data in them has been transferred. Calls
// TransferPrecedingMembers() and TransferFollowingMembers(). Fields involved are:
// m_markers, m_endMarkers, m_inlineBindingMarkers, m_inlineBindingEndMarkers,
// m_inlineNonbindingMarkers, m_inlineNonbindingEndMarkers; m_precPunct, m_follPunct and
// m_follOuterPunct. We always, however, transfer m_bVerse and m_inform values.
// This function is used in one context: in the import edited source text feature, to
// transfer punctuation and markers (but not flags) from the incoming (edited) source
// text's lexically unchanged CSourcePhrase instances to the existing matching CSourcePhrase
// instances - so as to ensure any user edits of the UFSM structure or punctuation are not
// missed in the merge process. This function is not suitable for transferring data to a
// merged CSourcePhrase instance -- use TransferMarkersAndPunctsForMerger() for that.
void TransferPunctsAndMarkersOnly(CSourcePhrase* pFrom, CSourcePhrase* pTo, bool bClearAfterwards)
{
	TransferPrecedingMembers(pFrom, pTo, FALSE, FALSE, bClearAfterwards);
	TransferFollowingMembers(pFrom, pTo, FALSE, bClearAfterwards);
	pTo->m_bVerse = pFrom->m_bVerse;
	pTo->m_inform = pFrom->m_inform;
}

// Transfers the contents of all punctuation and marker fields in the range of
// CSourcePhrase instances represented by newStartAt to newEndAt, inclusive, from the
// arrNew array of CSourcePhrase instances resulting from tokenization of the edited
// source text imported using the Import Edited Source Text command. The transfers are
// done to the CSourcePhrase in arrOld, pTo, and pTo must be a merger, and the number of
// arrNew instances must numerically be equal to the pTo->m_nSrcWords member (because this
// is the count for the instances in pTo->m_pSavedWords, and these represent the data that
// corresponds to the merger (and since mergers are not created at import time and so
// arrNew doesn't have any)). It does not do anything with any of the flags. Fields
// involved are:
// m_markers, m_endMarkers, m_inlineBindingMarkers, m_inlineBindingEndMarkers,
// m_inlineNonbindingMarkers, m_inlineNonbindingEndMarkers; m_precPunct, m_follPunct and
// m_follOuterPunct. Additionally, the m_adaption and m_targetStr contents in the
// instances saved in pTo->m_pSavedWords are copied back to the range of arrNew instances,
// and then those arrNew instances are deep copied and replace the ones in
// pTo->m_pSavedWords in case the user later unmerges the merger. Also, m_srcPhrase and
// m_targetStr have to be updated, in case punctuation was added or removed, and similarly
// in the updated saved originals in m_pSavedWords member.
// This function is used in one context: in the import edited source text feature, to
// transfer punctuation and markers (but not flags) from the incoming (edited) source
// text's lexically unchanged CSourcePhrase instances to the existing matching CSourcePhrase
// instances - so as to ensure any user edits of the UFSM structure or punctuation are not
// missed in the merge process.
// BEW 28May11, changed the signature to conform to the other Transfer...() functions' signatures
// old signature: bool TransferPunctsAndMarkersToMerger(SPArray& arrNew, int newStartAt,
//                int newEndAt, CSourcePhrase* pTo)
bool TransferPunctsAndMarkersToMerger(SPArray& arrOld, SPArray& arrNew, int oldIndex,
		int newIndex, Subspan* pSubspan, int& oldDoneToIncluding, int & newDoneToIncluding)
{
	wxASSERT(pSubspan->spanType == commonSpan);

	// check indices don't violate pSubspan's  bounds
	if (oldIndex < pSubspan->oldStartPos || oldIndex > pSubspan->oldEndPos)
	{
		// out of range in subspan in arrOld
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	if (newIndex < pSubspan->newStartPos || newIndex > pSubspan->newEndPos)
	{
		// out of range in subspan in arrNew
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	CSourcePhrase* pTo = arrOld.Item(oldIndex);
	wxASSERT(pTo->m_nSrcWords > 1);
	wxASSERT((int)pTo->m_pSavedWords->GetCount() == pTo->m_nSrcWords);

	int newStartAt = newIndex;
    int newEndAt = newIndex + pTo->m_nSrcWords - 1;

	//int newCount = arrNew.GetCount();
#if defined( _DEBUG)
	int newRange = newEndAt - newStartAt + 1;
	int oldRange = pTo->m_nSrcWords;
	wxASSERT(oldRange == newRange);
#endif
	// create a wxArrayPtrVoid storing the new CSourcePhrase instances
	wxArrayPtrVoid* pRangeNew = new wxArrayPtrVoid;
	int index;
	for (index = newStartAt; index <= newEndAt; index++)
	{
		pRangeNew->Add(arrNew.Item(index));
	}
	int last = pRangeNew->GetCount() - 1;
	// first, transfer preceding punctuation and markers to pTo, using the first
	// CSourcePhrase in the new array's subrange as the source of the material to transfer;
	// FALSE, FALSE, FALSE is: bool bAICustomMkrsAlso, bool bFlagsToo, bool bClearAfterwards
	TransferPrecedingMembers((CSourcePhrase*)pRangeNew->Item(0), pTo, FALSE, FALSE, FALSE);
	// second, transfer the following punctuation and markers to pTo, using the last
	// instance in new array's subrange (anything else will go, further below, in the
	// m_pMedialPuncts wxArrayString, and m_pSavedWords SPList, using code further below)
	// // FALSE, FALSE is: bool bFlagsToo, bool bClearAfterwards
	TransferFollowingMembers((CSourcePhrase*)pRangeNew->Item(last), pTo, FALSE, FALSE);

	// now gather and transfer the medial punctuation, and markers, if any, and put into the
	// m_pMedialPuncts array and m_pMedialMarkers array, as appropriate
	wxString parentPrecPunct = pTo->m_precPunct;
	wxString parentFollPunct = pTo->m_follPunct + pTo->GetFollowingOuterPunct();
	wxString srcPhraseUpdated; srcPhraseUpdated.Empty();
	ReplaceMedialPunctuationAndMarkersInMerger(pTo, pRangeNew, parentPrecPunct,
								parentFollPunct, srcPhraseUpdated);
	if (!srcPhraseUpdated.IsEmpty())
	{
		pTo->m_srcPhrase = srcPhraseUpdated;
	}

	// The source text has been handled, and the m_targetStr members in m_pSavedWords
	// also, but unfortunately we can't reliably rebuild the m_targetStr for the owning
	// merged CSourcePhrase, because there may be fewer, the same or more words in the
	// m_targetStr member, and the location of punctuation may be different. So all we can
	// do is leave that member unchanged - it's up to the user to eyeball that instance
	// and edit it using the phrase box if he's unsatisfied with it

	// do the storage of the updated CSourcePhrase instances in m_pSavedWords after
	// adaptations were tranferred to the new instances first
	ReplaceSavedOriginalSrcPhrases(pTo, pRangeNew);

	pRangeNew->Clear();
	if (pRangeNew != NULL) // whm 11Jun12 added NULL test
		delete pRangeNew;
	// update the values to be returned as indices preceding the next kick-off locations
	oldDoneToIncluding = oldIndex;
	newDoneToIncluding = newEndAt;
	return TRUE;
}

// Grab the contents of marker fields and Add() them to pArray (also include the following
// space character when present, and also including verse and chapter numbers if \v and \c
// are encountered - we don't think any merger will actually overlap a verse or chapter
// boundary, so we don't expect to ever grab these two markers - but if we do, they can end
// up as medial markers, including the following number - since all the user is ever asked
// to do is to "place" them and he could then also place the number as well). The fields
// are the following (ignore a field if it's contents are empty):
// m_markers, m_inlineBindingMarkers, m_inlineNonbindingMarkers
// Return a count of how many marker fields were stored in pArray (0 if nothing was stored)
// We use this function for populating the m_pMedialMarkers member of a merged
// CSourcePhrase - it's useless anywhere else because information about which field coughed
// up a given marker is lost.
int PutBeginMarkersIntoArray(CSourcePhrase* pSrcPhrase, wxArrayString* pArray)
{
	int count = 0;
	pArray->Clear();
	if (!pSrcPhrase->m_markers.IsEmpty())
	{
		pArray->Add(pSrcPhrase->m_markers);
		count++;
	}
	if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
	{
		pArray->Add(pSrcPhrase->GetInlineBindingMarkers());
		count++;
	}
	if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
	{
		pArray->Add(pSrcPhrase->GetInlineNonbindingMarkers());
		count++;
	}
	return count;
}

// Grab the contents of marker fields and Add() them to pArray (also include the following
// space character when present). The fields are the following (ignore a field if it's
// contents are empty):
// m_endMarkers, m_inlineBindingEndMarkers, m_inlineNonbindingEndMarkers
// Return a count of how many marker fields were stored in pArray (0 if nothing was stored)
// We use this function for populating the m_pMedialMarkers member of a merged
// CSourcePhrase - it's useless anywhere else because information about which field coughed
// up a given marker is lost.
int PutEndMarkersIntoArray(CSourcePhrase* pSrcPhrase, wxArrayString* pArray)
{
	int count = 0;
	pArray->Clear();
	if (!pSrcPhrase->GetEndMarkers().IsEmpty())
	{
		pArray->Add(pSrcPhrase->GetEndMarkers());
		count++;
	}
	if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
	{
		pArray->Add(pSrcPhrase->GetInlineBindingEndMarkers());
		count++;
	}
	if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
	{
		pArray->Add(pSrcPhrase->GetInlineNonbindingEndMarkers());
		count++;
	}
	return count;
}

// Returns nothing
// pMergedSP is a merged CSourcePhrase in a list of old (that is, 'unedited') instances,
// and it is "in common" with a subrange of newly created (ie. just tokenized)
// CSourcePhrase instances derived by imported possibly edited source text - the subrange
// of new instances are temporarily stored in pArrayNew. The ones in pArrayNew are examined
// and the punctuation and marker fields which are non-empty and which would be medial if
// that subarray of new instances were merged, are extracted and stored in the
// wxArrayString arrays m_pMedialPuncts and m_pMedialMarkers, after the latter pair of
// members has been cleared of their former contents - if any. parentPrevPunct is for
// inputting the intial m_precPunct value for the parent instance, and parentFollPunct
// does the same for the m_follPunct and m_follOuterPunct values for the parent; while the
// medial information builds strFromMedials from the new material's m_key members plus
// punctuation, and returns it to the caller by means of the signature, where it will have
// the parent's previous and following puncts added - to form a new m_srcPhrase value.
// We also rebuild the m_targetStr members for both parent and it's children.
// This function is used within TransferPunctsAndMarkersToMerger()
void ReplaceMedialPunctuationAndMarkersInMerger(CSourcePhrase* pMergedSP, wxArrayPtrVoid* pArrayNew,
				wxString& parentPrevPunct, wxString& parentFollPunct, wxString& strFromMedials)
{
	wxString spaceStr = _T(" ");

	wxArrayPtrVoid arrRangeOfOldOnes;
	SPList::Node* pos = pMergedSP->m_pSavedWords->GetFirst();
	while (pos != NULL)
	{
		arrRangeOfOldOnes.Add(pos->GetData());
		pos = pos->GetNext();
	}

	pMergedSP->m_pMedialPuncts->Clear();
	pMergedSP->m_pMedialMarkers->Clear();
	int total = pArrayNew->GetCount();
	strFromMedials += parentPrevPunct;
	int index;
	wxString targetStr;
	for (index = 0; index < total; index++)
	{
		CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pArrayNew->Item(index);
		if (index == 0)
		{
            // add the new instance's m_key to the string being composed for parent's
            // m_srcPhrase updated value
			strFromMedials += pSrcPhrase->m_key;
			strFromMedials += pSrcPhrase->m_follPunct;
			strFromMedials += pSrcPhrase->GetFollowingOuterPunct();
			strFromMedials += spaceStr;

			// build a with-punctuation m_targetStr from the old stored original
			// sourcephrase and copy it to the pSrcPhrase->m_targetStr member
			CSourcePhrase* pOrig = (CSourcePhrase*)arrRangeOfOldOnes.Item(index);
			if (!pOrig->m_adaption.IsEmpty())
			{
				// we build these members, even though not displayed, just in case the
				// user later unmergers this merger in the edited & merged document
				targetStr = parentPrevPunct;
				targetStr += pOrig->m_adaption;
				targetStr += pOrig->m_follPunct;
				targetStr += pOrig->GetFollowingOuterPunct();
				// insert it into the m_targetStr member of pSrcPhrase, also copy
				// m_adaption
				pSrcPhrase->m_adaption = pOrig->m_adaption;
				pSrcPhrase->m_targetStr = targetStr;
			}

			// the first instance contributes only from stored end-marker fields, and
			// following puncts and following outer puncts
			// BEW 15Aug11, need to wrap each line in a test for non-empty string,
			// otherwise it adds empty strings to m_pMedialMarkers and m_pMedialPuncts
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetEndMarkers());
			}
			if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineBindingEndMarkers());
			}
			if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineNonbindingEndMarkers());
			}

			if (!pSrcPhrase->m_follPunct.IsEmpty())
			{
				pMergedSP->m_pMedialPuncts->Add(pSrcPhrase->m_follPunct);
			}
			if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
			{
				pMergedSP->m_pMedialPuncts->Add(pSrcPhrase->GetFollowingOuterPunct());
			}
		}
		else if (index == total - 1)
		{
            // add the new instance's m_key to the string being composed for parent's
            // m_srcPhrase updated value
			strFromMedials += pSrcPhrase->m_precPunct;
			strFromMedials += pSrcPhrase->m_key;
			strFromMedials += parentFollPunct; // LHS combines m_follPunct + m_follOuterPunct

			// build a with-punctuation m_targetStr from the old stored original
			// sourcephrase and copy it to the pSrcPhrase->m_targetStr member, etc
			CSourcePhrase* pOrig = (CSourcePhrase*)arrRangeOfOldOnes.Item(index);
			if (!pOrig->m_adaption.IsEmpty())
			{
				// we build these members, even though not displayed, just in case the
				// user later unmergers this merger in the edited & merged document
				targetStr = pOrig->m_precPunct;
				targetStr += pOrig->m_adaption;
				targetStr += parentFollPunct; // combines m_follPunct + m_follOuterPunct
				// insert it into the m_targetStr member of pSrcPhrase, also copy
				// m_adaption
				pSrcPhrase->m_adaption = pOrig->m_adaption;
				pSrcPhrase->m_targetStr = targetStr;
			}

			// the last instance contributes only from stored begin-marker fields, and
			// preceding puncts
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->m_markers);
			}
			if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineBindingMarkers());
			}
			if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineNonbindingMarkers());
			}

			if (!pSrcPhrase->m_precPunct.IsEmpty())
			{
				pMergedSP->m_pMedialPuncts->Add(pSrcPhrase->m_precPunct);
			}
		}
		else
		{
            // add the new instance's m_key to the string being composed for parent's
            // m_srcPhrase updated value
			strFromMedials += pSrcPhrase->m_precPunct;
			strFromMedials += pSrcPhrase->m_key;
			strFromMedials += pSrcPhrase->m_follPunct;
			strFromMedials += pSrcPhrase->GetFollowingOuterPunct();
			strFromMedials += spaceStr;

			// build a with-punctuation m_targetStr from the old stored original
			// sourcephrase and copy it to the pSrcPhrase->m_targetStr member, etc
			CSourcePhrase* pOrig = (CSourcePhrase*)arrRangeOfOldOnes.Item(index);
			if (!pOrig->m_adaption.IsEmpty())
			{
				// we build these members, even though not displayed, just in case the
				// user later unmergers this merger in the edited & merged document
				targetStr = pOrig->m_precPunct;
				targetStr += pOrig->m_adaption;
				targetStr += pOrig->m_follPunct;
				targetStr += pOrig->GetFollowingOuterPunct();
				// insert it into the m_targetStr member of pSrcPhrase, also copy
				// m_adaption
				pSrcPhrase->m_adaption = pOrig->m_adaption;
				pSrcPhrase->m_targetStr = targetStr;
			}

			// any other instances contribute from both begin-marker fields and end-marker
			// fields; and preceding and following and followingOuter punctuation fields
			if (!pSrcPhrase->m_markers.IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->m_markers);
			}
			if (!pSrcPhrase->GetInlineBindingMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineBindingMarkers());
			}
			if (!pSrcPhrase->GetInlineNonbindingMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineNonbindingMarkers());
			}
			if (!pSrcPhrase->GetEndMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetEndMarkers());
			}
			if (!pSrcPhrase->GetInlineBindingEndMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineBindingEndMarkers());
			}
			if (!pSrcPhrase->GetInlineNonbindingEndMarkers().IsEmpty())
			{
				pMergedSP->m_pMedialMarkers->Add(pSrcPhrase->GetInlineNonbindingEndMarkers());
			}

			if (!pSrcPhrase->m_precPunct.IsEmpty())
			{
				pMergedSP->m_pMedialPuncts->Add(pSrcPhrase->m_precPunct);
			}
			if (!pSrcPhrase->m_follPunct.IsEmpty())
			{
				pMergedSP->m_pMedialPuncts->Add(pSrcPhrase->m_follPunct);
			}
			if (!pSrcPhrase->GetFollowingOuterPunct().IsEmpty())
			{
				pMergedSP->m_pMedialPuncts->Add(pSrcPhrase->GetFollowingOuterPunct());
			}
		}
	}
}

// Returns nothing. Takes a list of newly tokenized CSourcePhrase instances corresponding
// to the pMergedSP merged CSourcePhrase in a different list of old (that is, 'unedited')
// CSourcePhrase instances, transfers the adaptation information from pMergedSP's SPList
// m_pSavedWords's instances (the old originals from before the merger), and copies those
// adaptations to the matching instances in pArrayNew. Then m_pSavedWords is cleared, and
// the instances in pArrayNew are deep copied, in top down order (which corresponds to the
// word order of their m_key words within the (here unedited) source text just imported)
// into m_pSavedWords - so that if the user later decides to unmerge the merger, the
// exposed now-unmerged instances will show any user edits of punctuation or USFM
// structure done when the source text was outside of Adapt It and potentially edited
// there. This function is used within TransferPunctuationAndMarkersForMerger().
void ReplaceSavedOriginalSrcPhrases(CSourcePhrase* pMergedSP, wxArrayPtrVoid* pArrayNew)
{
	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	// set up (again) the array of old CSourcePhrase ptrs in pMergedSP's m_pSavedWords
	// member because we have to delete these before we can append the new copies we've
	// just constructed in the caller
	wxArrayPtrVoid arrRangeOfOldOnes;
	SPList::Node* pos = pMergedSP->m_pSavedWords->GetFirst();
	while (pos != NULL)
	{
		arrRangeOfOldOnes.Add(pos->GetData());
		pos = pos->GetNext();
	}
	int index;
	int count = pMergedSP->m_pSavedWords->GetCount();
	// delete all the old instances in pMergedSP->m_pSavedWords; since the pointers
	// are copied to arrRangeOfOldOnes, we can do it easier from there
	for (index = 0; index < count; index++)
	{
		// FALSE is bool bDoPartnerPileDeletionAlso, FALSE because these instances by
		// definition, being in m_pSavedWords, can't have partner piles
		pDoc->DeleteSingleSrcPhrase((CSourcePhrase*)arrRangeOfOldOnes.Item(index), FALSE);
	}
	pMergedSP->m_pSavedWords->Clear(); // default call doesn't call delete
								// on the stored pointers" (because we've deleted them
								// in the loop above)
	// now deep copy the new array's matching instances, and append the ptr-to-copy in the
	// now cleared pMergedSP->m_pSavedWords SPList
	for (index = 0; index < count; index++)
	{
		CSourcePhrase* pSrcPhrase = new CSourcePhrase(*(CSourcePhrase*)pArrayNew->Item(index));
		pSrcPhrase->DeepCopy();
		pMergedSP->m_pSavedWords->Append(pSrcPhrase);
	}
    // the old instances in m_pSavedWords now reflect any punctuation or marker changes
    // which the user may have done when editing the source text outside of Adapt It; and
    // the parent and saved original instances are rebuilt in their m_srcPhrase and
    // m_targetStr members with the user's punctuation edits, this will catch any
    // punctuation changes (whether additions, removals, or just left unchanged)
}

//////////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE, if the attempt at widening leftwards by a "single" jump
///                         succeeds, otherwise FALSE
/// \param  arrOld          ->  array of old CSourcePhrase instances
/// \param  arrNew          ->  array of new CSourcePhrase instances
/// \param  oldStartAt      ->  starting index in arrOld for parent Subspan
/// \param  oldEndAt        ->  ending (inclusive) index in arrOld for parent Subspan
/// \param  newStartAt      ->  starting index in arrNew for parent Subspan
/// \param  newEndAt        ->  ending (inclusive) index in arrNew for parent Subspan
/// \param  oldStartingPos  ->  the index in arrOld from which we start our leftwards jump
/// \param  newStartingPos  ->  the index in arrNew from which we start our leftwards jump
/// \param  oldCount        <-  ref to a count of the number of CSourcePhrase instances to accept
///                             to the left in our "single" jump within the arrOld array (it may
///                             not be just one - see below)
/// \param  newCount        <-  ref to a count of the number of CSourcePhrase instances to accept
///                             to the left in our "single" jump within the arrNew array (it should
///                             always be just one - see below, because arrNew will NEVER
///                             have any retranslations or placeholders in it)
///
/// \remarks
/// This function tries to extend a matchup of an in-common word leftwards by one step,
/// and since it is called repeatedly until a failure results, the kick off point will
/// move leftwards for each iteration in both arrOld and arrNew. If there are no
/// placeholders nor retranslations to the immediate left, then we have only to make a
/// simple test for matching m_key values in the CSourcePhrase to the immediate left (i.e.
/// in the array, at the next smallest index value) in both arrOld and arrNew. Return TRUE
/// if they match, FALSE if they don't - and returning FALSE indicates we've come to the
/// left bound of an in-common span of CSourcePhrase instances.
///
/// Note: the recursion algorithm must not change the number or order of CSourcePhrase
/// instances in arrOld and arrNew. Only after recursion is completed and the merging is
/// therefore completed can we do cleanup actions which change number of final
/// CSourcePhrase instances (such as removing placeholders in partially destroyed
/// retranslations - and even then, such cleanup is not done in either of arrOld or
/// arrNew)
///
/// Retranslations (and those may end with zero, one or more placeholders) and
/// placeholders complicate the situation significantly. This function is designed to
/// encapsulate these complications within it, so that higher level functions do not have
/// to consider either complication. Also, our algorithms will compose a merged (edited)
/// source text into the document without trying to maintain the integrity of
/// retranslations - and therefore, once the recursions are finished, it may be the case
/// that we have some retranslations which are lost (that's not a problem), some retained
/// (that's not a problem either), and some which have the start or end chopped off (that
/// IS a problem and we must make an appropriate adjustment). We do the adjustment for the
/// latter in a separate function which runs after recursion is completed and which spans
/// whole newly merged document looking for the messed-up retranslation subspans, and fixes
/// things - the fixes involve removing the adaptations within the retranslation fragments
/// that have lost either end.
///
/// But the WidenLeftwardsOnce() function still has to handle retranslations to some extent
/// (see below), and all placeholders which are not placeholders within a retranslation.
/// What follows are the rules...
/// (1) If a placeholder belonging to a retranslation is to the left, defer the decision as
/// to whether or not it belongs in the commonSpan, and check what lies to the left of it
/// (if another such placeholder precedes, continue checking leftwards until a
/// non-placeholder is encountered)
/// (2) When a non-retranslation, and non-placeholder CSourcePhrase is encountered in
/// arrOld, the next-to-the-left CSourcePhrase instance in arrNew is compared (their m_key
/// values are what are checked, for identity) - if the keys are identical, then the
/// leftwards widening has succeeded. At this point, any placeholders traversed in arrOld
/// are added to the oldCount value - in that way, oldCount can be greater than 1; and so
/// those placeholders are deemed to lie within the commonSpan. However, if the test of
/// the two keys shows they are not identical, then we've reached the leftmost end of the
/// commonSpan, and since retranslation placeholders belong with their retranslation, in
/// that case we exclude the placeholders from the commonSpan, and newCount and oldCount
/// would be returned as 0, and FALSE returned from the function.
/// (3) The rules for any placeholder which is not within a retranslation are a bit
/// different. Moving leftwards and encountering such a placeholder, any decision about
/// inclusion within commonSpan is deferred until a non-placeholder potential match pair is
/// checked for identity of m_key values. If the latter's keys match, then the widening
/// attempt has succeeded and both the matched CSourcePhrase and its following placeholder
/// are accepted into the commonSpan. If the non-placeholder CSourcePhrase pair's m_key
/// values are not identical, then the left bound of the commonSpan has been reached, and
/// we need additional criteria to help us decide whether or not the placeholder should be
/// included in the commonSpan, or considered to belong in the beforeSpan. Here is the
/// protocol for deciding this:
/// (a) Check the placeholder, is it left-associated, right-associated, or neither
/// (b) If right-associated, deem it to belong in commonSpan as the latter's first
/// CSourcePhrase instance (we'll support sequences of manually inserted placeholders,
/// although it's highly likely they never will occur);
/// (c) If left-associated, deem it to belong at the end of beforeSpan;
/// (d) If it is neither left- nor right-associated, treat it the same as for (c)
///
/// An additional complication is that arrOld will probably contain mergers, but arrNew
/// never will. So when widening, if an merger is encountered, IsMergerAMatch() must be
/// called to determine if the potentially equivalent word sequence is in arrNew starting
/// at an appropriate index value. This also leads to newCount being > 1 if the
/// corresponding sequence exists in arrNew.
/// (There is also a WidenRightwardsOnce() function which has similar rules, but different
/// in places for obvious reasons)
bool WidenLeftwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount)
{
	oldEndAt = oldEndAt; newEndAt = newEndAt; // avoid compiler warnings about unused variables
	CSourcePhrase* pOldSrcPhrase = NULL;
	CSourcePhrase* pNewSrcPhrase = NULL;
	oldCount = 0;
	newCount = 0;
	if (oldStartingPos < oldStartAt || newStartingPos < newStartAt)
	{
		// we are earlier than the parent's left bound, so can't widen to the left any
		// further
#if defined( LEFTRIGHT) && defined( _DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at ONE"));
#endif
		return FALSE;
	}
	int oldIndex = oldStartingPos;
	int newIndex = newStartingPos;
	if (oldIndex > oldStartAt && newIndex > newStartAt)
	{
		pOldSrcPhrase = arrOld.Item(oldIndex);
		pNewSrcPhrase = arrNew.Item(newIndex);
		// The first test should be for the most commonly occurring situation - a
		// non-merged, non-placeholder, non-retranslation CSourcePhrase
		if (pOldSrcPhrase->m_nSrcWords == 1 && !pOldSrcPhrase->m_bRetranslation && !pOldSrcPhrase->m_bNullSourcePhrase)
		{
			if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			{
				// we can extend the commonSpan successfully leftwards to this
				// pair of instances
				oldCount++;
				newCount++;
				return TRUE;
			}
			else
			{
				// a non-match means that the left bound for the commonSpan has been
				// reached
#if defined( LEFTRIGHT) && defined( _DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at TWO -- the single-single mismatch"));
#endif
				return FALSE;
			}
		}
		// The second test should be for pOldSrcPhrase being a merger (if so, it can't be a
		// placeholder nor within a retranslation) because that is more likely to happen
		// than the other complications we must handle. We'll also handle fixedspace
		// conjoining here too - and accept a non-conjoined identical word pair in arrNew
		// as a match (ie. we won't require that ~ also conjoins the arrNew's word pair)
		else if (pOldSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger, or conjoined pseudo-merger
			wxASSERT(!pOldSrcPhrase->m_bRetranslation);
			wxASSERT(!pOldSrcPhrase->m_bNullSourcePhrase);
			int numWords = pOldSrcPhrase->m_nSrcWords;
			bool bIsFixedspaceConjoined = IsFixedSpaceSymbolWithin(pOldSrcPhrase);
            // If it's a normal (non-fixedspace) merger, then we do the same check as for a
            // fixedspace merger, because if ~ is still in the edited source text, then
            // that will parse to a conjoined (pseudo merger) CSourcePhrase with
            // m_nSrcWords set to 2 - so check for either of these, and if there isn't a
            // match, then check if the old instance really is a fixedspace merger, and is
            // so try matching each word of the two word sequence at the approprate
            // location, without any ~, -- if that succeeds, treat it as a match; if none
            // of that gives a match, then there isn't one and we are done - return FALSE
            // if so
			if (bIsFixedspaceConjoined)
			{
				if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
				{
					// we can extend the commonSpan successfully leftwards to this
					// pair of instances, each has the fixedspace (~) marker in its
					// m_key member
					oldCount++;
					newCount++;
					return TRUE;
				}
				else
				{
					// the arrNew array at the match location doesn't have a fixedspace
					// pseudo-merger, so attempt to match the words there individually
					if (newIndex - numWords + 1 >= newStartAt)
					{
                        // there are enough words available for a match attempt... get the
                        // individual words in pOldSrcPhrase and then check for a match
                        // with two successive new CSourcePhrase instances' m_key values
						int newStartingIndex = newIndex - numWords + 1;
						SPList::Node* pos = pOldSrcPhrase->m_pSavedWords->GetFirst();
						wxASSERT(pos != NULL);
						wxString word1 = pos->GetData()->m_key;
						pos = pos->GetNext();
						wxString word2 = pos->GetData()->m_key;
						wxString newWord1 = arrNew.Item(newStartingIndex)->m_key;
						wxString newWord2 = arrNew.Item(newStartingIndex + 1)->m_key;
						if (word1 == newWord1 && word2 == newWord2)
						{
							// consider this a match
							oldCount++;
							newCount += 2;
							return TRUE;
						}
						else
						{
							// no match
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at THREE"));
#endif
							return FALSE;
						}
					}
					else
					{
						// not enough words available, so no match is possible
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at FOUR"));
#endif
						return FALSE;
					}
				}
			} // end of TRUE block for test: if (bIsFixedspaceConjoined)
			else
			{
                // it's a plain vanilla merger, so get the starting index for it and ensure
                // that lies within arrNew's bounds, if so we assume then that arrNew may
                // have a matching word sequence, so we test for the match
				int newStartIndex = newIndex - numWords + 1;
				wxASSERT(newStartIndex >= 0); // otherwise a bounds error
				bool bMatched = IsMergerAMatch(arrOld, arrNew, oldIndex, newStartIndex);
				if (bMatched)
				{
					oldCount++; // it now equals 1
					newCount += numWords; // no mergers in arrNew, so we count the
							// requisite number of words which belong in commonSpan
					return TRUE;
				}
				else
				{
					// no match, so return FALSE
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at THREE - the merger-sequence mismatch"));
#endif
					return FALSE;
				}
			} // end of else block for test: if (bIsFixedspaceConjoined)
		} // end of TRUE block for test: if (pOldSrcPhrase->m_nSrcWords > 1)

        // Next complications are placeholders and retranslations. So, because placeholders
        // can occur at the end of a retranslation, test first for pOldSrcPhrase being a
        // placeholder, and then within the TRUE block distinguish between placeholders
        // within retranslations and one or more plain vanilla (i.e. manually inserted
        // previously) placeholders which are not in a retranslation. (We expect that a
        // user would never insert two or more placeholders manually at the same location,
        // but just in case he does, we'll handle them.) Our approach to plain vanilla
        // placeholder(s) is to defer action until we know if there is a match of the
        // farther-out abutting CSourcePhrase instance in arrOld with the potential
        // matching one in arrNew - if the match obtains, then the enclosed placeholder(s)
        // are taken into commonSpan; if the matchup fails, then the placeholder(s) are at
        // the boundary of beforeSpan and commonSpan - and which side of it will then
        // depend on whether there is indication of left association (then it/they belong
        // in beforeSpan) or right association (then it/they belong in commonSpan) or no
        // indication of association -- in which case we have no criterion to guide us, so
        // we'll assume that it/they should be in beforeSpan (and so get removed in the
        // merger process)
		if (pOldSrcPhrase->m_bNullSourcePhrase)
		{
			// it's a placeholder
			bool bItsInARetranslation = FALSE;
			if (pOldSrcPhrase->m_bRetranslation)
			{
				// It's a placeholder at the end of a retranslation.
				//
                // Our approach for these is to keep such placeholder(s) with the
                // retranslation unit to which they belong - so we defer decision until we
                // check for a match (in arrNew) with the first non-placeholder in arrOld
                // preceding the one or more retranslation-final placeholders. If the match
                // obtains, then we take the placeholders and the preceding pOldSrcPhrase
                // into commonSpan (but we don't require that the whole retranslation end
                // up in commonSpan, because the out-of-Adapt_It editing of the new source
                // text may have changed some of the source text which was in this
                // particular retranslation, and we'll need to support partial updates of a
                // retranslation. We can do that by later on, after recursion ends, finding
                // where a CSourcePhrase has m_bRetranslation TRUE but without
                // m_bBeginRetranslation also being true - that identifies the start of
                // what remains of a retranslation, and we can then set this
                // CSourcePhrase's m_bBeginRetranslation flag to TRUE, and do a "Remove
                // Retranslation" operation on it, to remove the bogus target text values
                // in it, and the final placeholders, prior to displaying the merged
                // document to the user.
                //
                // But if the pre-placeholder(s) CSourcePhrase we find doesn't
                // match the one in arrNew, then we are at the left boundary for
                // commonSpan, and so the retranslation placeholder(s) belong with the rest
                // of the retranslation - which is in beforeSpan, so we exclude them by
                // returning oldCount = 0
                //
				// The code for implementing this protocol requires we traverse over the
				// one or more placeholders to find a preceding placeholder; which is what
				// we must do for a manually inserted placeholder - so since the only
                // differences between these two are minor, we'll set a flag,
                // bItsInARetranslation, and use the flag to handle any differences
				bItsInARetranslation = TRUE;
			} // end of TRUE block for test: if (pOldSrcPhrase->m_bRetranslation)

			// It's either a plain vanila (i.e. user-manually-created) placeholder, not one
			// in a retranslation; or if bItsInARetranslation is TRUE, its a placeholder
			// which is at the end of a retranslation section.

			// Get past this and any additional non-retranslation placeholders until
			// either we reach oldStartAt and can't check further, or we get a
			// non-placeholder which is at or after oldStartAt which we can test for a
			// match of m_Key values with the one at arrNew; decide what to do
			// according to the protocols spelled out above.
			int nExtraCount = 0;
			int prevIndex = oldIndex - 1;
			CSourcePhrase* pPrevSrcPhrase = NULL;
			bool bFoundNonPlaceholder = FALSE;
			while (prevIndex >= oldStartAt)
			{
				pPrevSrcPhrase = arrOld.Item(prevIndex);
				if (pPrevSrcPhrase->m_bNullSourcePhrase && !pPrevSrcPhrase->m_bEndRetranslation)
				{
					// we've another placeholder next door, so keep moving back
					nExtraCount++;
					prevIndex--;
					continue;
				}
				else
				{
					// we've found a non-placeholder within the parent span, so check
					// it out for a match with what is at newIndex in arrNew
					bFoundNonPlaceholder = TRUE;
					// it could be a single-word CSourcePhrase, or a merger, so we
					// must deal with both possibilities; but if we are traversing
					// placeholders in a retranslation, then it won't be a merger
					if (pPrevSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					{
						// it's not a merger - so check for a match with the one in arrNew
						if (pPrevSrcPhrase->m_key == pNewSrcPhrase->m_key)
						{
							// we can extend the commonSpan successfully leftwards to this
							// pair of instances, pulling the traversed placeholders
							// into commonSpan along with the non-placeholder one at
							// their left -- this also applies to ones within a retranslation
							oldCount++; // counts the placeholder at oldIndex
							oldCount += nExtraCount; // adds the count of the extra ones traversed
							oldCount++; // counts this pPrevSrcPhrase which isn't a placeholder
							newCount++;
							return TRUE;
						}
						else
						{
							// a non-match
							if (bItsInARetranslation)
							{
								// all the one or more placeholders belong in beforeSpan,
								// so return oldCount = 0, newCount = 0, and FALSE
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at FOUR"));
#endif
								return FALSE;
							}
							else
							{
                                // a non-match for the non-placeholder CSourcePhrase
                                // immediately preceding manually created placeholders
                                // means that the left bound for the commonSpan has been
                                // reached; so here we must work out if the right-most
                                // placeholder is left associated, or the left-most
                                // placeholder is right associated, and if neither, we'll
                                // assign it/them to beforeSpan
								int indexOfLeftmostPlaceholder = oldIndex - nExtraCount;
								// assign the pointer at that location to pPrevSrcPhrase
								// since we don't need the latter for any other purpose now
								pPrevSrcPhrase = arrOld.Item(indexOfLeftmostPlaceholder);
								int indexOfRightmostPlaceholder = oldIndex;
								CSourcePhrase* pRightmostPlaceholder = arrOld.Item(indexOfRightmostPlaceholder);
								if (IsRightAssociatedPlaceholder(pPrevSrcPhrase))
								{
									// it/they are right-associated, so keep it/them with
									// what follows it/them, which means that it/they
									// belong in commonSpan, and we can't widen any further
									oldCount++; // counts the placeholder at oldIndex
									oldCount += nExtraCount; // adds the count of the extra ones traversed
									//newCount is unchanged (still zero)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at FIVE"));
#endif
									return FALSE; // tell the caller not to try another leftwards widening
								}
								else if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
								{
									// it/they are left-associated, so keep it/them with
									// what precedes it/them, which means that it/they
									// belong in beforeSpan, and we can't widen any further
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at SIX"));
#endif
									return FALSE; // (oldCount and newCount both returned as zero)
								}
								// neither left nor right associated, so handle the
								// same as left-associated
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at SEVEN"));
#endif
								return FALSE;
							} // end of else block for test: if (bItsInARetranslation)
						} // end of else block for test: if (pPrevSrcPhrase->m_key == pNewSrcPhrase->m_key)
					} // end of TRUE block for test: if (pPrevSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					else
					{
						// it's a merger (we'll assume no fixed-space conjoining, but just
						// a normal merger) -- pass in prevIndex here, as we are using
						// that index for our 'looking back' within arrOld
						int numWords = pPrevSrcPhrase->m_nSrcWords;
                        // the loop condition tests for prevIndex >= oldStartAt, so we
                        // don't need to repeat the test here, but here we do need to test
                        // to ensure that there are enough words in arrNew at or after
                        // newStartAt for a potential match - if not, we return FALSE
						int newStartIndex = prevIndex - numWords + 1;
						if (newStartIndex >= newStartAt)
						{
							// there are enough words for a test for a match
							bool bMatched = IsMergerAMatch(arrOld, arrNew, prevIndex, newStartIndex);
							if (bMatched)
							{
								oldCount++;
								newCount += numWords;
								return TRUE;
							}
							else
							{
								// no match
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at EIGHT"));
#endif
								return FALSE;
							}
						}
						else
						{
							// not enough words available
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at NINE"));
#endif
							return FALSE;
						}
					} // end of else block for test: if (pPrevSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					break;
				} // end of else block for test:
				  // if (pPrevSrcPhrase->m_bNullSourcePhrase && !pPrevSrcPhrase->m_bEndRetranslation)
			} // end of loop: while (prevIndex >= oldStartAt)

            // here we only need to handle the loop ending without any non-placeholder
            // instance being found (i.e. we traversed zero or more additional
            // placeholders, but then got to the span left boundary without finding any
            // non-placeholder CSourcePhrase instance) - & that means that the one or
            // more placeholders are either all in beforeSpan (and that's all that
            // would be in this beforeSpan), or all in commonSpan (in which case
            // beforeSpan would be empty) - we have to work out which is the case
            // below. But if a non-placeholder was found, then all the needed decisions
            // will have been made already within the loop and control returned to the
            // caller before the loop ended
			if (!bFoundNonPlaceholder)
			{
                // a non-match means that the left bound for the commonSpan
                // has been reached; so here we must work out if the
                // right-most placeholder is left associated, or the
                // left-most placeholder is right associated, and if
                // neither, we'll assign it/them to beforeSpan
				int indexOfLeftmostPlaceholder = oldIndex - nExtraCount;
				// assign the pointer at that location to pPrevSrcPhrase
				// since we don't need the latter for any other purpose now
				pPrevSrcPhrase = arrOld.Item(indexOfLeftmostPlaceholder);
				int indexOfRightmostPlaceholder = oldIndex;
				CSourcePhrase* pRightmostPlaceholder = arrOld.Item(indexOfRightmostPlaceholder);
				if (IsRightAssociatedPlaceholder(pPrevSrcPhrase))
				{
                    // it/they are right-associated, so keep it/them with
                    // what follows it/them, which means that it/they
                    // belong in commonSpan, and we can't widen any further
					oldCount++; // counts the placeholder at oldIndex
					oldCount += nExtraCount; // adds the count of the extra ones traversed
					//newCount is unchanged (still zero)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at TEN"));
#endif
					return FALSE; // tell the caller not to try another leftwards widening
				}
				else if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
				{
                    // it/they are left-associated, so keep it/them with
                    // what precedes it/them, which means that it/they
                    // belong in beforeSpan, and we can't widen any further
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at ELEVEN"));
#endif
                    return FALSE; // (oldCount and newCount both returned as zero)
				}
				// neither left nor right associated, so handle the
				// same as left-associated
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at TWELVE"));
#endif
				return FALSE;
			}

		} // end of TRUE block for test: if (pOldSrcPhrase->m_bNullSourcePhrase)

	} // end of TRUE block for test: if (oldIndex > oldStartAt && newIndex > newStartAt)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Leftwards ONCE: exiting at THE_END (THIRTEEN)"));
#endif
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE, if the attempt at widening rightwards by a "single" jump
///                         succeeds, otherwise FALSE
/// \param  arrOld          ->  array of old CSourcePhrase instances
/// \param  arrNew          ->  array of new CSourcePhrase instances
/// \param  oldStartAt      ->  starting index in arrOld for parent Subspan
/// \param  oldEndAt        ->  ending (inclusive) index in arrOld for parent Subspan
/// \param  newStartAt      ->  starting index in arrNew for parent Subspan
/// \param  newEndAt        ->  ending (inclusive) index in arrNew for parent Subspan
/// \param  oldStartingPos  ->  the index in arrOld from which we start our rightwards jump
/// \param  newStartingPos  ->  the index in arrNew from which we start our rightwards jump
/// \param  oldCount        <-  ref to a count of the number of CSourcePhrase instances to accept
///                             to the right in our "single" jump within the arrOld array (it may
///                             not be just one - see below)
/// \param  newCount        <-  ref to a count of the number of CSourcePhrase instances to accept
///                             to the right in our "single" jump within the arrNew array (it should
///                             always be just one - see below, because arrNew will NEVER
///                             have any retranslations or placeholders in it)
///
/// \remarks
/// Note: WidenRightwardsOnce() has passed to it either the oldEndAt set to the highest
/// index in arrOld (when bClosedEnd is FALSE in the parent), and newEndAt set to the
/// highest index in arrNew (when bClosedEnd is FALSE in the parent); or, when bCloseEnd
/// is TRUE in the parent, limited values for both oldEndAt and newEndAt (according to the
/// SPAN_LIMIT value -- see AdaptitConstants.h) When the end is 'open' we don't want to
/// cut short successful rightwards widening at some arbitrary point, but let it go as far
/// as possible.
///
/// This function tries to extend a matchup of an in-common word rightwards by one step,
/// and since it is called repeatedly until a failure results, the kick off point will move
/// leftwards for each iteration in both arrOld and arrNew. If there are no placeholders
/// nor retranslations to the immediate right, then we have only to make a simple test for
/// matching m_key values in the CSourcePhrase to the immediate right (i.e. in the array, at
/// the next greater index value) in both arrOld and arrNew. Return TRUE if they match,
/// FALSE if they don't - and returning FALSE indicates we've come to the right bound of an
/// in-common span of CSourcePhrase instances. (Note: if bClosedEnd is FALSE, iterations
/// to match additional rightwards CSourcePhrase instances in the two arrays can continue
/// on until a match failure happens, or the end of one of arrOld or arrNew is encountered.)
///
/// Note: the recursion algorithm must not change the number or order of CSourcePhrase
/// instances in arrOld and arrNew. Only after recursion is completed and the merging is
/// therefore completed can we do cleanup actions which change number of final
/// CSourcePhrase instances (such as removing placeholders in partially destroyed
/// retranslations - and even then, such cleanup is not done in either of arrOld or
/// arrNew)
///
/// Retranslations (and those may end with zero, one or more placeholders) and placeholders
/// complicate the situation significantly. This function is designed to encapsulate these
/// complications within it, so that higher level functions do not have to consider either
/// complication. Also, our algorithms will compose a merged (edited) source text into the
/// document without trying to maintain the integrity of retranslations - and therefore,
/// once the recursions are finished, it may be the case that we have some retranslations
/// which are lost (that's not a problem), some retained (that's not a problem either), and
/// some which have the start or end chopped off (that IS a problem and we must fix it. We
/// do the fix for the latter in a separate function which runs after recursion is
/// completed and which spans whole newly merged document looking for the messed-up
/// retranslation subspans, and fixes things - the fixes remove the adaptations within the
/// retranslation fragments that have lost either end.
///
/// But the WidenRightwardsOnce() function still has to handle retranslations to some
/// extent (see below), and all placeholders which are not placeholders within a
/// retranslation. What follows are the rules...
/// (1) If a placeholder belonging to a retranslation is to the right, include it in
/// commonSpan -- because we must keep any final placeholder sequence belonging to a
/// retranslation with the retranslation since Adapt It treats a retranslation as a unit;
/// (if another such placeholder follows, continue checking rightwards until a
/// non-placeholder is encountered, including each in the commonSpan)
/// (2) When a non-retranslation, and non-placeholder CSourcePhrase is encountered in
/// arrOld, the next-to-the-right CSourcePhrase instance in arrNew is compared (their m_key
/// values are what are checked, for identity) - if the keys are identical, then the
/// rightwards widening has succeeded. At this point, any placeholders traversed in arrOld
/// are added to the oldCount value - in that way, oldCount can be greater than 1; and
/// those placeholders are deemed to lie within the commonSpan. However, if the test of the
/// two keys shows they are not identical, then we've reached the rightmost end of the
/// commonSpan, and since retranslation placeholders belong with their retranslation, in
/// that case we must still include the placeholders within the commonSpan - at its end,
/// and oldCount will be augmented by the number of such placeholders traversed, whereas
/// newCount would be returned as 0, and FALSE returned from the function.
/// (3) The rules for any placeholder which is not within a retranslation are a bit
/// different. Moving rightwards and encountering such a placeholder, any decision about
/// inclusion within commonSpan is deferred until a non-placeholder potential match pair is
/// checked for identity of m_key values. If the latter's keys match, then the widening
/// attempt has succeeded and both the matched CSourcePhrase and its preceding placeholder
/// are accepted into the commonSpan. If the non-placeholder CSourcePhrase pair's m_key
/// values are not identical, then the right bound of the commonSpan has been reached, and
/// we need additional criteria to help us decide whether or not the placeholder should be
/// included in the commonSpan, or considered to belong in the afterSpan. Here is the
/// protocol for deciding this:
/// (a) Check the placeholder, is it left-associated, right-associated, or neither
/// (b) If left-associated, deem it to belong in commonSpan as the latter's final
/// CSourcePhrase instance (we'll support sequences of manually inserted placeholders,
/// although it's highly likely they never will occur);
/// (c) If right-associated, deem it to belong at the start of afterSpan;
/// (d) If it is neither left- nor right-associated, treat it the same as for (c)
///
/// An additional complication is that arrOld will probably contain mergers, but arrNew
/// never will. So when widening, if an merger is encountered, IsMergerAMatch() must be
/// called to determine if the potentially equivalent word sequence is in arrNew starting
/// at an appropriate index value.
/// (There is also a WidenLeftwardsOnce() function which has similar rules, but different
/// in places for obvious reasons, and it doesn't need the bClosedEnd parameter)
bool WidenRightwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount)
{
	newStartAt = newStartAt; oldStartAt = oldStartAt; // avoid compiler warning re unused variables
	CSourcePhrase* pOldSrcPhrase = NULL;
	CSourcePhrase* pNewSrcPhrase = NULL;
	oldCount = 0;
	newCount = 0;
	if (oldStartingPos > oldEndAt || newStartingPos > newEndAt)
	{
		// we are past the parent's right bound, so can't widen to the right any further
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at ONE"));
#endif
		return FALSE;
	}
	int oldIndex = oldStartingPos;
	int newIndex = newStartingPos;
	if (oldIndex < oldEndAt && newIndex < newEndAt)
	{
		pOldSrcPhrase = arrOld.Item(oldIndex);
		pNewSrcPhrase = arrNew.Item(newIndex);

#if defined( myLogDebugCalls) && defined(_DEBUG)
		if (oldIndex == 42)
		{
//			int break_point = 1;
		}
#endif
        // The first test should be for the most commonly occurring situation - a
        // non-merged, non-placeholder, non-retranslation CSourcePhrase potential pair;
        // we also don't mind if it is one from a retranslation provided it is not a
        // placeholder instance
		if (pOldSrcPhrase->m_nSrcWords == 1 && !pOldSrcPhrase->m_bNullSourcePhrase)
		{
			if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			{
				// we can extend the commonSpan successfully rightwards to this
				// pair of instances
				oldCount++;
				newCount++;
				return TRUE;
			}
			else
			{
				// a non-match means that the right bound for the commonSpan has been
				// reached
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at TWO -- the single-single mismatch"));
#endif
				return FALSE;
			}
		}
		// The second test should be for pOldSrcPhrase being a merger (if so, it can't be a
		// placeholder nor within a retranslation) because that is more likely to happen
		// than the other complications we must handle. We'll also handle fixedspace
		// conjoining here too - and accept a non-conjoined identical word pair in arrNew
		// as a match (ie. we won't require that ~ also conjoins the arrNew's word pair)
		else if (pOldSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger, or conjoined pseudo-merger
			wxASSERT(!pOldSrcPhrase->m_bRetranslation);
			wxASSERT(!pOldSrcPhrase->m_bNullSourcePhrase);
			int numWords = pOldSrcPhrase->m_nSrcWords;
 			bool bIsFixedspaceConjoined = IsFixedSpaceSymbolWithin(pOldSrcPhrase);
            // If it's a normal (non-fixedspace) merger, then we do the same check as for a
            // fixedspace merger, because if ~ is still in the edited source text, then
            // that will parse to a conjoined (pseudo merger) CSourcePhrase with
            // m_nSrcWords set to 2 - so check for either of these, and if there isn't a
            // match, then check if the old instance really is a fixedspace merger, and is
            // so try matching each word of the two word sequence at the approprate
            // location, without any ~, -- if that succeeds, treat it as a match; if none
            // of that gives a match, then there isn't one and we are done - return FALSE
            // if so
			if (bIsFixedspaceConjoined)
			{
				if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
				{
					// we can extend the commonSpan successfully rightwards to this
					// pair of instances, each has the fixedspace (~) marker in its
					// m_key member
					oldCount++;
					newCount++;
					return TRUE;
				}
				else
				{
					// the arrNew array at the match location doesn't have a fixedspace
					// pseudo-merger, so attempt to match the words there individually
					if (newIndex + numWords - 1 <= newEndAt)
					{
                        // there are enough words available for a match attempt... get the
                        // individual words in pOldSrcPhrase and then check for a match
                        // with two successive new CSourcePhrase instances' m_key values
						int newStartingIndex = newIndex;
						SPList::Node* pos = pOldSrcPhrase->m_pSavedWords->GetFirst();
						wxASSERT(pos != NULL);
						wxString word1 = pos->GetData()->m_key;
						pos = pos->GetNext();
						wxString word2 = pos->GetData()->m_key;
						wxString newWord1 = arrNew.Item(newStartingIndex)->m_key;
						wxString newWord2 = arrNew.Item(newStartingIndex + 1)->m_key;
						if (word1 == newWord1 && word2 == newWord2)
						{
							// consider this a match
							oldCount++;
							newCount += 2;
							return TRUE;
						}
						else
						{
							// no match
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at THREE - the merger-sequence mismatch"));
#endif
							return FALSE;
						}
					}
					else
					{
						// not enough words available, so no match is possible
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at FOUR"));
#endif
						return FALSE;
					}
				}
			} // end of TRUE block for test: if (bIsFixedspaceConjoined)
			else
			{
                // it's a plain vanilla merger, so get the ending index for it and ensure
                // that lies within arrNew's bounds, if so we assume then that arrNew may
                // have a matching word sequence, so we test for the match
				int newEndIndex = newStartingPos + numWords - 1;
				if (newEndIndex <= newEndAt)
				{
					// there is a potential match
					bool bMatched = IsMergerAMatch(arrOld, arrNew, oldIndex, newIndex);
					if (bMatched)
					{
						oldCount++; // it now equals 1
						newCount += numWords; // count matched CSourcePhrase instances
											  // (each stores only a single word)
						return TRUE;
					}
					else
					{
						// didn't match match, so we are at the end of commonSpan
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at FIVE"));
#endif
						return FALSE;
					}
				}
				else
				{
					// not enough words for a match, so return FALSE
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at SIX"));
#endif
					return FALSE;
				}
			} // end of else block for test: if (bIsFixedspaceConjoined)
		} // end of TRUE block for test: else if (pOldSrcPhrase->m_nSrcWords > 1)

        // If none of the above apply, the complications are placeholders and
        // retranslations. Test first for pOldSrcPhrase being a placeholder, and then
        // within the TRUE block distinguish between placeholders within retranslations and
        // one or more plain vanilla (i.e. manually inserted previously) placeholders which
        // are not in a retranslation. (We expect that a user would never insert two or
        // more placeholders manually at the same location, but just in case he does, we'll
        // handle them.) Our approach to plain vanilla placeholder(s) is to defer action
        // until we know if there is a match of the farther-out abutting CSourcePhrase
        // instance in arrOld with the potential matching one in arrNew - if the match
        // obtains, then the enclosed placeholder(s) are taken into commonSpan; if the
        // matchup fails, then the placeholder(s) are at the boundary of commonSpan and
        // afterSpan - and on which side of it will then depend on whether there is
        // indication of left association (then it/they belong in commonSpan) or right
        // association (then it/they belong in afterSpan) or no indication of association
        // -- in which case we have no criterion to guide us, so we'll assume that it/they
        // should be in afterSpan (and so get removed in the merger process)
		if (pOldSrcPhrase->m_bNullSourcePhrase)
		{
			// it's a placeholder
			bool bItsInARetranslation = FALSE;
			if (pOldSrcPhrase->m_bRetranslation)
			{
				// It's the first placeholder at the end of a retranslation.
				//
                // Our approach for these is to keep such placeholder(s) with the
				// retranslation unit to which they belong - so we find out how many of
				// them there are, and make them be within commonSpan. Then we go to the
				// next index to see if we have a match of the first CSourcePhrase
				// non-placeholder instance (which isn't within the retranslation, though
				// there could be a second retranslation following, but that we don't care
				// about because it wouldn't have a placeholder starting it) after the
				// last placeholder, matching against the one CSourcePhrase instance in
				// arrNew at newIndex - and if there is a match, we have succeeded in
				// extending rightwards (return TRUE after setting count values), but if
				// there is no match, then we must close off commonSpan & return FALSE
				bItsInARetranslation = TRUE;
			} // end of TRUE block for test: if (pOldSrcPhrase->m_bRetranslation)

            // It's either a plain vanila (i.e. user-manually-created) placeholder, not one
            // in a retranslation; or if bItsInARetranslation is TRUE, its the first
            // placeholder which is at the end of a retranslation section.

			// Get past this and any additional non-retranslation placeholders until
			// either we reach oldEndAt and can't check further, or we get a
			// non-placeholder which is at or before oldEndAt which we can test for a
			// match of m_Key values with the one at arrNew; decide what to do
			// according to the protocols spelled out above.
			int nExtraCount = 0;
			int nextIndex = oldIndex + 1;
			CSourcePhrase* pNextSrcPhrase = NULL;
			bool bFoundNonPlaceholder = FALSE;
			while (nextIndex <= oldEndAt)
			{
				pNextSrcPhrase = arrOld.Item(nextIndex);
				if (pNextSrcPhrase->m_bNullSourcePhrase)
				{
					// we've another placeholder next door, so keep moving forwards
					nExtraCount++;
					nextIndex++;
					continue;
				}
				else
				{
					// we've found a non-placeholder within the parent span, so check
					// it out for a match with what is at newIndex in arrNew (it could be
					// at the start of a retranslation, but we don't care - we treat it as
					// a normal instance at this point in the algorithm)
					bFoundNonPlaceholder = TRUE;
					// it could be a single-word CSourcePhrase, or a merger, so we
					// must deal with both possibilities
					if (pNextSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					{
						// it's not a merger - so check for a match with the one in arrNew
						if (pNextSrcPhrase->m_key == pNewSrcPhrase->m_key)
						{
							// we can extend the commonSpan successfully rightwards to this
							// pair of instances, pulling the traversed placeholders
							// into commonSpan along with the non-placeholder one at
							// their right -- this also applies to ones within a retranslation
							oldCount++; // counts the placeholder at oldIndex
							oldCount += nExtraCount; // adds the count of the extra ones traversed
							oldCount++; // counts this pNextSrcPhrase which isn't a placeholder
							newCount++; // this now equals 1
							return TRUE;
						}
						else
						{
							// a non-match
							if (bItsInARetranslation)
							{
								// all the one or more placeholders belong at the end of
								// commonSpan, so return oldCount = the number of
								// placeholders traversed, newCount = 0, and FALSE
								oldCount++; // counts the placeholder at oldIndex
								oldCount += nExtraCount; // adds the count of the extra ones traversed
								//newCount is unchanged (still zero)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at SEVEN"));
#endif
								return FALSE;
							}
							else
							{
                                // a non-match for the non-placeholder CSourcePhrase
                                // immediately following manually created placeholders
                                // means that the right bound for the commonSpan has been
                                // reached; so here we must work out if the right-most
                                // placeholder is left associated, or the left-most
                                // placeholder is right associated, and if neither, we'll
                                // assign it/them to afterSpan
								int indexOfLeftmostPlaceholder = oldIndex;
								// assign the pointer at that location to pNextSrcPhrase
								// since we don't need the latter for any other purpose now
								pNextSrcPhrase = arrOld.Item(indexOfLeftmostPlaceholder);
								int indexOfRightmostPlaceholder = oldIndex + nExtraCount - 1;
								CSourcePhrase* pRightmostPlaceholder = arrOld.Item(indexOfRightmostPlaceholder);
								if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
								{
									// it/they are left-associated, so keep it/them with
									// what precedes it/them, which means that it/they
									// belong in commonSpan, and we can't widen any further
									oldCount++; // counts the placeholder at oldIndex
									oldCount += nExtraCount; // adds the count of the extra ones traversed
									//newCount is unchanged (still zero)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at EIGHT"));
#endif
									return FALSE; // tell the caller not to try another leftwards widening
								}
								else if (IsRightAssociatedPlaceholder(pNextSrcPhrase))
								{
                                    // it/they are right-associated, so keep it/them with
                                    // what follows it/them, which means that it/they
                                    // belong in at the start of afterSpan, and we can't
                                    // widen commonSpan any further
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at NINE"));
#endif
									return FALSE; // (oldCount and newCount both returned as zero)
								}
								// neither left nor right associated, so handle the
								// same as right-associated
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at TEN"));
#endif
								return FALSE;
							} // end of else block for test: if (bItsInARetranslation)
						} // end of else block for test: if (pPrevSrcPhrase->m_key == pNewSrcPhrase->m_key)
					} // end of TRUE block for test: if (pNextSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					else
					{
						// it's a merger (we'll assume no fixed-space conjoining, but just
						// a normal merger) -- pass in nextIndex here, as we are using
						// that index for our 'looking ahead' within arrOld
						int numWords = pNextSrcPhrase->m_nSrcWords;
                        // the loop condition tests for nextIndex <= oldEndAt, so we don't
                        // need to repeat the test here, but here we do need to test to
                        // ensure that nextIndex + numWords - 1 is <= to newEndAt, so that
                        // there are enough words in arrNew for a potential match - if not,
                        // we return FALSE
						if (newIndex + numWords - 1 <= newEndAt)
						{
							// there are enough words for the match attempt
							bool bMatched = IsMergerAMatch(arrOld, arrNew, nextIndex, newIndex);
							if (bMatched)
							{
								oldCount++; // it now equals 1
								newCount += numWords; // no mergers in arrNew (other than fixedspace
										// pseudo mergers), so we count the requisite number of words
										// which belong in commonSpan
								return TRUE;
							}
							else
							{
								// no match
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at ELEVEN"));
#endif
								return FALSE;
							}
						} // end of TRUE block for test: if (newIndex + numWords - 1 <= newEndAt)
						else
						{
							// not enough words available
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at TWELVE"));
#endif
							return FALSE;
						} // end of else block for test: if (newIndex + numWords - 1 <= newEndAt)
					} // end of else block for test: if (pNextSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					break;
				} // end of else block for test: if (pNextSrcPhrase->m_bNullSourcePhrase)

			} // end of loop: while (nextIndex <= oldEndAt)

            // Here we only need to handle the loop ending without any non-placeholder
            // instance being found (i.e. we traversed zero or more additional
            // placeholders, but then got to the span right boundary without finding any
            // non-placeholder CSourcePhrase instance) - & that means that the one or more
            // placeholders are either all in commonSpan, or all in afterSpan - we have to
            // work out which is the case below. But if a non-placeholder was found, then
            // all the needed decisions will have been made already within the loop and
            // control returned to the caller before the loop ended
			if (!bFoundNonPlaceholder)
			{
                // a non-match for the non-placeholder CSourcePhrase
                // immediately following manually created placeholders
                // means that the right bound for the commonSpan has been
                // reached; so here we must work out if the right-most
                // placeholder is left associated, or the left-most
                // placeholder is right associated, and if neither, we'll
                // assign it/them to afterSpan
				int indexOfLeftmostPlaceholder = oldIndex;
				// assign the pointer at that location to pNextSrcPhrase
				// since we don't need the latter for any other purpose now
				pNextSrcPhrase = arrOld.Item(indexOfLeftmostPlaceholder);
				int indexOfRightmostPlaceholder = oldIndex + nExtraCount - 1;
				CSourcePhrase* pRightmostPlaceholder = arrOld.Item(indexOfRightmostPlaceholder);
				if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
				{
					// it/they are left-associated, so keep it/them with
					// what precedes it/them, which means that it/they
					// belong in commonSpan, and we can't widen any further
					oldCount++; // counts the placeholder at oldIndex
					oldCount += nExtraCount; // adds the count of the extra ones traversed
					//newCount is unchanged (still zero)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at THIRTEEN"));
#endif
					return FALSE; // tell the caller not to try another leftwards widening
				}
				else if (IsRightAssociatedPlaceholder(pNextSrcPhrase))
				{
                    // it/they are right-associated, so keep it/them with
                    // what follows it/them, which means that it/they
                    // belong in at the start of afterSpan, and we can't
                    // widen commonSpan any further
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at FOURTEEN"));
#endif
					return FALSE; // (oldCount and newCount both returned as zero)
				}
				// neither left nor right associated, so handle the
				// same as right-associated
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at FIFTEEN"));
#endif
				return FALSE;
			}

		} // end of TRUE block for test: if (pOldSrcPhrase->m_bNullSourcePhrase)

	} // end of TRUE block for test: if (oldIndex < oldEndAt && newIndex < newEndAt)
#if defined( LEFTRIGHT) && defined(_DEBUG)
		wxLogDebug(_T("Rightwards ONCE: exiting at THE_END (SIXTEEN)"));
#endif
	return FALSE; // we didn't make any matches, so widening must halt
}

//////////////////////////////////////////////////////////////////////////////////////
/// \return                         pointer to the Subspan instance which has maximum
///                                 composite width (ie. sum of widths from both arrays)
/// \param  arrOld              ->  array of old CSourcePhrase instances
/// \param  arrNew              ->  array of new CSourcePhrase instances
/// \param  pParentSubspan      ->  the Subspan which we are decomposing by finding the
///                                 longest in-common subspans within arrOld and arrNew
///                                 which are matched
/// \param  limit               ->  the span width limit value, used for defining a
///                                 less-than-all width, or if -1, use-all width for the
///                                 subspans in which the set of unique in-common words
///                                 are obtained
/// \remarks
/// We take the parent Subspan instance, use it's bounds to delineate subspans of arrOld
/// and arrNew (typically SPAN_LIMIT amound of CSourcePhrase instances, but could be less
/// if not that many are available for the parent Subspan), and then from those two
/// subspans obtain an array of unique in-common words (yes, words, not CSourcePhrase
/// instances) which are common to both subspans. These words enable us to find matchup
/// locations within arrOld and arrNew, and ultimately to get the longest such matchup -
/// the latter jobs are done by calling GetAllCommonSubspansFromOneParentSpan. Internally
/// we create some arrays that we need for doing these tasks, an array for the unique
/// in-common words, one for all Subspan instances that we find - and a parallel array of
/// their composite width values. The longest Subspan is taken and all the others (which
/// are all on the heap) are deleted, and the longest is then used in the caller, along
/// with the parent Subspan bounds, to work out the beginning and ending indices for the
/// tuple of beforeSpan, commonSpan, and afterSpan.
Subspan* GetMaxInCommonSubspan(SPArray& arrOld, SPArray& arrNew, Subspan* pParentSubspan, int limit)
{
	wxArrayString arrUniqueInCommonWords;
	wxArrayPtrVoid arrSubspans;
	wxArrayInt arrWidths;
	// The first task is to populate the arrUniqueInCommonWords array, based on the data
	// in pParentSubspan
	int wordCount = GetWordsInCommon(arrOld, arrNew, pParentSubspan, arrUniqueInCommonWords, limit);
	wordCount = wordCount; // avoid compiler warning
	// The second task is to use the array of in-common words to get the set of all
	// possible Subspan instances definable within the bounds of pParentSpan (when the
	// parent span is the rightmost one, it's bClosedEnd member will be FALSE, and in that
	// case it is possible that one of the spans will end beyond the bounds set by the
	// oldEndPos and newEndPos members - potentially going as far as the ends of arrOld or
	// arrNew or the ends of both), and the in-sync array of width values (a composite
	// constructed from the widths of the subspan in arrOld plus the width of the matched
	// subspan in arrNew). The Subspan pointers, and the widths, are stored in arrSubspans
	// and arrWidths, respectively.
	bool bThereAreSpans = GetAllCommonSubspansFromOneParentSpan(arrOld, arrNew, pParentSubspan,
				&arrUniqueInCommonWords, &arrSubspans, &arrWidths, pParentSubspan->bClosedEnd);
	if (bThereAreSpans)
	{
		wxASSERT(arrSubspans.GetCount() == arrWidths.GetCount()); // verify they are in-sync

		// find the maximum width stored Subspan instance, delete the rest from the heap,
		// and return the max one to the caller in pMaxInCommonSubspan, clear the arrays
		int widthsCount = arrWidths.GetCount();
		int maxWidth = arrWidths.Item(0); // initialize to composite width of the first
#if defined( myLogDebugCalls) && defined(_DEBUG)
		wxLogDebug(_T("Composite WIDTH =  %d  for index value  %d"),maxWidth, 0);
#endif
		int lastIndexForMax = 0; // initialize
		int i;
		for (i = 1; i < widthsCount; i++)
		{
#if defined(_DEBUG) && defined(_RECURSE_)
			Subspan* pSub = (Subspan*)arrSubspans.Item(i);
			wxLogDebug(_T("GetMaxInCommonSubspan(), loop index %d, Composite WIDTH  %d  for Subspan: (%d,%d) <-> (%d,%d)"),
						i, arrWidths.Item(i), pSub->oldStartPos, pSub->oldEndPos, pSub->newStartPos, pSub->newEndPos);
#endif
			if (arrWidths.Item(i) > maxWidth)
			{
				// only accept a new value if it is bigger, this way if any are equal
				// size, we'll take the index for the first of them, which is a better
				// idea when trying to work left to right
				maxWidth = arrWidths.Item(i);
				lastIndexForMax = i;
			}
		}
		Subspan* pMaxInCommonSubspan = (Subspan*)arrSubspans.Item(lastIndexForMax);
#if defined(_DEBUG) && defined(_RECURSE_)
		wxLogDebug(_T("index chosen for Max in-common span = %d  for Subspan:  (%d,%d) <-> (%d,%d)"),
			lastIndexForMax, pMaxInCommonSubspan->oldStartPos, pMaxInCommonSubspan->oldEndPos,
			pMaxInCommonSubspan->newStartPos, pMaxInCommonSubspan->newEndPos );
#endif

		// delete the rest
		arrWidths.Clear();
		for (i = 0; i < widthsCount; i++)
		{
			// delete all except the one we are returning to the caller
			if (i != lastIndexForMax)
			{
				if ((Subspan*)arrSubspans.Item(i) != NULL) // whm 11Jun12 added NULL test
				{
#if defined(_DEBUG) && defined(myLogDebugCalls)
					countCommonSpanDeletions++; // because there are many made & rejected, the counts for these are large
#endif
					delete (Subspan*)arrSubspans.Item(i);
				}
			}
		}
		arrSubspans.Clear();
		return pMaxInCommonSubspan; // Returning a commonSpan Subspan instance means the
					 // parent span is segmentable into beforeSpan, commonSpan and
					 // and afterSpan, thereby defining a child tuple - which must
					 // then be processed by a call of RecursiveTupleProcessor(),
					 // passing in that tuple
	}
    // There were no in-common words, and hence no commonSpan type of Subspan instances
    // created in order to find a child in-common Subspan; so return NULL because that
    // will tell the caller to halt recursion and use the parent span to instead do the
    // merge of the relevant CSourcePhrase instances from arrNew
	return NULL;
}

bool IsMatchupWithinAnyStoredSpanPair(int oldPosStart, int oldPosEnd, int newPosStart,
						int newPosEnd, wxArrayPtrVoid* pSubspansArray)
{
	int count = pSubspansArray->GetCount();
	if (count == 0)
		return FALSE;
	int index;
	Subspan* pSubspan = NULL;
	for (index = 0; index < count; index++)
	{
		pSubspan = (Subspan*)pSubspansArray->Item(index);
		if (oldPosStart >= pSubspan->oldStartPos && oldPosEnd <= pSubspan->oldEndPos)
		{
            // oldPosStart to oldPosEnd is within the subspan derived from arrOld, so this
            // is a candidate for a possible matchup
			if (newPosStart >= pSubspan->newStartPos && newPosEnd <= pSubspan->newEndPos)
			{
				// this matchup lies within this span, so return TRUE;
				return TRUE;
			}
		}
		// no matchup, so try another of the stored Subspan instances
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                     TRUE in all circumstances except when there are no unique
///                             in-common words in the pUniqueCommonWordsArray passed in,
///                             in which case FALSE is returned. (FALSE is equivalent to
///                             the caller testing that the array of unique common words
///                             is empty and that no Subspan instances were created and
///                             stored)
/// \param  arrOld           -> array of old CSourcePhrase instances (may have mergers,
///                             placeholders, retranslation, fixedspace conjoinings as
///                             well as minimal CSourcePhrase instances)
/// \param  arrNew           -> array of new CSourcePhrase instances (will only be minimal
///                             CSourcePhrase instances, but could also contain fixedspace
///                             conjoined instances too)
/// \param  pParentSubspan   -> ptr to the parent Subspan instance which is to be analysed to
///                             collect the set of all possible commonSpan Subspans which
///                             get stored in pSubspansArray
/// \param  pUniqueCommonWordsArray -> array of unique in-common words derived from the current
///                             (parent) Subspan's old and new subspan pair
/// \param  pSubspansArray   <- store each commonSpan Subspan ptr instance we obtain, in here
/// \param  pWidthsArray     <- add here the sum of the span widths in arrOld and arrNew which
///                             form a matched pair of subspans within the Subspan instance
///                             last stored in pSubspansArray (since the two subspans may
///                             have different widths, we get a more accurate idea of
///                             which is the "widest" if we do the comparisons with values
///                             formed by summing the widths of each of the matched pairs
/// \param  bClosedEnd      ->  the parent Subspan's bClosedEnd value (if FALSE, then in this
///                             function matching words in common can go beyond limited
///                             end locations we otherwise define; but if TRUE, the bounds must
///                             be obeyed. It can be FALSE only when the parent is the
///                             rightmost afterSpan so far defined. That is, the afterSpan
///                             instances at the right edge of the hierarchy of tuples will
///                             each have their bClosedEnd values cleared to FALSE.)
///
/// \remarks
/// This function takes the passed in array of unique in-common words within a (limited
/// width) span of CSourcePhrase instances from both arrOld and arrNew, and for each word
/// in the array, and proceeding from left to right, occurrences of the word in both
/// arrOld and arrNew are searched for. Each time a match is made in each array, the
/// location pair of indices is considered a "matchup". Such matches are then widened both
/// leftwards and rightwards in each of arrOld and arrNew, so long as the CSourcePhrase
/// instances have matching keys. (There are other complications, such as merger,
/// placeholders, retranslations and possibly fixedspace pseudo-mergers. These are all
/// dealt with, but encapsulated within lower level functions - see the comments for
/// WidenLeftwardsOnce() and WidenRightwardsOnce(), we won't discuss these issues here.)
///
/// Not every matchup can be widened, spurious accidental matchups can occur - these will
/// tend to be of limited width, but we must still derive them and store them until it is
/// determined that they can be abandoned. After widening, a Subspan is created on the heap
/// to retain the index information thus gained. All such matchups for all the unique
/// words are found and widening is attempted for each. New matchups, of course, will often
/// be located within existing Subspan limits - so we don't widen when that is the case,
/// just abandon the matchup and try for the next matchup (we don't need to find the same
/// Subspan a second time - the set of Subspans will be far fewer than the set of unique
/// words being tested). A given matchup may occur within the limited parent Subspan a
/// number of times, so we have to do the job with a nested loop. The other loop finds the
/// next location for a word in arrOld, and the inner loop iterates over all possible
/// locations at which the same word is located in the section of arrNew being considered
/// (SPAN_LIMIT number of instances of consecutive CSourcePhrase instances, SPAN_LIMIT
/// being 80 (currently)), and for each matchup widening is attempted provided that the
/// matchup index values don't lie within an already found Subspan instance.
///
/// The "width" values we store are the sum of counts of the consecutive CSourcePhrase
/// instances in the arrOld limited span within that larger array, and the consecutive
/// CSourcePhrase instances in the arrNew limited span within that larger array. For
/// example, if a delineated Subspan has an arrOld sequence of 5 CSourcePhrase instances
/// matched with an arrNew sequence of 8 CSourcePhrase instances, should the "width" be 5
/// or 8 when we compare with other Subspan instances delineated? To avoid having to find
/// the larger of the two, it is simpler to just add 5+8 = 13, and store 13 in
/// pWidthsArray. That way, it is easier to see which Subspan really is the maximal one;
/// the minimal one would have a combined width of 2 (one CSourcePhrase instance from each
/// of arrOld and arrNew).
///
/// The Subspan ptr instances are on the heap, and a higher level function will find which
/// is the maximal one - when that is done, it is retained and the others must be deleted
/// so as not to leak memory. As we recursively iterate though the data, we deal with
/// smaller and smaller subspans as the nesting level deepens, and progressively from left
/// to right across the whole of arrOld and arrNew as recursion level decreases. (That's a
/// simplification, but you get the idea. I hope.)
///
/// Note: widening a matchup to the right must stop at the bounding oldEndPos index within
/// arrOld, and the newEndPos index within arrNew. However, the rightmost tuple's afterSpan
/// at every nesting level of the right hand edge of the hierachy of tuples must have the
/// afterSpan Subspan's member, bClosedEnd, set to FALSE. The FALSE value is used to permit
/// widening to the right to go beyond the passed in oldEndPos and newEndPos values for the
/// parent afterSpan being analysed here - potentially as far as the end bounding index
/// values for arrOld and arrNew, respectively, because there is no good reason why
/// processing should arbitrarily be cut short when widening is successfully happening into
/// the as-yet-unprocessed right-hand ends of arrOld and arrNew. This can potentially save
/// a lot of processing time, especially if the user's edits of the source text were all
/// near the start of the exported source text file.
///
/// Calls GetNextMatchup() which returned TRUE if a matchup succeeded, and FALSE if it
/// didn't; and when successful, the signature returns counts for how many consecutive
/// CSourcePhrase instances in arrOld and arrNew are involved in the matchup - this
/// information is then passed in to the WidenMatchup() function.
////////////////////////////////////////////////////////////////////////////////////////
bool GetAllCommonSubspansFromOneParentSpan(SPArray& arrOld, SPArray& arrNew,
			Subspan* pParentSubspan, wxArrayString* pUniqueCommonWordsArray,
			wxArrayPtrVoid* pSubspansArray, wxArrayInt* pWidthsArray, bool bClosedEnd)
{
	// store in local variables the start and end (both fixed) index values for the span
	// within arrOld which the passed in pSubspan defines
	int oldParentSpanStart = pParentSubspan->oldStartPos;
	int oldParentSpanEnd = pParentSubspan->oldEndPos;
	// store in local variables the start and end (both fixed) index values for the span
	// within arrNew which the passed in pSubspan defines
	int newParentSpanStart = pParentSubspan->newStartPos;
	int newParentSpanEnd = pParentSubspan->newEndPos;

	// define iterators for scanning across the two spans as defined above; must start
	// always at the start of the parent span (not before), otherwise we can get an
	// infinite loop because we keep matching an outside-of-boundaries early subspan
	int oldIndex = oldParentSpanStart; // use for the subspan within arrOld
	int newIndex = newParentSpanStart; // use for the subspan within arrNew

	// clear pSubspansArray prior to filling it with pSubspan instances below, likewise
	// for pWidthsArray -- note, the latter stores the sum of the arrOld and arrNew
	// matched spans' widths, not just one of these
	pSubspansArray->Clear();
	pWidthsArray->Clear();

    // if there are no unique in-common words, just return FALSE - the caller also can
    // detect that pSubspansArray is empty and so too would be pWidthsArray and act
    // appropriately
	if (pUniqueCommonWordsArray->IsEmpty())
		return FALSE;
	// next four delineate a matchup location, usually all oldMatchedStart = oldMatchedEnd
	// = 1, and newMatchedStart = newMatchedEnd = 1. However, mergers etc can result in
	// different values. A matchup failure will return wxNOT_FOUND in these, from the
	// GetNextCommonSpan() call. These four are initialized internally in
	// GetNextCommonSpan() to each be wxNOT_FOUND when the latter function is entered
	int oldMatchedStart;
	int oldMatchedEnd;
	int newMatchedStart;
	int newMatchedEnd;
	// the next two are index variables which define the rightmost index values in arrOld
	// and arrNew that have been considered in the loop at that point in time, each
	// iteration of the inner while loop advances newLastIndex, and each iteration of the
	// outer while loop advances newLastIndex; and when the newLastIndex is returned as
	// wxNOT_FOUND, the outer loop's word is advanced to the next location in arrOld and
	// the inner loop tried all over again from start to finish etc. These variables allow
	// the start-form location to be determined for each successive iteration within one
	// of the while loops
	int oldLastIndex = wxNOT_FOUND;
	int newLastIndex = wxNOT_FOUND;

	// the outer loop ranges over all the unique words within pUniqueCommonWordsArray -
	// all must be tried (but GetNextCommonSpan() abandons processing when a matchup is
	// within a Subspan instance already stored in the pSubspansArray array, to save time
	// by avoiding to redundantly delineate an already found Subspan)
	wxString searchWord;
	bool bMatchupSucceeded;
	int wordCount = pUniqueCommonWordsArray->GetCount();
	int wordIndex;
	for (wordIndex = 0; wordIndex < wordCount; wordIndex++)
	{
		searchWord = pUniqueCommonWordsArray->Item(wordIndex);
		if (searchWord.IsEmpty())
			continue; // ignore empty strings (there shouldn't be any, but I got one
					  // when testing, so best to simply ensure they are ignored
		// all possible combinations of the searchWord at all locations it occurs in both
		// arrOld and arrNew must be tried - pSubspansArray and pWidthsArray are populated
		// as processing progresses - the elements of these two arrays are synced by index

        // the other while loop tries for all matches of searchWord within the old-text's
        // subspan within arrOld
		while (TRUE)
		{
			// the inner while loop tries for all matches of searchWord within the
			// new-text's subspan within arrNew
			while (TRUE)
			{
				// try for the first or next matchup (oldMatchedStart to newMatchedEnd are
				// uninitialized, but set to -1 when the function has just been entered)
				bMatchupSucceeded = GetNextCommonSpan(searchWord, arrOld, arrNew,
					oldParentSpanStart, newParentSpanStart, oldIndex, oldParentSpanEnd,
					newIndex, newParentSpanEnd, oldMatchedStart, oldMatchedEnd,
					newMatchedStart, newMatchedEnd, oldLastIndex, newLastIndex,
					bClosedEnd, pSubspansArray, pWidthsArray);
				// determine whether the inner loop has finished or not, and if not,
				// update the oldIndex and newIndex values to be one greater than the
				// oldLastIndex and newLastIndex values respectively, provided the latter
				// are positive; but if the latter are wxNOT_FOUND, the inner loop is
				// ended and the outer loop iterates to try searchWord at the next
				// possible location within arrOld's subspan
				if (bMatchupSucceeded)
				{
					// a new pSubspan has been stored, and it's composite width stored
					// too, so just prepare for a new iteration
					newIndex = newLastIndex + 1;
					if (newIndex >= newParentSpanEnd)
					{
						newIndex = newParentSpanStart; // start over when next outer
													   // loop location is tested
						break; // no room for looking ahead, so the inner loop ends
					}
				}
				else
				{
                    // we must test oldLastIndex first, because if it returns as -1 because
                    // the inner loop has exited and there are no more Finds possible in
                    // the outer loop, then we don't want the inner loop to chug along any
                    // more (it's newLastIndex value won't be -1 typically at such a time),
                    // so we have to test for oldLastIndex being -1 and if so force
                    // newLastIndex to -1 so that both while loops exit immediately.
					if (oldLastIndex == wxNOT_FOUND)
					{
						// force exit of both while loops
						newLastIndex = wxNOT_FOUND;
					}
                    // if newLastIndex is wxNOT_FOUND, we can't do another inner loop
                    // iteration (it would be this value if the newIndex was out-of-bounds
                    // (we'll not code explicitly for this, it shouldn't happen; but we
                    // will code for the following condition:) or if the internal Find...()
                    // call fails in arrNew
					if (newLastIndex == wxNOT_FOUND)
					{
						// the inner loop is done, for the given searchWord at the given
						// location within arrOld
						newIndex = newParentSpanStart; // start over when next outer loop
													   // location is tested
						break;
					}
					else
					{
						// no matchup, but we did advance to a new location in arrNew from
						// which we can do a Find...() starting from what lies beyond that
						// location - so prepare for the next iteration
						newIndex = newLastIndex + 1;
						if (newIndex >= newParentSpanEnd)
						{
							newIndex = newParentSpanStart; // start over when next outer
														   // loop location is tested
							break; // no room for looking ahead, so the inner loop ends
						}
					}
				}
			} // end of inner while loop with test: while (TRUE)
			if (oldLastIndex == wxNOT_FOUND)
			{
				// the searchWord is not found further ahead in the subspan within arrOld,
				// so exit the outer loop
				break;
			}
			else
			{
                // no matchup, but we have not yet had a match failure in arrOld from a
                // Find...(); so starting from what lies beyond the oldLastIndex location,
                // prepare a new oldIndex value for the next iteration in arrOld
				oldIndex = oldLastIndex + 1;
				if (oldIndex >= oldParentSpanEnd)
				{
					// no room, so the outer loop is done
					break;
				}
			}
		} // end of outer while loop with test: while (TRUE)

		// prepare for a new searchWord, by initializing the start locations to start of
		// each subspan again
		oldIndex = oldParentSpanStart;
		newIndex = newParentSpanStart;
	} // end of for loop: for (wordIndex = 0; wordIndex < wordCount; wordIndex)

    // TRUE returned means that one or more matchups were made, & so one will be found to
    // be the biggest or as large as any others, and so recursion will happen; FALSE
    // returned means we didn't find any matchups (most unlikely!)
	return pWidthsArray->GetCount() > 0;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                     TRUE for a successful matchup, FALSE otherwise
/// \param  word             -> the word being searched for at or after the starting index
///                             locations in arrOld and arrNew
/// \param  arrOld           -> array of old CSourcePhrase instances (may have mergers,
///                             placeholders, retranslation, fixedspace conjoinings as
///                             well as minimal CSourcePhrase instances)
/// \param  arrNew           -> array of new CSourcePhrase instances (will only be minimal
///                             CSourcePhrase instances, but could also contain fixedspace
///                             conjoined instances too)
/// \param  oldStartAt       -> index at which span is left-bounded (right-bounded for RTL
///                             languages)
/// \param  newStartAt       -> index at which span is right-bounded (left-bounded for RTL
///                             languages)
/// \param  oldStartFrom     -> index from which to start searching in arrOld
/// \param  oldEndAt         -> index in arrOld at which last search is to be tried
/// \param  newStartFrom     -> index from which to start searching in arrNew
/// \param  newEndAt         -> index in arrNew at which last search is to be tried
/// \param  oldMatchedStart  <- index of initial matched CSourcePhrase in arrOld,
///                             wxNOT_FOUND if no match
/// \param  oldMatchedEnd    <- index of last matched CSourcePhrase in arrOld,
///                             wxNOT_FOUND if no match
/// \param  newMatchedStart  <- index of initial matched CSourcePhrase in arrNew,
///                             wxNOT_FOUND if no match
/// \param  newMatchedEnd    <- index of last matched CSourcePhrase in arrNew,
///                             wxNOT_FOUND if no match
/// \param  oldLastIndex     <- index of CSourcePhrase in which the word matched in arrOld,
///                             (but it may not have resulted in a matchup) but we need it
///                             in the caller so that we can retry the call starting from
///                             the index following it, provided that index is in the span;
///                             set to wxNOT_FOUND if the Find... operation fails,
///                             otherwise it is +ve
/// \param  newLastIndex     <- index of CSourcePhrase in which the word matched in arrNew,
///                             (but it may not have resulted in a matchup) but we need it
///                             in the caller so that we can retry the call starting from
///                             the index following it, provided that index is in the span;
///                             set to wxNOT_FOUND if the Find... operation fails,
///                             otherwise it is +ve
///
/// \remarks
/// Besides the easy and most common situation where a the two matched up CSourcePhrase
/// instances contain just the same word in their m_key members, it also has to handle the
/// possibility that the word being matched might, in the arrOld array, be within a merged
/// CSourcePhrase, or a fixedspace conjoined pseudo-merger; and in arrNew, it may be within
/// a fixedspace conjoined pseudo-merger. When mergers are involved, the merger will be
/// only in arrOld, and there won't be a merger equivalent to it in arrNew, because
/// parsing in the edited source data will have produced a sequence of CSourcePhrase
/// instances each with a single word of the old source text's merged phrase - providing
/// of course that the user did not edit the phrase at that location in the source text
/// before importing the edited source text back into Adapt It. Similarly, for a
/// fixedspace pseudo-merger in arrOld, the user may have (a) left the fixed space
/// conjoined source text pair of words unedited, in which case arrNew will have a single
/// CSourcePhrase instance containing the conjoining, available for a match; or (b)
/// removed the ~ fixedspace marker but left the words either side unchanged, in which
/// case arrNew will have a sequence of two CSourcePhrase instances each with one word of
/// the former conjoining - and we'll want to match that as a successful matchup; or (c)
/// same as (a) or (b) but one or both of the words is spelled differently - in which case
/// we'd want to treat that as a matchup failure. When a matchup is made, the number
/// of CSourcePhrase instances have to be returned as well, so that the counts for these
/// (ie. a count for arrOld's matchup, and a count for arrNew's matchup) can be passed on
/// to the other functions which need to know them.
/// Note: the source text ... ellipsis (three dots) of a placeholder will never be a
/// candidate word for matching, because such source text can only appear in arrOld, and
/// hence the unique words which are in both arrOld and arrNew subspans can never have ...
/// in common.
/// Because the matchup may involve mergers, etc, we have to return the matched
/// CSourcePhrase instances as a range from oldMatchedStart to oldMatchedEnd, inclusive,
/// and newMatchedStart to newMatchedEnd, inclusive. In addition, since we only pass in a
/// single word for searching, the word passed in may match any of the components of a
/// merger, or a fixedspace conjoining - and a matchup requires that all such larger
/// "units" match in their entirety -- so it's possible that the matched location will
/// still fail to return a matchup. In such a circumstance, we'll be passing back
/// wxNOT_FOUND values, but we don't want those to be interpretted as meaning "don't look
/// further ahead in arrOld or arrNew for later matchup possibilities using the same word
/// for the search". So, to facilitate this, we also pass back to the caller oldLastIndex
/// and newLastIndex values - they'll be wxNOT_FOUND if the internal FindNextInArray()
/// call returns wxNOT_FOUND, in which case we don't look further; but if the call returns
/// a 0 or positive index, the we pass back the index found - even if subsequent code
/// determines the matchup isn't valid. The caller will need those non-negative
/// oldLastIndex and newLastIndex values in order to increment them by one and use the
/// resulting values as the new oldStartAt and newStartAt values for a new call of
/// GetNextMatchup()
bool GetNextMatchup(wxString& word, SPArray& arrOld, SPArray& arrNew, int oldStartAt,
		int newStartAt, int oldStartFrom, int oldEndAt, int newStartFrom, int newEndAt,
		int& oldMatchedStart, int& oldMatchedEnd, int & newMatchedStart,
		int& newMatchedEnd, int& oldLastIndex, int& newLastIndex)
{
	oldStartAt = oldStartAt; newStartAt = newStartAt; // prevent two compiler warnings

    // default initializations; initialize the oldLastIndex and newLastIndex ones to the
    // starting from indices, so that only a FindNextInArray() failure to find results in
    // a wxNOT_FOUND value for these
	oldLastIndex = oldStartFrom;
	newLastIndex = newStartFrom;
	oldMatchedStart = wxNOT_FOUND;
	oldMatchedEnd = wxNOT_FOUND;
	newMatchedStart = wxNOT_FOUND;
	newMatchedEnd = wxNOT_FOUND;
	CSourcePhrase* pOldSrcPhrase = NULL;
	CSourcePhrase* pNewSrcPhrase = NULL;
	int oldMatchIndex;
	int newMatchIndex;
	wxString phrase; // used for a search in arrOld
	wxString phrase2; // used for a search in arrNew
	bool bOldIsFixedspaceConjoined = FALSE;
	bool bNewIsFixedspaceConjoined = FALSE;

	// test the sanity conditions for the bounds , return FALSE if not in bounds; the
	// sanity conditions for arrOld would be that oldStartFrom is is <= oldEndAt, and that
	// oldEndAt is < arrOld's count value; similar ones apply for arrNew
	if (oldStartFrom > oldEndAt || newStartFrom > newEndAt)
	{
		oldLastIndex = wxNOT_FOUND; // GetAllCommonSpansFromOneParentSpan() needs this
		return FALSE;
	}
	if (oldEndAt >= (int)arrOld.GetCount() || newEndAt >= (int)arrNew.GetCount())
	{
		newLastIndex = wxNOT_FOUND; // GetAllCommonSpansFromOneParentSpan() needs this
		return FALSE;
	}

	// get the next occurrence of word in arrOld, return wxNOT_FOUND in oldLastIndex
	// if not found
	oldMatchIndex = FindNextInArray(word, arrOld, oldStartFrom, oldEndAt, phrase);
	if (oldMatchIndex == wxNOT_FOUND)
	{
		oldLastIndex = wxNOT_FOUND; // & newLastIndex is still set to newStartFrom
		return FALSE;
	}
	oldLastIndex = oldMatchIndex; // ensure oldLastIndex value advances for the caller
	// try the equivalent Find... in arrNew
	newMatchIndex = FindNextInArray(word, arrNew, newStartFrom, newEndAt, phrase2);
	if (newMatchIndex == wxNOT_FOUND)
	{
		newLastIndex = wxNOT_FOUND; // & oldLastIndex is still set +ve
		return FALSE;
	}
	newLastIndex = newMatchIndex; // ensure newLastIndex value advances for the caller

#if defined( myLogDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("GetNextMatchup(): word = %s , oldStartFrom = %d , newStartFrom = %d, oldMatchIndex = %d, newMatchIndex = %d "),
		word.c_str(), oldStartFrom, newStartFrom, oldMatchIndex, newMatchIndex);
#endif

	// get the CSourcePhrase's m_key value - what we do depends on whether it is a single
	// word, a merger, or a fixed space conjoining (placeholder ... ellipses are never
	// matched)
	pOldSrcPhrase = arrOld.Item(oldMatchIndex);
	if (pOldSrcPhrase->m_nSrcWords == 1)
	{
		// old instance's key is a single word...
		// & we have a potential match location in newArr, check it out in more detail
		pNewSrcPhrase = arrNew.Item(newMatchIndex);
		if (pNewSrcPhrase->m_nSrcWords == 1)
		{
			// the keys match, we don't need a further test
			oldMatchedStart = oldMatchIndex;
			oldMatchedEnd = oldMatchIndex;
			newMatchedStart = newMatchIndex;
			newMatchedEnd = newMatchIndex;

			oldLastIndex = oldMatchIndex;
			newLastIndex = newMatchIndex;
			return TRUE;
		}
		else
		{
			// it must be a fixedspace pseudo-merger CSourcePhrase in arrNew
			int numWords = pNewSrcPhrase->m_nSrcWords;
			numWords = numWords; // avoid compiler warning in the Release build
			wxASSERT(numWords == 2);
 			bNewIsFixedspaceConjoined = IsFixedSpaceSymbolWithin(phrase2);
			if (bNewIsFixedspaceConjoined)
			{
                // we require both words in arrOld and arrNew to match, and within the
                // bounds defined by newStartFrom and newEndAt - if not, treat it as
                // unmatched
				wxString word1;
				wxString word2;
				SPList::Node* pos = pNewSrcPhrase->m_pSavedWords->GetFirst();
				wxASSERT(pos != NULL);
				CSourcePhrase* pSP1 = pos->GetData();
				pos = pos->GetNext();
				CSourcePhrase* pSP2 = pos->GetData();
				word1 = pSP1->m_key;
				word2 = pSP2->m_key;
				if (word == word1)
				{
					// the match is with the first word of the pseudo-merger, so require
					// that the second word also matches in the next location in arrOld
					// and that that location is <= oldEndAt
					int nextOldIndex = oldMatchIndex + 1;
					if (nextOldIndex <= oldEndAt)
					{
						CSourcePhrase* pNextSP = arrOld.Item(nextOldIndex);
						wxASSERT(pNextSP != NULL);
						if (pNextSP->m_key == word2)
						{
							oldMatchedStart = oldMatchIndex;
							oldMatchedEnd = nextOldIndex;
							newMatchedStart = newMatchIndex;
							newMatchedEnd = newMatchIndex;
							// set the next two so that the caller can search for another span
							oldLastIndex = oldMatchedEnd;
							newLastIndex = newMatchedEnd;
							return TRUE;
						}
					}
					else
					{
						// right bound is exceeded (a later recurse should pick it up
						// even though this one didn't)
						oldLastIndex = oldMatchIndex;
						newLastIndex = newMatchIndex;
						return FALSE;
					}
				}
				else
				{
					// the word must have matched the second word of the pseudo-merger, so
					// require that the first word, word1, matches the preceding word in
					// arrOld, and that its location in arrOld is >= oldStartFrom
					int prevOldIndex = oldMatchIndex - 1;
					if (prevOldIndex >= oldStartFrom)
					{
						CSourcePhrase* pPrevSP = arrOld.Item(prevOldIndex);
						wxASSERT(pPrevSP != NULL);
						if (pPrevSP->m_key == word1)
						{
							oldMatchedStart = prevOldIndex;
							oldMatchedEnd = oldMatchIndex;
							newMatchedStart = newMatchIndex;
							newMatchedEnd = newMatchIndex;
							// set the next two so that the caller can search for another span
							oldLastIndex = oldMatchedEnd;
							newLastIndex = newMatchedEnd;
							return TRUE;
						}
					}
					else
					{
						// left bound is violated
						oldLastIndex = oldMatchIndex;
						newLastIndex = newMatchIndex;
						return FALSE;
					}
				}
			}
			else
			{
				// ? it's not conjoined - how can there be a merger in arrNew except that
				// way? (we don't expect control ever to enter here, so just return FALSE)
				oldLastIndex = oldMatchIndex;
				newLastIndex = newMatchIndex;
				return FALSE;
			}
		}
	} // end of TRUE block for test: if (pOldSrcPhrase->m_nSrcWords == 1)
	else
	{
		// it's a merger, or a fixedspace conjoined pseudo-merger
		bOldIsFixedspaceConjoined = IsFixedSpaceSymbolWithin(phrase);
		if (bOldIsFixedspaceConjoined)
		{
			// it's fixedspace conjoined pseudo-merger - for this to succeed in the
			// matchup attempt, both words must be within the the bounds oldStartFrom to
			// oldEndAt inclusive; and their must be either the identical fixedspace
			// conjoining at the arrNew matching location, or at that location the two words
			// must be on successive CSourcePhrase instances which lie within the
			// boundaries newStartFrom to newEndAt, inclusive.

			// test for identical fixedspace pseudo-mergers first
			pNewSrcPhrase = arrNew.Item(newMatchIndex);
			if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			{
				// they match, so this is a successful matchup
				oldMatchedStart = oldMatchIndex;
				oldMatchedEnd = oldMatchIndex;
				newMatchedStart = newMatchIndex;
				newMatchedEnd = newMatchIndex;
				// set next two so that caller can search for other spans further on
				oldLastIndex = oldMatchedEnd;
				newLastIndex = newMatchedEnd;
				return TRUE;
			}
			// there was no matching fixedspace pseudo-merger, so look for two ordinary
			// CSourcePhrase instances with matching m_key values for the two words in the
			// arrOld's fixedspace pseudo-merger
			wxString word1;
			wxString word2;
			SPList::Node* pos = pOldSrcPhrase->m_pSavedWords->GetFirst();
			wxASSERT(pos != NULL);
			CSourcePhrase* pSP1 = pos->GetData();
			pos = pos->GetNext();
			CSourcePhrase* pSP2 = pos->GetData();
			word1 = pSP1->m_key;
			word2 = pSP2->m_key;
			// test for a match of word1 with what is at the match location in arrNew
			if (word1 == pNewSrcPhrase->m_key)
			{
				// the first word matches, test now for a match of word2 with the key
				// within the next CSourcePhrase instance in arrNew
				CSourcePhrase* pNextNewSrcPhrase = arrNew.Item(newMatchIndex + 1);
				if (word2 == pNextNewSrcPhrase->m_key)
				{
					// both match, so treat this as a successful matchup
					oldMatchedStart = oldMatchIndex;
					oldMatchedEnd = oldMatchIndex;
					newMatchedStart = newMatchIndex;
					newMatchedEnd = newMatchIndex + 1;
					// set next two so that caller can search for other spans further on
					oldLastIndex = oldMatchedEnd;
					newLastIndex = newMatchedEnd;
					return TRUE;
				}
				else
				{
					// second word does not match, so this matchup attempt fails
					oldLastIndex = oldMatchIndex;
					newLastIndex = newMatchIndex;
					return FALSE;
				}
			}
			else
			{
				// the first word doesn't match, so this matchup attempt fails
				oldLastIndex = oldMatchIndex;
				newLastIndex = newMatchIndex;
				return FALSE;
			}
		} // end of TRUE block for test: if (bOldIsFixedspaceConjoined)
		else
		{
			// it's an ordinary merger - but we don't know which word in the merger we
			// matched; so we have to work out which word it is, and then look for the
			// word in arrNew at newStartAt and onwards, and if matched, see if the
			// adjoining words either side match those in the arrOld's merger & if they do
			// then we have a successful matchup
			wxString thePhrase = pOldSrcPhrase->m_key;
			int numWords = pOldSrcPhrase->m_nSrcWords;
			wxArrayString arrKeys; // store the individual word tokens here
			wxString delimiters = _T(' ');
			// in the next call, FALSE is bStoreEmptyStringsToo value
			long howmanywords = SmartTokenize(delimiters, thePhrase, arrKeys, FALSE);
			howmanywords = howmanywords; // avoid compiler warning in release version
			wxASSERT((int)howmanywords == numWords);
			wxASSERT(howmanywords > 1L);
			// find the index for the word which matches the one passed in
			int i;
			wxString aWord;
			for (i = 0; i < numWords; i++)
			{
				aWord = arrKeys.Item(i);
				if (aWord == word)
				{
					break;
				}
			}
			wxASSERT(i < numWords); // must match one of them!
			int numBefore = i;
			int numAfter = numWords - i - 1;

			// now try for a match in arrNew, the match must do the following:
			// (a) FindNextInArray() must return a positive index <= newEndAt
			// (b) the m_key value for that location's CSourcePhrase must equal the
			// string in the passed-in word parameter
			// (c) the numBefore amount of preceding CSourcePhrase instances in arrNew
			// must be all single-word CSourceInstances with m_key values matching those
			// in arrKeys, in reverse order, starting from the index one less than
			// newMatchIndex's value
			// (d) the numAfter amount of following CSourcePhrase instances in arrNew
			// must be all single-word CSourcePhrase instances with m_key values
			// matching those in arrKeys, in increasing order, starting from the index one
			// more than newMatchIndex's value

			// Looking for a matchup there is a potential (rare) problem. Suppose the user
			// edited the source text that was imported at the matchup location, and the
			// edit was to use ~ (USFM fixedspace) to conjoin two words which, in arrOld
			// are part of the merger now being compared. What should we do? If would be
			// inappropriate to honour the conjoining, since mergers are very important
			// for transfer of meaning accurately, so we will honour the merger and
			// abandon the conjoining.
            // The first algorithm is to move leftwards, getting each word, and comparing
            // with what is in the corresponding cell of arrKeys
			int leftWordCount = 0;
			int leftIndex = newMatchIndex;
			leftIndex--;
			while (leftIndex >= newStartFrom && leftWordCount < numBefore)
			{
				CSourcePhrase* pSrcPhrase = arrNew.Item(leftIndex);
				wxString aWord = pSrcPhrase->m_key;
				bool bHasFixedspaceMkr = IsFixedSpaceSymbolWithin(aWord);
				if (bHasFixedspaceMkr)
				{
					int offset = aWord.Find(_T('~'));
					wxASSERT(offset != wxNOT_FOUND);
					wxString word1 = aWord.Left(offset);
					offset++;
					wxString word2 = aWord.Mid(offset);
					// now test them for matches with the words in arrKeys
					int anIndex = numBefore - leftWordCount - 1;
					if (arrKeys.Item(anIndex) == word2)
					{
						// word2 matches with the arrOld's one in the merger
						leftWordCount++; // count this word
						anIndex--;
						if (leftWordCount >= numBefore)
						{
							// word1 is a word too many, so no matchup - we require the
							// whole conjoining to be in commonSpan, if not, exclude it
							newLastIndex = newMatchIndex;
							oldLastIndex = oldMatchIndex;
							return FALSE;
						}
						// we can test word1 now
						if (arrKeys.Item(anIndex) == word1)
						{
							// word1 matches with the arrOld's one in the merger too, so
							// the whole conjoining has matched - hence so far we are
							// successful
							leftWordCount++; // count this word
							leftIndex--; // prepare for next iteration
						}
					}
					else
					{
						// the words don't match, so no matchup
						newLastIndex = newMatchIndex;
						oldLastIndex = oldMatchIndex;
						return FALSE;
					}
				} // end of TRUE block for test: if (bHasFixedspaceMkr)
				else
				{
					// the m_key value is just a single source text word
					int anIndex = numBefore - leftWordCount - 1;
					if (arrKeys.Item(anIndex) == aWord)
					{
						// the word matches with the corresponding one in arrKeys, so all
						// is okay so far
						leftWordCount++; // count the word
						leftIndex--; // prepare for next iteration
					}
					else
					{
						// word match failure, so no matchup
						newLastIndex = newMatchIndex;
						oldLastIndex = oldMatchIndex;
						return FALSE;
					}
				} // end of else block for test: if (bHasFixedspaceMkr), ie. it's a single word

			} // end of while loop: while (leftIndex >= newStartFrom && leftWordCount < numBefore)

			// The loop may exit because all words to the left were matched (in which case
			// leftWordCount will equal numBefore), or because leftIndex became less than
			// the newStartFrom value -- and in the latter situation it may or may not be the
			// case that all words were matched, so we have to test
			if (leftWordCount < numBefore)
			{
				// unsuccessful matchup attempt
				newLastIndex = newMatchIndex;
				oldLastIndex = oldMatchIndex;
				return FALSE;
			}
			// the words to the left were matched successfully, and leftIndex will be one
			// less than the index at which matching words commences so set
			// oldMatchedStart and newMatchedStart while we know what the values should be
			oldMatchedStart = oldMatchIndex;
			newMatchedStart = ++leftIndex;
			// *** NOTE *** the above pair need to be reset to wxNOT_FOUND if the attempt
			// to match rightwards for the rest of the merged words should fail

            // The second algorithm is to move rightwards, getting each word, and comparing
            // with what is in the corresponding cell of arrKeys
			int rightWordCount = 0;
			int rightIndex = newMatchIndex;
			rightIndex++;
			while (rightIndex <= newEndAt && rightWordCount < numAfter)
			{
				CSourcePhrase* pSrcPhrase = arrNew.Item(rightIndex);
				wxString aWord = pSrcPhrase->m_key;
				bool bHasFixedspaceMkr = IsFixedSpaceSymbolWithin(aWord);
				if (bHasFixedspaceMkr)
				{
					int offset = aWord.Find(_T('~'));
					wxASSERT(offset != wxNOT_FOUND);
					wxString word1 = aWord.Left(offset);
					offset++;
					wxString word2 = aWord.Mid(offset);
					// now test them for matches with the words in arrKeys
					int anIndex = rightWordCount + 1;
					if (arrKeys.Item(anIndex) == word1)
					{
						// word1 matches with the arrOld's one in the merger
						rightWordCount++; // count this word
						anIndex++;
						if (rightWordCount >= numAfter)
						{
							// word2 is a word too many, so no matchup - we require the
							// whole conjoining to be in commonSpan, if not, exclude it
							newLastIndex = newMatchIndex;
							oldLastIndex = oldMatchIndex;
							oldMatchedStart = wxNOT_FOUND;
							newMatchedStart = wxNOT_FOUND;
							return FALSE;
						}
						// we can test word2 now
						if (arrKeys.Item(anIndex) == word2)
						{
							// word2 matches with the arrOld's one in the merger too, so
							// the whole conjoining has matched - hence so far we are
							// successful
							rightWordCount++; // count this word
							rightIndex++; // prepare for next iteration
						}
					}
					else
					{
						// the words don't match, so no matchup
						newLastIndex = newMatchIndex;
						oldLastIndex = oldMatchIndex;
						oldMatchedStart = wxNOT_FOUND;
						newMatchedStart = wxNOT_FOUND;
						return FALSE;
					}
				} // end of TRUE block for test: if (bHasFixedspaceMkr)
				else
				{
					// the m_key value is just a single source text word
					int anIndex = rightWordCount + 1;
					if (arrKeys.Item(anIndex) == aWord)
					{
						// the word matches with the corresponding one in arrKeys, so all
						// is okay so far
						rightWordCount++; // count the word
						rightIndex++; // prepare for next iteration
					}
					else
					{
						// word match failure, so no matchup
						newLastIndex = newMatchIndex;
						oldLastIndex = oldMatchIndex;
						oldMatchedStart = wxNOT_FOUND;
						newMatchedStart = wxNOT_FOUND;
						return FALSE;
					}
				} // end of else block for test: if (bHasFixedspaceMkr), ie. it's a single word

			} // end of while loop: while (rightIndex <= newEndAt && rightWordCount < numAfter)

			// The loop may exit because all words to the right were matched (in which case
			// rightWordCount will equal numAfter), or because rightIndex became greater than
			// the newEndAt value -- and in the latter situation it may or may not be the
			// case that all words were matched, so we have to test
			if (rightWordCount < numAfter)
			{
				// unsuccessful matchup attempt
				newLastIndex = newMatchIndex;
				oldLastIndex = oldMatchIndex;
				oldMatchedStart = wxNOT_FOUND;
				newMatchedStart = wxNOT_FOUND;
				return FALSE;
			}
			// the words to the right were matched successfully, and rightIndex will be one
			// more than the index at which matching words ended so set
			// oldMatchedEnd and newMatchedEnd while we know what the values should be
			oldMatchedEnd = oldMatchIndex;
			newMatchedEnd = --rightIndex;
			// set the next two so that further spans can be searched for in the caller
			oldLastIndex = oldMatchedEnd;
			newLastIndex = newMatchedEnd;
			// *** NOTE *** the above pair need to be reset to wxNOT_FOUND if the attempt
			// to match rightwards for the rest of the merged words should fail
		} // end of else block for test: if (bOldIsFixedspaceConjoined), i.e. it's an ordinary merger

	} // end of else block for test: if (pOldSrcPhrase->m_nSrcWords == 1)
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                     TRUE for a successful matchup and widening, FALSE
///                             otherwise, that is, FALSE means that this tentative
///                             Subspan instance should be abandoned (ie. deleted from the
///                             heap)
/// \param  word             -> the word being searched for at or after the starting index
///                             locations in arrOld and arrNew
/// \param  arrOld           -> array of old CSourcePhrase instances (may have mergers,
///                             placeholders, retranslation, fixedspace conjoinings as
///                             well as minimal CSourcePhrase instances)
/// \param  arrNew           -> array of new CSourcePhrase instances (will only be minimal
///                             CSourcePhrase instances, but could also contain fixedspace
///                             conjoined instances too)
/// \param  oldStartAt       -> index at which span is left-bounded (right-bounded for RTL
///                             languages)
/// \param  newStartAt       -> index at which span is right-bounded (left-bounded for RTL
///                             languages)
/// \param  oldStartFrom     -> index from which to start searching in arrOld
/// \param  oldEndAt         -> index in arrOld at which last search is to be tried
/// \param  newStartFrom     -> index from which to start searching in arrNew
/// \param  newEndAt         -> index in arrNew at which last search is to be tried
/// \param  oldMatchedStart  <- index of initial matched CSourcePhrase in arrOld,
///                             wxNOT_FOUND if no match
/// \param  oldMatchedEnd    <- index of last matched CSourcePhrase in arrOld,
///                             wxNOT_FOUND if no match
/// \param  newMatchedStart  <- index of initial matched CSourcePhrase in arrNew,
///                             wxNOT_FOUND if no match
/// \param  newMatchedEnd    <- index of last matched CSourcePhrase in arrNew,
///                             wxNOT_FOUND if no match
/// \param  oldLastIndex     <- index of CSourcePhrase in which the word matched in arrOld,
///                             (but it may not have resulted in a matchup) but we need it
///                             in the caller so that we can retry the call starting from
///                             the index following it, provided that index is in the span
/// \param  newLastIndex     <- index of CSourcePhrase in which the word matched in arrNew,
///                             (but it may not have resulted in a matchup) but we need it
///                             in the caller so that we can retry the call starting from
///                             the index following it, provided that index is in the span
/// \param  bClosedEnd       -> TRUE if the right boundary index values are to be strictly
///                             obeyed when attempting to widen to the right, FALSE if the
///                             boundaries are to be ignored when rightwards widening,
///                             allowing the rightwards widening to potentially go as far
///                             as the limits of both arrOld and arrNew
/// \param  pCommonSpans     <- ptr to array of already delineated pSubspan instances for the
///                             subspan being analysed. If the matchup location falls within
///                             any of these, we don't bother to widen, since the same
///                             span would result; and just pass back FALSE; but if the
///                             span matchup doesn't lie within any of them, we add the
///                             new pSubspan passed in to this array. (We do this check to
///                             save time.)
/// \param  pWidthsArray     <- Stores, in sync with pCommonSpans Subspan instance, the width
///                             of that Subspan's subspan in arrOld plus the width of the
///                             matching subspan in arrNew as a single integer value. The
///                             caller will later use this array to quickly find which at
///                             which index the composite span width value is largest -
///                             and then look up that Subspan instance in pCommonSpan in
///                             order to use that in the tuple as the 'longest' one.
/// \remarks
/// For how the parameters are used, see the description of the GetNextMatchup() function.
/// The GetNextCommonSpan() function adds an extra parameter to the former's ones, the
/// pSubspan parameter which is to be defined in scope (ie. starting and ending index
/// values in both arrOld and arrNew) for the CSourcePhrase instances which are in this
/// tentative new commonSpan. We get a matchup location using GetNextCommonSpan(), and then
/// call WidenLeftwardsOnce() in a loop until we can't widen further, than do similarly
/// rightwards using WidenRightwardsOnce(), thereby (providing a failure condition has not
/// been encountered yet by any of those 3 functions) defining the boundaries for the
/// in-common span pair.
/// The parameters are used to set the relevant indices in a newly created (on the heap)
/// pSubspan instance, provided all went well, and the new instance is stored in the
/// pCommonSpans array so that the caller will have access to it, and a composite count of
/// the length in pWidthsArray, and TRUE is returned; failure to get a properly defined
/// commonSpan causes FALSE to be returned, and in that circumstance no Subspan instance is
/// created nor stored etc. A valid pSubspan must be checked here first to make sure it
/// doesn't define an already defined subspan, and provided that is so, the new span is
/// added to the above-mentioned array which is of type wxArrayPtrVoid (later, a higher
/// level function will examine all such stored subspans to determine which to keep - the
/// "widest", the rest would get abandoned and their instances removed from the heap).
///
/// Note 1: GetNextMatchup() doesn't necessarily return indices for just one CSourcePhrase
/// in arrOld, and one in arrNew, for a matchup. A matchup can involve mergers, for
/// instance, or fixedspace conjoined pairs, and so what is returned is, for a valid
/// matchup, the starting and ending index values in arrOld and the same in arrNew. Only
/// when the matchup involves simple one-word CSourcePhrase instances are the starting and
/// ending indices the same for the matchup.
///
/// Note 2: GetNextMatchup() also initializes to wxNOT_FOUND the following parameters:
/// oldMatchedStart, oldMatchedEnd, newMatchedStart, newMatchedEnd, oldLastIndex, and
/// newLastIndex; also oldCount and newCount are internally initialized to 0 at each call.
///
/// Note 3: oldLastIndex and newLastIndex are updated within the GetNextMatchup() call,
/// and will be returned with new (larger) values if the internal calls within it to
/// FindNextInArray() return non-negative values, but that does not mean that the matchup
/// has succeeded - the return value must be also checked for TRUE returned, and when
/// FALSE is returned, oldMatchedStart, etc, will have the value wxNOT_FOUND, indicating a
/// matchup failure, even though the word passed in resulted in positive finds in both
/// arrOld and arrNew. For instance, the word passed in may be successfully found within a
/// merger within arrOld, and also at some location in arrNew, but the ensuing attempt to
/// determine that all the merger's words have matching equivalents in arrNew at the
/// location found in arrNew, fails.
///
/// Note 4: Don't get confused with the constraints on index values for the intial matchup
/// - which has to return indices with the range oldStartFrom to oldEndAt in arrOld, and
/// newStartFrom to newEndAt in arrNew, because a matchup is being sought at index values
/// at or beyond oldStartFrom in arrOld and newStartFrom in arrNew - up to the bounding
/// values set for each subarray. However the subsequent widening-to-left attempt is
/// allowed to go to index values less than oldStartAt in arrOld, and less than newStartAt
/// in arrNew, because we want our widening to go as wide as there are in-common
/// CSourcePhrase instances within the parent subspan.
bool GetNextCommonSpan(wxString& word, SPArray& arrOld, SPArray& arrNew, int oldStartAt,
			int newStartAt, int oldStartFrom, int oldEndAt, int newStartFrom, int newEndAt,
			int& oldMatchedStart, int& oldMatchedEnd, int& newMatchedStart,
			int& newMatchedEnd, int& oldLastIndex, int& newLastIndex, bool bClosedEnd,
			wxArrayPtrVoid* pCommonSpans, wxArrayInt* pWidthsArray)
{
	if (GetNextMatchup(word, arrOld, arrNew, oldStartAt, newStartAt, oldStartFrom,
						oldEndAt, newStartFrom, newEndAt, oldMatchedStart, oldMatchedEnd,
						newMatchedStart, newMatchedEnd, oldLastIndex, newLastIndex))
	{
		Subspan* pSubspan = NULL; // create on heap only if we delineate a Subspan successfully
		// we obtained a valid matchup within the allowed index ranges; first check if it
		// lies within an already-delineated Subspan instance - if so, abandon this
		// attempt and return FALSE; if not, add the new pSubspan instance to this array
		// later below
        bool bAlreadyExists = IsMatchupWithinAnyStoredSpanPair(oldMatchedStart, oldMatchedEnd,
											newMatchedStart, newMatchedEnd, pCommonSpans);
        if (bAlreadyExists)
		{
			// reset the matchup indices to -1, so that the caller will get the right
			// message from the FALSE which we return here
			oldMatchedStart = wxNOT_FOUND;
			oldMatchedEnd = wxNOT_FOUND;
			newMatchedStart = wxNOT_FOUND;
			newMatchedEnd = wxNOT_FOUND;
			return FALSE;
		}
        // now try to widen the matchup to either side, starting with a left widening loop;
        // a return value of FALSE from a widening attempt means "the boundary for widening
        // in that direction has been reached"

		// the next two track the left boundaries within arrOld and arrNew of the commonSpan
		// being delimited by the widening loop below
		int oldLeftBdryIndex = oldMatchedStart;
		int newLeftBdryIndex = newMatchedStart;
		// suppressors to prevent bounds errors in the loops below
		bool bSuppressLeftWidening = FALSE;
		bool bSuppressRightWidening = FALSE;

        // The next two track the jumping off locations in arrOld and arrNew, for the loop
		int oldJumpFromIndex = oldMatchedStart - 1; // might be out of bounds
		int newJumpFromIndex = newMatchedStart - 1; // might be out of bounds
		// suppress if out of bounds
		if (oldJumpFromIndex < oldStartAt || newJumpFromIndex < newStartAt)
			bSuppressLeftWidening = TRUE;

		// the next two track the counts of how many CSourcePhrase instances were
		// traversed in arrOld and arrNew for a single call of WidenLeftwardsOnce() - each
		// is initialized to zero at the start of WidenLeftwardsOnce()
		int oldLeftCount = 0;
		int newLeftCount = 0;
		bool bOK = TRUE;
		if (!bSuppressLeftWidening)
		{
            // only widen leftwards if the current in-common left boundary is later
            // than the span's left boundary
			while (oldLeftBdryIndex > oldStartAt && newLeftBdryIndex > newStartAt)
			{
				// attempt another leftwards "step"
				bOK = WidenLeftwardsOnce(arrOld, arrNew, oldStartAt, oldEndAt, newStartAt,
								newEndAt, oldJumpFromIndex, newJumpFromIndex, oldLeftCount,
								newLeftCount);
				if (bOK)
				{
					// calculate the new values for the left bounds
					oldLeftBdryIndex -= oldLeftCount;
					newLeftBdryIndex -= newLeftCount;
					// prepare for the next iteration, get updated jump index values
					oldJumpFromIndex = oldLeftBdryIndex - 1;
					newJumpFromIndex = newLeftBdryIndex - 1;
				}
				else
				{
					// the widening attempt fails, so exit the loop, retaining the boundary
					// indices as they were at the end of the last iteration
					break;
				}
			}
		}
		// do rightwards widening now, using a rightwards widening loop, if bounds permit
        // The next two track the jumping off locations in arrOld and arrNew, for the loop

		// the next two track the right boundaries within arrOld and arrNew of the commonSpan
		// being delimited by the widening loop below
		int oldRightBdryIndex = oldMatchedEnd;
		int newRightBdryIndex = newMatchedEnd;

		// next two may be out of bounds
		oldJumpFromIndex = oldMatchedEnd + 1;
		newJumpFromIndex = newMatchedEnd + 1;
        // check bounds and suppress if out of bounds (also see comments a little further
        // down in the if(!bClosedEnd) test's TRUE block)
		if (bClosedEnd)
		{
			// the passed in oldEndAt and newEndAt values must be strictly obeyed
			if (oldJumpFromIndex > oldEndAt || newJumpFromIndex > newEndAt)
				bSuppressRightWidening = TRUE;
		}
		else
		{
			// the span's end, for widening purposes, is considered to be open, so the
			// suppression test here is more generous
			if (oldJumpFromIndex >= (int)arrOld.GetCount() || newJumpFromIndex > (int)arrNew.GetCount())
				bSuppressRightWidening = TRUE;
		}

		// the next two track the counts of how many CSourcePhrase instances were
		// traversed in arrOld and arrNew for a single call of WidenRightwardsOnce() - each
		// is initialized to zero at the start of WidenRightwardsOnce()-- note, for an
		// open afterSpan, we can widen past the passed in bounding end values. If that is
		// the case, update oldEndAt and newEndAt here to array-end values for arrOld &
		// arrNew
		int oldRightCount = 0;
		int newRightCount = 0;
		// set the right limits for widening, according to whether the end is closed or open
		if (!bClosedEnd)
		{
            // When FALSE, we are free to widen the end bound as far as the end of arrOld,
            // and/or the end of arrNew NOTE: oldEndAt and newEndAt are not passed in as
            // reference variables, so these changed values disappear once
            // GetNextCommonSpan() returns.
            // THESE NEXT TWO LINES ARE THE SHARP END OF WHAT THE bClosedEnd PARAMETER IS
            // ALL ABOUT, IT'S ONLY HERE AND A FEW LINES ABOVE THAT IT DOES ITS JOB OF
            // ENABLING A WIDER SPAN
			oldEndAt = arrOld.GetCount() - 1;
			newEndAt = arrNew.GetCount() - 1;
		}
		if (!bSuppressRightWidening)
		{
            // only widen rightwards if the current in-common right boundary is earlier
            // than the span's right boundary
			while (oldRightBdryIndex < oldEndAt && newRightBdryIndex < newEndAt)
			{
				// attempt another rightwards "step"
				bOK = WidenRightwardsOnce(arrOld, arrNew, oldStartAt, oldEndAt, newStartAt,
								newEndAt, oldJumpFromIndex, newJumpFromIndex, oldRightCount,
								newRightCount);
				if (bOK)
				{
					// calculate the new values for the left bounds
					oldRightBdryIndex += oldRightCount;
					newRightBdryIndex += newRightCount;
					// prepare for the next iteration, get updated jump index values
					oldJumpFromIndex = oldRightBdryIndex + 1;
					newJumpFromIndex = newRightBdryIndex + 1;
				}
				else
				{
					// the widening attempt fails, so exit the loop, retaining the boundary
					// indices as they were at the end of the last iteration
					break;
				}
			}
		}
		// create a new commonSpan type of Subspan instance & set up the index values within
		// pSubspan, and recall that commonSpans are always bClosedEnd = TRUE
		pSubspan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
	countCommonSpans++;
#endif
		InitializeSubspan(pSubspan, commonSpan, oldLeftBdryIndex, oldRightBdryIndex,
							newLeftBdryIndex, newRightBdryIndex, TRUE); // TRUE is bClosedEnd

		// add the span to pCommonSpans, and calculate the composite width value and add
		// it to pWidthsArray
		pCommonSpans->Add((void*)pSubspan);

		// calculate and store the composite spans width
		int oldWidth = oldRightBdryIndex - oldLeftBdryIndex + 1;
		int newWidth = newRightBdryIndex - newLeftBdryIndex + 1;
		pWidthsArray->Add(oldWidth + newWidth);

		return TRUE; // indicate to caller that we succeeded
	} // end of TRUE block for test:  	if (GetNextMatchup(word, ...other params omitted...))
	else
	{
		// getting a valid matchup with GetNextMatchup() failed ; there are two possibilities...
		// (1) oldMatchedStart, oldMatchedEnd, newMatchedStart & newMatchedEnd are each
		// wxNOT_FOUND, but the internal Find...() calls succeeded, so that oldLastIndex
		// and newLastIndex are +ve  which means that the word can, in the caller, be
		// searched for again at a different kickoff location (one may be held constant
		// while the other is advanced in an inner loop). OR
		// (2) the above, except that oldLastIndex and newLastIndex are each wxNOT_FOUND,
		// this means that the word should no longer be searched for, or, that the word
		// should be searched for from a later location in the outer loop, and the inner
		// loop iterates over all possible locations.
		// Either way, we return the values to the caller for it to decide what to do.
		;
	}
	return FALSE;
}

// Test if the merged's CSourcePhrase at oldLoc has a m_key which matches the string
// formed by taking the appropriate number of m_key values in arrNew starting at
// newFirstLoc. (Since newArray is a just-tokenized array of CSourcePhrase instances,
// there can be no mergers in it as yet, since the user hasn't had a chance to even see it
// yet)
bool IsMergerAMatch(SPArray& arrOld, SPArray& arrNew, int oldLoc, int newFirstLoc)
{
	wxString oldPhrase = arrOld.Item(oldLoc)->m_key; // what we want to check for a match in arrNew
	int numWords = arrOld.Item(oldLoc)->m_nSrcWords;
	// first check there are enough words to enable the check
	int count = arrNew.GetCount();
	if (newFirstLoc > count - numWords)
	{
		// not enough words, so there can't possibly be a match
		return FALSE;
	}
	wxString newPhrase;
	int index;
	int newIndex = newFirstLoc;
	for (index = 0; index < numWords; index++)
	{
		CSourcePhrase* pSrcPhrase = arrNew.Item(newIndex + index);
		newPhrase += pSrcPhrase->m_key;
		newPhrase += _T(' ');
	}
	newPhrase.Trim(); // chop off final space
	return oldPhrase == newPhrase;
}

// the next is an overload for the above, passing in a Subspan pointer with it's
// oldStartPos and newStartPos members already set correctly
void SetEndIndices(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, int limit)
{
	if (pSubspan->bClosedEnd)
	{
		// use oldEndAt and newEndAt as passed in, they must not be changed; it shouldn't
		// be called when bClosedEnd is TRUE, but if it is, this ensures no damage is done
		return;
	}
	else
	{
        // it's an open ended span (only the rightmost afterSpan at any recursion level
        // qualifies for being open ended) so set the nominal oldEndAt and newEndAt values
        // - these are what are used for the determination of the set of unique in-common
        // words, but the repeated call of WidenRightwardsOnce(), in the
        // GetNextCommonSpan() function, which gets a right boundary for the in-common
        // CSourcePhrase instances, will ignore these limits - and potentially the iteration
        // loop may result in matchups going as far as the ends of arrOld and/or arrNew
		if (limit == -1)
		{
			// no limitation is wanted ... the whole extent of the CSourcePhrase instances
			// should be used - this requires we set the end index to the max index for each
			// array (NOTE: this would bog processing down if there are many CSourcePhrase
			// instances, such as more than a verse or two's worth, so only use limit = -1
			// when processing of a verse or at most a few verses is being done.)
			pSubspan->oldEndPos = arrOld.GetCount() - 1;
			pSubspan->newEndPos = arrNew.GetCount() - 1;
		}
		else
		{
			// go as far as we can, up to the limit value, or array end, whichever comes first
			pSubspan->oldEndPos =
				wxMin((unsigned int)(pSubspan->oldStartPos + limit), (unsigned int)(arrOld.GetCount())) - 1;
			pSubspan->newEndPos =
				wxMin((unsigned int)(pSubspan->newStartPos + limit), (unsigned int)(arrNew.GetCount())) - 1;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE if all went well, or even if it didn't - provided the
///                         caller can work out how to continue; return FALSE if the error
///                         is of sufficient gravity that the import must be abandoned
///                         (for errors of this gravity, the returned processed location
///                         values will be -1 and -1, so check for these)
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as
///                         well as minimal CSourcePhrase instances)
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be minimal
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too)
/// \param  pSubspan    ->  the Subspan instance, of commonSpan type, which delimits the new
///                         CSourcePhrase instances which are to be the origin of the USFM
///                         and punctuation data which needs to flow to the retained original
///                         instances in arrOld
/// \remarks
/// Because the edited source text import / merging algorithm only looks at the m_key
/// members for its decisions, if the user has left words unchanged but edited the USFM
/// structure or changed the punctuation or its location, those kinds of changes would not
/// be seen. So this function rectifies the situation, making sure that such changes are
/// taken account of properly.
/// What has to be done here is not trivial. Speaking generally, there are 3 kinds of data
/// to be considered: (i) the boolean flags, (ii) markers, (iii) punctuation. The arrOld
/// instances can, of course, have all kinds of user-generated things, such as mergers,
/// manually inserted placeholders, retranslations, stored free translations, stored
/// notes, stored collected back translations. We don't want to lose any of that
/// information unnecessarily. The new source text being imported cannot, by definition,
/// have any of that stuff - it will just have single-word CSourcePhrase instances, and
/// the only variation for that is that conjoined pairs may be in the tokenized new
/// instances too. So, what do we do?
/// First, stored free translations, stored notes, andstored collected back
/// translations, we leave untouched - they are not stored with markers, so leaving them
/// in the arrOld instances ought to be safe. (We can't preclude, however, that an editing
/// change may occur at a word which, when tokenized, replaces an arrOld instance that
/// stores a free translation, or note, or collected back translation - if so, well that's
/// just too bad, the user will have to recreate it after the import, if it matters).
/// Second, punctuation and markers matter - and these can certainly be changed by the
/// user's editing actions, so we must give careful attention to these.
/// Third, flags - many flags pertain to information about things which the incoming new
/// source text can't possibly know about: e.g. whether or not a word or phrase is in the
/// KB, or in the glossing KB, whether a free translation starts at this word, or ends at
/// this word, whether or not the matching instance in arrOld is part of a retranslation,
/// and so forth. Our approach to flag values in the arrOld instances is that we'll simply
/// retain them unchanged unless we can determine from properties within the arrNew
/// CSourcePhrase instances that a flag value needs changing - and we'll then change it.
/// The above are general comments, now to specifics.....
/// *** MERGERS in arrOld instances ***
/// Docv5 encapsulates punctuation placement dialogs within the merge code, so we can't
/// simply take the relevant arrNew instances and remake the merger - it may result in an
/// incomprehensible Place... dialog being shown to the user during Import. So we have to
/// retain the arrOld instance's merger -- and that means we must check punctuation and
/// markers and flow any differences from the arrNew series of instances corresponding to
/// the merger, into the arrOld instance - handling medial markers and punctuation changes
/// too; and having done that, copy m_adaption and m_targetStr from the arrOld's
/// m_pSavedWords instances down to the arrNew's instances, and then make deep copies of
/// the latter and replace the m_pSavedWords instances with those deep copies! Whew!!!
/// *** Normal single-word instances in arrOld ***
/// These, whether they are in a retranslation or not, we just get the corresponding
/// arrNew instances, test for changes to punctuation and markers, and transfer any changes
/// to the arrOld paired instance. Easy peasy, if somewhat tedious to code.
/// *** Manually inserted PLACEHOLDERS in arrOld ***
/// These are a headache, because there could be left or right association which as moved
/// values for punctuation, markers, m_curTextType, m_bFirstOfType, m_bSpecialText, to or
/// from a neighbouring instance. Of course, placeholders are never in the tokenized
/// arrNew material, because the user doesn't get a chance to put any there. So our
/// approach is as follows... Leave the arrOld's placeholder 'as is', but make a clone of
/// it so that we can apply IsLeftAssociatedPlaceholder() and
/// IsRightAssociatedPlaceholder() and save the boolean results of those two tests. Then
/// we 'fix' the preceding and following CSourcePhrase instances in arrOld to have any
/// necessary punctuation and/or markers updated in the arrOld instances. Then we check
/// the aforemented two saved boolean values, and we redo the left or right association
/// data transfers - that is, left association will mean we move data from the end of the
/// preceding CSourcePhrase in arrOld to the end of the placeholder, and clear where it
/// came from; and right association clears initial data (eg preceding punctuation) at the
/// start of the following CSourcePhrase instance in arrOld after first transferring that
/// data to the start of the placeholder. Then we delete the cloned placeholder. And,
/// despite the comments above about free translations and collected back translations
/// being left 'as is', in the case of a manually placed merger in arrOld, if there was
/// transfer of other of those info types due to a right association, we will have to
/// restore the transfer of those info types as well when we do the stuff just above.
/// Likewise for a non-empty m_filteredInfo member under right assocation.
///
/// Remember that all the above is done just in a commonSpan -- one where the arrOld and
/// arrNew data is judged to be "in common". What happens with punctuation and markers in
/// beforeSpan and afterSpan? For those, if there is no arrNew CSourcePhrase instances,
/// any arrOld instances get deleted -- which can remove markers and punctuation etc. But
/// if the arrNew material isn't empty, it replaces the instances from arrOld - which
/// brings with it any new marker structure and punctuation changes resulting from the
/// user's edits. These behaviours, coupled with the commonSpan protocols mentioned above,
/// really do work. For instance, suppose arrOld had a subheading immediately following a
/// \c chapter marker, eg. for chapter 3, and the user edited out
/// the subheading entirely from the source text being imported. Would the chapter number
/// disappear? (Because m_markers in the arrOld instance would be "\c 3 \s ", and that
/// member is carried by the CSourcePhrase which is for the first word of the subheading
/// itself - so if the user outside of Adapt It deletes the whole subheading, the
/// tokenized arrNew instances won't have that subheading first word, and so "\c 3 \s"
/// will not be there!) But the day is saved by the fact that the edited text will have \c
/// 3 and possibly \p and \v 1 followed by the first word of the first verse, and the
/// tokenization of that stuff results in an m_markers which is "\c 3 \p \v 1 " on the new
/// CSourcePhrase which is in arrNew at the start of the verse 1; moreover, and this is
/// the essential point, that word is "in common" with the arrOld's word in the
/// CSourcePhrase instance which is first in the old version of verse 1. And the fact
/// these are in common means that the protocols mentioned above for how
/// DoUSFMandPunctuationAlterations() is to work, will replace the old "\p \v1 " on the
/// arrOld instance's m_markers with the correct value, "\c 3 \p \v 1 ", from the matched
/// instance in arrNew. So, not only is the chapter number not lost, but the correct USFM
/// markup is established.
/// I've commented what happens in this detail so that anyone coming to this later on will
/// not have to recreate all the thinking that went into making this import feature work
/// robustly.
////////////////////////////////////////////////////////////////////////////////////////
bool DoUSFMandPunctuationAlterations(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan)
{
	wxASSERT(pSubspan->spanType == commonSpan);
	int oldSpanStart = pSubspan->oldStartPos;
	int oldSpanEnd = pSubspan->oldEndPos;
	int newSpanStart = pSubspan->newStartPos;
	int newSpanEnd = pSubspan->newEndPos;
	// Our approach is to maintain two 'position' variables, oldIndex (indexing into the
	// arrOld instances in pSubspan) and newIndex (doing the same job, but for arrNew
	// instances), and pass these as paramters to the transfer functions. The transfer
	// functions will also return, in separate parameters of their signatures, the
	// locations in arrOld and arrNew which were the last CSourcePhrase instance
	// processed, and our loop will work out from those values what the kick-off values
	// are for the next iteration. It is expected that oldIndex and newIndex will get out
	// of sync, since arrOld may have mergers, auto-inserted placeholders, manually placed
	// placeholdres, fixedspace conjoined pairs, as well as simple singleton CSourcePhrase
	// instances (i.e. these just store a single word of source text); whereas arrNew will
	// only have singletons, and possibly fixedspace conjoined pairs. Once either oldIndex
	// or newIndex gets past the bounding upper index value for the subspan, we halt
	// processing. We expect that both indices will get to the end together, but if that
	// doesn't happen, the arrOld or arrNew instances unprocessed will remain so.
	int oldIndex = oldSpanStart; // pass in
	int newIndex = newSpanStart; // pass in
	int oldEndedAt = -1; // pass back arrOld's last processed instance's location in this
	int newEndedAt = -1; // pass back arrNew's last processed instance's location in this
	WhatYouAre oldSPtype; // one of the vaues in the enum WhatYouAre for a CSourcePhrase
						  // instance within the arrOld subpan defined by pSubspan
	CSourcePhrase* pOldSP = NULL;
	CSourcePhrase* pNewSP = NULL;
	bool bOK = TRUE;
	while (oldIndex <= oldSpanEnd && newIndex <= newSpanEnd)
	{
		pOldSP = arrOld.Item(oldIndex);
		pNewSP = arrNew.Item(newIndex);
		wxASSERT(pOldSP != NULL);
		wxASSERT(pNewSP != NULL);
		oldSPtype = WhatKindAreYou(pOldSP, pNewSP);
		// the types are, in their defined order:
		//singleton,
		//singleton_in_retrans,
		//singleton_matches_new_conjoined,
		//merger,
		//conjoined,
		//manual_placeholder,
		//placeholder_in_retrans
		switch (oldSPtype)
		{
		case singleton:
			bOK = TransferToSingleton(arrOld, arrNew, oldIndex, newIndex,
									pSubspan, oldEndedAt, newEndedAt);
			break;
		case singleton_in_retrans:
			bOK = TransferToSingleton(arrOld, arrNew, oldIndex, newIndex,
									pSubspan, oldEndedAt, newEndedAt);
			break;
		case merger:
			bOK = TransferPunctsAndMarkersToMerger(arrOld, arrNew, oldIndex,
								newIndex, pSubspan, oldEndedAt, newEndedAt);
			break;
		case singleton_matches_new_conjoined:
		case conjoined:
			bOK = TransferForFixedSpaceConjoinedPair(arrOld, arrNew, oldIndex,
								newIndex, pSubspan, oldEndedAt, newEndedAt);
			break;
		case manual_placeholder:
			bOK = TransferToManualPlaceholder(arrOld, arrNew, oldIndex,
								newIndex, pSubspan, oldEndedAt, newEndedAt);
			break;
		case placeholder_in_retrans:
			bOK = TransferToPlaceholderInRetranslation(arrOld, arrNew, oldIndex,
								newIndex, pSubspan, oldEndedAt, newEndedAt);
			break;
		default: // assume singleton

			break;
		}
		if (!bOK)
		{
			// check if it is a grave error from which recovery is impossible
			if (oldEndedAt == -1 && newEndedAt == -1)
			{
				// yep, can't recover; most probably a bounds error; a non-localizable
				// message for the developers should suffice
				wxString msg;
				msg = msg.Format(_T(
"DoUSFMandPunctuationAlterations() failed badly, probably a bounds error. Import will be cancelled.\n oldIndex %d , old subspan %d to %d ; newIndex %d , new subspan %d to %d"),
				oldIndex, oldSpanStart, oldSpanEnd, newIndex, newSpanStart, newSpanEnd);
				wxMessageBox(msg,_T("Error: Out of bounds?"), wxICON_ERROR | wxOK);
				return FALSE;
			}
			// should be able to recover, work out what to do... and continue iterating;
			// oldSpanEnd and newSpanEnd we'll assume are good, and keep trying
			oldIndex = oldEndedAt + 1;
			newIndex =  newEndedAt + 1;
		}
		else
		{
			// there were no errors or problems, so work out kick-off locations and iterate
			oldIndex = oldEndedAt + 1;
			newIndex =  newEndedAt + 1;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                 FALSE if there is no fixedspace conjoining to be handled, also
///                         returns FALSE if there is an index bounds error;
///                         returns TRUE if there a conjoining (ie. one of situations 1 2
///                         or 3 below)
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as
///                         well as minimal CSourcePhrase instances) We pass in the WHOLE
///                         array, not just a subrange for the commonSpan
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be single-word
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too) We pass in the WHOLE
///                         array, not just a subrange for the commonSpan
/// \param  oldIndex    ->  index in arrOld where we are up to in the scan
/// \param  newIndex    ->  index in arrNew where we are up to in the scan
/// \param  oldDoneToIncluding  <-  index in arrOld of the last CSourcePhrase instance
///                                 processed at the time the function returns; it will
///                                 point at the previous index to oldIndex if FALSE is
///                                 returned
/// \param  newDoneToIncluding  <-  index in arrNew of the last CSourcePhrase instance
///                                 processed at the time the function returns; it will
///                                 point at the previous index to newIndex if FALSE is
///                                 returned
/// \remarks
/// There are three possibilities to consider: (1) the arrOld location has no conjoining at
/// oldIndex, but the arrNew location has one at at newIndex; (2) both arrOld and arrNew
/// have fixed space conjoinings at oldIndex and newIndex respectively; (3) arrOld has a
/// conjoining at oldIndex, but arrNew no longer has one at newIndex.
/// A complication is that we do not permit the number of CSourcePhrase instanes in arrOld
/// and arrNew to be changed within the import merger process for edited source text. So
/// we have to do the best we can with situations (1) and (3): what we'll do is just
/// transfer punctuation and markers to what is in arrOld; for (2) we can instead copy the
/// adaptations in the one in arrOld to the one in arrNew, make a deep copy of the arrNew
/// one after doing that, and then replace the arrOld one with the deep copy. (This only
/// changes what's in arrOld, not the SPList it comes from, but that doesn't matter
/// because we will delete the latter and the deep copy will indeed get into pMergedList
/// which replaces it when the import & merge has finished)
bool TransferForFixedSpaceConjoinedPair(SPArray& arrOld, SPArray& arrNew, int oldIndex,
		int newIndex, Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding)
{
	wxASSERT(pSubspan->spanType == commonSpan);

	// check indices don't violate pSubspan's  bounds
	if (oldIndex < pSubspan->oldStartPos || oldIndex > pSubspan->oldEndPos)
	{
		// out of range in subspan in arrOld
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	if (newIndex < pSubspan->newStartPos || newIndex > pSubspan->newEndPos)
	{
		// out of range in subspan in arrNew
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}

	CAdapt_ItDoc* pDoc = gpApp->GetDocument();
	CSourcePhrase* pOldSP = arrOld.Item(oldIndex);
	CSourcePhrase* pNewSP = arrNew.Item(newIndex);
	// test: is there no  ~ (USFM fixedspace marker) in pOldSP?
	if (!IsFixedSpaceSymbolWithin(pOldSP))
	{
        // it's not present in pOldSP, so either it is situation (1) above, or if pNewSP
        // has no fixed space, there would be nothing to do and FALSE should be returned
		if (IsFixedSpaceSymbolWithin(pNewSP))
		{
			// it's situation (1) as explained above; so do the member fields data
			// transfers, for USFMs and punctuations, if any; unfortunately, the user's
			// addition of a fixedspace to conjoin will be lost - but probably that will
			// affect next to nobody, conjoinings are rare as hens teeth and the
			// probability of this exact scenario happening anyway, is small
			int oldNextIndex = oldIndex + 1;
			CSourcePhrase* pOldNextSP = arrOld.Item(oldNextIndex); // it's gotta be there

			SPList::Node* pos = pNewSP->m_pSavedWords->GetFirst();
			SPList::Node* posLast = pNewSP->m_pSavedWords->GetLast();
			CSourcePhrase* pWordFirstSP = pos->GetData();
			CSourcePhrase* pWordLastSP = posLast->GetData();

			// handle the arrNew's conjoined CSourcePhrase's embedded CSourcePhrases first;
			// FALSE is  bool bClearAfterwards
			TransferPunctsAndMarkersOnly(pWordFirstSP, pOldSP, FALSE);
			TransferPunctsAndMarkersOnly(pWordLastSP, pOldNextSP, FALSE);

			// punctuation changes will affect m_srcPhrase and m_targetStr in both pOldSP
			// and pOldNextSP, so generate the relevant punctuated strings using the m_key
			// and m_adaption members (both are punctuation-less members) as starting points
			wxString srcPhrase = pOldSP->m_precPunct;
			srcPhrase += pOldSP->m_key;
			srcPhrase += pOldSP->m_follPunct;
			srcPhrase += pOldSP->GetFollowingOuterPunct();
			pOldSP->m_srcPhrase = srcPhrase;
			// do the same for pOldNextSP's m_srcPhrase member
			srcPhrase = pOldNextSP->m_precPunct;
			srcPhrase += pOldNextSP->m_key;
			srcPhrase += pOldNextSP->m_follPunct;
			srcPhrase += pOldNextSP->GetFollowingOuterPunct();
			pOldNextSP->m_srcPhrase = srcPhrase;
			// now do pOldSP's m_targetStr member
			wxString tgtStr;
			if (!pOldSP->m_adaption.IsEmpty())
			{
				tgtStr = pOldSP->m_precPunct;
				tgtStr += pOldSP->m_adaption;
				tgtStr += pOldSP->m_follPunct;
				tgtStr += pOldSP->GetFollowingOuterPunct();
				pOldSP->m_targetStr = tgtStr;
			}
			else
			{
				pOldSP->m_targetStr.Empty();
			}
			// finally, the same for pOldNext...
			if (!pOldNextSP->m_adaption.IsEmpty())
			{
				tgtStr = pOldNextSP->m_precPunct;
				tgtStr += pOldNextSP->m_adaption;
				tgtStr += pOldNextSP->m_follPunct;
				tgtStr += pOldNextSP->GetFollowingOuterPunct();
				pOldNextSP->m_targetStr = tgtStr;
			}
			else
			{
				pOldNextSP->m_targetStr.Empty();
			}

			// the fixedspace get's lost, so that's all we can do here, and
			// get the indices right for where to kick off from in the parent
			oldDoneToIncluding = oldNextIndex;
			newDoneToIncluding = newIndex;
			return TRUE;
		}
		else
		{
			// no conjoining here, return FALSE, ,and
			// get the indices right for where to kick off from in the parent
			oldDoneToIncluding = oldIndex - 1;
			newDoneToIncluding = newIndex - 1;
			return FALSE;
		}
	}
	else
	{
		// it's either situation (2) or situation (3), because pOldSP is a fixedspace
		// conjoining
		if (IsFixedSpaceSymbolWithin(pNewSP))
		{
			// it's situation (2) as explained above, so copy the m_adaption contents from pOldSP
			// to pNewSP, recreate the m_targetStr members using the possibly changed punctuation,
			// then deep copy pNewSP and replace pOldSp with it
			pNewSP->m_adaption = pOldSP->m_adaption; // this is <word1>~<word2>, with no punctuation

			// in order to build a pNewSP->m_targetStr with the possibly new punctuation
			// settings, we need access to the two individual new CSourcePhrase instances
			SPList::Node* posNew = pNewSP->m_pSavedWords->GetFirst();
			SPList::Node* posNewLast = pNewSP->m_pSavedWords->GetLast();
			CSourcePhrase* pNewWordFirst = posNew->GetData();
			CSourcePhrase* pNewWordLast = posNewLast->GetData();

			// we also need the equivalent instances from pOldSP...
			SPList::Node* pos = pOldSP->m_pSavedWords->GetFirst();
			SPList::Node* posLast = pOldSP->m_pSavedWords->GetLast();
			CSourcePhrase* pWordFirst = pos->GetData();
			CSourcePhrase* pWordLast = posLast->GetData();

			// first, rebuild pNewWordFirst->m_targetStr, starting from pWordFirst->adaption
			pNewWordFirst->m_targetStr = pNewWordFirst->m_precPunct;
			pNewWordFirst->m_targetStr += pWordFirst->m_adaption;
			pNewWordFirst->m_targetStr += pNewWordFirst->m_follPunct;
			pNewWordFirst->m_targetStr += pNewWordFirst->GetFollowingOuterPunct();

			// next, rebuild pNewWordLast->m_targetStr, starting from pWordLast->adaption
			pNewWordLast->m_targetStr = pNewWordLast->m_precPunct;
			pNewWordLast->m_targetStr += pWordLast->m_adaption;
			pNewWordLast->m_targetStr += pNewWordLast->m_follPunct;
			pNewWordLast->m_targetStr += pNewWordLast->GetFollowingOuterPunct();

			// finally, rebuild pNewSP->m_targetStr by concatenating the previous two with ~
			pNewSP->m_targetStr = pNewWordFirst->m_targetStr + _T("~");
			pNewSP->m_targetStr += pNewWordLast->m_targetStr;

			// now make a deep copy of pNewSP
			CSourcePhrase* pNewDeepCopy = new CSourcePhrase(*pNewSP);
			pNewDeepCopy->DeepCopy();

			// now replace pOldSP with pNewDeepCopy and delete pOldSP
#ifdef _WXDEBUG__
			CSourcePhrase** pDetached = arrOld.Detach(oldIndex);
			wxASSERT(pOldSP == *pDetached);
#endif
			arrOld.Insert(pNewDeepCopy,oldIndex);
			pDoc->DeleteSingleSrcPhrase(pOldSP,TRUE); // assume there may be a partner
					// pile, and ask for it to be removed from the document's pile list,
					// otherwise we'd leak memory here (it's safe, doesn't crash if there
					// actually is no partner pile in existence)
			// get the indices right for where to kick off from in the parent
			oldDoneToIncluding = oldIndex;
			newDoneToIncluding = newIndex;
			return TRUE;
		}
		else
		{
			// it's situation (3) as explained above; unfortunately, the fixedspace will
			// be retained even though the user's source phrase edit removed it, but at
			// least we'll get the USFMs and punctuations right
			int newNextIndex = newIndex + 1;
			CSourcePhrase* pNewNextSP = arrNew.Item(newNextIndex); // it's gotta be there

			SPList::Node* pos = pOldSP->m_pSavedWords->GetFirst();
			SPList::Node* posLast = pOldSP->m_pSavedWords->GetLast();
			CSourcePhrase* pWordFirstSP = pos->GetData();
			CSourcePhrase* pWordLastSP = posLast->GetData();

			// handle the arrOld's conjoined CSourcePhrase's embedded CSourcePhrases first;
			// FALSE is  bool bClearAfterwards; after that, handle their parent instance
			TransferPunctsAndMarkersOnly(pNewSP, pWordFirstSP, FALSE);
			TransferPunctsAndMarkersOnly(pNewNextSP, pWordLastSP, FALSE);

			// now the parent
			// FALSE, FALSE, FALSE, is: bool bAICustomMkrsAlso, bool bFlagsToo, bool bClearAfterwards
			TransferPrecedingMembers(pNewSP, pOldSP, FALSE, FALSE, FALSE);
			// FALSE, FALSE, is: bool bFlagsToo, bool bClearAfterwards
			TransferFollowingMembers(pNewNextSP, pOldSP, FALSE, FALSE);

			// Now we need to rebuild pOldSP->m_srcPhrase, and pOldSP->m_targetStr with
			// the new punctuation (possibly changed) settings, and also the same members
			// in the instances stored in pOldSP->m_pSavedWords.
			// Start by rebuilding pWordFirstSP->m_srcPhrase starting from its m_key member
			pWordFirstSP->m_srcPhrase = pNewSP->m_precPunct;
			pWordFirstSP->m_srcPhrase += pNewSP->m_key; // RHS could instead be pWordFirstSP->m_key
			pWordFirstSP->m_srcPhrase += pNewSP->m_follPunct;
			pWordFirstSP->m_srcPhrase += pNewSP->GetFollowingOuterPunct();
			// Next, rebuild pWordLastSP->m_srcPhrase, starting from m_key
			pWordLastSP->m_srcPhrase = pNewNextSP->m_precPunct;
			pWordLastSP->m_srcPhrase += pNewNextSP->m_key; // RHS could instead be pWordLastSP->m_key
			pWordLastSP->m_srcPhrase += pNewNextSP->m_follPunct;
			pWordLastSP->m_srcPhrase += pNewNextSP->GetFollowingOuterPunct();
			// now rebuild the parent's m_srcPhrase member
			pOldSP->m_srcPhrase = pWordFirstSP->m_srcPhrase + _T("~");
			pOldSP->m_srcPhrase += pWordLastSP->m_srcPhrase;
			// Next, rebuild pWordFirstSP->m_targetStr from its m_adaption member
			if (!pWordFirstSP->m_adaption.IsEmpty())
			{
				pWordFirstSP->m_targetStr = pNewSP->m_precPunct;
				pWordFirstSP->m_targetStr += pWordFirstSP->m_adaption;
				pWordFirstSP->m_targetStr += pNewSP->m_follPunct;
				pWordFirstSP->m_targetStr += pNewSP->GetFollowingOuterPunct();
			}
			else
			{
				pWordFirstSP->m_targetStr.Empty();
			}
			// Next, rebuild pWordLastSP->m_targetStr from its m_adaption member
			if (!pWordLastSP->m_adaption.IsEmpty())
			{
				pWordLastSP->m_targetStr = pNewNextSP->m_precPunct;
				pWordLastSP->m_targetStr += pWordLastSP->m_adaption;
				pWordLastSP->m_targetStr += pNewNextSP->m_follPunct;
				pWordLastSP->m_targetStr += pNewNextSP->GetFollowingOuterPunct();
			}
			else
			{
				pWordLastSP->m_targetStr.Empty();
			}
			// now do pOldSP's m_targetStr member, creating the conjoining
			wxString tgtStr;
			wxString tilde = _T("~");
			if (!pWordFirstSP->m_targetStr.IsEmpty())
			{
				tgtStr = pWordFirstSP->m_targetStr;
			}
			tgtStr += tilde;
			if (!pWordLastSP->m_targetStr.IsEmpty())
			{
				tgtStr += pWordLastSP->m_targetStr;
			}
			if (tgtStr.Find(tilde) != wxNOT_FOUND && tgtStr.Len() == 1)
			{
				// don't accept a conjoining of two empty strings as meaning anything
				tgtStr.Empty();
			}
			pOldSP->m_targetStr = tgtStr;

			// get the indices right for where to kick off from in the parent
			oldDoneToIncluding = oldIndex;
			newDoneToIncluding = newNextIndex;
			return TRUE;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                 FALSE immediately if the placeholder is within a retranslation,
///                         also FALSE is returned if there is a following merger in
///                         arrOld and the attempt to transfer USFMs and punctuation from
///                         the relevant part of arrNew fails - and in this case,
///                         oldDoneToIncluding and newDoneToIncluding will return -1 and -1.
///                         Otherwise do the processing and return TRUE (for a manually
///                         placed placeholder)
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as
///                         well as minimal CSourcePhrase instances) We pass in the WHOLE
///                         array, not just a subrange for the commonSpan
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be single-word
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too) We pass in the WHOLE
///                         array, not just a subrange for the commonSpan
/// \param  oldIndex    ->  index in arrOld where the placeholder is (or if the user has
///                         inserted more than one, where the first one is)
/// \param  newIndex    ->  the next CSourcePhrase instance in the arrNew array, it can't
///                         be a placeholder of course, so is the first of what follows
///                         where an equivalent one would be if present (placeholders on
///                         span boundaries will not be included in the in-common span, so
///                         that means there should be at least one CSourcePhrase following
///                         the oldIndex location in the in-common subspan in arrOld, and
///                         hence at matching one or ones in arrNew, since this is commonSpan
/// \param  pSubspan    ->  the Subspan instance, it's a commonSpan, which we are doing the
///                         transfers of USFM and punctuation to the to-be-retained
///                         CSourcePhrase instances in this subspan of arrOld
/// \param  oldDoneToIncluding  <-  index in arrOld of the last post-placeholder which has
///                                 been updated within this function (note, if there are
///                                 consecutive manually placed placeholders, it would be
///                                 the first CSourcePhrase past the last of those, rather
///                                 than the one following the one at oldIndex)
/// \param  newDoneToIncluding  <-  index in arrNew of the last CSourcePhrase involved in
///                                 updating whatever is the first CSourcePhrase instance
///                                 past the last placeholder in arrOld updated herein
/// \remarks
/// The caller needs the last two parameters, as the kick-off location for scanning
/// forwards for further associations of instances between arrOld and arrNew starts from
/// these index values + 1. If the placeholder has right-association, then we need to
/// update what follows it first, so that we can then apply the data transfers appropriate
/// for right association - overwriting the relevant members in the placeolder with the
/// possibly new data for markers and punctuation etc. The reason we pass in the whole
/// arrOld and arrNew is that we may need to access a CSourcePhrase instance in arrOld and
/// or arrNew which lies beyond the bounds of the commonSpan itself. Our approach in this
/// function is to update what's either side of the merger, and then re-establish any left
/// or right association that we detect.
bool TransferToManualPlaceholder(SPArray& arrOld, SPArray& arrNew, int oldIndex, int newIndex,
				Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding)
{
	wxASSERT(pSubspan->spanType == commonSpan);

	// check indices don't violate pSubspan's  bounds
	if (oldIndex < pSubspan->oldStartPos || oldIndex > pSubspan->oldEndPos)
	{
		// out of range in subspan in arrOld
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	if (newIndex < pSubspan->newStartPos || newIndex > pSubspan->newEndPos)
	{
		// out of range in subspan in arrNew
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}

	// verify it really is a placeholder and get the index of the first non-placeholder
	// following it (placeholders within arrOld retranslations are autoinserted, not
	// user-inserted, so we don't try to do anything with those, because we don't allow a
	// manually placed placeholder to left-associate to the end of a retranslation where
	// they'd be if the retranslation was long enough)
	CSourcePhrase* pPlaceholder = arrOld.Item(oldIndex);
	int oldLastIndex = arrOld.GetCount() - 1;
	//int newLastIndex = arrNew.GetCount() - 1;
	if (!(pPlaceholder->m_bNullSourcePhrase && !pPlaceholder->m_bRetranslation))
	{
		// the placeholder is within a retranslation, or it's not a placeholder; so just
		// treat this as a location where nothing needs to be done, and have the caller
		// kick-off from it on next iteration (we don't expect control to ever enter this
		// block)
		oldDoneToIncluding = oldIndex;
		newDoneToIncluding = newIndex;
		return FALSE;
	}
	// get the index of the last placeholder if there is a sequence, and the indices of
	// the CSourcePhrase instances immediately before the first, and immediately after the
	// last
	int oldPrevIndex = oldIndex - 1;
	CSourcePhrase* pPrevSrcPhrase = arrOld.Item(oldPrevIndex); // there should be at least one such
	int oldFollIndex = oldIndex + 1;
	CSourcePhrase* pFollSrcPhrase = arrOld.Item(oldFollIndex);
	while ((pFollSrcPhrase->m_bNullSourcePhrase && !pFollSrcPhrase->m_bRetranslation))
	{
		oldFollIndex++;
		if (oldFollIndex <= oldLastIndex)
		{
			pFollSrcPhrase = arrOld.Item(oldFollIndex);
		}
		else
		{
			oldFollIndex--;
			break;
		}
	}
	pFollSrcPhrase = arrOld.Item(oldFollIndex);
	// the CSourcePhrase instance at newIndex may be a fixedspace conjoined pair, so we
	// have to test for this
	CSourcePhrase* pSPAtNewLoc = arrNew.Item(newIndex);
	WhatYouAre follSrcPhraseType = WhatKindAreYou(pFollSrcPhrase, pSPAtNewLoc);

	int oldLastPlaceholder = oldFollIndex - 1;
	CSourcePhrase* pLastPlaceholder = arrOld.Item(oldLastPlaceholder);

	// store a record of whether the only or first placeholder is left associated
	bool bIsLeftAssociated = IsLeftAssociatedPlaceholder(pPlaceholder);
	// store a record of whether the only or last placeholder is right associated
	bool bIsRightAssociated = IsRightAssociatedPlaceholder(pLastPlaceholder);

    // the previous non-placeholder CSourcePhrase will have been updated already, so we can
    // use it's USFM and punctuation settings - so if bIsLeftAssociated, do the data
    // transfers and clear the relevant fields on the non-placeholder instance, and rebuild
    // the m_targetStr member, and then return TRUE
	if (bIsLeftAssociated)
	{
		// TRUE, TRUE is:  bool bFlagsToo, bool bClearAfterwards
		TransferFollowingMembers(pPrevSrcPhrase, pPlaceholder, TRUE, TRUE);

		wxString tgtStr = pPlaceholder->m_precPunct;
		tgtStr += pPlaceholder->m_adaption;
		tgtStr += pPlaceholder->m_follPunct;
		tgtStr += pPlaceholder->GetFollowingOuterPunct();
		pPlaceholder->m_targetStr = tgtStr;

		oldDoneToIncluding = oldLastPlaceholder;
		newDoneToIncluding = newIndex - 1;
		return TRUE;
	}
    // the non-placeholder which follows the only or last placeholder will NOT have been
    // updated, so we must do it now, but only if bIsRightAssociated is TRUE; if the latter
    // is FALSE, we've nothing to do and we can return
	if (!bIsRightAssociated)
	{
		// it's not right associated...
		oldDoneToIncluding = oldLastPlaceholder;
		newDoneToIncluding = newIndex - 1;
		return TRUE;
	}
	else
	{
		// it IS right associated, so we've some work to do to update what is at
		// oldFollIndex -  it might be just a single-word CSourcePhrase instance, or a
		// merger, or a retranslation's beginning, or a fixedspace conjoined pair (if a
		// retranslation beginning, right association isn't allowed, so just leave the
		// placeholder as is)... so work out what is there and do the relevant processing,
		// then re-do the right association
		if (pFollSrcPhrase->m_bBeginRetranslation)
		{
			// we can't right associate
			oldDoneToIncluding = oldLastPlaceholder;
			newDoneToIncluding = newIndex - 1;
			return TRUE;
		}
		// is it a plain jane single-word CSourcePhrase instance which follows?
		if (follSrcPhraseType == singleton && pFollSrcPhrase->m_nSrcWords == 1)
		{
			// first, update using the arrNew possibly different USFM and markers info
			bool bDidItOk = TRUE;
			int nOldTempIndex;
			int nNewTempIndex;
			bDidItOk = TransferToSingleton(arrOld, arrNew, oldFollIndex, newIndex,
						 pSubspan, nOldTempIndex, nNewTempIndex);
			// it's unlikely that the return value will be FALSE
			gpApp->LogUserAction(_T("TransferToManualPlaceholder(): did not transfer - might be a bounds error, line 6333 in MergeUpdatedSrc.cpp"));
			wxCHECK_MSG(bDidItOk, FALSE, _T("TransferToManualPlaceholder(): did not transfer - probably a bounds error, line 6333 in MergeUpdatedSrc.cpp, processing will continue..."));
			// get the updated copy of pFollSrcPhrase
			pFollSrcPhrase = arrOld.Item(oldFollIndex);

            // now do the transfers involved with right-association (this doesn't access
            // any arrNew stuff, because that kind of info is already built into
            // pFollSrcPhrase from the call immmediately above), and we now have to
            // transfer any filtered info, and free trans, preceding punctuation, begin
            // markers, etc, if present
            // TRUE, TRUE, TRUE is: bool bAICustomMkrsAlso, bool bFlagsToo, bool bClearAfterwards
			TransferPrecedingMembers(pFollSrcPhrase, pLastPlaceholder, TRUE, TRUE, TRUE);

			// finally, rebuild pLastPlaceholder->m_targetStr because the punctuation may
			// have changed
			pLastPlaceholder->m_targetStr = pLastPlaceholder->m_precPunct;
			pLastPlaceholder->m_targetStr += pLastPlaceholder->m_adaption;
			pLastPlaceholder->m_targetStr += pLastPlaceholder->m_follPunct;
			pLastPlaceholder->m_targetStr += pLastPlaceholder->GetFollowingOuterPunct();

			// we are done, set the kickoff locations and return TRUE
			oldDoneToIncluding = nOldTempIndex;
			newDoneToIncluding = nNewTempIndex;
			return TRUE;
		}
		// is it a merger? update it if so, etc
		if (follSrcPhraseType == merger && pFollSrcPhrase->m_nSrcWords > 1)
		{
			// it is a merger, and not a conjoining with fixed space
			int numWords = pFollSrcPhrase->m_nSrcWords;
			numWords = numWords; // avoid compiler warning in release version
			int dummyOldLoc;
			int dummyNewLoc;
			bool bOK = TransferPunctsAndMarkersToMerger(arrOld, arrNew, oldFollIndex,
										newIndex, pSubspan, dummyOldLoc, dummyNewLoc);
			if (!bOK)
			{
				// unlikely to fail, but accomodate it just in case -- in this scenario,
				// we'll just accept the placeholder without having done right-association
				// data transfers, so that we don't abandon the import process
				oldDoneToIncluding = oldLastPlaceholder;
				newDoneToIncluding = newIndex;
				return FALSE;
			}
            // now do the transfers involved with right-association (this doesn't access
            // any arrNew stuff, because that kind of info is already built into
            // pFollSrcPhrase from the call immmediately above), and we now have to
            // transfer any filtered info, and free trans, preceding punctuation, begin
            // markers, etc, if present
            // TRUE, TRUE, TRUE is: bool bAICustomMkrsAlso, bool bFlagsToo, bool bClearAfterwards
			TransferPrecedingMembers(pFollSrcPhrase, pLastPlaceholder, TRUE, TRUE, TRUE);

			// finally, rebuild pLastPlaceholder->m_targetStr because the punctuation may
			// have changed
			pLastPlaceholder->m_targetStr = pLastPlaceholder->m_precPunct;
			pLastPlaceholder->m_targetStr += pLastPlaceholder->m_adaption;
			pLastPlaceholder->m_targetStr += pLastPlaceholder->m_follPunct;
			pLastPlaceholder->m_targetStr += pLastPlaceholder->GetFollowingOuterPunct();

			// we are done, set the kickoff locations and return TRUE
			oldDoneToIncluding = dummyOldLoc;
			newDoneToIncluding = dummyNewLoc;
			wxASSERT(newDoneToIncluding == newIndex + numWords - 1);
			return TRUE;
		}
		// is it a fixedspace conjoining in arrOld, or a singleton in arrOld which matches
		// up with a newly-formed fixedspace conjoining in arrNew? (shouldn't be the
		// former, but could be the latter)
		if (follSrcPhraseType == conjoined || follSrcPhraseType == singleton_matches_new_conjoined)
		{
			// if the former, our document model does not permit right association to a
			// fixedspace conjoined pair, so just leave the placeholder unchanged; but if
			// the latter, then move the formerly transferred information back to the
			// arrOld's singleton, so that the caller's next iteration can deal with that
			// matchup properly
			if (follSrcPhraseType == conjoined)
			{
				// this block almost certainly will never be entered, since right
				// association to a conjoining is forbidden
				oldDoneToIncluding = oldLastPlaceholder;
				newDoneToIncluding = newIndex - 1;
			}
			else
			{
				// must be the singleton_matches_new_conjoined situation, so move the
				// transferred info back to pFollSrcPhrase, and clear the members where
				// data was taken from
				// TRUE, TRUE, TRUE is: bool bAICustomMkrsAlso, bool bFlagsToo, bool bClearAfterwards
				TransferPrecedingMembers(pLastPlaceholder, pFollSrcPhrase, TRUE, TRUE, TRUE);

				oldDoneToIncluding = oldLastPlaceholder;
				newDoneToIncluding = newIndex - 1;
			}
		}
	} // end of else block for test: if (!bIsRightAssociated), i.e. it IS right-associated
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                 FALSE immediately if arrOld or arrNew passed in index values
///                         are outside the bounds specified by the passed in pSubspan
///                         Otherwise do the processing and return TRUE (for a manually
///                         placed placeholder)
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as
///                         well as minimal CSourcePhrase instances) We pass in the WHOLE
///                         array, not just a subrange for the commonSpan
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be single-word
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too) We pass in the WHOLE
///                         array, not just a subrange for the commonSpan
/// \param  oldIndex    ->  index in arrOld where the CSourcePhrase is (it's a singleton)
/// \param  newIndex    ->  index in arrNew where the matching CSourcePhrase is (it too is
///                         a singleton)
/// \param  pSubspan    ->  ptr to the commonSpan Subspan struct which defines the range of
///                         CSourcePhrase instances in arrOld which are having their
///                         punctuation and USFM data updated by the matched instances within
///                         arrNew
/// \param  oldDoneToIncluding  <-  index in arrOld of the CSourcePhrase just updated
/// \param  newDoneToIncluding  <-  index in arrNew of the CSourcePhrase that contributed
///                                 the data for the updating
/// \remarks
/// The caller needs the last two parameters, as the kick-off location for scanning
/// forwards for further associations of instances between arrOld and arrNew starts from
/// these index values + 1. This function updates punctuation and SFM or USFM markers from
/// the information of those types in the associated CSourcePhrase instance in arrNew,
/// updating to the singleton in arrOld, which is being retained because it is "in common"
/// with the new source text data and within the commonSpan, pSubspan, being processed
bool TransferToSingleton(SPArray& arrOld, SPArray& arrNew, int oldIndex, int newIndex,
						Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding)
{
	wxASSERT(pSubspan->spanType == commonSpan);

	// check indices don't violate pSubspan's  bounds
	if (oldIndex < pSubspan->oldStartPos || oldIndex > pSubspan->oldEndPos)
	{
		// out of range in subspan in arrOld
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	if (newIndex < pSubspan->newStartPos || newIndex > pSubspan->newEndPos)
	{
		// out of range in subspan in arrNew
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	// the indices are in range, get the CSourcePhrase instances which are matched
	CAdapt_ItView* pView = gpApp->GetView();
	CSourcePhrase* pOldSP = arrOld.Item(oldIndex);
	CSourcePhrase* pNewSP = arrNew.Item(newIndex);
#if defined( myLogDebugCalls) && defined(_DEBUG)
	wxLogDebug(_T("singleton: <old key>  %s , at oldIndex = %d ; <new key>  %s , at newIndex = %d"),
		pOldSP->m_key.c_str(), oldIndex, pNewSP->m_key.c_str(), newIndex);
#endif
	wxASSERT(pOldSP->m_key == pNewSP->m_key); // they should be in sync

	// transfer the USFM and punctuation data from the new instance to the old
	// FALSE is: bool bClearAfterwards
	oldDoneToIncluding = oldIndex;
	newDoneToIncluding = newIndex;
	TransferPunctsAndMarkersOnly(pNewSP, pOldSP, FALSE);

    // the m_srcPhrase value may have changed because of punctuation added, or punctuation
    // removed, so copy it too; and set the punctuation for m_targetStr as well... use
	// view's member function MakeTargetStringIncludingPunctuation() for that job; but
	// don't do it if the pOldSP instance is in a retranslation
	pOldSP->m_srcPhrase = pNewSP->m_srcPhrase;
	if (!pOldSP->m_bRetranslation)
	{
		pView->MakeTargetStringIncludingPunctuation(pOldSP, pOldSP->m_targetStr);
	}
	return TRUE;
}

// This function doesn't actually do any data transfers, it just performs the
// indices-in-bounds checks, and returns the passed in index values as the kickoff
// locations to the caller so it can use a systematic syntax for kicking off to the next
// location. Autoinserted placeholders don't have any correspondence to anything in
// arrNew, so there's no data to be moved here
// return TRUE if all's well, FALSE if an index is out of bounds
bool TransferToPlaceholderInRetranslation(SPArray& arrOld, SPArray& arrNew, int oldIndex,
		int newIndex, Subspan* pSubspan, int& oldDoneToIncluding, int& newDoneToIncluding)
{
	wxASSERT(pSubspan->spanType == commonSpan);
	arrOld.GetCount(); // to avoid compiler warning
	arrNew.GetCount(); // to avoid compiler warning

	// check indices don't violate pSubspan's  bounds
	if (oldIndex < pSubspan->oldStartPos || oldIndex > pSubspan->oldEndPos)
	{
		// out of range in subspan in arrOld
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}
	if (newIndex < pSubspan->newStartPos || newIndex > pSubspan->newEndPos)
	{
		// out of range in subspan in arrNew
		oldDoneToIncluding = - 1;
		newDoneToIncluding = - 1;
		return FALSE;
	}

	// return the wanted index values, and TRUE
	oldDoneToIncluding = oldIndex;
	newDoneToIncluding = newIndex - 1; // return the pre-advanced index value, since
									   // placeholders are not in arrNew
	return TRUE;
}



////////////////////////////////////////////////////////////////////////////////////////
/// \return                 nothing
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as
///                         well as minimal CSourcePhrase instances)
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be single-word
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too)
/// \param  pSubspan    ->  the Subspan instance which defines either the new CSourcePhrase
///                         instances which are to replace the old ones for a range of index
///                         values in arrOld; or the old CSourcePhrase instances which are
///                         in common and so will be retained from arrOld after having any
///                         USFM and/or punctuation alterations copied over from arrNew
/// \param  pMergedList ->  the list of CSourcePhrase instances being built - when done,
///                         this will become the new version of the document and be laid
///                         out for the user to fill the "holes" with new adaptations
/// \remarks
/// MergeOldAndNew() is called at the bottom of the recursion process, after parent Subspan
/// instances have been successively decomposed to smaller and smaller tuples, finally, at
/// the leaves of the recursion tree we get a Subspan instance which has nothing in common
/// (these can be either beforeSpan or afterSpan types) or all is in common (and every
/// commonSpan is, by definition, one such). When such leaves are reached, merging can be
/// done. The commonSpan merging first copies over any changes from arrNew to the USFM
/// markup and/or punctuation changes to the commonSpan's pointed-at CSourcePhrase
/// instances from arrOld, and then these are appended to pMergedList. On the other hand,
/// beforeSpan or afterSpan will have nothing in common, and so the CSourcPhrase instances
/// they point to are copied from arrNew, appending them to pMergedList.
///
/// Note 1: The source text editing done earlier by someone outside of Adapt It can do any
/// or all of the following (1) alter the spelling of words, (2) move blocks of words
/// around, (3) insert new words, (4) remove existing words. Moving blocks is equivalent to
/// removing from one location and inserting at some other location, so (2) is just a
/// sequence of (3) and (4). Removing words manifests within a Subspan instance as an empty
/// subspan in arrNew, and we signal empty subspans by the starting and ending indices for
/// the subspan being (-1,-1). Inserting words manifests as an empty subspan in arrOld
/// (indicated by indices (-1,-1) for (oldStartPos,oldEndPos) within the Subspan instance).
/// Words edited in their spelling manifest by a subspan within arrOld having a different
/// set of CSourcePhrase instances (ie. different m_key values in the latter) in the
/// subspan within arrNew. The arrNew instances then must replace the arrOld instances.
///
/// Note 2: after the merger done as described above, the Subspan instance passed in MUST
/// be removed from the heap - it is required no longer, and a memory leak would result if
/// it was not deleted after it's data was used.
///
/// Note 3: a Subspan which is ready for merger never has any Subspan instances managed by
/// its childSubspans member - this 3 member array will just be {NULL,NULL,NULL} and so it
/// manages nothing on the heap
////////////////////////////////////////////////////////////////////////////////////////
void MergeOldAndNew(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, SPList* pMergedList)
{
	if (pSubspan->spanType == commonSpan)
	{
		// retain the old ones; but the data has to be scanned for changes to punctuation
		// and SFM structure, and the retained old ones have to receive any alterations
		// needed from the new CSourcePhrase instances before deep copies are made
		bool bOK = DoUSFMandPunctuationAlterations(arrOld, arrNew, pSubspan);
		// we don't expect an error, but if we got a bad one, a non-localizable message
		// will have been seen already, so just go on and use the material in arrOld's
		// span with no updates of USFMs or punctuation from the error location onwards
		if (!bOK)
		{
			wxBell(); // do something here though
		}

		// now make the needed deep copies and store them on pMergedList
		int index;
		CSourcePhrase* pSrcPhrase = NULL;
		for (index = pSubspan->oldStartPos; index <= pSubspan->oldEndPos; index++)
		{
			pSrcPhrase = (CSourcePhrase*)arrOld.Item(index);
			// make a deep copy and append to pMergedList
			CSourcePhrase* pDeepCopy = new CSourcePhrase(*pSrcPhrase);
			pDeepCopy->DeepCopy();
			pMergedList->Append(pDeepCopy);
		}
	}
	else
	{
        // retain the new ones - but the situation is a bit more complex than that, we must
        // distinguish between replacements, insertions, and removals (see the function
        // description's Note 1.) Former human editing resulting in insertions
        // or replacements just require deep copying the relevant subspan from arrNew here;
        // instances removed by human editing, however, mean that the arrOld ones in this
        // subspan are just ignored, and nothing is copied from arrNew.
		int index;
		CSourcePhrase* pSrcPhrase = NULL;
		if (pSubspan->newStartPos != -1 && pSubspan->newEndPos != -1)
		{
			// it's not a removal, that is, it's either an insertion or a replacement
			for (index = pSubspan->newStartPos; index <= pSubspan->newEndPos; index++)
			{
				pSrcPhrase = (CSourcePhrase*)arrNew.Item(index);
				// make a deep copy and append to pMergedList
				CSourcePhrase* pDeepCopy = new CSourcePhrase(*pSrcPhrase);
				pDeepCopy->DeepCopy();
				pMergedList->Append(pDeepCopy);
			}
		}
	}
	// delete the Subspan instance
#if defined(_DEBUG) && defined(myLogDebugCalls)
	if (pSubspan->spanType == commonSpan)
	{
		wxString typeStr = _T("commonSpan");
		wxLogDebug(_T("    ** DELETING in MergeOldAndNew() the  %s  which is Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
			typeStr.c_str(), pSubspan->oldStartPos, pSubspan->oldEndPos, pSubspan->newStartPos,
			pSubspan->newEndPos, (int)pSubspan->bClosedEnd);
	}
	else if (pSubspan->spanType == beforeSpan)
	{
		wxString typeStr = _T("beforeSpan");
		wxLogDebug(_T("    ** DELETING in MergeOldAndNew() the  %s  which is Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
			typeStr.c_str(), pSubspan->oldStartPos, pSubspan->oldEndPos, pSubspan->newStartPos,
			pSubspan->newEndPos, (int)pSubspan->bClosedEnd);
	}
	else
	{
		wxString typeStr = _T("afterSpan");
		wxLogDebug(_T("    ** DELETING in MergeOldAndNew() the  %s  which is Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
			typeStr.c_str(), pSubspan->oldStartPos, pSubspan->oldEndPos, pSubspan->newStartPos,
			pSubspan->newEndPos, (int)pSubspan->bClosedEnd);
	}

#endif
	if (pSubspan != NULL) // whm 11Jun12 added NULL test
	{
#if defined(_DEBUG) && defined(myLogDebugCalls)
		if (pSubspan->spanType == commonSpan)
		{
			countCommonSpanDeletions++;
		}
		else if (pSubspan->spanType == beforeSpan)
		{
			countBeforeSpanDeletions++;
		}
		else
		{
			countAfterSpanDeletions++;
		}
#endif
		delete pSubspan;
	}
	pSubspan = NULL;

#if defined(_DEBUG) && defined(myMilestoneDebugCalls)
	// track progress in populating the pMergedList
	wxLogDebug(_T("in MergeOldAndNew(): pMergedList progressive count:  %d  ( from arrOld count = %d , and arrNew count = %d )"),
		pMergedList->GetCount(), arrOld.Count(), arrNew.Count());
#endif
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE if a commonSpan is found, FALSE if not; if the parent
///                         Subspan passed in is a commonSpan, this will be detected and
///                         FALSE returned and no attempt made to find a child tuple
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as
///                         well as minimal CSourcePhrase instances)
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be minimal
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too)
/// \param  pParentSubspan ->  the parent Subspsan instance which defines, by it's index
///                         values, the two subspans (on in arrOld, the other in arrNew)
///                         being compared in order to find the largest commonSpan, and
///                         hence by index arithmetic, what the beforeSpan, commonSpan,
///                         and afterSpan (children) are
/// \param  tuple       <-> an empty one,  passed in and if there are any in-common
///                         words at the appropriate index ranges of arrOld and arrNew,
///                         the child tuple is built after determining what index ranges
///                         are maximal for commonSpan, and RecursiveTupleProcessor()
///                         called on it; but if nothing is in common, tuple remains
///                         empty (storing three null pointers)
/// \param  limit       ->  used for defining how big Subspans are
/// \remarks
/// SetupChildTuple() is called whenever a Subspan instance is ready to be decomposed into
/// a tuple of beforeSpan, commonSpan and afterSpan. Then the tuple is set up,
/// RecursiveTupleProcessor() must be immediately called, passing in the child tuple just
/// defined. When recursion returns to this level, from a beforeSpan Subspan instance, the
/// lower level child tuples will have been processed and the CSourcePhrase instances
/// pertaining to those subspans merged. Then the caller, which is
/// RecursiveTupleProcessor(), will merge the CSourcePhrase instances in current tuple's
/// commonSpan; then RecursiveTupleProcessor() will attempt to handle it's afterSpan - so
/// then SetupChildTuple is called again, and recursion potentially happens. Then recursion
/// returns, in the parent Subspan instance, to that Subspan's parent level - and all the
/// way up in this fashion to the 0th level - where unprocessed CSourcePhrase instances to
/// the right are checked for, a Subspan defined for the next SPAN_LIMIT amount of them,
/// and recusion called on that -- the latter process is how the index ranges being
/// considered move progressively from left to right until all of arrOld and arrNew are
/// processed. That is, processing continues until the 0th level's right hand ends get to
/// the end of arrNew and arrOld. SetupChildTuple(), however, just does part of the job:
/// it defines a child tuple, if one can be defined, and for that it needs to call
/// GetMaxInCommonSubspan().
/// SetupChildTuple() calls RecursiveTupleProcessor() to recurse to lower (less wider)
/// Subspans after GetMaxInCommonSubspan() returns TRUE, and but returns FALSE if no
/// commonSpan was found. (When FALSE is returned, the caller, RecursiveTupleProcessor(),
/// will call MergeOldAndNew() to do the mergers.)
/// GetMaxInCommonSubspan() is used to attempt to find a child commonSpan Subspan. It
/// returns the maximal one if there is one, but NULL if there wasn't one - meaning nothing
/// was in common for the index ranges being considered. If this function returns NULL,
/// FALSE is returned from SetupChildTuple().
/// Note: bClosedEnd is passed in within pParentSubspan, it is used for determining when
/// the parent pSubspan is a bClosedEnd == FALSE one, the child afterSpan then also has to
/// have it's bClosedEnd member set FALSE, since that is the rightmost one in the arrays,
/// for that level
////////////////////////////////////////////////////////////////////////////////////////
bool SetupChildTuple(SPArray& arrOld, SPArray& arrNew, Subspan* pParentSubspan, Subspan* tuple[],
						int limit)
{
 	wxASSERT(pParentSubspan != NULL);
    // get bClosedEnd, if the parent Subspan is the rightmost one bClosedEnd in
    // pParentSubspan is false, we pass a bClosedEnd = FALSE value to the child afterSpan
    // if we succed in defining the latter; otherwise, if bCloseEnd from pParentSubspan is
    // TRUE (the default value) then an afterSpan, if we define one, will not be the
    // rightmost, and the TRUE value must be passed to the afterSpan child Subspan instance
	bool bClosedEnd = pParentSubspan->bClosedEnd;
	// the 3 cells of the tuple passed in always start with NULL stored in each
	//wxASSERT(tuple[0] == NULL);
	//wxASSERT(tuple[1] == NULL);
	//wxASSERT(tuple[2] == NULL);
	if (pParentSubspan->spanType == commonSpan)
	{
		// return FALSE, there was no need to make the SetupChildTuple() call for a
		// Subspan we already know is a commonSpan type
		return FALSE;
	}
	// if control gets to here, the parent Subspan instance is either a beforeSpan or an
	// afterSpan, in either case we need to find it's maximum in-common matched pair of
	// subspans
	Subspan* pCommonSubspan = GetMaxInCommonSubspan(arrOld, arrNew, pParentSubspan, limit);
#if defined( myLogDebugCalls) && defined(_DEBUG)
/*
	// get a log display in the Output window to see what we got for the longest in-common
	// Subspan instance
	if (pCommonSubspan)
	{
		// display what we got
		Subspan* pMaxInCommonSubspan = pCommonSubspan;
		int incommonsCount = pMaxInCommonSubspan->oldEndPos - pMaxInCommonSubspan->oldStartPos + 1;
		{
			int i = 0;
			wxString word[12];
			int nLines = incommonsCount / 12;
			if (incommonsCount % 12 > 0)
				nLines++;
			wxLogDebug(_T("\n\n  MAX Subspan, incommonsCount = %d     in-common CSourcePhrase instances' m_keys in succession:"), incommonsCount);
			int index = 0;
			for (i = 0; i < nLines; i++)
			{
				for (index = 0; index < 12; index++)
				{
					int index2 = i*12 + index + pMaxInCommonSubspan->oldStartPos;
					if (index2 < incommonsCount + pMaxInCommonSubspan->oldStartPos)
					{
						CSourcePhrase* pSP = arrOld.Item(index2);
						word[index] = pSP->m_key;
					}
					else
					{
						word[index] = _T(' ');
					}
				}
				wxLogDebug(_T("   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s"),
					word[0].c_str(), word[1].c_str(), word[2].c_str(), word[3].c_str(), word[4].c_str(),
					word[5].c_str(), word[6].c_str(), word[7].c_str(), word[8].c_str(), word[9].c_str(),
					word[10].c_str(), word[11].c_str());
			}
		}

	} // end of TRUE block for test: if (pCommonSubspan)
	else
	{
		if (pParentSubspan->newStartPos != -1)
		{
			// the subspan in arrNew is not empty, so display the words
			wxLogDebug(_T("*** No Subspans created (as there were no in-common words) ***  arrNew words in this span: "));
			int nonCommonsCount = pParentSubspan->newEndPos - pParentSubspan->newStartPos + 1;
			{
				int i = 0;
				wxString word[12];
				int nLines = nonCommonsCount / 12;
				if (nonCommonsCount % 12 > 0)
					nLines++;
				int index = 0;
				for (i = 0; i < nLines; i++)
				{
					for (index = 0; index < 12; index++)
					{
						int index2 = i*12 + index + pParentSubspan->newStartPos;
						if (index2 < nonCommonsCount + pParentSubspan->newStartPos)
						{
							CSourcePhrase* pSP = arrNew.Item(index2);
							word[index] = pSP->m_key;
						}
						else
						{
							word[index] = _T(' ');
						}
					}
					wxLogDebug(_T("   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s   %s"),
						word[0].c_str(), word[1].c_str(), word[2].c_str(), word[3].c_str(), word[4].c_str(),
						word[5].c_str(), word[6].c_str(), word[7].c_str(), word[8].c_str(), word[9].c_str(),
						word[10].c_str(), word[11].c_str());
				}
			}
		}
		else
		{
			// the subspan in arrNew is empty
			wxLogDebug(_T("*** No Subspans created (as there were no in-common words) ***  arrNew subspan is empty (-1,-1)"));
		}
	} // end of else block for test: if (pCommonSubspan)
*/
#endif

	if (pCommonSubspan != NULL)
	{
		// there are one or more CSourcePhrases which are in common, for the maximal
		// commonSpan Subspan child
		tuple[1] = pCommonSubspan; // its spanType is already set to commonSpan, and
								   // bClosedEnd is already set to TRUE
		// define the beforeSpan and afterSpan
		Subspan* pBeforeSpan = NULL;
		Subspan* pAfterSpan = NULL;
		int parentOldStartAt = pParentSubspan->oldStartPos;
		int parentOldEndAt = pParentSubspan->oldEndPos; // note, if the parent Subspan was
                    // one with its bClosedEnd member set FALSE, then the pCommonSubspan
                    // found above may potentially have it's oldEndPos value set much
                    // larger than parentOldStartAt + SPAN_LIMIT value, in fact the latter
                    // could be as large as arrOld.GetCount() - 1 if the user did not make
                    // edits to the source text later in the document
        int parentNewStartAt = pParentSubspan->newStartPos;
		int parentNewEndAt = pParentSubspan->newEndPos; // note, if the parent Subspan was
                    // one with its bClosedEnd member set FALSE, then the pCommonSubspan
                    // found above may potentially have it's newEndPos value set much
                    // larger than parentNewStartAt + SPAN_LIMIT value, in fact the latter
                    // could be as large as arrNew.GetCount() - 1 if the user did not make
                    // edits to the source text later in the document

		// handle any beforeSpan first....in the InitializeSubspan() calls below, TRUE is
        // bClosedEnd
		if (parentOldStartAt < pCommonSubspan->oldStartPos)
		{
			// the beforeSpan has some content from arrOld, so check the situation in arrNew
			if (parentNewStartAt < pCommonSubspan->newStartPos)
			{
				pBeforeSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
				countBeforeSpans++;
#endif
                // there is content from both arrOld and arrNew for this pBeforeSpan (that
                // means there is potentially some old CSourcePhrase instances to be
                // replaced with ones from the user's edits; we say "potentially" because
                // possibly not all of it would end up being replaced, as there could be
                // smaller subspans within it which are in common, and these would be
                // delineated in the recursing to greater depth
				InitializeSubspan(pBeforeSpan, beforeSpan, parentOldStartAt,
						pCommonSubspan->oldStartPos - 1, parentNewStartAt,
						pCommonSubspan->newStartPos - 1, TRUE);
#if defined( myLogDebugCalls) && defined(_DEBUG)
				wxString typeStr = _T("beforeSpan");
				wxLogDebug(_T("   ** CREATING  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
					typeStr.c_str(), pBeforeSpan->oldStartPos, pBeforeSpan->oldEndPos, pBeforeSpan->newStartPos,
					pBeforeSpan->newEndPos, (int)pBeforeSpan->bClosedEnd);
#endif
			}
			else
			{
				pBeforeSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
				countBeforeSpans++;
#endif
                // There is no content from arrNew to match what is in arrOld here, in that
                // case, set pBeforeSpan's newStartPos and newEndPos values to -1 so the
                // caller will know that the arrNew subspan in this pBeforeSpan is empty
                // (this situation arises when the user's editing of the source phrase text
                // has removed some of it, so the material in arrOld's subspan has to be
                // ignored and nothing from this pBeforeSpan goes into pMergedList)
				InitializeSubspan(pBeforeSpan, beforeSpan, parentOldStartAt,
						pCommonSubspan->oldStartPos - 1, -1, -1, TRUE);
#if defined( myLogDebugCalls) && defined(_DEBUG)
				wxString typeStr = _T("beforeSpan");
				wxLogDebug(_T("    ** CREATING  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
					typeStr.c_str(), pBeforeSpan->oldStartPos, pBeforeSpan->oldEndPos, pBeforeSpan->newStartPos,
					pBeforeSpan->newEndPos, (int)pBeforeSpan->bClosedEnd);
#endif
			}
		} // end of TRUE block for test: if (parentOldStartAt < pCommonSubspan->oldStartPos)
		else
		{
			// there is no content for pBeforeSpan's subspan in arrOld
			if (parentNewStartAt < pCommonSubspan->newStartPos)
			{
				pBeforeSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
				countBeforeSpans++;
#endif
                // when there is no content for beforeSpan in arrOld, set pBeforeSpan's
                // oldStartPos and oldEndPos values to -1 so the caller will know this
                // subspan was empty (this means that the user's editing of the source text
                // at this point inserted CSourcePhrase instances into the source text, so
                // they appear in arrNew, so we must add them to pMergedList later)
				InitializeSubspan(pBeforeSpan, beforeSpan, -1, -1, parentNewStartAt,
					pCommonSubspan->newStartPos - 1, TRUE);
#if defined( myLogDebugCalls) && defined(_DEBUG)
				wxString typeStr = _T("beforeSpan");
				wxLogDebug(_T("     ** CREATING  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
					typeStr.c_str(), pBeforeSpan->oldStartPos, pBeforeSpan->oldEndPos, pBeforeSpan->newStartPos,
					pBeforeSpan->newEndPos, (int)pBeforeSpan->bClosedEnd);
#endif
			}
			else
			{
                // the pCommonSubspan abutts both the start of the parent subspan in arrOld
                // and the parent subspan in arrNew -- meaning that beforeSpan is empty, so
                // in this case delete it and use NULL as it's pointer
				pBeforeSpan = NULL;
			}
		} // end of else block for test: if (parentOldStartAt < pCommonSubspan->oldStartPos)

        // Next, the afterSpan -- and well do different processing depending on the
        // bClosedEnd value. If the latter is TRUE, the end limits are strict limits and
        // we'll process accordingly. If the latter is FALSE, the afterSpan is anything
        // after the pCommonSubspan's oldEndPos value in arrOld, and newEndPos value in
        // arrNew - and it would make sense, since the latter two values may lie close to
        // the parent Subspan's right boundaries, to extend oldEndPos and newEndPos to be a
        // new amount of CSourcePhrase instances, using the limit value (unless limit is
        // -1, in which case oldEndPos and newEndPos for the child pAfterSpan should be set
        // to the arrOld and arrNew limit indices) -- SetEndIndices() does this right
        // boundary setting job for us
        // TRUE or FALSE in the InitializeSubspan() calls below, is the value of bClosedEnd
		if (bClosedEnd)
		{
			// right boundary index limits are to be strictly obeyed...
			if (parentOldEndAt > pCommonSubspan->oldEndPos)
			{
				// pAfterSpan has some arrOld content ..., now check the end of the arrNew
				// subspan
				if (parentNewEndAt > pCommonSubspan->newEndPos)
				{
					pAfterSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
					countAfterSpans++;
#endif
					pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value
							//  in the child pAfterSpan, so that if the parent was rightmost,
							// the child pAfterSpan will also be rightmost
					// pAfterSpan's subspan in arrNew has some content too - obey the end
					// limits (TRUE forces that)
					InitializeSubspan(pAfterSpan, afterSpan, pCommonSubspan->oldEndPos + 1,
								pParentSubspan->oldEndPos, pCommonSubspan->newEndPos + 1,
								pParentSubspan->newEndPos, TRUE);
				}
				else
				{
 					pAfterSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
					countAfterSpans++;
#endif
					pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value
							//  in the child pAfterSpan, so that if the parent was rightmost,
							// the child pAfterSpan will also be rightmost
                   // the end of the preceding commonSpan reaches, in arrNew, to the
                    // pParentSubspan's newEndPos value (this means that the user has
                    // earlier edited the old source text at this point by removing some
                    // CSourcePhrase instances, so those in arrOld's subspan have to be
                    // ignored and not copied to pMergedList))
					InitializeSubspan(pAfterSpan, afterSpan, pCommonSubspan->oldEndPos + 1,
								pParentSubspan->oldEndPos, -1, -1, TRUE);
				}
			} // end of TRUE block for test: if (parentOldEndAt > pCommonSubspan->oldEndPos)
			else
			{
                // the end of the preceding commonSpan reaches, in arrOld, to the
                // pParentSubspan's oldEndPos value (this means that the user has, in his
                // edits of the source text, at this location in the document inserted some
                // CSourcePhrase instances - these will need to be added to pMergedList)
				if (parentNewEndAt > pCommonSubspan->newEndPos)
				{
					pAfterSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
					countAfterSpans++;
#endif
					pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value
							//  in the child pAfterSpan, so that if the parent was rightmost,
							// the child pAfterSpan will also be rightmost
					InitializeSubspan(pAfterSpan, afterSpan, -1, -1,
								pCommonSubspan->newEndPos + 1, parentNewEndAt, TRUE);
				}
				else
				{
					// the pCommonSubspan has no content from either arrOld nor arrNew
					// after the commonSpan
					pAfterSpan = NULL;
				}
			} // end of else block for test: if (parentOldEndAt > pCommonSubspan->oldEndPos)
		} // end of TRUE block for test: if (bClosedEnd)
		else
		{
			// right boundary index limits are potentially fluid - we've a bit more to do...
			// (FALSE in the InitializeSubspan() calls is value to be assigned to
			// bClosedEnd member)
			int oldMaxIndex = arrOld.GetCount() - 1;
			int newMaxIndex = arrNew.GetCount() - 1;

            // Get tentative values for the new oldStartAt and newStartAt kick-off values
            // for this pAfterSpan based on what follows the end of the in-common span - if
            // anything follows, etc
			int oldStartAt = pCommonSubspan->oldEndPos + 1; // might be > oldMaxIndex
			int newStartAt = pCommonSubspan->newEndPos + 1; // might be > newMaxIndex

			// Check if oldStartAt and/or newStartAt exceed the array bounds - if one
			// does, do a special calculation, if neither do, just call SetEndIndices() to
			// get appropriate end indices set up
			if (oldStartAt > oldMaxIndex)
			{
                // the subspan in pCommonSubspan belonging to arrOld must end at
				// oldMaxIndex, so a special calc is needed here

				// also check the situation which exists in arrNew...
				if (newStartAt > newMaxIndex)
				{
                    // the subspan in pCommonSubspan belonging to arrNew must end at
                    // newMaxIndex also, so both commonSpan subspans end at their
                    // respective arrOld and arrNew ends - hence pAfterSpan is empty & can
                    // be deleted and its pointer set to NULL (for assigning to tuple[2]
                    // below)
					pAfterSpan = NULL;
				}
				else
				{
 					pAfterSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
					countAfterSpans++;
#endif
					pAfterSpan->oldStartPos = oldStartAt; // invalid, but we'll correct it below
					pAfterSpan->newStartPos = newStartAt; // possibly valid, next test will find out

					pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value
							//  in the child pAfterSpan, so that if the parent was rightmost,
							// the child pAfterSpan will also be rightmost
                   // the subspan in pCommonSubspan belonging to arrNew ends earlier than
                    // the end of arrNew, so there are one or more CSourcePhrase instances
                    // beyond pCommonSubspan->newEndPos which we need to process
					InitializeSubspan(pAfterSpan, afterSpan, pCommonSubspan->oldEndPos + 1,
								pParentSubspan->oldEndPos, pCommonSubspan->newEndPos + 1,
								pParentSubspan->newEndPos, FALSE);
                    // override the oldEndPos and newEndPos values in pAfterSpan with more
                    // useful span widths which will promote progress rightwards through
                    // the data
					SetEndIndices(arrOld, arrNew, pAfterSpan, limit);
					if (pAfterSpan->oldEndPos == pCommonSubspan->oldEndPos)
					{
						// there's nothing in the arrOld subspan within this pAfterSpan,
						// so indicate that fact
						pAfterSpan->oldStartPos = -1;
						pAfterSpan->oldEndPos = -1;
					}
				}
			} // end of TRUE block for test: if (oldStartAt > oldMaxIndex)
			else
			{
                // the subspan in pCommonSubspan belonging to arrOld ends earlier than the
                // end of arrOld, so there are one or more CSourcePhrase instances beyond
                // pCommonSubspan->oldEndPos which we need to process

				// check the situation which exists in arrNew...
				if (newStartAt > newMaxIndex)
				{
					pAfterSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
					countAfterSpans++;
#endif
					pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value
							//  in the child pAfterSpan, so that if the parent was rightmost,
							// the child pAfterSpan will also be rightmost
					pAfterSpan->oldStartPos = oldStartAt; // valid
					pAfterSpan->newStartPos = newStartAt; // possibly valid, next test will find out

					// the subspan in pCommonSubspan belonging to arrNew must end at
					// newMaxIndex, so a special calc is needed here
					InitializeSubspan(pAfterSpan, afterSpan, pCommonSubspan->oldEndPos + 1,
								pParentSubspan->oldEndPos, -1, -1, FALSE);
                    // override the oldEndPos and newEndPos values in pAfterSpan with more
                    // useful span widths which will promote progress rightwards through
                    // the data
					SetEndIndices(arrOld, arrNew, pAfterSpan, limit); // only oldStartPos and
																	  // oldEndPos are valid
					// newStartPos and newEndPos have to be -1, so override again
					pAfterSpan->newStartPos = -1; // tell caller it's an empty subspan
					pAfterSpan->newEndPos = -1;   // within arrNew
				}
				else
				{
					pAfterSpan = new Subspan;
#if defined( myLogDebugCalls) && defined(_DEBUG)
					countAfterSpans++;
#endif
					pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value
							//  in the child pAfterSpan, so that if the parent was rightmost,
							// the child pAfterSpan will also be rightmost
					pAfterSpan->oldStartPos = oldStartAt; // valid
					pAfterSpan->newStartPos = newStartAt; // possibly valid, next test will find out

                    // the subspan in pCommonSubspan belonging to arrNew ends earlier than
                    // the end of arrNew, so there are one or more CSourcePhrase instances
                    // beyond pCommonSubspan->newEndPos which we need to process
					InitializeSubspan(pAfterSpan, afterSpan, oldStartAt, pParentSubspan->oldEndPos,
								newStartAt, pParentSubspan->newEndPos, FALSE);
                    // override the oldEndPos and newEndPos values in pAfterSpan with more
                    // useful span widths which will promote progress rightwards through
                    // the data
					SetEndIndices(arrOld, arrNew, pAfterSpan, limit);
				}
			}  // end of else block for test: if (oldStartAt > oldMaxIndex)

		} // end of else block for test: if (bClosedEnd) i.e. the pAfterSpan is open ended

		// tuple[1] is set already, so set the remaining two
		tuple[0] = pBeforeSpan;
		tuple[2] = pAfterSpan;
	} // end of TRUE block for test: if (pCommonSubspan != NULL)
	else
	{
		// NULL was returned, so there is nothing in common; tell that to the caller,
		// which will then append the parent Subspan's arrNew instances to pMergedList
		// (the tuple passed in still has each of its 3 cells storing NULL)
		return FALSE;
	}
	return TRUE;
}

void RecursiveTupleProcessor(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList,
							   int limit, Subspan* tuple[])
{
	int siz = tupleSize; // set to:  const int 3;
	int tupleIndex;
	Subspan* pParent = NULL;
	SubspanType type;
	bool bMadeChildTuple = FALSE;
	bool bIsClosedEnd = TRUE; // default, but can be changed below

#if defined(_DEBUG) && defined(myLogDebugCalls)
	wxString allOldSrcWords; // for debugging displays in Output window using
	wxString allNewSrcWords; // wxLogDebug() calls that are below
#endif

	for (tupleIndex = 0; tupleIndex < siz; tupleIndex++)
	{
		if (tupleIndex == 1) //  the commonSpan (middle of the three)
		{
			if (tuple[1] == NULL)
			{
				continue;
			}
			else
			{
                // Handle the CSourcePhrase instances which are in common (i.e. unchanged
                // by the user's edits of the source text outside of the application). For
                // these we need no child tuple, but can immediately make deep copies and
                // store in pMergedList, after copying over any USFM changes and/or
                // punctuation changes. (The ptr to Subspan which is stored in
                // tuple[tupleIndex] is removed from the heap before MergeOldAndNew()
                // returns)
                pParent = tuple[tupleIndex];
				type = pParent->spanType;
				wxASSERT(type == commonSpan);
				type = type; // avoid warning
				bIsClosedEnd = TRUE;
				wxASSERT(bIsClosedEnd == pParent->bClosedEnd);
				bIsClosedEnd = bIsClosedEnd; // avoid warning

#if defined(_DEBUG) && defined(myLogDebugCalls)
				{
				// The GetKeysAsAString_KeepDuplicates() calls are not needed, but useful initially for
				// debugging purposes -- comment them out later on
				int oldWordCount = 0;
				// TRUE is bShowOld
				oldWordCount = GetKeysAsAString_KeepDuplicates(arrOld, pParent, TRUE, allOldSrcWords, limit);
				wxLogDebug(_T("tupleIndex = %d; commonSpan oldWordCount = %d ,  allOldSrcWords:  %s"), tupleIndex, oldWordCount, allOldSrcWords.c_str());
				int newWordCount = 0;
				// FALSE is bShowOld
				newWordCount = GetKeysAsAString_KeepDuplicates(arrNew, pParent, FALSE, allNewSrcWords, limit);
				wxLogDebug(_T("tupleIndex = %d; commonSpan newWordCount = %d ,  allNewSrcWords:  %s"), tupleIndex, newWordCount, allNewSrcWords.c_str());
				}
#endif
#if defined(_DEBUG) && defined(myLogDebugCalls)
					//SubspanType type = pParent->spanType;
					if (type == beforeSpan)
					{
						//wxString typeStr = _T("beforeSpan");
						//wxLogDebug(_T("** NO RECURSE for  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						//	typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
						//	pParent->newEndPos, (int)pParent->bClosedEnd);
					}
					else if (type == commonSpan)
					{
						wxString typeStr = _T("commonSpan");
						wxLogDebug(_T("** NO RECURSE for  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
							typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
							pParent->newEndPos, (int)pParent->bClosedEnd);
					}
					else
					{
						//wxString typeStr = _T("afterSpan");
						//wxLogDebug(_T("** NO RECURSE for  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						//	typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
						//	pParent->newEndPos, (int)pParent->bClosedEnd);
					}
#endif
				// do the updating of the old CSourcePhrase instances, then the transfers
				// to pMergedList
				MergeOldAndNew(arrOld, arrNew, pParent, pMergedList);
			}
		}
		else
		{
			// tupleIndex is either 0 or 2, that is, we are dealing with either the
			// beforeSpan or the afterSpan
			if (tupleIndex == 0 && tuple[0] == NULL)
			{
				continue; // not defined, so skip this Subspan
			}
			if (tupleIndex == 2 && tuple[2] == NULL)
			{
				continue; // not defined, so skip this Subspan
			}
			// the Subspan ptr stored in tuple[tupleIndex] must exist, so process it -
			// where 'it' is either a beforeSpan or an afterSpan
			pParent = tuple[tupleIndex];

#if defined(_DEBUG) && defined(myLogDebugCalls)
			{
			// The GetKeysAsAString_KeepDuplicates() calls are not needed, but useful initially for
			// debugging purposes -- comment them out later on
			int oldWordCount = 0;
			// TRUE is bShowOld
			oldWordCount = GetKeysAsAString_KeepDuplicates(arrOld, pParent, TRUE, allOldSrcWords, limit);
			wxLogDebug(_T("tupleIndex = %d; oldWordCount = %d ,  allOldSrcWords:  %s"), tupleIndex, oldWordCount, allOldSrcWords.c_str());
			int newWordCount = 0;
			// FALSE is bShowOld
			newWordCount = GetKeysAsAString_KeepDuplicates(arrNew, pParent, FALSE, allNewSrcWords, limit);
			wxLogDebug(_T("tupleIndex = %d; newWordCount = %d ,  allNewSrcWords:  %s"), tupleIndex, newWordCount, allNewSrcWords.c_str());
			}
#endif


#if defined(_DEBUG) && defined(myLogDebugCalls)
			type = pParent->spanType;
			bIsClosedEnd = pParent->bClosedEnd; // only the rightmost afterSpan at
						// any given nesting level will be FALSE, other afterSpans at
						// the same level are always TRUE, and all beforeSpans are TRUE
						// regardless of the level they are at, as are all commonSpans
						// likewise
			if (tupleIndex == 0)
			{
				// pParent stores a beforeSpan, so we must try make a child
				// tuple, and if that returns FALSE, call MergeOldAndNew() to replace the
				// old CSourcePhrase instances from the subspan within arrOld with those
				// from the subspan within arrNew
				wxASSERT(type == beforeSpan); // it has to be a beforeSpan
			}
			else
			{
                // tuple[tupleIndex] stores an afterSpan - it may or may not be closed
                // ended. The SetupChildTuple() will overwrite the default TRUE value of
                // bClosedEnd, in the child afterSpan, with FALSE if it sees that the
                // passed in Subspan's bClosedEnd value is FALSE
				wxASSERT(type == afterSpan); // it has to be a beforeSpan
			}
#endif
			Subspan* aChildTuple[3]; // an array of three pointer-to-Subspan (the
									 // SetupChildTuple() call will create the
									 // pointers internally and pass them back)
			// Make a child tuple which can be populated with beforeSpan, commonSpan, and
			// afterSpan Subspan pointers by the SetupChildTuple() call below
			aChildTuple[0] = NULL;
			aChildTuple[1] = NULL;
			aChildTuple[2] = NULL;

			bMadeChildTuple = SetupChildTuple(arrOld, arrNew, pParent, aChildTuple, limit);

			// are we at a leaf of the recursion tree? If so, do the merging, if not
			// then a child tuple was successfully made, and we must immediately
			// recurse to process it
			if (bMadeChildTuple)
			{

#if defined(_DEBUG) && defined(myLogDebugCalls)
				if (pParent != NULL)
				{
					wxLogDebug(_T("**** RECURSING to process TUPLE made from parent Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos, pParent->newEndPos, (int)pParent->bClosedEnd);
				}
#endif
				RecursiveTupleProcessor(arrOld, arrNew, pMergedList, limit, aChildTuple);

				// on return, if it's a beforeSpan owner, or an afterSpan owner, then it
				// should be deleted here
				if (pParent->spanType == beforeSpan)
				{
#if defined(_DEBUG) && defined(myLogDebugCalls)
					wxString typeStr = _T("beforeSpan");
					wxLogDebug(_T("   **(on return) DELETING  the parent subspan which is a  %s : { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
						pParent->newEndPos, (int)pParent->bClosedEnd);
#endif
					if (pParent != NULL) // whm 11Jun12 added NULL test
					{
						delete pParent;
#if defined(_DEBUG) && defined(myLogDebugCalls)
						countBeforeSpanDeletions++;
#endif
					}
				}
				else if (pParent->spanType == afterSpan)
				{
#if defined(_DEBUG) && defined(myLogDebugCalls)
					wxString typeStr = _T("afterSpan");
					wxLogDebug(_T("   **(on return) DELETING  the parent subspan which is a  %s : { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
						pParent->newEndPos, (int)pParent->bClosedEnd);
#endif
					if (pParent != NULL) // whm 11Jun12 added NULL test
					{
						delete pParent;
#if defined(_DEBUG) && defined(myLogDebugCalls)
						countAfterSpanDeletions++;
#endif
					}
				}
			}
			else
			{
				// didn't make a child tuple, so the local variable aChildTuple is
				// still [NULL,NULL,NULL]; so we don't recurse, but merge the Subspan
				// instance that pParent points at
				if (pParent != NULL)
				{
#if defined(_DEBUG) && defined(myLogDebugCalls)
					SubspanType type = pParent->spanType;
					if (type == beforeSpan)
					{
						wxString typeStr = _T("beforeSpan");
						wxLogDebug(_T("  ** NO RECURSE, but copying for  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
							typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
							pParent->newEndPos, (int)pParent->bClosedEnd);
					}
					else if (type == commonSpan)
					{
						wxString typeStr = _T("commonSpan");
						wxLogDebug(_T("  ** NO RECURSE, but copying for  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
							typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
							pParent->newEndPos, (int)pParent->bClosedEnd);
					}
					else
					{
						wxString typeStr = _T("afterSpan");
						wxLogDebug(_T("  ** NO RECURSE, but copying for  %s  Subspan: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
							typeStr.c_str(), pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos,
							pParent->newEndPos, (int)pParent->bClosedEnd);
					}
#endif
				}
				MergeOldAndNew(arrOld, arrNew, pParent, pMergedList);
			}
		} // end of else block for test: if (tupleIndex == 1) i.e. it is 0 or 2

	} // end of for loop: for (tupleIndex = 0; tupleIndex < tupleSize; tupleIndex++)

#if defined(_DEBUG) && defined(myLogDebugCalls)
 	// logDebug the counts for each span type
	wxLogDebug(_T("\nTUPLE DONE  *** Count of beforeSpan: %d"),countBeforeSpans);
	wxLogDebug(_T("TUPLE DONE  Count of commonSpan: %d"),countCommonSpans);
	wxLogDebug(_T("TUPLE DONE  Count of  afterSpan: %d"),countAfterSpans);
	wxLogDebug(_T("\nTUPLE DONE  *** Count of beforeSpan deletions: %d"),countBeforeSpanDeletions);
	wxLogDebug(_T("TUPLE DONE  *** Count of commonSpan deletions: %d"),countCommonSpanDeletions);
	wxLogDebug(_T("TUPLE DONE  *** Count of  afterSpan deletions: %d"),countAfterSpanDeletions);
	wxLogDebug(_T("\nTUPLE DONE  *** UNDELETED AS YET: beforeSpans %d   commonSpans %d   afterSpans %d"),
		countBeforeSpans - countBeforeSpanDeletions, countCommonSpans - countCommonSpanDeletions, countAfterSpans - countAfterSpanDeletions);
#endif
}

/// returns                 TRUE for a successful analysis, FALSE if unsuccessful or an
///                         empty string was passed in
///
/// \param  strChapVerse        ->  ref to a chapter:verse string, or chapter:verse_range string which
///                                 is passed in to be analysed into its parts
/// \param  strChapter          <-  ref to the chapter number as a string
/// \param  nChapter            <-  the int value of strChapter
/// \param  strDelimiter        <-  if present, whatever separates the parts of a verse range
/// \param  strStartingVerse    <-  the starting verse of a range, as a string
/// \param  nStartingVerse      <-  the int value of strStartingVerse
/// \param  strStartingVerseSuffix <- usually a single lower case letter such as a or b
///                                   after strStartingVerse
/// \param  strEndingVerse      <-  the ending verse of a range, as a string
/// \param  nStartingVerse      <-  the int value of strEndingVerse
/// \param  strEndingVerseSuffix <- usually a single lower case letter such as a or b,
///                                 after strEndingVerse
/// \remarks
/// This is similar to the CAdapt_ItView::AnalyseReference() function, but the latter does
/// not consider the possibility that there may be suffix characters, and it assumes the
/// only delimiters will be hyphen or comma, it also can handle Arabic chapter number
/// conversion for the Mac (which can't convert using Atoi() or Wtoi() if the input is
/// Arabic digits.
/// AnalyseChapterVerseRef() will likewise handle conversion of Arabic chapter numbers to
/// western digits for the Mac - doing it also for the verse digits, and will get the
/// suffix characters if present - it is meant for filling out most of the members of the
/// SfmChunk struct.
/// If "0:0" is passed in, then there was no chapter:verse reference information found in
/// the span (and there should have been) - we will set the 0 for chapter and verse in the
/// caller's struct, and require that our merging algorithms have a robust behaviour if
/// that happens.
bool AnalyseChapterVerseRef(wxString& strChapVerse, wxString& strChapter, int& nChapter,
					wxString& strDelimiter, wxString& strStartingVerse, int& nStartingVerse,
					wxChar& charStartingVerseSuffix, wxString& strEndingVerse,
					int& nEndingVerse, wxChar& charEndingVerseSuffix)
{
    // The Adapt It chapterVerse reference string is always of the form ch:vs or
    // ch:vsrange, the colon is always there except for single chapter books. Single
    // chapter books with no chapter marker will return 1 as the chapter number
	nChapter = -1;
	nStartingVerse = -1;
	nEndingVerse = -1;
	strStartingVerse.Empty();
	strEndingVerse.Empty();
	charStartingVerseSuffix = _T('\0');
	charEndingVerseSuffix = _T('\0');
	strDelimiter.Empty();
	if (strChapVerse.IsEmpty())
		return FALSE; // reference passed in was empty
	wxString range;
	range.Empty();

	// first determine if there is a chapter number present; handle Arabic digits if on a
	// Mac machine and Arabic digits were input
	int nFound = strChapVerse.Find(_T(':'));
	if (nFound == wxNOT_FOUND)
	{
		// no chapter number, so set chapter to 1
		range = strChapVerse;
		nChapter = 1;
		strChapter = _T("1");
	}
	else
	{
		// chapter number exists, extract it and put the remainder after the colon into range
		strChapter = SpanExcluding(strChapVerse,_T(":"));
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
		size_t imak;
		for (imak=0; imak < strChapter.Len(); imak++)
		{
			wxChar imaCh = strChapter.GetChar(imak);
			if (imaCh >= (wchar_t)0x6f0 && imaCh <= (wchar_t)0x6f9)
				strChapter.SetChar(imak, imaCh & (wchar_t)0x3f);	// zero out the higher bits of these Arabic digits
		}
#endif /* __WXMAC__ */
		nChapter = wxAtoi(strChapter);

		nFound++; // index the first char after the colon
		range = strChapVerse.Mid(nFound);
	}

	// now deal with the verse range, or single verse
	int numChars = range.Len();
	int index;
	wxChar aChar = _T('\0');
	// get the verse number, or the first verse number of the range
	int count = 0;
	for (index = 0; index < numChars; index++)
	{
		aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
		if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
		{
			aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
		}
#endif /* __WXMAC__ */
		int isDigit = wxIsdigit(aChar);
		if (isDigit != 0)
		{
			// it's a digit
			strStartingVerse += aChar;
			count++;
		}
		else
		{
			// it's not a digit, so exit with what we've collected so far
			break;
		}
	}
	if (count == numChars)
	{
		// all there was in the range variable was the digits of a verse number, so set
		// the return parameter values and return TRUE
		nStartingVerse = wxAtoi(strStartingVerse);
		nEndingVerse = nStartingVerse;
		strEndingVerse = strStartingVerse;
		return TRUE;
	}
	else
	{
		// there's more, but get what we've got so far and trim that stuff off of range
		nStartingVerse = wxAtoi(strStartingVerse);
		range = range.Mid(count);
		numChars = range.Len();
		// if a part-verse marker (assume one of a or b or c only), get it
		// whm 11Jun12 modified below. GetChar(0) should not be called on an empty string
		if (!range.IsEmpty())
			aChar = range.GetChar(0);
		else
			aChar = _T('\0');
		if ( aChar == _T('a') || aChar == _T('b') || aChar == _T('c'))
		{
			charStartingVerseSuffix = aChar;
			range = range.Mid(1); // remove the suffix character
			numChars = range.Len();
		}
		if (numChars == 0)
		{
			// we've exhausted the range string, fill params and return TRUE
			strEndingVerse = strStartingVerse;
			charEndingVerseSuffix = charStartingVerseSuffix;
			nEndingVerse = nStartingVerse;
			return TRUE;
		}
		else
		{
			// there is more still, what follows must be the delimiter, we'll assume it is
			// possible to have more than a single character (exotic scripts might need
			// more than one) so search for a following digit as the end point, or string
			// end; and handle Arabic digits too (ie. convert them)
			count = 0;
			for (index = 0; index < numChars; index++)
			{
				aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
				if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
				{
					aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
				}
#endif /* __WXMAC__ */
				int isDigit = wxIsdigit(aChar);
				if (isDigit != 0)
				{
					// it's a digit, so we've reached the end of the delimiter
					break;
				}
				else
				{
					// it's not a digit, so it's part of the delimiter string
					strDelimiter += aChar;
					count++;
				}
			}
			if (count == numChars)
			{
				// it was "all delimiter" - a formatting error, as there is not verse
				// number to indicate the end of the range - so just take the starting
				// verse number (and any suffix) and return FALSE
				strEndingVerse = strStartingVerse;
				charEndingVerseSuffix = charStartingVerseSuffix;
				nEndingVerse = nStartingVerse;
				return FALSE;
			}
			else
			{
                // we stopped earlier than the end of the range string, and so presumably
                // we stopped at the first digit of the verse which ends the range
				range = range.Mid(count); // now just the final verse and possibly an a, b or c suffix
				numChars = range.Len();
				// get the final verse...
				count = 0;
				for (index = 0; index < numChars; index++)
				{
					aChar = range.GetChar(index);
#ifdef __WXMAC__
// Kludge because the atoi() function in the MacOS X standard library can't handle Arabic digits
					if (aChar >= (wchar_t)0x6f0 && aChar <= (wchar_t)0x6f9)
					{
						aChar = aChar & (wchar_t)0x3f; // zero out the higher bits of this Arabic digit
					}
#endif /* __WXMAC__ */
					int isDigit = wxIsdigit(aChar);
					if (isDigit != 0)
					{
						// it's a digit
						strEndingVerse += aChar;
						count++;
					}
					else
					{
						// it's not a digit, so exit with what we've collected so far
						break;
					}
				}
				if (count == numChars)
				{
                    // all there was in the range variable was the digits of the ending
                    // verse number, so set the return parameter values and return TRUE
					nEndingVerse = wxAtoi(strEndingVerse);
					return TRUE;
				}
				else
				{
					// there's more, but get what we've got so far and trim that stuff
					// off of range
					nEndingVerse = wxAtoi(strEndingVerse);
					range = range.Mid(count);
					numChars = range.Len();
					// if a part-verse marker (assume one of a or b or c only), get it
					if (numChars > 0)
					{
						// what remains should just be a final a or b or c
						// whm 11Jun12 modified below. GetChar(0) should not be called on an empty string.
						if (!range.IsEmpty())
							aChar = range.GetChar(0);
						else
							aChar = _T('\0');
						if ( aChar == _T('a') || aChar == _T('b') || aChar == _T('c'))
						{
							charEndingVerseSuffix = aChar;
							range = range.Mid(1); // remove the suffix character
							numChars = range.Len(); // numChars should now be 0
						}
						if (numChars != 0)
						{
							// rhere's still something remaining, so just ignore it, but
							// alert the developer, not with a localizable string
							wxString suffix1;
							wxString suffix2;
							if (charStartingVerseSuffix != _T('\0'))
							{
								suffix1 = charStartingVerseSuffix;
							}
							if (charEndingVerseSuffix != _T('\0'))
							{
								suffix2 = charEndingVerseSuffix;
							}
							wxString msg;
							msg = msg.Format(
_T("The verse range was parsed, and the following remains unparsed: %s\nfrom the specification %s:%s%s%s%s%s%s"),
							range.c_str(),strChapter.c_str(),strStartingVerse.c_str(),suffix1.c_str(),
							strDelimiter.c_str(),strEndingVerse.c_str(),suffix2.c_str(),range.c_str());
							wxMessageBox(msg,_T("Verse range specification error (ignored)"),wxICON_EXCLAMATION | wxOK);
						}
					} //end of TRUE block for test: if (numChars > 0)
				} // end of else block for test: if (count == numChars)
			} // end of else block for test: if (count == numChars)
		} // end of else block for test: if (count == numChars)
	} // end of else block for test: if (numChars == 0)
	return TRUE;
}

// Starting at the index, startFrom, in the array pArray, search ahead for a CSourcePhrase
// instance which does not have an empty m_markers member, return the index if one is
// found, and if the index goes out of bounds, set bReachedEndOfArray TRUE as well,
// otherwise the latter returns FALSE. Return the index (ie. the sequence number), if the
// conditions are met, if not, return -1
int GetNextNonemptyMarkers(SPArray* pArray, int& startFrom, bool& bReachedEndOfArray)
{
	wxString verseMkr = _T("\\v");
	bReachedEndOfArray = FALSE;
	int count = pArray->GetCount();
	if (startFrom >= count)
	{
		bReachedEndOfArray = TRUE;
		return wxNOT_FOUND;
	}
	int index;
	for (index = startFrom; index < count; index++)
	{
		CSourcePhrase* pSrcPhrase = pArray->Item(index);
		// do following block it m_markers is not empty, (it may or may not contain a verse
		// marker - we let the caller test for what it in the m_markers we've found)
		// (including \vn too, so that we will match either \v or \vn successfully)
		if (!pSrcPhrase->m_markers.IsEmpty())
		{
			// the caller will need to examine the contents of the m_markers we found, so
			// we must return the sequence number of it's pSrcPhrase, which is also the
			// current index value in this loop -- and since it's a CSourcePhrase
			// instance, we can't possibly be yet at the end of the array
			return index;

			// BEW 16Aug12, the following was wrong logic - it resulted in two empty
			// CSourcePhrase instances, each a verse, being coalesced into a single group
			//if (index == count - 1)
			//{
			//	bReachedEndOfArray = TRUE;
			//}
			//return index;
		}
	}
	// if control gets to here, we didn't find one
	bReachedEndOfArray = TRUE;
	return wxNOT_FOUND;
}

// Check if the start of arr contains material belonging to stuff which is preceding an
// introduction (if there is an introduction) or before the first chapter, if there are
// chapters, or before the first verse, if there are no chapters; if that is so, keep
// looking until that book-initial material ends - either at an introduction, or chapter
// marker or if not any chapters, at the first verse marker encountered,
// Return the index values for the CSourcePhrase instances which lie at the start and end
// of the book-introduction span and return TRUE, if we do not succeed in delineating any
// such span, return FALSE (and in that case, startsAt and endsAt values are undefined -
// I'll probably set them to -1 whenever FALSE is returned)
bool GetBookInitialChunk(SPArray* arrP, int& startsAt, int& endsAt)
{
	int count = arrP->GetCount();
	int endIndex = count - 1;
	int index = startsAt;
	wxString markers;
	CSourcePhrase* pSrcPhrase = NULL;
	if (count == 0 || (count > 0 && startsAt >= count))
	{
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// does arr start with book-initial material, such as \id or a \mt or \h etc
	// (this stuff is in titleMkrs)
	pSrcPhrase = arrP->Item(index);
	markers = pSrcPhrase->m_markers;
	if (markers.IsEmpty())
	{
		// we don't expect this, but it does at least mean that probably the initial
		// CSourcePhrase isn't at an \id location, nor an introduction, so caller
		// probably should assume a verse type of unit
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// m_markers has content, so check what might be in it
	bool bIsIdOrTitleWithin = IsSubstringWithin(titleMkrs, markers);
	bool bIsIntroductionWithin = IsSubstringWithin(introductionMkrs, markers);
	bool bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
	bool bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
	bool bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);
	bool bIsBookInitialChunk = FALSE;
	if (bIsIdOrTitleWithin)
	{
		bIsBookInitialChunk = TRUE;
	}
	else
	{
		// else, we'll assume it's book-introduction material provided m_markers doesn't
		// contain any introduction markers, nor a chapter marker, nor a subheading, nor
		// a verse marker
		if (!bIsIntroductionWithin && !bIsChapterMkrWithin && !bIsVerseMkrWithin
			&& !bIsSubheadingMkrWithin)
		{
			bIsBookInitialChunk = TRUE;
		}
	}
	if (!bIsBookInitialChunk)
	{
		// return FALSE, we don't know what it is
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}

    // we are in a book-initial chunk; so look ahead until we come to, in order of testing,
    // first, any introduction material, if not that, a chapter number, if not that, a
    // verse marker - those constitute an end of the book-introduction material
	index++; // equals 1 now
	if (index > endIndex)
	{
		// it's a very short array!!!!
		startsAt = 0;
		endsAt = 0;
		return TRUE;
	}
	bool bReachedEndOfArray = FALSE;
	int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
	while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
	{
		// get the m_markers content in this instance, put it into markers
		pSrcPhrase = arrP->Item(foundIndex);
		markers = pSrcPhrase->m_markers;

		// check for any marker which indicates the book-initial material is ended
		bIsIntroductionWithin = IsSubstringWithin(introductionMkrs, markers);
		bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
		bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
		bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

		// are we done?
		if (bIsIntroductionWithin || bIsChapterMkrWithin ||
			bIsSubheadingMkrWithin || bIsVerseMkrWithin)
		{
			// we've found the start of the next information chunk, so the previous index
			// is the ending one for this chunk
			endsAt = foundIndex - 1;
			bReachedEndOfArray = FALSE; // must be so
			return TRUE;
		}
		// end of the chunk wasn't found, so prepare to iterate
		index = foundIndex + 1;
		if (index > endIndex)
		{
			// array end was reached
			bReachedEndOfArray = TRUE;
			endsAt = endIndex;
			return TRUE;
		}
		// search for the next non-empty m_markers member
		foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
	}

	// we didn't find an end, so take it all as book-initial material
	endsAt = endIndex;
	return TRUE;
}

// Check if the start of arr contains material belonging to stuff which is after any
// book-initial material, but before a chapter marker, standard subheader or verse, or
// other markers preceding the first chapter or verse (e.g. \ms ). If that
// is so, keep looking until that introduction material ends - either at a chapter marker
// or if not any chapters, at the first verse marker encountered, or a subheading
// of some kind.
// Return the index values for the CSourcePhrase instances which lie at the start and end
// of the introduction span and return TRUE, if we do not succeed in delineating any
// such span, return FALSE (and in that case, startsAt and endsAt values are undefined -
// I'll probably set them to -1 whenever FALSE is returned)
bool GetIntroductionChunk(SPArray* arrP, int& startsAt, int& endsAt)
{
	int count = arrP->GetCount();
	int endIndex = count - 1;
	int index = startsAt;
	wxString markers;
	CSourcePhrase* pSrcPhrase = NULL;
	if (count == 0 || (count > 0 && startsAt >= count))
	{
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// does the starting position within arr have introduction material, such as \imt or
	// \ip etc? this stuff is in m_introductionMkrs)
	pSrcPhrase = arrP->Item(index);
	markers = pSrcPhrase->m_markers;
	if (markers.IsEmpty())
	{
		// we don't expect this, because an introduction should commence with a marker
		// from the introductionMkrs set, so return FALSE so that the caller won't advance
		// the starting location for the next test (for a chapter beginning)
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// m_markers has content, so check what might be in it
	bool bIsIntroductionWithin = IsSubstringWithin(introductionMkrs, markers);
	bool bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
	bool bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
	bool bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);
	bool bIsIntroductionChunk = FALSE;
    // it's introduction material provided m_markers doesn't contain any subheading
    // markers, nor a chapter marker, nor a subheading, nor a verse marker, but it does
    // have a marker from introductionMkrs set
	if (bIsIntroductionWithin && (!bIsChapterMkrWithin && !bIsVerseMkrWithin && !bIsSubheadingMkrWithin))
	{
		bIsIntroductionChunk = TRUE;
	}
	else
	{
		bIsIntroductionChunk = FALSE;
	}
	if (!bIsIntroductionChunk)
	{
		// return FALSE, we don't know what it is, and we've not yet got to any chapters
		// or verses
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}

    // we are in an introduction chunk; so look ahead until we come to, in order of testing,
    // first, a chapter number, if not that, a subheading, if not that, a
	// verse marker - those constitute an end of the introduction material; we'll also
	// include some other things like \mr, \d \ms, which the user may wrongly put
	// before the chapter number
	bool bIsRangeOrPsalmMkrs = FALSE;
	bool bIsMajorOrSeriesMkrs = FALSE;
	index++;
	if (index > endIndex)
	{
		// we are done
		endsAt = endIndex;
		return TRUE;
	}
	bool bReachedEndOfArray = FALSE;
	int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
	while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
	{
		// get the m_markers content in this instance, put it into markers
		pSrcPhrase = arrP->Item(foundIndex);
		markers = pSrcPhrase->m_markers;

		// check for any marker which indicates the introduction material is ended
		bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
		bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
		bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);
		bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
		bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);


		// are we done?
		if (bIsChapterMkrWithin || bIsSubheadingMkrWithin || bIsVerseMkrWithin ||
			bIsMajorOrSeriesMkrs || bIsRangeOrPsalmMkrs)
		{
			// we've found the start of the next information chunk, so the previous index
			// is the ending one for this chunk
			endsAt = foundIndex - 1;
			bReachedEndOfArray = FALSE; // must be so
			return TRUE;
		}
		// end of the chunk wasn't found, so prepare to iterate
		index = foundIndex + 1;
		if (index > endIndex)
		{
			// array end was reached
			bReachedEndOfArray = TRUE;
			endsAt = endIndex;
			return TRUE;
		}
		// search for the next non-empty m_markers member
		foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
	}

	// we didn't find an end, so take it all as introduction material
	endsAt = endIndex;
	return TRUE;
}

// Check if the start of arr contains material belonging to stuff which is after any
// book-initial and or introduction material, but before the first chapter marker, or if
// none, then before the first verse. Return the index values for the CSourcePhrase
// instances which lie at the start and end of such a span and return TRUE; return FALSE if
// there was an error (and in that case, startsAt and endsAt values are undefined - set
// them to -1 whenever FALSE is returned); and if we run to the end of the text because
// there are no chapter or verse markers, we must return the index of the last word in
// endsAt, and return TRUE, since that is a successful result in such a circumstance.
bool GetPreFirstChapterChunk(SPArray* arrP, int& startsAt, int& endsAt)
{
	int count = arrP->GetCount();
	int endIndex = count - 1;
	int index = startsAt;
	wxString markers;
	CSourcePhrase* pSrcPhrase = NULL;
	if (count == 0 || (count > 0 && startsAt >= count))
	{
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// loop to find the first \c, or failing that, the first \v marker -- any such
	// CSourcePhrase instance is the first of the following section, so end at the one
	// prior to that
	wxString chapterMkr = _T("\\c");
	wxString verseMkr = _T("\\v");
	int offset = wxNOT_FOUND;
	int lastSrcPhraseIndex;
	while (index <= endIndex)
	{
		pSrcPhrase = arrP->Item(index);
		markers = pSrcPhrase->m_markers;
		offset = markers.Find(chapterMkr);
		if (offset != wxNOT_FOUND)
		{
			// found a chapter marker
			lastSrcPhraseIndex = index - 1;
		}
		else
		{
			offset = markers.Find(verseMkr);
			if (offset != wxNOT_FOUND)
			{
				// no chapter marker, but found a verse marker instead, so the milestoned
				// material starts there - so end of any preFirstChapter material is the
				// preceding index
				lastSrcPhraseIndex = index - 1;
			}
			else
			{
				index++; // check the next CSourcePhrase instance for the end
				continue;
			}
		}
		// if control gets to here, we've found either a \c or a \v, but the CSourcePhrase
		// bearing it may have been the one at the startsAt index value -- if so, then
		// there is nothing in the preFirstChapterChunk; but if lastSrcPhraseIndex is
		// equal to or greater than startsAt, then this chunk has some content
		if (lastSrcPhraseIndex < startsAt)
		{
			// the chunk has no content; return FALSE so that the caller won't advance
			// the starting location
			return FALSE;
		}
		else
		{
			// the chunk has content
			endsAt = lastSrcPhraseIndex;
			break;
		}
	} // end of loop:	while (index <= endIndex)

	// if index gets past endIndex, then there were no \v or \c markers, and so we must
	// return the whole lot as a successful match
	if (index > endIndex)
	{
		endsAt = endIndex;
	}
	return TRUE;
}

// Check if the start of arr contains material belonging to stuff which is part of the
// beginning of a chapter: things where \c occurs, and going as far as the end of the
// first verse which follows (this ensures the chunk has a verse number or verse range).
// We do it this way because a \c nnn marker and number always is in the m_markers for a
// bit of text having content, and the latter may be a subheading, or \ms stuff, or \mr
// stuff, or the first verse. Because of the variations possible, it is better to go from
// the \c until the end of the first verse, then it doesn't matter what other content is
// in that chunk.
// Return the index values for the CSourcePhrase instances which lie at the start and end
// of the chapter-plus-verse chunk and return TRUE, if we do not succeed in delineating any
// such span, return FALSE (and in that case, startsAt and endsAt values are undefined -
// I'll probably set them to -1 whenever FALSE is returned)
bool GetChapterPlusVerseChunk(SPArray* arrP, int& startsAt, int& endsAt)
{
	int count = arrP->GetCount();
	int endIndex = count - 1;
	int index = startsAt;
	wxString markers;
	CSourcePhrase* pSrcPhrase = NULL;
	if (count == 0 || (count > 0 && startsAt >= count))
	{
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// does the starting position within arr have chapter material, such as \c or \ms
	// etc? this stuff is in m_chapterMkrs and majorOrSeriesMkrs or rangeOrPsalmMkrs or
	// normalOrMinorMkrs (i.e. subheading markers) or parallelPassageHeadMkrs or verseMkrs)
	pSrcPhrase = arrP->Item(index);
	markers = pSrcPhrase->m_markers;
	if (markers.IsEmpty())
	{
        // we don't expect this, because the start of a chapter should commence with a \c
        // from the chapterMkrs set or with a few other possibilities, so return FALSE so
        // that the caller won't advance the starting location for the next test (for a
        // subheading)
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// m_markers has content, so check what might be in it
	bool bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
	//bool bIsMajorOrSeriesMkrWithin = IsSubstringWithin(majorOrSeriesMkrs, markers);
	//bool bIsRangeOrPsalmMkrWithin = IsSubstringWithin(rangeOrPsalmMkrs, markers);
	bool bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
	//bool bIsParallelPassageHeadMkrWithin = IsSubstringWithin(parallelPassageHeadMkrs, markers);
	bool bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

	bool bIsChapterPlusVerseChunk = FALSE;
    // it's material from \c and we want everything up to the end of the first verse
	if (bIsChapterMkrWithin)
	{
		bIsChapterPlusVerseChunk = TRUE;
	}
	else
	{
		bIsChapterPlusVerseChunk = FALSE;
	}
	if (!bIsChapterPlusVerseChunk)
	{
		// return FALSE, it might be a subheading +/- parallel passage material followed
		// by verses, and we check for that next if this function returns FALSE
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}

    // we are in a chapterPlusVerseChunk; so look ahead until we come to a CSourcePhrase
    // instance with \v in it's m_markers (it may be in the same m_markers that \c is in,
    // or it may be later on, depending on whether or not there is subheading and/or other
    // stuff before the verse commences). Once we have a CSourcePhrase with a \v (or \vn),
    // search ahead to the start of the next instance with either \c or \v (or \vn) - and
    // then the end of the chunk is the previous CSourcePhrase instance to that one. Once
    // we are past any introduction material, we want each chunk to just have a single
    // verse in it, regardless of how much other stuff there might be as well between the
    // verse and whatever chunk preceded it.
	bool bReachedEndOfArray = FALSE;
	bool bCandVonSameInstance = FALSE;
	if (bIsVerseMkrWithin)
	{
        // the \c and the \v are stored on the one CSourcePhrase, so get to the end of the
        // verse - the end will be prior to a following verse or instance with \c stored on
        // it, or instance which starts a subheading, or the end of the array if none of
        // those
		bCandVonSameInstance = TRUE;
		index++;
		if (index > endIndex)
		{
			// we are done
			endsAt = endIndex;
			return TRUE;
		}
		int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		{
			// get the m_markers content in this instance, put it into markers
			pSrcPhrase = arrP->Item(foundIndex);
			markers = pSrcPhrase->m_markers;

			// check for any marker which indicates the chapterPlusVerse material is ended
			bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);
			bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);

            // is this loop done? (don't test for an end at an \s or \s# because we want to
            // subsume any subheading within this chunk and it's first following verse)
			if (bIsChapterMkrWithin || bIsVerseMkrWithin || bIsSubheadingMkrWithin)
			{
				// we've found the start of the next chapter or subheading or verse; so
				// the instance immediately preceding is the ending instance for the chunk
				endsAt = foundIndex - 1;
				return TRUE;
			}
			// start of next chapter or start of first verse of current chapter wasn't found, so
			// prepare to iterate
			index = foundIndex + 1;
			if (index > endIndex)
			{
				// array end was reached
				bReachedEndOfArray = TRUE;
				endsAt = endIndex;
				return TRUE;
			}
			// search for the next non-empty m_markers member
			foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		} // end of loop: while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		// on exit, we didn't find a new verse before getting to the end, so we know where
		// the end must be and we can set it here and return TRUE (actually, control
		// shouldn't ever get here, but no harm in playing safe)
		endsAt = endIndex;
		return TRUE;
	}
	else
	{
        // \v is not stored in the m_markers which stores \c, so scan across the start of
        // chapter material until the first verse is found and exit this block at that
        // point
		index++;
		if (index > endIndex)
		{
			// we are done
			endsAt = endIndex;
			return TRUE;
		}
		int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		{
			// get the m_markers content in this instance, put it into markers
			pSrcPhrase = arrP->Item(foundIndex);
			markers = pSrcPhrase->m_markers;

			// check for any marker which indicates the chapterPlusVerse material is ended
			bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

            // is this loop done? (don't test for an end at an \s or \s# because we want to
            // subsume any subheading within this chunk and it's first following verse)
			if (bIsChapterMkrWithin || bIsVerseMkrWithin)
			{
				// we've found the start of the next chapter, or first verse of the same chapter
				index = foundIndex; // update index to this location before breaking
				break;
			}
			// start of next chapter or start of first verse of current chapter wasn't found, so
			// prepare to iterate
			index = foundIndex + 1;
			if (index > endIndex)
			{
				// array end was reached
				bReachedEndOfArray = TRUE;
				endsAt = endIndex;
				return TRUE;
			}
			// search for the next non-empty m_markers member
			foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		} // end of loop: while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
	} // end of else block for test: if (bIsVerseMkrWithin)

    // Once control gets here, we've come to the start of a new chapter, or the start of a
    // new verse (but not the array end, if we came to that, we'd have returned in the code
    // above) - in the last circumstance the end of the first verse is the CSourcePhrase
    // preceding the next CSourcePhrase with a \v or \vn marker that we can find in a new
    // loop below, -- so move forward with that loop and define the chunk's end; in the
    // former circumstance, the preceding instance from where we currently are pointing is
    // the chunk's end
	int end_AtStartOfVerseOrChapter = index;
	if (bIsChapterMkrWithin)
	{
		// exited from the above loop because we came to a chapter \c marker; so can't go
		// further to include verse material
		if (startsAt == end_AtStartOfVerseOrChapter && bCandVonSameInstance)
		{
			// we are done
			endsAt = end_AtStartOfVerseOrChapter;
			return TRUE;
		}
		else if (startsAt == end_AtStartOfVerseOrChapter && !bCandVonSameInstance)
		{
			// we didn't advance at all (highly unexpected) so return FALSE and let the
			// other two functions have a crack at it instead (I don't expect that control
			// will ever go through this block)
			startsAt = -1;
			endsAt = -1;
			return FALSE;
		}
		else
		{
			//  there was progression, so accept what we traversed
			endsAt = end_AtStartOfVerseOrChapter - 1;
			bReachedEndOfArray = FALSE;
			return TRUE;
		}
	}
	else
	{
		index++; // get past the CSourcePhrase instance with the \v in its m_markers
		if (index > endIndex)
		{
			// we are done
			endsAt = endIndex;
			return TRUE;
		}
		bReachedEndOfArray = FALSE;
		int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		bool bHasAdvanced = FALSE;
		while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		{
			// get the m_markers content in this instance, put it into markers
			bHasAdvanced = TRUE;
			pSrcPhrase = arrP->Item(foundIndex);
			markers = pSrcPhrase->m_markers;

            // check for any marker which indicates the chapterPlusVerse material is ended
            // (and we'll have to allow for a second \s or \s# (a subheading) to be
            // encountered too, though unlikely so soon - because if there is one, we'll
            // want to halt and get back to the caller so it can parse the new subheading
            // instance with the GetSubheadingPlusVerseChunk() later on)
			bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
			bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
			//bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
			//bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

			// is this loop done?
			if (bIsChapterMkrWithin || bIsSubheadingMkrWithin || bIsVerseMkrWithin )
			{
				// we've found the start of the next chapter, or start of a new
				// subheading, or start of the second verse of the same chapter
				index = foundIndex; // before breaking, update index to the location we found
				break;
			}
            // start of next chapter or a subheading or start of second verse of current
            // chapter wasn't found, so prepare to iterate
			index = foundIndex + 1;
			if (index > endIndex)
			{
				// array end was reached
				bReachedEndOfArray = TRUE;
				endsAt = endIndex;
				return TRUE;
			}
			// search for the next non-empty m_markers member
			foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		} // end of loop: while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)

		// Test for non-advance (it would happen if there was no non-empty m_markers ahead
		// - as would be the case for the last verse of the last chapter). In such a case,
		// we just return FALSE, and let the ensuing GetVerseChunk() grab the final material.
		if (!bHasAdvanced && (end_AtStartOfVerseOrChapter == startsAt))
		{
			// no advance (that is, neither in first loop nor the second)
			startsAt = -1;
			endsAt = -1;
			return FALSE;
		}
        // We got over a verse to the start of the next, or to the start of a new chapter
        // or the start of a (new) subheading: no matter which was the case, the end of the
        // chunk we are delineating is at the previous index value
		endsAt = --index;
	} // end of else block for test: if (bIsChapterMkrWithin)
	return TRUE;
}

// Check if the start of arr contains material belonging to stuff which is a subheading,
// that is, \s or \s1 or \s2 etc \c occurs, and going as far as the end of the first verse
// which follows (this ensures the chunk has a verse number or verse range). We do it this
// way because if there is \r material after the subheading, it will be automatically
// included. Because of the variations possible, it is better to go from the \s until the
// end of the first verse, then it doesn't matter what other content is in that chunk.
// Return the index values for the CSourcePhrase instances which lie at the start and end
// of the subheading-plus-verse chunk and return TRUE, if we do not succeed in delineating
// any such span, return FALSE (and in that case, startsAt and endsAt values are undefined
// - I'll probably set them to -1 whenever FALSE is returned)
bool GetSubheadingPlusVerseChunk(SPArray* arrP, int& startsAt, int& endsAt)
{
	int count = arrP->GetCount();
	int endIndex = count - 1;
	int index = startsAt;
	wxString markers;
	CSourcePhrase* pSrcPhrase = NULL;
	if (count == 0 || (count > 0 && startsAt >= count))
	{
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// does the starting position within arr have subheading material, such as \s or \s#
	// etc? this stuff is in normalOrMinorMkrs (i.e. subheading markers)
	pSrcPhrase = arrP->Item(index);
	markers = pSrcPhrase->m_markers;
	if (markers.IsEmpty())
	{
        // we don't expect this, because the start of a subheading should commence with as
        // \s or \s1 etc from the normalOrMinorMkrs, so return FALSE so that the caller
        // won't advance the starting location for the next test (for a verse chunk)
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// m_markers has content, so check what might be in it
	bool bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
	//bool bIsMajorOrSeriesMkrWithin = IsSubstringWithin(majorOrSeriesMkrs, markers);
	//bool bIsRangeOrPsalmMkrWithin = IsSubstringWithin(rangeOrPsalmMkrs, markers);
	bool bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
	//bool bIsParallelPassageHeadMkrWithin = IsSubstringWithin(parallelPassageHeadMkrs, markers);
	bool bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

	bool bIsSubheadingPlusVerseChunk = FALSE;
    // it's material from \s or \s1 or some other \s# marker, and we want everything up to
    // the end of the first verse
	if (bIsSubheadingMkrWithin)
	{
		bIsSubheadingPlusVerseChunk = TRUE;
	}
	else
	{
		bIsSubheadingPlusVerseChunk = FALSE;
	}
	if (!bIsSubheadingPlusVerseChunk)
	{
		// return FALSE, it might be just a verse without any other non-verse marker preceding,
		// and we check for that next if this function returns FALSE
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}

    // we are in a subheadingPlusVerseChunk; so look ahead until we come to a CSourcePhrase
    // instance with \v in it's m_markers (it may be in the same m_markers that \s is in,
    // or it may be later on, depending on whether or not there is subheading and/or other
    // stuff before the verse commences). Once we have a CSourcePhrase with a \v or \vn,
    // search ahead to the start of the next instance with either \c or \v (or \vn) - and
    // then the end of the chunk is the previous CSourcePhrase instance to that one. Once
    // we are past any subheading material, we want each chunk to just have a single verse
    // in it, regardless of how much other stuff there might be as well between the
    // subheading and the first verse which follows.
	bool bReachedEndOfArray = FALSE;
	if (bIsVerseMkrWithin)
	{
		// the \s or \s# and the \v are stored on the one CSourcePhrase, so get to the end
		// of the verse in the second loop below (note: the only way \s and \v can be on
		// the same CSourcePhrase is for \s to have no text content - we don't expect that
		// to be the case, but it can't be guaranteed so we have to handle the possibility)
		;
	}
	else
	{
        // \v or \vn is not stored in the m_markers which stores \s or \s#, so scan across
        // the the subheading material (and anything else which is non-verse material)
        // until the start of the first verse is found and exit this block at that point
        // (the loop which then follows this one will get us to the end of that verse we've
        // come to here in this loop)
		index++;
		if (index > endIndex)
		{
			// we are done
			endsAt = endIndex;
			return TRUE;
		}
		int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		{
			// get the m_markers content in this instance, put it into markers
			pSrcPhrase = arrP->Item(foundIndex);
			markers = pSrcPhrase->m_markers;

			// check for any marker which indicates the subheadingPlusVerse material is
			// ended - that could be a verse, or another subheading, or a chapter marker
			// comes next - test for these
			bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
			bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
			//bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
			//bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

			// is this loop done? (we expect to come to a verse, but for data which was
			// just USFMs without any text content, we can indeed have \c \s and \v all in
			// the one CSourcePhrase instance's m_markers member)
			if (bIsVerseMkrWithin || bIsChapterMkrWithin || bIsSubheadingMkrWithin)
			{
				// we've found the start of the next chapter, or the start of a new
				// subheading or the start of a verse (typically the start of a verse)
				index = foundIndex; // update index to this location before breaking
				break;
			}
			// start of next chapter or start of a new subheading, or start of a verse of
			// the current chapter wasn't found, so prepare to iterate
			index = foundIndex + 1;
			if (index > endIndex)
			{
				// array end was reached
				bReachedEndOfArray = TRUE;
				endsAt = endIndex;
				return TRUE;
			}
			// search for the next non-empty m_markers member
			foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		} // end of loop: while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
	} // end of else block for test: if (bIsVerseMkrWithin)

    // Once control gets here, we've come to the start of a new verse, or the start of a
    // new chapter or subheading, (but not the array end, if we came to that, we'd have
    // returned in the code above). If, above, we found a verse start, then we are still at
    // that index, and have to search forward for the end of the verse. Otherwise, the
    // index we are pointing at starts a chapter or new subheading and so we can't look
    // further ahead -- but either of those two would be an unusual circumstance.
	int end_AtStartOfVerseOrSubheadingOrChapter = index;
	if (bIsChapterMkrWithin || bIsSubheadingMkrWithin)
	{
		// exited from the above loop because we came to a chapter \c marker or a \s or
		// \s# subheading marker; so can't go further to try include a verse's material
		if (startsAt == end_AtStartOfVerseOrSubheadingOrChapter)
		{
			// we didn't advance - so don't try, we'll let our GetVerseChunk() do any
			// advancing in such a special circumstance, and we'll try that next if we
			// return FALSE
			startsAt = -1;
			endsAt = -1;
			return FALSE;
		}
		else
		{
			// we progressed, so accept what we traversed
			endsAt = end_AtStartOfVerseOrSubheadingOrChapter - 1;
			bReachedEndOfArray = FALSE;
			return TRUE;
		}
	}
	else
	{
		index++; // get past the CSourcePhrase instance with the \v or \vn in its m_markers
		if (index > endIndex)
		{
			// we are done
			endsAt = endIndex;
			return TRUE;
		}
		bReachedEndOfArray = FALSE;
		int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		bool bHasAdvanced = FALSE;
		while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		{
			// get the m_markers content in this instance, put it into markers
			bHasAdvanced = TRUE;
			pSrcPhrase = arrP->Item(foundIndex);
			markers = pSrcPhrase->m_markers;

			// check for any marker which indicates the subheadingPlusVerse material is ended
			bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
			bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers); // test
																// for a 2nd subheading ahead
			//bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
			//bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

			// is this loop done?
			if (bIsChapterMkrWithin || bIsSubheadingMkrWithin || bIsVerseMkrWithin)
			{
				// we've found the start of the next chapter, or start of a (new)
				// subheading, or the start of the second verse of the same chapter
				index = foundIndex; // update index to this location before breaking
				break;
			}
            // start of next chapter or start of a new subheading or start of the second
            // verse of current chapter wasn't found, so prepare to iterate
			index = foundIndex + 1;
			if (index > endIndex)
			{
				// array end was reached
				bReachedEndOfArray = TRUE;
				endsAt = endIndex;
				return TRUE;
			}
			// search for the next non-empty m_markers member
			foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
		} // end of loop: while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)

		// Test for non-advance (it would happen if there was no non-empty m_markers ahead
		// - as would be the case for the last verse of the last chapter). In such a case,
		// we just return FALSE, and let the ensuing GetVerseChunk() grab the final material.
		if (!bHasAdvanced && (end_AtStartOfVerseOrSubheadingOrChapter == startsAt))
		{
			// no advance (that is, neither in first loop nor the second)
			startsAt = -1;
			endsAt = -1;
			return FALSE;
		}
        // We got over a verse to the start of the next, or to the start of a new chapter
        // or subheading - so no matter which was the case, the end of the chunk we are
        // delineating is at the previous index value
		endsAt = --index;
	} // end of else block for test: if (bIsChapterMkrWithin)
	return TRUE;
}

// Check if the start of arr contains material belonging to stuff which is a (next) verse,
// that is, \v or \vn occurs, and going as far as the end of that verse. (Either a chapter
// start or subheading start or a new verse or end of the array follows, for our purposes.)
// This ensures the chunk has a verse number or verse range. Return the index values for
// the CSourcePhrase instances which lie within the verse and return TRUE, but if we do not
// succeed in delineating any such span, return FALSE (and in that case, startsAt and
// endsAt values are undefined - I'll probably set them to -1 whenever FALSE is returned).
//
// NOTE: if no SFM or USFM is present, verse chunks can't be delineated and in that
// circumstance we return FALSE, and the caller will detect that all 3 functions returned
// FALSE within one iteration and then itself return FALSE (indicating we have plain text
// data without SFMs or USFMs originally - in which case our milestoning pre-processing
// won't work, and any support of merging edited new source text will have to use a limit
// value large enough to encompass the largest number of words added as a block in the
// editing operation done by the user -- may have to ask the user for a manually supplied
// value for the limit parameter in that circumstance.
bool GetVerseChunk(SPArray* arrP, int& startsAt, int& endsAt)
{
	int count = arrP->GetCount();
	int endIndex = count - 1;
	int index = startsAt;
	wxString markers;
	CSourcePhrase* pSrcPhrase = NULL;
	if (count == 0 || (count > 0 && startsAt >= count))
	{
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// Does the starting position within arr have verse material? that is, \v or \vn ?
	// This stuff is in normalOrMinorMkrs (i.e. subheading markers, \s or \s1 or \s2 or
	// \s3 or \s4)
	pSrcPhrase = arrP->Item(index);
	markers = pSrcPhrase->m_markers;
	if (markers.IsEmpty())
	{
		// we don't expect this, because we get here only if there was not a chapter or
		// subheading previously followed by a verse; so we should only be pointing at the
		// start of a verse
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}
	// m_markers has content, so check what might be in it
	bool bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
	//bool bIsMajorOrSeriesMkrWithin = IsSubstringWithin(majorOrSeriesMkrs, markers);
	//bool bIsRangeOrPsalmMkrWithin = IsSubstringWithin(rangeOrPsalmMkrs, markers);
	bool bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
	//bool bIsParallelPassageHeadMkrWithin = IsSubstringWithin(parallelPassageHeadMkrs, markers);
	bool bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

	bool bIsVerseChunk = FALSE; // initialize
    // it's gunna be material from \v or \vn, and we want everything up to the end of the
    // next verse or to the next chapter start or start of a subheading
	if (bIsVerseMkrWithin)
	{
		bIsVerseChunk = TRUE;
	}
	else
	{
		bIsVerseChunk = FALSE;
	}
	if (!bIsVerseChunk)
	{
		// return FALSE, this should never happen
		startsAt = -1;
		endsAt = -1;
		return FALSE;
	}

	// We are in a verseChunk; so look ahead until we come to a CSourcePhrase with a
	// non-empty m_markers which has a chapter marker, or subheading marker, or another
	// verse marker, or we come to the array end
	bool bReachedEndOfArray = FALSE;
	index++; // get past the CSourcePhrase with the start-of-verse \v or \vn marker
	int foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);

	// handle the last verse of the final chapter
	if (foundIndex == wxNOT_FOUND)
	{
		endsAt = endIndex;
		return TRUE;
	}
	while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
	{
		// get the m_markers content in this instance, put it into markers
		pSrcPhrase = arrP->Item(foundIndex);
		markers = pSrcPhrase->m_markers;

		// check for any marker which indicates the subheadingPlusVerse material is ended
		bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
		bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
		//bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
		//bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);
		bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

		// is this loop done?
		if (bIsChapterMkrWithin || bIsSubheadingMkrWithin || bIsVerseMkrWithin)
		{
			// we've found the start of the next chapter, or start of a
			// subheading, or the start of the next verse of the same chapter
			index = foundIndex; // update index to this location before breaking
								// (we decrement it by 1 below, outside of the loop)
			break;
		}
        // start of next chapter or start of a subheading or start of the next
        // verse of current chapter wasn't found, so prepare to iterate
		index = foundIndex + 1;
		if (index > endIndex)
		{
			// array end was reached
			endsAt = endIndex;
			return TRUE;
		}
		// search for the next non-empty m_markers member
		foundIndex = GetNextNonemptyMarkers(arrP, index, bReachedEndOfArray);
	} // end of loop: while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)

	// if the end of the array was reached, we have to set index there etc
	if (bReachedEndOfArray)
	{
		endsAt = endIndex;
		return TRUE;
	}

    // We are not at the end of the array, but we got over a verse to the start of the
    // next, or to the start of a new chapter or to a subheading - so no matter which was
    // the case, the end of the chunk we are delineating is at the previous index value
	endsAt = --index;
	return TRUE;
}

// return the chapter:verse or chapter:verse_range string from the first non-empty
// m_chapterVerse member encountered in the passed in range of CSourcPhrase instances
wxString FindVerseReference(SPArray* arrP, int startFrom, int endAt)
{
	int index;
	wxString strRef = _T("0:0");
    // the m_chapterVerse member is non-empty only after a \v or \vn has been seen, that is
    // only within a verse - for any \c \ms \mr \s etc material prior to the verse, the
    // m_chapterVerse member is not set on any of the CSourcePhrase instances tokenized
    // from that material - so we only would get 0:0 needing to be returned if there was
    // such chapter initial material defined but no verse following - and data structured
    // that way ought not to occur (but we must allow for it happening, hence 0:0 returned)
	for (index = startFrom; index <= endAt; index++)
	{
		CSourcePhrase* pSrcPhrase = arrP->Item(index);
		if (!pSrcPhrase->m_chapterVerse.IsEmpty())
		{
			strRef = pSrcPhrase->m_chapterVerse;
			break;
		}
	}

	// didn't find an m_chapVerse member which was non-empty, so send back the default
	// "0:0" string
	return strRef;
}

void InitializeNonVerseChunk(SfmChunk* pStruct)
{
	pStruct->type = unknownChunkType;
	pStruct->strChapter.Empty();
	pStruct->nChapter = -1;
	pStruct->strDelimiter.Empty();
	pStruct->strStartingVerse.Empty();
	pStruct->nStartingVerse = -1;
	pStruct->charStartingVerseSuffix = _T('\0');
	pStruct->strEndingVerse.Empty();
	pStruct->nEndingVerse = -1;
	pStruct->charEndingVerseSuffix = _T('\0');
	pStruct->startsAt = -1;
	pStruct->endsAt = -1;
	pStruct->bContainsText = TRUE;
}

// Test whether or not the chunk of CSourcePhrase instances delineated by the range
// [startsAt,endsAt] contains any text in their m_key members - only need a single
// character to force a return of TRUE, return FALSE if **ALL** m_key members are empty
bool DoesChunkContainSourceText(SPArray* pArray, int startsAt, int endsAt)
{
	int index;
	for (index = startsAt; index <= endsAt; index++)
	{
		CSourcePhrase* pSrcPhrase = pArray->Item(index);
		if (!pSrcPhrase->m_key.IsEmpty())
		{
			// at least one has some text content, so return TRUE
			return TRUE;
		}
	}
	// all are empty
	return FALSE;
}

// Copies CSourcePhrase pointers from arr to subArray (clears the latter first) from index
// value fromIndex up to and including toIndex. Both fromIndex and toIndex must lie within
// the bounds of the passed in arr parameter. When done using the copied pointers, they
// should not be deleted from the heap since they are copies only, and so create no new
// memory chunks.
void CopySubArray(SPArray& arr, int fromIndex, int toIndex, SPArray& subArray)
{
	wxASSERT(fromIndex >= 0 && toIndex < (int)arr.GetCount());
	subArray.Clear();
	int index;
	CSourcePhrase* pSrcPhrase = NULL;
	for (index = fromIndex; index <= toIndex; index++)
	{
		pSrcPhrase = arr.Item(index);
		subArray.Add(pSrcPhrase);
	}
}

// Similar to the above CopySubArray(), except that the instances are deep copies, and the
// other difference is that they are appended to the passed in SPList, pList, -- and pList
// does not have to be empty, and the function does not empty it, it merely appends to
// whatever is already there. (Intended use is direct copying of arrNew CSourcePhrase
// instances from Importing Edited Source Text to arrOld, when no recursion is needed.)
void CopyToList(SPArray& arr, int fromIndex, int toIndex, SPList* pList)
{
	wxASSERT(fromIndex >= 0 && toIndex < (int)arr.GetCount());
	int index;
	CSourcePhrase* pSrcPhrase = NULL;
	for (index = fromIndex; index <= toIndex; index++)
	{
		pSrcPhrase = arr.Item(index);
		CSourcePhrase* pDeepCopy = new CSourcePhrase(*pSrcPhrase);
		pDeepCopy->DeepCopy();
		pList->Append(pDeepCopy);
	}
}

// In this function, "Ones" are CSourcePhrase instances - the "old" ones are from the
// document, and the "new" ones are from the array of instances being imported for merger;
// or chapter markers \c, or verse markers \v, depending on what we pass in.
// Return TRUE if the sizes differ by mor than 150%
bool AreInventoriesDisparate(int oldOnesCount, int newOnesCount)
{
	int maxItems = wxMax(oldOnesCount, newOnesCount);
	int minItems = wxMin(oldOnesCount, newOnesCount);
	int second = (int)DISPARATE_SIZES_NUMERATOR * minItems;
	int first =  (int)DISPARATE_SIZES_DENOMINATOR * maxItems;
	if (first > second)
		return TRUE;
	else
		return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE if the array could be chunked because markers were present,
///                         FALSE if it could not be chunked (that would almost certainly
///                         be because the original Plain Text source text from which the
///                         tokenized CSourcePhrase instances were obtained, did not have
///                         any USFM or SFM markup)
/// \param  pInputArray ->  an array of CSourcePhrase pointers (it may be a subarray
///                         extracted from a larger array, it can't be assumed to be
///                         the whole document - but almost certainly always will be, and
///                         so far Adapt It only uses it this way)
/// \param  pChunkSpecs <-  an array of pointers to SfmChunk structs
/// \param  countOfChapters <- returns a count of how many \c markers there were
/// \param  countOfVerses   <- returns a count of how many \v (or \vn) markers there were
/// \remarks
/// The SfmChunk struct has the following structure, & is declared in MereUpdatedSrc.h
/// and comments there describe what each member field is for:
///struct SfmChunk {
///	SfmChunkType		type;
///	wxString			strChapter;
///	int					nChapter;
///	int					strDelimiter;
///	wxString			strStartingVerse;
///	int					nStartingVerse;
///	wxChar				charStartingVerseSuffix;
///	wxString			strEndingVerse;
///	int					nEndingVerse;
///	wxChar				charEndingVerseSuffix;
/// int					startsAt;
///	int					endsAt;
///	bool				bContainsText;
///};
/// SfmChunkType is an enum used for typing each SfmChunk instance, with values from the
/// following: 	bookInitialChunk, introductionChunk, chapterStartChunk, subheadingChunk,
/// verseChunk
/// Note that not every chunk will pertain to a verse of scripture, some will have no
/// verse information - actually, all of them, except verseChunk; each chunk indexes into
/// the passed in pInputArray array of CSourcePhrase instances (in document order,
/// although the first in the array does not necessarily have a m_nSequNumber value of 0)
bool AnalyseSPArrayChunks(SPArray* pInputArray, wxArrayPtrVoid* pChunkSpecs,
							 int& countOfChapters, int& countOfVerses)
{
	bool bCannotChunkThisArray = TRUE; // it might turn out to have no (U)SFMs in it
	pChunkSpecs->Clear(); // ensure it starts empty
	int nStartsAt = 0;
	int nEndsAt = wxNOT_FOUND;
	int count = pInputArray->GetCount();
	int endIndex = count - 1;
	int lastSuccessfulEndsAt = wxNOT_FOUND; // -1
	if (count == 0)
	{
		return FALSE;
	}

	// first, try for a once-only initial chunk of book-initial material
	bool bHasBookInitialChunk = GetBookInitialChunk(pInputArray, nStartsAt, nEndsAt);
	if (bHasBookInitialChunk)
	{
		bCannotChunkThisArray = FALSE; // there are (U)SFMs, and also book-initial material

		// create on the heap a SfmChunk struct, populate it and store in pChunkSpecs
		SfmChunk* pSfmChunk = new SfmChunk;
		InitializeNonVerseChunk(pSfmChunk);
		pSfmChunk->type = bookInitialChunk;
		pSfmChunk->startsAt = nStartsAt;
		pSfmChunk->endsAt = nEndsAt;
		pSfmChunk->bContainsText = DoesChunkContainSourceText(pInputArray, nStartsAt, nEndsAt);
		pChunkSpecs->Add(pSfmChunk);

		// consume this chunk, by advancing where we start from next
		lastSuccessfulEndsAt = nEndsAt;
		nStartsAt = lastSuccessfulEndsAt + 1;
		nEndsAt = wxNOT_FOUND;
		if (nStartsAt > endIndex)
		{
			// all we've got is book-initial material
			return TRUE;
		}
	}
	else
	{
		// don't advance, so now we need to try below for a chunk containing introduction
		// material
		nStartsAt = 0;
		nEndsAt = wxNOT_FOUND;
	}

	// second, try for an introduction chunk
	bool bHasIntroductionChunk = GetIntroductionChunk(pInputArray, nStartsAt, nEndsAt);
	if (bHasIntroductionChunk)
	{
		bCannotChunkThisArray = FALSE; // there are (U)SFMs, and also book-initial material

		// create on the heap a SfmChunk struct, populate it and store in pChunkSpecs
		SfmChunk* pSfmChunk = new SfmChunk;
		InitializeNonVerseChunk(pSfmChunk);
		pSfmChunk->type = introductionChunk;
		pSfmChunk->startsAt = nStartsAt;
		pSfmChunk->endsAt = nEndsAt;
		pSfmChunk->bContainsText = DoesChunkContainSourceText(pInputArray, nStartsAt, nEndsAt);
		pChunkSpecs->Add(pSfmChunk);

		// consume this chunk, by advancing where we start from next
		lastSuccessfulEndsAt = nEndsAt;
		nStartsAt = lastSuccessfulEndsAt + 1;
		nEndsAt = wxNOT_FOUND;
		if (nStartsAt > endIndex)
		{
			// all we've got is any book-initial material found early, plus this
			// introduction material
			return TRUE;
		}
	}
	else
	{
		// don't advance, so now we need to try below for a chunk containing chapter-starting
		// material - such as \c marker, maybe \ms and/or \mr or \r, but excluding a subheading
		if (lastSuccessfulEndsAt != wxNOT_FOUND)
		{
			nStartsAt = lastSuccessfulEndsAt + 1;
		}
		else
		{
			nStartsAt = 0;
		}
		nEndsAt = wxNOT_FOUND;
	}

	// now check in case there is some other material (eg. a \ms section) prior to the
	// first chapter (or verse, if there is no chapter marker in the data); note, if there
	// are no verse or chapter markers in the source text, then this scan for a chapter
	// marker etc will run to the end of the source text -- we must test for that and if
	// so, make sure the nEndsAt is set to the word count less 1, and other variables in
	// agreement with this
	bool bHasPreFirstChapterChunk = GetPreFirstChapterChunk(pInputArray, nStartsAt, nEndsAt);
	if (bHasPreFirstChapterChunk)
	{
		bCannotChunkThisArray = FALSE; // there are (U)SFMs in this chunk

		// create on the heap a SfmChunk struct, populate it and store in pChunkSpecs
		SfmChunk* pSfmChunk = new SfmChunk;
		InitializeNonVerseChunk(pSfmChunk);
		pSfmChunk->type = preFirstChapterChunk;
		pSfmChunk->startsAt = nStartsAt;
		pSfmChunk->endsAt = nEndsAt;
		pSfmChunk->bContainsText = DoesChunkContainSourceText(pInputArray, nStartsAt, nEndsAt);
		pChunkSpecs->Add(pSfmChunk);

		// consume this chunk, by advancing where we start from next
		lastSuccessfulEndsAt = nEndsAt;
		nStartsAt = lastSuccessfulEndsAt + 1;
		nEndsAt = wxNOT_FOUND;
		if (nStartsAt > endIndex)
		{
			// all we've got is any book initial & introduction material found earlier, plus this
			// pre-first-chapter material (which might be rather large if there are no \c
			// or \v in the document -- so use limit = -1)
			return TRUE;
		}
	}
	else
	{
		// don't advance, so now we need to try below for a chunk containing chapter-starting
		// material - such as \c marker, etc, but excluding a subheading
		if (lastSuccessfulEndsAt != wxNOT_FOUND)
		{
			nStartsAt = lastSuccessfulEndsAt + 1;
		}
		else
		{
			nStartsAt = 0;
		}
		nEndsAt = wxNOT_FOUND;
	}

	countOfChapters = 0;
	countOfVerses = 0;

	// Now we analyse one or more chapters until done. This will involve smaller chunks -
	// largest should be a verse. The chunks, in order of occurrence that we will search
	// for, in a loop, are:
	// 1. chapterPlusVerseChunk, 2. subheadingPlusVerseChunk, 3. verseChunk
	// Failure to get the first, the second is tried, failure to get the second, the third
	// is tried (and must succeed); iterate the loop, after consuming each successfully
	// delineated subspan, until no more data is available
	//bool bHasChapterPlusVerseChunk; // set but unused
	//bool bHasSubheadingPlusVerseChunk; // set but unused
	//bool bHasVerseChunk; // set but unused

    // this loop must consume the rest of the material to be analysed, so the
    // lastSuccessfulEndsAt local variable enables an appropriate starting value to be
    // computed when one of the three following calls within the loop fail to obtain a
    // chunk - on failure we need to reestablish the initial value and retry with the next
    // function of the three - and the third function, which gets a verse chunk, must
    // always succeed until the end is arrived at (that is, all three must not fail while
	// there is still material to be processed) except when there is no USFM or SFM markup
	// at all, in which case we must avoid an infinite loop and exit FALSE immediately
	while (lastSuccessfulEndsAt <= endIndex)
	{
		// first, try for a ChapterPlusVerse chunk; if that fails, try instead for a
		// SubheadingPlusVerse chunk; if it succeeds, advance the starting location using
		// the lastSuccessfulEndsAt value, and iterate the loop
		//bHasChapterPlusVerseChunk = FALSE;
		//bHasSubheadingPlusVerseChunk = FALSE;
		//bHasVerseChunk = FALSE;
#if defined(_DEBUG)
//		if (lastSuccessfulEndsAt == 71)
//		{
			// do one empty one, at 72 for verse 13, and then at 73 find out why it makes
			// one which is [73,74] instead of two: [73,73] and [74,74]
//			int halt_here = 1;
//		}
#endif
		if (lastSuccessfulEndsAt != wxNOT_FOUND)
		{
			nStartsAt = lastSuccessfulEndsAt + 1;
		}
		else
		{
			nStartsAt = 0;
		}
		if (lastSuccessfulEndsAt >= endIndex)
		{
			// there's no more material to be processed
			return TRUE;
		}
		nEndsAt = wxNOT_FOUND;
		bool bHasChapterPlusVerseChunk = GetChapterPlusVerseChunk(pInputArray, nStartsAt, nEndsAt);
		if (bHasChapterPlusVerseChunk)
		{
			bCannotChunkThisArray = FALSE; // there are (U)SFMs, \c is first, and there may be
							// other SFMs, as well as a verse's material

			// create on the heap a SfmChunk struct, populate it and store in pChunkSpecs
			SfmChunk* pSfmChunk = new SfmChunk;
			InitializeNonVerseChunk(pSfmChunk);
			pSfmChunk->type = chapterPlusVerseChunk;
			pSfmChunk->startsAt = nStartsAt;
			pSfmChunk->endsAt = nEndsAt;
			pSfmChunk->bContainsText = DoesChunkContainSourceText(pInputArray, nStartsAt, nEndsAt);
			wxString strChapterVerseRef = FindVerseReference(pInputArray, nStartsAt, nEndsAt);
			// parse and store the chapter/verse reference information (note: "0:0"  may
			// have been returned)
			bool bValidReference = AnalyseChapterVerseRef(strChapterVerseRef, pSfmChunk->strChapter,
									pSfmChunk->nChapter, pSfmChunk->strDelimiter,
									pSfmChunk->strStartingVerse, pSfmChunk->nStartingVerse,
									pSfmChunk->charStartingVerseSuffix, pSfmChunk->strEndingVerse,
									pSfmChunk->nEndingVerse, pSfmChunk->charEndingVerseSuffix);
			if (!bValidReference)
			{
				pSfmChunk->strChapter = _T("0");
				pSfmChunk->nChapter = 0;
				pSfmChunk->strDelimiter.Empty();
				pSfmChunk->strStartingVerse = _T("0");
				pSfmChunk->nStartingVerse = 0;
				pSfmChunk->charStartingVerseSuffix = _T('\0');
				pSfmChunk->strEndingVerse = _T("0");
				pSfmChunk->nEndingVerse = 0;
				pSfmChunk->charEndingVerseSuffix = _T('\0');
			}
			else
			{
				if (!pSfmChunk->strChapter.IsEmpty())
				{
					countOfChapters++;
				}
				if (!pSfmChunk->strStartingVerse.IsEmpty())
				{
					countOfVerses++;
				}
			}
			pChunkSpecs->Add(pSfmChunk);

			// consume this chunk, by advancing where we start from next
			lastSuccessfulEndsAt = nEndsAt;
		}
		else
		{
			// Failure, so don't advance. So retry here for a subheadingPlusVerse chunk
			nStartsAt = lastSuccessfulEndsAt + 1;
			nEndsAt = wxNOT_FOUND;
			bHasChapterPlusVerseChunk = FALSE;
			bool bHasSubheadingPlusVerseChunk = GetSubheadingPlusVerseChunk(pInputArray, nStartsAt, nEndsAt);
			if (bHasSubheadingPlusVerseChunk)
			{
				bCannotChunkThisArray = FALSE; // there are (U)SFMs, \s or \s1 etc is first,
							// and there may be other SFMs, as well as a verse's material

				// create on the heap a SfmChunk struct, populate it and store in pChunkSpecs
				SfmChunk* pSfmChunk = new SfmChunk;
				InitializeNonVerseChunk(pSfmChunk);
				pSfmChunk->type = subheadingPlusVerseChunk;
				pSfmChunk->startsAt = nStartsAt;
				pSfmChunk->endsAt = nEndsAt;
				pSfmChunk->bContainsText = DoesChunkContainSourceText(pInputArray, nStartsAt, nEndsAt);
				wxString strChapterVerseRef = FindVerseReference(pInputArray, nStartsAt, nEndsAt);
				// parse and store the chapter/verse reference information (note: "0:0"  may
				// have been returned)
				bool bValidReference = AnalyseChapterVerseRef(strChapterVerseRef, pSfmChunk->strChapter,
										pSfmChunk->nChapter, pSfmChunk->strDelimiter,
										pSfmChunk->strStartingVerse, pSfmChunk->nStartingVerse,
										pSfmChunk->charStartingVerseSuffix, pSfmChunk->strEndingVerse,
										pSfmChunk->nEndingVerse, pSfmChunk->charEndingVerseSuffix);
				if (!bValidReference)
				{
					pSfmChunk->strChapter = _T("0");
					pSfmChunk->nChapter = 0;
					pSfmChunk->strDelimiter.Empty();
					pSfmChunk->strStartingVerse = _T("0");
					pSfmChunk->nStartingVerse = 0;
					pSfmChunk->charStartingVerseSuffix = _T('\0');
					pSfmChunk->strEndingVerse = _T("0");
					pSfmChunk->nEndingVerse = 0;
					pSfmChunk->charEndingVerseSuffix = _T('\0');
				}
				else
				{
					if (!pSfmChunk->strStartingVerse.IsEmpty())
					{
						countOfVerses++;
					}
				}
				pChunkSpecs->Add(pSfmChunk);

				// consume this chunk, by advancing where we start from next
				lastSuccessfulEndsAt = nEndsAt;
			}
			else
			{
				// Failure, so don't advance. So retry here for a verse chunk
				nStartsAt = lastSuccessfulEndsAt + 1;
				nEndsAt = wxNOT_FOUND;
				bHasSubheadingPlusVerseChunk = FALSE;
				bool bHasVerseChunk = GetVerseChunk(pInputArray, nStartsAt, nEndsAt);
				if (bHasVerseChunk)
				{
					bCannotChunkThisArray = FALSE; // there are (U)SFMs, \v or \vn etc is first,
								// and there may be other SFMs (eg. \q1, \q2 etc), as well
								// as the verse's material
					// create on the heap a SfmChunk struct, populate it and store in pChunkSpecs
					SfmChunk* pSfmChunk = new SfmChunk;
					InitializeNonVerseChunk(pSfmChunk);
					pSfmChunk->type = verseChunk;
					pSfmChunk->startsAt = nStartsAt;
					pSfmChunk->endsAt = nEndsAt;
					pSfmChunk->bContainsText = DoesChunkContainSourceText(pInputArray, nStartsAt, nEndsAt);
					wxString strChapterVerseRef = FindVerseReference(pInputArray, nStartsAt, nEndsAt);
					// parse and store the chapter/verse reference information (note: "0:0"  may
					// have been returned)
					bool bValidReference = AnalyseChapterVerseRef(strChapterVerseRef, pSfmChunk->strChapter,
											pSfmChunk->nChapter, pSfmChunk->strDelimiter,
											pSfmChunk->strStartingVerse, pSfmChunk->nStartingVerse,
											pSfmChunk->charStartingVerseSuffix, pSfmChunk->strEndingVerse,
											pSfmChunk->nEndingVerse, pSfmChunk->charEndingVerseSuffix);
					if (!bValidReference)
					{
						pSfmChunk->strChapter = _T("0");
						pSfmChunk->nChapter = 0;
						pSfmChunk->strDelimiter.Empty();
						pSfmChunk->strStartingVerse = _T("0");
						pSfmChunk->nStartingVerse = 0;
						pSfmChunk->charStartingVerseSuffix = _T('\0');
						pSfmChunk->strEndingVerse = _T("0");
						pSfmChunk->nEndingVerse = 0;
						pSfmChunk->charEndingVerseSuffix = _T('\0');
					}
					else
					{
						if (!pSfmChunk->strStartingVerse.IsEmpty())
						{
							countOfVerses++;
						}
					}
					pChunkSpecs->Add(pSfmChunk);

					// consume this chunk, by advancing where we start from next
					lastSuccessfulEndsAt = nEndsAt;
				}
				else
				{
					// Failure, so don't advance. Failure here means all 3 functions
					// failed, so don't loop any longer - in all probability the data
					// doesn't have SFMs or USFMs
					nStartsAt = lastSuccessfulEndsAt + 1; // in case we want to wxLogDebug() this value
					nEndsAt = wxNOT_FOUND;
					bHasVerseChunk = FALSE;
					if (!bHasChapterPlusVerseChunk && !bHasSubheadingPlusVerseChunk && !bHasVerseChunk)
					{
						bCannotChunkThisArray = TRUE;
						break;
					}
				} // end of else block for test: if (bHasVerseChunk)
			} // end of else block for test: if (bHasSubheadingPlusVerseChunk)
		} // end of else block for test: if (bHasChapterPlusVerseChunk)
	} // end of loop for test: while (lastSuccessfulEndsAt < endIndex)
	if (bCannotChunkThisArray)
	{
		// control enters here only if there was no SFM or USFM data anywhere - e.g.
		// literacy data or office-related text data etc

		// TODO? at the moment we just return FALSE, but I think that may be enough - the
		// caller can be given the knowledge of what to do when FALSE is returned
		return FALSE;
	}
	return TRUE;
}

// Return what kind of CSourcePhrase the passed in one is,
// returning one of the following values:
//enum WhatAreYou {
//	singleton,
//	singleton_in_retrans,
//	singleton_matches_new_conjoined,
//	merger,
//	conjoined,
//	manual_placeholder,
//	placeholder_in_retrans
//};
WhatYouAre WhatKindAreYou(CSourcePhrase* pSrcPhrase, CSourcePhrase* pNewSrcPhrase)
{
	if (pSrcPhrase->m_nSrcWords > 1)
	{
		// either a merger or a conjoining
		if (IsFixedSpaceSymbolWithin(pSrcPhrase))
		{
			// you are a fixedspace conjoining of a pair of singletons
			return conjoined;
		}
		else
		{
			// you are a merger
			return merger;
		}
	}
	else
	{
        // m_nSrcWords = 1, so either you are a retranslation (singleton, or placeholder)
        // or a manually inserted placeholder, and if none of those, then you must be a
		// plain jane singleton; but the arrOld singleton may match a newly
		// edited-into-existance fixedspace conjoining in arrNew, so take care of that
		// possibility too
		if (pSrcPhrase->m_bRetranslation)
		{
			if (pSrcPhrase->m_bNullSourcePhrase)
			{
				// you are an auto-inserted placeholder at the end of a retranslation, in
				// order to pad out the require length for the extra adaptations needed
				return placeholder_in_retrans;
			}
			else
			{
				// you are a plane jane singleton within the retranslation, and not a
				// placeholder
				return singleton_in_retrans;
			}
		}
		else if (pSrcPhrase->m_bNullSourcePhrase)
		{
			// you are a manually placed placeholder
			return manual_placeholder;
		}
		else if (IsFixedSpaceSymbolWithin(pNewSrcPhrase))
		{
			return singleton_matches_new_conjoined;
		}
	}
	return singleton; // it's a singleton to singleton matchup
}














