/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			XML.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for XML routines used in Adapt It for Dana and the WX version.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "XML.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if defined(__VISUALC__) && __VISUALC__ >= 1400
#pragma warning(disable:4428)	// VC 8.0 wrongly issues warning C4428: universal-character-name 
								// encountered in source for a statement like 
								// ellipsis = _T('\u2026');
								// which contains a unicode character \u2026 in a string literal.
								// The MSDN docs for warning C4428 are also misleading!
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
#include <wx/file.h>
#include <wx/progdlg.h> // for wxProgressDialog
#include <wx/filename.h> // for wxFileName
#include <wx/busyinfo.h>

#include "XML.h"

// AdaptItDana's XML support (unvalidated, & only elements and attributes)
#include "Stack.h"
#include "Adapt_It.h"
#include "helpers.h"
#include "AdaptitConstants.h"
#include "Adapt_ItDoc.h"
#include "BString.h" // this needs to be included before "XMLErrorDlg.h"
#include "XMLErrorDlg.h"
#include "SourcePhrase.h"
#include "KB.h"
#include "TargetUnit.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "MainFrm.h"
#include "WaitDlg.h"

/// Length of the byte-order-mark (BOM) which consists of the three bytes 0xEF, 0xBB and 0xBF
/// in UTF-8 encoding.
#define nBOMLen 3

/// Length of the byte-order-mark (BOM) which consists of the two bytes 0xFF and 0xFE in
/// in UTF-16 encoding.
#define nU16BOMLen 2

extern wxChar gSFescapechar; // the escape char used for start of a standard format marker
extern const wxChar* filterMkr; // defined in the Doc
extern const wxChar* filterMkrEnd; // defined in the Doc
const int filterMkrLen = 8;
const int filterMkrEndLen = 9;

#ifdef _UNICODE
static unsigned char szBOM[nBOMLen] = {0xEF, 0xBB, 0xBF};
static unsigned char szU16BOM[nU16BOMLen] = {0xFF, 0xFE};
#endif

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // for rapid access to the app class

/// This global is defined in Adapt_It.cpp.
extern BookNamePair* gpBookNamePair;

/// This global is defined in Adapt_It.cpp.
extern bool gbTryingMRUOpen;

// parsing Adapt It documents
static CAdapt_ItDoc* gpDoc = NULL;
extern CSourcePhrase* gpSrcPhrase; // this is already defined in the view class
CSourcePhrase* gpEmbeddedSrcPhrase;
int	gnDocVersion;

// BEW 1Jun10, added this global for storing the kbVersion number from the xml file for a
// CKB which is being loaded from LoadKB() or LoadGlossingKB()
int gnKbVersionBeingParsed; // will have value 1 (for kbv1) or 2 (for kbv2)
							// but actually we'll use the symbolic constants KB_VERSION1
							// and KB_VERSION2, respectively, see Adapt_ItConstants.h

// parsing KB files
CKB* gpKB; // pointer to the adapting or glossing KB (both are instances of CKB)
CTargetUnit* gpTU; // pointer to the current CTargetUnit instance
CRefString* gpRefStr; // pointer to the current CRefString instance
MapKeyStringToTgtUnit* gpMap; // pointer to the current map
int gnMapIndex; // 0-based index to the current map
wxString gKeyStr;// source text key string for the map entry
int gnRefCount; // reference count for the current CRefString instance
//bool bKeyDefined = FALSE;
//extern bool gbIsGlossing;

static CTargetUnit* gpTU_From_Map; // for LIFT support, this will be non-NULL when,
						// for a given key, the relevant map contains a CTargetUnit
						// instance, and this will be a pointer to it
static char emptyStr[32];
void* r = memset((void*)emptyStr,0,32); // after the above line


/// This global is defined in MainFrm.cpp.
extern SPList* gpDocList; // for synch scrolling support (see MainFrm.cpp)

extern bool gbSyncMsgReceived_DocScanInProgress;

/// This global is defined in Adapt_It.cpp.
extern USFMAnalysis* gpUsfmAnalysis;

/// This global is defined in Adapt_It.cpp.
extern bool gbHackedDataCharWarningGiven;

Item gStoreItem; // temporary storage for popped items

// previous values below:
//#define BUFFSIZE 5120
//#define worklimit 3600
//#define FourKB 4096

// new values below:
#define BUFFSIZE 40960
#define safelimit 15360
#define TwentyKB 20480

// define our needed tags, entities and attribute names

// the standard entities
const char amp[] = "&amp;";
const char quote[] = "&quot;";
const char apos[] = "&apos;";
const char lt[] = "&lt;";
const char gt[] = "&gt;";

// some basic XML strings
const char xml[] = "<?xml";
const char comment[] = "<!--";
const char endcomment[] = "-->";

// Unicode BOMs (from the perspective of bytes)
const char bomUTF8[] = "\u00EF\u00BB\u00BF";

// this group of tags are for the books.xml file
const char books[] = "BOOKS";
const char namepair[] = "NAMEPAIR";
const char division[] = "DIV";

// this group are for the attribute names within books.xml
const char name[] = "name";
const char count[] = "count";
const char defaultbook[] = "defaultBook";
const char folder[] = "folder";
const char view[] = "view";
const char title[] = "title";
const char code[] = "code";

// these static variables are used for books.xml parsing
// for tracking an array index and the counts for divisions and total
static int divIndex = -1;
static int divCount = 0;
static int totalCount = 0;

// this group of tags are for the AI_USFM.xml file
const char usfmsupport[] = "USFMsupport";
const char sfm[] = "SFM";

// this group are for the attribute names
const char markerStr[] = "marker";
const char endmarkerStr[] = "endMarker";
const char descriptionStr[] = "description";
const char usfmStr[] = "usfm";
const char pngStr[] = "png";
const char filterStr[] = "filter";
const char usercansetfilterStr[] = "userCanSetFilter";
const char inlineStr[] = "inLine";
const char specialStr[] = "special";
const char bdryonlastStr[] = "bdryOnLast";
const char informStr[] = "inform";
const char navigationtextStr[] = "navigationText";
const char texttypeStr[] = "textType";
const char notypeStr[] = "noType";
const char wrapStr[] = "wrap";
const char stylenameStr[] = "styleName"; // added for version 3
const char styletypeStr[] = "styleType";
const char fontsizeStr[] = "fontSize";
const char colorStr[] = "color";
const char italicStr[] = "italic";
const char boldStr[] = "bold";
const char underlineStr[] = "underline";
const char smallcapsStr[] = "smallCaps";
const char superscriptStr[] = "superScript";
const char justificationStr[] = "justification";
const char spaceaboveStr[] = "spaceAbove";
const char spacebelowStr[] = "spaceBelow";
const char leadingmarginStr[] = "leadingMargin";
const char followingmarginStr[] = "followingMargin";
const char firstlineindentStr[] = "firstLineIndent";
const char basedonStr[] = "basedOn"; // added for version 3
const char nextstyleStr[] = "nextStyle"; // added for version 3
const char keeptogetherStr[] = "keepTogether"; // added for version 3
const char keepwithnextStr[] = "keepWithNext"; // added for version 3
const char one[] = "1";
const char zero[] = "0";

// this group are for the TextType equivalent string names
const char noTypeStr[] = "noType";
const char verseStr[] = "verse";
const char poetryStr[] = "poetry";
const char sectionHeadStr[] = "sectionHead";
const char mainTitleStr[] = "mainTitle";
const char secondaryTitleStr[] = "secondaryTitle";
const char noneStr[] = "none";
const char footnoteStr[] = "footnote";
const char headerStr[] = "header";
const char identificationStr[] = "identification";
const char rightMarginReferenceStr[] = "rightMarginReference";
const char crossReferenceStr[] = "crossReference";
const char noteStr[] = "note";

// this group are for the styleType equivalent string names
const char paragraphStr[] = "paragraph";
const char characterStr[] = "character";
const char tableStr[] = "table"; // whm added 21Oct05
const char footnote_callerStr[] = "footnote_caller"; // " " "
const char footnote_textStr[] = "footnote_text"; // " " "
const char default_para_fontStr[] = "default_para_font"; // " " "
const char footerStyStr[] = "footerSty"; // " " "
const char headerStyStr[] = "headerSty"; // " " "
const char horiz_ruleStr[] = "horiz_rule"; // " " "
const char boxed_paraStr[] = "boxed_para"; // " " "
const char hnoteStr[] = "hidden_note"; // " " "
//const char noteStr[] = "note"; // UBS uses this as a StyleType

// this group are for the justification equivalent string names
const char leadingStr[] = "leading";
const char followingStr[] = "following";
const char centerStr[] = "center";
const char justifiedStr[] = "justified"; // added for version 3

#ifdef Output_Default_Style_Strings

// static CString is used to accumulate a Unix-style data string for output
// to an external file. This string will be composed and output for each usfm
// defined in AI_USFM_full.xml. Once all the output strings are written to the
// AI_USFM_full.txt file they can be used to construct an array of strings residing
// hard coded in program code that, once parsed, can be used to set the 
// default attributes of the array of USFMAnalysis structs stored on the heap.
// These defaults would be used when the AI_USFM.xml file is not available.
// These little conditionally compiled routines greatly simplify the creation
// of these hard coded strings, and can be re-produced any time a change in
// default attributes is deemed necessary, helping to insure that the program's
// hard coded defaults are the same as the AI_USFM.xml file distributed with
// the program.
static wxString usfmUnixDataStr = _T("");
static int numUSFMMarkersOutput = 0;
#endif



// helper, for repetitive writing of elements to an arbitrary length FILE, utf-8, in TEXT mode
void DoWrite(wxFile& file, CBString& str)
{
	wxLogNull logNo; // avoid spurious messages from the system

	Int32 len = str.GetLength();
	char* pstr = (char*)str;
	file.Write(pstr,len);
}

/*****************************************************************
*
* Helper functions for returning composed parts of XML elements
*
******************************************************************/

CBString AddOpeningTagWithNewline(const char* tag)
{
	CBString s = "<";
	s += tag;
	s += ">\n";
	return s;
}

CBString AddElemNameTag(const char* tag)
{
	CBString s = "<";
	s += tag;
	return s;
}

CBString AddElemNameTagFull(const char* tag)
{
	CBString s = "<";
	s += tag;
	s += ">";
	return s;
}

CBString AddElemClosingTagFull(const char* tag)
{
	CBString s = "</";
	s += tag;
	s += ">\n";
	return s;
}

CBString AddFirstAttrPre(const char* attr)
{
	CBString s = " ";
	s += attr;
	s += "=\"";
	return s;
}

CBString AddAttrToNextAttr(const char* attr)
{
	CBString s = "\"";
	s += AddFirstAttrPre(attr);
	return s;
}

CBString AddCloseAttrCloseOpeningTag()
{
	CBString s = "\">";
	return s;
}

CBString AddPCDATA(const CBString pcdata)
{
	CBString s = " "; 
	s += pcdata;
	s += " ";
	return s;
}

CBString AddElemClosingTag(const char* tag)
{
	CBString s = "</";
	s += tag;
	s += ">\n";
	return s;
}

CBString AddCloseAttrCloseEmptyElem()
{
	CBString s = "\"/>\n";
	return s;
}

CBString AddClosingTagWithNewline(const char* tag)
{
	CBString s = "</";
	s += tag;
	s += ">\n";
	return s;
}

void DoEntityInsert(CBString& s,Int16& offset,CBString& ch,const char* ent)
{
a:	offset++;
	offset = s.Find(ch,offset);
	if (offset > -1)
	{
		// found an instance of ampersand
		s.Delete(offset); // remove the character at offset
		s.Insert(offset,ent); // insert the entity correponding to it
		goto a;
	}
}

CBString MakeMSWORDWarning(bool bExtraKBnote)
{
	wxString resStr1, resStr2; //, resStr3;
	// IDS_WARN_NO_MS_WORD
	resStr1 = resStr1.Format(_("Note: Using Microsoft WORD 2003 or later is not a good way to edit this xml file."));
	// IDS_SAFE_PROCESSORS
	resStr2 = resStr2.Format(_("Instead, use NotePad or WordPad."));
	// IDS_ELEM_ORDER_NOTE
	//resStr3 = resStr3.Format(_("Please note: the order of the TU elements in this xml file might differ each time the knowledge base is saved. This is not an error."));
	wxString msg;
	// whm modified 1Sep09 to remove the resStr3 part of the msg
	bExtraKBnote = bExtraKBnote; // to avoid compiler warning
	//if (bExtraKBnote)
	//{
	//	msg = msg.Format(_T("<!--\r\n\t %s\r\n\t %s\r\n\t %s -->\r\n"),resStr1.c_str(),resStr2.c_str(),resStr3.c_str());
	//}
	//else
	//{
	msg = msg.Format(_T("<!--\r\n\t %s\r\n\t %s -->\r\n"),resStr1.c_str(),resStr2.c_str());
	//}
	CBString returnStr;
#ifdef _UNICODE
	returnStr = gpApp->Convert16to8(msg);
#else
	returnStr = msg;
#endif
	return returnStr;
}

void InsertEntities(CBString& s)
{	
	CBString ch;
	Int16 offset;
	ch = "&";
	offset = -1;
	DoEntityInsert(s,offset,ch,amp);
	ch = "<";
	offset = -1;
	DoEntityInsert(s,offset,ch,lt);
	ch = "\'";
	offset = -1;
	DoEntityInsert(s,offset,ch,apos);
	ch = ">";
	offset = -1;
	DoEntityInsert(s,offset,ch,gt);
	ch = "\"";
	offset = -1;
	DoEntityInsert(s,offset,ch,quote);
}

void DoEntityReplace(CBString& s,Int16& offset,const char* ent,char ch)
{
	CBString theEntity = ent;
	UInt16 length = theEntity.GetLength();
a:	offset++;
	offset = s.Find(ent,offset);
	if (offset > -1)
	{
		// found an instance of the entity
		s.Delete(offset,length); // remove the entity
		s.Insert(offset,ch); // insert its replacement character
		goto a;
	}
}

void ReplaceEntities(CBString& s)
{
#ifndef __WXMSW__
#ifndef _UNICODE
	// whm note: For wx ANSI version, when compiled for wxGTK, we should
	// check the key strings for illegal 8-bit chars, specifically for MS' smart
	// quotes which cause problems because the wxGTK library internally
	// keeps strings in UTF-8. We'll quietly convert any smart quotes we find
	// to normal quotes. Note: The replacement of smart quotes with normal
	// quotes is done here at the beginning of the ReplaceEntities() call 
	// since this should catch any that exist within xml files read in the
	// wx version. 
	int ct;
	bool hackedFontCharPresent = FALSE;
	int hackedCt = 0;
	wxString hackedStr;
	hackedStr.Empty();
	for (ct = 0; ct < s.GetLength(); ct++)
	{
		int charValue;
		charValue = (int)s.GetAt(ct);
		// whm note: The decimal int value of extended ASCII chars is represented
		// as its decimal extended value minus 256, making any extended characters
		// be represented as negative numbers
		if (charValue < 0)
		{
			// we have an extended ASCII character in the string
			switch(charValue)
			{
			case -111: s.SetAt(ct,_T('\'')); // left single quotation mark
				break;
			case -110: s.SetAt(ct,_T('\'')); // right single quotation mark
				break;
			case -109: s.SetAt(ct,_T('\"')); // left double quotation mark
				break;
			case -108: s.SetAt(ct,_T('\"')); // right double quotation mark
				break;
			default:
				{
					// the default case indicates there is a non-smart quote extended
					// ASCII character present. In this case we should warn the user
					// that the data cannot be handled until it is converted to Unicode
					//::wxBell();
					hackedFontCharPresent = TRUE;
					hackedCt++;
					if (hackedCt < 10)
					{
						hackedStr += _T("\n   character with ASCII value: ");
						hackedStr << (charValue+256);
					}
					else if (hackedCt == 10)
						hackedStr += _T("...\n");
				}
			}
		}
	}
	if (hackedFontCharPresent && !gbHackedDataCharWarningGiven)
	{
		gbHackedDataCharWarningGiven = TRUE;
		wxString msg2 = _("\nYou should not use this non-Unicode version of Adapt It.\nYour data should first be converted to Unicode using TecKit\nand then you should use the Unicode version of Adapt It.");
		wxString msg1 = _("Extended 8-bit ASCII characters were detected in your\ndata files (see below):");
		msg1 += hackedStr + msg2;
		wxMessageBox(msg1,_("Warning: Invalid Characters Detected"),wxICON_WARNING);
	}
#endif
#endif
	// punctuation and < or >, within XML element attributes in this parser
	char ch;
	Int16 offset;
	ch = '&';
	offset = -1;
	DoEntityReplace(s,offset,amp,ch);
	ch = '<';
	offset = -1;
	DoEntityReplace(s,offset,lt,ch);
	ch = '\'';
	offset = -1;
	DoEntityReplace(s,offset,apos,ch);
	ch = '>';
	offset = -1;
	DoEntityReplace(s,offset,gt,ch);
	ch = '\"';
	offset = -1;
	DoEntityReplace(s,offset,quote,ch);
}

void SkipWhiteSpace(char*& pPos,char* pEnd)
{
a:	if (pPos == pEnd)
	return;
	if (IsWhiteSpace(pPos,pEnd))
	{
		pPos++;
		goto a;
	}
}

char* FindElemEnd(char* pPos,char* pEnd)
{
	// scan from the passed in pPos (current) location forwards
	// until either /> or > is reached, or until the end of the
	// buffer's data (pEnd) is reached - if won't always find an
	// element end, but if called from where an error condition has
	// been detected, it should encompass that bad bit of XML
	do {
	if ( (strncmp(pPos,"/>",2) == 0) || (strncmp(pPos,">",1) == 0))
	{
		if (*pPos == '/') pPos++;
		pPos++;
		break;
	}
	else
	{
		if (pPos < pEnd)
			pPos++;
		else
			return pEnd;
	}
	} while (pPos < pEnd);	
	return pPos;
}

bool ParseAttrName(char*& pPos,char* pEnd)
{
	// BEW 3Jun10, added to tests to allow as legal all the following structures:
	// attrname="value"   <<-- formerly we accepted just this, but LIFT can be different
	// attrname ="value"
	// attrname = "value"
	// attrname= "value"
	// and removed the useless tests, but keep the test for '>' in order to halt parsing
	// when there has been no proper termination of the parse of the attr name
	char* pAux = pPos; // in case of error, to enable repositioning pPos for the error dlg
	do {
		if ((*pPos == '=') || (strncmp(pPos," =",2) == 0) || (strncmp(pPos,"= ",2) == 0) 
			|| (strncmp(pPos," = ",3) == 0) || (*pPos == '>'))
		{
			break;
		}
		else
		{
			if (pPos < pEnd)
				pPos++;
			else
				break;
		}
	} while (pPos < pEnd);
	if (*pPos == '>')
	{
		// we didn't find "=" or "= " or " =" or " = ", so we have a structure error
		pPos = pAux;
		return FALSE;
	}
	return TRUE;
}

bool ParseAttrValue(char*& pPos,char* pEnd)
{
	// I tried this simpler code, to see if it produced a quicker parse, but it didn't.
	// Old code: LoadKB() in 156 ms, LoadGlossingKB() in 47 ms. This code, 156  & 47, so
	// there is nothing to be gained by using it 
	//char* pLocation = strstr(pPos,"\"");
	//if (pLocation < pEnd)
	//{
	//	pPos = pLocation;
	//	return TRUE;
	//}
	//else
	//{
	//	return FALSE;
	//}
	// enough tests are provided that if the attribute is not
	// closed with a terminating quote symbol, the function will
	// stop at /> or >, but only a quote is legal; if the xml is valid, only the first
	// test will ever be made anyway, so strstr() would not be faster
	do {
		if ((*pPos == '\"') || (strncmp(pPos,"/>",2) == 0) || (*pPos == '>'))
		{
			break;
		}
		else
		{
			if (pPos < pEnd)
				pPos++;
			else
				break;
		}
	} while (pPos < pEnd);
	if (*pPos != '\"')
		return FALSE;
	return TRUE;
}

bool ParseTag(char*& pPos,char* pEnd,bool& bHaltedAtSpace)
{
	// we use bHaltedAtSpace in the caller, so that if a space follows
	// a tagname, there has to be a following attribute otherwise the
	// tag is malformed
	bHaltedAtSpace = FALSE;
	//bool bValid = TRUE; // unused
	do {
		if ( (strncmp(pPos,"/>",2) == 0) || IsWhiteSpace(pPos,pEnd) 
				|| (*pPos == '>') || (*pPos == '=') || (*pPos == '&')
				|| (*pPos == ';') || (*pPos == '\"'))
		{
			break;
		}
		else
		{
			if (pPos < pEnd)
				pPos++;
			else
				break;
		}
	} while (pPos < pEnd);
	if ((*pPos == '=') || (*pPos == '&') || (*pPos == ';') || (*pPos == '\"'))
	{
		return FALSE;
	}
	else
	{
		if (IsWhiteSpace(pPos,pEnd))
			bHaltedAtSpace = TRUE;
	}
	return TRUE;
}

bool ParseClosingTag(char*& pPos,char* pEnd)
{
	//bool bValid = TRUE; // unused
	do {
		if ( (strncmp(pPos,"/>",2) == 0) || IsWhiteSpace(pPos,pEnd) 
				|| (*pPos == '>') || (*pPos == '=') || (*pPos == '&')
				|| (*pPos == ';') || (*pPos == '\"'))
		{
			break;
		}
		else
		{
			if (pPos < pEnd)
				pPos++;
			else
				break;
		}
	} while (pPos < pEnd);
	if (IsWhiteSpace(pPos,pEnd) || (*pPos == '=') || (*pPos == '&') 
					|| (*pPos == ';') || (*pPos == '\"'))
	{
		return FALSE;
	}
	else
	return TRUE;
}

bool IsWhiteSpace(char* pPos,char* pEnd)
{
	if (pPos == pEnd)
		return FALSE;
	if ((*pPos == ' ') || (*pPos == '\t') || (*pPos == '\n') || (*pPos == '\r'))
		return TRUE;
	else
		return FALSE;
}

void MakeStrFromPtrs(char* pStart,char* pFinish,CBString& s)
{
	Int32 len = pFinish - pStart;
	if (len == 0L)
	{
		s.Empty();
		return;
	}
	char* buffP = new char[len + 1]; // temporary buffer
	memset(buffP,0,len + 1);
	strncpy(buffP,pStart,len);
	s = buffP;
	delete[] buffP;
}

