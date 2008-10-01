/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			XML.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for XML routines used in Adapt It for Dana and the WX version.
/////////////////////////////////////////////////////////////////////////////

#ifndef XML_h
#define XML_h

//#define Output_Default_Style_Strings	// uncomment to output default Unix-style usfm strings
										// to books.txt and AI_USFM_full.txt. For this to work
										// properly, the up-to-date AI_USFM_full.xml file should 
										// be located in the Adapt It Work folder. The normally
										// used AI_USFM.xml file need not be renamed, since when
										// this symbol is defined, AI_USFM_full.xml only will be
										// used.

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ClassName.h"
#endif

typedef short unsigned int UInt16;
typedef unsigned int UInt32;
typedef short int Int16;
typedef int Int32;
typedef char Int8;
typedef unsigned char UInt8;
typedef unsigned int Err;

// masks for the CSourcePhrase BOOL flags
const UInt32 hasKBEntryMask				= 1; // position 1
const UInt32 notInKBMask				= 2; // position 2
const UInt32 hasGlossingKBEntryMask		= 4; // position 3
const UInt32 specialTextMask			= 8; // position 4
const UInt32 firstOfTypeMask			= 16; // position 5
const UInt32 boundaryMask				= 32; // position 6
const UInt32 nullSourcePhraseMask		= 64; // position 7
const UInt32 retranslationMask			= 128; // position 8
const UInt32 beginRetranslationMask		= 256; // position 9
const UInt32 endRetranslationMask		= 512; // position 10
const UInt32 hasFreeTransMask			= 1024; // position 11
const UInt32 startFreeTransMask			= 2048; // position 12
const UInt32 endFreeTransMask			= 4096; // position 13
const UInt32 hasNoteMask				= 8192; // position 14
const UInt32 hasBookmarkMask			= 16384; // position 15
const UInt32 chapterMask				= 32768; // position 16
const UInt32 verseMask					= 65536; // position 17
const UInt32 hasInternalMarkersMask		= 131072; // position 18
const UInt32 hasInternalPunctMask		= 262144; // position 19
const UInt32 footnoteMask				= 524288; // position 20
const UInt32 footnoteEndMask			= 1048576; // position 21
const UInt32 paragraphMask				= 2097152; // position 22

/*
// whm note: I've moved the following constants to Adapt_It.cpp
// for Adapt It document output as XML, and parsing of elements
const char adaptitdoc[] = "AdaptItDoc";
const char settings[] = "Settings";
const char scap[] = "S";
const char mpcap[] = "MP";
const char mmcap[] = "MM";

// attribute names for Adapt It documents
const char docversion[] = "docVersion";
const char sizex[] = "sizex";
const char sizey[] = "sizey";
const char specialcolor[] = "specialcolor";
const char retranscolor[] = "retranscolor";
const char navcolor[] = "navcolor";
const char curchap[] = "curchap";
const char srcname[] = "srcname";
const char tgtname[] = "tgtname";
const char others[] = "others";
// next ones are for the sourcephrases themselves
const char a[] = "a"; // m_adaption
const char k[] = "k"; // m_key
const char s[] = "s"; // m_srcPhrase
const char t[] = "t"; // m_targetStr
const char g[] = "g"; // m_gloss
const char f[] = "f"; // flags (32digit number, all 0 or 1)
const char sn[] = "sn"; // m_nSequNumber
const char w[] = "w"; // m_nSrcWords
const char ty[] = "ty"; // m_curTextType
const char pp[] = "pp"; // m_precPunct
const char fp[] = "fp"; // m_follPunct
const char i[] = "i"; // m_inform
const char c[] = "c"; // m_chapterVerse
const char m[] = "m"; // m_markers
const char mp[] = "mp"; // some medial punctuation
const char mm[] = "mm"; // one or more medial markers (no filtered stuff)

// tag & attribute names for KB i/o
const char aikb[] = "AdaptItKnowledgeBase";
const char kb[] = "KB";
const char gkb[] = "GKB";
const char map[] = "MAP";
const char tu[] = "TU";
const char rs[] = "RS";
const char srcnm[] = "srcName";
const char tgtnm[] = "tgtName";
const char n[] = "n";
const char max[] = "max";
const char mn[] = "mn";
const char xmlns[] = "xmlns";
*/
// global helper for constructing the flags attribute in the XML document
class CBString;
class CSourcePhrase;
CBString	MakeFlags(CSourcePhrase* pSP);
void		MakeBOOLs(CSourcePhrase*& pSP, CBString& digits);
class CStack;
class CBString;
class CFile;
class CKB;
class CAdapt_ItDoc;

