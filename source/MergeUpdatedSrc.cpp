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
/// them into the return string. Any pseudo-merger conjoined pairs are decomposed into two
/// individual words.
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
/// parameter ( so that the caller can check if the phrase occurs in the other array).
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
				// it's a merger, or a fixedspace pseudo-merger
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
/// IS a problem and we must fix it). We do the fix for the latter in a separate function
/// which runs after recursion is completed and which spans whole newly merged document
/// looking for the messed-up retranslation subspans, and fixes things - the fixes remove
/// the adaptations within the retranslation fragments that have lost either end. 
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
/// at an appropriate index value.
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
	if (oldStartingPos == oldStartAt || newStartingPos == newStartAt)
	{
		// we are at the parent's left bound, so can't widen to the left any further
		return FALSE;
	}
	int oldIndex = oldStartingPos;
	int newIndex = newStartingPos;
	if (oldIndex > oldStartAt && newIndex > newStartAt)
	{
		oldIndex--;
		newIndex--;
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
				return FALSE;
			}
		}
		// The second test should be for pOldSrcPhrase being a merger (if so, it can't be a
		// placeholder nor within a retranslation) because that is more likely to happen
		// than the other complications we must handle. We'll also handle fixedspace
		// conjoining here too - and accept a non-conjoined identical word pair in arrNew
		// as a match (ie. we won't require that ~ also conjoins the arrNew's word pair) 
		if (pOldSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger, or conjoined pseudo-merger
			wxASSERT(!pOldSrcPhrase->m_bRetranslation);
			wxASSERT(!pOldSrcPhrase->m_bNullSourcePhrase);
			int numWords = pOldSrcPhrase->m_nSrcWords;
			bool bIsFixedspaceConjoined = IsFixedSpaceSymbolWithin(pOldSrcPhrase);
            // If it's a normal (non-fixedspace) merger, then we do the same check
            // as for a fixedspace merger, because if ~ is still in the edited
            // source text, then that will parse to a conjoined (pseudo merger)
            // CSourcePhrase with m_nSrcWords set to 2 - so check for either of
            // these first, and if there isn't a match, then check if the old
            // instance really is a fixedspace merger, and is so try matching each
            // word of the two word sequence at the approprate location, without
			// any ~, -- if that succeeds, treat it as a match; if none of that gives a
			// match, then there isn't one and we are done - return FALSE if so
			if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			{
				if (bIsFixedspaceConjoined)
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
					// it's a plain vanilla merger, so get the starting index for it on
					// the assumption that arrNew has a matching word sequence, then test
					// for the match
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
						// no match
						return FALSE;
					}
				} // end of else block for test: if (bIsFixedspaceConjoined)
			} // end of TRUE block for test: if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			else if (bIsFixedspaceConjoined && (newIndex - numWords + 1 >= newStartAt))
			{
                // for a non-match, check if there was a fixedspace (~) in
                // pOldSrcPhrase->m_key, and if so try for a match by matching the two
                // words separately -- but the match is only possible provided there are
                // numWords amount of matching keys available in arrNew at or after
                // the newStartAt value
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
					return FALSE;
				}	
			}
			else
			{
                // we've exhausted all possibilities, so there is no match & so we are at
                // the left hand end of commonSpan
				return FALSE;
			}
		} // end of TRUE block for test: if (pOldSrcPhrase->m_nSrcWords > 1)

		// Next complications are placeholders and mergers. So, because placeholders can
		// occur at the end of a retranslation, I'll test first for pOldSrcPhrase being a
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
		// depend on whether there is indication of left association (then it/they belong in
		// beforeSpan) or right association (then it/they belong in commonSpan) or no
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
									return FALSE; // tell the caller not to try another leftwards widening
								}
								else if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
								{
									// it/they are left-associated, so keep it/them with
									// what precedes it/them, which means that it/they
									// belong in beforeSpan, and we can't widen any further
									return FALSE; // (oldCount and newCount both returned as zero)
								}
								// neither left nor right associated, so handle the
								// same as left-associated
								return FALSE;
							} // end of else block for test: if (bItsInARetranslation)
						} // end of else block for test: if (pPrevSrcPhrase->m_key == pNewSrcPhrase->m_key)
					} // end of TRUE block for test: if (pPrevSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					else
					{
						// it's a merger
						int numWords = pPrevSrcPhrase->m_nSrcWords;
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
							// no match
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
					return FALSE; // tell the caller not to try another leftwards widening
				}
				else if (IsLeftAssociatedPlaceholder(pRightmostPlaceholder))
				{
                    // it/they are left-associated, so keep it/them with
                    // what precedes it/them, which means that it/they
                    // belong in beforeSpan, and we can't widen any further
                    return FALSE; // (oldCount and newCount both returned as zero)
				}
				// neither left nor right associated, so handle the
				// same as left-associated
				return FALSE;
			}

		} // end of TRUE block for test: if (pOldSrcPhrase->m_bNullSourcePhrase)

	} // end of TRUE block for test: if (oldIndex > oldStartAt && newIndex > newStartAt)
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////
/// \return                 TRUE, if the attempt at widening rightwards by a "single" jump
///                         succeeds, otherwise FALSE
/// \param  arrOld          ->  array of old CSourcePhrase instances
/// \param  arrNew          ->  array of new CSourcePhrase instances
/// \param  oldStartAt      ->  starting index in arrOld for parent Subspan
/// \param  oldEndAt        ->  ref to ending (inclusive) index in arrOld for parent Subspan
/// \param  newStartAt      ->  starting index in arrNew for parent Subspan                
/// \param  newEndAt        ->  ref to ending (inclusive) index in arrNew for parent Subspan
/// \param  oldStartingPos  ->  the index in arrOld from which we start our rightwards jump
/// \param  newStartingPos  ->  the index in arrNew from which we start our rightwards jump
/// \param  oldCount        <-  ref to a count of the number of CSourcePhrase instances to accept
///                             to the right in our "single" jump within the arrOld array (it may 
///                             not be just one - see below)
/// \param  newCount        <-  ref to a count of the number of CSourcePhrase instances to accept
///                             to the right in our "single" jump within the arrNew array (it should 
///                             always be just one - see below, because arrNew will NEVER
///                             have any retranslations or placeholders in it)
/// \param  bClosedEnd      ->  the parent Subspan's bClosedEnd value (if FALSE, then in this
///                             function matching words in common can go beyond the oldEndAt and 
///                             newEndAt bounding values, but if TRUE, the bounds must be obeyed)
///                         
/// \remarks
/// Note: WidenRightwardsOnce() passes in oldEndAt and newEndAt as references, because
/// when the parent span is bClosedEnd == FALSE (ie. is open), we check within the
/// function to make sure that the oldEndAt and newEndAt values passed in are the max
/// index values possible for the contents of arrOld and arrNew respectively -- and if
/// that is not the case, we fix the values to be so - and we want the fixes to be
/// returned to the parent Subspan (which would be the rightmost afterSpan) - because we
/// allow widening rightwards to go as far as the end of the arrOld and arrNew arrays when
/// bClosedEnd is FALSE in the caller's afterSpan.
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
bool WidenRightwardsOnce(SPArray& arrOld, SPArray& arrNew, int oldStartAt, int& oldEndAt,
				int newStartAt, int& newEndAt, int oldStartingPos, int newStartingPos,
				int& oldCount, int& newCount, bool bClosedEnd)
{
	newStartAt = newStartAt; oldStartAt = oldStartAt; // avoid compiler warning re unused variables
	CSourcePhrase* pOldSrcPhrase = NULL;
	CSourcePhrase* pNewSrcPhrase = NULL;
	oldCount = 0;
	newCount = 0;
	// check if the end is open, if it is, then ensure that oldEndAt is equal to the size
	// of arrOld less 1; and that newEndAt is equal to the size of arrNew less 1; and if
	// that is not the case, fix it (the values are passed in by reference, and so the
	// fixes will appear in the caller's afterSpan too)
	if (!bClosedEnd)
	{
		int oldEndIndex = arrOld.GetCount() - 1;
		int newEndIndex = arrNew.GetCount() - 1;
		if (oldEndAt < oldEndIndex)
		{
			// a fix is needed
			oldEndAt = oldEndIndex;
		}
		if (newEndAt < newEndIndex)
		{
			// a fix is needed
			newEndAt = newEndIndex;
		}
	}
	if (oldStartingPos == oldEndAt || newStartingPos == newEndAt)
	{
		// we are at the parent's right bound, so can't widen to the right any further
		return FALSE;
	}
	int oldIndex = oldStartingPos;
	int newIndex = newStartingPos;
	if (oldIndex < oldEndAt && newIndex < newEndAt)
	{
		oldIndex++;
		newIndex++;
		pOldSrcPhrase = arrOld.Item(oldIndex);
		pNewSrcPhrase = arrNew.Item(newIndex);
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
				return FALSE;
			}
		}
		// The second test should be for pOldSrcPhrase being a merger (if so, it can't be a
		// placeholder nor within a retranslation) because that is more likely to happen
		// than the other complications we must handle. We'll also handle fixedspace
		// conjoining here too - and accept a non-conjoined identical word pair in arrNew
		// as a match (ie. we won't require that ~ also conjoins the arrNew's word pair) 
		if (pOldSrcPhrase->m_nSrcWords > 1)
		{
			// it's a merger, or conjoined pseudo-merger
			wxASSERT(!pOldSrcPhrase->m_bRetranslation);
			wxASSERT(!pOldSrcPhrase->m_bNullSourcePhrase);
			int numWords = pOldSrcPhrase->m_nSrcWords;
 			bool bIsFixedspaceConjoined = IsFixedSpaceSymbolWithin(pOldSrcPhrase);
            // If it's a normal (non-fixedspace) merger, then we do the same check
            // as for a fixedspace merger, because if ~ is still in the edited
            // source text, then that will parse to a conjoined (pseudo merger)
            // CSourcePhrase with m_nSrcWords set to 2 - so check for either of
            // these first, and if there isn't a match, then check if the old
            // instance really is a fixedspace merger, and is so try matching each
            // word of the two word sequence at the approprate location, without
			// any ~, -- if that succeeds, treat it as a match; if none of that gives a
			// match, then there isn't one and we are done - return FALSE if so
			if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			{
				if (bIsFixedspaceConjoined)
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
                    // it's a plain vanilla merger, so get the ending index for it and
                    // ensure that lies within arrNew's bounds, if so we assume then that
                    // arrNew may have a matching word sequence, so we test for the match
					int newEndIndex = newStartingPos + numWords - 1;
					if (newEndIndex <= newEndAt)
					{
						// there is a potential match
						bool bMatched = IsMergerAMatch(arrOld, arrNew, oldIndex, newIndex);
						if (bMatched)
						{
							oldCount++; // it now equals 1
							newCount += numWords; // no non-conjoined mergers can be in 
											// arrNew, so we count the requisite number of 
											// words which belong in commonSpan
							return TRUE;
						}
						else
						{
							// didn't match match, so we are at the end of commonSpan
							return FALSE;
						}
					}
					else
					{
						// not enough words for a match, so return FALSE
						return FALSE;
					}
				} // end of else block for test: if (bIsFixedspaceConjoined)
			} // end of TRUE block for test: if (pOldSrcPhrase->m_key == pNewSrcPhrase->m_key)
			else if (bIsFixedspaceConjoined && (newIndex + numWords - 1 <= newEndAt))
			{
                // for a non-match, check if there was a fixedspace (~) in
                // pOldSrcPhrase->m_key, and if so try for a match by matching the two
                // words separately -- but the match is only possible provided there are
                // numWords amount of matching keys available in arrNew prior to or up to
                // the newEndAt value, so test for this too
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
					return FALSE;
				}
					
			}
			else
			{
				// we've exhausted all possibilities, so there is no match & we are at the
				// right hand end of commonSpan
				return FALSE;
			}
		} // end of TRUE block for test: if (pOldSrcPhrase->m_nSrcWords > 1)

        // Next complications are placeholders and mergers. I'll test first for
        // pOldSrcPhrase being a placeholder, and then within the TRUE block distinguish
        // between placeholders within retranslations and one or more plain vanilla (i.e.
        // manually inserted previously) placeholders which are not in a retranslation. (We
        // expect that a user would never insert two or more placeholders manually at the
        // same location, but just in case he does, we'll handle them.) Our approach to
        // plain vanilla placeholder(s) is to defer action until we know if there is a
        // match of the farther-out abutting CSourcePhrase instance in arrOld with the
        // potential matching one in arrNew - if the match obtains, then the enclosed
        // placeholder(s) are taken into commonSpan; if the matchup fails, then the
        // placeholder(s) are at the boundary of commonSpan and afterSpan - and on which
        // side of it will then depend on whether there is indication of left association
        // (then it/they belong in commonSpan) or right association (then it/they belong in
        // afterSpan) or no indication of association -- in which case we have no criterion
        // to guide us, so we'll assume that it/they should be in afterSpan (and so get
        // removed in the merger process)
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
									return FALSE; // tell the caller not to try another leftwards widening
								}
								else if (IsRightAssociatedPlaceholder(pNextSrcPhrase))
								{
                                    // it/they are right-associated, so keep it/them with
                                    // what follows it/them, which means that it/they
                                    // belong in at the start of afterSpan, and we can't
                                    // widen commonSpan any further
									return FALSE; // (oldCount and newCount both returned as zero)
								}
								// neither left nor right associated, so handle the
								// same as right-associated
								return FALSE;
							} // end of else block for test: if (bItsInARetranslation)
						} // end of else block for test: if (pPrevSrcPhrase->m_key == pNewSrcPhrase->m_key)
					} // end of TRUE block for test: if (pNextSrcPhrase->m_nSrcWords == 1 || bItsInARetranslation)
					else
					{
						// it's a merger
						int numWords = pNextSrcPhrase->m_nSrcWords;
						bool bMatched = IsMergerAMatch(arrOld, arrNew, oldIndex, newIndex);
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
							return FALSE;
						}
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
					return FALSE; // tell the caller not to try another leftwards widening
				}
				else if (IsRightAssociatedPlaceholder(pNextSrcPhrase))
				{
                    // it/they are right-associated, so keep it/them with
                    // what follows it/them, which means that it/they
                    // belong in at the start of afterSpan, and we can't
                    // widen commonSpan any further
					return FALSE; // (oldCount and newCount both returned as zero)
				}
				// neither left nor right associated, so handle the
				// same as right-associated
				return FALSE;
			}

		} // end of TRUE block for test: if (pOldSrcPhrase->m_bNullSourcePhrase)

	} // end of TRUE block for test: if (oldIndex < oldEndAt && newIndex < newEndAt)
	return FALSE;
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
/// \return                     nothing
/// \param  arrOld          ->  array of old CSourcePhrase instances (may have mergers,
///                             placeholders, retranslation, fixedspace conjoinings as 
///                             well as minimal CSourcePhrase instances)
/// \param  arrNew          ->  array of new CSourcePhrase instances (will only be minimal
///                             CSourcePhrase instances, but could also contain fixedspace
///                             conjoined instances too)
/// \param  oldStartPos      -> starting index in arrOld for the parent Subspan
/// \param  newStartPos      -> starting index in arrNew for the parent Subspan                
/// \param  oldEndPos        -> ref to ending (inclusive) index in arrOld for the parent Subspan
/// \param  newEndPos        -> ref to ending (inclusive) index in arrNew for the parent Subspan
/// \param  oldMatchLoc      -> the index in arrOld from which we start our expansion to the
///                             left and to the right
/// \param  newMatchLoc      -> the index in arrNew from which we start our expansion to the
///                             left and to the right
/// \param  pSubspan        <-  ptr to a Subspan struct in which we store the extents of
///                             the paired in-common CSourcPhrase instances from arrOld
///                             and arrNew; pass this back to the caller for storage in a
///                             wxArrayPtrVoid which collects all such structs from the
///                             current span being examined, in order to then work out
///                             which is the longest, and retain that - building it in to
///                             a tuple of beforeSpan, commonSpan and afterSpan, which is
///                             then processed recursively.
/// \param  bClosedEnd      ->  the parent Subspan's bClosedEnd value (if FALSE, then in this
///                             function matching words in common can go beyond limited oldEndAt 
///                             and newEndAt bounding values, going as far as the ends of
///                             arrOld and arrNew; but if TRUE, the bounds must be obeyed)
///                         
/// \remarks
/// This function builds a single Subspan instance of type commonSpan, that is, in-common
/// data which hasn't changed in the user's source text edits done outside of Adapt It. It
/// starts from a matchup location (that is, an assocation of two index values, one in
/// arrOld, and one in arrNew, at which the same source text word or phrase was found) and
/// expands the matchup to both the left and the right in both arrays, one step at a time.
/// "One step" may turn out to actually traverse more than a single CSourcePhrase instance
/// if one or more placeholders are encountered - and these can be either manually inserted
/// ones (we support sequences of these too), or programmatically inserted ones at the end
/// of a retranslation in order to provide enough CSourcePhrase instances to display all
/// the retranslation's words lined up interlinearly. The details of placeholders,
/// retranslations, mergers, and fixedspace (~) pseudo-mergers, are all encapsulated in the
/// lower level functions, WidenLeftwardsOnce() and WidenRightwardsOnce(), which are called
/// here. The commonSpan is returned as an assocations of old consecutive CSourcePhrase
/// instances, and the matching set of new CSourcePhrase instances obtained by tokenizing
/// the edited source text that was imported. At a higher level, after all possible Subspan
/// instances in the span are identified, the widest is taken and the others thrown away.
/// (They will be recreated of course at a later stage in the recursion, and the longest
/// taken each time, etc.)
/// 
/// We iterate the leftwards widening first. It proceeds until WidenLeftwardsOnce()
/// returns FALSE - that indicates the left bound for the widening has been reached,
/// however that does NOT also imply that no CSourcePhrase instances were traversed
/// because it is possible that one or more placeholders were traversed. We then iterate
/// from the starting location rightwards, until WidenRightwardsOnce()returns FALSE,
/// indicating the right boundary for the commonSpan based on the passed in matchup, and
/// the same non-implication applies here too. WidenRightwardsOnce() also internally
/// checks the passed in bClosedEnd value, and if FALSE, it makes sure that oldEndPos is
/// set to the max index in arrOld (and changes it to be so if that is not the case), and
/// likewise, if the passed in newEndPos value is not the same as the max index in arrNew,
/// it is reset to that - and the possibly adjusted oldEndPos and newEndPos values are
/// returned to WidenMatchup() via the signature of WidenRightwardsOnce().
/// 
/// Note 1: the commonSpan Subspan obtained does not necessarily start at the same index
/// value in both arrOld and arrNew, nor end at the same index value in arrOld and arrNew,
/// nor are there necessarily the same number of CSourcePhrase instances in each (due to
/// the possibility that the oldArr sequence of CSourcePhrase instances contains mergers
/// and / or placeholders and / or fixedspace pseudo-mergers, and arrNew may contain
/// fixedspace pseudo-mergers retained, or added in the user's editing before the import
/// was done).
/// 
/// Note 2: we don't pass the potentially internally altered values of oldEndPos and
/// newEndPos (which can internally get reset to the max index values of arrOld and arrNew,
/// respectively, when bCloseEnd is FALSE) back to the caller. This is because the caller
/// has to be restricted to finding commonSpan Subspan instances from a limited range of
/// indices within arrOld and arrNew, otherwise processing will bog down due to the huge
/// number of possibilities needing to be checked (see the explanation of the SPAN_LIMIT
/// value in AdaptitConstants.h). So the caller won't get upset if the oldEndAt and
/// newEndAt values passed back to it in pSubspan lie beyond the oldEndPos and newEndPos
/// values passed in here - provided that the caller is processing the rightmost afterSpan
/// Subspan - and the latter always must have the bClosedEnd parameter set to FALSE.
/// 
/// Note 3: bounds checking for the index values is done within the called functions for
/// widening to the left and to the right.
////////////////////////////////////////////////////////////////////////////////////////
void WidenMatchup(SPArray& arrOld, SPArray& arrNew,  int oldStartPos, int newStartPos, 
					 int oldEndPos, int newEndPos, int oldMatchLoc, int newMatchLoc, 
					 bool bClosedEnd, Subspan* pSubspan)
{
	wxASSERT(pSubspan->spanType == commonSpan);
	// oldIndex and newIndex will change with each iteration of the loop, the loop each
	// time starts with the passed in oldMatchLoc and newMatchLoc values
	int oldIndex = oldMatchLoc;
	int newIndex = newMatchLoc;
	// oldCount and newCount are passed in (their value doesn't matter), are internally
	// set to zero, and then each stores a count of the CSourcePhrase instances
	// successfully traversed as belonging in the commonSpan - oldCount counts those
	// traversed within arrOld, and newCount counts those traversed within arrNew - and
	// these two counts, while usually both 0 or both 1, can potentially each be different
	// than 1. For example, a matched merger of 5 source phrase words will return 1 for
	// oldCount, but 5 for newCount.
	int oldCount = 0;
	int newCount = 0;
	// accumulate the counts in the following local variables
	int oldAccumulatedLeftCount = 0;
	int newAccumulatedLeftCount = 0;
	int oldAccumulatedRightCount = 0;
	int newAccumulatedRightCount = 0;
	// widen to smaller sequence numbers first (i.e. leftwards), leftwards widening can
	// never be 'open', so the bClosedEnd param value doesn't have any bearing on it
	while (WidenLeftwardsOnce(arrOld,arrNew,oldStartPos,oldEndPos,newStartPos,newEndPos,
			oldIndex,newIndex,oldCount,newCount))
	{
		// update the accumulated counts
		oldAccumulatedLeftCount += oldCount;
		newAccumulatedLeftCount += newCount;

		// update the oldIndex and newIndex values ready for the next iteration, pass in
		// the leftmost matched SourcePhrase instance's index in both arrOld and arrNew
		oldIndex -= oldCount;
		newIndex -= newCount;

		// next two lines are redundant, but retained as they are self-documenting
		oldCount = 0;
		newCount = 0;
	}
    // update the accumulated counts when FALSE was returned (we can't assume both oldCount
    // and newCount will be returned as zero)
	oldAccumulatedLeftCount += oldCount;
	newAccumulatedLeftCount += newCount;

	// now do the same for widening rightwards, in this case, if the parent span is the
	// rightmost (so far defined) afterSpan, then it's bClosedEnd value should be FALSE
	// (meaning the limited span is 'opened' for widening to the end of the arrays) - so
	// WidenRightwardsOnce() requires the bClosedEnd value passed in so it can check, and
	// adjust if necessary, the end bounding index values
	oldCount = 0;
	newCount = 0;
	oldIndex = oldMatchLoc;
	newIndex = newMatchLoc;
	while (WidenRightwardsOnce(arrOld,arrNew,oldStartPos,oldEndPos,newStartPos,newEndPos,
			oldIndex,newIndex,oldCount,newCount,bClosedEnd))
	{
		// update the accumulated counts
		oldAccumulatedRightCount += oldCount;
		newAccumulatedRightCount += newCount;

		// update the oldIndex and newIndex values ready for the next iteration, pass in
		// the rightmost matched SourcePhrase instance's index in both arrOld and arrNew
		oldIndex += oldCount;
		newIndex += newCount;

		// next two lines are redundant, but retained as they are self-documenting
		oldCount = 0;
		newCount = 0;
	}
    // update the accumulated counts when FALSE was returned (we can't assume both oldCount
    // and newCount will be returned as zero)
	oldAccumulatedRightCount += oldCount;
	newAccumulatedRightCount += newCount;
	
	// return the commonSpan index data
	pSubspan->oldStartPos = oldMatchLoc - oldAccumulatedLeftCount;
	pSubspan->oldEndPos = oldMatchLoc + oldAccumulatedRightCount;
	pSubspan->newStartPos = newMatchLoc - newAccumulatedLeftCount;
	pSubspan->newEndPos = newMatchLoc + newAccumulatedRightCount;
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return                     nothing
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
void GetAllCommonSubspansFromOneParentSpan(SPArray& arrOld, SPArray& arrNew, 
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

	// define iterators for scanning across the two spans as defined above
	int oldIndex;
	int newIndex;

	// the outer loop ranges over all the unique words within



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
///                             the index following it, provided that index is in the span
/// \param  newLastIndex     <- index of CSourcePhrase in which the word matched in arrNew,
///                             (but it may not have resulted in a matchup) but we need it
///                             in the caller so that we can retry the call starting from
///                             the index following it, provided that index is in the span
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
	// default initializations
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
	oldLastIndex = wxNOT_FOUND;
	newLastIndex = wxNOT_FOUND;

	// test we are starting within bounds, return FALSE if not
	if (oldStartFrom > oldEndAt || newStartFrom > newEndAt || 
		oldStartFrom < oldStartAt || newStartFrom < newStartAt)
		return FALSE;

	// get the next occurrence of word in arrOld, return wxNOT_FOUND in oldLastIndex 
	// if not found
	oldMatchIndex = FindNextInArray(word, arrOld, oldStartFrom, oldEndAt, phrase);
	if (oldMatchIndex == wxNOT_FOUND)
	{
		return FALSE; // & oldLastIndex is already set to wxNOT_FOUND
	}
	// get the CSourcePhrase's m_key value - what we do depends on whether it is a single
	// word, a merger, or a fixed space conjoining (placeholder ... ellipses are never
	// matched)
	pOldSrcPhrase = arrOld.Item(oldMatchIndex);
	if (pOldSrcPhrase->m_nSrcWords == 1)
	{
		// old instance's key is a single word - see if there is a match in arrNew
		newMatchIndex = FindNextInArray(word, arrNew, newStartFrom, newEndAt, phrase2);
		if (newMatchIndex == wxNOT_FOUND)
		{
			return FALSE; // & newLastIndex is already set to wxNOT_FOUND
		}
		// we have a potential match location, check it out in more detail
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
			newMatchIndex = FindNextInArray(word, arrNew, newStartFrom, newEndAt, phrase2);
			if (newMatchIndex == wxNOT_FOUND)
			{
				return FALSE; // & newLastIndex is already set to wxNOT_FOUND
			}
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
			newMatchIndex = FindNextInArray(word, arrNew, newStartFrom, newEndAt, phrase2);
			if (newMatchIndex == wxNOT_FOUND)
			{
				return FALSE; // & newLastIndex is already set to wxNOT_FOUND
			}
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
			newMatchedStart = leftIndex++;
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
			newMatchedEnd = rightIndex--;
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
/// \param  pSubspan         <- ptr to a commonSpan Subspan instance on the heap which stores
///                             the widened (to both left and right, as far as possible)
///                             spans pair for in-common CSourcePhrase instances (i.e.
///                             unedited ones). If the commonSpan doesn't get defined
///                             properly, the function returns FALSE and the caller must
///                             test for that and detele pSubspan from the heap.                        
/// \remarks
/// For how the parameters are used, see the description of the GetNextMatchup() function.
/// The GetNextCommonSpan() function adds an extra parameter to the former's ones, the
/// pSubspan parameter which is to be defined in scope (ie. starting and ending index
/// values in both arrOld and arrNew) for the CSourcePhrase instances which are in this
/// tentative new commonSpan. We get a matchup location using GetNextCommonSpan(), and
/// then call WidenLeftwardsOnce() in a loop until we can't widen further, than do
/// similarly rightwards using WidenRightwardsOnce(), thereby (providing a failure
/// condition has not been encountered yet by any of those 3 functions) defining the
/// boundaries for the in-common span pair. The returned parameters are used to set the
/// relevant indices in the passed in pSubspan instance, and TRUE is returned; failure to
/// get a properly defined commonSpan causes FALSE to be returned, and in that
/// circumstance the caller must remove pSubspan from the heap. A valid pSubspan must be
/// checked in the caller to make sure it doesn't define an already defined subspan, and
/// provided that is so, the new span is, in the caller, added to the relevant storage
/// array which is of type wxArrayPtrVoid (later, a higher level function will examine all
/// such stored subspans to determine which to keep - the "widest", the rest would get
/// abandoned and their instances removed from the heap).
/// Note 1: GetNextMatchup() doesn't necessarily return indices for just one CSourcePhrase
/// in arrOld, and one in arrNew, for a matchup. A matchup can involve mergers, for
/// instance, or fixedspace conjoined pairs, and so what is returned is, for a valid
/// matchup, the starting and ending index values in arrOld and the same in arrNew. Only
/// when the matchup involves simple one-word CSourcePhrase instances are the starting and
/// ending indices the same for the matchup.
/// Note 2: GetNextMatchup() also initializes to wxNOT_FOUND the following parameters:
/// oldMatchedStart, oldMatchedEnd, newMatchedStart, newMatchedEnd, oldLastIndex, and
/// newLastIndex; alsa oldCount and newCount are internally initialized to 0 at each call.
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
/// Note 4: Each time GetNextCommonSpan() is called, the caller should have initialized the
/// pSubspan indices to oldStartAt & oldEndAt within arrOld, and newStartAt & newEndAt
/// within arrNew. Don't get confused with the intial matchup - which has to return
/// indices with the range oldStartFrom to oldEndAt in arrOld, and newStartFrom to
/// newEndAt in arrNew, because a matchup is being sought at index values at or beyond
/// oldStartFrom in arrOld and newStartFrom in arrNew - up to the bounding values set for
/// each subarray; but the subsequent widening-to-left attempt is allowed to go to index
/// values less than oldStartAt in arrOld, and less than newStartAt in arrNew, because we
/// want our widening to go as wide as there are in-common CSourcePhrase instances within
/// the parent subspan.
bool GetNextCommonSpan(wxString& word, SPArray& arrOld, SPArray& arrNew, int oldStartAt, 
			int newStartAt, int oldStartFrom, int oldEndAt, int newStartFrom, int newEndAt, 
			int& oldMatchedStart, int& oldMatchedEnd, int& newMatchedStart, 
			int& newMatchedEnd, int& oldLastIndex, int& newLastIndex, Subspan* pSubspan)
{
	if (GetNextMatchup(word, arrOld, arrNew, oldStartAt, newStartAt, oldStartFrom,
		oldEndAt, newStartFrom, newEndAt, oldMatchedStart, oldMatchedEnd, newMatchedStart,
		newMatchedEnd, oldLastIndex, newLastIndex))
	{
        // we obtained a valid matchup within the allowed index ranges; now try to widen it
        // to either side, starting with a left widening loop; a return value of FALSE from
        // a widening attempt means "the boundary for widening in that direction has been
        // reached"
		int oldLeftBdryIndex = oldMatchedStart - 1;
		int newLeftBdryIndex = newMatchedStart - 1;
		int oldLeftCount = 0;
		int newLeftCount = 0;
		bool bOK = WidenLeftwardsOnce(arrOld, arrNew, oldStartAt, oldEndAt, newStartAt, 
				newEndAt, oldLeftBdryIndex, newLeftBdryIndex, oldLeftCount, newLeftCount);
		if (bOK)
		{
			// update the left boundary variables
			if (oldLeftCount > 0)
			{
				oldLeftBdryIndex  -= oldLeftCount;
			}
			if (newLeftCount > 0)
			{
				newLeftBdryIndex  -= newLeftCount;
			}
// *** done to here ****


			// use a loop for the rest of the leftwards widening attempts




		}
		else
		{
			// a failure shouldn't involve some widening - but we'll look at the counts
			// and use any values returned as non-zero
			if (oldLeftCount > 0)
			{
				oldLeftBdryIndex  -= oldLeftCount;
			}
			if (newLeftCount > 0)
			{
				newLeftBdryIndex  -= newLeftCount;
			}
		}


		// do rightwards widening now, using a rightwards widening loop
		int oldRightBdryIndex ;
		int newRightBdryIndex;
		int rightOldCount = 0;
		int rightNewCount = 0;




		return TRUE;
	}
	// getting a valid span failed
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

























