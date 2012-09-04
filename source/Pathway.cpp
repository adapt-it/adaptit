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
	wxRegKey keyPWInstallDir(_T("HKEY_LOCAL_MACHINE\\SOFTWARE\\SIL\\Pathway"));
	if (keyPWInstallDir.Exists() && keyPWInstallDir.HasValues())
	{
		wxString dirStrValue;
		dirStrValue.Empty();
		if (keyPWInstallDir.Open(wxRegKey::Read)) // open the key for reading only!
		{
			// get the folder path stored in the key, (i.e., C:\Program Files\Paratext7\)
			// Note: the dirStrValue path ends with a backslash so we don't add one here.
			if (keyPWInstallDir.QueryValue(_T("PathwayDir"), dirStrValue))
			{
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
	}
#endif
#ifdef __WXGTK__ // linux -- just look for the files in /usr/lib/Paratext/
    wxString strPWRegistryDir = _T("/etc/mono/registry/LocalMachine/software/sil/pathway/");

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
    wxString strRegPath = _T("/etc/mono/registry/LocalMachine/software/sil/pathway/");
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
