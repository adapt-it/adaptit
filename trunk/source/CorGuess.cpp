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


// ////////////////////////////////////////////////////////////////////////
// whm added standard wxWidgets headers below which includes wx.h (and defines the __WXWINDOWS__ symbol needed below)
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "CorGuess.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif
// whm added standard wxWidgets headers above which includes wx.h (and defines the __WXWINDOWS__ symbol needed below)
// ////////////////////////////////////////////////////////////////////////


#ifndef __WXWINDOWS__
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
	iNumInstances = 0;
	iNumExceptions = 0;
	pcorNext = NULL;
	}

Corresp::Corresp( const wxChar* pszSrc1, const wxChar* pszTar1 ) // Constructor that allocates strings for source and target
	{
	pszSrc = new wxChar[ wxStrlen( pszSrc1 ) + 1 ];
	wxStrcpy( pszSrc, pszSrc1 );
	pszTar = new wxChar[ wxStrlen( pszTar1 ) + 1 ];
	wxStrcpy( pszTar, pszTar1 );
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
//	iRequiredSuccessPercent = 50; // 1.5.8va This one not used, stored in guesser
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

void CorrespList::Add( Corresp* pcorNew ) // Add a new correspondence to the end ofthe list
	{
	if ( !pcorFirst ) // If list was empty, new becomes first
		pcorFirst = pcorNew;
	else // Else, previous last points to new
		pcorLast->pcorNext = pcorNew;
	pcorLast = pcorNew; // New becomes last
	}

void CorrespList::Add( const wxChar* pszSrc, const wxChar* pszTar, bool bCount = false ) // Add a new corresponcence to the end of the list
	{
	if ( bCount )
		{
		Corresp* pcorF = pcorFind( pszSrc, pszTar ); // If already in list, increment count
		if ( pcorF )
			{
			pcorF->iNumInstances++;
			return;
			}
		}
	Add( new Corresp( pszSrc, pszTar ) );
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
	corlstSuff.ClearAll(); // Guessed suffixes
	corlstRoot.ClearAll(); // Guessed roots
	corlstPref.ClearAll(); // Guess prefixes
	corlst.ClearAll(); // Raw correspondences given to guesser
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

void Guesser::AddCorrespondence( const wxChar* pszSrc, const wxChar* pszTar ) // Make a correspondence to the list
	{
	if ( iGuessLevel == 0 ) // 1.5.8va 
		return; // 1.5.8va 
	corlst.Add( pszSrc, pszTar );
	}

bool Guesser::bTargetGuess( const wxChar* pszSrc, wxChar** ppszTar ) // Return target guess
	{
	if ( corlstSuff.bIsEmpty() ) // If correspondences have not been calculated, do it now
		{
		Corresp* pcor = NULL;
		Corresp* pcorPrev = NULL;
#ifdef RootAndAffixDiscovery
		for ( pcor = corlst.pcorFirst; pcor; pcor = pcor->pcorNext ) // Make and store all suffix possibilities
			{
			wxChar* pszS = pcor->pszSrc;
			int iLen = wxStrlen( pszS );
			int iMaxLen = iMaxSuffLen;
			if ( iMaxLen > iLen - 1 )
				iMaxLen = iLen - 1;
			wxChar* pszSuffEnd = pszS + iLen;
			wxChar* pszSuffStrt = pszSuffEnd - iMaxLen;
			for ( ; *pszSuffStrt; pszSuffStrt++ )
				corlstSuff.Add( pszSuffStrt, "", true ); // 1.4bs Collect and count possible suffix strings
			}
		Corresp* pcorPrev = NULL;
		pcor = corlstSuff.pcorFirst;
		while ( pcor )
			{
			bool bDelete = false;
			if ( pcor->iNumInstances < iMinSuffExamples ) // Delete all suffixes that have too few occurrences
				bDelete = true;
			if ( bDelete )				
				pcor = corlstSuff.pcorDelete( pcor, pcorPrev );
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
		for ( pcor = corlst.pcorFirst; pcor; pcor = pcor->pcorNext ) // Make and store all suffix correspondences
			{
			wxChar* pszS = pcor->pszSrc;
			wxChar* pszT = pcor->pszTar;
			int iCorrBack = iCorrespondenceBack( pszS, pszT, iStart, iEnd1, iEnd2 ); // See how different guess is from correct
			if ( iCorrBack ) // If there is a correspondence, store it
				{
				pszS += iEnd1;
				pszT += iEnd2;
				corlstSuff.Add( pszS, pszT, true ); // Add to list, count if already there
				}
			}
		pcor = corlstSuff.pcorFirst;
		while ( pcor )
			{
			bool bDelete = false;
			if ( pcor->iNumInstances < iMinSuffExamples ) // Delete all correspondences that occur too few times // 1.5.8va 
				bDelete = true;
			if ( bDelete )				
				pcor = corlstSuff.pcorDelete( pcor, pcorPrev );
			else
				{
				pcorPrev = pcor;
				pcor = pcor->pcorNext;
				}
			}
		for ( Corresp* pcorSuff = corlstSuff.pcorFirst; pcorSuff; pcorSuff = pcorSuff->pcorNext ) // Count exceptions for each correspondence
			{
			wxChar* pszSuffSrc = pcorSuff->pszSrc;
			wxChar* pszSuffTar = pcorSuff->pszTar;
			int iLenSrc = wxStrlen( pszSuffSrc );
			//int iLenTar; // set but unused
			//iLenTar = wxStrlen( pszSuffTar );
			for ( pcor = corlst.pcorFirst; pcor; pcor = pcor->pcorNext ) // Look at each knowledge base pair to see if it is an exception
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
		pcor = corlstSuff.pcorFirst; // Delete correspondences that have less than required success percentage
		while ( pcor )
			{
			int iSuccessPercent = ( pcor->iNumInstances * 100 ) / ( pcor->iNumInstances + pcor->iNumExceptions );
			if ( iSuccessPercent < iRequiredSuccessPercent ) // 1.4bd Make required success ratio a parameter
				pcor = corlstSuff.pcorDelete( pcor, pcorPrev );
			else
				{
				pcorPrev = pcor;
				pcor = pcor->pcorNext;
				}
			}
#endif
		}
	for ( Corresp* pcorSuff = corlstSuff.pcorFirst; pcorSuff; pcorSuff = pcorSuff->pcorNext ) // See if a guess is possible for this string
		{
		**ppszTar = 0; // Clear target if no guess, safer in case caller tries to read it
		wxChar* pszSuffSrc = pcorSuff->pszSrc;
		int iLenSrc = wxStrlen( pszSuffSrc );
		const wxChar* pszEndSrc = pszSrc; // Get end of source for guess
		pszEndSrc += wxStrlen( pszEndSrc ) - iLenSrc;
		if ( !wxStrcmp( pszSuffSrc, pszEndSrc ) ) // If source matches, replace it with target
			{
			if ( wxStrlen( pszSrc ) >= ( MAX_GUESS_LENGTH - 10 ) ) // If not enough room for possible guess, don't try
				return false;
			wxChar* pszSuffTar = pcorSuff->pszTar;
			//int iLenTar; // set but unused
			//iLenTar = wxStrlen( pszSuffTar );
			wxStrcpy( *ppszTar, pszSrc ); // Copy source to target
			wxChar* pszStartReplace = *ppszTar + wxStrlen( pszSrc ) - iLenSrc;
			wxStrcpy( pszStartReplace, pszSuffTar ); // Overwrite end of source with target
			return true;
			}
		}
	return false;
	}