// new error numbers (fileErrorClass is 0x1600, and there are 17 
// predefined errors in the file stream manager header, so mine start
// at value 18.
//const UInt32 fileErrShortWrite = IDS_SHORT_WRITE_ERR;   

// helper, for writing to the (arbitrary length) FILE object on the
// expansion card (or to a Windows folder when using the emulator);
// for VFS on a real device, we must define an overloaded version of
// this function
// ****************** TODO - the overloaded fn for VFS ****************
void DoWrite(wxFile& file, CBString& str);

// functions to read or write XML documents
//bool WriteDoc_XML(CBString& path); // stub only
void ShowXMLErrorDialog(CBString& badElement,Int32 offset,
							bool bCallbackSucceeded);

// XML document building support
//CBString AddPrologue(); // BEW removed 13Aug07, as Bob's 
						  // GetEncodingStringForXmlFiles() is used now
CBString AddOpeningTagWithNewline(const char* tag); // <TAG> + newline
CBString AddElemNameTag(const char* tag); // <TAG
CBString AddElemNameTagFull(const char* tag); // <TAG>
CBString AddElemClosingTagFull(const char* tag); // </TAG> + newline
CBString AddFirstAttrPre(const char* attr); // space + attr="
CBString AddAttrToNextAttr(const char* attr); // " + space + attr="
CBString AddCloseAttrCloseOpeningTag(); // ">
CBString AddPCDATA(const CBString pcdata); // space + PCDATA + space
CBString AddElemClosingTag(const char* tag); // </TAG> + newline
CBString AddCloseAttrCloseEmptyElem(); // "/> + newline
CBString AddClosingTagWithNewline(const char* tag); // </TAG> + newline
CBString MakeMSWORDWarning(bool bExtraKBnote = FALSE);

// support for elements which contain entities
void DoEntityInsert(CBString& s,Int16& offset,CBString& ch,const char* ent);
void DoEntityReplace(CBString& s,Int16& offset,const char* ent,char ch);
void InsertEntities(CBString& s); // handle & " ' < and > using &amp; etc)
void ReplaceEntities(CBString& s);

// XML document parsing support
bool IsWhiteSpace(char* pPos,char* pEnd);
void SkipWhiteSpace(char*& pPos,char* pEnd);
void MakeStrFromPtrs(char* pStart,char* pFinish,CBString& s);
char* FindElemEnd(char* pPos,char* pEnd); // scans to next /> or >

bool ParseAttrName(char*& pPos,char* pEnd); // scan a tag
bool ParseTag(char*& pPos,char* pEnd,bool& bHaltedAtSpace);
bool ParseClosingTag(char*& pPos,char* pEnd);

bool ParsePCDATA(char*& pPos,char* pEnd,CBString& pcdata); // scan PCDATA

bool ParseXML(wxString& path, bool (*pAtTag)(CBString& tag),
		bool (*pAtEmptyElementClose)(CBString& tag),
		bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue),
		bool (*pAtEndTag)(CBString& tag),
		bool (*pAtPCDATA)(CBString& tag,CBString& pcdata));
		