void ShowXMLErrorDialog(CBString& badElement,Int32 offset,bool bCallbackSucceeded)
{	wxString msg;
	wxString offsetStr;
	CXMLErrorDlg dlg((wxWindow*) wxGetApp().GetMainFrame());
	
	// get the appropriate message for the top of the dialog
	if (bCallbackSucceeded)
	{	
		// IDS_XML_MARKUP_ERR
		msg = _("XML markup error occurs in:\n");

		// calculate the character position string (offset + 1)
		offsetStr.Empty();
		offsetStr << (int)offset + 1;
	}
	else
	{
		// IDS_DATA_ERR
		msg = _("Unhandled data, or an error, occurs in:\n");
		//IDS_UNKNOWN
		offsetStr = _("unknown");
	}
	dlg.m_messageStr = msg;
	dlg.m_errorStr = badElement;
	dlg.m_offsetStr = offsetStr;
	
	dlg.Centre();
	// display the dialog modally
	dlg.ShowModal();
}

/*****************************************************************
*
* WriteDoc_XML
*
* Returns: TRUE if no error, else FALSE
*
* Parameters:
* -> path  -- absolute path to the file on the permanent storage medium
*
* Opens the wxFile type of object, and then constructs the XML Adapt It document
* for the XML file which is to store it, using DoWrite() to write out
* each element or larger constructed document part as the case may be,as soon as 
* it is composed, and then closes the CFile when done. (For speed, some document
* parts will be constructed by functions in the doc class, and the CSourcePhrase
* class.)
*
* Note; for a unicode build, DoInputConversion() or a similar function will
* be needed for converting any UTF-16 strings to UTF-8 in the code below
*
*******************************************************************/
/* // retain, it may be useful as the starting point for some XML output, but it is 
// more efficient and quicker to write the XML doc-constructing code in the class 
// which is pertinent and using literal strings -- see DoFileSave() in the Doc
// class for an example
bool WriteDoc_XML(CBString& path)
{
	//char strInt[12];
	
	// create the wxFile object for writing to the disk
	wxFile file;
	wxFile* fileP = &file;
	//UInt32 nOpenFlags = CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive;
#ifdef _UNICODE
	wxString wpath;
	gpApp->Convert8to16(path,wpath);
	const wxChar* pathP = wpath;
#else
	const char* pathP = (const char*)path;
#endif
	//CFileException e;
	bool bOpenOK = file.Open(pathP,wxFile::write); //bool bOpenOK = file.Open(pathP,nOpenFlags,&e);
	if (!bOpenOK)
	{
		// the stream was not opened, so tell the user 
		wxString errStr = _T("Could not open the XML data file for writing. Cause: ");
		wxChar digits[34];

		// the ANSI C errno global variable probably reports the same int value
		// that is returned by CFileException's e.m_cause member. Since we don't
		// have the same exception processing facility in wxWidgets, we'll just
		// report the errno number
		//wxString cause = _itot(e.m_cause,digits,10);
		wxString cause;
		wxSnprintf(digits, 34, "%d", errno);
		cause = digits;
		errStr += cause;
		wxMessageBox(errStr,_T(""), wxICON_STOP);
		return FALSE;
	}
	
	CBString s;
//	Int8 value8;
//	Int16 value16;
//	Int32 value32;
	CBString element; 
	element.Empty();
	
	// prologue
	element = AddPrologue();
	DoWrite(file,element);
	
	// settings opening tag
	element = AddOpeningTagWithNewline(xml_adaptitdoc);
	DoWrite(file,element);

	//  the settings info and the source phrases go here... (not done)

	// settings closing tag
	element = AddClosingTagWithNewline(xml_adaptitdoc);
	DoWrite(file,element);
	
	
	// release record and close state db, (assume no errors here, very
	// unlikely to get errors on these) 

	// close the FILE object
		file.Close();
	return TRUE;
}
*/
/*******************************************************************
*
* ParseXML
*
* Returns: TRUE if no errors, FALSE if there was an error or invalid
* document structure (document structure is checked, but not exhaustively,
* but it should catch most errors.)
*
* Parameters:
* -> path -- absolute path to the file on an expansion card.
*
* This is the top level function. It just creates an empty tag stack,
* ensures the XML prologue is present, and jumps it, then hands over
* element parsing to other functions. Parsing will be done not with
* Wchar, but rather char, since even for UTF-8 in the XML char parsing
* will be adequate, because markup is only ASCII characters. We also
* use a 40kb work buffer to get the data in 20kb chunks. 
*
*********************************************************************/

bool ParseXML(wxString& path, bool (*pAtTag)(CBString& tag,CStack*& pStack),
		bool (*pAtEmptyElementClose)(CBString& tag,CStack*& pStack),
		bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue,CStack*& pStack),
		bool (*pAtEndTag)(CBString& tag,CStack*& pStack),
		bool (*pAtPCDATA)(CBString& tag,CBString& pcdata,CStack*& pStack))
{
	bool bReadAll = FALSE;

	// make a 40KB locked work buffer in dynamic heap for storing next 
	// part of the stream which is due for parsing
	char* pBuff = new char[BUFFSIZE];
	if (pBuff == NULL)
	{
		// IDS_BUFFER_ALLOC_ERR
		wxMessageBox(_("Failed to allocate a buffer for parsing in the XML document's data"), _T(""), wxICON_WARNING);
		return FALSE;
	}
	memset(pBuff,0,BUFFSIZE); // assume no error

	// variables for buffering in the data from off the expansion card
	wxUint32  fileSize;
	wxUint32  nRead;
	wxUint32  nChunks;
	wxUint32  nCurrChunk = 0;
	wxUint32  nInputCount = 0;
	wxUint32  nInputLeft = 0;

	// note: nRead will always be less than expected, since each
	// cr lf pair counts in input as 1 byte, however, the st_size
	// value is correct
	wxFile file;

	wxString testStr;
	testStr = ::wxGetCwd();

	bool bOpenOK = file.Open(path,wxFile::read);
	if (bOpenOK)
	{
		wxStructStat status;
		if (wxStat(path, &status))
		{
			// wxStat failed - notify user???
		}
		fileSize = status.st_size;

		nInputLeft = fileSize;
		nChunks = fileSize / TwentyKB;
		if (fileSize % TwentyKB > 0) ++nChunks;
	
		// get the first 20kb of data, or however many there 
		// are if fewer, into the work buffer
		if (fileSize <= (Int32)TwentyKB)
		{
			// books.xml is 3,426 bytes so it doesn't require 
			// more than one chunk to be read
			nRead = file.Read(pBuff,fileSize);
			bReadAll = TRUE;
			nInputLeft = 0;
			nInputCount = fileSize;
		}
		else
		{
			// AI_USFM.xml is approx 80-90 Kbytes so it would
			// require at least 5 chunks to be read
			nRead = file.Read(pBuff,(Int32)TwentyKB);
			nInputCount = (Int32)TwentyKB;
			nInputLeft -= (Int32)TwentyKB;
		}
		nCurrChunk++;
	}
	else
	{
		// the stream was not opened, so tell the user 
		// BEW modified 08Nov05 so that the user did not get the message if the read was
		// attempted from a MRU list file choice by the user - these often no longer exist
		// and so when that happens we let the document class's OnOpenDocument() give a
		// nice cosy message and then the Start Working... wizard will open to help him out
		if (gbTryingMRUOpen)
		{
			return FALSE; // return with no message here
		}
		// otherwise, we have a more serious problem and the user will need to get a cause
		// to report to us if he can't solve the problem himself
		wxString errStr;
		errStr = errStr.Format(_("Could not open the XML data file %s for reading. "),path.c_str());

		// the ANSI C errno global variable probably reports the same int value
		// that is returned by CFileException's e.m_cause member. Since we don't
		// have the same exception processing facility in wxWidgets, we'll just
		// report the errno number
		errStr << _("\nIf you wish you can try opening some other document and continue working.");
		wxMessageBox(errStr, _T(""), wxICON_STOP);
		return FALSE;
	}

#ifdef Output_Default_Style_Strings
// create an output file for Unix-style default strings
	wxFile dfile;
	// create a .txt file to hold default strings based on the xml file name
	int posn;
	wxString dpath;
	posn = path.Find(_T('.'),TRUE); // TRUE is find from end //posn  = path.ReverseFind(_T('.'));
	if (posn != -1)
	{
		
		dpath = path.Left(posn); 
		dpath += _T(".txt");
	}

#ifdef _UNICODE
	wxString dwpath;
	gpApp->Convert8to16(dpath,dwpath);
	const wxChar* dpathP = dwpath;
#else
	const char* dpathP = (const char*)dpath;
#endif
	//CFileException de;
	bool bdOpenOK = dfile.Open( dpathP, wxFile::write); //bool bdOpenOK = dfile.Open(dpathP,nOpenFlags,&de);
	if (!bdOpenOK)
	{
		// the stream was not opened, so tell the user 
		wxString errStr = _T("Could not open the string defaults data file for writing. Cause: ");
		wxString cause;
		cause.Empty();
		cause << errno;
		errStr << cause;
		wxMessageBox(errStr, _T(""), wxICON_STOP);
		return FALSE;
	}
	
#endif

	// set up needed variables for the double-buffering which we do
	char* pPos = pBuff;
	char* pEnd = NULL;

	// BEW added 10Aug05 to permit an edited xml Adapt It document file, which may
	// have acquired a UTF-8 BOM in the process (unbeknown to the user), to still be
	// processed without any hiccups. Adapt It Unicode always produces XML output
	// which is UTF-8 and BOM-less, so any vagrant BOM will be eliminated at the
	// next save of the document.
#ifdef _UNICODE
	if (nCurrChunk == 1) // only do this when the first data chunk is in the buffer
	{
		// skip a BOM if the user edited the XML file in a
		// word processor in Unicode and the word processor added a BOM without
		// his/her knowledge - and if the BOM is the UTF-16 one, we'll abort the
		// parsing and tell the user to output the file from the word processor
		// again but with UTF-8 encoding, then try load it into Adapt It Unicode again
		if (!memcmp(pBuff,szU16BOM,nU16BOMLen))
		{
			// a UTF-16 BOM is present -- we can't handle UTF-16 as valid XML, so abort the load
			wxString path1 = path;
			path1 = MakeReverse(path1);
			int nFound = path1.Find(gpApp->PathSeparator);
			wxASSERT( nFound > 0);
			path1 = path1.Left(nFound); // get the filename
			path1 = MakeReverse(path1);
			wxString errStr;
			// IDS_UTF16_BOM_PRESENT
			errStr = errStr.Format(_("The XML file (%s) being loaded begins with a UTF-16 BOM (Byte Order Mark), so it is not UTF-8 data. Maybe you edited the file in a word processor and did not specify UTF-8 output. Please do that then try opening the document again."), path1.c_str());
			wxMessageBox(errStr,_T(""), wxICON_WARNING);
			//close the stream before returning
			file.Close();
			delete[] pBuff;
			return FALSE;
		}

		if (!memcmp(pBuff,szBOM,nBOMLen))
		{
			// a UTF-8 BOM is present -- we can handle this document, provided we skip the BOM
			pPos += nBOMLen; // skip over the BOM
		}
	}
	#endif

	if (bReadAll)
	{
		// entire xml file is in the work buffer
		pEnd = (char*)(pBuff + fileSize);
	}
	else
	{
		// start with pEnd pointing one past the end of the buffer
		pEnd = (char*)(pBuff + (Int32)TwentyKB);
	}

	// we need a stack on which we will push each pointer to a CBString holding
	// a tag name
	CStack stack; // an empty stack
	CStack* pStack = &stack; // pointer which we will pass to parsing functions
	CBString tagname; // where we will store a tagname

	// check for XML data
	// ... but first check for a BOM
	if (strncmp(pPos, bomUTF8, strlen(bomUTF8)) == 0)
		pPos += 3;

	Int16 comp = strncmp(pPos,xml,5);
	if (comp != 0)
	{
			//close the stream before returning
			file.Close();
			delete[] pBuff; // whm added 25Jan05
		
#ifdef Output_Default_Style_Strings
			dfile.Close();
#endif
		
		return FALSE; // its not an XML prologue
	}
	
	// it's XML, so skip the rest of the prologue
	// whm Note: Warning the following while loop would go infinite if the xml file were
	// truncated in the middle of the prologue, i.e., no '>' exists in the remainder of
	// the file! TODO: Fix This!! (add: && pPos < pEnd)
	while (*pPos != '>') pPos++;
	pPos++; // skip the final >
	SkipWhiteSpace(pPos,pEnd); // skip any newline and or indenting
	
	bool bOKElement = TRUE;
	bool bCallbackSucceeded = TRUE;
	do {
		char* pElStart = pPos; // save element start location
		char* pElEnd = NULL; // to be used for showing text in error dialog

		// skip any comment
r:		comp = strncmp(pPos,comment,4);
		if (comp == 0)
		{
			// we are at a comment
			pPos += 4;
			// whm Note: Warning the following while loop would go infinite if the xml file were
			// truncated in the middle of an xml comment, i.e., no '-->' exists in the remainder of
			// the file! TODO: Fix This!! (add: && pPos < pEnd)
			while (strncmp(pPos,endcomment,3))
			{
				pPos++;
			}
			pPos += 3;
			SkipWhiteSpace(pPos,pEnd);
			goto r;
		}
		// we are at an element, so parse it
#ifdef Output_Default_Style_Strings
		bOKElement = ParseXMLElement(dfile, pStack,tagname,pBuff,pPos,pEnd,
							bCallbackSucceeded,pAtTag,pAtEmptyElementClose,
							pAtAttr,pAtEndTag,pAtPCDATA);
#else
		bOKElement = ParseXMLElement(pStack,tagname,pBuff,pPos,pEnd,
							bCallbackSucceeded,pAtTag,pAtEmptyElementClose,
							pAtAttr,pAtEndTag,pAtPCDATA);
#endif
		if (!bOKElement)
		{
			Int32 offset = pPos - pElStart;
			pElEnd = FindElemEnd(pPos,pEnd); // up to & including /> or >
			CBString badElement;
			MakeStrFromPtrs(pElStart,pElEnd,badElement);
			ShowXMLErrorDialog(badElement,offset,bCallbackSucceeded);

			// close the stream before returning
			file.Close();
			delete[] pBuff;

#ifdef Output_Default_Style_Strings
			dfile.Close();
#endif
		
			return FALSE;
		}
	
		// update buffer contents if we have processed enough data...
		// After processing until less than 15KB of data remains, if there is another full or
		// partial chunk in the stream yet to be transferred to the work buffer
		// then remove the processed part, move the remnant down and append the
		// next full or partial chunk to the remnant, resetting pointers
		// appropriately. A 'chunk' is 20KB, so we never will get overflow.
		if (nCurrChunk >= nChunks || bReadAll)
			continue; // no more data to transfer
		Int32 fullspan = pEnd - pBuff;
		if (fullspan < (Int32)TwentyKB)
			continue;	// total data is less than 20kb character blocksize, so no 
						// refill is possible
		Int32 unprocessed = pEnd - pPos;
		if (unprocessed < (Int32)safelimit)
		{
			// a buffer refill is needed...			
			// move the unprocessed data to the start of the buffer
			// zero the space above it, and then transfer the next
			// chunk from the stream
			memmove(pBuff,pPos,unprocessed);
			pPos = pBuff;
			pEnd = (char*)(pBuff + unprocessed);
			Int32 theRest = (Int32)BUFFSIZE - unprocessed;
			memset(pEnd,0,theRest);
			if (nInputLeft <= (Int32)TwentyKB)
			{
				//nRead = file.Read(pBuff,nInputLeft); // doesn't work
				nRead = file.Read(pEnd,nInputLeft); // works
				pEnd = pBuff + unprocessed + nInputLeft;
				nInputLeft = 0;
				nInputCount = fileSize;
			}
			else
			{
				//nRead = file.Read(pBuff,(Int32)TwentyKB); // doesn't work
				nRead = file.Read(pEnd,(Int32)TwentyKB); // works
				pEnd = pBuff + unprocessed + (Int32)TwentyKB;
				nInputLeft -= (Int32)TwentyKB;
				nInputCount += (Int32)TwentyKB;
			}
			nCurrChunk++;
		}
	} while (!stack.IsEmpty() && (pPos < pEnd));
	if (!stack.IsEmpty())
	{
		// nesting of elements got out of synch, document malformed
		// close the stream before returning
		file.Close();
		delete[] pBuff;

#ifdef Output_Default_Style_Strings
			dfile.Close();
#endif
		
		return FALSE;
	}
	
	// remove the buffer; also close the file stream
	// close the stream before returning
	file.Close();
	delete[] pBuff;

#ifdef Output_Default_Style_Strings
	wxString numUSFMs;
	numUSFMs.Empty();
	numUSFMs << numUSFMMarkersOutput;
	numUSFMs << _T("\n");
	dfile.Write(numUSFMs,numUSFMs.Length());
	dfile.Close();
	numUSFMMarkersOutput = 0;
#endif

	return TRUE;
}


#ifdef Output_Default_Style_Strings
bool ParseXMLElement(wxFile& dfile,
		CStack*& pStack,CBString& tagname,char*& pBuff,
		char*& pPos,char*& pEnd,bool& bCallbackSucceeded,
		bool (*pAtTag)(CBString& tag,CStack*& pStack),
		bool (*pAtEmptyElementClose)(CBString& tag,CStack*& pStack),
		bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue,CStack*& pStack),
		bool (*pAtEndTag)(CBString& tag,CStack*& pStack),
		bool (*pAtPCDATA)(CBString& tag,CBString& pcdata,CStack*& pStack))

#else
bool ParseXMLElement(CStack*& pStack,CBString& tagname,char*& pBuff,
		char*& pPos,char*& pEnd,bool& bCallbackSucceeded,
		bool (*pAtTag)(CBString& tag,CStack*& pStack),
		bool (*pAtEmptyElementClose)(CBString& tag,CStack*& pStack),
		bool (*pAtAttr)(CBString& tag,CBString& attrName,CBString& attrValue,CStack*& pStack),
		bool (*pAtEndTag)(CBString& tag,CStack*& pStack),
		bool (*pAtPCDATA)(CBString& tag,CBString& pcdata,CStack*& pStack))
