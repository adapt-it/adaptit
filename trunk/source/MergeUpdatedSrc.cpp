/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MergeUpdatedSrc.cpp
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
#include "SourcePhrase.h"
#include "helpers.h"
#include "Adapt_ItDoc.h"
#include "AdaptitConstants.h"
#include "MergeUpdatedSrc.h"

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(SPArray);

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

void InitializeUsfmMkrs()
{
	titleMkrs = _T("\\id \\ide \\h \\h1 \\h2 \\h3 \\mt \\mt1 \\mt2 \\mt3 ");
	introductionMkrs = _T("\\imt \\imt1 \\imt2 \\imt3 \\imte \\is \\is1 \\is2 \\is3 \\ip \\ipi \\ipq \\ipr \\iq \\iq1 \\iq2 \\iq3 \\im \\imi \\imq \\io \\io1 \\io2 \\io3 \\iot \\iex \\ib \\ili \\ili1 \\ili2 \\ie ");
	chapterMkrs = _T("\\c \\cl "); // \ca \ca* \p & \cd omitted, they follow 
								   // \c so aren't needed for chunking
	verseMkrs = _T("\\v \vn "); // \va \va* \vp \vp* omitted, they follow \v
				// and so are not needed for chunking; \vn is a non-standard
				// 'verse number' marker that some people use
	normalOrMinorMkrs = _T("\\s \\s1 \\s2 \\s3 \\s4 ");
	majorOrSeriesMkrs = _T("\\ms \\ms1 \\qa \\ms2 \\ms3 ");
	parallelPassageHeadMkrs = _T("\\r ");
	rangeOrPsalmMkrs = _T("\\mr \\d ");

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
///                            (default is SPAN_LIMIT, currently set to 50) use -1 if no
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
        // possible values passed in are -1, or SPAN_LIMIT (ie. 50), or an explicit
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
            // possible values passed in are -1, or SPAN_LIMIT (ie. 50), or an explicit
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
            // possible values passed in are -1, or SPAN_LIMIT (ie. 50), or an explicit
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
	if (nStartAt > count - 1)
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
///                            (default is SPAN_LIMIT, currently set to 50) use -1 if no
///                            limit is wanted & so all are to be checked. If the
///                            array does not have the limit amount of instances, all are
///                            checked.
/// \remarks
/// The returned array, strArray, contains all of the unique individual words which are in
/// both the old passed in uniqueKeysStr and in the source text keys (with mergers reduced
/// to the individual words for storage in strArray) - collected from the first (or all)
/// of the words in arrNew's CSourcePhrase instances -- exactly how many are used depends
/// on the 4th param, limit, which if absent defaults to SPAN_LIMIT = 50 (CSourcePhrase
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
        // possible values passed in are -1, or SPAN_LIMIT (ie. 50), or an explicit
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
//#ifdef __WXDEBUG__
//	if (oldUniqueWordsCount > 0)
//		wxLogDebug(_T("oldUniqueWordsCount = %d ,  oldUniqueSrcWords:  %s"), oldUniqueWordsCount, oldUniqueSrcWords.c_str());
//	else
//		wxLogDebug(_T("oldUniqueWordsCount = 0 ,  oldUniqueSrcWords:  <empty string>"));
//#endif
	if (oldUniqueWordsCount == 0)
		return 0;
	// compare the initial words from the arrNew array of CSourcePhrase instances with
	// those in oldUniqueSrcWords
	int commonsCount = GetWordsInCommon(arrNew, pSubspan, oldUniqueSrcWords, strArray, limit);
//#ifdef __WXDEBUG__
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
//#endif
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
/// default value of SPAN_LIMIT (set to 50) is suitably large without generating heaps of
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

void MergeUpdatedSourceText(SPList& oldList, SPList& newList, SPList* pMergedList, int limit)
{
	// turn the lists into arrays of CSourcePhrase*; note, we are using arrays to manage
	// the same pointers as the SPLists do, so don't in this function try to delete any of
	// the oldList or newList contents, nor what's in the arrays
	int nStartingSequNum;
	SPArray arrOld;
	SPArray arrNew;
	ConvertSPList2SPArray(&oldList, &arrOld);
	int oldSPCount = oldList.GetCount();
	if (oldSPCount == 0)
		return;
	nStartingSequNum = (arrOld.Item(0))->m_nSequNumber; // store this, to get the 
														// sequence numbers right later
	ConvertSPList2SPArray(&newList, &arrNew);
	int newSPCount = newList.GetCount();
	if (newSPCount == 0)
		return;
	InitializeUsfmMkrs();

	wxArrayPtrVoid* pChunksOld = new wxArrayPtrVoid; // remember to delete contents and 
													 // remove from heap before returning
	bool bSuccessful_Old =  AnalyseSPArrayChunks(&arrOld, pChunksOld);

    // Note: we impose a limit on maximum span size, to keep our algorithms from getting
    // bogged down by having to handle too much data in any one iteration. The limit
    // parameter specifies what to do.
    // If limit is -1 then bogging down potential is to be ignored, and we'll always
    // take the largest span possible. If limit is not explicitly specified in the call,
    // then default to SPAN_LIMIT (currently set in AdaptitConstants.h to 50 -- big enough
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
    
	// set up the top level tuple, beforeSpan and commonSpan will be empty (that is, the
	// tuple[0] and tuple[1] struct pointers will be NULL. Tuple[3] will have the whole
	// extent of both oldSPArray and newSPArray.
	Subspan* tuple[3]; // an array of three pointer-to-Subspan
	tuple[0] = NULL;
	tuple[1] = NULL;
	Subspan* pSubspan = new Subspan;
	tuple[2] = pSubspan;
    // initialize tuple[2] to store an open-ended Subspan pointer spanning the whole
    // extents of arrOld and arrNew -- the subsequent SetEndIndices() call will limit the
    // value, provided limit is not -1
    // In this call: FALSE is bClosedEnd, i.e. it's an open-ended afterSpan
	InitializeSubspan(pSubspan, afterSpan, 0, oldSPCount - 1, 0, newSPCount - 1, FALSE); 
	// update the end indices to more reasonable values, the above could bog processing down
	SetEndIndices(arrOld, arrNew, pSubspan, SPAN_LIMIT); // SPAN_LIMIT is currently 50

    // Pass the top level tuple to the RecursiveTupleProcessor() function. When this next
    // call finally returns after being recursively called possibly very many times, the
    // merging is complete
    RecursiveTupleProcessor(arrOld, arrNew, pMergedList, limit, tuple);

	// Get the CSourcePhrase instances appropriately numbered in the temporary
	// list which is to be returned to the caller
	if (!pMergedList->IsEmpty())
	{
		gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);

        // Call an adjustment helper function which looks for retranslation spans that were
        // partially truncated (at their beginning and/or at their ending), and removes the
        // adaptations in the remainder fragments of such retranslation trucated spans,
        // fixing the relevant flags, and if placeholders need to be removed from the end
        // of a retranslation, does any transfers of ending puncts and/or inline markers
        // back to the last non-placeholder instance
		EraseAdaptationsFromRetranslationTruncations(pMergedList);
	}
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
			SPList::Node* posStart = posLocal;
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
				posStart = posLocal;
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at ONE"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at TWO -- the single-single mismatch"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at THREE"));
//#endif
							return FALSE;
						}	
					}
					else
					{
						// not enough words available, so no match is possible
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at FOUR"));
//#endif
						return FALSE;
					}
				}
			} // end of TRUE block for test: if (bIsFixedspaceConjoined)
			else
			{
                // it's a plain vanilla merger, so get the starting index for it and ensure
                // that lies within arrNew's bounds, if so we assume then that arrNew may
                // have a matching word sequence, so we test for the match
				int newStartIndex = newStartingPos - numWords; // == newIndex - numWords + 1
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at THREE - the merger-sequence mismatch"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at FOUR"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at FIVE"));
//#endif
									return FALSE; // tell the caller not to try another leftwards widening
								}
								else if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
								{
									// it/they are left-associated, so keep it/them with
									// what precedes it/them, which means that it/they
									// belong in beforeSpan, and we can't widen any further
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at SIX"));
//#endif
									return FALSE; // (oldCount and newCount both returned as zero)
								}
								// neither left nor right associated, so handle the
								// same as left-associated
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at SEVEN"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at EIGHT"));
//#endif
								return FALSE;
							}
						}
						else
						{
							// not enough words available
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at NINE"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at TEN"));
//#endif
					return FALSE; // tell the caller not to try another leftwards widening
				}
				else if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
				{
                    // it/they are left-associated, so keep it/them with
                    // what precedes it/them, which means that it/they
                    // belong in beforeSpan, and we can't widen any further
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at ELEVEN"));
//#endif
                    return FALSE; // (oldCount and newCount both returned as zero)
				}
				// neither left nor right associated, so handle the
				// same as left-associated
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at TWELVE"));
//#endif
				return FALSE;
			}

		} // end of TRUE block for test: if (pOldSrcPhrase->m_bNullSourcePhrase)

	} // end of TRUE block for test: if (oldIndex > oldStartAt && newIndex > newStartAt)
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Leftwards ONCE: exiting at THE_END (THIRTEEN)"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at ONE"));
//#endif
		return FALSE;
	}
	int oldIndex = oldStartingPos;
	int newIndex = newStartingPos;
	if (oldIndex < oldEndAt && newIndex < newEndAt)
	{
		pOldSrcPhrase = arrOld.Item(oldIndex);
		pNewSrcPhrase = arrNew.Item(newIndex);

//#ifdef __WXDEBUG__
//		if (oldIndex == 42)
//		{
//			int break_point = 1;
//		}
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at TWO -- the single-single mismatch"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at THREE - the merger-sequence mismatch"));
//#endif
							return FALSE;
						}
					}
					else
					{
						// not enough words available, so no match is possible
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at FOUR"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at FIVE"));
//#endif
						return FALSE;
					}
				}
				else
				{
					// not enough words for a match, so return FALSE
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at SIX"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at SEVEN"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at EIGHT"));
//#endif
									return FALSE; // tell the caller not to try another leftwards widening
								}
								else if (IsRightAssociatedPlaceholder(pNextSrcPhrase))
								{
                                    // it/they are right-associated, so keep it/them with
                                    // what follows it/them, which means that it/they
                                    // belong in at the start of afterSpan, and we can't
                                    // widen commonSpan any further
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at NINE"));
//#endif
									return FALSE; // (oldCount and newCount both returned as zero)
								}
								// neither left nor right associated, so handle the
								// same as right-associated
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at TEN"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at ELEVEN"));
//#endif
								return FALSE;
							}
						} // end of TRUE block for test: if (newIndex + numWords - 1 <= newEndAt)
						else
						{
							// not enough words available
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at TWELVE"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at THIRTEEN"));
//#endif
					return FALSE; // tell the caller not to try another leftwards widening
				}
				else if (IsRightAssociatedPlaceholder(pNextSrcPhrase))
				{
                    // it/they are right-associated, so keep it/them with
                    // what follows it/them, which means that it/they
                    // belong in at the start of afterSpan, and we can't
                    // widen commonSpan any further
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at FOURTEEN"));
//#endif
					return FALSE; // (oldCount and newCount both returned as zero)
				}
				// neither left nor right associated, so handle the
				// same as right-associated
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at FIFTEEN"));
//#endif
				return FALSE;
			}

		} // end of TRUE block for test: if (pOldSrcPhrase->m_bNullSourcePhrase)

	} // end of TRUE block for test: if (oldIndex < oldEndAt && newIndex < newEndAt)
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Rightwards ONCE: exiting at THE_END (SIXTEEN)"));
//#endif
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
//#ifdef __WXDEBUG__
//		wxLogDebug(_T("Composite WIDTH =  %d  for index value  %d"),maxWidth, 0); 
//#endif
		int lastIndexForMax = 0; // initialize 
		int i;
		for (i = 1; i < widthsCount; i++)
		{
//#ifdef __WXDEBUG__
//			wxLogDebug(_T("\nComposite WIDTH =  %d  for index value  %d"),arrWidths.Item(i), i);
//			Subspan* pSub = (Subspan*)arrSubspans.Item(i);
//			wxLogDebug(_T("Subspan's two spans: OLD  start = %d  end = %d ,  NEW  start = %d  end = %d"),
//				pSub->oldStartPos, pSub->oldEndPos, pSub->newStartPos, pSub->newEndPos);
//#endif
			if (arrWidths.Item(i) > maxWidth)
			{	
				// only accept a new value if it is bigger, this way if all are equal
				// size, we'll take the index for the first of them, which is a better
				// idea when trying to work left to right
				maxWidth = arrWidths.Item(i);
				lastIndexForMax = i;
			}
		}
