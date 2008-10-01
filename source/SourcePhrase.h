/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			SourcePhrase.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CSourcePhrase class. 
/// The CSourcePhrase class represents what could be called a "TranslationUnit".
/// When the input source text is parsed, each word gets stored on one instance
/// of CSourcePhrase on the heap. Mergers of source text words cause their
/// respective CSourcePhrase instances to be merged to a single one, storing
/// the resulting phrase. CSourcePhrase also stores the final associated target 
/// text adaptation/translation, along with a number of different attributes 
/// associated with the overall translation unit, including sfm markers, preceeding
/// and following punctuation, its sequence number, text type, etc.
/// \derivation		The CSourcePhrase class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef SourcePhrase_h
#define SourcePhrase_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "SourcePhrase.h"
#endif

// forward declarations
class CRefString;
class CTargetUnit;
class CAdapt_ItView;
class CBString;
class wxDataOutputStream; // needed for SerializeOut() function
class wxDataInputStream; // needed for SerializeIn() function
//#include "XML.h"

class CSourcePhrase; // This forward reference is needed because the macro 
// declaration below must be in general namespace, before CSourcePhrase is declared.
// The macro below together with the macro list declaration in the .cpp file
// define a new list class called SPList. Its list elements are of type CSourcePhrase.
// wxList uses this macro for its declaration

/// wxList declaration and partial implementation of the SPList class being
/// a list of pointers to CSourcePhrase objects
WX_DECLARE_LIST(CSourcePhrase, SPList); // see list definition macro in .cpp file

// globals
// whm Note: If textTypes are added or removed from this enum, be sure to
// make adjustments to AtSFMAttr() and ParseXMLAttr() in XML.cpp
// From version 3.x, the unused TextType enum are commented out, but the
// underlying enum numbering forced to remain the same as for prior 
// versions for backward compatibility.
enum TextType {
	noType,
	verse,
	poetry,
	sectionHead,
	mainTitle,
	secondaryTitle,
	none, // = 6, BEW added 23May05 for inline marker pairs which don't change the TextType; these
		  // currently are: ord, bd, it, em, bdit, sc, pro, ior, w, wr, wh, wg, ndx, k, pn, qs, fk, xk
		  // (this type is used for suppression of the normal TextType and BOOL setting, so that
		  // the application will process alternative code for the special cases,  - especially
		  // the restoration of the earlier TextType and m_bSpecialText value after an endmarker,
		  // and the appropriate treatment of m_bFirstOfType & m_bBoundary (on pLast in 
		  // AnalyzeMarker() ); and we will include footnotes in this behaviour even though we 
		  // do not set them to have this TextType value - the doc functions AnalyzeMarker() and 
		  // IsTextTypeNoneOrFootnote() implement this protocol)
	footnote = 9,
	header,
	identification,
	rightMarginReference = 32,
	crossReference,
	note
};



/// The CSourcePhrase class represents what could be called a "TranslationUnit".
/// When the input source text is parsed, each word gets stored on one instance
/// of CSourcePhrase on the heap. Mergers of source text words cause their
/// respective CSourcePhrase instances to be merged to a single one, storing
/// the resulting phrase. CSourcePhrase also stores the final associated target 
/// text adaptation/translation, along with a number of different attributes 
/// associated with the overall translation unit, including sfm markers, preceeding
/// and following punctuation, its sequence number, text type, etc.
/// \derivation		The CSourcePhrase class is derived from wxObject.
class CSourcePhrase : public wxObject
{
public:
	CSourcePhrase(void); // constructor
	
	// copy constructor (not a deep copy, the m_pSavedWords CObList member has only the pointers copied;
	// a deep copy would have been better in hindsight, but 8 years later it is too late, so in 2008
	// I have added a DeepCopy() member function to obtain a deep copy when it is wanted - eg. as when
	// I refactored the Edit Source Text functionality)
	CSourcePhrase(const CSourcePhrase& sp);

	// destructor
	virtual ~CSourcePhrase(void);

	// methods

protected:

	// implementation & interface
public:
	wxString		m_inform;		// text to be shown above the strip when info type changes
	wxString		m_chapterVerse; // text for the chapter and verse is stored here, as required
	bool			m_bFirstOfType; // TRUE when textType changes. Used for extending a type to
									//  succeeding source phrases (with this flag FALSE), until a 
									// new TRUE value is set
	bool			m_bFootnoteEnd;	// TRUE for the last source phrase in a section of footnote text
	bool			m_bFootnote;	// TRUE for the first source phrase in a section of footnote text
	bool			m_bChapter;		// TRUE if the source phrase is first one in a new chapter
	bool			m_bVerse;		// TRUE if the source phrase is the first one in a new verse
	bool			m_bParagraph;   // TRUE if the source phrase is the first one after a \p marker
	bool			m_bSpecialText; // TRUE if the text is special, such as \id contents, or 
									// subheading, etc
	bool			m_bBoundary;	// marks right boundary for selection extension, to prevent spurious
									// source phrases being created by the merge operation
	bool			m_bHasInternalMarkers; // if TRUE, user will have to manually located the target
									   // markers using a dialog in the Export... function
	bool			m_bHasInternalPunct; // if TRUE, user will be asked to manually locate target 
										// punctuation
	bool			m_bRetranslation;	 // set when srcPhrase > 10 words, or target text approaches
									 // docWidth, or user explicitly marks a selection as a
									 // retranslation, rather than an adaption; 
									 // - scrPhrases so marked are not entered in KB 
	bool			m_bNotInKB; // set TRUE if user specifies no KB map entry for this adaption, and 
								// obligatorily TRUE for retranslations, null source phrases, and 
								// too-long phrases
	bool			m_bNullSourcePhrase; // TRUE if this is a null src phrase inserted by the user 
										// (shows as ...)
	bool			m_bHasKBEntry;		// TRUE when m_key of this source phrase is used to put an entry in
										// the KB