#endif
{
	char* pAux = pPos;
	bool bWithinOpeningTag = FALSE;
	bool bHaltedAtSpace = FALSE;
	CBString check;
	CBString attrName;
	CBString attrValue;
	CBString pcdata;
	bool bIsWhite;
	bool bContainsAttr;
	char* ptag;
	bool bValidTag;
	
	// we should enter with pPos pointing at an <, but play safe
	SkipWhiteSpace(pPos,pEnd); // skip any newline and or indenting

	if (strncmp(pPos,"</",2) == 0)
	{
		// we are at the end tag which terminates one or more
		// nested elements, so parse it
		goto f;
	}
	else if (*pPos == '<')
	{
		// we are at the start of an opening tag
		pPos++; // skip the < wedge
		goto go;
	}
	else
	{
		// we must have entered after having parsed a closing tag
		// which wraps a nested group of elements; so we could be
		// at the end of the data, or there may be more PCDATA to
		// parse in an element not yet closed
		if (pStack->IsEmpty())
		{
			// we are at the end of the data
			SkipWhiteSpace(pPos,pEnd);
			return TRUE;
		}
		else
		{
			// stack is not empty yet
			SkipWhiteSpace(pPos,pEnd);
			if (pPos == pEnd)
			{
				// we are at the end of the data
				// so the document must be malformed
				return FALSE;
			}
		
			// only possibility left is that we are halted at
			// yet more PCDATA, so assume so & parse it, after
			// first restoring the current tag's name to tagname
			// (so callbacks can use tagname) and pushing it back on
			// the stack to keep our nesting level correct
			pStack->Pop(gStoreItem);
			tagname = gStoreItem;
			pStack->Push(gStoreItem);
			bWithinOpeningTag = FALSE;
			goto b;
		}
	}
	
	// identify the tagname, and push onto the stack
go:	pAux = pPos; // save start of tag location
	bValidTag = ParseTag(pPos,pEnd,bHaltedAtSpace);
	if (!bValidTag)
		return FALSE; // halt parsing
	MakeStrFromPtrs(pAux,pPos,tagname);
	ptag = (char*)tagname;
	pStack->Push(ptag);
	bWithinOpeningTag = TRUE;
	bContainsAttr = FALSE;

	// allow the application to do something now the tag has been identified
	bCallbackSucceeded = (*pAtTag)(tagname,pStack); // BEW 4Jun10, added pStack param
	if (!bCallbackSucceeded)
		return FALSE; // halt parsing if the callback fails
	bCallbackSucceeded = TRUE; // reset default, in case markup error follows

	// branch, depending on what the pPos is pointing at - either white space,
	// or /> for an empty element, or > for a normal element (which could still
	// be empty)
	bIsWhite = IsWhiteSpace(pPos,pEnd);
	if (bIsWhite)
	{
		// we are at a space, tab or newline (& still within opening tag)
		SkipWhiteSpace(pPos,pEnd);
		if (bWithinOpeningTag)
		{
			// we must have found an attribute, so parse it
e:			bContainsAttr = TRUE;
			bool bOKAttr = ParseXMLAttribute(tagname,pBuff,pPos,pEnd,
												attrName,attrValue);
			if (!bOKAttr)
			{
				// there was invalid attribute structure
				return FALSE;
			}
			bHaltedAtSpace = FALSE; // turn it off once an attribute
									// has been encountered
			
			// allow the app to do something with the attribute name
			// and its string value, using a callback
			bCallbackSucceeded = (*pAtAttr)(tagname,attrName,attrValue,pStack); // BEW 4Jun10, added pStack param
			if (!bCallbackSucceeded)
				return FALSE; // halt parsing if the callback fails
			bCallbackSucceeded = TRUE; // reset default, in case markup error follows
			
			// we are approaching either another attribute or the end
			// of the opening tag - find out which and branch accordingly
			SkipWhiteSpace(pPos,pEnd);
			if (strncmp(pPos,"/>",2) == 0)
				goto c; // parse end of empty element
			if (*pPos == '>')
				goto d; // parse end of normal element
			// only possibility left is that we are at the start of
			// another attribute, so loop
			goto e;
		}
		else
		{
			// we are not within an opening tag, so probably at the
			// start of PCDATA - assume so & parse it
			goto b;
		}
	}
	else if (strncmp(pPos,"</",2) == 0)
	{
		// we are at the start of a closing tag, so close the element off
f:		pPos += 2;
		if (bWithinOpeningTag)
		{
			// malformed document
			return FALSE;
		}
		
		// parse the tagname, and check it matches the opening one
		pAux = pPos; // save start of tag location
		if (bHaltedAtSpace)
		{
			// the previous opening tag was parsed to a terminating space,
			// and so an attribute was expected and none was encountered
			pPos -= 3; // attempt to point at something indicative of the error
			return FALSE;
		}
		bool bValidTag = ParseClosingTag(pPos,pEnd);
		if (!bValidTag)
			return FALSE; // halt parsing
		MakeStrFromPtrs(pAux,pPos,check);
		pStack->Pop(gStoreItem);
		tagname = gStoreItem;
		if (tagname != check)
		{
			// our nesting has got out of synch, so doc is malformed
			return FALSE;
		}
		if (*pPos != '>')
		{
			// malformed document
			return FALSE;
		}
		pPos++; // jump the final >
		
		// all's well, so allow the application to do something now the 
		// closing tag has been identified
		bCallbackSucceeded = (*pAtEndTag)(tagname,pStack); // BEW 4Jun10, added pStack param
		if (!bCallbackSucceeded)
			return FALSE; // halt parsing if there was a callback failure
		bCallbackSucceeded = TRUE; // reset default, in case markup error follows
		
		// skip white space and return to caller
		SkipWhiteSpace(pPos,pEnd);
		goto h;
	}
	else if (strncmp(pPos,"/>",2) == 0)
	{
		// we are at the close of an empty element (empty elements can have
		// attributes though)
c:		if (!bWithinOpeningTag)
		{
			// we have a malformed XML document
			return FALSE;
		}
		if (bHaltedAtSpace)
		{
			// the opening tag was terminated by a space, so an attribute
			// was expected but none was parsed before this end of the
			// tag was encountered, so the document is malformed
			--pPos; // try to point at something indicative of the error
			return FALSE;
		}
		if (!bContainsAttr)
		{
			// we have an element of the form <TAG/> ie. empty, so we
			// need to allow the callback for PCDATA to register an
			// empty string just in case this attribute can sometimes be
			// <TAG> PCDATA </TAG>, that is, sometimes have nonempty PCDATA
			pcdata.Empty();
			bCallbackSucceeded = (*pAtPCDATA)(tagname,pcdata,pStack); // Bill added this one
			if (!bCallbackSucceeded)
				return FALSE; // halt parsing if there was a callback failure
			bCallbackSucceeded = TRUE; // reset default, in case markup error follows
		}
		pPos += 2;
		
		// allow the app to do something now we are at the closure of
		// an empty tag (which may have had attributes)
		bCallbackSucceeded = (*pAtEmptyElementClose)(tagname,pStack); // BEW 4Jun10, added pStack param
		if (!bCallbackSucceeded)
			return FALSE; // halt parsing if there was a callback failure
		bCallbackSucceeded = TRUE; // reset default, in case markup error follows
		
		// tidy up & parse to start of next element
		bWithinOpeningTag = FALSE;
		SkipWhiteSpace(pPos,pEnd);
		
		// element is finished so pop the tagname from the top of stack
		// and allow control to return to the caller
		check = tagname;
		pStack->Pop(gStoreItem);
		tagname = gStoreItem;
		if (tagname != check)
		{
			// our nesting has got out of synch, so doc is malformed
			return FALSE;
		}
	}
	else if (*pPos == '>')
	{
		// we are at the close of a normal opening tag (or should be)
d:		if (bHaltedAtSpace)
		{
			// the opening tag was terminated by a space, so an attribute
			// was expected but none was parsed before this end of the
			// tag was encountered, so the document is malformed
			--pPos; // point at the bad space (or newline or tab)
			return FALSE;
		}
		pPos++; // skip the >
		if (!bWithinOpeningTag)
		{
			// we have a malformed XML document
			return FALSE;
		}
b:		bWithinOpeningTag = FALSE;
		SkipWhiteSpace(pPos,pEnd);

		// what now follows could be PCDATA, or a closing tag </TAG..., 
		// or the opening tag of a nested element..
		if (strncmp(pPos,"</",2) == 0)
		{
			// we are at the start of a closing tag, so close off;
			// but since there was a preceding > and nothing but
			// white space before this closing tag, this element is
			// therefore empty - so we must allow the callback for
			// PCDATA to find an empty string here.
			pcdata.Empty();
			bCallbackSucceeded = (*pAtPCDATA)(tagname,pcdata,pStack); // Bill added pStack
			if (!bCallbackSucceeded)
				return FALSE; // alert the user if there was a callback failure
			bCallbackSucceeded = TRUE; // reset default, in case markup error follows
			goto f;
		}
		else if (*pPos == '<')
		{
			// we are about to enter a nested element, or a nested
			// group, so return & iterate - this is the only circumstance
			// we exit without completing the element (except for errors),
			// and this is what makes our stack grow to match the
			// nesting level
			goto h;
		}
		else
		{
			// can only be PCDATA if the document is not malformed, so
			// attempt to parse it
			bool bOKpcDATA = ParsePCDATA(pPos,pEnd,pcdata);
			if (!bOKpcDATA)
			{
				// bad PCDATA, document is malformed
				return FALSE;
			}
			
			// allow the application to do something with the PCDATA
			bCallbackSucceeded = (*pAtPCDATA)(tagname,pcdata,pStack); // Bill added pStack
			if (!bCallbackSucceeded)
				return FALSE; // alert the user if there was a callback failure
			bCallbackSucceeded = TRUE; // reset default, in case markup error follows
			
			// we are now halted at either </ of the element's closing
			// tag, or at < where nesting begins - branch accordingly
			if (strncmp(pPos,"</",2) == 0)
			{
				//we are at the close tag, so parse it and return
				goto f;
			}
			else
			{
				// about to commence nesting, so return
				goto h;
			}
		}
	}
h:	
#ifdef Output_Default_Style_Strings
	if (usfmUnixDataStr.length() != 0)
	{
		usfmUnixDataStr = _T("_T(\"") + usfmUnixDataStr;
		usfmUnixDataStr += _T("\"),\n");
		int len = usfmUnixDataStr.Length();
		dfile.Write(usfmUnixDataStr,len);
		usfmUnixDataStr = _T(""); // prepare for new default string accumulation
		numUSFMMarkersOutput++;
	}
#endif

	return TRUE;
}

bool ParseXMLAttribute(CBString& WXUNUSED(tagname),char*& WXUNUSED(pBuff),char*& pPos,char*& pEnd,
						CBString& attrName,CBString& attrValue)
{
	char* pAux = pPos; // preserve start location
	bool bValidAttrNm = ParseAttrName(pPos,pEnd);
	if (!bValidAttrNm)
	{
		// malformed document
		return FALSE;
	}
	else
	{
		MakeStrFromPtrs(pAux,pPos,attrName);
	}
	// BEW 3Jun10, ParseAttrName() was changed internally to allow the following
	// substrings to terminate the attr name's parse (they vary with respect to spaces):
	// attrname="value"   <<-- formerly we accepted just this, but LIFT can be different
	// attrname ="value"
	// attrname = "value"
	// attrname= "value"
	// So now we need not to test that =" follows, but rather parse over the space(s) and
	// the first " character as well
	while (*pPos == '=' || *pPos == ' ') { pPos++; }
	if (*pPos == '\"') { pPos++; }
	// pointing now to start of attribute, keep location
	pAux = pPos;
	bool bOkAttr = ParseAttrValue(pPos,pEnd);
	if (!bOkAttr)
	{
		// malformed document
		pPos = pAux; // put pPos at start of what should be the attr value
		return FALSE;
	}
	if (*pPos != '\"')
	{
		// malformed document
		return FALSE;
	}
	MakeStrFromPtrs(pAux,pPos,attrValue);
#ifdef Output_Default_Style_Strings
	CBString attrTemp = attrValue;
	if (strcmp(attrTemp,"0") == 0)
		attrTemp.Empty(); // make zeros be empty fields
	// by using a string integer value for the enum values rather than storing 
	// the string name of the StyleType, TextType, and Justification enums, we
	// can save a lot of program code memory space. 
	if (strcmp(attrName, texttypeStr) == 0)
	{
		if (strcmp(attrTemp, noTypeStr) == 0)
			_itot(noType, attrTemp, 10); //"0"	// get string equiv of enum integral value
		else if (strcmp(attrTemp,verseStr) == 0)
			_itot(verse, attrTemp, 10); //"1"	// in case it gets explicitly put in AI_stlye.xml
		else if (strcmp(attrTemp, poetryStr) == 0)
			_itot(poetry, attrTemp, 10); //"2"
		else if (strcmp(attrTemp, sectionHeadStr) == 0) 
			_itot(sectionHead, attrTemp, 10); //"3"
		else if (strcmp(attrTemp, mainTitleStr) == 0) 
			_itot(mainTitle, attrTemp, 10); //"4"
		else if (strcmp(attrTemp, secondaryTitleStr) == 0) 
			_itot(secondaryTitle, attrTemp, 10); //"5"
		else if (strcmp(attrTemp, noneStr) == 0) 
			_itot(none, attrTemp, 10); //"6" BEW added 23May05
		else if (strcmp(attrTemp, footnoteStr) == 0) 
			_itot(footnote, attrTemp, 10); //"9"
		else if (strcmp(attrTemp, headerStr) == 0)  
			_itot(header, attrTemp, 10); //"10"
		else if (strcmp(attrTemp, identificationStr) == 0) 
			_itot(identification, attrTemp, 10); //"11"
		else if (strcmp(attrTemp, rightMarginReferenceStr) == 0) 
			_itot(rightMarginReference, attrTemp, 10); //"32"
		else if (strcmp(attrTemp, crossReferenceStr) == 0) 
			_itot(crossReference, attrTemp, 10); //"33"
		else if (strcmp(attrTemp, noteStr) == 0) 
			_itot(note, attrTemp, 10); //"34"
		else
			attrTemp = "??"; // use "??" to flag unknown TextType in output text strings
	}
	else if (strcmp(attrName, styletypeStr) == 0)
	{
		// this group are for the styleType equivalent string names
		if (strcmp(attrTemp, paragraphStr) == 0) 
			_itot(paragraph, attrTemp, 10); // "0" // in case it gets explicitly put in AI_stlye.xml
		else if (strcmp(attrTemp, characterStr) == 0) 
			_itot(character, attrTemp, 10); // "1"
		else if (strcmp(attrTemp, tableStr) == 0)
			_itot(table_type, attrTemp, 10); // "2"
		else if (strcmp(attrTemp, footnote_callerStr) == 0)
			_itot(footnote_caller, attrTemp, 10); // "3"
		else if (strcmp(attrTemp, footnote_textStr) == 0)
			_itot(footnote_text, attrTemp, 10); // "4"
		else if (strcmp(attrTemp, default_para_fontStr) == 0)
			_itot(default_para_font, attrTemp, 10); // "5"
		else if (strcmp(attrTemp, footerStyStr) == 0)
			_itot(footerSty, attrTemp, 10); // "6"
		else if (strcmp(attrTemp, headerStyStr) == 0)
			_itot(headerSty, attrTemp, 10); // "7"
		else if (strcmp(attrTemp, horiz_ruleStr) == 0)
			_itot(horiz_rule, attrTemp, 10); // "8"
		else if (strcmp(attrTemp, boxed_paraStr) == 0)
			_itot(boxed_para, attrTemp, 10); // "10"
		else if (strcmp(attrTemp, hnoteStr) == 0)
			_itot(hidden_note, attrTemp, 10); // "11"
		else
			attrTemp = "??";  // use "??" to flag unknown StyleType in output text strings
		//noteStr note // UBS uses this as a StyleType
	}
	else if (strcmp(attrName, justificationStr) == 0)
	{
		// this group are for the justification equivalent string names
		if (strcmp(attrTemp, leadingStr) == 0) 
			_itot(leading, attrTemp, 10); // "0"; // in case it gets explicitly put in AI_stlye.xml
		else if (strcmp(attrTemp, followingStr) == 0) 
			_itot(following, attrTemp, 10); // "1";
		else if (strcmp(attrTemp, centerStr) == 0) 
			_itot(center, attrTemp, 10); // "2";
		else if (strcmp(attrTemp, justifiedStr) == 0) // added for version 3
			_itot(justified, attrTemp, 10); // "3";
		else
			attrTemp = "??";  // use "??" to flag unknown Justification in output text strings
	}

	usfmUnixDataStr += attrTemp; // accumulate Unix-style data field plus delimiter
	usfmUnixDataStr += _T(':'); 
#endif
	pPos++; // jump the final quote char
	return TRUE;
}

bool ParsePCDATA(char*& pPos,char* pEnd,CBString& pcdata)
{
	char* pAux = pPos;
	do {
		// use for parsing PCDATA and attribute values
		if ( (*pPos == '<') || (*pPos == '\"') || (pPos == pEnd)
		|| (*pPos == '>') || (strncmp(pPos,"/>",2) == 0))
		{
			break;
		}
		else
		{
			if (pPos < pEnd)
				pPos++;
			else
				break;
		}
	} while (pPos < pEnd);
	if (*pPos == '\"')
	{
		// we've found a doublequote symbol, which means we are parsing
		// an attribute name and shouldn't be, so the document is malformed
		--pPos; //point at any preceding = symbol, which should be there
		return FALSE;
	}
	if (pPos == pEnd)
	{
		// malformed document
		return FALSE;
	}
	if ((*pPos == '>') || (strncmp(pPos,"/>",2) == 0))
	{
		// closing tag must have lacked opening <, so try
		// get pPos back to the error location
		while (!IsWhiteSpace(pPos,pEnd))
		{
			--pPos;
		}
		++pPos; // hopefully we are pointing at start of end tag
				// which lacks its opening < wedge
		return FALSE;
	}

	// scan back over any final white space at the end of the PCDATA
	char* pSavePos = pPos;
	char* pFinis = pPos;
	do {
		--pFinis;
		if (pFinis < pAux)
		{
			// the PCDATA must have been an empty string,
			// so just return an empty string
			pPos = pSavePos;
			pcdata.Empty();
			return TRUE;
		}
		if (IsWhiteSpace(pFinis,pEnd))
			continue;
		else
		{
			++pFinis;
			break;
		}
	} while (pFinis > pAux);
	MakeStrFromPtrs(pAux,pFinis,pcdata);

	//MakeStrFromPtrs(pAux,pPos,pcdata);
	return TRUE;
}

/***************************************************************************
* Callbacks - these interface the parsed XML data to the rest of the app.
* 
* Return Value: each returns a Boolean, TRUE if there was no detectable data
*  error (detection of such errors is the callback designer's responsibility),
* FALSE if something unknown was detected, or some other error condition was
* encountered. Note: returning FALSE halts XML parsing irreversibly, so don't
* return FALSE for when the callback just wants to ignore something but allow
* parsing to continue.
*
* Each callback passes in the name of the tag for the current element, and
* some callbacks pass in more: the callback for parsing an attribute passes
* in the attribute name and its (string) value as well, and the callback for
* PCDATA also passes in the string for the PCDATA.
*
* By defining different sets of callback functions, the application designer
* can make the application be able to respond to different types of XML
* document content. To use a certain set of callbacks in your app's code,
* define the callback set, and then at the place where parsing of input data
* is to be done, call ParseXML and pass in the names of your five callbacks.
* Callbacks used with other XML document types can be used if the same element
* is used in the new XML document type and the app is to respond in the same 
* way as before.
*
* For the unicode app, some of the parsed strings (UTF-8) will need to be converted to
* UTF-16, so a buffer-based (eg. alloca()) conversion function will be needed.
*
****************************************************************************/

bool AtBooksTag(CBString& tag,CStack*& WXUNUSED(pStack))
{	
	if (tag == namepair)
	{
		// create a new struct to accept the values from the element's attributes
		gpBookNamePair = new BookNamePair;

		// increment the DIVision's count, and the total count
		divCount++;
		totalCount++;
	}
	else if (tag == books) // if it's a "BOOKS" tag
	{
		// do nothing except re-initialize static variables, since the app class's m_pBibleBooks 
		// CPtrArray is already ready for pointers to be added (it gets created when CAdapt_ItApp
		// class is created)
		divIndex = -1; // add 1 each time a DIV element is encountered
		divCount = 0; // add 1 each time a NAMEPAIR element is encountered, & reset to 0 for each DIV element
		totalCount = 0; // add 1 each time a NAMEPAIR element is encountered, but don't reset except here
	}
	else if (tag == division) // if it's a "DIV" tag
	{
		// reset the DIVision's counter to initialize in preparation for the next division
		divCount = 0;
		// increment the index for the division's array(s)  for the next division (used in AtBooksAttr())
		divIndex++;
	}
	else
	{
		return FALSE; // unknown element, so signal the error to the caller
	}
	return TRUE; // no error
}

bool AtBooksEmptyElemClose(CBString& tag,CStack*& WXUNUSED(pStack))
{
	if (tag == namepair)
	{
		// save the now complete BookNamePair instance in m_pBibleBooks CPtrArray
		// and then clear the pointer to the struct
		gpApp->m_pBibleBooks->Add((void*)gpBookNamePair);
		gpBookNamePair = NULL;

		// update the variable which preserves how many there are in each division
		// (this is reset to a higher value every time a new NAMEPAIR is parsed, so the 
		// final value in it will be whatever was the divCount value for the NAMEPAIR
		// element which is last in the current DIVision)
		gpApp->m_nDivSize[divIndex] = divCount;
	}
	return TRUE; // unused
}

// the Unicode app will need a conditional compile here
// because the UTF-8 will have to be converted to UTF-16; since BookNamePair
// contains CStrings internally, not CBStrings

bool AtBooksAttr(CBString& tag,CBString& attrName,CBString& attrValue,CStack*& WXUNUSED(pStack))
{
#ifdef _UNICODE // Unicode application
	wxString valueStr;
	gpApp->Convert8to16(attrValue,valueStr);
	const wxChar* pValue = valueStr;
#else // ANSI application
	char* pValue = (char*)attrValue;
#endif 
	if (tag == namepair)
	{
		wxASSERT(gpBookNamePair); // check it only once
		if (attrName == folder)
		{
			gpBookNamePair->dirName = pValue;
		}
		else if (attrName == view)
		{
			gpBookNamePair->seeName = pValue;
		}
		else if (attrName == code)
		{
			gpBookNamePair->bookCode = pValue;
		}
		else
		{
			// unknown attribute, which is an error
			return FALSE;
		}
	}
	else if (tag == division)
	{
		if (attrName == title)
		{
			gpApp->m_strDivLabel[divIndex] = pValue;
		}
		else
		{
			return FALSE; // unknown attribute, which is an error
		}
	}
	else if (tag == books)
	{
		if (attrName == defaultbook)
		{
			// the XML attribute has a 1-based number, which we must
			// decrement to get a 0-based index
			gpApp->m_nDefaultBookIndex = wxAtoi(pValue) - 1;
		}
		else
		{
			// no other attrs in the BOOKS element, so it's an error to get to here
			return FALSE;
		}
	}
	return TRUE;
}

bool AtBooksPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata),CStack*& WXUNUSED(pStack))
{
	// there is no PCDATA in books.xml
	return TRUE;
}

bool AtBooksEndTag(CBString& tag,CStack*& WXUNUSED(pStack))
{
	if (tag == books)
	{
		// preserve the count of how many books there are (sum over count for each division)
		gpApp->m_nTotalBooks = totalCount;
	}
	return TRUE;
}

/***************************************************************************
* Callbacks - for parsing the AI_USFM.xml data. whm added 19Jan05
****************************************************************************/
bool AtSFMTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	if (tag == sfm)
	{
		// create a new struct to accept the values from the sfm's attributes
		gpUsfmAnalysis = new USFMAnalysis;
		gpUsfmAnalysis->marker = _T("");
		gpUsfmAnalysis->endMarker = _T("");
		gpUsfmAnalysis->description = _T("");
		gpUsfmAnalysis->usfm = FALSE;
		gpUsfmAnalysis->png = FALSE;
		gpUsfmAnalysis->filter = FALSE;
		gpUsfmAnalysis->userCanSetFilter = FALSE;
		gpUsfmAnalysis->inLine = FALSE;
		gpUsfmAnalysis->special = FALSE;
		gpUsfmAnalysis->bdryOnLast = FALSE;
		gpUsfmAnalysis->inform = FALSE;
		gpUsfmAnalysis->navigationText = _T("");
		gpUsfmAnalysis->textType = verse;		// use verse unless xml file explicitly changes it
		gpUsfmAnalysis->wrap = FALSE;
		gpUsfmAnalysis->styleName = _T(""); // added for version 3
		gpUsfmAnalysis->styleType = paragraph;	// use paragraph unless xml file explicitly changes it
		gpUsfmAnalysis->fontSize = 12;			// use 12 points unless xml file explicitly changes it
												// a fontSize of 0 indicates font size is not a significant
												// attribute for the current marker and the View's 
												// BuildRTFStyleTagString() function will not add
												// \fsN tag to the RTF style definiton string
		gpUsfmAnalysis->color = 0;
		gpUsfmAnalysis->italic = FALSE;
		gpUsfmAnalysis->bold = FALSE;
		gpUsfmAnalysis->underline = FALSE;
		gpUsfmAnalysis->smallCaps = FALSE;
		gpUsfmAnalysis->superScript = FALSE;
		gpUsfmAnalysis->justification = justified;// use justified unless xml file explicitly changes it
		gpUsfmAnalysis->spaceAbove = 0;
		gpUsfmAnalysis->spaceBelow = 0;
		gpUsfmAnalysis->leadingMargin = 0.0;
		gpUsfmAnalysis->followingMargin = 0.0;
		gpUsfmAnalysis->firstLineIndent = 0.0;
		gpUsfmAnalysis->basedOn = _T(""); // added for version 3
		gpUsfmAnalysis->nextStyle = _T(""); // added for version 3
		gpUsfmAnalysis->keepTogether = FALSE; // added for version 3
		gpUsfmAnalysis->keepWithNext = FALSE; // added for version 3
	}
	return TRUE;
}

bool AtSFMEmptyElemClose(CBString& WXUNUSED(tag), CStack*& WXUNUSED(pStack))
{
	// AI_USFM.xml uses the explicit element name in close tags, i.e.,
	// <SFM ... attributes ...></SFM> so it doesn't add elements to the
	// CMapStringToObs here, but below in AtSFMEndTag(). books.xml
	// on the other hand processes its NAMEPAIR elements at the corresponding
	// AtBooksEmptyElemClose() function because the attributes for the
	// NAMEPAIR elements are terminated with an empty element close </>
	// rather than </NAMEPAIR>.

	return TRUE; // unused
}

// the Unicode app will need a conditional compile here
// because the UTF-8 will have to be converted to UTF-16; since BookNamePair
// contains CStrings internally, not CBStrings