//#ifdef __WXDEBUG__
//			wxLogDebug(_T("lastIndexForMax value = %d"), lastIndexForMax); 
//#endif
		Subspan* pMaxInCommonSubspan = (Subspan*)arrSubspans.Item(lastIndexForMax);
		// delete the rest
		arrWidths.Clear();
		for (i = 0; i < widthsCount; i++)
		{
			// delete all except the one we are returning to the caller
			if (i != lastIndexForMax)
			{
				delete (Subspan*)arrSubspans.Item(i);
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
/// determined that they can be abandoned. After widening, a Subspan is create on the heap
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
/// being 50 (currently)), and for each matchup widening is attempted provided that the
/// matchup index values don't lie within an already found Subspan instance.
/// 
/// The "width" values we store are the sum of counts of the consecutive CSourcePhrase
/// instances in the arrOld limited span within that larger arry, and the consecutive
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

//#ifdef __WXDEBUG__
//	wxLogDebug(_T("GetNextMatchup(): word = %s , oldStartFrom = %d , newStartFrom = %d, oldMatchIndex = %d, newMatchIndex = %d "),
//		word.c_str(), oldStartFrom, newStartFrom, oldMatchIndex, newMatchIndex);
//#endif

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

/* don't think I'll need this one
// Sets the oldEndAt and newEndAt index values, based on the current values passed in for
// oldStartAt and newStartAt, and oldEndAt and newEndAt, using limit and bClosedEnd values
// to control if new end values are calculated and what they should be. When the end of the
// span is closed, the oldEndAt and newEndAt values are known and retained unchanged - they
// are strict limits which must be obeyed in order that our subspans don't overlap
// resulting in doubling up of the same CSourcePhrase instances in the final merged list.
// When the span is open, the end locations are determined by the limit value in
// conjunction with the arrOld and arrNew element counts.
void SetEndIndices(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int newStartAt, int& oldEndAt,
					  int& newEndAt, int limit, bool bClosedEnd)
{
	if (bClosedEnd)
	{
		// use oldEndAt and newEndAt as passed in, they must not be changed
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
			oldEndAt = arrOld.GetCount() - 1;
			newEndAt = arrNew.GetCount() - 1;
		}
		else
		{
			// go as far as we can, up to the limit value, or array end, whichever comes first
			oldEndAt = wxMin((unsigned int)(oldStartAt + limit), (unsigned int)(arrOld.GetCount())) - 1;
			newEndAt = wxMin((unsigned int)(newStartAt + limit), (unsigned int)(arrNew.GetCount())) - 1;
		}
	}
}
*/
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
/// \return                 nothing
/// \param  arrOld      ->  array of old CSourcePhrase instances (may have mergers,
///                         placeholders, retranslation, fixedspace conjoinings as 
///                         well as minimal CSourcePhrase instances)
/// \param  arrNew      ->  array of new CSourcePhrase instances (will only be minimal
///                         CSourcePhrase instances, but could also contain fixedspace
///                         conjoined instances too)
/// \param  pSubspan    ->  the Subspan instance which defines either the new CSourcePhrase
///                         instances which are to replace the old ones for a range of index
///                         values in arrOld, or the old CSourcePhrase instances which are
///                         in common and so will be retained from arrOld
/// \param  pMergedList ->  the list of CSourcePhrase instances being built - when done,
///                         this will become the new version of the document and be laid
///                         out for the user to fill the "holes" with new adaptations
/// \remarks
/// MergeOldAndNew() is called at the bottom of the recursion process, after parent Subspan
/// instances have been successively decomposed to smaller and smaller tuples, finally, at
/// the leaves of the recursion tree we get a Subspan instance which has nothing in common
/// (these can be either beforeSpan or afterSpan types) or all is in common (and every
/// commonSpan is, by definition, one such). When such leaves are reached, merging can be
/// done. The commonSpan merging just copies the commonSpan's pointed-at CSourcePhrase
/// instances from arrOld, appending them to pMergedList; beforeSpan or afterSpan will have
/// nothing in common, and so the CSourcPhrase they point to are copied from arrNew,
/// appending them to pMergedList.
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
		// retain the old ones
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
	delete pSubspan;
//#ifdef __WXDEBUG__
	// track progress in populating the pMergedList
//	wxLogDebug(_T("pMergedList progressive count:  %d  ( compare arrOld %d , arrNew %d )"),
//		pMergedList->GetCount(), arrOld.Count(), arrNew.Count());
//#endif
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
	//wxASSERT(tuple[0] == NULL);
	//wxASSERT(tuple[1] == NULL);
	//wxASSERT(tuple[2] == NULL);
	if (pParentSubspan->spanType == commonSpan)
	{
		// return FALSE, there was no need to make the SetupChildTuple() call for a
		// Subspan we already know is a commonSpan type
		return FALSE;
	}
	Subspan* pCommonSubspan = GetMaxInCommonSubspan(arrOld, arrNew, pParentSubspan, limit);
//#ifdef __WXDEBUG__
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
//#endif

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
		pBeforeSpan = new Subspan;
		if (parentOldStartAt < pCommonSubspan->oldStartPos)
		{
			// the beforeSpan has some content from arrOld, so check the situation in arrNew 
			if (parentNewStartAt < pCommonSubspan->newStartPos)
			{
                // there is content from both arrOld and arrNew for this pBeforeSpan (that
                // means there is potentially some old CSourcePhrase instances to be
                // replaced with ones from the user's edits; we say "potentially" because
                // possibly not all of it would end up being replaced, as there could be
                // smaller subspans within it which are in common, and these would be
                // delineated in the recursing to greater depth
				InitializeSubspan(pBeforeSpan, beforeSpan, parentOldStartAt, 
						pCommonSubspan->oldStartPos - 1, parentNewStartAt, 
						pCommonSubspan->newStartPos - 1, TRUE);
			}
			else
			{
                // There is no content from arrNew to match what is in arrOld here, in that
                // case, set pBeforeSpan's newStartPos and newEndPos values to -1 so the
                // caller will know that the arrNew subspan in this pBeforeSpan is empty
                // (this situation arises when the user's editing of the source phrase text
                // has removed some of it, so the material in arrOld's subspan has to be
                // ignored and nothing from this pBeforeSpan goes into pMergedList)
				InitializeSubspan(pBeforeSpan, beforeSpan, parentOldStartAt, 
						pCommonSubspan->oldStartPos - 1, -1, -1, TRUE);
			}
		} // end of TRUE block for test: if (parentOldStartAt < pCommonSubspan->oldStartPos)
		else
		{
			// there is no content for pBeforeSpan's subspan in arrOld
			if (parentNewStartAt < pCommonSubspan->newStartPos)
			{
                // when there is no content for beforeSpan in arrOld, set pBeforeSpan's
                // oldStartPos and oldEndPos values to -1 so the caller will know this
                // subspan was empty (this means that the user's editing of the source text
                // at this point inserted CSourcePhrase instances into the source text, so
                // they appear in arrNew, so we must add them to pMergedList later)
				InitializeSubspan(pBeforeSpan, beforeSpan, -1, -1, parentNewStartAt,
					pCommonSubspan->newStartPos - 1, TRUE);
			}
			else
			{
                // the pCommonSubspan abutts both the start of the parent subspan in arrOld
                // and the parent subspan in arrNew -- meaning that beforeSpan is empty, so
                // in this case delete it and use NULL as it's pointer
				delete pBeforeSpan;
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
		pAfterSpan = new Subspan;
		pAfterSpan->bClosedEnd = bClosedEnd; // set the parent's bClosedEnd value in the
											 // child pAfterSpan, so that if the parent was
											 // rightmost, the child pAfterSpan will also
											 // be rightmost
		if (bClosedEnd)
		{
			// right boundary index limits are to be strictly obeyed...
			if (parentOldEndAt > pCommonSubspan->oldEndPos)
			{
				// pAfterSpan has some arrOld content ..., now check the end of the arrNew
				// subspan
				if (parentNewEndAt > pCommonSubspan->newEndPos)
				{
					// pAfterSpan's subspan in arrNew has some content too - obey the end
					// limits (TRUE forces that)
					InitializeSubspan(pAfterSpan, afterSpan, pCommonSubspan->oldEndPos + 1,
								pParentSubspan->oldEndPos, pCommonSubspan->newEndPos + 1, 
								pParentSubspan->newEndPos, TRUE);
				}
				else
				{
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
					InitializeSubspan(pAfterSpan, afterSpan, -1, -1, 
								pCommonSubspan->newEndPos + 1, parentNewEndAt, TRUE);
				}
				else
				{
					// the pCommonSubspan has no content from either arrOld nor arrNew
					// after the commonSpan
					delete pAfterSpan;
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
				// oldMaxIndex, so a special calc is needed here - we can call
				// SetEndIndices() here so long as when it returns we reset the
				// oldStartPos and oldEndPos values in pAfterSpan to -1 to indicate that
				// the arrOld's subspan within pAfterSpan is empty
				pAfterSpan->oldStartPos = oldStartAt; // invalid, but we'll correct it below
				pAfterSpan->newStartPos = newStartAt; // possibly valid, next test will find out
				
				// also check the situation which exists in arrNew...
				if (newStartAt > newMaxIndex)
				{
                    // the subspan in pCommonSubspan belonging to arrNew must end at
                    // newMaxIndex also, so both commonSpan subspans end at their
                    // respective arrOld and arrNew ends - hence pAfterSpan is empty & can
                    // be deleted and its pointer set to NULL (for assigning to tuple[2]
                    // below)
					delete pAfterSpan;
					pAfterSpan = NULL;
				}
				else
				{
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
				}
			} // end of TRUE block for test: if (oldStartAt > oldMaxIndex)
			else
			{
                // the subspan in pCommonSubspan belonging to arrOld ends earlier than the
                // end of arrOld, so there are one or more CSourcePhrase instances beyond
                // pCommonSubspan->oldEndPos which we need to process
				pAfterSpan->oldStartPos = oldStartAt; // valid
				pAfterSpan->newStartPos = newStartAt; // possibly valid, next test will find out

				// check the situation which exists in arrNew...
				if (newStartAt > newMaxIndex)
				{
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
	/*
#ifdef __WXDEBUG__
	wxString allOldSrcWords; // for debugging displays in Output window using
	wxString allNewSrcWords; // wxLogDebug() calls that are below
#endif
	*/
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
                // store in pMergedList. (The ptr to Subspan which is stored in
                // tuple[tupleIndex] is removed from the heap before MergeOldAndNew()
                // returns)
                pParent = tuple[tupleIndex];
				type = pParent->spanType;
				wxASSERT(type == commonSpan);
				bIsClosedEnd = TRUE;
				wxASSERT(bIsClosedEnd == pParent->bClosedEnd);

				// do the old CSourcePhrase instances' transfers
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
			// the Subspan ptr stored in tuple[tupleIndex] must exist, so process it
			pParent = tuple[tupleIndex];
			/*				
#ifdef __WXDEBUG__
			// The GetKeysAsAString_KeepDuplicates() calls are not needed, but useful initially for
			// debugging purposes -- comment them out later on
			int oldWordCount = 0;
			// TRUE is bShowOld
			oldWordCount = GetKeysAsAString_KeepDuplicates(arrOld, pParent, TRUE, allOldSrcWords, limit); 													
			wxLogDebug(_T("oldWordCount = %d ,  allOldSrcWords:  %s"), oldWordCount, allOldSrcWords.c_str());
			if (allOldSrcWords.IsEmpty())
				return;
			int newWordCount = 0;
			// FALSE is bShowOld
			newWordCount = GetKeysAsAString_KeepDuplicates(arrNew, pParent, FALSE, allNewSrcWords, limit); 													
			wxLogDebug(_T("newWordCount = %d ,  allNewSrcWords:  %s"), newWordCount, allNewSrcWords.c_str());
			if (allNewSrcWords.IsEmpty())
				return;
#endif
			*/
			/*
#ifdef __WXDEBUG__
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
			*/
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
				/*
#ifdef __WXDEBUG__
				if (pParent != NULL)
				{
					wxLogDebug(_T("** Recursing into child tuple from parent: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos, pParent->newEndPos, (int)pParent->bClosedEnd); 
				}
#endif
				*/
				RecursiveTupleProcessor(arrOld, arrNew, pMergedList, limit, aChildTuple);
			}
			else
			{
				if (pParent != NULL)
				{
					wxLogDebug(_T("** Merging the parent: { arrOld subspan(%d,%d) , arrNew subspan(%d,%d) } Closed-ended? %d"),
						pParent->oldStartPos, pParent->oldEndPos, pParent->newStartPos, pParent->newEndPos, (int)pParent->bClosedEnd); 
				}
				MergeOldAndNew(arrOld, arrNew, pParent, pMergedList);
			}
		} // end of else block for test: if (tupleIndex == 1) i.e. it is 0 or 2

	} // end of for loop: for (tupleIndex = 0; tupleIndex < tupleSize; tupleIndex++)
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
		aChar = range.GetChar(0);
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
						aChar = range.GetChar(0);
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
							wxMessageBox(msg,_T("Verse range specification error (ignored)"),wxICON_WARNING);
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
// found, and if the match is at the last instance in pArray, or the index goes out of
// bounds, set bReachedEndOfArray TRUE as well, otherwise the latter returns FALSE.
int GetNextNonemptyMarkers(SPArray* pArray, int& startFrom, bool& bReachedEndOfArray)
{
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
		if (!pSrcPhrase->m_markers.IsEmpty())
		{
			if (index == count - 1)
			{
				bReachedEndOfArray = TRUE;
			}
			return index;
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
	// (this stuff is in m_titleMkrs)
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
// book-initial material, but before a chapter marker, standard subheader or verse. If that
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
	//bool bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
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
	// stuff before the verse commences). Once we have a CSourcePhrase with a \v, search
	// ahead to the start of the next instance with either \c or \v - and then the end of
	// the chunk is the previous CSourcePhrase instance to that one. Once we are past any
	// introduction material, we want each chunk to just have a single verse in it,
	// regardless of how much other stuff there might be as well.
	bool bReachedEndOfArray = FALSE;
	if (bIsVerseMkrWithin)
	{
		// the \c and the \v are stored on the one CSourcePhrase, so get to the end of the
		// verse
		;
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
			//bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
			//bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
			//bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);


			// is this loop done?
			if (bIsChapterMkrWithin || bIsVerseMkrWithin)
			{
				// we've found the start of the next chapter, or first verse of the same chapter
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
    // preceding the next one we find -- so move forward and define the chunk's end; in the
    // former circumstance, the preceding instance from where we currently are pointing is
    // the chunk's end
	int end_AtStartOfVerseOrChapter = index;
	if (bIsChapterMkrWithin)
	{
		// exited from the above loop because we came to a chapter \c marker; so can't go
		// further to include verse material
		endsAt = end_AtStartOfVerseOrChapter - 1;
		bReachedEndOfArray = FALSE;
		return TRUE;
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
		while(foundIndex != wxNOT_FOUND && !bReachedEndOfArray)
		{
			// get the m_markers content in this instance, put it into markers
			pSrcPhrase = arrP->Item(foundIndex);
			markers = pSrcPhrase->m_markers;

			// check for any marker which indicates the chapterPlusVerse material is ended
			bIsChapterMkrWithin = IsSubstringWithin(chapterMkrs, markers);
			//bIsSubheadingMkrWithin = IsSubstringWithin(normalOrMinorMkrs, markers);
			//bIsMajorOrSeriesMkrs = IsSubstringWithin(majorOrSeriesMkrs, markers);
			//bIsRangeOrPsalmMkrs = IsSubstringWithin(rangeOrPsalmMkrs, markers);
			bIsVerseMkrWithin = IsSubstringWithin(verseMkrs, markers);

			// is this loop done?
			if (bIsChapterMkrWithin || bIsVerseMkrWithin)
			{
				// we've found the start of the next chapter, or first verse of the same chapter
				break;
			}
			// start of next chapter or start of second verse of current chapter wasn't found, so 
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

		// we got over a verse to the start of the next, or to the start of a new chapter
		// - so the end of the chunk we are delineating is at the previous index value
		endsAt = --index;
		return TRUE;
	} // end of else block for test: if (bIsChapterMkrWithin)

	// Control should never get here, but this is harmless and compiler will be happy
	endsAt = endIndex;
	return TRUE;
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

///////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE if the array could be chunked because markers were present,
///                         FALSE if it could not be chunked (that would almost certainly
///                         be because the original Plain Text source text from which the
///                         tokenized CSourcePhrase instances were obtained, did not have
///                         any USFM or SFM markup)
/// \param  pInputArray ->  an array of CSourcePhrase pointers (it may be a subarray
///                         extracted from a larger array, it can't be assumed to be
///                         the whole document)
/// \param  pChunkSpecs <-  an array of pointers to SfmChunk structs
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
bool AnalyseSPArrayChunks(SPArray* pInputArray, wxArrayPtrVoid* pChunkSpecs)
{
	bool bCannotChunkThisArray = TRUE; // it might turn out to have no (U)SFMs in it
	pChunkSpecs->Clear(); // ensure it starts empty
	int nStartsAt = 0;
	int nEndsAt = wxNOT_FOUND;
	int count = pInputArray->GetCount();
	int endIndex = count - 1;
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
		nStartsAt = nEndsAt + 1;
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
		nStartsAt = nEndsAt + 1;
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
		nStartsAt = 0;
		nEndsAt = wxNOT_FOUND; 
	}

	// Now we analyse one or more chapters until done. This will involve smaller chunks -
	// largest should be a verse. The chunks, in order of occurrence that we will search
	// for, in a loop, are:
	// 1. chapterBeginChunk, 2. subheadingChunk, 3. parallelRefChunk, 4. verseChunk
	// All four are tried, in order, per iteration of the loop.
	while (nStartsAt <= endIndex)
	{







// ** done to here **


	}
	if (bCannotChunkThisArray)
	{
		return FALSE;
	}
	return TRUE;
}

























