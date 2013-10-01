////////////////////////////////////////////////////////////////
/// \project	adaptit
/// \file		CorGuess.cpp
/// \author		Alan Buseman
/// \date_created 1 May 2004
/// \rcs_id $Id$
/// \copyright	2010 SIL International
/// \license	The Common Public License or the GNU Lesser General Public
///				License (see license directory)
/// \description This is the implementation file for the Correspondence Guesser.
/// The Correspondence Guesser is composed of three classes: Corresp, CorrespList,
/// and the main class Guesser.
////////////////////////////////////////////////////////////////

// For documentation see CorGuess.h


#ifndef _MBCS // 1.6.0ad Update CorGuess.cpp for compiling in Adapt It // 1.6.1ba Update guesser to test _MCBS for non wxWidgets
// ////////////////////////////////////////////////////////////////////////
// whm added standard wxWidgets headers below which includes wx.h
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CorGuess.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif
// whm added standard wxWidgets headers above which includes wx.h
// ////////////////////////////////////////////////////////////////////////
#endif // 1.6.0ad 

#ifdef _MBCS // 1.6.1ba 
#include <string.h> // Only generic string functions used
#endif

#include "CorGuess.h"

int iCorrespondenceBack( wxChar* psz1, wxChar* psz2, int& iStart, int& iEnd1, int& iEnd2 ) // Return length of correspondence from start
	{
	// Find a difference, and return start and end of difference
	//int iDiff; // set but unused
	//iDiff = 0;
	iStart = 0;
	iEnd1 = 0;
	iEnd2 = 0;
	int iLen1 = wxStrlen( psz1 );
	int iLen2 = wxStrlen( psz2 );
	int i1 = iLen1 - 1;
	int i2 = iLen2 - 1;
	while ( i1 > 1 && i2 > 1 ) // Look for first difference
		{
		if ( *(psz1+i1) != *(psz2+i2) ) // If mismatch, note difference
			{
			iStart = i1; // Note start of difference
			bool bSucc = false;
			while ( true ) // Look for end of difference
				{
				if ( i1 < iLen1 - 1 || i2 < iLen2 - 1 ) // If not at the very end, check for one wxChar insert or delete // 1.4bp Allow one letter added at end (made it worse)
					{
					if ( *(psz1+i1-1) == *(psz2+i2) 
							&& *(psz1+i1-2) == *(psz2+i2-1) ) // If match at one wxChar diff, note end of difference
						{
						iEnd1 = i1;
						iEnd2 = i2 + 1;
						bSucc = true;
						break;
						}
					else if ( *(psz1+i1) == *(psz2+i2-1) 
							&& *(psz1+i1-1) == *(psz2+i2-2) ) // If match at one wxChar diff, note end of difference
						{
						iEnd1 = i1 + 1;
						iEnd2 = i2;
						bSucc = true;
						break;
						}
					}
				i1--; // Step to prev wxChar
				i2--;
				if ( *(psz1+i1) == *(psz2+i2) // If match, note end of difference
						&& ( *(psz1+i1-1) == *(psz2+i2-1) // Match two letters
							|| i1 < iLen1 - 3 ) ) // Or have length of at least 3
					{
					iEnd1 = i1 + 1;
					iEnd2 = i2 + 1;
					bSucc = true;
					break;
					}
				else if ( i1 <= 1 || i2 <= 1 ) // If different all the way to front, return failure
					return 0;
				}
			if ( bSucc )
				{
				if ( iEnd1 > 0 && iEnd1 == iLen1 - 1 ) // If correspondence only matched one wxChar, match 2 instead
					{
					iEnd1--;
					iEnd2--;
					}
				if ( iLen1 - iEnd1 > 5 ) // If longer than 5, don't do it as a back correspondence
					return 0;
				return 1;
				}
			}
		i1--; // Step to next wxChar
		i2--;
		}
	return 0; // Return no correspondence
	}