#ifdef Output_Default_Style_Strings
bool ParseXMLElement(wxFile& dfile,
		CStack*& pStack,CBString& tagname,char*& pBuff,
		char*& pPos,char*& pEnd,bool& bCallbackSucceeded,
		bool (*pAtTag)(CBString& tag),
		bool (*pAtEmptyElementClose)(CBString& tag),
		bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue),
		bool (*pAtEndTag)(CBString& tag),
		bool (*pAtPCDATA)(CBString& tag,CBString& pcdata));
#else
bool ParseXMLElement(CStack*& pStack,CBString& tagname,char*& pBuff,
		char*& pPos,char*& pEnd,bool& bCallbackSucceeded,
		bool (*pAtTag)(CBString& tag),
		bool (*pAtEmptyElementClose)(CBString& tag),
		bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue),
		bool (*pAtEndTag)(CBString& tag),
		bool (*pAtPCDATA)(CBString& tag,CBString& pcdata));
#endif
		
bool ParseXMLAttribute(CBString& WXUNUSED(tagname),char*& WXUNUSED(pBuff),char*& pPos,char*& pEnd,
						CBString& attrName,CBString& attrValue);

bool ParseAttrValue(char*& pPos,char* pEnd); // scan to "

// Callbacks, for implementing application-specific interaction with
// various XML documents; their declarations are in the function signature
// which use them, so prototypes would be an error;  but I list them here
// for documentation purposes. See also the comments preceding the callback
// functions towards the end of the XML.cpp file.
/*
// calls pAtTag when element's tag has just been parsed, ie <TAG
bool (*pAtTag)(CBString& tag) 
// calls pAtEmptyElementClose when /> has just been parsed (as in either
// <TAG/>              or       <TAG ... one or more attributes... />
bool (*pAtEmptyElementClose)(CBString& tag)
// calls pAtAttr when an attribute is about to be parsed
// (and returns the attribute name and its (string) value
bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue)
// calls pAtEndTag when </TAG> has just been parsed
bool (*pAtEndTag)(CBString& tag)
// calls pAtPCDATA when some PCData has just been parsed (the parsed
// character data does NOT include any initial or final spaces)
bool (*pAtPCDATA)(tagname,pcdata)
*/

// Functions used as callbacks for Book mode support
bool AtBooksTag(CBString& tag);
bool AtBooksEmptyElemClose(CBString& tag);
bool AtBooksAttr(CBString& tag,CBString& attrName,CBString& attrValue);
bool AtBooksEndTag(CBString& tag);
bool AtBooksPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata));

// Functions used as callbacks for AI_USFM.xml
bool AtSFMTag(CBString& tag);
bool AtSFMEmptyElemClose(CBString& WXUNUSED(tag));
bool AtSFMAttr(CBString& tag,CBString& attrName,CBString& attrValue);
bool AtSFMEndTag(CBString& tag);
bool AtSFMPCDATA(CBString& WXUNUSED(tag),CBString& pcdata);

// Functions used as callbacks for XML-marked-up Adapt It documents
bool AtDocTag(CBString& tag);
bool AtDocEmptyElemClose(CBString& WXUNUSED(tag));
bool AtDocAttr(CBString& tag,CBString& attrName,CBString& attrValue);
bool AtDocEndTag(CBString& tag);
bool AtDocPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata));

// Functions used as callbacks for XML-marked-up KB and GlossingKB files
bool AtKBTag(CBString& tag);
bool AtKBEmptyElemClose(CBString& WXUNUSED(tag));
bool AtKBAttr(CBString& tag,CBString& attrName,CBString& attrValue);
bool AtKBEndTag(CBString& tag);
bool AtKBPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata));

// the read and parse functions;
bool ReadBooks_XML(wxString& path);


// read and parse function for AI_USFM.xml
bool ReadSFM_XML(wxString& path);

// read and parse function for Adapt It xml documents
bool ReadDoc_XML(wxString& path, CAdapt_ItDoc* pDoc);

// read and parse function for Adapt It xml KB and GlossingKB files
// pKB is a pointer to the CKB instance which is being filled out by the
// parsing of the XML file
bool ReadKB_XML(wxString& path, CKB* pKB);

#endif // XML_h