bool AtSFMAttr(CBString& tag,CBString& attrName,CBString& attrValue, CStack*& WXUNUSED(pStack))
{
	wxASSERT(gpUsfmAnalysis);
#ifdef _UNICODE
	wxString valueStr;
	gpApp->Convert8to16(attrValue,valueStr);
	const wxChar* pValueW = valueStr; //const wxChar* pValueW = (LPCTSTR)valueStr;
#endif
	char* pValue = (char*)attrValue; // always do this one, whether unicode or not
	if (tag == sfm)
	{
		if (attrName == markerStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->marker = pValueW;
#else
			gpUsfmAnalysis->marker = pValue;
#endif
		}
		else if (attrName == endmarkerStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->endMarker = pValueW;
#else
			gpUsfmAnalysis->endMarker = pValue;
#endif
		}
		else if (attrName == descriptionStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->description = pValueW;
#else
			gpUsfmAnalysis->description = pValue;
#endif
		}
		else if (attrName == usfmStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->usfm = TRUE;
			else
				gpUsfmAnalysis->usfm = FALSE;
		}
		else if (attrName == pngStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->png = TRUE;
			else
				gpUsfmAnalysis->png = FALSE;
		}
		else if (attrName == filterStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->filter = TRUE;
			else
				gpUsfmAnalysis->filter = FALSE;
		}
		else if (attrName == usercansetfilterStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->userCanSetFilter = TRUE;
			else
				gpUsfmAnalysis->userCanSetFilter = FALSE;
		}
		else if (attrName == inlineStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->inLine = TRUE;
			else
				gpUsfmAnalysis->inLine = FALSE;
		}
		else if (attrName == specialStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->special = TRUE;
			else
				gpUsfmAnalysis->special = FALSE;
		}
		else if (attrName == bdryonlastStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->bdryOnLast = TRUE;
			else
				gpUsfmAnalysis->bdryOnLast = FALSE;
		}
		else if (attrName == informStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->inform = TRUE;
			else
				gpUsfmAnalysis->inform = FALSE;
		}
		else if (attrName == navigationtextStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->navigationText = pValueW;
#else
			gpUsfmAnalysis->navigationText = pValue;
#endif
		}
		else if (attrName == texttypeStr)
		{
			// Note: If textTypes are added or removed from this code, be sure to
			// make adjustments to the TextType enum in SourcePhrase.h
			if (strcmp(pValue, verseStr) == 0)
				gpUsfmAnalysis->textType = verse;
			else if (strcmp(pValue, poetryStr) == 0) 
				gpUsfmAnalysis->textType = poetry;
			else if (strcmp(pValue, sectionHeadStr) == 0) 
				gpUsfmAnalysis->textType = sectionHead;
			else if (strcmp(pValue, mainTitleStr) == 0) 
				gpUsfmAnalysis->textType = mainTitle;
			else if (strcmp(pValue, secondaryTitleStr) == 0) 
				gpUsfmAnalysis->textType = secondaryTitle;
			else if (strcmp(pValue, noneStr) == 0) 
				gpUsfmAnalysis->textType = none; // BEW added 23May05
			else if (strcmp(pValue, footnoteStr) == 0) 
				gpUsfmAnalysis->textType = footnote;
			else if (strcmp(pValue, headerStr) == 0)
				gpUsfmAnalysis->textType = header;
			else if (strcmp(pValue, identificationStr) == 0) 
				gpUsfmAnalysis->textType = identification;
			else if (strcmp(pValue, rightMarginReferenceStr) == 0) 
				gpUsfmAnalysis->textType = rightMarginReference;
			else if (strcmp(pValue, crossReferenceStr) == 0) 
				gpUsfmAnalysis->textType = crossReference;
			else if (strcmp(pValue, noteStr) == 0) 
				gpUsfmAnalysis->textType = note;
			else if (strcmp(pValue, noTypeStr) == 0)
				gpUsfmAnalysis->textType = noType;
			// no else clause here - a new struct defaults to verse as its textType 
			// unless explicitly changed above
		}
		else if (attrName == wrapStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->wrap = TRUE;
			else
				gpUsfmAnalysis->wrap = FALSE;
		}
		else if (attrName == stylenameStr) // added for version 3
		{
#ifdef _UNICODE
			gpUsfmAnalysis->styleName = pValueW;
#else
			gpUsfmAnalysis->styleName = pValue;
#endif
		}
		else if (attrName == styletypeStr)
		{
			if (strcmp(pValue, paragraphStr) == 0)
				gpUsfmAnalysis->styleType = paragraph;
			else if (strcmp(pValue, characterStr) == 0)
				gpUsfmAnalysis->styleType = character;
			else if (strcmp(pValue, tableStr) == 0)
				gpUsfmAnalysis->styleType = table_type; // whm added 21 Oct05
			else if (strcmp(pValue, footnote_callerStr) == 0)
				gpUsfmAnalysis->styleType = footnote_caller; // " " "
			else if (strcmp(pValue, footnote_textStr) == 0)
				gpUsfmAnalysis->styleType = footnote_text; // " " "
			else if (strcmp(pValue, default_para_fontStr) == 0)
				gpUsfmAnalysis->styleType = default_para_font; // " " "
			else if (strcmp(pValue, footerStyStr) == 0)
				gpUsfmAnalysis->styleType = footerSty; // " " "
			else if (strcmp(pValue, headerStyStr) == 0)
				gpUsfmAnalysis->styleType = headerSty; // " " "
			else if (strcmp(pValue, horiz_ruleStr) == 0)
				gpUsfmAnalysis->styleType = horiz_rule; // " " "
			else if (strcmp(pValue, boxed_paraStr) == 0)
				gpUsfmAnalysis->styleType = boxed_para; // " " "
			else if (strcmp(pValue, hnoteStr) == 0)
				gpUsfmAnalysis->styleType = hidden_note; // " " "
			// no else clause here - - a new struct defaults to paragraph as its styleType 
			// unless explicitly changed above
		}
		else if (attrName == fontsizeStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->fontSize = wxAtoi(pValueW);
#else
			gpUsfmAnalysis->fontSize = wxAtoi(pValue);
#endif
		}
		else if (attrName == colorStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->color = wxAtoi(pValueW);
#else
			gpUsfmAnalysis->color = wxAtoi(pValue);
#endif
		}
		else if (attrName == italicStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->italic = TRUE;
			else
				gpUsfmAnalysis->italic = FALSE;
		}
		else if (attrName == boldStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->bold = TRUE;
			else
				gpUsfmAnalysis->bold = FALSE;
		}
		else if (attrName == underlineStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->underline = TRUE;
			else
				gpUsfmAnalysis->underline = FALSE;
		}
		else if (attrName == smallcapsStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->smallCaps = TRUE;
			else
				gpUsfmAnalysis->smallCaps = FALSE;
		}
		else if (attrName == superscriptStr)
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->superScript = TRUE;
			else
				gpUsfmAnalysis->superScript = FALSE;
		}
		else if (attrName == justificationStr)
		{
			if (strcmp(pValue, leadingStr) == 0)
				gpUsfmAnalysis->justification = leading;
			else if (strcmp(pValue, centerStr) == 0)
				gpUsfmAnalysis->justification = center;
			else if (strcmp(pValue, followingStr) == 0)
				gpUsfmAnalysis->justification = following;
			else  // added for version 3
				gpUsfmAnalysis->justification = justified;
			// a new struct defaults to justified as its justification 
			// unless explicitly changed above
		}
		else if (attrName == spaceaboveStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->spaceAbove = wxAtoi(pValueW);
#else
			gpUsfmAnalysis->spaceAbove = wxAtoi(pValue);
#endif
		}
		else if (attrName == spacebelowStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->spaceBelow = wxAtoi(pValueW);
#else
			gpUsfmAnalysis->spaceBelow = wxAtoi(pValue);
#endif
		}
		else if (attrName == leadingmarginStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->leadingMargin = (float)wxAtof(pValueW);
#else
			gpUsfmAnalysis->leadingMargin = (float)atof(pValue);
#endif
		}
		else if (attrName == followingmarginStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->followingMargin = (float)wxAtof(pValueW);
#else
			gpUsfmAnalysis->followingMargin = (float)atof(pValue);
#endif
		}
		else if (attrName == firstlineindentStr)
		{
#ifdef _UNICODE
			gpUsfmAnalysis->firstLineIndent = (float)wxAtof(pValueW);
#else
			gpUsfmAnalysis->firstLineIndent = (float)atof(pValue);
#endif
		}
		else if (attrName == basedonStr) // added in version 3
		{
#ifdef _UNICODE
			gpUsfmAnalysis->basedOn = pValueW;
#else
			gpUsfmAnalysis->basedOn = pValue;
#endif
		}
		else if (attrName == nextstyleStr) // added in version 3
		{
#ifdef _UNICODE
			gpUsfmAnalysis->nextStyle = pValueW;
#else
			gpUsfmAnalysis->nextStyle = pValue;
#endif
		}
		else if (attrName == keeptogetherStr) // added in version 3
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->keepTogether = TRUE;
			else
				gpUsfmAnalysis->keepTogether = FALSE;
		}
		else if (attrName == keepwithnextStr) // added in version 3
		{
			if (strcmp(pValue, one) == 0)
				gpUsfmAnalysis->keepWithNext = TRUE;
			else
				gpUsfmAnalysis->keepWithNext = FALSE;
		}
		else
		{
			// unknown attribute, which is an error
			return false;
		}
	}
	return TRUE;
}

// the Unicode case will need a conditional compile here
// because the UTF-8 will have to be converted to UTF-16

bool AtSFMPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata),CStack*& WXUNUSED(pStack))
{
	// we don't use PCDATA in AI_USFM.xml
	return TRUE;
}

bool AtSFMEndTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	// AI_USFM.xml uses end tag xml format, whereas books.xml has empty end tag format
	if (tag == sfm)
	{
		// save the now complete gpUsfmAnalysis instance in the various maps
		// and then clear the pointer to the struct
		// Note: When AI_USFM.xml is not available the m_pUsfmStylesMap, 
		// m_pPngStylesMap, and m_pUsfmAndPngStylesMap are populated 
		// by the SetupDefaultStylesMap() function in the app, rather than here.
		wxString bareMkr = gpUsfmAnalysis->marker;
		int posn = bareMkr.Find(_T('*')); // do we have an end marker?
		if (posn >= 0 && bareMkr[bareMkr.Length() -1] == _T('*'))
		{
			bareMkr = bareMkr.Mid(0,posn-1); // strip off asterisk for attribute lookup
		}
		// Load the Usfm, Png and UsfmAndPng maps.
		// Any changes to the filling of the CMapStringToOb routines below shold also
		// be carried over to the SetupDefaultStylesMap() routine in the App.
		// We don't want duplicate entries in any of the three maps, so we'll
		// always do a loopup first, and only add a marker if it doesn't already
		// exist in the given map.
		// whm Note 11Jul05: Below we do NOT use the LookupSFM routine because Lookup here
		// is done to insure we don't get duplicate entries in the USFMAnalysis maps. 
		// Compare code below to code in the App's SetupDefaultStylesMap
		MapSfmToUSFMAnalysisStruct::iterator iter;
		if (gpUsfmAnalysis->usfm)
		{
			// Note: The wxHashMap does not have Lookup() or SetAt() methods. 
			// It works like standard template <map> using iterators and has find and 
			// insert methods.
			iter = gpApp->m_pUsfmStylesMap->find(bareMkr);
			if (iter == gpApp->m_pUsfmStylesMap->end())
			{
				// key doesn't already exist in the map, so add it
				(*gpApp->m_pUsfmStylesMap)[bareMkr] = gpUsfmAnalysis;
			}
			
			iter = gpApp->m_pUsfmAndPngStylesMap->find(bareMkr);
			if (iter == gpApp->m_pUsfmAndPngStylesMap->end())
			{
				// key doesn't already exist in the map, so add it
				(*gpApp->m_pUsfmAndPngStylesMap)[bareMkr] = gpUsfmAnalysis;
			}
			
		}
		if (gpUsfmAnalysis->png)
		{
			iter = gpApp->m_pPngStylesMap->find(bareMkr);
			if (iter == gpApp->m_pPngStylesMap->end())
			{
				// key doesn't already exist in the map, so add it
				(*gpApp->m_pPngStylesMap)[bareMkr] = gpUsfmAnalysis;
			}
			iter = gpApp->m_pUsfmAndPngStylesMap->find(bareMkr);
			if (iter == gpApp->m_pUsfmAndPngStylesMap->end())
			{
				// key doesn't already exist in the map, so add it
				(*gpApp->m_pUsfmAndPngStylesMap)[bareMkr] = gpUsfmAnalysis;
			}
		}
		if (!gpUsfmAnalysis->usfm && !gpUsfmAnalysis->png)
		{
			// the marker is neither usfm nor png and therefore not stored in
			// any of the three possible maps, so we must delete it
			delete gpUsfmAnalysis;
		}
		else
		{
			gpApp->m_pMappedObjectPointers->Add(gpUsfmAnalysis);
		}
		gpUsfmAnalysis = NULL;
	}
	return TRUE;
}

/********************************************************************************
*
*  Adapt It document input/output as XML - call back functions
*
*********************************************************************************/


bool AtDocTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	if (tag == xml_adaptitdoc) // if it's an "AdaptItDoc" tag
	{
		// initialize the two sourcephrase pointers to NULL
		gpSrcPhrase = NULL;
		gpEmbeddedSrcPhrase = NULL;
		return TRUE;
	}
	
	// the rest of them are versionable, so use a switch once we
	// have something which differs according to version; in the meantime,
	// the switch can be commented out
	if (tag == xml_settings) // if it's a "Settings" tag
	{
		// this tag only has attributes, so nothing to be done (the first attribute
		// within the Settings tag will set the docVersion number, and the rest
		// of doc input will then be versionable
		return TRUE;
	}
	else 
	{
		switch (gnDocVersion)
		{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			// no changes in AtDocTag() for VERSION_NUMBER #defined as 5
			case 5:
			{
				if (tag == xml_scap) // if it's an "S" tag
				{
					// when we encounter an S opening tag, we might be about to create an
					// unmerged sourcephrase - and if so, gpSrcPhrase will be NULL; but
					// if it is not NULL, then we are about to construct an old sourcephrase
					// which got merged, and so we need to create gpEmbeddedSrcPhrase for it
					if (gpSrcPhrase == NULL)
					{
						gpSrcPhrase = new CSourcePhrase; // the constructor creates empty m_pSavedWords
														// m_pMedialPuncts and m_pMedialMarkers list too;
														// all BOOLs are FALSE, and all CStrings empty
						// strictly speaking I should test for gpEmbeddedSrcPhrase == NULL here, but I'll assume it is
					}
					else
					{
						// we are creating an embedded sourcephrase within gpSrcPhrase
						if (gpEmbeddedSrcPhrase != NULL)
						{
							// we have a data error -- it should have been made NULL at the </S> endtag 
							// for an earlier embedded one
							return FALSE;
						}
						else
						{
							// it's NULL, so make a new one
							gpEmbeddedSrcPhrase = new CSourcePhrase;
						}
					}
					return TRUE;
				}
				else if (tag == xml_mpcap) // if it's an "MP" tag
				{
					// nothing needs to be done
					return TRUE;
				}
				else if (tag == xml_mmcap) // if it's an "MM" tag
				{
					// nothing needs to be done
					return TRUE;
				}
				else
				{
					return FALSE; // unknown element, so signal the error to the caller
				}
				break;
			} // end block for docVersion case 4: or 5:
		} // end block for switch (gnDocVersion)
	} // end else block for test: if (tag == xml_settings)
	return TRUE; // no error
}

bool AtDocEmptyElemClose(CBString& WXUNUSED(tag), CStack*& WXUNUSED(pStack))
{
	// unused
	return TRUE;
}

