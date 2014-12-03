////////////////////////////////////////////////////////////////
/// \project	adaptit
/// \file		CorGuess.h
/// \author		Alan Buseman
/// \date_created 1 May 2004
/// \rcs_id $Id$
/// \copyright	2010 SIL International
/// \license	The Common Public License or the GNU Lesser General Public
///				License (see license directory)
/// \description This is the header file for the Correspondence Guesser.
/// The Correspondence Guesser is composed of three classes: Corresp, CorrespList,
/// and the main class Guesser.
////////////////////////////////////////////////////////////////
#ifndef CorGuess_h
#define CorGuess_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CorGuess.h"
#endif

/* Correspondence Guesser takes a list of source/target pairs and tries to guess 
the corresponding target for a source word that is not in the list.
*/

/* Documentation of how to use the correspondence guesser:

Compile and link CorGuess.cpp into your project.
Include CorGuess.h in the files in which you use the guesser.

Interface consists of one class and 3 functions:
	Guesser gue;
	void gue.Init( int iGuessLevel ); // Clear old correspondences, prepare to start new load, set guess level
	void gue.AddCorrespondence( const wxChar* pszSrc, const wxChar* pszTar, int iFreq = 1 ); // Add a correspondence to the list
	bool gue.bTargetGuess( const wxChar* pszSrc, wxChar** ppszTar ); // Return target guess

To use the correspondence guesser:
1. To load a knowledge base into the guesser, call AddCorrespondence once for each correspondence pair.
2. To ask for a guess, call bTargetGuess.
3. To add a new correspondence pair to an existing set of correspondences, call AddCorrespondence with it.
4. To clear all correspondences to prepare for a fresh load, call Init.
5. To change the guess level, call Init and reload the knowledge base into the guesser.
	Guess level is between 0 and 100 where 0 is no guessing, 50 is conservative, 100 is wild guessing.

Sample calling code:
	#include "CorGuess.h"
	
	Guesser gue;
	gue.Init( iGuessLevel ); // Optionally set the initial guess level, default is 50 on a scale of 0-100.
	for ( ... ) // For each knowledge base pair
		gue.AddCorrespondence( pszSrc, pszTar, iFreq ); // Add pair to list correspondence guesser's list

	if ( ... ) // If word not found in knowledge base
		{
		wxChar* pszGuess = (wxChar*)_alloca( MAX_GUESS_LENGTH ); // Alloc space to pass as pointer, 100 is enough
		if ( gue.bTargetGuess( pszWord, &pszGuess ) ) // If guesser does a guess, show it to user in place of the source word
			...; // Give user the guess instead of the source word for editing into target form, possibly with special color or font to let user know it is a guess
		}

The optional iFreq frequency argument to AddCorrespondence has two purposes. If it is a positive
number, it is understood to give the number of times this correspondence has been observed in
adaptation. If it is -1, this correspondence is understood to be a prefix. If it is -2, this
correspondence is understood to be a suffix. If it is zero, this correspondence is understood
to be a root. The default value is 1, a standard word pair with a low frequency of occurrence.

There is no way to delete or modify a correspondence. If the user edits an existing target form, the
easiest is not to tell the guesser anything. The guesser is statistical and needs multiple examples
to form a correspondence, so missing a modification is unlikely to make any significant difference.
The next time the user starts the program or the guesser is reloaded, the guesser's correspondence 
list will be freshened.

*/

#ifdef _MBCS // If not wxWidgets, define back to generic char and string // 1.6.1ba 
#define wxChar char
#define wxStrlen strlen
#define wxStrcpy strcpy
#define wxStrcat strcat
#define wxStrcmp strcmp
#endif

#define MAX_GUESS_LENGTH 1000 // Maximum length of guess, length of buffer for return of guess

class Corresp // cor Correspondence struct, one for each correspondence noted
	{
public:
	wxChar* pszSrc; // Source of correspondence
	wxChar* pszTar; // Target of correspondence
	int iFreq; // Frequency of correspondence in kb as fed in // 1.6.1aa 
	int iNumInstances; // Number of instances found for guess
	int iNumExceptions; // Number of exceptions found for guess
	Corresp* pcorNext; // Pointer to next correspondence in linked list
	Corresp();
	Corresp( const wxChar* pszSrc1, const wxChar* pszTar1, int iFreq );
	~Corresp();
	};

