/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Usfm2Oxes.h
/// \author			Bruce Waters
/// \date_created	2 Sept 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the header file for the Usfm2Oxes class. 
/// The USFM2Oxes class takes an in-memory plain text (U)SFM marked up text, chunks it
/// according to criteria relevant to oxes support, and then builds oxes xml from each
/// chunk in the appropriate order of chunks. The chunks are stored on structs. 
/// Initially oxes version 1 is supported, later it will also support version 2 when the
/// latter standard is finalized. (Deuterocannon books are not supported explicitly.)
/// \derivation		The Usfm2Oxes class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#ifndef Usfm2Oxes_h
#define Usfm2Oxes_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "Usfm2Oxes.h"
#endif

//#include <wx/gdicmn.h>

// forward declarations
class wxObject;
class CAdapt_ItApp;
class CBString;

enum CustomMarkers {
	excludeCustomMarkers,
	includeCustomMarkers
};










struct TitleInfo
{
	// currently supporting: id h h1 h2 h3 mt mt1 mt2 mt3, and SFM PNG: st
	// 
	// markers in USFM which could be here but our OXES export code is not yet supporting:
	// rem (paratext remark) ide (encoding specification) sts (status code, 1 = first
	// draft, 2 = team draft, 3 = reviewed draft, 4 = clean text), toc1 (long 'table of
	// contents' text), toc2 (short 'table of contents' text), toc3 (book abbreviation)
	// mte (major title at ending) 
	wxString bookCode;
	bool bChunkExists;
	wxString strChunk;
	wxArrayString arrPossibleMarkers;
	wxArrayString arrMkrSequInData;
	wxArrayString arrMkrConvertFrom;
	wxArrayString arrMkrConvertTo;
	wxString idStr;
	wxString hStr;
	wxString h1Str;
	wxString h2Str;
	wxString h3Str;
	wxString mtStr;
	wxString mt1Str;
	wxString mt2Str;
	wxString mt3Str;
	wxString stStr;
	wxString freeStr;
	wxString noteStr;
};

class Usfm2Oxes : public wxEvtHandler
{
	//friend class CLayout;
public:

	Usfm2Oxes(); // default constructor
	Usfm2Oxes(CAdapt_ItApp* app); // use this one

	virtual ~Usfm2Oxes();// destructor

	// DoOxesExport() is the public function called to get the job done
	wxString DoOxesExport(wxString& buff);
	void SetOXESVersionNumber(int versionNum);

private:
	void Initialize();
	//bool IsEmbeddedWholeMarker(wxChar* pChar);
	bool IsAHaltingMarker(wxChar* pChar, CustomMarkers inclOrExcl); // used to ensure
											// a halt when parsing a marker's content
	bool IsSpecialTextStyleMkr(wxChar* pChar);
	void ClearTitleInfo();
	bool IsOneOf(wxString& str, wxArrayString& array, CustomMarkers inclOrExcl); // eg. check for SFMkr in array
	void WarnNoEndmarkerFound(wxString endMkr, wxString content);
	void WarnAtBufferEndWithoutFindingEndmarker(wxString endMkr);


	// chunking functions
	wxString* GetTitleInfoChunk(wxString* pInputBuffer);


	// a utility to convert Unicode markers to ASCII (actually UTF-8, but all markers fall
	// within the ASCII range of UTF-8)
	CBString toUTF8(wxString& str);

	// a parser which parses over inline (ie. formatting) USFM markers until next halting
	// SF marker is encountered
int ParseMarker_Content_Endmarker(wxString& buffer, wxString& mkr, wxString& endMkr,
					 wxString& dataStr, bool& bEmbeddedSpan, CustomMarkers inclOrExcl);


public:

private:
	CAdapt_ItApp*	m_pApp;	// The app owns this
	int m_version; // initially = 1, later can be 1 or 2
	wxString m_haltingMarkers; // populated in Initialize()
	wxString m_specialMkrs; // special markers not listed in the global 
							// charFormatMkrs & populated in class creator
	wxString m_specialEndMkrs; // special markers not listed in the global 
							   // charFormatMkrs & populated in class creator
	wxString* m_pBuffer; // ptr to the (ex-class) buffer containing the (U)SFM text
	bool m_bContainsFreeTrans;
	bool m_bContainsNotes;
	wxString m_freeMkr; // "\free"
	wxString m_noteMkr; // "\note"

	TitleInfo* m_pTitleInfo; // struct for storage for TitleInfo part of document

	DECLARE_EVENT_TABLE()
};

#endif /* Usfm2Oxes_h */