bool AtDocAttr(CBString& tag,CBString& attrName,CBString& attrValue, CStack*& WXUNUSED(pStack))
{
	int num;
	if (tag == xml_settings && attrName == xml_docversion)
	{
		// (the docVersion attribute is not versionable, so have it outside of the switch)
		// set the gnDocVersion global with the document's versionable serialization number
		gnDocVersion = atoi(attrValue);
		return TRUE;
	}
	switch (gnDocVersion)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		{
			if (tag == xml_settings) // it's a "Settings" tag
			{
				// none of this tag's attributes need entity replacement; first
				// attributes are numbers, and so are handled the same for the
				// regular & unicode apps; only strings require different treatment
				if (attrName == xml_sizex)
				{
					gpApp->m_docSize.x = atoi(attrValue); 
				}
				else if (attrName == xml_sizey)
				{
					gpApp->m_docSize.y = atoi(attrValue);
				}
				else if (attrName == xml_specialcolor)
				{
					num = atoi(attrValue);
					//gpApp->specialTextColor = num; // no longer used in WX version
					// The special text color stored in the xml doc file
					// The special text color stored in the doc's xml file is ignored in the wx version
					// TODO: see if this is acceptable to Bruce
					//gpApp->m_specialTextColor = (COLORREF)num;
				}
				else if (attrName == xml_retranscolor)
				{
					num = atoi(attrValue);
					//gpApp->reTranslnTextColor = num; // no longer used in WX version
					// The retranslation text color stored in the xml doc file
					// The retranslation text color stored in the doc's xml file is ignored in the wx version
					// TODO: see if this is acceptable to Bruce
					//gpApp->m_reTranslnTextColor = (COLORREF)num;
				}
				else if (attrName == xml_navcolor)
				{
					num = atoi(attrValue);
					// gpApp->navTextColor = num; // no longer used in WX version
					// The nav text color stored in the xml doc file
					// The nav text color stored in the doc's xml file is ignored in the wx version
					// TODO: see if this is acceptable to Bruce
					// gpApp->m_navTextColor = (COLORREF)num;
				}
#ifndef _UNICODE // ANSI version (ie. regular)
				else if (attrName == xml_curchap)
				{
					gpApp->m_curChapter = attrValue;
				}
				else if (attrName == xml_srcname)
				{
					// BEW modified 27Nov05; only use the doc values for restoring when doing an MRU open,
					// because otherwise the doc could have been copied from another person's project and
					// would give invalid names which would then find their way into the project config
					// file and end up fouling things up
					if (gbTryingMRUOpen)
						gpApp->m_sourceName = attrValue;
				}
				else if (attrName == xml_tgtname)
				{
					// BEW modified 27Nov05; only use the doc values for restoring when doing an MRU open,
					// because otherwise the doc could have been copied from another person's project and
					// would give invalid names which would then find their way into the project config
					// file and end up fouling things up
					if (gbTryingMRUOpen)
						gpApp->m_targetName = attrValue;
				}
				else if (attrName == xml_others)
				{
					wxString buffer(attrValue);
					gpDoc->RestoreDocParamsOnInput(buffer); // BEW added 08Aug05
					if (gpApp->m_pBuffer == NULL)
					{
						gpApp->m_pBuffer = new wxString; // ensure it's available
						gpApp->m_nInputFileLength = buffer.Length(); // we don't use this, but play safe
					}
					*(gpApp->m_pBuffer) = attrValue;
				}
				else
				{
					// unknown attribute in the Settings tag
					return FALSE;
				}
				return TRUE;
			}
			else if (tag == xml_scap) // it's an "S" tag
			{
				// all of them are versionable. 
				if (gpEmbeddedSrcPhrase != NULL)
				{
					// we are constructing an old instance embedded in a merged sourcephrase, and
					// so these attribute values belong to the gpEmbeddedSrcPhrase instance, which
					// is itself to be saved in the m_pSavedWords list of the parent sourcephrase
					// which is gpSrcPhrase

					// do the number attributes first, since these don't need entity replacement
					// and then the couple of strings which also don't need it, then the ones
					// needing it do them last
					if (attrName == xml_f)
					{
						MakeBOOLs(gpEmbeddedSrcPhrase,attrValue);
					}
					else if (attrName == xml_sn)
					{
						gpEmbeddedSrcPhrase->m_nSequNumber = atoi(attrValue);
					}
					else if (attrName == xml_w)
					{
						gpEmbeddedSrcPhrase->m_nSrcWords = atoi(attrValue);
					}
					else if (attrName == xml_ty)
					{
						gpEmbeddedSrcPhrase->m_curTextType = (TextType)atoi(attrValue);
					}
					else if (attrName == xml_i)
					{
						gpEmbeddedSrcPhrase->m_inform = attrValue;
					}
					else if (attrName == xml_c)
					{
						gpEmbeddedSrcPhrase->m_chapterVerse = attrValue;
					}
					else if (attrName == xml_em)
					{
						gpEmbeddedSrcPhrase->SetEndMarkers((char*)attrValue);
					}
					else
					{
						// The rest of the string ones may potentially contain " or > (though unlikely),
						// so ReplaceEntities() will need to be called with the bTwoOnly flag set TRUE
						ReplaceEntities(attrValue); // most require it, so do it on all
						if (attrName == xml_s)
						{
							gpEmbeddedSrcPhrase->m_srcPhrase = attrValue;
						}
						else if (attrName == xml_k)
						{
							gpEmbeddedSrcPhrase->m_key = attrValue;
						}
						else if (attrName == xml_t)
						{
							gpEmbeddedSrcPhrase->m_targetStr = attrValue;
						}
						else if (attrName == xml_a)
						{
							gpEmbeddedSrcPhrase->m_adaption = attrValue;
						}
						else if (attrName == xml_g)
						{
							gpEmbeddedSrcPhrase->m_gloss = attrValue;
						}
						else if (attrName == xml_pp)
						{
							gpEmbeddedSrcPhrase->m_precPunct = attrValue;
						}
						else if (attrName == xml_fp)
						{
							gpEmbeddedSrcPhrase->m_follPunct = attrValue;
						}
						else if (attrName == xml_m)
						{
							gpEmbeddedSrcPhrase->m_markers = attrValue;
						}
						else if (attrName == xml_ft)
						{
							gpEmbeddedSrcPhrase->SetFreeTrans((char*)attrValue);
						}
						else if (attrName == xml_no)
						{
							gpEmbeddedSrcPhrase->SetNote((char*)attrValue);
						}
						else if (attrName == xml_bt)
						{
							gpEmbeddedSrcPhrase->SetCollectedBackTrans((char*)attrValue);
						}
						else if (attrName == xml_fi)
						{
							gpEmbeddedSrcPhrase->SetFilteredInfo((char*)attrValue);
						}
						else
						{
							// unknown attribute
							return FALSE;
						}
					}
				}
				else
				{
                    // we are constructing an unmerged instance, or a parent to two or more
                    // originals involved in a merger, to be saved in the doc's
                    // m_pSourcePhrases member do the number attributes first, since these
                    // don't need entity replacement and then the couple of strings which
                    // also don't need it, then the ones needing it do them last
					if (attrName == xml_f)
					{
						MakeBOOLs(gpSrcPhrase,attrValue);
					}
					else if (attrName == xml_sn)
					{
						gpSrcPhrase->m_nSequNumber = atoi(attrValue);
					}
					else if (attrName == xml_w)
					{
						gpSrcPhrase->m_nSrcWords = atoi(attrValue);
					}
					else if (attrName == xml_ty)
					{
						gpSrcPhrase->m_curTextType = (TextType)atoi(attrValue);
					}
					else if (attrName == xml_i)
					{
						gpSrcPhrase->m_inform = attrValue;
					}
					else if (attrName == xml_c)
					{
						gpSrcPhrase->m_chapterVerse = attrValue;
					}
					else if (attrName == xml_em)
					{
						gpSrcPhrase->SetEndMarkers((char*)attrValue);
					}
					else
					{
						// The rest of the string ones may potentially contain " or > (though unlikely),
						// so ReplaceEntities() will need to be called with the bTwoOnly flag set TRUE
						ReplaceEntities(attrValue); // most require it, so do it on all
						if (attrName == xml_s)
						{
							gpSrcPhrase->m_srcPhrase = attrValue;
						}
						else if (attrName == xml_k)
						{
							gpSrcPhrase->m_key = attrValue;
						}
						else if (attrName == xml_t)
						{
							gpSrcPhrase->m_targetStr = attrValue;
						}
						else if (attrName == xml_a)
						{
							gpSrcPhrase->m_adaption = attrValue;
						}
						else if (attrName == xml_g)
						{
							gpSrcPhrase->m_gloss = attrValue;
						}
						else if (attrName == xml_pp)
						{
							gpSrcPhrase->m_precPunct = attrValue;
						}
						else if (attrName == xml_fp)
						{
							gpSrcPhrase->m_follPunct = attrValue;
						}
						else if (attrName == xml_m)
						{
							gpSrcPhrase->m_markers = attrValue;
						}
						else if (attrName == xml_ft)
						{
							gpSrcPhrase->SetFreeTrans((char*)attrValue);
						}
						else if (attrName == xml_no)
						{
							gpSrcPhrase->SetNote((char*)attrValue);
						}
						else if (attrName == xml_bt)
						{
							gpSrcPhrase->SetCollectedBackTrans((char*)attrValue);
						}
						else if (attrName == xml_fi)
						{
							gpSrcPhrase->SetFilteredInfo((char*)attrValue);
						}
						else
						{
							// unknown attribute
							return FALSE;
						}
					}
				}
				return TRUE;
			}
			else if (tag == xml_mpcap) // it's an "MP" tag
			{
				// potentially versionable
				if (attrName == xml_mp)
				{
					// its a member of a medial puncts wxString list
					ReplaceEntities(attrValue);
					if (gpEmbeddedSrcPhrase != NULL)
					{
						// impossible - unmerged ones can never have content in this list
						return FALSE;
					}
					else
					{
						gpSrcPhrase->m_pMedialPuncts->Add(wxString(attrValue));
					}
					return TRUE;
				}
				else
				{
					// unknown attribute
					return FALSE;
				}
			}
			else if (tag == xml_mmcap) // it's an "MM" tag
			{
				// potentially versionable, but should never require entity support
				if (attrName == xml_mm)
				{
					// its a member of a medial markers wxString list
					if (gpEmbeddedSrcPhrase != NULL)
					{
						// impossible - unmerged ones can never have content in this list
						return FALSE;
					}
					else
					{
						gpSrcPhrase->m_pMedialMarkers->Add(wxString(attrValue)); 
					}
					return TRUE;
				}
				else
				{
					// unknown attribute
					return FALSE;
				}
			}
			else
			{
				// it's an unknown tag
				return FALSE;
			}
			break;
		}
#else // Unicode version

				else if (attrName == xml_curchap)
				{
					gpApp->m_curChapter = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_srcname)
				{
					gpApp->m_sourceName = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_tgtname)
				{
					gpApp->m_targetName = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_others)
				{
					wxString buffer = gpApp->Convert8to16(attrValue);
					gpDoc->RestoreDocParamsOnInput(buffer); // BEW added 08Aug05
					if (gpApp->m_pBuffer == NULL)
					{
						gpApp->m_pBuffer = new wxString; // ensure it's available
						gpApp->m_nInputFileLength = buffer.Length(); // we don't use this, but play safe
					}
					*(gpApp->m_pBuffer) = buffer;
				}
				else
				{
					// unknown attribute in the Settings tag
					return FALSE;
				}
				return TRUE;
			}
			else if (tag == xml_scap) // it's an "S" tag
			{
				// all of them are versionable. 
				if (gpEmbeddedSrcPhrase != NULL)
				{
					// we are constructing an old instance embedded in a merged sourcephrase, and
					// so these attribute values belong to the gpEmbeddedSrcPhrase instance, which
					// is itself to be saved in the m_pSavedWords list of the parent sourcephrase
					// which is gpSrcPhrase

					// do the number attributes first, since these don't need entity replacement
					// and then the couple of strings which also don't need it, then the ones
					// needing it do them last
					if (attrName == xml_f)
					{
						MakeBOOLs(gpEmbeddedSrcPhrase,attrValue);
					}
					else if (attrName == xml_sn)
					{
						gpEmbeddedSrcPhrase->m_nSequNumber = atoi(attrValue);
					}
					else if (attrName == xml_w)
					{
						gpEmbeddedSrcPhrase->m_nSrcWords = atoi(attrValue);
					}
					else if (attrName == xml_ty)
					{
						gpEmbeddedSrcPhrase->m_curTextType = (TextType)atoi(attrValue);
					}
					else if (attrName == xml_i)
					{
						gpEmbeddedSrcPhrase->m_inform = gpApp->Convert8to16(attrValue);
					}
					else if (attrName == xml_c)
					{
						gpEmbeddedSrcPhrase->m_chapterVerse = gpApp->Convert8to16(attrValue);
					}
					else if (attrName == xml_em)
					{
						gpEmbeddedSrcPhrase->SetEndMarkers(gpApp->Convert8to16(attrValue));
					}
					else
					{
						// The rest of the string ones may potentially contain " or > (though unlikely),
						// so ReplaceEntities() will need to be called with the bTwoOnly flag set TRUE
						ReplaceEntities(attrValue); // most require it, so do it on all
						if (attrName == xml_s)
						{
							gpEmbeddedSrcPhrase->m_srcPhrase = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_k)
						{
							gpEmbeddedSrcPhrase->m_key = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_t)
						{
							gpEmbeddedSrcPhrase->m_targetStr = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_a)
						{
							gpEmbeddedSrcPhrase->m_adaption = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_g)
						{
							gpEmbeddedSrcPhrase->m_gloss = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_pp)
						{
							gpEmbeddedSrcPhrase->m_precPunct = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_fp)
						{
							gpEmbeddedSrcPhrase->m_follPunct = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_m)
						{
							gpEmbeddedSrcPhrase->m_markers = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_ft)
						{
							gpEmbeddedSrcPhrase->SetFreeTrans(gpApp->Convert8to16(attrValue));
						}
						else if (attrName == xml_no)
						{
							gpEmbeddedSrcPhrase->SetNote(gpApp->Convert8to16(attrValue));
						}
						else if (attrName == xml_bt)
						{
							gpEmbeddedSrcPhrase->SetCollectedBackTrans(gpApp->Convert8to16(attrValue));
						}
						else if (attrName == xml_fi)
						{
							gpEmbeddedSrcPhrase->SetFilteredInfo(gpApp->Convert8to16(attrValue));
						}
						else
						{
							// unknown attribute
							return FALSE;
						}
					}
				}
				else
				{
                    // we are constructing an unmerged instance, or a parent to two or more
                    // originals involved in a merger, to be saved in the doc's
                    // m_pSourcePhrases member do the number attributes first, since these
                    // don't need entity replacement and then the couple of strings which
                    // also don't need it, then the ones needing it do them last
					if (attrName == xml_f)
					{
						MakeBOOLs(gpSrcPhrase,attrValue);
					}
					else if (attrName == xml_sn)
					{
						gpSrcPhrase->m_nSequNumber = atoi(attrValue);
					}
					else if (attrName == xml_w)
					{
						gpSrcPhrase->m_nSrcWords = atoi(attrValue);
					}
					else if (attrName == xml_ty)
					{
						gpSrcPhrase->m_curTextType = (TextType)atoi(attrValue);
					}
					else if (attrName == xml_i)
					{
						gpSrcPhrase->m_inform = gpApp->Convert8to16(attrValue);
					}
					else if (attrName == xml_c)
					{
						gpSrcPhrase->m_chapterVerse = gpApp->Convert8to16(attrValue);
					}
					else if (attrName == xml_em)
					{
						gpSrcPhrase->SetEndMarkers(gpApp->Convert8to16(attrValue));
					}
					else
					{
						// The rest of the string ones may potentially contain " or > (though unlikely),
						// so ReplaceEntities() will need to be called with the bTwoOnly flag set TRUE
						ReplaceEntities(attrValue); // most require it, so do it on all
						if (attrName == xml_s)
						{
							gpSrcPhrase->m_srcPhrase = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_k)
						{
							gpSrcPhrase->m_key = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_t)
						{
							gpSrcPhrase->m_targetStr = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_a)
						{
							gpSrcPhrase->m_adaption = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_g)
						{
							gpSrcPhrase->m_gloss = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_pp)
						{
							gpSrcPhrase->m_precPunct = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_fp)
						{
							gpSrcPhrase->m_follPunct = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_m)
						{
							gpSrcPhrase->m_markers = gpApp->Convert8to16(attrValue);
						}
						else if (attrName == xml_ft)
						{
							gpSrcPhrase->SetFreeTrans(gpApp->Convert8to16(attrValue));
						}
						else if (attrName == xml_no)
						{
							gpSrcPhrase->SetNote(gpApp->Convert8to16(attrValue));
						}
						else if (attrName == xml_bt)
						{
							gpSrcPhrase->SetCollectedBackTrans(gpApp->Convert8to16(attrValue));
						}
						else if (attrName == xml_fi)
						{
							gpSrcPhrase->SetFilteredInfo(gpApp->Convert8to16(attrValue));
						}
						else
						{
							// unknown attribute
							return FALSE;
						}
					}
				}
				return TRUE;
			}
			else if (tag == xml_mpcap) // it's an "MP" tag
			{
				// potentially versionable
				if (attrName == xml_mp)
				{
					// its a member of a medial puncts wxString list
					ReplaceEntities(attrValue);
					if (gpEmbeddedSrcPhrase != NULL)
					{
						// impossible - unmerged ones can never have content in this list
						return FALSE;
					}
					else
					{
						gpSrcPhrase->m_pMedialPuncts->Add(gpApp->Convert8to16(attrValue));
					}
					return TRUE;
				}
				else
				{
					// unknown attribute
					return FALSE;
				}
			}
			else if (tag == xml_mmcap) // it's an "MM" tag
			{
				// potentially versionable
				if (attrName == xml_mm)
				{
					// its a member of a medial markers wxString list
					if (gpEmbeddedSrcPhrase != NULL)
					{
						// impossible - unmerged ones can never have content in this list
						return FALSE;
					}
					else
					{
						gpSrcPhrase->m_pMedialMarkers->Add(gpApp->Convert8to16(attrValue));
					}
					return TRUE;
				}
				else
				{
					// unknown attribute
					return FALSE;
				}
			}
			else
			{
				// it's an unknown tag
				return FALSE;
			}
			break;
		}
#endif
	}
	return TRUE; // no error
}

bool AtDocEndTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	switch (gnDocVersion) 
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		// case 5: for gnDocVersion = 4 requires, if we are to read version 4 documents
		// and convert them to version 5, a conversion function which is to be called on
		// each of gpEmbeddedSrcPhrase and gpSrcPhrase before they are inserted in the
		// list; the difference is that the contents of m_markers in version 4 is split
		// between 4 members in version 5, m_endMarkers, m_freeTrans, m_note,
		// m_collectedBackTrans, and m_filteredInfo
		case 5:
		{
			// the only one we are interested in is the "</S>" endtag, so we can
			// determine whether to save to a parent sourcephrase's m_pSavedWords list, 
			// or save it as a parent sourcephrase in the doc's m_pSourcePhrases list;
			// and of course the </AdaptItDoc> endtag
			if (tag == xml_scap)
			{
				if (gpEmbeddedSrcPhrase != NULL)
				{
					// we have just come to the end of one of the original sourcephrase instances
					// to be stored in the m_pSavedWords member of a merged sourcephrase which is
					// pointed at by gpSrcPhrase, so add it to the list & then clear the pointer
					wxASSERT(gpSrcPhrase);
					if (gnDocVersion == 4)
					{
						FromDocVersion4ToDocVersion5(gpSrcPhrase->m_pSavedWords, gpEmbeddedSrcPhrase, TRUE);
					}
					gpSrcPhrase->m_pSavedWords->Append(gpEmbeddedSrcPhrase);
					gpEmbeddedSrcPhrase = NULL;
				}
				else 
				{
					// gpEmbeddedSrcPhrase is NULL, so we've been constructing an unmerged one,
					// so now we can add it to the doc member m_pSourcePhrases and clear the pointer
					if (gbSyncMsgReceived_DocScanInProgress)
					{
						if (gnDocVersion == 4)
						{
							FromDocVersion4ToDocVersion5(gpDocList, gpSrcPhrase, FALSE);
						}
						gpDocList->Append(gpSrcPhrase);
					}
					else
					{
						if (gnDocVersion == 4)
						{
							FromDocVersion4ToDocVersion5(gpApp->m_pSourcePhrases, gpSrcPhrase, FALSE);
						}
						gpApp->m_pSourcePhrases->Append(gpSrcPhrase);
					}
					gpSrcPhrase = NULL;
				}

			}
			else if (tag == xml_adaptitdoc)
			{
				// we are done
				return TRUE;
			}
			break;
		}
	}
	return TRUE;
}

bool AtDocPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata), CStack*& WXUNUSED(pStack))
{
	// we don't have any PCDATA in the document XML files
	return TRUE;
}

void FromDocVersion4ToDocVersion5( SPList* pList, CSourcePhrase* pSrcPhrase, bool bIsEmbedded)
{
	if (pSrcPhrase->m_markers.IsEmpty())
		return; // no conversions needed for this one

	// If the pList list is empty on entry, then the GetLast() call will return NULL
	// rather than a valid pos value
	wxString strModifiers = pSrcPhrase->m_markers;
	CSourcePhrase* pLastSrcPhrase = NULL;
	SPList::Node* pos = pList->GetLast();

    // The list, on entry, is either the m_pSavedWords SPList* member in CSourcePhrase, or
    // the m_pSourcePhrases list in the document CAdapt_ItDoc class
    // 
	// Note 1: in a merger, the endmarkers, if any, are on the second or later
	// CSourcePhrase instances of a merger, never the first, so the stuff below won't
	// break due to the fact that we don't try to access m_endMarkers member on the
	// first CSourcePhrase
	
    // Note 2: When the list is m_pSourcePhrases member of the CAdapt_ItDoc class...
    // docVersion = 4 would not have any endmarkers in the very first CSourcePhrase
    // instance in m_pSourcePhrases list, and so we can be sure that not trying to convert
    // to endmarkers on the first instance will not cause loss of data So this block not
    // only has to handle moving of the endmarkers from the current CSourcePhrase instance
    // to a preceding non-merged one's m_endMarkers member, but also when the preceding one
    // is a merger, it must do that and then in addition look at the m_pSavedWords member
    // and find the last of the original instances that comprised the merger, and add the
    // endmarkers to that one's m_endMarkers member as well - any earlier ones in the
    // m_pSavedWords list will have been handled by an earlier call. This extra transfer is
    // handled below by a block which tests for bIsEmbedded == FALSE
		
	bool bSomethingTransferred = FALSE;
	if (pos != NULL)
	{
		// there might be endmarkers, so deal with them; endmarkers, if they occur,
		// will always be at the start of the pSrcPhrase->m_markers member in version 4,
		// and since we might be using a legacy SFM set, we can't assume an endmarker
		// will end with an asterisk
		pLastSrcPhrase = pos->GetData();
		bSomethingTransferred = TransferEndMarkers(strModifiers, pLastSrcPhrase); // parent
		if (bSomethingTransferred)
		{
			// since one or more endMarkers were transferred, strModifiers has been
			// shortened by these having been removed from its start, and any initial
			// whitespace trimmed off, so update the original storage CSourcePhrase
			// instance to have the shorter string
			pSrcPhrase->m_markers = strModifiers;
		}
		if (!bIsEmbedded)
		{
			// the list is not the m_pSavedWords SPList* member in CSourcePhrase
			// Note: in a merger, the endmarkers, if any, are on the second or later
			// CSourcePhrase instances of a merger, never the first, so the stuff below won't
			// break due to the fact that we don't try to access m_endMarkers member on the
			// first CSourcePhrase

			wxString strEndmarkersTransferred = pLastSrcPhrase->GetEndMarkers(); // use below
			// if we did it at the parent level, then do it on last of the sublist's instances
			if (!pLastSrcPhrase->m_pSavedWords->IsEmpty() && bSomethingTransferred)
			{
				// this CSourcePhrase instance is a merger, so also transfer to its last
				// saved instance in the m_pSavedWords list
				SPList::Node* pos = pLastSrcPhrase->m_pSavedWords->GetLast();
				wxASSERT(pos != NULL);
				CSourcePhrase* pLastInSublist = pos->GetData();
				wxASSERT(pLastInSublist != NULL);	
				pLastInSublist->SetEndMarkers(strEndmarkersTransferred);
			}
		} // end TRUE block for text: if (!bIsEmbedded)
	}
	// If pList is not empty, then the current CSourcePhrase instance that is passed
	// in is at least the second or later instance in the doc's list. Now that
	// endmarkers have been bled out of the strModifiers string, deal with the rest of
	// the transfers for the new storage strings
	strModifiers.Trim(FALSE); // a precautionary trim whitespace from left
	if (!strModifiers.IsEmpty())
	{
		// there is content to be dealt with still - there could be filtered material,
		// and or other markers (such as chapter, verse, subheading, poetry, etc) to
		// bleed out any filtered material (each info type of any filtered stuff is 
		// always wrapped, in version 4 docs, with \~FILTER and \~FILTER* wrappers;
		// and whatever follows such info then is left in the m_markers member), but
		// deal separately with free trans, notes, collected back trans
		int offset = strModifiers.Find(filterMkr); // look for \~FILTER (if present
									// it will now be at the start of strModifiers)

		if (offset == wxNOT_FOUND)
		{
			// there is no filtered info needing to be dealt with, so the rest
			// remains in m_markers
			pSrcPhrase->m_markers = strModifiers;
		}
		else
		{
            // extract all the wrapped filtered info into its own string, deal
            // separately with free translations, notes, and collected back
            // translations, and the rest belongs in m_markers
            // (ExtractWrappedFilteredInfor() is defined in helpers.cpp)
			wxString strRemainder;
			wxString strFreeTrans;
			wxString strNote;
			wxString strCollectedBackTrans;
			// the next call strips of \~FILTER and \~FILTER* and any marker and endmarker
			// wrapped by these wrapper markers, if returning data via strFreeTrans, strNote,
			// and/or strCollectedBackTrans; but the normal return string which goes to
			// filteredInfo will have neither the filter marker wrappers, nor the marker
			// and endmarker (if present)wrapped by them, removed - because Adapt It makes
			// no use of the m_filteredInfo content in version 5, that member is just
			// there as a catch all for all filtered stuff needing to be kept in case the
			// use calls for an export - in which case it needs to be put into the export
			// at the appropriate places, but until then we just squirrel it away and
			// forget about it
			wxString filteredInfo = ExtractWrappedFilteredInfo(strModifiers, strFreeTrans,
				strNote, strCollectedBackTrans, strRemainder);
			if (!strFreeTrans.IsEmpty())
			{
				// transfer the unwrapped content (with \free and \free* markers removed)
				// to the m_freeTrans member
				pSrcPhrase->SetFreeTrans(strFreeTrans);
			}
			if (!strNote.IsEmpty())
			{
				// transfer the unwrapped content (with \note and \note* markers removed)
				// to the m_note member
				pSrcPhrase->SetNote(strNote);
				pSrcPhrase->m_bHasNote = TRUE; // make sure it's set
			}
			if (!strCollectedBackTrans.IsEmpty())
			{
				// transfer the unwrapped content (with \bt, or any \bt-initial marker, removed)
				// to the m_collectedBackTrans member
				pSrcPhrase->SetCollectedBackTrans(strCollectedBackTrans);
			}
			// transfer filteredInfo returned string to m_filteredInfo member (& it may
			// be an empty string)
			pSrcPhrase->SetFilteredInfo(filteredInfo);

			// update m_markers to have whatever remains of strModifiers (it could be nothing)
			pSrcPhrase->m_markers = strRemainder;
		}
	}
}