class CorrespList // corlst List of correspondence structures
	{
public:
	Corresp* pcorFirst; // First correspondence in the full list
	int iLen; // 1.6.1dc Add len to correspondence list
protected:
	Corresp* pcorLast; // Last correspondence in the full list
	void Add( Corresp *pcorNew ); // Helper function for Add new correspondence
public:
	CorrespList();
	~CorrespList();
	void ClearAll(); // 1.4vyd 
	void Add( const wxChar* pszSrc, const wxChar* pszTar, int iFreq ); // Add a new correspondence to the end ofthe list // 1.6.1ad if iFreq is zero, just count if existing 
	void SortLongestFirst(); // 1.6.1bd Sort longest first
	void SortLongestLast(); // 1.6.1bf Sort longest last
	bool bIsEmpty() { return pcorFirst == NULL; } // Return true if list is empty
	Corresp* pcorFind( const wxChar* pszSrc, const wxChar* pszTar ); // Find the same pair, return NULL if not found
	Corresp* pcorFind( const wxChar* pszSrc ); // Find the same source, return NULL if not found
	Corresp* pcorDelete( Corresp* pcor, Corresp* pcorPrev ); // Delete a correspondence from the list, return next after deleted one
	};

class CorrespListKB : public CorrespList // corlst List of KB correspondence structures // 1.6.1bc 
	{
public:
	void Add( const wxChar* pszSrc, const wxChar* pszTar, int iFreq ); // Add a new correspondence to the list // 1.6.1bc
	};

class Guesser // gue Guesser, main class that holds everything
{
protected: // Not to be changed by user, carefully tuned for maximum performance
	int iRequiredSuccessPercent; // Required percentage of successes
	int iMaxSuffLen; // Longest suffix length to be considered
	int iMinSuffExamples; // Minimum number of examples of suffix to be considered
	int iGuessLevel; // Level of guess, 0 is no guessing, 50 is conservative, 100 is wild guessing
	bool bInit; // 1.6.1df Used to avoid calculate corresp during intial load
protected:
	CorrespListKB corlstKB; // Raw correspondences given to guesser
	CorrespList corlstSuffGiven; // Given suffixes // 1.6.1ac Add place to store given affixes in guesser
	CorrespList corlstRootGiven; // Given roots // 1.6.1ac 
	CorrespList corlstPrefGiven; // Given prefixes // 1.6.1ac 
	CorrespList corlstSuffGuess; // Guessed suffixes // 1.6.1ab Distinguish guessed affixes from given
	CorrespList corlstRootGuess; // Guessed roots // 1.6.1ab 
	CorrespList corlstPrefGuess; // Guessed prefixes // 1.6.1ab
	void CalculateCorrespondences(); // Calculate correspondences // 1.6.1aj Make this a function

	bool bRootReplace( const wxChar* pszSrc, wxChar** ppszTar ); // Try to replace a root // 1.6.1aj 
	bool bPrefReplace( const wxChar* pszSrc, wxChar** ppszTar, bool bReplace = true ); // Try to replace a prefix // 1.6.1aj 
	bool bSuffReplace( const wxChar* pszSrc, wxChar** ppszTar, bool bReplace = true ); // Try to replace a suffix // 1.6.1aj 
public:
	void DoCalcCorrespondences(); // accessor for CalculateCorrespondences()
	Guesser();
	void Init( int iGuessLevel1 = 50 ); // 1.5.8u Change ClearAll to Init, add guess level
	void AddCorrespondence( const wxChar* pszSrc, const wxChar* pszTar, int iFreq = 1 ); // Add a correspondence to the list // 1.6.1aa Add frequency arg to guesser AddCorrespondence
	bool bTargetGuess( const wxChar* pszSrc, wxChar** ppszTar ); // Return target guess
};

#endif // CorGuess_h
