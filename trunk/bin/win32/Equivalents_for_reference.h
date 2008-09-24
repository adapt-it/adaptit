/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			Equivalents_for_reference.h
/// \author			Bill Martin
/// \date_created	17 August 2004
/// \date_revised	29 December 2007
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is a header file containing MFC to wxWidgets equivalents. 
/// This file has no actual compilable code in it. It is added to the wxWidgets project
/// simply to make "Find in Files" act as a lookup for quickly seeing the wxWidgets
/// equivalents to MFC and standard C functions. The equivalents for the standard C
/// functions can be seen mostly in the #define statements in the copied wxchar.h part.
/////////////////////////////////////////////////////////////////////////////

// The last part of this file is taken verbatim from the wxWidgets library 
// file wxchar.h - a main header file in wxWidgets.

/*
// Put sample conversions here in this comment block (<<< indicates "converts to"):
wxChar ch; <<< TCHAR ch;
ch = bufferStr.GetChar(0); <<< ch = bufferStr.GetAt(0);

The following are event macro related conversions:
DECLARE_DYNAMIC_CLASS(CAdapt_ItDoc) <<< DECLARE_DYNCREATE(CAdapt_ItDoc) [in .h file]
IMPLEMENT_DYNAMIC_CLASS(CAdapt_ItDoc, wxDocument) <<< IMPLEMENT_DYNCREATE(CAdapt_ItDoc, CDocument) [in .cpp file]
DECLARE_EVENT_TABLE() <<< DECLARE_MESSAGE_MAP() [in .h file]
BEGIN_EVENT_TABLE(CAdapt_ItDoc, wxDocument) <<< BEGIN_MESSAGE_MAP(CAdapt_ItDoc, CDocument) [in .cpp file]
ON_COMMAND(ID_FILE_SAVE, OnFileSave) <<< ON_COMMAND(ID_FILE_SAVE, OnFileSave) Note: wx requires ClassName::HandlerName
EVT_UPDATE_UI(wxID_SAVE, CAdapt_ItDoc::OnUpdateFileSave) <<< ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave) Note: wx requires ClassName::HandlerName
END_EVENT_TABLE() <<< END_MESSAGE_MAP() [in .cpp file]

The following are string related conversions:
exportFilename.Remove(len-3,3); <<< exportFilename.Delete(len-3,3);
wxSnprintf(buff, 34, "%d", nCount); <<< _itot(nCount,buff,10); [Note: often easier to str << nCount; without buff in wx]
str.Replace(_T("*"),_T("")); <<< str.Remove(_T('*'));

The following are file dialog related conversions:
name = fn.GetFullName(); <<< name = fileDlg.GetFileName(); [file dialog methods]
ext = fn.GetExt(); <<< ext = fileDlg.GetFileExt(); [file dialog methods]
exportPath = fileDlg.GetPath(); <<< exportPath = dlg.GetPathName(); [file dialog methods]
MFC's GetPathName() and wxFileDialog.GetPath() both get whole dir + file name.


The following are font related conversions:
pFont->SetPointSize(fontSize); <<< srcLF.lfHeight = -PointSizeToLFHeight(fontSize); [MFC uses LOGFONT]
pFont->SetWeight(wxFONTWEIGHT_BOLD); <<< srcLF.lfWeight = FW_BOLD; [MFC uses LOGFONT]
pFont->SetWeight(wxFONTWEIGHT_NORMAL); <<< srcLF.lfWeight = FW_NORMAL; [MFC uses LOGFONT]
pFont->SetUnderlined(TRUE); <<< srcLF.lfUnderline = TRUE; [MFC uses LOGFONT]
pFont->SetUnderlined(FALSE); <<< srcLF.lfUnderline = FALSE; [MFC uses LOGFONT]
pFont->SetStyle(wxFONTSTYLE_ITALIC); <<< srcLF.lfUnderline = TRUE; [MFC uses LOGFONT]
pFont->SetStyle(wxFONTSTYLE_NORMAL); <<< srcLF.lfUnderline = FALSE; [MFC uses LOGFONT]


The following are file related conversions:
if (!f.Open(gpApp->m_curOutputFilename,wxFile::write)) <<< if (!f.Open(m_curOutputFilename,CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive, &e))
if (!::wxRenameFile(projectPath,renamedPath)) <<< f.Rename(projectPath,renamedPath);	
bOK = ::wxSetWorkingDirectory(dirPath); <<< bOK = ::SetCurrentDirectory(dirPath);
bExists = ::wxFileExists(path) && !::wxDirExists(path); <<< CFileStatus status; CFile f; f.GetStatus(path,status);
pFile->Write(oldText); <<< pFile->WriteString(oldText);
bool bOK = ::wxMkdir(path); <<< bool bOK = ::CreateDirectory(path,NULL);
if (!::wxRemoveFile(filePath)) <<< f.Remove(filePath); 

ReleaseMouse(); <<< ReleaseCapture();

pView->canvas->Freeze(); <<< pView->LockWindowUpdate();
pView->canvas->Thaw(); <<< pView->UnlockWindowUpdate();

int width = wxSystemSettings::GetMetric(wxSYS_SCREEN_X); <<< int width = ::GetSystemMetrics(SM_CXSCREEN);
::wxBell(); <<< MessageBeep(0);
pApp->m_bZoomed = pFrame->IsMaximized(); <<< pApp->zoomed = (int)pFrame->IsZoomed();

wxTimeSpan ts(0,mins,secs,0); <<< CTimeSpan ts(0,0,mins,secs); WARNING: MFC has (days,hours,minutes,seconds) wx has (hours,minutes,seconds,milliseconds)


The following are doc/view related conversions:
OnSaveModified() <<< SaveModified() [different names for same methods]
Modify(FALSE); <<< SetModifiedFlag(FALSE); [in the Doc]
pDoc->SetFilename(pathName,TRUE); <<< pDoc->SetPathName((LPCTSTR)pathName,TRUE);

The following are dialog related conversions:
if (fileDlg.ShowModal() != wxID_OK) <<< if (fileDlg.DoModal() != IDOK)
SetDefault(); <<< SetDefID(IDOK);

The following are display context related conversions:
dc.GetDeviceOrigin() <<< dc->GetWindowOrg()
canvas->SetVirtualSize(pApp->m_docSize); <<< pView->SetScrollSizes(MM_TEXT, pApp->m_docSize);
pDC->SetDeviceOrigin(0,0); <<< pDC->SetViewportOrg(0,0); [both can also use a point as parameter]
rectFrame = pFrame->GetRect(); <<< pFrame->GetWindowRect(&rectFrame);
pDC->SetLogicalOrigin(0, pOffsets->nTop); <<< pDC->SetWindowOrg(0, pOffsets->nTop); [SetLogicalOrigin only documented for wxPrintout but works with wxDC too]
pDC->GetSize(&sizViewPortExt.x,&sizViewPortExt.y); <<< CSize sizViewPortExt = pDC->GetViewportExt(); 
pDC->SetTextForeground(m_navColor); <<< pDC->SetTextColor(m_navColor);
dC.GetTextExtent(str,&StrSize.x,&StrSize.y); <<< StrSize = dC.GetTextExtent(str); [StrSize is a wxSize/CSize]
pDC->DrawText(str,rectBounding.GetLeft(),rectBounding.GetTop()); <<< pDC->DrawText(str,rectBounding,nFormat);
GetClientSize(&rectDlg.width, &rectDlg.height); <<< GetClientRect(&rectDlg);
SetSize(left,rectScreen.GetBottom()-dlgHeight-50,500,150,wxSIZE_USE_EXISTING); <<< SetWindowPos(&CWnd::wndTop,left,rectScreen.bottom-dlgHeight-50,500,150,SWP_NOSIZE);

int nLimit = GetScrollRange(wxVERTICAL); <<< int nLimit = GetScrollLimit(SB_VERT);

The following are list collections related conversions:
WX_DECLARE_LIST(CSourcePhrase, SPList); [this macro declares wxList in .h file - see WX_DEFINE_LIST][not in MFC]
WX_DEFINE_LIST(SPList); [this macro defines wxList in .cpp file - see WX_DECLARE_LIST][not in MFC]
SPList <<< CObList [list of CSourcePhrase pointers]
TUList <<< CObList [list of CTargetUnit pointers]
TranslationsList <<< CObList [list of CRefString pointers]
POList <<< CObList [list of PageOffsets pointers]
CCellList <<< CObList [list of CCell pointers]
KPlusCList <<< CObList [list of KeyPlusCount pointers]
AFList <<< CObList [list of AutoFixRecord pointers]
WordList <<< CObList [list of wxString pointers]
CellxNList <<< CObList [list of wxString pointers]
CellxNListFree <<< CObList [list of wxString pointers]
CellxNListBT <<< CObList [list of wxString pointers]
SrcList <<< CObList [list of wxString pointers]
TgtList <<< CObList [list of wxString pointers]
GlsList <<< CObList [list of wxString pointers]
NavList <<< CObList [list of wxString pointers]
FreeTransList <<< CObList [list of wxString pointers]
BackTransList <<< CObList [list of wxString pointers]
ChList <<< CObList [list of Chapter pointers]

The following are hash map collections related conversions:
WX_DECLARE_HASH_MAP( wxString,CTargetUnit*,wxStringHash,wxStringEqual,MapKeyStringToTgtUnit ); [declare wxHashMap in .h]
WX_DECLARE_HASH_MAP( wxString,wxString,wxStringHash,wxStringEqual,MapWholeMkrToFilterStatus ); [declare wxHashMap in .h]
WX_DECLARE_HASH_MAP( wxString,wxString,wxStringHash,wxStringEqual,MapBareMkrToRTFTags ); [declare wxHashMap in .h]
WX_DECLARE_HASH_MAP( wxString,wxString,wxStringHash,wxStringEqual,MapMkrToColorStr ); [declare wxHashMap in .h]
WX_DECLARE_HASH_MAP( wxString,CTargetUnit*,wxStringHash,wxStringEqual,MapKeyStringToTgtUnit ); [declare wxHashMap in .h]
MapSfmToUSFMAnalysisStruct* m_pUsfmStylesMap; <<< CMapStringToOb* m_pUsfmStylesMap; [see next lines] [maps sfm string to pointer to USFMAnalysis struct]
  [WX uses: iter->first and (wxStrint)iter->second to retrieve the mapped associations] <<< CMapStringToOb::CPair* pPair;
  MapSfmToUSFMAnalysisStruct::iterator iter; <<< [MFC uses methods of the CMapStringToOb like PGetFirstAssoc(), PGetNextAssoc(), etc.]
MapWholeMkrToFilterStatus m_FilterStatusMap; <<< CMapStringToString* pMap; [see next lines] [maps a WholeMkr string to a FilterStatus string of "1" or "0"]
  [WX uses: iter->first and (wxStrint)iter->second to retrieve the mapped associations] <<< CMapStringToString::CPair* pPair;
  MapWholeMkrToFilterStatus::iterator iter; <<< [MFC uses methods of the CMapStringToString like PGetFirstAssoc(), PGetNextAssoc(), etc.]
  ++iter <<< pPair = pMap->PGetNextAssoc(pPair);
MapBareMkrToRTFTags rtfTagsMap; <<< std::map <CString, CString> rtfTagsMap; [see next two lines] [maps a bare sfm marker to a string of RTF tags]
  [WX uses: rtfIter->first and (wxStrint)rtfIter->second to retrieve the mapped associations] <<< typedef std::pair <const CString, CString> MkrTagStr_Pair; [declares the association pair]
  MapBareMkrToRTFTags::iterator rtfIter; <<< std::map <CString, CString> :: const_iterator rtfIter; [declared the iterator for the map]
MapMkrToColorStr colorMap; <<< CMapStringToString colorMap; [see next lines] [maps a whole sfm marker to a color string value]
  MapMkrToColorStr::iterator citer; <<< [MFC uses methods of the CMapStringToString like PGetFirstAssoc(), PGetNextAssoc(), etc.]
MapKeyStringToTgtUnit* m_pMap[10]; <<< CMapStringToOb* m_pMap[10]
  [WX uses: iter->first and (wxStrint)iter->second to retrieve the mapped associations] <<< CMapStringToOb::CPair* pPair;
  MapKeyStringToTgtUnit::iterator iter; <<< [MFC uses methods of the CMapStringToOb like PGetFirstAssoc(), PGetNextAssoc(), etc.]

SPList or TranslationsList or TUList [special typed wxList objects used] <<< CObList 
wxList::Clear() <<< CObList::RemoveAll()
PSrcPhrase = pos->GetData(); pos = pos->GetNext() <<< pSrcPhrase = (CSourcePhrase*)pList->GetNext(pos);
SPList::Node* pos ... pos->GetData(); pos = pos->GetNext() <<< CObList::GetNext() [see note below]
   Note: The MFC GetNext(pos) method of CObList does two things at once, it is equivalent to first 
         calling wxList's GetData() then calling the node's pos->GetNext() to move the node 
		 pointer/iterator forward one node.
fpos = pNewList->GetFirst(); pNewSrcPhrase = fpos->GetData(); <<< pNewSrcPhrase = (CSourcePhrase*)pNewList->GetHead();
CSourcePhrase* pSP = pos->GetData(); pos = pos->GetPrevious; <<< CSourcePhrase* pSP = (CSourcePhrase*)pSaved->GetPrev(pos);
SPList::Node* pos = pList->Item(nFirstSequNum) <<< POSITION pos = pList->FindIndex(nFirstSequNum);
CSourcePhrase* pSrcPhrase = pos->GetData();  <<< CSourcePhrase* pSrcPhrase = (CSourcePhrase*)pList->GetAt(pos);
POSITION pos = pSrcPhrases->GetHeadPosition()  <<< SPList::Node* pos1 = pPhrases->GetFirst();
[see example and notes in the View's RestoreOriginalMinPhrases() function] <<< pList->InsertAfter(savePos,pSP)
pList->DeleteNode(pos); <<< pList->RemoveAt(pos);
pList->Append(pSrcPhrase); <<<  pList->AddTail(pSrcPhrase);
for (SPList::Node *node = pAddList->GetFirst(); node; node = node->GetNext()) {pList->Append(node->GetData())} <<< pList->AddTail(pAddList); [must append pAddList to pList manually]
pList->Insert(pSrcPhrase); [Insert always adds to head of list] <<< pList->AddHead(pSrcPhrase);
pList->Insert(pos,pSrcPhr); <<< pList->InsertBefore(pos,pSrcPhr);
pApp->m_selection.Insert(pPrevCell); <<< pApp->m_selection.AddHead(pPrevCell);
SPList::Node* pos1 = pOtherList->Find(pSP);  <<< POSITION pos1 = pOtherList->Find(pSP);

wxStringTokenizer tkz(wholeMkr); [see context] <<< wholeMkr = str.Tokenize(_T(" "),curPos);

m_pTranslations [wx uses pointer to TranslationsList] <<< m_translations [MFC uses actual CObList]

CAdapt_ItApp* pApp = &wxGetApp(); <<< CAdapt_ItApp* pApp = (CAdapt_ItApp*)AfxGetApp();

tempStr << n; OR wxSprintf(numStr,_T("%d"),n); <<< itoa(n,numStr,10); [wx can use wxSprintf or overloaded << operator]
nVal = wxAtoi(strTemp); <<< nVal = _ttoi(strTemp);

wxMessageBox(errStr, _T(""), wxICON_STOP); [also wxICON_ERROR] <<< AfxMessageBox(errStr,MB_ICONSTOP);
wxMessageBox(errStr, _T(""), wxICON_INFORMATION); <<< AfxMessageBox(errStr,MB_ICONINFORMATION);
wxMessageBox(errStr, _T(""), wxICON_EXCLAMATION); <<< AfxMessageBox(errStr,MB_ICONEXCLAMATION);
wxMessageBox(errStr, _T(""), wxICON_WARNING); <<< AfxMessageBox(errStr,MB_ICONWARNING);
if( wxMessageBox(Qmsg, _T(""), wxYES_NO | wxICON_INFORMATION) == wxNO) <<< if( AfxMessageBox(Qmsg, MB_YESNO | MB_ICONINFORMATION) == IDNO)
if( wxMessageBox(Qmsg, _T(""), wxYES_NO | wxICON_INFORMATION) == wxYES) <<< if( AfxMessageBox(Qmsg, MB_YESNO | MB_ICONINFORMATION) == IDYES)
if ( wxMessageBox(Qmsg, _T(""), wxICON_QUESTION | wxYES_NO) != wxYES ) <<< if( AfxMessageBox(msg,MB_YESNO | MB_ICONINFORMATION) != IDYES)
int nResult = wxMessageBox(msg, _T(""), wxOK | wxCANCEL) <<< int nResult = AfxMessageBox(msg,MB_OKCANCEL)

wxDateTime theTime = wxDateTime::Now(); <<< CTime theTime(CTime::GetCurrentTime());

if (rect.Contains(point)) <<< if (rect.PtInRect(point))













*/