// convert from doc version 5's various filtered content storage members, back to the
// legacy doc version 4 storage regime, where filtered info and endmarkers (for
// non-filtered info) were all stored on m_markers. This function must only be called on a
// deep copies of the CSourcePhrase instances within the document's m_pSourcePhrases list,
// because this function will modify the content in each deep copied instance in order that
// the old legacy doc version 4 xml construction code will correctly build the legacy
// document xml format, without corrupting the original doc version 5 storage regime.
// 
// pSrcPhrase is either a deep copy of a CSourcePhrase instance from the m_pSourcePhrases
// list which comprises the document. Unlike the function which converts 4 to 5, this
// function must handle conversions of instances in a non-empty m_pSavedWords list
// internally. It must input any endmarkers from the last call, in order to place them in
// the current call -- this is done this way: for non-merger, insert at the start of
// m_markers; for a merger, insert at the start of m_markers, and then insert at the start
// of m_markers of the first CSourcePhrase instance in m_pSavedWords. Also, if the earlier
// call was a merger with endmarkers - the endmarkers in the last CSourcePhrase instance
// of that earlier merged CSourcePhrase's m_pSavedWords list can be ignored, because they
// will have been copied to the merger which stores that list anyway, and will have been
// returned to the caller from that merger instance.
void FromDocVersion5ToDocVersion4(CSourcePhrase* pSrcPhrase, wxString* pEndMarkersStr)
{
	wxString aSpace = _T(" ");
	wxString storedEndMarkers;
	wxString newMarkersMember = RewrapFilteredInfoForDocV4(pSrcPhrase, storedEndMarkers);
	// if storedEndMarkers is non-empty, it will eventually have to be returned in
	// pEndMarkersStr, but we don't reset the latter until we use any content that may
	// have been passed in by it (such content would result from what was returned in
	// pEndMarkersStr at the previous iteration of the caller's loop)

	// update m_markers member; ensure it ends with just a single space if it has
	// non-space content
	newMarkersMember.Trim();
	if (!newMarkersMember.IsEmpty())
	{
		newMarkersMember += aSpace;
	}
	pSrcPhrase->m_markers = newMarkersMember;
	// insert any endmarkers passed in, at its beginning
	if (!pEndMarkersStr->IsEmpty())
	{
		// we won't bother with a delimiting space between the last endmarker and the
		// start of the m_markers material, because it is unnecessary (but we did have one
		// in the legacy versions)
		pSrcPhrase->m_markers = *pEndMarkersStr + pSrcPhrase->m_markers;
	}
	// check if this pSrcPhrase is a merger - if it is, we have to also convert the
	// CSourcePhrase instances in the m_pSavedWords list back to docVersion 4 as well, but
	// we don't need to get any endmarkers on the last in that list, as the merger process
	// will have copied them to the owning merged CSourcPhrase instance, and we'll have
	// already gotten them into storedEndMarkers in the code above
	wxString storedEndMarkers_Originals;
	wxArrayString endmarkersArray;
	if (pSrcPhrase->m_nSrcWords > 1)
	{
		// it's a merger, so process the saved originals
		int counter = 0;
		SPList::Node* pos = pSrcPhrase->m_pSavedWords->GetFirst();
		wxASSERT(pos != NULL);
		bool bFirst = TRUE; // use this to get the passed in endmarkers, if any, inserted
							// only at the start of the modified m_markers member of the
							// first CSourcePhrase instance in this list
		while (pos != NULL)
		{
			CSourcePhrase* pOriginalSPh = pos->GetData();
			newMarkersMember = RewrapFilteredInfoForDocV4(pOriginalSPh, storedEndMarkers_Originals);
            // add any endmarkers to the array, and for non-first iteration, check if
            // endmarkers were stored here on the last iteration, and if so, insert them at
            // start of current pOriginalSPh's m_markers string
			endmarkersArray.Add(storedEndMarkers_Originals); // store whatever it is, even empty string
			counter++;
			// update m_markers on the current instance... if nonempty, it should always end with a
			// space, so check and fix if necessary (easiest way is to unilaterally Trim()
			// at the end and add a single space
			newMarkersMember.Trim();
			if (!newMarkersMember.IsEmpty())
			{
				newMarkersMember += aSpace;
			}
			pOriginalSPh->m_markers = newMarkersMember;
			
			if (bFirst)
			{
				// do any needed endmarkers insertion on the first, using any passed in
				if (!pEndMarkersStr->IsEmpty())
				{
					pOriginalSPh->m_markers = *pEndMarkersStr + pOriginalSPh->m_markers;
				}
			}
			// handle transfer of any medial endmarkers found, transferring to the 'next'
			// instance's m_markers member's start; (if one is one the last CSourcePhrase
			// instance, then it won't get transferred because the loop will exit first,
			// but that is fine because we already have such endmarkers content stored in
			// storedEndMarkers above, and the next iteration in the caller's loop will
			// place that endmarkers information correctly)
			if (!bFirst)
			{
				wxString endmarkersStored = endmarkersArray.Item(counter - 2); // -1 gives 
										// an index, a further -1 gives the previous item
				if (!endmarkersStored.IsEmpty())
				{
					// previous iteration found and stored endmarkers, so place them now
					pOriginalSPh->m_markers = endmarkersStored + pOriginalSPh->m_markers;
				}
			}
			bFirst = FALSE; // prevent reentry to the if (bFirst) == TRUE block (do it here
							// because we want bFirst to remain TRUE on the first iteration
							// so that the if (!bFirst) == TRUE block is skipped on the
							// first iteration
			pos = pos->GetNext();
		} // end of the while (pos != NULL) loop
	} // end of TRUE block for test: if (pSrcPhrase->m_nSrcWords > 1)

	// the final thing to do is to return any endmarkers required by the caller
	// (or the empty string if there are none) -- add a space at the end, it's not really
	// needed, but it gives a doc version 4 more like the legacy ones
	if (!storedEndMarkers.IsEmpty())
	{
		storedEndMarkers += aSpace;
	}
	*pEndMarkersStr = storedEndMarkers;
}


// return a docversion 4 m_markers wxString with the docversion 5 filter storage members'
// contents rewrapped with \~FILTER and \FILTER* bracketing markers, but leave addition of
// any endmarkers to be done by its caller, just return any stored endmarkers in the
// second parameter letting the caller decide what to do with them. In Doc version 4 the
// order of information in m_markers is:
// endmarker filteredinfo collectedbacktranslation note freetranslation SF markers (and
// verse or chapter number as appropriate)
wxString RewrapFilteredInfoForDocV4(CSourcePhrase* pSrcPhrase, wxString& endmarkers)
{
	wxASSERT(endmarkers.IsEmpty());
	wxString str; // accumulate the return string here
	// markers needed, since doc version 5 doesn't store some filtered 
	// stuff using them
	wxString freeMkr(_T("\\free"));
	wxString freeEndMkr = freeMkr + _T("*");
	wxString noteMkr(_T("\\note"));
	wxString noteEndMkr = noteMkr + _T("*");
	wxString backTransMkr(_T("\\bt"));
	wxString aSpace = _T(" ");
	wxString filterStr(filterMkr);  // builds "\~FILTER"
	wxString filterEndStr(filterMkrEnd);  // builds "\~FILTER*"

	// scratch strings, in wxWidgets these local string objects start off empty
	wxString markersStr; 
	wxString endMarkersStr;
	wxString freeTransStr;
	wxString noteStr;
	wxString collBackTransStr;
	wxString filteredInfoStr;
	wxString subStr; // a scratch string for building each "\~FILTER \mkr content \mkr* \~FILTER*" string

	// get the other string information we want, putting it in the 
	// scratch strings
	GetMarkersAndFilteredStrings(pSrcPhrase, markersStr, endMarkersStr,
					freeTransStr, noteStr, collBackTransStr, filteredInfoStr);
	endmarkers = endMarkersStr;

	// first comes any content from m_filteredInfo member (it might be empty)
	str = filteredInfoStr; // this is already wrapped by filterMkr and filterMkrEnd pairs

	// next comes any collected backtranslation
	if (!collBackTransStr.IsEmpty())
	{
		subStr = filterStr + aSpace;
		subStr += backTransMkr + aSpace;
		subStr += collBackTransStr + aSpace;
		subStr += filterEndStr;
		// add a space initially unilaterally, later use Trim(FALSE) to remove any initial
		// one 
		str += aSpace + subStr;
	}

	// next comes a note, if there is one on this instance
	//if (!noteStr.IsEmpty() || pSrcPhrase->m_bHasNote) // uncomment out and comment
	//out the next line, if at a later time we support empty Adapt It Notes
	if (!noteStr.IsEmpty())
	{
		subStr = filterStr + aSpace;
		subStr += noteMkr + aSpace;
		if (!noteStr.IsEmpty())
		{
			subStr += noteStr + aSpace;
		}
		subStr += noteEndMkr + aSpace;
		subStr += filterEndStr;
		// add a space initially unilaterally, later use Trim(FALSE) to remove any initial
		// one 
		str += aSpace + subStr;
	}

	// next comes a free translation, if there is one stored on this instance
	if (!freeTransStr.IsEmpty() || pSrcPhrase->m_bStartFreeTrans)
	{
		// the m_bStartFreeTrans test is in order to support empty free translation
		// sections - in such a case, the content added is nil, but the marker and
		// endmarker are present, and wrapped by filter bracket markers as per normal
		subStr = filterStr + aSpace;
		subStr += freeMkr + aSpace;
		if (!freeTransStr.IsEmpty())
		{
			subStr += freeTransStr + aSpace;
		}
		subStr += freeEndMkr + aSpace;
		subStr += filterEndStr;
		// add a space initially unilaterally, later use Trim(FALSE) to remove any initial
		// one 
		str += aSpace + subStr;
	}

	// next comes any SF markers info removed from the user's sight but not filtered, such
	// stuff is things like "\c 3 \p \v 1 " or "\q1 " and so forth
	if (!markersStr.IsEmpty())
	{
		str += aSpace + markersStr;
	}
	str.Trim(FALSE); // finally, remove any string-initial whitespace
	// ensure it ends with a space, if it has content
	if (!str.IsEmpty())
	{
		str.Trim();
		str += aSpace;
	}
	return str;
}

// returns TRUE if one or more endmarkers was transferred, FALSE if none were transferred
// Used in the conversion of documents that were saved in docVersion 4, to docVersion 5.
// The latter has an m_endMarkers member on CSourcePhrase, and endmarkers are then no
// longer stored on the start of the m_markers member of the CSourcePhrase which follows
// the one which is the end of the content for the endmarker in question. (Note: allowing
// the user to do Save As... so as to save to a legacy doc version, such as 4, requires a
// similar function to transfer endmarkers from the end of a CSourcePhrase to the start of
// the m_markers member of the one which follows. Call that one TransferEndMarkersBackToDocV4()
bool TransferEndMarkers(wxString& markers, CSourcePhrase* pLastSrcPhrase)
{
	bool bTransferred = FALSE;
	markers.Trim(FALSE); // trim at left, but should never be necessary
	int length;									// the wxString's data buffer
	bool bWasEndMarker = FALSE;
	do {
		const wxChar* ptr = markers.GetData(); // point at first possible marker in markers
		length = ParseMarker(ptr);
		wxString marker(ptr,length);
		if (!marker.IsEmpty() && marker.GetChar(0) == _T('\\'))
		{
			// it's a marker of some type (but may not be an endmarker)
			if (gpApp->gCurrentSfmSet == PngOnly && (marker == _T("\\fe") || marker == _T("\\F")))
			{
				// it's the 1998 PNG marker set's endmarker for footnote, either \fe
				// or \F (there are never two of these in succession)
				bWasEndMarker = TRUE;
			}
			else if (marker.Find(_T('*')) != wxNOT_FOUND)
			{
				// it's a USFM endmarker (there can be two or more of these in succession)
				bWasEndMarker = TRUE;
			}
			else
			{
				bWasEndMarker = FALSE;
			}
			// test whether we have an endmarker or not
			if (!bWasEndMarker)
			{
				// it was not an endmarker - so no more endmarkers can follow so
				// break out of loop
				break;
			}
			else
			{
				// transfer the endmarker to m_endMarkers and prepare for next iteration,
				// and then iterate the loop
				if (gpApp->gCurrentSfmSet == PngOnly)
				{
					// these need delimiting spaces (but unlikely to be two, so the else
					// block is almost certainly never going to be entered -- but two
					// consecutive footnotes in a legacy PngOnly SFM set could cause it to
					// happen)
					wxString currentEndMkrs = pLastSrcPhrase->GetEndMarkers();
					if (currentEndMkrs.IsEmpty())
						currentEndMkrs = marker;
					else
						currentEndMkrs += _T(" ") + marker;
					pLastSrcPhrase->SetEndMarkers(currentEndMkrs);
					bTransferred = TRUE;
				}
				else
				{
					// for USFM ones, we don't need delimiting spaces between them
					pLastSrcPhrase->AddEndMarker(marker);
					bTransferred = TRUE;
				}
				markers = markers.Mid(length);
				markers.Trim(FALSE); // ready to test for another
			}
		} // end of block for test: if (!marker.IsEmpty() && marker.GetChar(0) == _T('\\'))
		else
		{
			// isn't a marker of any kind, so break out of loop
			break;
		}
	} while (TRUE);
	return bTransferred;
}

// returns TRUE if endmarkers were transferred to the start of the pNextSrcPhrase, FALSE
// if no transfer was done. DocVersion 4 stored endmarkers for non-filtered information at
// the start of the m_markers member of the next CSourcePhrase instance, rather than in
// the CSourcePhrase instance which is where the information type ends. So this function
// checks for endmarker(s) in the m_endMarkers member, and if present, inserts them at the
// start of pNextSrcPhrase->m_markers. In the case of the current source phrase being a
// merger, then m_endMarkers will have the same content for the merged CSourcePhrase, and
// also for the last one in its m_pSavedWords array of original CSourcePhrase instances;
// in that case, not only do we do the transfer, but the last one in m_pSavedWords has to
// be cleared to the empty string, because what we transfer to pNextSrcPhrase->m_markers
// suffices for both. 
// Note: if the pThisOne instance is the last in the document, then the caller would have
// to create a dummy CSourcPhrase instance and append to the doc's end, in order to carry
// the transferred endmarkers (and pNextSrcPhrase would then be that dummy one)
bool TransferEndMarkersBackToDocV4(CSourcePhrase* pThisOne, CSourcePhrase* pNextSrcPhrase)
{
	wxASSERT(pNextSrcPhrase != NULL);
	if (pThisOne->GetEndMarkers().IsEmpty())
	{
		// there is nothing to transfer
		return FALSE;
	}
	wxString emptyStr = _T("");
	wxString nextMarkers = pNextSrcPhrase->m_markers;
	wxString endmarkersStr = pThisOne->GetEndMarkers();
	if (pThisOne->m_nSrcWords > 1)
	{
		// its a merger, so we've a bit more to do
		SPList* pList = pThisOne->m_pSavedWords;
		SPList::Node* pos = pList->GetLast();
		wxASSERT(pos != NULL);
		CSourcePhrase* pOriginalLast = pos->GetData();
		pOriginalLast->SetEndMarkers(emptyStr);
	}
	// make the transfer
	if (nextMarkers.IsEmpty())
	{
		nextMarkers = endmarkersStr;
	}
	else
	{
		nextMarkers = endmarkersStr + _T(" ") + nextMarkers;
	}
	// update the m_markers member with the transferred data
	pNextSrcPhrase->m_markers = nextMarkers; 
	pThisOne->SetEndMarkers(emptyStr);
	return TRUE;
}

// this function takes an input string of the following form (where [ ] brackets
// indicate optionality)
// \~FILTER \somemkr some content [\somemkr*] \~FILTER*[\~FILTER \mrk2 content... etc]
// and it strips off the first \~FILTER and matching \~FILTER* endmarker, returning the
// string "\somemkr some content [\somemkr*\]
// If the PNG 1998 marker set is being used, the returned endmarker (if it exists) would
// not have any asterisk, as there are few endmarkers in that set and none use *.
wxString RemoveOuterWrappers(wxString wrappedStr)
{
	int offset = wrappedStr.Find(filterMkr);
	wxASSERT(offset != wxNOT_FOUND);
	offset += filterMkrLen;
	wrappedStr = wrappedStr.Mid(offset);
	wrappedStr.Trim(FALSE); // trim whitespace on left
	offset = wrappedStr.Find(filterMkrEnd);
	wxASSERT(offset != wxNOT_FOUND);
	wrappedStr = wrappedStr.Left(offset);
	wrappedStr.Trim(); // trim whitespace on right
	return wrappedStr;
}

// Take a mkrsAndContent string of form:
// \mkr some data content \mkr* (USFM, and \mkr* may be absent), or for the PNG SFM marker set,
// \f footnote material \fe (or possibly: \F instead of \fe)
// and return the separate bits by means of the formal parameters
// This function is used in XML.cpp and CSourcePhrase.cpp, once for each.
void ParseMarkersAndContent(wxString& mkrsAndContent, wxString& mkr, wxString& content, wxString& endMkr)
{
	wxString str = mkrsAndContent;
	const wxChar* ptr = str.GetData(); // the wxString's data buffer
	int length;

	// extract the starting marker, and then shorten str so that it starts at the content
	// part of mkrsAndContent
	length = ParseMarker(ptr);
	wxString marker(ptr,length);
	mkr = marker;
	str = str.Mid(length); // chop of the initial marker
	str.Trim(FALSE); // chop off initial white space 
	
	// reverse the string
	wxString rev = MakeReverse(str);

	//  extract the reversed endmarker, if one exists (code defensively, in case the
	//  content string contains an embedded gFSescapechar)
	int offset = rev.Find(gSFescapechar);
	if (offset == wxNOT_FOUND)
	{
		// there is no endmarker
		wxString endMarker = _T("");
		endMkr = endMarker;
		rev.Trim(FALSE); // trim any initial white space
		str = MakeReverse(rev); // restore normal order, the result is the 'content' string
		content = str;
		return;
	}
	else
	{
		// there is an endmarker - what lies before it is the reversed characters of that
		// endmarker 
		const wxChar* ptr2 = rev.GetData(); // the gFSescapechar will cause premature exit of ParseMarker()
							 // so allow for this below
		length = ParseMarker(ptr2);
		length++; // count the back slash (i.e. gFSescapechar)
		wxString reversedEndMkr(rev,length);
		endMkr = MakeReverse(reversedEndMkr);
		rev = rev.Mid(length);
		rev.Trim(FALSE); // trim any now-initial whitespace
		content = MakeReverse(rev);
	}
}

/**************************************************************************************
*   MakeBOOLs
*
*  Returns: nothing
*  Parameters:
*  pSP		->	ref to a pointer to a CSourcePhrase instance which is to have some bool members set
*  digits	->	ref to a 22 byte byte-string of 0s and 1s, which is interpretted as a bitfield
*  Remarks:
*  Used to convert the 22 byte string of 0s and 1s (currently 22, at Aug 2005) into an integer
*  and then use the masking constants to reconstuct any TRUE bool values, and set the appropriate
*  bool members in pSP. If we subsequently add extra BOOLs to CSourcePhrase, we only
*  need to add the code lines for the extra ones in the function and we'll be able to read legacy 
*  Adapt It documents in XML format without having to bump a version number.
***************************************************************************************/
void MakeBOOLs(CSourcePhrase*& pSP, CBString& digits)
{
	CBString flagsStr;
	UInt32 n = Btoi(digits); // defined in helpers.h
	wxASSERT(n >= 0);
	if (n & hasKBEntryMask)
		pSP->m_bHasKBEntry = TRUE;
	if (n & notInKBMask)
		pSP->m_bNotInKB = TRUE;
	if (n & hasGlossingKBEntryMask)
		pSP->m_bHasGlossingKBEntry = TRUE;
	if (n & specialTextMask)
		pSP->m_bSpecialText = TRUE;
	if (n & firstOfTypeMask)
		pSP->m_bFirstOfType = TRUE;
	if (n & boundaryMask)
		pSP->m_bBoundary = TRUE;
	if (n & nullSourcePhraseMask)
		pSP->m_bNullSourcePhrase = TRUE;
	if (n & retranslationMask)
		pSP->m_bRetranslation = TRUE;
	if (n & beginRetranslationMask)
		pSP->m_bBeginRetranslation = TRUE;
	if (n & endRetranslationMask)
		pSP->m_bEndRetranslation = TRUE;
	if (n & hasFreeTransMask)
		pSP->m_bHasFreeTrans = TRUE;
	if (n & startFreeTransMask)
		pSP->m_bStartFreeTrans = TRUE;
	if (n & endFreeTransMask)
		pSP->m_bEndFreeTrans = TRUE;
	if (n & hasNoteMask)
		pSP->m_bHasNote = TRUE;
	if (n & hasBookmarkMask)
		pSP->m_bHasBookmark = TRUE;
	if (n & chapterMask)
		pSP->m_bChapter = TRUE;
	if (n & verseMask)
		pSP->m_bVerse = TRUE;
	if (n & hasInternalMarkersMask)
		pSP->m_bHasInternalMarkers = TRUE;
	if (n & hasInternalPunctMask)
		pSP->m_bHasInternalPunct = TRUE;
	if (n & footnoteMask)
		pSP->m_bFootnote = TRUE;
	if (n & footnoteEndMask)
		pSP->m_bFootnoteEnd = TRUE;
	if (n & paragraphMask)
		pSP->m_bParagraph = TRUE;
}

/**************************************************************************************
*   MakeFlags
*
*  Returns: a CBString (string of single-byte characters) containing 22 0s and 1s digits
*  Parameters:
*  pSP  ->	pointer to a CSourcePhrase instance
*  Remarks:
*  Used to convert the (currently, at Aug 2005) 22 bool values in pSP to a 22 digit byte
*  string to be output in the f=" " attribute of the Adapt It document's sourcephrase
*  representations in XML. If we subsequently add extra BOOLs to CSourcePhrase, we only
*  need to make sure that any bool we add has a safe default value of FALSE (ie. zero), and
*  we then only need to change the hard coded constant below from 22 to 23 or whatever and
*  fix the rest accordingly and we'll be able to read legacy Adapt It documents in XML format
*  without having to bump a version number.
***************************************************************************************/
CBString MakeFlags(CSourcePhrase* pSP)
{
	CBString flagsStr;
	UInt32 n = 0;
	//char bin[34];
	if (pSP->m_bHasKBEntry)
		n |= hasKBEntryMask; // digit 1
	if (pSP->m_bNotInKB)
		n |= notInKBMask; // digit 2
	if (pSP->m_bHasGlossingKBEntry)
		n |= hasGlossingKBEntryMask; // digit 3
	if (pSP->m_bSpecialText)
		n |= specialTextMask; // digit 4
	if (pSP->m_bFirstOfType)
		n |= firstOfTypeMask; // digit 5
	if (pSP->m_bBoundary)
		n |= boundaryMask;	// digit 6
	if (pSP->m_bNullSourcePhrase)
		n |= nullSourcePhraseMask; // digit 7
	if (pSP->m_bRetranslation)
		n |= retranslationMask; // digit 8
	if (pSP->m_bBeginRetranslation)
		n |= beginRetranslationMask; // digit 9
	if (pSP->m_bEndRetranslation)
		n |= endRetranslationMask; // digit 10
	if (pSP->m_bHasFreeTrans)
		n |= hasFreeTransMask; // digit 11
	if (pSP->m_bStartFreeTrans)
		n |= startFreeTransMask; // digit 12
	if (pSP->m_bEndFreeTrans)
		n |= endFreeTransMask; // digit 13
	if (pSP->m_bHasNote)
		n |= hasNoteMask; // digit 14
	if (pSP->m_bHasBookmark)
		n |= hasBookmarkMask; // digit 15
	if (pSP->m_bChapter)
		n |= chapterMask; // digit 16
	if (pSP->m_bVerse)
		n |= verseMask; // digit 17
	if (pSP->m_bHasInternalMarkers)
		n |= hasInternalMarkersMask; // digit 18
	if (pSP->m_bHasInternalPunct)
		n |= hasInternalPunctMask; // digit 19
	if (pSP->m_bFootnote)
		n |= footnoteMask; // digit 20
	if (pSP->m_bFootnoteEnd)
		n |= footnoteEndMask; // digit 21
	if (pSP->m_bParagraph)
		n |= paragraphMask; // digit 22


	// convert it to an ascii string
	// the atoi() conversion function is not standard and the conversion to binary (with radix of 2) has 
	// no equivalent in wxWidgets, so I've defined a DecimalToBinary() function in helpers.cpp to do
	// the conversion
	char binValue[32];	// needs to be 32; DecimalToBinary assigns string index 0 through 30 leaving 
						// binValue[31] as hex zero (as set by memset below) to terminate the c-string 
	memset(binValue,0,sizeof(binValue));
	short sigDigits;
	sigDigits = DecimalToBinary(n, binValue);
	flagsStr = binValue;
	flagsStr = flagsStr.Mid(0,22); // Adapt It's source phrase currently uses only first 22 flags
	flagsStr.MakeReverse();
	return flagsStr;
}

