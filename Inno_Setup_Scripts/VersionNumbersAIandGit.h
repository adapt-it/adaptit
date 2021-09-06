; This is the VersionNumbersAIandGit.h file. 
; This file is included in our Inno Setup scripts for creating Windows installers
; for Adapt It and our custom Git Downloader.
; Changes to version numbers for the Adapt It (AI_...) defines below should match
; the version number of the Adapt It application itself (in Adapt_It.h).
; Changes to version numbers for the Git installer and our custom Git Downloader
; (GIT_...) should match the version number of the latest Git installer that is
; available at: https://git-scm.com/downloads.

; The following three defines are the only three lines that need updating within this script
; when releasing a new version of Adapt It.
; YOU MUST ALSO modify the corresponding AI version info in the InstallGitOptionsDlg.cpp 
; source file (about lines 107-109)
; For a given Adapt It release, make sure that the other Adapt It Inno Setup scripts listed 
; below have the SAME VERSION NUMBERS as this Adapt It Unicode.iss script:
;   Adapt It Unicode-Minimal.iss
;   Adapt It Unicode-No Html Help.iss
#define AI_VERSION_MAJOR 6
#define AI_VERSION_MINOR 10
#define AI_REVISION 5

; The following three defines are the only three lines that need updating within this script
; when releasing a new version of git to be downloaded by this Adapt It installer.
; Updates of the git version below must have matching version changes in the Git_Downloader...
; script 'Adapt It Unicode Git.iss', and both of these version numbers mush match the 
; version numbers in the Adapt It source code file 'InstallGitOptionsDlg.cpp'.
#define GIT_VERSION_MAJOR 2
#define GIT_VERSION_MINOR 32
#define GIT_REVISION 0