Corresp::Corresp()
	{
	pszSrc = NULL;
	pszTar = NULL;
	iFreq = 0; // 1.6.1ad 
	iNumInstances = 0;
	iNumExceptions = 0;
	pcorNext = NULL;
	}

Corresp::Corresp( const wxChar* pszSrc1, const wxChar* pszTar1, int iFreq ) // Constructor that allocates strings for source and target
	{
	pszSrc = new wxChar[ wxStrlen( pszSrc1 ) + 1 ];
	wxStrcpy( pszSrc, pszSrc1 );
	pszTar = new wxChar[ wxStrlen( pszTar1 ) + 1 ];
	wxStrcpy( pszTar, pszTar1 );
	iFreq = iFreq; // 1.6.1ad 
	iNumInstances = 1;
	iNumExceptions = 0;
	pcorNext = NULL;
	}

Corresp::~Corresp()
	{
	if ( pszSrc )
		delete pszSrc;
	if ( pszTar )
		delete pszTar;
	}

CorrespList::CorrespList()
	{
	pcorFirst = NULL;
	pcorLast = NULL;
	}

CorrespList::~CorrespList()
	{
	ClearAll(); // 1.4vyd 
	}

void CorrespList::ClearAll() // 1.4vyd Add ClearAll function
	{
	while ( pcorFirst )
		{
		Corresp* pcor = pcorFirst->pcorNext;
		delete pcorFirst;
		pcorFirst = pcor;
		}
	}

void CorrespList::Add( Corresp* pcorNew ) // Add a new correspondence to the end ofthe list // 1.6.1af Change to top of list
	{
	pcorNew->pcorNext = pcorFirst; // 1.6.1af Always add correspondence to top of list
	if ( !pcorFirst ) // 1.6.1af If list was empty, need to set last
		pcorLast = pcorNew; // 1.6.1af New becomes last
	pcorFirst = pcorNew; // 1.6.1af 
#ifdef AddLast
	if ( !pcorFirst ) // If list was empty, new becomes first
		pcorFirst = pcorNew;
	else // Else, previous last points to new
		pcorLast->pcorNext = pcorNew;
	pcorLast = pcorNew; // New becomes last
#endif
	}

void CorrespList::Add( const wxChar* pszSrc, const wxChar* pszTar, int iFreq ) // Add a new corresponcence to the end of the list
{
	Corresp* pcorF = pcorFind( pszSrc, pszTar ); // If already in list, increment count or check frequency
	if ( pcorF )
		{
		if ( iFreq == 0 ) // 1.6.1ad If iFreq is zero, count if in list
			pcorF->iNumInstances++;
		else if ( iFreq > pcorF->iFreq ) // 1.6.1ae If higher frequency, replace previous with this
			{
// #ifdef NotNeeded // 1.6.1bb Since source and target are the same, no need to replace them
			delete pcorF->pszSrc;
			delete pcorF->pszTar;
			pcorF->pszSrc = new wxChar[ wxStrlen( pszSrc ) + 1 ];
			wxStrcpy( pcorF->pszSrc, pszSrc );
			pcorF->pszTar = new wxChar[ wxStrlen( pszTar ) + 1 ];
			wxStrcpy( pcorF->pszTar, pszTar );
// #endif // 1.6.1bb 
			pcorF->iFreq = iFreq; // 1.6.1ae Store new frequency
			}
		return;
		}
	Add( new Corresp( pszSrc, pszTar, iFreq ) );
	}

Corresp* CorrespList::pcorFind( const wxChar* pszSrc, const wxChar* pszTar ) // Find the same pair, return NULL if not found
	{
	for ( Corresp* pcor = pcorFirst; pcor; pcor = pcor->pcorNext )
		{
		if ( !wxStrcmp( pszSrc, pcor->pszSrc ) && !wxStrcmp( pszTar, pcor->pszTar ) )
			return pcor;
		}
	return NULL;
	}