	// two new flags for version_number == 2, 
	// these flags used for preventing concatenation of retranslations
	// *** DONT FORGET TO UPDATE THE COPY CONSTRUCTOR AND THE OVERLOADED ASSIGNMENT OPERATOR AS WELL!
	bool			m_bBeginRetranslation;
	bool			m_bEndRetranslation;

	TextType		m_curTextType;	// from the global enum. Used for setting the text type tag to 
									// be shown in grey above line 1 of each strip
	SPList*			m_pSavedWords;	// save list of CSourcePhrase instances comprising the phrase
	wxString		m_adaption;		// a copy of the adaption (line 3) text, if user wants no save to 
									// KB done, this will be the only place the adaption is preserved
	wxString		m_markers;		// the standard format markers which precede this source phrase, if any
	wxString		m_follPunct;	// punction characters which follow this source word or phrase
	wxString		m_precPunct;	// punctuation characters which precede this source word or phrase
	wxArrayString*	m_pMedialPuncts; // accumulate medial punctuations here, when merging 
									 //words/phrases. MFC uses CStringList
	wxArrayString*	m_pMedialMarkers; // accumulate medial standard format markers here, when merging. MFC uses CStringList
	wxString		m_srcPhrase;	// the source phrase, including any punctuation (shown in line 1)
	wxString		m_key;			// the source phrase, with any puntuation stripped off (shown in line 2)
	wxString		m_targetStr;	// final (line 4) adaptation phrase for the target language
	int				m_nSrcWords;	// how many words are in the source phrase
	int				m_nSequNumber;	// first word is 0, next 1, etc. to text end (for a single file)s

	// new attributes for VERSION_NUMBER 3, m_gloss  (used with gbEnableGlossing and gbIsGlossing
	// flags) to allow the user to give word for word glosses and have them saved in a glossing KB
	wxString		m_gloss; // save a 'gloss' - which can be anything, eg gloss, or an adaptation
							 // from a different project to a different target text, etc
	bool			m_bHasGlossingKBEntry; // parallels the function of m_bHasKBEntry

	// new attributes for VERSION_NUMBER 4, for supporting free translations, notes and bookmarks
	bool			m_bHasFreeTrans; // TRUE whenever this sourcephrase is associated with a free translation
	bool			m_bStartFreeTrans; // TRUE if this sourcephrase is the first in a free translation
									   // section - this is the one which stores its text, filtered, in m_markers
	bool			m_bEndFreeTrans; // TRUE if this sourcephrase is the last in a free translation
	bool			m_bHasNote; // TRUE if this sourcephrase contains a note (in m_markers, filtered, with
								// marker \note and endmarker \note*)
	bool			m_bHasBookmark; // TRUE if this sourcephrase is bookmarked (this member is its sole exponent)

// Serialization
	//virtual void	Serialize(CArchive& ar); // MFC used this
	// MFC's Serialize() function is handled in the wxWidgets version with SaveObject() 
	// and LoadObject() below
	//wxOutputStream& SaveObject(wxOutputStream& stream, bool bParentCall);
	//wxInputStream& LoadObject(wxInputStream& stream, bool bParentCall);

// Operations
public:
	bool			Merge(CAdapt_ItView* WXUNUSED(pView), CSourcePhrase* pSrcPhrase);
	CSourcePhrase&	operator =(const CSourcePhrase& sp);
	void			CopySameTypeParams(const CSourcePhrase& sp);
	CBString		MakeXML(int nTabLevel); // nTabLevel specifies how many tabs are to start each line,
												// nTabLevel == 1 inserts one, 2 inserts two, etc
	void			DeepCopy(void); // BEW added 16Apr08, to obtain copies of any saved original
									// CSourcePhrases from a merger, and have pointers to the copies
									// replace the pointers in the m_pSavedWords member of a new instance
									// of CSourcePhrase produced with the copy constructor or operator=
									// Usage: for example: 
									// CSourcePhrase oldSP; ....more application code defining oldSP contents....
									// CSourcePhrase* pNewSP = new CSourcePhrase(oldSP); // uses operator=
									//			pNewSP.DeepCopy(); // *pNewSP is now a deep copy of oldSP

// Getters/Setters/Shorthands

	bool GetStartsNewChapter() {return this->m_bChapter != 0;}

	wxString GetChapterNumberString();

	// BEW added 04Nov05
	bool ChapterColonVerseStringIsNotEmpty();


private:
	DECLARE_DYNAMIC_CLASS(CSourcePhrase)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to 
	// declare that the objects of this class should be dynamically 
	// creatable from run-time type information. 
	// MFC uses DECLARE_SERIAL(CSourcePhrase). wxWidgets does not
	// implement Serialization of objects. Therefore, we will handle
	// serialization manually.
};
#endif /* SourcePhrase_h */