/********************************************************************************
*
*  LIFT input as XML - call back functions
*
*********************************************************************************/
/*
// whm added 19May10
bool AtLIFTTag(CBString& tag)
{
	if (tag == xml_lift)
	{
		gpSrcPhrase = new CSourcePhrase;
		return TRUE;
	}
	else if (tag == xml_entry)
	{
		// create a new CTargetUnit instance on the heap
		gpTU = new CTargetUnit;
	}
	else if (tag == xml_sense)
	{
		// create a new CRefString instance on the heap
		gpRefStr = new CRefString;

		// set the pointer to the owning CTargetUnit, and add it to the m_translations member
		gpRefStr->m_pTgtUnit = gpTU; // the current one
		gpTU->m_pTranslations->Append(gpRefStr);
	}
	else if (tag == xml_gloss)
	{
		;
	}
	else if (tag == xml_definition)
	{
		;
	}
	else if (tag == xml_text)
	{
		;
	}
	else if (tag == xml_form)
	{
		;
	}
	else if (tag == xml_lexical_unit)
	{
		;
	}
	else
	{
		// unknown tag
		return FALSE;
	}
	return TRUE; // no error
}

bool AtLIFTEmptyElemClose(CBString& WXUNUSED(tag))
{
	// unused
	return TRUE;
}

bool AtLIFTAttr(CBString& tag,CBString& attrName,CBString& WXUNUSED(attrValue))
{
	if (tag == xml_lift)
	{
		if (attrName == xml_lift_version)
		{
			// unused
			;
		}
	}
	if (tag == xml_entry)
	{
		if (attrName == xml_guid)
		{
			// unused
			;
		}
		else if (attrName == xml_id)
		{
			// unused
			;
		}
	}
	if (tag == xml_form)
	{
		if (attrName == xml_lang)
		{
			// unused
			;
		}
	}
	return TRUE; // no error
}

bool AtLIFTEndTag(CBString& tag)
{
	if (tag == xml_entry)
	{
		// add the completed CTargetUnit to the CKB's m_pTargetUnits SPList
		// BEW 28May10 removed, as TUList is redundant
		//gpKB->m_pTargetUnits->Append(gpTU);

		// The following is patterned after the code in the sfm processing part
		// of DoKBImport().
		gpSrcPhrase->m_key.Empty();
		int nWordCount;
		
		// store the association in the KB, provided it is not already there
		if (gbIsGlossing)
		{
			gpSrcPhrase->m_gloss.Empty();
			gpSrcPhrase->m_bHasGlossingKBEntry = FALSE;
			nWordCount = 1;
			gpSrcPhrase->m_nSrcWords = 1; // temp default value
		}
		else
		{
			gpSrcPhrase->m_adaption.Empty();
			gpSrcPhrase->m_bHasKBEntry = FALSE;
			nWordCount = TrimAndCountWordsInString(gKeyStr); // strips off any leading and following whitespace
			gpSrcPhrase->m_nSrcWords = nWordCount;
		}

		if (nWordCount == 0 || nWordCount > MAX_WORDS)
		{
			// error condition
			gpSrcPhrase->m_nSrcWords = 1;
			gKeyStr.Empty();
			bKeyDefined = FALSE;
		}
		else
		{
			gpSrcPhrase->m_nSrcWords = nWordCount;
			gpSrcPhrase->m_key = gKeyStr;
		}

		// set up the association between this CTargetUnit's pointer and the source text key
		// in the current map
		(*gpMap)[gKeyStr] = gpTU;

		// clear the pointer (not necessary, but a good idea for making sure the code is sound)
		gpTU = NULL; // the m_pTargetUnits list will manage the pointer from now on
		// clear the gKeyStr
		gKeyStr.Empty();
		bKeyDefined = FALSE;
	}
	else if (tag == xml_lift)
	{
		delete gpSrcPhrase;
	}
	else if (tag == xml_sense || tag == xml_gloss) // treat sense and gloss the same
	{
		if (bKeyDefined)
		{
			if (gbIsGlossing)
			{
				if (!gpApp->m_pGlossingKB->IsAlreadyInKB(1,gKeyStr,gpRefStr->m_translation))
					gpApp->m_pGlossingKB->StoreText(gpSrcPhrase,gpRefStr->m_translation,TRUE);
													// TRUE means "allow empty string save"
			}
			else
			{
				if (!gpApp->m_pKB->IsAlreadyInKB(gpSrcPhrase->m_nSrcWords,gKeyStr,gpRefStr->m_translation))
					gpApp->m_pKB->StoreText(gpSrcPhrase,gpRefStr->m_translation,TRUE);
													// TRUE means "allow empty string save"
			}
			// prepare for another adaptation (or gloss) for this key
			gpSrcPhrase->m_adaption.Empty();
			gpRefStr->m_translation.Empty();
		}
	}
	else if (tag == xml_definition)
	{
		// nothing to be done
		;
	}
	else if (tag == xml_text)
	{
		// nothing to do
		;
	}
	else if (tag == xml_form)
	{
		// nothing to do
		;
	}
	else if (tag == xml_lexical_unit)
	{
		// nothing to do
		;
	}
	else
	{
		// unknown tag
		return FALSE;
	}
	return TRUE;
}

bool AtLIFTPCDATA(CBString& WXUNUSED(tag),CBString& pcdata,CStack*& pStack)
{
	// we use PCDATA in LIFT imports for the content delimited by <text>...</text> tags
	// in LIFT data <text> can occur as a tag embedded in either <lexical-unit> or <sense>
	if (pStack->Contains(xml_lexical_unit))
	{
		ReplaceEntities(pcdata);
#ifdef _UNICODE
		gKeyStr = gpApp->Convert8to16(pcdata); // key string for the map hashing to use
#else
		gKeyStr = pcdata.GetBuffer();
#endif
		gpSrcPhrase->m_key = gKeyStr;
		bKeyDefined = TRUE;
	}
	else if (pStack->Contains(xml_sense) || pStack->Contains(xml_gloss))
	{
		ReplaceEntities(pcdata);
		wxASSERT(gpRefStr != NULL);
#ifdef _UNICODE
		gpRefStr->m_translation = gpApp->Convert8to16(pcdata);
#else
		gpRefStr->m_translation = pcdata.GetBuffer();
#endif
		gpSrcPhrase->m_adaption = gpRefStr->m_translation;
	}
	return TRUE;
}
*/

bool AtLIFTTag(CBString& tag, CStack*& pStack)
{
	if (tag == xml_entry)
	{
		// create a new CTargetUnit instance on the heap - this may eventually be managed
		// by one of the maps, or if there is an instance in the map for the key later
		// being tested, this will needed to be deleted before the next xml_entry start
		// tag is encountered
		gpTU = new CTargetUnit;

        // this is tag stores (nested within its <lexical-unit> element) a lexeme, which to
        // Adapt It will become a KB adaptation or gloss, depending on whether we are
        // importing an adaptingKB or a glossingKB; do prepare an empty wxString ready to
        // receive its value
		gKeyStr.Empty(); // prepare for PCDATA parse later on (the wxChar string)
	}
	else if (tag == xml_sense)
	{
        // Note: we accept only a <text> tag which is in a definition or gloss, either of
        // which is embedded in a sense. But there can be many other <text> elements in a
        // <sense>, but at deeper levels of nesting than <gloss> or <definition>. The
        // deeper ones will have different parent tags on the stack, a fact which we'll use
        // to advantage with a function called MyParentsAre(), in order to exclude parsing
        // <text> elements which have nothing to do with a gloss or definition. Tags which
        // can be in <sense> and which can have <text> embedded in them are:
        // relation, grammatical-info, note, example, reversal or illustration (but we
        // allow subsense - which can contain additional <sense> elements... -- the best
        // way to handle complex nesting conditions like this is to have a function that
        // checks for explicit tags above the current one - hence MyParentsAre() which can
        // test for up to 3 explicit parent tags for the current one

		// To prepare for the possibility that we will add it to the KB, 
		// create a new CRefString instance on the heap (we won't do anything except
		// delete it if we later find that it already is on the heap)
		gpRefStr = new CRefString; // also creates and initializes an owned CRefStringMetadata
	}
	else
	{
		// unknown tag
		return TRUE; // ignore other tags
	}
	return TRUE; // no error
}

bool AtLIFTEmptyElemClose(CBString& WXUNUSED(tag), CStack*& WXUNUSED(pStack))
{
	// unused
	return TRUE;
}

bool AtLIFTAttr(CBString& WXUNUSED(tag),CBString& WXUNUSED(attrName),
				CBString& WXUNUSED(attrValue),CStack*& WXUNUSED(pStack))
{
	// there are no attributes in LIFT that we need to process
	return TRUE; // no error
}

bool AtLIFTEndTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	if (tag == xml_sense)
	{
		//if (gpRefStr != NULL)
		//{
			gpRefStr->DeleteRefString(); // also deletes its CRefStringMetadata instance
		//}
	}
	else if (tag == xml_entry)
	{
		//if (gpTU != NULL)
		//{
			// this one is not being managed by an entry in a map, and so we must delete
			// it here to avoid a memory leak
			delete gpTU; // we've not added anything to its m_pTranslations list, 
						 // so it's safe to just delete it alone here
		//}
	}
	else
	{
		// unknown tag
		return TRUE; // ignore other irrelevant tags
	}
	return TRUE;
}

bool AtLIFTPCDATA(CBString& tag,CBString& pcdata, CStack*& pStack)
{
	// we use PCDATA in LIFT imports for the content delimited by <text>...</text> tags
	// in LIFT data <text> can occur as a tag embedded in either <lexical-unit> or <sense>
	if (pStack->MyParentsAre(3, xml_form, xml_lexical_unit, xml_entry) )
	{
		// this is tag stores a lexeme, which to Adapt It will become a KB adaptation or
		// gloss, depending on whether we are importing an adaptingKB or a glossingKB
		ReplaceEntities(pcdata);
#ifdef _UNICODE
		gKeyStr = gpApp->Convert8to16(pcdata); // key string for the map hashing to use
#else
		gKeyStr = pcdata.GetBuffer();
#endif
		// set up the map pointer - this depends on how many words there are in the source
		// text in gKeyStr if the KB is an adapting one, but for a glossingKB it is always
		// the first map
		int numWords = 1;
		if (gpKB->IsThisAGlossingKB())
		{
			gpMap = gpApp->m_pGlossingKB->m_pMap[0];
		}
		else
		{
			numWords = TrimAndCountWordsInString(gKeyStr); // strips off any leading and following whitespace
			gpMap = gpApp->m_pKB->m_pMap[numWords - 1];
		}

		// find out if there is CTargetUnit in this map already, for this key; the
		// following call returns NULL if there is no CTargetUnit for the key yet in the
		// map, otherwise it returns the map's CTargetUnit instance
		gpTU_From_Map = gpKB->GetTargetUnit(numWords, gKeyStr); // does an AutoCapsLookup()
		// if gpTU_From_Map is non-NULL, then each <definition> tag, or <gloss> tag, will
		// yield a potential adaptation (or gloss if the KB is a glossingKB) which will
		// either belong to this CTargetUnit already, or it won't - in which case we'll
		// have to add it. We can't do this test until we've re-entered AtLiftPCDATA() in
		// order to process PCDATA from the <text> tag within either a <definition> tag or
		// a <gloss> tag 
	}
	else if (pStack->MyParentsAre(3,xml_form, xml_definition, xml_sense) ||
			 pStack->MyParentsAre(3,xml_gloss, xml_sense, xml_entry) )
	{
		ReplaceEntities(pcdata);
		wxASSERT(gpRefStr != NULL);
		wxString textStr;
#ifdef _UNICODE
		textStr = gpApp->Convert8to16(pcdata);
#else
		textStr = pcdata.GetBuffer();
#endif
		if (gpTU_From_Map == NULL)
		{
			// there is no CTargetUnit pointer instance in the map for the given key, so
			// put add the gpRefStr instance to the gpTU instance, and fill out the value
			// of the members, as this gpTU will have to be managed henceforth by the map,
			// so we don't delete it...
			// set the pointer to the owning CTargetUnit, and add it to the m_translations
			// member
			gpRefStr->m_pTgtUnit = gpTU;
			gpTU->m_pTranslations->Append(gpRefStr);

			// add its adaptation, or gloss if the pKB is a glossingKB (creation datetime
			// is the datetime of the import - this is potentially more useful than any
			// other datetime, so leave it that way)
			gpRefStr->m_translation = textStr;
			gpRefStr->m_refCount = 1;
			gpRefStr->GetRefStringMetadata()->SetCreationDateTime(GetDateTimeNow());
			gpRefStr->GetRefStringMetadata()->SetWhoCreated(SetWho());

            // it's ready, so store it in the map
			(*gpMap)[gKeyStr] = gpTU;

            // we create a new pointer, as the map now manages the old one; we use the
            // default constructor, which does not pre-fill the m_creatorDateTime and
            // m_whoCreated members of its owned CRefStringMetadata - this is better, to
            // set these explicitly only for those CRefString instances whose management is
            // to be taken over by the map, (this saves time, as not all CRefString
            // instances created by the parser succeed in being managed by the map, those
            // that don't have to be deleted from the heap)
			gpRefStr = new CRefString; 

			// now that pTU is managed by the map, we can't risk leaving it non-NULL
			// because later on it would be deleted in AtLIFTEndTag(); so instead, create a
			// new one to replace it and continue with that one - if the new one is to be
			// deleted, it won't then matter
			gpTU = new CTargetUnit;
		}
		else
		{
			// there is a CTargetUnit pointer in the map for the given key; so find out if
			// there is a CRefString instance for the given textStr
			textStr.Trim();
			textStr.Trim(FALSE);
			CRefString* pRefStr_In_TU = gpKB->GetRefString(gpTU_From_Map,textStr);
			if (pRefStr_In_TU == NULL)
			{
                // this particular adaptation or gloss is not yet in the map's CTargetUnit
                // instance, so put it in there and have the map manage the CRefString
                // instance's pointer, but the gpTU instance we created earlier in the
                // callbacks is not then needed once all the <sense> elements relevant to 
                // it have been processed, and so it will have to be removed from the heap
                // eventually (at AtLIFTEndTag() callback)
				gpRefStr->m_refCount = 1;
				gpRefStr->m_translation = textStr;
				gpRefStr->m_pTgtUnit = gpTU_From_Map;
				gpRefStr->GetRefStringMetadata()->SetCreationDateTime(GetDateTimeNow());
				gpRefStr->GetRefStringMetadata()->SetWhoCreated(SetWho());
				// the CRefStringMetadata has been initialized with this use:computer's
				// m_whoCreated value, and the import datetime as the m_creationDateTime
				// (the latter is more meaningful than using the a creation datetime from
				// the imported file - especially if we mistakenly are importing malicious
				// data and need to remove it later on)
				gpTU_From_Map->m_pTranslations->Append(gpRefStr); // the map entry now manages
																  // this CRefString instance

				// we create a new pointer, as the map now manages the old one; we use the
				// default constructor, which does not pre-fill the m_creatorDateTime and
				// m_whoCreated members of its owned CRefStringMetadata - this is better, to
				// set these explicitly only for those CRefString instances whose management is
				// to be taken over by the map, (this saves time, as not all CRefString
				// instances created by the parser succeed in being managed by the map, those
				// that don't have to be deleted from the heap)
				gpRefStr = new CRefString; 

				// leave gpTU unchanged, so that the LIFT end-tag callback will delete it
                // when the current <entry> contents have finished being processed
			}
			else
			{
				// this particular adaptation or gloss is in the map's CTargetUnit pointer
				// already, so we can ignore it. We must delete both the gpRefStr (and
				// its owned CRefStringMetadata instance), and also delete the gpTU we
				// created, so there is nothing more to do here (i.e. *DON'T* set gpTU and
				// gpRefStr to NULL) as the deletions will be done in AtLIFTEndTag()
				;
			}
		}
	}
	return TRUE;
}


/********************************************************************************
*
*  Adapt It KB and GlossingKB input as XML - call back functions
*
*********************************************************************************/

// BEW changed 19Apr06 in support of eliminating GKB in favour of KB element for both KB types
// so as to better support SILConverters plug-in done by Bob Eaton, which needs to access either
// glossing or adapting kb. (We need to be able to read the older GKB element too, for legacy
// KB files -- we can do this without having to bump the version number.) Supporting this
// change requires no change here in XML.cpp, but only in the DoKBSaveAsXML() function in the
// Adapt_It.cpp file, so we there used KB rather than GKB for the element name, a very trivial change