Corresp* CorrespList::pcorDelete( Corresp* pcor, Corresp* pcorPrev ) // Delete a correspondence from the list, return next after deleted one
	{
	Corresp* pcorNext = pcor->pcorNext;
	if ( !pcorPrev ) // If first in list, then point top at next
		pcorFirst = pcorNext;
	else
		pcorPrev->pcorNext = pcorNext; // else point prev at next
	delete pcor;
	return pcorNext;
	}

Guesser::Guesser()
	{
	iRequiredSuccessPercent = 30; // Init required success percent
	iMaxSuffLen = 5; // Init max suffix length
	iMinSuffExamples = 2; // Minimum number of examples of suffix to be considered
	iGuessLevel = 50; // 1.5.8u Default guess level to conservative
	}

// =========== Start Main Routines
void Guesser::Init( int iGuessLevel1 ) // 1.4vyd Add ClearAll function // 1.5.8u Change to Init, add guess level
	{
	corlstSuffGuess.ClearAll(); // Guessed suffixes
	corlstRootGuess.ClearAll(); // Guessed roots
	corlstPrefGuess.ClearAll(); // Guessed prefixes
	corlstSuffGiven.ClearAll(); // Given suffixes
	corlstRootGiven.ClearAll(); // Given roots
	corlstPrefGiven.ClearAll(); // Given prefixes
	corlstKB.ClearAll(); // Raw correspondences given to guesser
	iGuessLevel = iGuessLevel1; // 1.5.8u Set guess level
	if ( iGuessLevel >= 50 )
		iRequiredSuccessPercent = 30 - ( ( ( iGuessLevel - 50 ) * 100 ) / 170 ); // 1.5.8va
	else
		iRequiredSuccessPercent = 30 + ( ( ( 50 - iGuessLevel ) * 100 ) / 72 ); // 1.5.8va
	if ( iGuessLevel >= 80 ) // 1.5.8va 
		iMinSuffExamples = iMinSuffExamples - 1; // 1.5.8va Allow fewer examples
	else if ( iGuessLevel <= 20 ) // 1.5.8va 
		iMinSuffExamples = iMinSuffExamples + 1; // 1.5.8va Require more examples
	}

void Guesser::AddCorrespondence( const wxChar* pszSrc, const wxChar* pszTar, int iFreq ) // Make a correspondence to the list
	{
	if ( iGuessLevel == 0 ) // 1.5.8va If guesser turned off, don't store correspondences
		return; // 1.5.8va 
	if ( iFreq == -1 ) // 1.6.1ad If given prefix store as that
		corlstPrefGiven.Add( pszSrc, pszTar, 10000 ); // 1.6.1ad Store as given prefix // 1.6.1ag Use high freq
	else if ( iFreq == -2 ) // 1.6.1ad If given suffix store as that
		corlstSuffGiven.Add( pszSrc, pszTar, 10000 ); // 1.6.1ad Store as given suffix // 1.6.1ag Use high freq
	else if ( iFreq == 0 ) // 1.6.1ad If given root store as that
		corlstRootGiven.Add( pszSrc, pszTar, 10000 ); // 1.6.1ad Store as given prefix // 1.6.1ag Use high freq
	else
		corlstKB.Add( pszSrc, pszTar, iFreq );
	}

