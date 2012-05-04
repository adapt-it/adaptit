/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Pathway.cpp
/// \author			Erik Brommers
/// \date_created	26 March 2012
/// \date_revised
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description This is the implementation file for SIL Pathway integration.
///              Pathway (http://pathway.sil.org) is a publishing utility that
///              can convert USFM to other formats (PDF, e-book, cell phone, etc.).
/////////////////////////////////////////////////////////////////////////////


// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h> // for wxRegKey - used in SantaFeFocus sync scrolling mechanism
#endif

// other includes
#include <wx/filesys.h> // for wxFileName
#include <wx/textfile.h> // for wxTextFile
#include <wx/utils.h> // for wxShell

#include "Adapt_It.h"
#include "ExportFunctions.h"

/// This global is defined in Adapt_ItView.cpp.
extern bool gbIsGlossing;

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp;


//////////////////////////////////////////////////////////////////////////////////////////
/// \return     TRUE if Pathway is installed on the host computer, FALSE otherwise
/// \remarks
/// Called from: the App's OnInit().
/// Looks in the Windows registry to see if Pathway is installed.
//////////////////////////////////////////////////////////////////////////////////////////
bool CAdapt_ItApp::PathwayIsInstalled()
{
	bool bPWInstalled = FALSE;
#ifdef __WXMSW__ // Windows host -- use registry

	wxLogNull logNo; // eliminate any spurious messages from the system
	// only transition data if the Adapt_It_WX key exists in the host Windows' registry
	wxRegKey keyPTInstallDir(_T("HKEY_LOCAL_MACHINE\\SOFTWARE\\SIL\\Pathway\\PathwayDir"));
	if (keyPTInstallDir.Exists() && keyPTInstallDir.HasValues())
	{
		wxString dirStrValue;
		dirStrValue.Empty();
		if (keyPTInstallDir.Open(wxRegKey::Read)) // open the key for reading only!
		{
			// get the folder path stored in the key, (i.e., C:\Program Files\Paratext7\)
			// Note: the dirStrValue path ends with a backslash so we don't add one here.
			dirStrValue = keyPTInstallDir.QueryDefaultValue();
			if (::wxDirExists(dirStrValue))
			{
				// Make sure the PathwayB command line executable is installed; this drives
				// the Pathway export progress for non-.NET applications
				if (::wxFileExists(dirStrValue + _T("PathwayB.exe")))
				{
					bPWInstalled = TRUE;
				}
			}
		}
	}
#endif
#ifdef __WXGTK__ // linux -- just look for the files in /usr/lib/Paratext/
    wxString strPWRegistryDir = _T("~/.mono/registry/CurrentUser/software/sil/pathway");

    if (::wxDirExists(strPWRegistryDir))
    {
        bPWInstalled = TRUE;
    }
#endif
	return bPWInstalled;

}