// BEW 27Mar10 updated for support of doc version 5 (extra case needed)
bool AtKBTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	if (tag == xml_kb) // if it's a "KB" tag
	{
		// this tag only has attributes; for kbv1 the second attribute
		// within the KB tag was docVersion - which was a design mistake, as nothing in
		// the KB xml relies on any particular version of the document; however, since we
		// did not originally make the KB xml versionable except meaninglessly with the
		// docVersion value, we can use the presence of docVersion attribute as a flag
		// indicating we are about to parse in a kbv1 KB, and so set the
		// gnKbVersionBeingParsed global boolean to KB_VERSION1 (defined as 1) in that
		// circumstance; otherwise, this attribute will be absent and instead and
		// attribute called kbVersion will replace it - in that circumstance, that
		// attribute will hold the version number for the KB file being parsed - and
		// currently that will mean KB_VERSION2 (defined as 2). Later versions of the KB,
		// if ever needed, will bump the kbVersion attribute's content to a higher
		// integer, and the KB code from that point on is duly versioned.
		 
		// ** note **, the next thing parsed will be either a docVersion attribute or
		// kbVersion attribute, and the callback for those is AtKBAttr() - so there is
		// where gbKbVersionBeingParsed will get set to whatever value it is to take in
		// the rest of the parse.
		
		// set the gpDoc pointer
		gpDoc = gpApp->GetDocument();
		return TRUE;
	}
	// remainder of the tag inventory are versionable except for the first attribute,
	// which from version 3.1.0 and onwards is the KbType (the rest of the stuff is same
	//  for regular and unicode apps, so no conditional compile required here)
	switch (gnKbVersionBeingParsed)
	{
		case KB_VERSION1:
		{
			if (tag == xml_map)
			{
				// nothing to do, because the map number is not parsed yet
				return TRUE;
			}
			else if (tag == xml_tu)
			{
				// create a new CTargetUnit instance on the heap
				gpTU = new CTargetUnit;
			}
			else if (tag == xml_rs)
			{
				// create a new CRefString instance on the heap -- when parsing in kbv1,
				// we must transition it to kbv2 automatically; to do that, we use the
				// CRefString creator which takes the owning CTargetUnit* as a parameter -
				// it not just hooks up to the owning CTargetUnit, but it calls a
				// non-default creator for the CRefStringMetadata instance which hooks
				// itself up to the owning CRefString instance and sets the members for
				// creation datetime and whoCreated, as well -- transitioning the kbv1 to
				// kbv2 in doing so
				gpRefStr = new CRefString(gpTU);

				// add it, plus its pointed at CRefStringMetadata instance, to the 
				// m_pTranslations member of the owning CTargetUnit instance
				gpTU->m_pTranslations->Append(gpRefStr);
			}
			else if (tag == xml_aikb)
			{
				// support .NET xml parsing -- nothing to be done here
				;
			}
			else
			{
				// unknown tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
		}
		break;
		case KB_VERSION2:
		{
			if (tag == xml_map)
			{
				// nothing to do, because the map number is not parsed yet
				return TRUE;
			}
			else if (tag == xml_tu)
			{
				// create a new CTargetUnit instance on the heap
				gpTU = new CTargetUnit;
			}
			else if (tag == xml_rs)
			{
				// create a new CRefString instance on the heap; this for kbv2 requires we
				// use the default CRefString creator, which doesn't try to initialize any
				// of the members of its pointed at CRefStringMetadata instance except the
				// m_pRefStringOwner member which points back to this owning CRefString
				// instance; then later parsing of AtKBAttr() will set the relevant member
				// variables of the CRefStringMetadata instance with the values parsed
				gpRefStr = new CRefString;

				// set the pointer to the owning CTargetUnit, and add it to the 
				// m_pTranslations member
				gpRefStr->m_pTgtUnit = gpTU; // the current one
				gpTU->m_pTranslations->Append(gpRefStr);
			}
			else if (tag == xml_aikb)
			{
				// support .NET xml parsing -- nothing to be done here
				;
			}
			else
			{
				// unknown  tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
		}
		break;
	}
	return TRUE; // no error
}

bool AtKBEmptyElemClose(CBString& WXUNUSED(tag), CStack*& WXUNUSED(pStack))
{
	// unused
	return TRUE;
}

// BEW 27Mar10 updated for support of doc version 5 (extra case needed)
bool AtKBAttr(CBString& tag,CBString& attrName,CBString& attrValue, CStack*& WXUNUSED(pStack))
{
	int num;
	if (tag == xml_kb && (attrName == xml_docversion || attrName == xml_kbversion))
	{
		// (the kbVersion attribute is not versionable, so have it outside of the switch)
		// see comments at the top of AtKBTag() about docVersion and kbVersion, the former
		// is deprecated, we use the latter from now on (except when transitioning a kbv1
		// KB to kbv2)
		if (attrName == xml_docversion)
		{
			// it must be a kbv1 KB, so use KB_VERSION1 case in the switch
			gnKbVersionBeingParsed = (int)KB_VERSION1;

			// note: kbv1 should not try to compensate for the lack of the xml_glossingKB
			// attribute (ie. to set or clear the m_bGlossingKB flag in CKB which defines
			// whether the CKB is a glossing one, or adapting one, respectively - because
			// the LoadGlossingKB() call and LoadKB() call will set or clear it instead
		}
		else
		{
			// it must be a kbv21 KB, so in the switch use whatever version number is
			// stored, it will be 2 (or more if we someday have a version3 KB or higher)
			gnKbVersionBeingParsed = atoi(attrValue);

            // note: kbv2 should not try use the parsed value for the xml_glossingKB
            // attribute to set or clear the m_bGlossingKB flag in CKB which defines
            // whether the CKB is a glossing one, or adapting one, respectively. This flag
            // is always set by the LoadGlossingKB() call, or the LoadKB() call,
            // respectively. The most we should do in the XML.cpp functions is just check
            // that the parsed in value is identical to that set by the LoadKB() or
            // LoadGlossingKB() caller - each of those functions sets or clears the flag,
            // before passing control to the xml parsing functions for loading of the rest
			// of the CKB information, so we can here count on the appropriate value
			// having been set already in the m_bGlossingKB member boolean.
		}
		return TRUE;
	}
	// the rest is versionable
	switch (gnKbVersionBeingParsed)
	{
		case KB_VERSION1:
		{
			// put the more commonly encountered tags at the top, for speed
#ifndef _UNICODE // ANSI version (ie. regular)
			if (tag == xml_rs)
			{
				if (attrName == xml_n)
				{
					gpRefStr->m_refCount = atoi(attrValue);
				}
				else if (attrName == xml_a)
				{
					ReplaceEntities(attrValue);
					gpRefStr->m_translation = attrValue;
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_tu)
			{
				if (attrName == xml_f)
				{
					// code below avoids: warning C4800: 'int' : 
					// forcing value to bool 'true' or 'false' (performance warning)
					if (attrValue == "0")
						gpTU->m_bAlwaysAsk = FALSE;
					else
						gpTU->m_bAlwaysAsk = TRUE;
				}
				else if (attrName == xml_k)
				{
					ReplaceEntities(attrValue);
					gKeyStr = attrValue; // key string for the map hashing to use
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_map)
			{
				if (attrName == xml_mn)
				{
					// set the map index (one less than the map number)
					num = atoi(attrValue);
					gnMapIndex = num - 1;
					wxASSERT(gnMapIndex >= 0);

					// now we know the index, we can set the map pointer
					gpMap = gpKB->m_pMap[gnMapIndex];
					wxASSERT(gpMap->size() == 0); // has to start off empty
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_kb)
			{
				if (attrName == xml_srcnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_sourceLanguageName = attrValue;
				}
				else if (attrName == xml_tgtnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_targetLanguageName = attrValue;
				}
				else if (attrName == xml_max)
				{
					gpKB->m_nMaxWords = atoi(attrValue);
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_aikb)
			{
				if (attrName == xml_xmlns)
				{
					// .NET support for xml parsing of KB file;
					// *ATTENTION BOB*  add any bool setting you need here
					wxString thePath(attrValue); // I've stored the http://www.sil.org/computing/schemas/AdaptIt KB.xsd
												 // here in a local wxString for now, in case you need to use it
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else
			{
				// unknown tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
#else // Unicode version
			if (tag == xml_rs)
			{
				if (attrName == xml_n)
				{
					gpRefStr->m_refCount = atoi(attrValue);
				}
				else if (attrName == xml_a)
				{
					ReplaceEntities(attrValue);
					gpRefStr->m_translation = gpApp->Convert8to16(attrValue);
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_tu)
			{
				if (attrName == xml_f)
				{
					if (attrValue == "0")
						gpTU->m_bAlwaysAsk = FALSE;
					else
						gpTU->m_bAlwaysAsk = TRUE;
				}
				else if (attrName == xml_k)
				{
					ReplaceEntities(attrValue);
					gKeyStr = gpApp->Convert8to16(attrValue); // key string for the map hashing to use
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_map)
			{
				if (attrName == xml_mn)
				{
					// set the map index (one less than the map number)
					num = atoi(attrValue);
					gnMapIndex = num - 1;
					wxASSERT(gnMapIndex >= 0);

					// now we know the index, we can set the map pointer
					gpMap = gpKB->m_pMap[gnMapIndex];
					wxASSERT(gpMap->size() == 0); // has to start off empty
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_kb)
			{
				if (attrName == xml_srcnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_sourceLanguageName = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_tgtnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_targetLanguageName = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_max)
				{
					gpKB->m_nMaxWords = atoi(attrValue);
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_aikb)
			{
				if (attrName == xml_xmlns)
				{
					// .NET support for xml parsing of KB file;
					// *ATTENTION BOB*  add any bool setting you need here
					// TODO: whm check the following conversion
					wxString thePath(attrValue,wxConvUTF8); // I've stored the http://www.sil.org/computing/schemas/AdaptIt KB.xsd
														// here in a local wxString for now, in case you need to use it
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else
			{
				// unknown tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
#endif // for _UNICODE #defined
		}
		break;
		case KB_VERSION2:
		{
			// put the more commonly encountered tags at the top, for speed
#ifndef _UNICODE // ANSI version (ie. regular)
			// new string contstants for kbv2 new attribute names
			//const char xml_creationDT[] = "cDT";
			//const char xml_modifiedDT[] = "mDT";
			//const char xml_deletedDT[] = "dDT";
			//const char xml_whocreated[] = "wC";
			//const char xml_deletedflag[] = "df";

			if (tag == xml_rs)
			{
				if (attrName == xml_n)
				{
					gpRefStr->m_refCount = atoi(attrValue);
				}
				else if (attrName == xml_a)
				{
					ReplaceEntities(attrValue);
					gpRefStr->m_translation = attrValue;
				}
				else if (attrName == xml_deletedflag)
				{
					bool flag = attrValue == "0" ? (bool)0 : (bool)1;	
					gpRefStr->SetDeletedFlag(flag); 
				}
				else if (attrName == xml_creationDT)
				{
					// no entity replacement needed for datetime values
					wxString value = attrValue;
					gpRefStr->GetRefStringMetadata()->SetCreationDateTime(value); 
				}
				else if (attrName == xml_whocreated)
				{
					// could potentially require entity replacement, so do it to be safe
					ReplaceEntities(attrValue);
					wxString value = attrValue;
					gpRefStr->GetRefStringMetadata()->SetWhoCreated(value);
				}
				else if (attrName == xml_modifiedDT)
				{
					// no entity replacement needed for datetime values
					wxString value = attrValue;
					gpRefStr->GetRefStringMetadata()->SetModifiedDateTime(value); 
				}
				else if (attrName == xml_deletedDT)
				{
					// no entity replacement needed for datetime values
					wxString value = attrValue;
					gpRefStr->GetRefStringMetadata()->SetDeletedDateTime(value); 
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_tu)
			{
				if (attrName == xml_f)
				{
					// code below avoids: warning C4800: 'int' : 
					// forcing value to bool 'true' or 'false' (performance warning)
					if (attrValue == "0")
						gpTU->m_bAlwaysAsk = FALSE;
					else
						gpTU->m_bAlwaysAsk = TRUE;
				}
				else if (attrName == xml_k)
				{
					ReplaceEntities(attrValue);
					gKeyStr = attrValue; // key string for the map hashing to use
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_map)
			{
				if (attrName == xml_mn)
				{
					// set the map index (one less than the map number)
					num = atoi(attrValue);
					gnMapIndex = num - 1;
					wxASSERT(gnMapIndex >= 0);

					// now we know the index, we can set the map pointer
					gpMap = gpKB->m_pMap[gnMapIndex];
					wxASSERT(gpMap->size() == 0); // has to start off empty
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_kb)
			{
				if (attrName == xml_max)
				{
					gpKB->m_nMaxWords = atoi(attrValue);
				}
				else if (attrName == xml_glossingKB)
				{
					// check the parsed value matches what was set by LoadKB() or
					// LoadGlossingKB() as the case may be - if there is no match then we
					// must abort the parse immediately because we'd either be trying to
					// load a glossingKB into as the adapting KB, or vise versa
					bool bGlossingKB;
					if (attrValue == "0")
					{
						bGlossingKB = FALSE;
					}
					else
					{
						bGlossingKB = TRUE;
					}
					if (GetGlossingKBFlag(*gpKB) != bGlossingKB)
					{
						// there is no match of the flag value parsed in with the
						// Load...() call, which is an error. This error should never
						// happen, so it can have an error message which is not localizable. 
						wxString str;
						str = str.Format(_T(
"Error. Adapt It is trying either to load a glossing knowledge base as if it was an adapting one, or an adapting one as if it was a glossing one. Either way, this must not happen so the load operation will now be aborted."));
						wxMessageBox(str, _T("Bad LoadKB() or bad LoadGlossingKB() call"), wxICON_ERROR);
						return FALSE;
					}
				}
				else if (attrName == xml_srcnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_sourceLanguageName = attrValue;
				}
				else if (attrName == xml_tgtnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_targetLanguageName = attrValue;
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_aikb)
			{
				if (attrName == xml_xmlns)
				{
					// .NET support for xml parsing of KB file;
					// *ATTENTION BOB*  add any bool setting you need here
					wxString thePath(attrValue); // I've stored the http://www.sil.org/computing/schemas/AdaptIt KB.xsd
												 // here in a local wxString for now, in case you need to use it
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else
			{
				// unknown tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
#else // Unicode version
			// new string contstants for kbv2 new attribute names
			//const char xml_creationDT[] = "cDT";
			//const char xml_modifiedDT[] = "mDT";
			//const char xml_deletedDT[] = "dDT";
			//const char xml_whocreated[] = "wC";
			//const char xml_deletedflag[] = "df";

			if (tag == xml_rs)
			{
				if (attrName == xml_n)
				{
					gpRefStr->m_refCount = atoi(attrValue);
				}
				else if (attrName == xml_a)
				{
					ReplaceEntities(attrValue);
					gpRefStr->m_translation = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_deletedflag)
				{
					bool flag = attrValue == "0" ? (bool)0 : (bool)1;	
					gpRefStr->SetDeletedFlag(flag); 
				}
				else if (attrName == xml_creationDT)
				{
					// no entity replacement needed for datetime values
					gpRefStr->GetRefStringMetadata()->SetCreationDateTime(gpApp->Convert8to16(attrValue)); 
				}
				else if (attrName == xml_whocreated)
				{
					// could potentially require entity replacement, so do it to be safe
					ReplaceEntities(attrValue);
					gpRefStr->GetRefStringMetadata()->SetWhoCreated(gpApp->Convert8to16(attrValue));
				}
				else if (attrName == xml_modifiedDT)
				{
					// no entity replacement needed for datetime values
					gpRefStr->GetRefStringMetadata()->SetModifiedDateTime(gpApp->Convert8to16(attrValue)); 
				}
				else if (attrName == xml_deletedDT)
				{
					// no entity replacement needed for datetime values
					gpRefStr->GetRefStringMetadata()->SetDeletedDateTime(gpApp->Convert8to16(attrValue)); 
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_tu)
			{
				if (attrName == xml_f)
				{
					if (attrValue == "0")
						gpTU->m_bAlwaysAsk = FALSE;
					else
						gpTU->m_bAlwaysAsk = TRUE;
				}
				else if (attrName == xml_k)
				{
					ReplaceEntities(attrValue);
					gKeyStr = gpApp->Convert8to16(attrValue); // key string for the map hashing to use
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_map)
			{
				if (attrName == xml_mn)
				{
					// set the map index (one less than the map number)
					num = atoi(attrValue);
					gnMapIndex = num - 1;
					wxASSERT(gnMapIndex >= 0);

					// now we know the index, we can set the map pointer
					gpMap = gpKB->m_pMap[gnMapIndex];
					wxASSERT(gpMap->size() == 0); // has to start off empty
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_kb)
			{
				if (attrName == xml_max)
				{
					gpKB->m_nMaxWords = atoi(attrValue);
				}
				else if (attrName == xml_glossingKB)
				{
					// check the parsed value matches what was set by LoadKB() or
					// LoadGlossingKB() as the case may be - if there is no match then we
					// must abort the parse immediately because we'd either be trying to
					// load a glossingKB into as the adapting KB, or vise versa
					bool bGlossingKB;
					if (attrValue == "0")
					{
						bGlossingKB = FALSE;
					}
					else
					{
						bGlossingKB = TRUE;
					}
					if (GetGlossingKBFlag(*gpKB) != bGlossingKB)
					{
						// there is no match of the flag value parsed in with the
						// Load...() call, which is an error. This error should never
						// happen, so it can have an error message which is not localizable. 
						wxString str;
						str = str.Format(_T(
"Error. Adapt It is trying either to load a glossing knowledge base as if it was an adapting one, or an adapting one as if it was a glossing one. Either way, this must not happen so the load operation will now be aborted."));
						wxMessageBox(str, _T("Bad LoadKB() or bad LoadGlossingKB() call"), wxICON_ERROR);
						return FALSE;
					}
				}
				else if (attrName == xml_srcnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_sourceLanguageName = gpApp->Convert8to16(attrValue);
				}
				else if (attrName == xml_tgtnm)
				{
					ReplaceEntities(attrValue);
					gpKB->m_targetLanguageName = gpApp->Convert8to16(attrValue);
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else if (tag == xml_aikb)
			{
				if (attrName == xml_xmlns)
				{
					// .NET support for xml parsing of KB file;
					// *ATTENTION BOB*  add any bool setting you need here
					// TODO: whm check the following conversion
					wxString thePath(attrValue,wxConvUTF8); // I've stored the http://www.sil.org/computing/schemas/AdaptIt KB.xsd
														// here in a local wxString for now, in case you need to use it
				}
				else
				{
					// unknown attribute
					// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
					// parse and hence the application, then return FALSE; otherwise return
					// TRUE to cause the parser to keep going (unknown data then just does not
					// find its way into the application's internal structures)
					//return FALSE;
					return TRUE;
				}
			}
			else
			{
				// unknown tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
#endif // for _UNICODE #defined
		}
		break;
	}
	return TRUE; // no error
}

// BEW 27Mar10 updated for support of doc version 5 (extra case needed)
bool AtKBEndTag(CBString& tag, CStack*& WXUNUSED(pStack))
{
	switch (gnKbVersionBeingParsed) 
	{
		case KB_VERSION1:
		case KB_VERSION2:
		{
			if (tag == xml_tu)
			{
				// add the completed CTargetUnit to the CKB's m_pTargetUnits SPList
				// BEW 28May10 removed, as TUList is redundant
				//gpKB->m_pTargetUnits->Append(gpTU);

				// set up the association between this CTargetUnit's pointer and the source text key
				// in the current map
				(*gpMap)[gKeyStr] = gpTU;

				// clear the pointer (not necessary, but a good idea for making sure the code is sound)
				gpTU = NULL; // the m_pTargetUnits list will manage the pointer from now on
			}
			else if (tag == xml_map)
			{
				// nothing to be done
				;
			}
			else if (tag == xml_kb)
			{
				// nothing to be done
				;
			}
			else if (tag == xml_aikb)
			{
				// nothing to do
				;
			}
			else
			{
				// unknown tag
				// BEW 3Jun10; if unknowns are to be treated as grounds for aborting the
				// parse and hence the application, then return FALSE; otherwise return
				// TRUE to cause the parser to keep going (unknown data then just does not
				// find its way into the application's internal structures)
				//return FALSE;
				return TRUE;
			}
			break;
		}
	}
	return TRUE;
}

bool AtKBPCDATA(CBString& WXUNUSED(tag),CBString& WXUNUSED(pcdata),CStack*& WXUNUSED(pStack))
{
	// we don't have any PCDATA in the document XML files
	return TRUE;
}


/*****************************************************************
*
* ReadBooks_XML
*
* Returns: TRUE if no error, else FALSE
*
* Parameters:
* -> path  -- (wxString) absolute path to the books.xml file on the storage medium
*
* Calls ParseXML to parse the books.xml file containing book names (typically
* OT and NT book names plust "Other Texts", but the file could contain non-Biblical
* names)
*
*******************************************************************/

bool ReadBooks_XML(wxString& path)
{
	bool bXMLok = ParseXML(path,AtBooksTag,AtBooksEmptyElemClose,AtBooksAttr,
							AtBooksEndTag,AtBooksPCDATA);
	return bXMLok;
}	

/*****************************************************************
*
* ReadSFM_XML
*
* Returns: TRUE if no error, else FALSE
*
* Parameters:
* -> path  -- (wxString) absolute path to the AI_USFM.xml file on the storage medium
*
* Calls ParseXML to parse the AI_USFM.xml file containing USFM and PNG SFM
* definitions and attributes
*
*******************************************************************/

bool ReadSFM_XML(wxString& path)
{
	bool bXMLok = ParseXML(path,AtSFMTag,AtSFMEmptyElemClose,AtSFMAttr,
							AtSFMEndTag,AtSFMPCDATA);
	return bXMLok;
}

/*****************************************************************
*
* ReadDoc_XML
*
* Returns: TRUE if no error, else FALSE
*
* Parameters:
*	path  -> (wxString) absolute path to the *.xml document file on the storage medium
*
* Calls ParseXML to parse an Adapt It (or Adapt It Unicode) xml document file
*
*******************************************************************/

bool ReadDoc_XML(wxString& path, CAdapt_ItDoc* pDoc)
{
	// BEW modified 07Nov05 to set gpDoc here, not in AtDocTag()
	// set the static document pointer used only for parsing the XML document
	gpDoc = pDoc;

#ifdef __WXMSW__
	// whm added 27May07 put up a progress dialog. Since we do not know the length of the
	// document at this point the dialog will simply display the message
	// "Reading XML Data For: <filename> Please Wait..." until the whole doc has been read.
	wxFileName fn(path);
	wxString progMsg = _("Reading XML Data For: %s Please Wait...");
	wxString msgDisplayed = progMsg.Format(progMsg,fn.GetFullName().c_str());
	wxProgressDialog progDlg(_("Opening The Document"),
                    msgDisplayed,
                    100,    // range
                    gpApp->GetMainFrame(),   // parent
                    //wxPD_CAN_ABORT |
                    //wxPD_CAN_SKIP |
                    wxPD_APP_MODAL |
                    // wxPD_AUTO_HIDE | -- try this as well
                    //wxPD_ELAPSED_TIME |
                    //wxPD_ESTIMATED_TIME |
                    //wxPD_REMAINING_TIME |
                    wxPD_SMOOTH // - makes indeterminate mode bar on WinXP very small
                    );
#else
	// wxProgressDialog tends to hang on wxGTK so I'll just use the simpler CWaitDlg
	// notification on wxGTK and wxMAC
	// put up a Wait dialog - otherwise nothing visible will happen until the operation is done
	CWaitDlg waitDlg(gpApp->GetMainFrame());
	// indicate we want the reading file wait message
	waitDlg.m_nWaitMsgNum = 2;	// 2 "Please wait while Adapt It opens the document..."
	waitDlg.Centre();
	waitDlg.Show(TRUE);
	waitDlg.Update();
	// the wait dialog is automatically destroyed when it goes out of scope below.
#endif


	bool bXMLok = ParseXML(path,AtDocTag,AtDocEmptyElemClose,AtDocAttr,
							AtDocEndTag,AtDocPCDATA);
	return bXMLok;
}	

/*****************************************************************
*
* ReadKB_XML
*
* Returns: TRUE if no error, else FALSE
*
* Parameters:
*	path  -> (wxString) absolute path to the *.xml KB or GlossingKB file on the storage medium
*	pKB   -> pointer to the CKB instance being filled out
*
* Calls ParseXML to parse an Adapt It (or Adapt It Unicode) xml knowledge base file
*
*******************************************************************/

bool ReadKB_XML(wxString& path, CKB* pKB)
{
	wxASSERT(pKB);
	gpKB = pKB; // set the global gpKB used by the callback functions
	bool bXMLok = ParseXML(path,AtKBTag,AtKBEmptyElemClose,AtKBAttr,
							AtKBEndTag,AtKBPCDATA);
	return bXMLok;
}	

/*****************************************************************
*
* ReadLIFT_XML
*
* Returns: TRUE if no error, else FALSE
*
* Parameters:
*	path  -> (wxString) absolute path to the *.xml KB or GlossingKB file on the storage medium
*	pKB   -> pointer to the CKB instance being filled out
*
* Calls ParseXML to parse a LIFT xml file
*
*******************************************************************/

bool ReadLIFT_XML(wxString& path, CKB* pKB)
{
	wxASSERT(pKB);
	gpKB = pKB; // set the global gpKB used by the callback functions
	bool bXMLok = ParseXML(path,AtLIFTTag,AtLIFTEmptyElemClose,AtLIFTAttr,
							AtLIFTEndTag,AtLIFTPCDATA);
	return bXMLok;
}	

// currently this is called in XML.cpp only, but it could be useful elsewhere
// The returned string keeps the filter marker wrappers and their markers and data
// contents, but the strFreeTrans, strNote, and strCollectedBackTrans parameters return
// their strings with markers, endmarkers and filter marker wrappers removed - that is,
// just the raw free translation, note, or collected back translation text only
wxString ExtractWrappedFilteredInfo(wxString strTheRestOfMarkers, wxString& strFreeTrans,
				wxString& strNote, wxString& strCollectedBackTrans, wxString& strRemainder)
{
	wxString info = strTheRestOfMarkers;
	strRemainder = strTheRestOfMarkers;
	if (info.IsEmpty())
	{
		return info;
	}
	wxString strConcat = _T(""); // store each substring here
	int offsetToStart = 0;
	int offsetToEnd = 0;

	// get the each \~FILTER ... \~FILTER*  wrapped substring, (each such contains one SF
	// marked up content string, of form \marker <content> \endMarker (but no < or >
	// chars)) and return the wrapped substring, concatenated to one string
	
	offsetToStart = info.Find(filterMkr);
	offsetToEnd = info.Find(filterMkrEnd);
	while (offsetToStart != wxNOT_FOUND && offsetToEnd != wxNOT_FOUND && !info.IsEmpty())
	{
		offsetToEnd += filterMkrEndLen; // add length of \~FILTER*  (9 chars)
		int length = offsetToEnd - offsetToStart;
		wxString substring = info.Mid(offsetToStart,length);

		// check for, and extract, any free trans, note, or collected back trans; but if
		// it is none of these, concatenate it to whatever is going to be returned to the
		// caller to go into the m_filteredInfo member of CSourcePhrase
		wxString innerStr = RemoveOuterWrappers(substring);
		wxString mkr;
		wxString endMkr;
		wxString content;
		// we have to discern between \bt and a \bt-derived marker, the former's content
		// goes in m_collectedBackTrans member, the latter into m_filteredInfo; we can do
		// this by searching for "\\bt " - the space will be present if collected back
		// translation informion is in this unwrapped string
		bool bIsOurBTMkr = FALSE;
		if (innerStr.Find(_T("\\bt ")) != wxNOT_FOUND)
		{
			bIsOurBTMkr = TRUE;
		}
		ParseMarkersAndContent(innerStr, mkr, content, endMkr);
		if (mkr == _T("\\free") || mkr == _T("\\note") || (mkr.Find(_T("\\bt")) != wxNOT_FOUND))
		{
			if (mkr == _T("\\free"))
			{
				strFreeTrans = content;
			}
			else if (mkr == _T("\\note"))
			{
				strNote = content;
			}
			else 
			{
				if (bIsOurBTMkr)
					strCollectedBackTrans = content;
				else
				{
					// it's a \bt=derived marker, such as one of \btv, \bth, \bts, etc
					strConcat += substring; // RHS is the string with filter marker wrappers still in place
				}
			}
		}
		else
		{
            // it's not one of the above 3 data types, so concatenate the substring to be
            // returned in strConcat to go in the caller into the member m_filteredInfo
			strConcat += substring;
		}
		// chop off the extracted substring from the info string, trim any whitespace at
		// the left, and then get new starting and ending offsets and iterate
		info = info.Mid(offsetToEnd);
		info.Trim(FALSE); // trim at left hand end (in doc version 4 there is usually a
						  // delimiting space needing to be chucked away here)
		strRemainder = info; // it's trimmed at left
		offsetToStart = info.Find(filterMkr);
		offsetToEnd = info.Find(filterMkrEnd);
	}
	return strConcat;
}