void Guesser::CalculateCorrespondences() // Calculate correspondences // 1.6.1aj Make this a function
	{
	if ( corlstSuffGuess.bIsEmpty() ) // If correspondences have not been calculated, do it now
		{
		Corresp* pcor = NULL;
		Corresp* pcorPrev = NULL;
#ifdef RootAndAffixDiscovery
		for ( pcor = corlstKB.pcorFirst; pcor; pcor = pcor->pcorNext ) // Make and store all suffix possibilities
			{
			wxChar* pszS = pcor->pszSrc;
			int iLen = wxStrlen( pszS );
			int iMaxLen = iMaxSuffLen;
			if ( iMaxLen > iLen - 1 )
				iMaxLen = iLen - 1;
			wxChar* pszSuffEnd = pszS + iLen;
			wxChar* pszSuffStrt = pszSuffEnd - iMaxLen;
			for ( ; *pszSuffStrt; pszSuffStrt++ )
				corlstSuffGuess.Add( pszSuffStrt, "", 0 ); // 1.4bs Collect and count possible suffix strings
			}
		Corresp* pcorPrev = NULL;
		pcor = corlstSuffGuess.pcorFirst;
		while ( pcor )
			{
			bool bDelete = false;
			if ( pcor->iNumInstances < iMinSuffExamples ) // Delete all suffixes that have too few occurrences
				bDelete = true;
			if ( bDelete )				
				pcor = corlstSuffGuess.pcorDelete( pcor, pcorPrev );
			else
				{
				pcorPrev = pcor;
				pcor = pcor->pcorNext;
				}
			}
#endif
#define CorrespondenceGuess
#ifdef CorrespondenceGuess
		int iStart, iEnd1, iEnd2 = 0;
		for ( pcor = corlstKB.pcorFirst; pcor; pcor = pcor->pcorNext ) // Make and store all suffix correspondences
			{
			wxChar* pszS = pcor->pszSrc;
			wxChar* pszT = pcor->pszTar;
			int iCorrBack = iCorrespondenceBack( pszS, pszT, iStart, iEnd1, iEnd2 ); // See how different guess is from correct
			if ( iCorrBack ) // If there is a correspondence, store it
				{
				pszS += iEnd1;
				pszT += iEnd2;
				corlstSuffGuess.Add( pszS, pszT, 0 ); // Add to list, count if already there
				}
			}
		pcor = corlstSuffGuess.pcorFirst;
		while ( pcor )
			{
			bool bDelete = false;
			if ( pcor->iNumInstances < iMinSuffExamples ) // Delete all correspondences that occur too few times // 1.5.8va 
				bDelete = true;
			if ( bDelete )				
				pcor = corlstSuffGuess.pcorDelete( pcor, pcorPrev );
			else
				{
				pcorPrev = pcor;
				pcor = pcor->pcorNext;
				}
			}
		for ( Corresp* pcorSuff = corlstSuffGuess.pcorFirst; pcorSuff; pcorSuff = pcorSuff->pcorNext ) // Count exceptions for each correspondence
			{
			wxChar* pszSuffSrc = pcorSuff->pszSrc;
			wxChar* pszSuffTar = pcorSuff->pszTar;
			int iLenSrc = wxStrlen( pszSuffSrc );
			//int iLenTar; // set but unused
			//iLenTar = wxStrlen( pszSuffTar );
			for ( pcor = corlstKB.pcorFirst; pcor; pcor = pcor->pcorNext ) // Look at each knowledge base pair to see if it is an exception
				{
				wxChar* pszEndSrc = pcor->pszSrc; // Get end of source of kb pair
				pszEndSrc += wxStrlen( pszEndSrc ) - iLenSrc;
				if ( !wxStrcmp( pszSuffSrc, pszEndSrc ) ) // If source matches, see if target matches, if not this is an exception
					{
					wxChar* pszEndTar = pcor->pszTar; // Get end of target of kb pair
					pszEndTar += wxStrlen( pszEndTar ) - iLenSrc;
					if ( wxStrcmp( pszSuffTar, pszEndTar ) ) // If exception, count it
						pcorSuff->iNumExceptions++;
					}
				}
			}

		pcorPrev = NULL;
		pcor = corlstSuffGuess.pcorFirst; // Delete correspondences that have less than required success percentage
		while ( pcor )
			{
			int iSuccessPercent = ( pcor->iNumInstances * 100 ) / ( pcor->iNumInstances + pcor->iNumExceptions );
			if ( iSuccessPercent < iRequiredSuccessPercent ) // 1.4bd Make required success ratio a parameter
				pcor = corlstSuffGuess.pcorDelete( pcor, pcorPrev );
			else
				{
				pcorPrev = pcor;
				pcor = pcor->pcorNext;
				}
			}
		for ( pcor = corlstSuffGiven.pcorFirst; pcor; pcor = pcor->pcorNext )  // 1.6.1ag Add given affixes to guess list
			corlstSuffGuess.Add( pcor->pszSrc, pcor->pszTar, pcor->iFreq ); // 1.6.1ag 
		for ( pcor = corlstPrefGiven.pcorFirst; pcor; pcor = pcor->pcorNext )  // 1.6.1ag Add given affixes to guess list
			corlstPrefGuess.Add( pcor->pszSrc, pcor->pszTar, pcor->iFreq ); // 1.6.1ag 
		for ( pcor = corlstRootGiven.pcorFirst; pcor; pcor = pcor->pcorNext )  // 1.6.1ag Add given affixes to guess list
			corlstRootGuess.Add( pcor->pszSrc, pcor->pszTar, pcor->iFreq ); // 1.6.1ag 
#endif
		}
	}

