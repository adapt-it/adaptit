/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			SourcePhrase.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
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
    // BEW 10Aug11 changed the order of the members so that commonly examined ones in the
    // debugger will appear at the top - the sequ num, how many words (indicates if merger
    // or not and how big a merger it is) then the list of stored originals... other string
    // members, and booleans are now in the 0 to 22 order, top down, that they appear in
    // the xml f attribute's 22_binary_digit string, reading right to left. I've done this
	// as a debugging aid, that's all. Comments below start with the byte offset at which the
	// respective member is located within the class, on a 32-bit machine.
	int				m_nSequNumber;	// 0: first word is 0, next 1, etc. to text end (for a single file)s
	int				m_nSrcWords;	// 4: how many words are in the source phrase
	SPList*			m_pSavedWords;	// 8 (ptr to 32): save list of CSourcePhrase instances comprising the phrase
	wxString		m_srcPhrase;	// 12: the source phrase, including any punctuation (shown in line 1)
	wxString		m_key;			// 16: the source phrase, with any puntuation stripped off (shown in line 2)
	wxString		m_adaption;		// 20: a copy of the adaption (line 3) text, if user wants no save to 
									// KB done, this will be the only place the adaption is preserved
	wxString		m_targetStr;	// 24: final (line 4) adaptation phrase for the target language
	wxString		m_markers;		// 28: the standard format markers which precede this source phrase, if any
	wxArrayString*	m_pMedialPuncts; // 32 (ptr to 16): accumulate medial punctuations here, when merging 
									 //words/phrases. MFC uses CStringList
	wxArrayString*	m_pMedialMarkers; // 36 (ptr to 16): accumulate medial standard format markers here, 
									  // when merging. MFC uses CStringList
	wxString		m_precPunct;	// 40: punctuation characters which precede this source word or phrase
	wxString		m_follPunct;	// 44: punction characters which follow this source word or phrase
	wxString		m_inform;		// 48: text to be shown above the strip when info type changes
	wxString		m_chapterVerse; // 52: text for the chapter and verse is stored here, as required
	wxString		m_gloss;		// 56: save a 'gloss' - which can be anything, eg gloss, or an adaptation
									// from a different project to a different target text, etc
	// for docVersion 6, added the next 4 strings
	wxString		m_lastAdaptionsPattern; // the value within m_adaptions at the time of the last placement dlg
	wxString		m_tgtMkrPattern; // remember where Place Medial Markers placed any markers, for tgt text export
	wxString		m_glossMkrPattern; // remember where Place Medial Markers placed any markers, for glossing export
	wxString		m_punctsPattern; // remember where Place Internal Punctuation placed any medial puncts (unused
									 // for version 6.2.0.  Possibly will be used in a later version.
	// for docVersion 9, added wxString storage for src text wordbreak character when it is not a space, 
	// but empty if it is a space
	// and storage as a wxArrayString for tgt text wordbreaks storage (when not a space) and empty if
	// it is a space. It is an array for support of replacement of wordbreaks in tgt text in mergers
	// and in retranslations
	wxString		m_srcWordBreak;
	wxArrayString	m_tgtWordBreaksArray; // in the XML output we store these as "entity1:entity2:entity3:.. etc
				// where entity is of the form  &#hhhh;  hhhh is uppercase hex digits for the unicode codepoint

	// booleans and TextType last -- each takes 1 byte presumably even though memory is
	// word-aligned, because the whole lot is 136 bytes for the CSourcePhrase class
	// character offsets below are too small as of docVersion 9
	bool			m_bHasKBEntry;	// 60: TRUE when m_key of this source phrase is used to put an entry in
									// the KB
	bool			m_bNotInKB;		// 61: set TRUE if user specifies no KB map entry for this adaption, and 
									// obligatorily TRUE for retranslations, null source phrases, and 
									// too-long phrases
	// new attribute for VERSION_NUMBER 3, m_gloss  (used with gbGlossingVisible and gbIsGlossing
	// flags) to allow the user to give word for word glosses and have them saved in a glossing KB
	bool			m_bHasGlossingKBEntry; // 62: parallels the function of m_bHasKBEntry
	bool			m_bSpecialText; // 63: TRUE if the text is special, such as \id contents, or 
									// subheading, etc
	bool			m_bFirstOfType; // 64: TRUE when textType changes. Used for extending a type to
									//  succeeding source phrases (with this flag FALSE), until a 
									// new TRUE value is set
	bool			m_bBoundary;	// 65: marks right boundary for selection extension, to prevent spurious
									// source phrases being created by the merge operation
	bool			m_bNullSourcePhrase; // 66: TRUE if this is a placeholder inserted by the user 
										// (shows as ...)
	bool			m_bRetranslation;	// 67: set when srcPhrase > 10 words, or target text approaches
									 // docWidth, or user explicitly marks a selection as a
									 // retranslation, rather than an adaption; 
									 // - scrPhrases so marked are not entered in KB 
	// two new flags for version_number == 2, 
	// these flags used for preventing concatenation of retranslations
	// *** DONT FORGET TO UPDATE THE COPY CONSTRUCTOR AND THE OVERLOADED ASSIGNMENT OPERATOR AS WELL!
	bool			m_bBeginRetranslation; // 68:
	bool			m_bEndRetranslation;   // 69:
	// new attributes for VERSION_NUMBER 4, for supporting free translations, notes and bookmarks
	bool			m_bHasFreeTrans;   // 70: TRUE whenever this sourcephrase is associated with a free translation
	bool			m_bStartFreeTrans; // 71: TRUE if this sourcephrase is the first in a free translation
									   // section - this is the one which stores its text, in m_freeTrans
									   // (considered filtered)
	bool			m_bEndFreeTrans;   // 72: TRUE if this sourcephrase is the last in a free translation
	bool			m_bHasNote;        // 73: TRUE if this sourcephrase contains a note (in m_note, considered filtered)
	bool			m_bSectionByVerse; // 74: TRUE if this sourcephrase is a free translation anchor
									   //     where the sectioning was done using the (by)"Verse" setting
									   //     (if FALSE, i.e. zero, the sectioning was (by)"Punctuation")
									   //     Note: FALSE is the default, so can't be used to indicate
									   //     that the CSourcePhrase has a free translation section
	bool			m_bChapter;		   // 75: TRUE if the source phrase is first one in a new chapter
	bool			m_bVerse;		   // 76: TRUE if the source phrase is the first one in a new verse
	bool			m_bHasInternalMarkers; // 77: if TRUE, user will have to manually located the target
									       // markers using a dialog in the Export... function
	bool			m_bHasInternalPunct; // 78: if TRUE, user will be asked to manually locate target  punctuation
	bool			m_bFootnoteEnd;	   // 79: TRUE for the last source phrase in a section of footnote text
	bool			m_bFootnote;	   // 80: TRUE for the first source phrase in a section of footnote text
	bool			m_bUnused;		   // 81:   
	TextType		m_curTextType;	   // 82: from the global enum. Used for setting the text type tag to 
									   // be shown in grey above line 1 of each strip

    // BEW added 9Feb10, extra wxString members for refactoring support for free
    // translations, notes, collected back translations, endMarkers, and filtered
	// information; these are private as should have been the case for the above too, but
	// that is a change we can defer to much later on