//////////////////////////////////////////////////////////////////////////////////////////
/// \return     a wxString representing the path to the Pathway installation directory
/// \remarks
/// Called from: the App's OnInit().
/// Looks in the Windows registry to get the path to the Pathway Install directory.
/// The following registry key is queried for the return value:
///    HKEY_CURRENT_USER/Software/Sil/Pathway/PsthwayDir
/// If the key is not found, or is found but the value string does not exist on
/// the system, the function returns an empty string. This function only reads/queries
/// the Windows registry; it does not make changes to it.
//////////////////////////////////////////////////////////////////////////////////////////
wxString CAdapt_ItApp::GetPathwayInstallDirPath()
{
	wxString path;
	path.Empty();
#ifdef __WXMSW__ // Windows host system - check the registry

	wxLogNull logNo; // eliminate any spurious messages from the system
	// only transition data if the Adapt_It_WX key exists in the host Windows' registry
	wxRegKey keyPTInstallDir(_T("HKEY_LOCAL_MACHINE\\SOFTWARE\\SIL\\Pathway"));
	if (keyPTInstallDir.Exists() && keyPTInstallDir.HasValues())
	{
		wxString dirStrValue;
		dirStrValue.Empty();
		if (keyPTInstallDir.Open(wxRegKey::Read)) // open the key for reading only!
		{
			// get the folder path stored in the key, (i.e., C:\Program Files\Paratext7\)
			if (!keyPTInstallDir.QueryValue(_T("PathwayDir"), dirStrValue))
			{
				// couldn't read the value - exit out
				return path;
			}
			// remove the final backslash, since our path values generally don't have a
			// trailing path separator.
			if (!dirStrValue.IsEmpty() && dirStrValue.GetChar(dirStrValue.Length()-1) == _T('\\'))
				dirStrValue.RemoveLast(1);
			if (::wxDirExists(dirStrValue))
			{
				path = dirStrValue;
			}
		}
	}
#endif
#ifdef __WXGTK__ // linux -- check mono directory for values.xml file
    wxString strRegPath = wxGetenv(_T("HOME"));
    strRegPath += _T("/.mono/registry/CurrentUser/software/sil/pathway");
    if (::wxDirExists(strRegPath))
    {
        // MONO_REGISTRY_PATH exists -- see if we can get the projects dir our of the values.xml
        // file located in strRegPath
        wxString strValuesFile = strRegPath + _T("/values.xml");
        wxString strBuf;
        wxString strPath;
        if (wxFileExists(strValuesFile))
        {
            wxTextFile tfile;
            if (tfile.Open(strValuesFile))
            {
                strBuf = tfile.GetFirstLine();
                while (!tfile.Eof())
                {
                    if (strBuf.Contains(_T("type=\"string\">")))
                    {
                        // extract the path from this string value
                        int nStart = (strBuf.Find(_T("\">"))) + 2;
                        int nEnd = strBuf.Find(_T("</value>"));
                        strPath = strBuf.Mid(nStart, (nEnd - nStart));
                        wxLogDebug(strPath);
                        break; // exit the while loop -- we've found our match
                    }
                    strBuf = tfile.GetNextLine();
                }
                tfile.Close();
            }
        }

        if (!strPath.IsEmpty() && strPath.GetChar(strPath.Length()-1) == _T('\\'))
            strPath.RemoveLast(1);
        if (::wxDirExists(strPath))
        {
            path = strPath;
        }
    }
#endif

	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// \remarks
/// Called from: the File / Export Through Pathway menu handler().
/// Takes the USFM from the current project and calls Pathway's command line to convert it
/// to the desired output format.
//////////////////////////////////////////////////////////////////////////////////////////
void CAdapt_ItApp::OnExportThroughPathway(wxCommandEvent& WXUNUSED(event))
{
	gpApp->LogUserAction(_T("Export Through Pathway"));
	// Step 1: export the target to USFM
	wxString usfmText = RebuildText_For_Collaboration(gpApp->GetSourcePhraseList(), targetTextExport, TRUE);
	wxTextFile usfmFile((gpApp->m_targetOutputsFolderPath + gpApp->PathSeparator + _T("ai_pw.sfm")));
	if (usfmFile.Exists())
	{
		usfmFile.Open();
		usfmFile.Clear();
	}
	else
	{
		usfmFile.Create();
	}
	usfmFile.AddLine(usfmText);
	usfmFile.Write();
	usfmFile.Close();

	// Step 2: Call PathwayB.exe on the exported USFM.
	// The full command line should look something like this:
	//    PathwayB.exe -d "D:\Project2" -if usfm -f * -i "Scripture" -n "SEN" -s
	// (A description of the PathwayB parameters can be found by calling PathwayB.exe
	// from the command prompt without any parameters.)
	wxArrayString textIOArray, errorsIOArray;
	wxString commandLine;
	// full path to PathwayB executable
	wxString PWBatchFilename = m_PathwayInstallDirPath + gpApp->PathSeparator + _T("PathwayB.exe");
	commandLine = _T("\"") + PWBatchFilename + _T("\" -d \"") + gpApp->m_targetOutputsFolderPath;
	commandLine += _T("\" -if usfm -f * -i \"Scripture\" -n \"");
	// if there is a language code specified, pass it along; if not, use a generic "MP1"
	commandLine += ((gpApp->m_targetLanguageCode.IsEmpty()) ? _T("MP1") : gpApp->m_targetLanguageCode);
	// show the dialog to let the user choose the format (TODO: do we want this, or just
	// take the defaults set up by the admin?)
	commandLine += _T("\" -s"); 
	int code = wxExecute(commandLine, wxEXEC_SYNC);
	//int code = wxExecute(commandLine,textIOArray,errorsIOArray);
	//int code = wxShell(commandLine);
	wxLogStatus(_T("Shell command '%s' terminated with exit code %d."),
                commandLine.c_str(), code);
}

////////////////////////////////////////////////////////////////////////////////////////
/// \return     nothing
/// \param      event   -> the wxUpdateUIEvent that is generated when the menu is about
///                         to be displayed
/// \remarks
/// Called from: The wxUpdateUIEvent mechanism when the associated menu item is selected,
/// and before the menu is displayed.
/// When Pathway and Paratext are installed the "Export Through Pathway" menu
//  command is enabled, otherwise it is disabled. 
////////////////////////////////////////////////////////////////////////////////////////
void CAdapt_ItApp::OnUpdateExportThroughPathway(wxUpdateUIEvent& event)
{
	if (m_bReadOnlyAccess)
	{
		event.Enable(FALSE);
		return;
	}
	// Pathway and Paratext have to be installed (PT is used by Pathway for the USFM conversion)
	// AND we need to be working on a project.
	if (!m_PathwayInstallDirPath.IsEmpty() && !m_ParatextInstallDirPath.IsEmpty())
		event.Enable(TRUE);
	else
		event.Enable(FALSE);
}