bool bStrMatch( const wxChar* pszS, const wxChar* pszMatch, int iStart ) // 1.6.1aj Utility function to test for match of pszMatch
	{
	const wxChar* pszT = pszS + iStart; // 1.6.1aj Set starting location
	while ( *pszMatch != 0 ) // 1.6.1aj While more in match
		{
		if ( *pszT++ != *pszMatch++ )
			return false;
		}
	return true;
	}

int iStrFind( const wxChar* pszS, const wxChar* pszFind ) // 1.6.1aj Utility function to find pszFind
	{
	int iLen = wxStrlen( pszS ); // 1.6.1aj Temp var to preven unsigned mismatch
	for ( int i = 0; i < iLen ; i++ ) // 1.6.1aj At each location, check for match
		if ( bStrMatch( pszS, pszFind, i ) )
			return i; // 1.6.1aj Return place found for success
	return -1; // 1.6.1aj Return -1 for failure
	}

void StrReplace( wxChar* pszS, wxChar* pszReplace, int iLoc, int iNum ) // 1.6.1aj Replace a section of pszS with pszReplace
	{
	wxChar* pszTail = new wxChar[ wxStrlen( pszS ) + 1 ]; // 1.6.1aj Temp storage of tail
	wxStrcpy( pszTail, pszS + iLoc + iNum ); // 1.6.1aj Save tail
	wxStrcpy( pszS + iLoc, _T("") ); // 1.6.1bb Terminate head
	wxStrcat( pszS, pszReplace ); // 1.6.1aj Copy replace in // 1.6.1bb Get rid of pointer arithmetic
	wxStrcat( pszS, pszTail ); // 1.6.1aj Put tail back on // 1.6.1bb 
	delete pszTail;
	}

bool Guesser::bRootReplace( const wxChar* pszSrc, wxChar** ppszTar ) // Try to replace a root // 1.6.1aj 
	{
	for ( Corresp* pcorRoot = corlstRootGuess.pcorFirst; pcorRoot; pcorRoot = pcorRoot->pcorNext ) // See if a guess is possible for this string
		{
		wxStrcpy( *ppszTar, pszSrc ); // Copy source to target, so caller can use it if no replace
		wxChar* pszRootSrc = pcorRoot->pszSrc; // Pointer to root to match
		wxChar* pszRootTar = pcorRoot->pszTar; // 1.6.1aj Pointer to target root
		int iFind = iStrFind( pszSrc, pszRootSrc ); // 1.6.1aj See if root found
		if ( iFind >= 0 ) // 1.6.1aj If root found, replace it
			{
			StrReplace( *ppszTar, pszRootTar, iFind, wxStrlen( pszRootSrc ) ); // 1.6.1aj Replace root
			return true;
			}
		}
	return false;
	}