private:
	wxString		m_endMarkers;			// 84:
	wxString		m_freeTrans;			// 88:
	wxString		m_note;					// 92:
	wxString		m_collectedBackTrans;	// 96:
	wxString		m_filteredInfo;			// 100
	// BEW added 11Oct10, the next five are needed in order to properly handle inline
	// (character formatting) markers, and the interactions of punctuation and marker
	// types - in particular to support punctuation which follows inline markers (rather
	// than assuming that punctuation binds more closely to the word than do endmarkers
	// and beginmarkers)
	wxString		m_inlineBindingMarkers;			// 104:
	wxString		m_inlineBindingEndMarkers;		// 108:
	wxString		m_inlineNonbindingMarkers;		// 112:
	wxString		m_inlineNonbindingEndMarkers;	// 116:
	wxString		m_follOuterPunct; // 146: store any punctuation after endmarker (inline
									  // non-binding, or \f* or \x*) here; but puncts
									  // after inline binding mkr go in m_follPunct
													// 120:
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
									//			pNewSP->DeepCopy(); // *pNewSP is now a deep copy of oldSP

// Getters/Setters/Shorthands

	bool GetStartsNewChapter() {return this->m_bChapter != 0;}

	wxString GetChapterNumberString();

	// BEW added 04Nov05
	bool ChapterColonVerseStringIsNotEmpty();

	// BEW added 12Feb10, getters and setters for the 5 new private wxString members
	wxString GetFreeTrans();
	wxString GetNote();
	wxString GetCollectedBackTrans();
	wxString GetFilteredInfo();
	bool	 GetFilteredInfoAsArrays(wxArrayString* pFilteredMarkers, 
									wxArrayString* pFilteredEndMarkers,
									wxArrayString* pFilteredContent,
									bool bUseSpaceForEmpty = FALSE);
	wxString GetEndMarkers();
	bool GetEndMarkersAsArray(wxArrayString* pEndmarkersArray); // return FALSE if empty, else TRUE
	//bool GetAllEndMarkersAsArray(wxArrayString* pEndmarkersArray); // ditto, gets not just from
				// m_endMarkers, but also from the two storage members for inline endmarkers
	void SetFreeTrans(wxString freeTrans);
	void SetNote(wxString note);
	void SetCollectedBackTrans(wxString collectedBackTrans);
	void AddToFilteredInfo(wxString filteredInfo);
	void SetFilteredInfo(wxString filteredInfo);
	void SetFilteredInfoFromArrays(wxArrayString* pFilteredMarkers, 
									wxArrayString* pFilteredEndMarkers,
									wxArrayString* pFilteredContent,
									bool bChangeSpaceToEmpty = FALSE);
	void SetEndMarkers(wxString endMarkers);
	void AddEndMarker(wxString endMarker);
	void SetEndMarkersAsNowMedial(wxArrayString* pMedialsArray);

	//BEW added 11Oct10
	wxString GetInlineBindingEndMarkers();
	wxString GetInlineNonbindingEndMarkers();
	wxString GetInlineBindingMarkers();
	wxString GetInlineNonbindingMarkers();
	wxString GetFollowingOuterPunct();
	void SetInlineBindingMarkers(wxString mkrStr);
	void SetInlineNonbindingMarkers(wxString mkrStr);
	void SetInlineBindingEndMarkers(wxString mkrStr);
	void SetInlineNonbindingEndMarkers(wxString mkrStr);
	void AppendToInlineBindingMarkers(wxString str);
	void AppendToInlineBindingEndMarkers(wxString str);
	void SetFollowingOuterPunct(wxString punct);
	void AddFollOuterPuncts(wxString outers);

/* uncomment out when we make m_markers a private member
	wxString GetMarkers();
	void SetMarkers(wxString markers);
*/

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
