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

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "helpers.h"
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
/// Note: some CSourcePhrase instances encountered may be mergers, and so will contain
/// two or more words. We have to break these into individual word tokens and check for
/// duplicates on each such token. We ignore ellipses (...) for placeholders and don't put
/// them into the return string.
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
	

/*
	int count = arr.GetCount();
	int activeSpanCount = count - nStartAt;
	oldSrcKeysStr.Empty();
	int wordCount = 0;
	if (activeSpanCount == 0)
		return 0;
	if (nStartAt > count - 1)
		return 0; // otherwise there would be a bounds error
	int index;
	wxString space = _T(' ');
	wxString ellipsis = _T("...");
	wxArrayString arrMerged;
	long mergeCount = 0;
    // limit the span of CSourcePhrase instances to be checked to the limit value:
    // possible values passed in are -1, or SPAN_LIMIT (ie. 50), or an explicit value. If
    // limit is -1 or those remaining from nStartAt onwards is positive but fewer than
    // limit remain from nStartAt onwards, then just use the remainder value instead; if
    // SPAN_LIMIT or an explicit value is passed in, use it - but shorten it appropriately
    // if the value would otherwise result in a bounds error.
    int howMany = activeSpanCount; // a default count of how many are to be checked, starting at nStartAt
	if (limit == -1 || (activeSpanCount > 0 && activeSpanCount < limit))
	{
		howMany = activeSpanCount;
	}
	else
	{
		howMany = limit;
	}
*/
	for (index = nStartAt; index < howMany; index++)
	{
		CSourcePhrase* pSrcPhrase = arr.Item(index);
		wxASSERT(pSrcPhrase != NULL);
		wxString aKey = pSrcPhrase->m_key;
		if (aKey == ellipsis)
			continue; // skip over any ... ellipses (placeholders)
		if (pSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger -- so get the word tokens and deal with each individually
			wxString aWordToken;
			arrMerged.Clear();
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
/// Note: some CSourcePhrase instances encountered may be mergers, and so will contain
/// two or more words. We have to break these into individual word tokens and check for
/// duplicates on each such token. We ignore ellipses (...) for placeholders - and there
/// shouldn't be any in the arr array anyway, since what is in arr will be the
/// just-tokenized CSourcePhrase instances arising from calling TokenizeText() on the
/// just-edited (outside of Adapt It) source text USFM plain text string.
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

	for (index = nStartAt; index < howMany; index++)
	{
		CSourcePhrase* pSrcPhrase = arrNew.Item(index);
		wxASSERT(pSrcPhrase != NULL);
		wxString aKey = pSrcPhrase->m_key;
		if (aKey == ellipsis)
			continue; // skip over any ... ellipses (placeholders)
		if (pSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger -- so get the word tokens and deal with each individually
			wxString aWordToken;
			arrMerged.Clear();
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
	} // end of for loop for scanning a span (or all)
	return strArray.GetCount();
}

// overloaded version, which takes both arrays as params etc
//int GetWordsInCommon(SPArray& arrOld, int oldStartAt, SPArray& arrNew, 
//					 int newStartAt, wxArrayString& strArray, int limit)
int	GetWordsInCommon(SPArray& arrOld, SPArray& arrNew, Subspan* pSubspan, wxArrayString& strArray, int limit){
	strArray.Clear();
    // turn the array of old CSourcePhrase instances into a fast access string made up of
    // the unique m_key words (note, if the instances contain mergers, phrases will be
    // obtained, but that's fine - they will have spaces between the words as required
    // here)
    wxString oldUniqueSrcWords;
	// there will be no placeholder ellipses in the returned string oldUniqueSrcWords
	int oldUniqueWordsCount = GetUniqueOldSrcKeysAsAString(arrOld, pSubspan, oldUniqueSrcWords, limit);
#ifdef __WXDEBUG__
	wxLogDebug(_T("oldUniqueWordsCount = %d ,  oldUniqueSrcWords:  %s"), oldUniqueWordsCount, oldUniqueSrcWords.c_str());
#endif
	wxASSERT(!oldUniqueSrcWords.IsEmpty());

	// compare the initial words from the arrNew array of CSourcePhrase instances with
	// those in oldUniqueSrcWords
	int commonsCount = GetWordsInCommon(arrNew, pSubspan, oldUniqueSrcWords, strArray, limit);
#ifdef __WXDEBUG__
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
#endif
	return commonsCount;
}

void InitializeSubspan(Subspan* pSubspan, SubspanType spanType, int oldStartPos, 
				int newStartPos, int oldEndPos, int newEndPos, bool bClosedEnd)
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
/// parameter ( so that the caller can check if the phrase occurs in the other array by
/// searching in the other array's derived fast access string)
/// We have startFrom and endAt bounds because we want to limit our algorithms to a short
/// span in which we have a certainty of there being at least one word in common - the
/// default value of SPAN_LIMIT (set to 50) is suitably large without generating heaps of
/// possibilities which all have to be checked.
/////////////////////////////////////////////////////////////////////////////////////////////
int FindNextInArray(wxString& word, SPArray& arr, int startFrom, int endAt, wxString& phrase)
{
	int i;
	bool bFound = FALSE;
	for (i = startFrom; i <= endAt; i++)
	{
		CSourcePhrase* pSrcPhrase = arr.Item(i);
		int offset = pSrcPhrase->m_key.Find(word);
		if (offset != wxNOT_FOUND)
		{
			// this instance contains the string within word, determine now if it's a
			// merger and if so, put the m_key value into the phrase parameter
			phrase.Empty();
			if (pSrcPhrase->m_nSrcWords > 1)
			{
				// it's a merger
				phrase = pSrcPhrase->m_key; // keys have no preceding or following space
											// so no need to call Trim()
			}
			bFound = TRUE;
			break;
		}
	}
	if (!bFound)
	{
		i = wxNOT_FOUND;
	}
	return i;
}

void MergeUpdatedSourceText(SPList& oldList, SPList& newList, SPList* pMergedList, int limit)
{
	// turn the lists into arrays of CSourcePhrase*; note, we are using arrays to manage
	// the same pointers as the SPLists do, so don't in this function try to delete any of
	// the oldList or newList contents, nor what's in the arrays
	int nStartingSequNum;
	SPArray arrOld;
	SPArray arrNew;
	SPList::Node* posOld = oldList.GetFirst();
	while (posOld != NULL)
	{
		CSourcePhrase* pSrcPhraseOld = posOld->GetData();
		posOld = posOld->GetNext();
		arrOld.Add(pSrcPhraseOld);
	}
	int oldSPCount = oldList.GetCount();
	if (oldSPCount == 0)
		return;
	nStartingSequNum = (arrOld.Item(0))->m_nSequNumber; // store this, to get the 
														// sequence numbers right later
	SPList::Node* posNew = newList.GetFirst();
	while (posNew != NULL)
	{
		CSourcePhrase* pSrcPhraseNew = posNew->GetData();
		posNew = posNew->GetNext();
		arrNew.Add(pSrcPhraseNew);
	}
	int newSPCount = newList.GetCount();
	if (newSPCount == 0)
		return;

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
	// initialize tuple[2]
	InitializeSubspan(pSubspan, afterSpan, 0, 0, oldSPCount - 1, 
						newSPCount - 1, FALSE); // FALSE is bClosedEnd

    // when this next call finally returns after being called possibly very many times, the
    // merging is complete 
    RecursiveTupleProcessor(arrOld, arrNew, pMergedList, limit, tuple);

	// finally, get the CSourcePhrase instances appropriately numbered in the temporary
	// list which is to be returned to the caller
	gpApp->GetDocument()->UpdateSequNumbers(nStartingSequNum, pMergedList);
}

//////////////////////////////////////////////////////////////////////////////////////
/// \return          count of words which are in common and in succession in each array
///                 (therefore the returned value could be 0, a small value, a large 
///                 value greater than SPAN_LIMIT and exceeding the parent span's breadth
///                 (but the latter only if bClosedEnd is FALSE, otherwise oldEndAt and 
///                 newEndAt are boundaries in each array)
/// \param  arrOld      ->  array of old CSourcePhrase instances
/// \param  arrNew      ->  array of new CSourcePhrase instances
/// \param  oldStartAt  ->  starting index in arrOld for parent Subspan
/// \param  oldEndAt    ->  ending (inclusive) index in arrOld for parent Subspan
/// \param  newStartAt  ->  starting index in arrNew for parent Subspan                
/// \param  newEndAt    ->  ending (inclusive) index in arrNew for parent Subspan
/// \param  bClosedEnd  ->  the parent Subspan's bClosedEnd value (if FALSE, then in this function
///                         matching words in common can go beyond the oldEndAt and newEndAt 
///                         bounding values, but if TRUE, the bounds must be obeyed)
/// \param  pWords      ->  array of words which are common to both arrOld and arrNew in the parent
///                         Subspan (these are what we search for, looking for matchups of embedded
///                         spans of in-common and in-seqence words, and we'll pass back the longest
///                         such span in pMaxInCommonSubspan)
/// \param  pMaxInCommonSubspan <-> passed in initialized to parent index values, but later
///                         filled out here with the details for the max subspan - to pass them
///                         back to the caller
/// \remarks
/// We look for a matchup -- a word from pWords is found in arrNew and in arrOld, start from
/// the left - the matchup is expanded to left and right in both arrays, until corresponding
/// words are not the same - that ends the expanding, and we store the word count of the
/// expanded matchup in a wxArrayInt, and the start and end index details in a Subspan* on
/// the heap - with pointer stored in a wxArrayPtrVoid - there may be several valid
/// subspans in the parent span, and we need to get each one and pass back the longest
/// (and delete the others - they'll be re-found at a later call in the recursion). If the
/// parent Subspan has SpanType of afterSpan, it will be bClosedEnd = FALSE, and in that
/// circumstance the longest matchup might go beyond the end of the parent's Subspan -- we
/// allow this because if the user edited just the start of the original source text,
/// there's no good reason to stop the matchups going to the very end of both arrOld and
/// arrNew, so as to save processing time by avoiding lots of recursion.
/// Our algorithm potentially finds the first subspan of in-common words quickly, if there
/// is such a span, it's words should be near the top of the passed in pWords array. Once
/// we have found the matchup and expanded it, we start the hunt for the next potential
/// subspan starting over in pWords, but starting the searching at the index values which
/// lie one greater than the end of the last found subspan of in-common and in-sequence
/// words. We find all such subspans, and as soon as one of them reaches or exceeds the
/// oldEndAt and / or newEndAt index values (these are passed in from the parent Subspan)
/// then we have found all possible in-common subarrays in the parent Subspan. At that
/// point we check for the longest, and use that one as the one we pass back in the
/// pMaxInComnmonSubspan parameter. (It's the caller which has the job of checking the
/// ending indices in the Subspan passed back - and setting up a tuple, and if the
/// matchups extend to the ends of arrOld and arrNew, then the tuple's afterSpan would be
/// empty.)
/// This function is the core of the Import Edited Source Text functionality.             
int	GetMaxInCommonSubspan(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int oldEndAt,
				int newStartAt, int newEndAt, bool bClosedEnd, wxArrayString* pWords,
				Subspan* pMaxInCommonSubspan)
{
	int wordsInCommonAndInSuccession = 0;
	wxArrayInt arrSpanWidths;
	wxArrayPtrVoid arrSubspans;
	wxString anInCommonWord;
	
	int inCommonCount = pWords->GetCount();
	int lastInCommonIndex = -1;
	int oldNextStartPos = oldStartAt;
	int newNextStartPos = newStartAt;
	int oldFoundStart;
	int oldFoundEnd;
	int newFoundStart;
	int newFoundEnd;
	int oldStartHere = oldStartAt;
	int newStartHere = newStartAt;
	bool bParentHasClosedEnd = bClosedEnd;
	wxString phrase;
	Subspan* pSubspan = new Subspan;
	// initialize to parent's params
	InitializeSubspan(pSubspan, commonSpan, oldStartAt, newStartAt, oldEndAt, newEndAt, bClosedEnd);
	int index;
	while (pWords->GetCount() > 0)
	{
		for (index = 0; index < inCommonCount; index++)
		{
			// get the next in-common word
			lastInCommonIndex++;
			anInCommonWord = pWords->Item(lastInCommonIndex);
			// find next instance of it in arrOld, starting from next instance after last subspan
			phrase.Empty(); 
			int anOldLoc = FindNextInArray(anInCommonWord, arrOld, oldStartHere, oldEndAt, phrase);
			// no matchup is possible within the bounds of the parent subspan 
			// if anOldLoc is wxNOT_FOUND, or if anOldLoc is +ve but greater than oldEndAt



		} // end of for loop: for (index = 0; index < inCommonCount; index++)

		// when control gets here, all possibilities for words-in ...??? careful, spurious
		// matches..?? rethink a bit...

	}


	return wordsInCommonAndInSuccession;
}

bool IsMatchupWithinAnySpan(int oldPos, int newPos, wxArrayPtrVoid* pSubspansArray)
{
	int count = pSubspansArray->GetCount();
	if (count == 0)
		return FALSE;
	int index;
	Subspan* pSubspan = NULL;
	for (index = 0; index < count; index++)
	{
		pSubspan = (Subspan*)pSubspansArray->Item(index);
		if (oldPos >= pSubspan->oldStartPos && oldPos <= pSubspan->oldEndPos)
		{
			// oldPos is within the subspan derived from arrOld, so this is a candidate
			// for a possible matchup
			if (newPos >= pSubspan->newStartPos && newPos <= pSubspan->newEndPos)
			{
				// we've got a matchup, so return TRUE;
				return TRUE;
			}
		}
		// no matchup, so try another of the stored Subspan instances

	}
	return FALSE;
}

int WidenMatchup(SPArray& arrOld, SPArray& arrNew,  int oldStartPos, int newStartPos, 
					 int oldEndPos, int newEndPos, int oldMatchLoc, int newMatchLoc, 
					 bool bClosedEnd, Subspan* pSubspan)
{
	wxASSERT(pSubspan->spanType == commonSpan);
	int span_width = 0;
	int oldIndex = oldMatchLoc;
	int newIndex = newMatchLoc;
	// widen to smaller sequence numbers first
	oldIndex--;
	newIndex--;
	while (oldIndex >= oldStartPos && newIndex >= newStartPos)
	{
		CSourcePhrase* pOldSrcPhrase = arrOld.Item(oldIndex);
		CSourcePhrase* pNewSrcPhrase = arrNew.Item(newIndex);
		if (pOldSrcPhrase->m_nSrcWords > 1)
		{
            // the arrOld's CSourcePhrase instance is a merger, check that newArr has the
			// same key if it was merged -- check bClosedEnd, if open, we have a less
			// stringent test, if closed, the arrNew's word sequence must not exceed the
			// newEndPos value
			if (bClosedEnd)
			{
			




			}
			else
			{





			}

			//bool	IsMergerAMatch(SPArray& arrOld, SPArray& arrNew, int oldLoc, int newFirstLoc);
				
		}
		else
		{
			// arrOld's CSourcePhrase instance is not a merger, test for identity of keys


		}



// *** TODO ***
	}

	return span_width;
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

void RecursiveTupleProcessor(SPArray& arrOld, SPArray& arrNew, SPList* pMergedList,
							   int limit, Subspan* tuple[])
{
	int siz = tupleSize;
	int tupleIndex;
	SubspanType type;
	bool bIsClosedEnd = TRUE; // default, but can be changed below
#ifdef __WXDEBUG__
	wxString allOldSrcWords; // for debugging displays in Output window using wxLogDebug
	wxString allNewSrcWords; // ditto
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
                // *** TODO **** code for handling the CSourcePhrase instances which are
                // in common (i.e. unchanged by the user's edits of the source text
                // outside of the application)
				type = tuple[tupleIndex]->spanType;
				bIsClosedEnd = TRUE; // the commonSpan type is always closed



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
			// the Subspan must exist, so process it
			type = tuple[tupleIndex]->spanType;
			bIsClosedEnd = tuple[tupleIndex]->bClosedEnd; // only the rightmost afterSpan at
							// any given nesting level will be FALSE, other afterSpans at the
							// same level are always TRUE, and all beforeSpans are TRUE
							// regardless of the level they are at, as are all commonSpans 
							// likewise
			int oldStartAt = tuple[tupleIndex]->oldStartPos;
			int newStartAt = tuple[tupleIndex]->newStartPos;
				
#ifdef __WXDEBUG__
			// The GetKeysAsAString_KeepDuplicates() calls are not needed, but useful initially for
			// debugging purposes -- comment them out later on
			// 
			// turn the array of old CSourcePhrase instances into a string of m_key values (there will
			// be no mergers in this stuff), and don't remove word duplicates; call without 4th
			// param int limit, so defaults to SPAN_LIMIT = 50 (we need the duplicates so we can
			// later check any merger to see if it's source text is retained unchanged in the
			// edited source text) ... no placeholder ellipses in the returned string
			// allOldSrcWords
			int oldWordCount = 0;
			// TRUE is bShowOld
			oldWordCount = GetKeysAsAString_KeepDuplicates(arrOld, tuple[tupleIndex], TRUE, allOldSrcWords, limit); 													
			wxLogDebug(_T("oldWordCount = %d ,  allOldSrcWords:  %s"), oldWordCount, allOldSrcWords.c_str());
			if (allOldSrcWords.IsEmpty())
				return;
			// turn the array of new CSourcePhrase instances into a string of m_key values (there will
			// be no mergers in this stuff), and don't remove word duplicates; call without 4th
			// param int limit, so defaults to SPAN_LIMIT = 50 (we need the duplicates so we can
			// later check any merger to see if it's source text is retained unchanged in the
			// edited source text) ... no placeholder ellipses in the returned string
			// allNewSrcWords
			int newWordCount = 0;
			// FALSE is bShowOld
			newWordCount = GetKeysAsAString_KeepDuplicates(arrNew, tuple[tupleIndex], FALSE, allNewSrcWords, limit); 													
			wxLogDebug(_T("newWordCount = %d ,  allNewSrcWords:  %s"), newWordCount, allNewSrcWords.c_str());
			if (allNewSrcWords.IsEmpty())
				return;
#endif
			// Take a span of limit CSourcePhrase instances in the new array, starting at
			// newStartAt, and find the set of unique m_key words by comparison with a similar
			// length span in the old array, starting at oldStartAt.(Note, if the instances contain
			// mergers they will be tokenized to single-word tokens). If limit is -1, the rest of
			// the arrays are checked, but we'll use the default value (the call without 6th param
			// limit explicitly set defaults it to SPAN_LIMIT = 50)
			wxArrayString arrInCommon; // store here source text words which are in both of the
									   // scanned initial parts of oldList and newList (or the
									   // whole if newList is short enough (i.e. < SPAN_LIMIT)
			int commonsCount = GetWordsInCommon(arrOld, arrNew, tuple[tupleIndex], arrInCommon, limit);
			commonsCount = commonsCount; // avoid compiler warning

			// determine match associated positions in each array using the set of common words,
			// starting from the top of the latter's array, since these will be approximately in
			// order of occurrence in both arrays - thereby reducing the number of tests needed to
			// get matches which we can then expand into subspans of common CSourcePhrase
			// instances, (but bogus matches will generate short subspans, so taking the longest
			// gives us safety from setting up a bogus span association) 
			int oldEndAt;
			int newEndAt;
			// set oldEndAt and newEndAt so that our searches don't go beyond the end of the
			// relevant limited (by the limit parameter's value) spans being tested
			if (bIsClosedEnd)
			{
				// when the end is closed, the end index coming from the oldEndPos (for
				// arrOld) and newEndPos (for arrNew) as stored in the tuple's Subspan
				// which we are processing supplies the strict end, which must not be
				// exceeded - otherwise neighbouring Subspans would overlap, which would
				// result in duplicating CSourcePhrase instances bogusly in pMergedList
				oldEndAt = tuple[tupleIndex]->oldEndPos;
				newEndAt = tuple[tupleIndex]->newEndPos;
			}
			else
			{
				// when the end is open, it's legal for the in-common instances matching
				// to progress past the calculated end of the subinterval - providing the
				// end of the array has not been reached
				if (limit == -1)
				{
					// no limitation is wanted - this requires we set the end index to the
					// max index for each array
					oldEndAt = arrOld.GetCount() - 1;
					newEndAt = arrNew.GetCount() - 1;
				}
				else
				{
					oldEndAt = wxMin((unsigned int)(oldStartAt + limit), (unsigned int)(arrOld.GetCount())) - 1;
					newEndAt = wxMin((unsigned int)(newStartAt + limit), (unsigned int)(arrNew.GetCount())) - 1;
				}
			}
			Subspan* pMaxInCommonSubspan = new Subspan;
			InitializeSubspan(pMaxInCommonSubspan, type, oldStartAt, 
						  newStartAt, oldEndAt, newEndAt, bIsClosedEnd);
			int nSpanWidth = GetMaxInCommonSubspan(arrOld, arrNew, oldStartAt, oldEndAt,
				newStartAt, newEndAt, bIsClosedEnd, &arrInCommon, pMaxInCommonSubspan);





		} // end of else block for test: if (tupleIndex == 1) i.e. it is 0 or 2
	} // end of for loop: for (tupleIndex = 0; tupleIndex < tupleSize; tupleIndex++)
}

