bool Guesser::bSuffReplace( const wxChar* pszSrc, wxChar** ppszTar, bool bReplace ) // Try to replace a suffix // 1.6.1aj 
	{
	for ( Corresp* pcorSuff = corlstSuffGuess.pcorFirst; pcorSuff; pcorSuff = pcorSuff->pcorNext ) // See if a guess is possible for this string
		{
		wxStrcpy( *ppszTar, pszSrc ); // Copy source to target, so caller can use it if no replace
		if ( !bReplace ) // 1.6.1aj If no replace, return target same as source
			return false;
		wxChar* pszSuffSrc = pcorSuff->pszSrc; // Pointer to suffix to match
		int iLenSuffSrc = wxStrlen( pszSuffSrc ); // Get length of suffix to match
		const wxChar* pszEndSrc = pszSrc + wxStrlen( pszSrc ) - iLenSuffSrc; // Get start of end of source for match
		if ( !wxStrcmp( pszSuffSrc, pszEndSrc ) ) // If source matches, replace it with target
			{
			wxChar* pszSrcShortened = new wxChar[ wxStrlen( pszSrc ) + 1 ]; // 1.6.1aj Shortened source to try further suff
			wxStrcpy( pszSrcShortened, pszSrc ); // Copy source to shortened source
//			*(pszSrcShortened + wxStrlen( pszSrc ) - iLenSuffSrc) = 0; // 1.6.1aj Shorten source
			wxStrcpy( pszSrcShortened + wxStrlen( pszSrc ) - iLenSuffSrc, _T("") ); // 1.6.1aj Shorten source // 1.6.1bb 
			bSuffReplace( pszSrcShortened, ppszTar, iGuessLevel >= 50 ); // 1.6.1aj Try replace suff on shortened source, but only if guess level 50 or more
			wxStrcat( *ppszTar, pcorSuff->pszTar ); // 1.6.1aj Append target suff to end of target
			delete pszSrcShortened; // 1.6.1aj 
			return true;
			}
		}
	return false;
	}

bool Guesser::bPrefReplace( const wxChar* pszSrc, wxChar** ppszTar, bool bReplace ) // Try to replace a prefix // 1.6.1aj 
	{
	for ( Corresp* pcorPref = corlstPrefGuess.pcorFirst; pcorPref; pcorPref = pcorPref->pcorNext ) // See if a guess is possible for this string
		{
		wxStrcpy( *ppszTar, pszSrc ); // Copy source to target, so caller can use it if no replace
		if ( !bReplace ) // 1.6.1aj If no replace, return target same as source
			return false;
		wxChar* pszPrefSrc = pcorPref->pszSrc; // Pointer to prefix to match
		int iLenPrefSrc = wxStrlen( pszPrefSrc ); // Get length of prefix to match
		if ( bStrMatch( pszSrc, pszPrefSrc, 0 ) ) // If source matches, replace it with target
			{
			wxChar* pszSrcShortened = new wxChar[ wxStrlen( pszSrc ) + 1 ]; // 1.6.1aj Shortened source to try further pref
			wxStrcpy( pszSrcShortened, pszSrc + iLenPrefSrc ); // Copy source to shortened source
			bPrefReplace( pszSrcShortened, ppszTar, iGuessLevel >= 50 ); // 1.6.1aj Try replace pref on shortened source, but only if guess level 50 or more
			StrReplace( *ppszTar, pcorPref->pszTar, 0, 0 ); // 1.6.1aj Prepend target pref to end of target
			delete pszSrcShortened; // 1.6.1aj 
			return true;
			}
		}
	return false;
	}

bool Guesser::bTargetGuess( const wxChar* pszSrc, wxChar** ppszTar ) // Return target guess
	{
	if ( corlstSuffGuess.bIsEmpty() ) // If correspondences have not been calculated, do it now
		CalculateCorrespondences(); // 1.6.1aj 
	bool bSucc = false; // 1.6.1aj 
	if ( bRootReplace( pszSrc, ppszTar ) ) // 1.6.1aj 
		bSucc = true;
	if ( bSuffReplace( *ppszTar, ppszTar ) ) // 1.6.1aj 
		bSucc = true;
	if ( bPrefReplace( *ppszTar, ppszTar ) ) // 1.6.1aj 
		bSucc = true;
	return bSucc; // 1.6.1aj 
	}