/*
// beginning of wxchar.h
//
// * Name:        wx/wxchar.h
// * Purpose:     Declarations common to wx char/wchar_t usage (wide chars)
// * Author:      Joel Farley, Ove Kåven
// * Modified by: Vadim Zeitlin, Robert Roebling, Ron Lee
// * Created:     1998/06/12
// * RCS-ID:      $Id: Equivalents_for_reference.h,v 1.13 2008-08-01 19:16:32 billm Exp $
// * Copyright:   (c) 1998-2002 Joel Farley, Ove Kåven, Robert Roebling, Ron Lee
// * Licence:     wxWidgets licence
// 
//
// THIS IS A C FILE, DON'T USE C++ FEATURES (IN PARTICULAR COMMENTS) IN IT 
//
//#ifndef _WX_WXCHAR_H_
//#define _WX_WXCHAR_H_
//
//#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
//    #pragma interface "wxchar.h"
//#endif
//
// defs.h indirectly includes this file, so don't include it here 
//#include "wx/platform.h"
//#include "wx/dlimpexp.h"
//
//#if defined(HAVE_STRTOK_R) && defined(__DARWIN__) && defined(_MSL_USING_MW_C_HEADERS) && _MSL_USING_MW_C_HEADERS
//    char *strtok_r(char *, const char *, char **);
//#endif
//
// check whether we have wchar_t and which size it is if we do 
//#if !defined(wxUSE_WCHAR_T)
//    #if defined(__UNIX__)
//        #if defined(HAVE_WCSTR_H) || defined(HAVE_WCHAR_H) || defined(__FreeBSD__) || defined(__DARWIN__)
//            #define wxUSE_WCHAR_T 1
//        #else
//            #define wxUSE_WCHAR_T 0
//        #endif
//    #elif defined(__GNUWIN32__) && !defined(__MINGW32__)
//        #define wxUSE_WCHAR_T 0
//    #elif defined(__WATCOMC__)
//        #define wxUSE_WCHAR_T 0
//    #elif defined(__VISAGECPP__) && (__IBMCPP__ < 400)
//        #define wxUSE_WCHAR_T 0
//    #else
//         add additional compiler checks if this fails 
//        #define wxUSE_WCHAR_T 1
//    #endif
//#endif  !defined(wxUSE_WCHAR_T) 
//
// Unicode support requires wchar_t 
//#if wxUSE_UNICODE && !wxUSE_WCHAR_T
//    #error "wchar_t must be available in Unicode build"
//#endif  Unicode 
//
//
//    Standard headers we need here.
//
//    NB: don't include any wxWidgets headers here because almost all of them include
//        this one!
// 
//
// Required for wxPrintf() etc 
//#include <stdarg.h>
//
// Almost all compiler have strdup(), but not quite all: CodeWarrior under Mac 
// and VC++ for Windows CE don't provide it 
//#if defined(__VISUALC__) && __VISUALC__ >= 1400
//    #define wxStrdupA _strdup
//#elif !(defined(__MWERKS__) && defined(__WXMAC__)) && !defined(__WXWINCE__)
//     use #define, not inline wrapper, as it is tested with #ifndef below 
//    #define wxStrdupA strdup
//#endif
//
//
//   non Unix compilers which do have wchar.h (but not tchar.h which is included
//   below and which includes wchar.h anyhow).
//
//   Actually MinGW has tchar.h, but it does not include wchar.h
// 
//#if defined(__MWERKS__) || defined(__VISAGECPP__) || defined(__MINGW32__) || defined(__WATCOMC__)
//    #ifndef HAVE_WCHAR_H
//        #define HAVE_WCHAR_H
//    #endif
//#endif
//#if defined(__MWERKS__) && !defined(__MACH__)
//    #ifndef HAVE_WCSLEN
//        #define HAVE_WCSLEN
//    #endif
//#endif
//
//#if wxUSE_WCHAR_T
//    #ifdef HAVE_WCHAR_H
//         the current (as of Nov 2002) version of cygwin has a bug in its 
//         wchar.h -- there is no extern "C" around the declarations in it and 
//         this results in linking errors later; also, at least on some 
//         Cygwin versions, wchar.h requires sys/types.h 
//        #ifdef __CYGWIN__
//            #include <sys/types.h>
//            #ifdef __cplusplus
//                extern "C" {
//            #endif
//        #endif  Cygwin 
//
//        #include <wchar.h>
//
//        #if defined(__CYGWIN__) && defined(__cplusplus)
//            }
//        #endif  Cygwin and C++ 
//
//    #elif defined(HAVE_WCSTR_H)
//         old compilers have relevant declarations here 
//        #include <wcstr.h>
//    #elif defined(__FreeBSD__) || defined(__DARWIN__) || defined(__EMX__)
//         include stdlib.h for wchar_t 
//        #include <stdlib.h>
//    #endif  HAVE_WCHAR_H 
//#endif  wxUSE_WCHAR_T 
//
// ---------------------------------------------------------------------------- 
// define wxHAVE_TCHAR_SUPPORT for the compilers which support the TCHAR type 
// mapped to either char or wchar_t depending on the ASCII/Unicode mode and have 
// the function mapping _tfoo() -> foo() or wfoo() 
// ---------------------------------------------------------------------------- 
//
// VC++ and BC++ starting with 5.2 have TCHAR support 
//#ifdef __VISUALC__
//    #define wxHAVE_TCHAR_SUPPORT
//#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x520)
//    #define wxHAVE_TCHAR_SUPPORT
//    #include <ctype.h>
//#elif defined(__WATCOMC__)
//    #define wxHAVE_TCHAR_SUPPORT
//#elif defined(__DMC__)
//    #define wxHAVE_TCHAR_SUPPORT
//#elif defined(__WXPALMOS__)
//    #include <stddef.h>
//#elif defined(__MINGW32__) && wxCHECK_W32API_VERSION( 1, 0 )
//    #define wxHAVE_TCHAR_SUPPORT
//    #include <stddef.h>
//    #include <string.h>
//    #include <ctype.h>
//#elif 0 && defined(__VISAGECPP__) && (__IBMCPP__ >= 400)
//     VZ: the old VisualAge definitions were completely wrong and had no 
//         chance at all to work in Unicode build anyhow so let's pretend that 
//         VisualAge does _not_ support TCHAR for the moment (as indicated by 
//         "0 &&" above) until someone really has time to delve into Unicode 
//         issues under OS/2 
//
//     VisualAge 4.0+ supports TCHAR 
//    #define wxHAVE_TCHAR_SUPPORT
//#endif  compilers with (good) TCHAR support 
//
//#ifdef __MWERKS__
//    #define HAVE_WPRINTF
//#endif
//
//#ifdef wxHAVE_TCHAR_SUPPORT
//     get TCHAR definition if we've got it 
//    #include <tchar.h>
//
//     we surely do have wchar_t if we have TCHAR 
//    #ifndef wxUSE_WCHAR_T
//        #define wxUSE_WCHAR_T 1
//    #endif  !defined(wxUSE_WCHAR_T) 
//
//     and we also do have wcslen() 
//    #ifndef HAVE_WCSLEN
//        #define HAVE_WCSLEN
//    #endif
//#endif  wxHAVE_TCHAR_SUPPORT 
//
// ---------------------------------------------------------------------------- 
// define wxChar type 
// ---------------------------------------------------------------------------- 
//
// TODO: define wxCharInt to be equal to either int or wint_t? 
//
//#if !wxUSE_UNICODE
//    typedef char wxChar;
//    typedef signed char wxSChar;
//    typedef unsigned char wxUChar;
//#else  Unicode 
//     VZ: note that VC++ defines _T[SU]CHAR simply as wchar_t and not as 
//         signed/unsigned version of it which (a) makes sense to me (unlike 
//         char wchar_t is always unsigned) and (b) was how the previous 
//         definitions worked so keep it like this 
//
//     Sun's SunPro compiler supports the wchar_t type and wide character 
//     functions, but does not define __WCHAR_TYPE__. Define it here to 
//     allow unicode enabled builds. 
//    #if defined(__SUNPRO_CC) || defined(__SUNPRO_C)
//    #define __WCHAR_TYPE__ wxchar_t
//    #endif
//
//     GNU libc has __WCHAR_TYPE__ which requires special treatment, see 
//     comment below 
//    #if !defined(__WCHAR_TYPE__) || \
//        (!defined(__GNUC__) || wxCHECK_GCC_VERSION(2, 96))
//         standard case 
//        typedef wchar_t wxChar;
//        typedef wchar_t wxSChar;
//        typedef wchar_t wxUChar;
//    #else  __WCHAR_TYPE__ and gcc < 2.96 
//         VS: wxWidgets used to define wxChar as __WCHAR_TYPE__ here. However, 
//             this doesn't work with new GCC 3.x compilers because wchar_t is 
//             C++'s builtin type in the new standard. OTOH, old compilers (GCC 
//             2.x) won't accept new definition of wx{S,U}Char, therefore we 
//             have to define wxChar conditionally depending on detected 
//             compiler & compiler version. 
//             with old definition of wxChar. 
//        typedef __WCHAR_TYPE__ wxChar;
//        typedef __WCHAR_TYPE__ wxSChar;
//        typedef __WCHAR_TYPE__ wxUChar;
//    #endif  __WCHAR_TYPE__ 
//#endif  ASCII/Unicode 
//
// ---------------------------------------------------------------------------- 
// define _T() and related macros 
// ---------------------------------------------------------------------------- 
//
// BSD systems define _T() to be something different in ctype.h, override it 
//#if defined(__FreeBSD__) || defined(__DARWIN__)
//    #include <ctype.h>
//    #undef _T
//#endif
//
// could already be defined by tchar.h (it's quasi standard) 
//#ifndef _T
//    #if !wxUSE_UNICODE
//        #define _T(x) x
//    #else  Unicode 
//        #define _T(x) L ## x
//    #endif  ASCII/Unicode 
//#endif  !defined(_T) 
//
// although global macros with such names are normally bad, we want to have 
// another name for _T() which should be used to avoid confusion between _T() 
// and _() in wxWidgets sources 
//#define wxT(x)       _T(x)
//
// Unicode-friendly __FILE__, __DATE__ and __TIME__ analogs 
//#ifndef __TFILE__
//    #define __XFILE__(x) wxT(x)
//    #define __TFILE__ __XFILE__(__FILE__)
//#endif
//
//#ifndef __TDATE__
//    #define __XDATE__(x) wxT(x)
//    #define __TDATE__ __XDATE__(__DATE__)
//#endif
//
//#ifndef __TTIME__
//    #define __XTIME__(x) wxT(x)
//    #define __TTIME__ __XTIME__(__TIME__)
//#endif
//
//
//    define wxFoo() function for each standard foo() function whose signature
//    (exceptionally including the return type) includes any mention of char:
//    wxFoo() is going to be a Unicode-friendly version of foo(), i.e. will have
//    the same signature but with char replaced by wxChar which allows us to use
//    it in Unicode build as well
// 
//
//#ifdef wxHAVE_TCHAR_SUPPORT
//    #include <ctype.h>
//
//    #if defined(__WATCOMC__) && defined(UNICODE)
//      #define WXWCHAR_T_CAST(c) (wint_t)(c)
//    #else
//      #define WXWCHAR_T_CAST(c) c
//    #endif
//
//     ctype.h functions 
//    #define  wxIsalnum(c)   _istalnum(WXWCHAR_T_CAST(c))
//    #define  wxIsalpha(c)   _istalpha(WXWCHAR_T_CAST(c))
//    #define  wxIscntrl(c)   _istcntrl(WXWCHAR_T_CAST(c))
//    #define  wxIsdigit(c)   _istdigit(WXWCHAR_T_CAST(c))
//    #define  wxIsgraph(c)   _istgraph(WXWCHAR_T_CAST(c))
//    #define  wxIslower(c)   _istlower(WXWCHAR_T_CAST(c))
//    #define  wxIsprint(c)   _istprint(WXWCHAR_T_CAST(c))
//    #define  wxIspunct(c)   _istpunct(WXWCHAR_T_CAST(c))
//    #define  wxIsspace(c)   _istspace(WXWCHAR_T_CAST(c))
//    #define  wxIsupper(c)   _istupper(WXWCHAR_T_CAST(c))
//    #define  wxIsxdigit(c)  _istxdigit(WXWCHAR_T_CAST(c))
//
//    
//       There is a bug in VC6 C RTL: toxxx() functions dosn't do anything with
//       signed chars < 0, so "fix" it here.
//     
//    #define  wxTolower(c) _totlower((wxUChar)(c))
//    #define  wxToupper(c) _totupper((wxUChar)(c))
//
//     locale.h functons 
//    #define  wxSetlocale _tsetlocale
//
//     string.h functions 
//    #define  wxStrcat    _tcscat
//    #define  wxStrchr    _tcschr
//    #define  wxStrcmp    _tcscmp
//    #define  wxStrcoll   _tcscoll
//    #define  wxStrcpy    _tcscpy
//    #define  wxStrcspn   _tcscspn
//    #define  wxStrdupW   _wcsdup         notice the 'W'! 
//    #define  wxStrftime  _tcsftime
//    #define  wxStricmp   _tcsicmp
//    #define  wxStrnicmp  _tcsnicmp
//    #define  wxStrlen_   _tcslen         used in wxStrlen inline function 
//    #define  wxStrncat   _tcsncat
//    #define  wxStrncmp   _tcsncmp
//    #define  wxStrncpy   _tcsncpy
//    #define  wxStrpbrk   _tcspbrk
//    #define  wxStrrchr   _tcsrchr
//    #define  wxStrspn    _tcsspn
//    #define  wxStrstr    _tcsstr
//    #define  wxStrtod    _tcstod
//    #define  wxStrtol    _tcstol
//    #define  wxStrtoul   _tcstoul
//    #define  wxStrxfrm   _tcsxfrm
//
//     stdio.h functions 
//    #define  wxFgetc     _fgettc
//    #define  wxFgetchar  _fgettchar
//    #define  wxFgets     _fgetts
//    #if wxUSE_UNICODE_MSLU
//        #define  wxFopen    wxMSLU__tfopen
//    #else
//        #define  wxFopen     _tfopen
//    #endif
//    #define  wxFputc     _fputtc
//    #define  wxFputchar  _fputtchar
//    #define  wxFprintf   _ftprintf
//    #define  wxFputs     _fputts
//    #define  wxFreopen   _tfreopen
//    #define  wxFscanf    _ftscanf
//    #define  wxGetc      _gettc
//    #define  wxGetchar   _gettchar
//    #define  wxGets      _getts
//    #define  wxPerror    _tperror
//    #define  wxPrintf    _tprintf
//    #define  wxPutc(c,f) _puttc(WXWCHAR_T_CAST(c),f)
//    #define  wxPutchar   _puttchar
//    #define  wxPuts      _putts
//    #define  wxScanf     _tscanf
//    #if defined(__DMC__)
//        #if wxUSE_UNICODE
//             Digital Mars adds count to _stprintf (C99) so prototype conversion see wxchar.cpp 
//            int wxSprintf (wchar_t * __RESTRICT s, const wchar_t * __RESTRICT format, ... ) ;
//        #else
//             and there is a bug in D Mars tchar.h prior to 8.39.4n, so define as sprintf 
//            #define wxSprintf sprintf
//        #endif
//    #else
//        #define  wxSprintf   _stprintf
//    #endif
//
//    #define  wxSscanf    _stscanf
//    #define  wxTmpnam    _ttmpnam
//    #define  wxUngetc    _tungetc
//    #define  wxVfprintf  _vftprintf
//    #define  wxVprintf   _vtprintf
//    #define  wxVsscanf   _vstscanf
//    #define  wxVsprintf  _vstprintf
//
//     special case: not all TCHAR-aware compilers have those 
//    #if defined(__VISUALC__) || \
//            (defined(__BORLANDC__) && __BORLANDC__ >= 0x540)
//        #define wxVsnprintf_    _vsntprintf
//        #define wxSnprintf_     _sntprintf
//    #endif
//
//     special case: these functions are missing under Win9x with Unicows so we 
//     have to implement them ourselves 
//    #if wxUSE_UNICODE_MSLU
//        #define  wxRemove    wxMSLU__tremove
//        #define  wxRename    wxMSLU__trename
//    #else
//        #ifdef __WXWINCE__
//             carefully: wxRemove() must return 0 on success while DeleteFile()
//               returns 0 on error, so don't just define one as the other 
//            int wxRemove(const wxChar *path);
//        #else
//            #define  wxRemove    _tremove
//            #define  wxRename    _trename
//        #endif
//    #endif
//
//     stdlib.h functions 
//    #define  wxAtoi      _ttoi
//    #define  wxAtol      _ttol
//     #define  wxAtof   _tttof -- notice that there is no such thing (why?) 
//     there are no env vars at all under CE, so no _tgetenv neither 
//    #ifdef __WXWINCE__
//         can't define as inline function as this is a C file... 
//        #define wxGetenv(name)  ((wxChar *)NULL)
//    #else
//        #define  wxGetenv    _tgetenv
//    #endif
//    #define  wxSystem    _tsystem
//
//     time.h functions 
//    #define  wxAsctime   _tasctime
//    #define  wxCtime     _tctime
//
//    #define wxMbstowcs mbstowcs
//    #define wxWcstombs wcstombs
//#else  !TCHAR-aware compilers 
//
//    
//        There are 2 unrelated problems with these functions under Mac:
//            a) Metrowerks MSL CRT implements them strictly in C99 sense and
//               doesn't support (very common) extension of allowing to call
//               mbstowcs(NULL, ...) which makes it pretty useless as you can't
//               know the size of the needed buffer
//            b) OS X <= 10.2 declares and even defined these functions but
//               doesn't really implement them -- they always return an error
//
//        So use our own replacements in both cases.
//     
//    #if defined(__MWERKS__) && defined(__MSL__)
//        #define wxNEED_WX_MBSTOWCS
//    #endif
//
//    #ifdef __DARWIN__
//        #if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_2
//            #define wxNEED_WX_MBSTOWCS
//        #endif
//    #endif
//
//    #ifdef wxNEED_WX_MBSTOWCS
//         even though they are defined and "implemented", they are bad and just
//           stubs so we need our own - we need these even in ANSI builds!! 
//        WXDLLIMPEXP_BASE size_t wxMbstowcs (wchar_t *, const char *, size_t);
//        WXDLLIMPEXP_BASE size_t wxWcstombs (char *, const wchar_t *, size_t);
//    #else
//        #define wxMbstowcs mbstowcs
//        #define wxWcstombs wcstombs
//    #endif
//
//     No UNICODE in the c library except wchar_t typedef on mac OSX 10.2 and less - roll our own 
//    #if !defined(__MWERKS__) && wxUSE_UNICODE && defined(__DARWIN__) && ( MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_2 )
//
//         we need everything! 
//        #define wxNEED_WX_STRING_H
//        #define wxNEED_WX_CTYPE_H
//
//        #define  wxFgetchar(c)  wxFgetc(c, stdin)
//        #define  wxFputc     wxPutc
//        #define  wxFputchar(c)  wxPutc(c, stdout)
//        #define  wxGetc      wxFgetc
//        #define  wxGetchar(c)   wxFgetc(c, stdin)
//
//        #include <stdio.h>
//
//        #define wxNEED_FGETC
//        #define wxNEED_FGETS
//        #define wxNEED_GETS
//        #define wxNEED_UNGETC
//
//        #define wxNEED_FPUTS
//        #define wxNEED_PUTS
//        #define wxNEED_PUTC
//
//        int wxFputs(const wxChar *ch, FILE *stream);
//        int wxPuts(const wxChar *ws);
//        int wxPutc(wxChar ch, FILE *stream);
//
//        #ifdef __cplusplus
//        extern "C" {
//        #endif
//            WXDLLIMPEXP_BASE size_t   wxStrlen_(const wxChar *s);
//        #ifdef __cplusplus
//        }
//        #endif
//
//        #define wxPutchar(wch) wxPutc(wch, stdout)
//
//        #define wxNEED_PRINTF_CONVERSION
//        #define wxNEED_WX_STDIO_H
//        #define wxNEED_WX_STDLIB_H
//        #define wxNEED_WX_TIME_H
//
//    #elif wxUSE_UNICODE
//        #include <wctype.h>
//
//         this is probably glibc-specific 
//        #if defined(__WCHAR_TYPE__) && !defined(__MWERKS__)
//             ctype.h functions (wctype.h) 
//            #define  wxIsalnum   iswalnum
//            #define  wxIsalpha   iswalpha
//            #define  wxIscntrl   iswcntrl
//            #define  wxIsdigit   iswdigit
//            #define  wxIsgraph   iswgraph
//            #define  wxIslower   iswlower
//            #define  wxIsprint   iswprint
//            #define  wxIspunct   iswpunct
//            #define  wxIsspace   iswspace
//            #define  wxIsupper   iswupper
//            #define  wxIsxdigit  iswxdigit
//
//            #if defined(__GLIBC__) && (__GLIBC__ == 2) && (__GLIBC_MINOR__ == 0)
//                 /usr/include/wctype.h incorrectly declares translations 
//                 tables which provokes tons of compile-time warnings -- try 
//                 to correct this 
//                #define  wxTolower(wc)   towctrans((wc), (wctrans_t)__ctype_tolower)
//                #define  wxToupper(wc)   towctrans((wc), (wctrans_t)__ctype_toupper)
//            #else  !glibc 2.0 
//                #define  wxTolower   towlower
//                #define  wxToupper   towupper
//            #endif  gcc/!gcc 
//
//             string.h functions (wchar.h) 
//            #define  wxStrcat    wcscat
//            #define  wxStrchr    wcschr
//            #define  wxStrcmp    wcscmp
//            #define  wxStrcoll   wcscoll
//            #define  wxStrcpy    wcscpy
//            #define  wxStrcspn   wcscspn
//            #define  wxStrlen_   wxWcslen  wxStrlen_() is used in wxStrlen() 
//            #define  wxStrncat   wcsncat
//            #define  wxStrncmp   wcsncmp
//            #define  wxStrncpy   wcsncpy
//            #define  wxStrpbrk   wcspbrk
//            #define  wxStrrchr   wcsrchr
//            #define  wxStrspn    wcsspn
//            #define  wxStrstr    wcsstr
//            #define  wxStrtod    wcstod
//            #define  wxStrtol    wcstol
//            #define  wxStrtoul   wcstoul
//            #define  wxStrxfrm   wcsxfrm
//
//            #define  wxFgetc     fgetwc
//            #define  wxFgetchar  fgetwchar
//            #define  wxFgets     fgetws
//            #define  wxFputc     fputwc
//            #define  wxFputchar  fputwchar
//            #define  wxGetc      getwc
//            #define  wxGetchar   getwchar
//            #define  wxGets      getws
//            #define  wxUngetc    ungetwc
//
//            #ifdef HAVE_FPUTWS
//                #define wxFputs     fputws
//            #else
//                #define wxNEED_FPUTS
//                #include <stdio.h>
//                int wxFputs(const wxChar *ch, FILE *stream);
//            #endif
//
//            #ifdef HAVE_WPUTC
//                #define wxPutc      wputc
//            #else
//                #define wxNEED_PUTC
//                #include <stdio.h>
//                int wxPutc(wxChar ch, FILE *stream);
//            #endif
//
//            #ifdef HAVE_WPUTCHAR
//                #define wxPutchar   wputchar
//            #else
//                #define wxPutchar(wch) wxPutc(wch, stdout)
//            #endif
//
//            #ifdef HAVE_PUTWS
//                #define wxPuts      putws
//            #else
//                #define wxNEED_PUTS
//                int wxPuts(const wxChar *ws);
//            #endif
//
//             we need %s to %ls conversion for printf and scanf etc 
//            #define wxNEED_PRINTF_CONVERSION
//
//             glibc doesn't have wide char equivalents of the other stuff so 
//             use our own versions 
//            #define wxNEED_WX_STDIO_H
//            #define wxNEED_WX_STDLIB_H
//            #define wxNEED_WX_TIME_H
//        #elif defined(__MWERKS__) && ( defined(__MSL__) || defined(__MACH__) )
//             ctype.h functions (wctype.h) 
//            #define  wxIsalnum   iswalnum
//            #define  wxIsalpha   iswalpha
//            #define  wxIscntrl   iswcntrl
//            #define  wxIsdigit   iswdigit
//            #define  wxIsgraph   iswgraph
//            #define  wxIslower   iswlower
//            #define  wxIsprint   iswprint
//            #define  wxIspunct   iswpunct
//            #define  wxIsspace   iswspace
//            #define  wxIsupper   iswupper
//            #define  wxIsxdigit  iswxdigit
//            #define  wxTolower   towlower
//            #define  wxToupper   towupper
//
//             string.h functions (wchar.h) 
//            #define  wxStrcat    wcscat
//            #define  wxStrchr    wcschr
//            #define  wxStrcmp    wcscmp
//            #define  wxStrcoll   wcscoll
//            #define  wxStrcpy    wcscpy
//            #define  wxStrcspn   wcscspn
//            #define  wxStrlen_   wxWcslen  wxStrlen_() is used in wxStrlen() 
//            #define  wxStrncat   wcsncat
//            #define  wxStrncmp   wcsncmp
//            #define  wxStrncpy   wcsncpy
//            #define  wxStrpbrk   wcspbrk
//            #define  wxStrrchr   wcsrchr
//            #define  wxStrspn    wcsspn
//            #define  wxStrstr    wcsstr
//            #define  wxStrtod    wcstod
//            #define  wxStrtol    wcstol
//            #define  wxStrtoul   wcstoul
//            #define  wxStrxfrm   wcsxfrm
//
//            #define  wxFgetc     fgetwc
//            #define  wxFgetchar  fgetwchar
//            #define  wxFgets     fgetws
//            #define  wxFputc     fputwc
//            #define  wxFputchar  fputwchar
//            #define  wxGetc      getwc
//            #define  wxGetchar   getwchar
//            #define  wxGets      getws
//            #define  wxUngetc    ungetwc
//
//            #define wxNEED_PRINTF_CONVERSION
//
//            #define  wxPutc      putwc
//            #define  wxPutchar   putwchar
//            #define  wxFputs     fputws
//
//             stdio.h functions 
//
//            #define wxNEED_WX_STDIO_H
//
//             stdlib.h functions 
//            #ifdef __MACH__
//            #define wxNEED_WX_STDLIB_H
//            #else
//            #define  wxAtof      watof
//            #define  wxAtoi      watoi
//            #define  wxAtol      watol
//            #define  wxGetenv(a)    ((wxChar*)NULL)
//            #define  wxSystem(a)    ((int)NULL)
//            #endif
//             time.h functions 
//            #define  wxAsctime   wasciitime
//            #define  wxCtime     wctime
//             #define  wxStrftime  wcsftime 
//
//            #define wxNEED_WX_TIME_H
//        #else  !metrowerks for apple 
//            #error  "Please define wide character functions for your environment"
//        #endif
//    #else  ASCII 
//        #include <ctype.h>
//        #include <string.h>
//
//         ctype.h functions 
//        #define  wxIsalnum   isalnum
//        #define  wxIsalpha   isalpha
//        #define  wxIscntrl   iscntrl
//        #define  wxIsdigit   isdigit
//        #define  wxIsgraph   isgraph
//        #define  wxIslower   islower
//        #define  wxIsprint   isprint
//        #define  wxIspunct   ispunct
//        #define  wxIsspace   isspace
//        #define  wxIsupper   isupper
//        #define  wxIsxdigit  isxdigit
//        #define  wxTolower   tolower
//        #define  wxToupper   toupper
//
//          locale.h functons 
//        #define  wxSetlocale setlocale
//
//          string.h functions 
//        #define  wxStrcat    strcat
//        #define  wxStrchr    strchr
//        #define  wxStrcmp    strcmp
//        #define  wxStrcoll   strcoll
//        #define  wxStrcpy    strcpy
//        #define  wxStrcspn   strcspn
//
//         wxStricmp and wxStrnicmp are defined below 
//        #define  wxStrlen_   strlen  used in wxStrlen inline function 
//        #define  wxStrncat   strncat
//        #define  wxStrncmp   strncmp
//        #define  wxStrncpy   strncpy
//        #define  wxStrpbrk   strpbrk
//        #define  wxStrrchr   strrchr
//        #define  wxStrspn    strspn
//        #define  wxStrstr    strstr
//        #define  wxStrtod    strtod
//        #ifdef HAVE_STRTOK_R
//            #define  wxStrtok(str, sep, last)    strtok_r(str, sep, last)
//        #endif
//        #define  wxStrtol    strtol
//        #define  wxStrtoul   strtoul
//        #define  wxStrxfrm   strxfrm
//
//         stdio.h functions 
//        #define  wxFopen     fopen
//        #define  wxFreopen   freopen
//        #define  wxRemove    remove
//        #define  wxRename    rename
//
//        #define  wxPerror    perror
//        #define  wxTmpnam    tmpnam
//
//        #define  wxFgetc     fgetc
//        #define  wxFgetchar  fgetchar
//        #define  wxFgets     fgets
//        #define  wxFputc     fputc
//        #define  wxFputs     fputs
//        #define  wxFputchar  fputchar
//        #define  wxFprintf   fprintf
//        #define  wxFscanf    fscanf
//        #define  wxGetc      getc
//        #define  wxGetchar   getchar
//        #define  wxGets      gets
//        #define  wxPrintf    printf
//        #define  wxPutc      putc
//        #define  wxPutchar   putchar
//        #define  wxPuts      puts
//        #define  wxScanf     scanf
//        #define  wxSprintf   sprintf
//        #define  wxSscanf    sscanf
//        #define  wxUngetc    ungetc
//        #define  wxVfprintf  vfprintf
//        #define  wxVprintf   vprintf
//        #define  wxVsscanf   vsscanf
//        #define  wxVsprintf  vsprintf
//
//         stdlib.h functions 
//        #define  wxAtof      atof
//        #define  wxAtoi      atoi
//        #define  wxAtol      atol
//        #define  wxGetenv    getenv
//        #define  wxSystem    system
//
//         time.h functions 
//        #define  wxAsctime   asctime
//        #define  wxCtime     ctime
//        #define  wxStrftime  strftime
//    #endif  Unicode/ASCII 
//#endif  TCHAR-aware compilers/the others 
//
//
//    various special cases
// 
//
// define wxStricmp and wxStrnicmp for various compilers 
//
// note that in Unicode mode we definitely are going to need our own version 
//#if !defined(wxStricmp) && !wxUSE_UNICODE
//    #if defined(__BORLANDC__) || defined(__WATCOMC__) || \
//            defined(__SALFORDC__) || defined(__VISAGECPP__) || \
//            defined(__EMX__) || defined(__DJGPP__)
//        #define wxStricmp stricmp
//        #define wxStrnicmp strnicmp
//    #elif defined(__WXPALMOS__)
//         FIXME: There is no equivalent to strnicmp in the Palm OS API.  This
//         * quick hack should do until one can be written.
//         
//        #define wxStricmp StrCaselessCompare
//        #define wxStrnicmp strnicmp
//    #elif defined(__SYMANTEC__) || defined(__VISUALC__) || \
//            (defined(__MWERKS__) && defined(__INTEL__))
//        #define wxStricmp _stricmp
//        #define wxStrnicmp _strnicmp
//    #elif defined(__UNIX__) || defined(__GNUWIN32__)
//        #define wxStricmp strcasecmp
//        #define wxStrnicmp strncasecmp
//     #else -- use wxWidgets implementation 
//    #endif
//#endif  !defined(wxStricmp) 
//
// define wxWcslen() which should be always available if wxUSE_WCHAR_T == 1 (as 
// it's used in wx/buffer.h -- and also might be used just below by wxStrlen() 
// when wxStrlen_() is #define'd as wxWcslen so do it before defining wxStrlen) 
//#if wxUSE_WCHAR_T
//    #ifdef HAVE_WCSLEN
//        #define wxWcslen wcslen
//    #else
//        WXDLLIMPEXP_BASE size_t wxWcslen(const wchar_t *s);
//    #endif
//#endif  wxUSE_WCHAR_T 
//
//#ifdef __cplusplus
// checks whether the passed in pointer is NULL and if the string is empty 
//inline bool wxIsEmpty(const wxChar *p) { return !p || !*p; }
//
// safe version of strlen() (returns 0 if passed NULL pointer) 
//inline size_t wxStrlen(const wxChar *psz) { return psz ? wxStrlen_(psz) : 0; }
//#endif
//
//
//    each of strdup() and wcsdup() may or may not be available but we need both
//    of them anyhow for wx/buffer.h so we define the missing one(s) in
//    wxchar.cpp and so we should always have both wxStrdupA and wxStrdupW
//    defined -- if this is somehow not the case in some situations, please
//    correct that and not the lines here
// 
//#if wxUSE_UNICODE
//    #define wxStrdup wxStrdupW
//#else
//    #define wxStrdup wxStrdupA
//#endif
//
//#ifdef __cplusplus
//WXDLLIMPEXP_BASE bool wxOKlibc();  for internal use 
//#endif
//
// printf() family saga 
//
//
//   For some systems [v]snprintf() exists in the system libraries but not in the
//   headers, so we need to declare it ourselves to be able to use it.
// 
//#if defined(HAVE_VSNPRINTF) && !defined(HAVE_VSNPRINTF_DECL)
//#ifdef __cplusplus
//    extern "C"
//#else
//    extern
//#endif
//    int vsnprintf(char *str, size_t size, const char *format, va_list ap);
//#endif  !HAVE_VSNPRINTF_DECL 
//
//#if defined(HAVE_SNPRINTF) && !defined(HAVE_SNPRINTF_DECL)
//#ifdef __cplusplus
//    extern "C"
//#else
//    extern
//#endif
//    int snprintf(char *str, size_t size, const char *format, ...);
//#endif  !HAVE_SNPRINTF_DECL 
//
// Wrapper for vsnprintf if it's 3rd parameter is non-const. Note: the
// * same isn't done for snprintf below, the builtin wxSnprintf_ is used
// * instead since it's already a simple wrapper 
//#if defined __cplusplus && defined HAVE_BROKEN_VSNPRINTF_DECL
//    inline int wx_fixed_vsnprintf(char *str, size_t size, const char *format, va_list ap)
//    {
//        return vsnprintf(str, size, (char*)format, ap);
//    }
//#endif
//
//
//   First of all, we always want to define safe snprintf() function to be used
//   instead of sprintf(). Some compilers already have it (or rather vsnprintf()
//   which we really need...), otherwise we implement it using our own printf()
//   code.
//
//   We define function with a trailing underscore here because the real one is a
//   wrapper around it as explained below
// 
//#ifndef wxVsnprintf_
//    #if wxUSE_UNICODE
//        #if defined(__MWERKS__)
//            #define HAVE_WCSRTOMBS 1
//            #define HAVE_VSWPRINTF 1
//        #endif
//        #if defined(__WATCOMC__)
//            #define wxVsnprintf_    _vsnwprintf
//            #define wxSnprintf_     _snwprintf
//        #endif
//        #if defined(HAVE__VSNWPRINTF)
//            #define wxVsnprintf_    _vsnwprintf
//         MinGW?MSVCRT has the wrong vswprintf 
//		 Mac OS X has a somehow buggy vswprintf 
//        #elif defined(HAVE_VSWPRINTF) && !defined(__MINGW32__) && !defined(__DARWIN__)
//            #define wxVsnprintf_    vswprintf
//        #endif
//    #else  ASCII 
//         all versions of CodeWarrior supported by wxWidgets apparently have 
//         both snprintf() and vsnprintf() 
//        #if defined(HAVE_SNPRINTF) || defined(__MWERKS__) || defined(__WATCOMC__)
//            #ifndef HAVE_BROKEN_SNPRINTF_DECL
//                #define wxSnprintf_     snprintf
//            #endif
//        #endif
//        #if defined(HAVE_VSNPRINTF) || defined(__MWERKS__) || defined(__WATCOMC__)
//            #if defined __cplusplus && defined HAVE_BROKEN_VSNPRINTF_DECL
//                #define wxVsnprintf_    wx_fixed_vsnprintf
//            #else
//                #define wxVsnprintf_    vsnprintf
//            #endif
//        #endif
//    #endif
//#endif  wxVsnprintf_ not defined yet 
//
//#ifndef wxSnprintf_
//     no [v]snprintf(), cook our own 
//    WXDLLIMPEXP_BASE int wxSnprintf_(wxChar *buf, size_t len, const wxChar *format,
//                                ...) ATTRIBUTE_PRINTF_3;
//#endif
//#ifndef wxVsnprintf_
//    WXDLLIMPEXP_BASE int wxVsnprintf_(wxChar *buf, size_t len, const wxChar *format,
//                                 va_list argptr);
//#endif
//
//
//   In Unicode mode we need to have all standard functions such as wprintf() and
//   so on but not all systems have them so use our own implementations in this
//   case.
// 
//#if wxUSE_UNICODE && !defined(wxHAVE_TCHAR_SUPPORT) && !defined(HAVE_WPRINTF)
//    #define wxNEED_WPRINTF
//#endif
//
//
//   More Unicode complications: although both ANSI C and C++ define a number of
//   wide character functions such as wprintf(), not all environments have them.
//   Worse, those which do have different behaviours: under Windows, %s format
//   specifier changes its meaning in Unicode build and expects a Unicode string
//   while under Unix/POSIX it still means an ASCII string even for wprintf() and
//   %ls has to be used for wide strings.
//
//   We choose to always emulate Windows behaviour as more useful for us so even
//   if we have wprintf() we still must wrap it in a non trivial wxPrintf().
//
//
//
//#if defined(wxNEED_PRINTF_CONVERSION) || defined(wxNEED_WPRINTF)
//    
//        we need to implement all wide character printf and scanf functions
//        either because we don't have them at all or because they don't have the
//        semantics we need
//     
//
//    #include <stdio.h>   for FILE 
//
//    int wxScanf( const wxChar *format, ... ) ATTRIBUTE_PRINTF_1;
//    int wxSscanf( const wxChar *str, const wxChar *format, ... ) ATTRIBUTE_PRINTF_2;
//    int wxFscanf( FILE *stream, const wxChar *format, ... ) ATTRIBUTE_PRINTF_2;
//    int wxVsscanf( const wxChar *str, const wxChar *format, va_list ap );
//    int wxPrintf( const wxChar *format, ... ) ATTRIBUTE_PRINTF_1;
//    int wxSprintf( wxChar *str, const wxChar *format, ... ) ATTRIBUTE_PRINTF_2;
//    int wxFprintf( FILE *stream, const wxChar *format, ... ) ATTRIBUTE_PRINTF_2;
//    int wxVfprintf( FILE *stream, const wxChar *format, va_list ap );
//    int wxVprintf( const wxChar *format, va_list ap );
//    int wxVsprintf( wxChar *str, const wxChar *format, va_list ap );
//#endif  wxNEED_PRINTF_CONVERSION 
//
// these 2 can be simply mapped to the versions with underscore at the end 
// if we don't have to do the conversion 
//
//   However, if we don't have any vswprintf() at all we don't need to redefine
//   anything as our own wxVsnprintf_() already behaves as needed.
//
//#if defined(wxNEED_PRINTF_CONVERSION) && defined(wxVsnprintf_)
//    int wxSnprintf( wxChar *str, size_t size, const wxChar *format, ... ) ATTRIBUTE_PRINTF_3;
//    int wxVsnprintf( wxChar *str, size_t size, const wxChar *format, va_list ap );
//#else
//    #define wxSnprintf wxSnprintf_
//    #define wxVsnprintf wxVsnprintf_
//#endif
//
//
//    various functions which might not be available in libc and for which we
//    provide our own replacements in wxchar.cpp
// 
//
// ctype.h functions 
//
// RN: Used only under OSX <= 10.2 currently 
//#ifdef wxNEED_WX_CTYPE_H
//    WXDLLIMPEXP_BASE int wxIsalnum(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsalpha(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIscntrl(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsdigit(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsgraph(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIslower(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsprint(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIspunct(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsspace(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsupper(wxChar ch);
//    WXDLLIMPEXP_BASE int wxIsxdigit(wxChar ch);
//    WXDLLIMPEXP_BASE int wxTolower(wxChar ch);
//    WXDLLIMPEXP_BASE int wxToupper(wxChar ch);
//#endif  wxNEED_WX_CTYPE_H 
//
// under VC++ 6.0 isspace() returns 1 for 8 bit chars which completely breaks 
// the file parsing -- this may be true for 5.0 as well, update #ifdef then 
//#if defined(__VISUALC__) && (__VISUALC__ >= 1200) && !wxUSE_UNICODE
//    #undef wxIsspace
//    #define wxIsspace(c) ((((unsigned)c) < 128) && isspace(c))
//#endif  VC++ 
//
//
//   a few compilers don't have the (non standard but common) isascii function,
//   define it ourselves for them
// 
//#ifndef isascii
//    #if defined(__MWERKS__)
//        #define wxNEED_ISASCII
//    #elif defined(_WIN32_WCE)
//        #if _WIN32_WCE <= 211
//            #define wxNEED_ISASCII
//        #endif
//    #endif
//#endif  isascii 
//
//#ifdef wxNEED_ISASCII
//    inline int isascii(int c) { return (unsigned)c < 0x80; }
//#endif
//
//#ifdef _WIN32_WCE
//    #if _WIN32_WCE <= 211
//        #define isspace(c) ((c) == _T(' ') || (c) == _T('\t'))
//    #endif
//#endif  _WIN32_WCE 
//
//
//   we had goofed and defined wxIsctrl() instead of (correct) wxIscntrl() in the
//   initial versions of this header -- now it is too late to remove it so
//   although we fixed the function/macro name above, still provide the
//   backwards-compatible synonym.
// 
//#define wxIsctrl wxIscntrl
//
// string.h functions 
//#ifndef strdup
//    #if defined(__MWERKS__) && !defined(__MACH__) && (__MSL__ < 0x00008000)
//        #define wxNEED_STRDUP
//    #elif defined(__WXWINCE__)
//        #if _WIN32_WCE <= 211
//            #define wxNEED_STRDUP
//        #endif
//    #endif
//#endif  strdup 
//
//#ifdef wxNEED_STRDUP
//    WXDLLIMPEXP_BASE char *strdup(const char* s);
//#endif
//
// RN: Used only under OSX <= 10.2 currently
//   The __cplusplus ifdefs are messy, but they are required to build
//   the regex library, since c does not support function overloading
//
//#ifdef wxNEED_WX_STRING_H
//# ifdef __cplusplus
//    extern "C" {
//# endif
//        WXDLLIMPEXP_BASE wxChar * wxStrcat(wxChar *dest, const wxChar *src);
//        WXDLLIMPEXP_BASE const wxChar * wxStrchr(const wxChar *s, wxChar c);
//        WXDLLIMPEXP_BASE int      wxStrcmp(const wxChar *s1, const wxChar *s2);
//        WXDLLIMPEXP_BASE int      wxStrcoll(const wxChar *s1, const wxChar *s2);
//        WXDLLIMPEXP_BASE wxChar * wxStrcpy(wxChar *dest, const wxChar *src);
//        WXDLLIMPEXP_BASE size_t   wxStrcspn(const wxChar *s, const wxChar *reject);
//        WXDLLIMPEXP_BASE wxChar * wxStrncat(wxChar *dest, const wxChar *src, size_t n);
//        WXDLLIMPEXP_BASE int      wxStrncmp(const wxChar *s1, const wxChar *s2, size_t n);
//        WXDLLIMPEXP_BASE wxChar * wxStrncpy(wxChar *dest, const wxChar *src, size_t n);
//        WXDLLIMPEXP_BASE const wxChar * wxStrpbrk(const wxChar *s, const wxChar *accept);
//        WXDLLIMPEXP_BASE const wxChar * wxStrrchr(const wxChar *s, wxChar c);
//        WXDLLIMPEXP_BASE size_t   wxStrspn(const wxChar *s, const wxChar *accept);
//        WXDLLIMPEXP_BASE const wxChar * wxStrstr(const wxChar *haystack, const wxChar *needle);
//# ifdef __cplusplus
//    }
//# endif
//
//     These functions use C++, so we can't c extern them 
//    WXDLLIMPEXP_BASE double   wxStrtod(const wxChar *nptr, wxChar **endptr);
//    WXDLLIMPEXP_BASE long int wxStrtol(const wxChar *nptr, wxChar **endptr, int base);
//    WXDLLIMPEXP_BASE unsigned long int wxStrtoul(const wxChar *nptr, wxChar **endptr, int base);
//    WXDLLIMPEXP_BASE size_t   wxStrxfrm(wxChar *dest, const wxChar *src, size_t n);
//
//     inlined versions 
//    #ifdef __cplusplus
//        inline wxChar * wxStrchr(wxChar *s, wxChar c)
//            { return (wxChar *)wxStrchr((const wxChar *)s, c); }
//        inline wxChar * wxStrpbrk(wxChar *s, const wxChar *accept)
//            { return (wxChar *)wxStrpbrk((const wxChar *)s, accept); }
//        inline wxChar * wxStrrchr(wxChar *s, wxChar c)
//            { return (wxChar *)wxStrrchr((const wxChar *)s, c); }
//        inline wxChar *wxStrstr(wxChar *haystack, const wxChar *needle)
//            { return (wxChar *)wxStrstr((const wxChar *)haystack, needle); }
//    #endif
//
//#endif  wxNEED_WX_STRING_H 
//
//#ifndef wxStrdupA
//WXDLLIMPEXP_BASE char *wxStrdupA(const char *psz);
//#endif
//
//#ifndef wxStrdupW
//WXDLLIMPEXP_BASE wchar_t *wxStrdupW(const wchar_t *pwz);
//#endif
//
//#ifndef wxStricmp
//WXDLLIMPEXP_BASE int wxStricmp(const wxChar *psz1, const wxChar *psz2);
//#endif
//
//#ifndef wxStrnicmp
//WXDLLIMPEXP_BASE int wxStrnicmp(const wxChar *psz1, const wxChar *psz2, size_t len);
//#endif
//
//#ifndef wxStrtok
//WXDLLIMPEXP_BASE wxChar * wxStrtok(wxChar *psz, const wxChar *delim, wxChar **save_ptr);
//#endif
//
//#ifdef __cplusplus
//#ifndef wxSetlocale
//class WXDLLIMPEXP_BASE wxWCharBuffer;
//WXDLLIMPEXP_BASE wxWCharBuffer wxSetlocale(int category, const wxChar *locale);
//#endif
//#endif
//
// stdio.h functions 
//#ifdef wxNEED_WX_STDIO_H
//    #include <stdio.h>
//    WXDLLIMPEXP_BASE FILE *   wxFopen(const wxChar *path, const wxChar *mode);
//    WXDLLIMPEXP_BASE FILE *   wxFreopen(const wxChar *path, const wxChar *mode, FILE *stream);
//    WXDLLIMPEXP_BASE int      wxRemove(const wxChar *path);
//    WXDLLIMPEXP_BASE int      wxRename(const wxChar *oldpath, const wxChar *newpath);
//
//     *printf() family is handled separately 
//#endif  wxNEED_WX_STDIO_H 
//
//
// stdlib.h functions 
//#ifndef wxAtof
//WXDLLIMPEXP_BASE double   wxAtof(const wxChar *psz);
//#endif
//
//#ifdef wxNEED_WX_STDLIB_H
//WXDLLIMPEXP_BASE int      wxAtoi(const wxChar *psz);
//WXDLLIMPEXP_BASE long     wxAtol(const wxChar *psz);
//WXDLLIMPEXP_BASE wxChar * wxGetenv(const wxChar *name);
//WXDLLIMPEXP_BASE int      wxSystem(const wxChar *psz);
//#endif
//
//
// time.h functions 
//#ifdef wxNEED_WX_TIME_H
//#if defined(__MWERKS__) && defined(macintosh)
//    #include <time.h>
//#endif
//    silent gabby compilers
//    struct tm;
//    WXDLLIMPEXP_BASE size_t wxStrftime(wxChar *s, size_t max,
//                                  const wxChar *fmt, const struct tm *tm);
//#endif  wxNEED_WX_TIME_H 
//
//#ifndef wxCtime
//#include <time.h>
//WXDLLIMPEXP_BASE wxChar *wxCtime(const time_t *timep);
//#endif
//
//
// missing functions in some WinCE versions 
//#ifdef _WIN32_WCE
//#if (_WIN32_WCE < 300)
//WXDLLIMPEXP_BASE void *calloc( size_t num, size_t size );
//#endif
//#endif  _WIN32_WCE 
//
// multibyte to wide char conversion functions and macros 
//
//#if wxUSE_WCHAR_T
//     multibyte<->widechar conversion 
//    WXDLLIMPEXP_BASE size_t wxMB2WC(wchar_t *buf, const char *psz, size_t n);
//    WXDLLIMPEXP_BASE size_t wxWC2MB(char *buf, const wchar_t *psz, size_t n);
//
//    #if wxUSE_UNICODE
//        #define wxMB2WX wxMB2WC
//        #define wxWX2MB wxWC2MB
//        #define wxWC2WX wxStrncpy
//        #define wxWX2WC wxStrncpy
//    #else
//        #define wxMB2WX wxStrncpy
//        #define wxWX2MB wxStrncpy
//        #define wxWC2WX wxWC2MB
//        #define wxWX2WC wxMB2WC
//    #endif
//#else  !wxUSE_UNICODE 
// Why is this here?
//#error ha 
//     No wxUSE_WCHAR_T: we have to do something (JACS) 
//    #define wxMB2WC wxStrncpy
//    #define wxWC2MB wxStrncpy
//    #define wxMB2WX wxStrncpy
//    #define wxWX2MB wxStrncpy
//    #define wxWC2WX wxWC2MB
//    #define wxWX2WC wxMB2WC
//#endif
//
//
//    RN:  The following are not normal versions of memcpy et al., rather
//    these are either char or widechar versions depending on
//    if unicode is used or not.
//
//
//#ifdef __cplusplus
//
//    //
//    //  RN: We could do the usual tricky compiler detection here,
//    //  and use their variant (such as wmemchr, etc.).  The problem
//    //  is that these functions are quite rare, even though they are
//    //  part of the current POSIX standard.  In addition, most compilers
//    //  (including even MSC) inline them just like we do right in their
//    //  headers.
//    //
//    #if wxUSE_UNICODE
//        #include <string.h> //for mem funcs
//
//        //implement our own wmem variants
//        inline wxChar* wxTmemchr(const wxChar* s, wxChar c, size_t l)
//        {
//            for(;l && *s != c;--l, ++s) {}
//
//            if(l)
//                return (wxChar*)s;
//            return NULL;
//        }
//
//        inline int wxTmemcmp(const wxChar* sz1, const wxChar* sz2, size_t len)
//        {
//            for(; *sz1 == *sz2 && len; --len, ++sz1, ++sz2) {}
//
//            if(len)
//                return *sz1 < *sz2 ? -1 : *sz1 > *sz2;
//            else
//                return 0;
//        }
//
//        inline wxChar* wxTmemcpy(wxChar* szOut, const wxChar* szIn, size_t len)
//        {
//            return (wxChar*) memcpy(szOut, szIn, len * sizeof(wxChar));
//        }
//
//        inline wxChar* wxTmemmove(wxChar* szOut, const wxChar* szIn, size_t len)
//        {
//            return (wxChar*) memmove(szOut, szIn, len * sizeof(wxChar));
//        }
//
//        inline wxChar* wxTmemset(wxChar* szOut, const wxChar cIn, size_t len)
//        {
//            wxChar* szRet = szOut;
//
//            while (len--)
//                *szOut++ = cIn;
//
//            return szRet;
//        }
//
//    #else //!wxUSE_UNICODE
//    #   define wxTmemchr memchr
//    #   define wxTmemcmp memcmp
//    #   define wxTmemcpy memcpy
//    #   define wxTmemmove memmove
//    #   define wxTmemset memset
//    #endif
//
//#endif __cplusplus
//
//
//#endif  _WX_WXCHAR_H_ 
//

// end of wxchar.h
*/