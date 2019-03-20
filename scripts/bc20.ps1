#
# program name 	  :   bc20.ps1 powershell script for Bill Martins' wx widget C++ build layout conversion routine
# date written    :   11/3/2019
# written by      :   L.Pearl C/- B.Waters - 
# revised by      :   B.Martin 19Mar2019 - Updated patterns to the current list in sedCommandFile.txt.
#                       Modified to process sublist of 3 .cpp source $inputfiles (mainly Adapt_It_wdr.cpp).
#                       Modified to in-place .Replace operations of the file in relative path ..\source\,
#                       assuming that the powershell script is located in the adaptit\scripts\ dir.
#                       Added dot progress indicators for each file processed.
#                       TODO: read and parse the sed find and replace patterns from the sedCommandFile.txt
#                             instead of hard coding them into this script file.  
#
# with 23 search & replace strings defined to run across 3 input files
# I tried an inline recursive process which always returned an 'out of memory' exception
# so I left the major function as a simple loop running each search & replace call separately
# performed  ... so , at some stage with an update to the .NET management libraries , there might 
# be a better future procedure for this .
#
# all of the sed/awk inline functions were written to allow proper memory recursive control
# of the calls ... powershell looks like it was never intended to be more than a better DOS
# batch processor ... but with some OO functionality thrown in .... hence some of t he java/perl
# code control constructs .... hopeless , why not just build in the way sed/awk work ???
#
# L.Pearl .. 28/2/2019
#
# the code is invoked as follows from an administrator priviledge commandline shell :-
# powershell.exe -ExecutionPolicy Bypass "c:\path-to-the-script-folder\bc20.ps1" 
#
#
$dirPathSeparator = [IO.Path]::DirectorySeparatorChar
$sourcepath = ".." + $dirPathSeparator + "source" + $dirPathSeparator # ../source/ or ..\source\
#$inputfiles = Get-ChildItem "$sourcepath" + "*.cpp"
$inputfiles = "Adapt_It_wdr.cpp", "ExportSaveAsDlg.cpp", "PeekAtFile.cpp" # array of filenames
$header_string = "// This file was processed by the powershell conversion script"

#
function doTAGreplaces 
{
    $filenumbercount = 0
	$filesprocessedcount = 0
    foreach( $sourcefile in $inputfiles )
    {
		 # whm 19Mar2019 added
		 #write-host "debug: sourcefile is: $sourcefile"
		 $i = $sourcepath + $sourcefile
		 $dirname = [System.IO.Path]::GetDirectoryName("$i")
		 $filename = [System.IO.Path]::GetFileName("$i")
         #
         # find a pattern match from a read on the entire input .cpp file
         # and if there is no match the .Length mthod returns ca zero
         #
         $matchcount = Get-ChildItem -Path $i | Select-String -Pattern '// this file was processed by the powershell conversion script'
         if( $matchcount.Matches.Length -lt 1 )
         {
		     write-host "Processing $filename " -NoNewline
             #
             # see the last part of this code block for the header string prepending
             #
             # set up the search and replace strings to use 
             #
             # whm 19Mar2019 modified search-replace below to preserve the wxALIGN_RIGHT flag
             $wxsearchstringRV = ", wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|"
             $wxreplacestringRV = ", wxALIGN_RIGHT|"
             #
             # next is just a debug statement ..
             #
             #write-host the search and replace part now for 1-RV...........................................
             #
             # read the entire content into RAM and do the bulk search/replace command over the lot just
             # like perl / ruby / python do in a 'slurp' read/write
			 #
			 # whm 19Mar2019 Note: Due to memory thrashing issues in doing something like:
			 #   $content = $content -Replace $wxsearchstringRV,$wxreplacestringRV
			 # for each of the replacement strings, Leon's design turns out to be much faster
			 # in simply reading the source file repeatedly from disk, doing the .Replace 
			 # method and calling WriteAllText for each of the 25 patterns.
             #
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringRV,$wxreplacestringRV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             # whm 19Mar2019 modified search-replace below to preserve the wxALIGN_RIGHT flag
             $wxsearchstringV0 = ", wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL,"
             $wxreplacestringV0 = ", wxALIGN_RIGHT,"
             #write-host the search and replace part now for 2-V0............................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringV0,$wxreplacestringV0 )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringBV = ", wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL|"
             $wxreplacestringBV = ", "
             #write-host the search and replace part now for 3-BV...........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringBV,$wxreplacestringBV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringBH = ", wxALIGN_BOTTOM|wxALIGN_CENTER_HORIZONTAL,"
             $wxreplacestringBH = ", 0,"
             #write-host the search and replace part now for 4-BH...........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringBH,$wxreplacestringBH )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringBHV = "wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL"
             $wxreplacestringBHV = "wxALIGN_CENTER"
             #write-host the search and replace part now for 5-BHV..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringBHV,$wxreplacestringBHV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringCV = ", wxALIGN_CENTER_VERTICAL,"
             $wxreplacestringCV = ", 0,"
             #write-host the search and replace part now for 6-CV..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringCV,$wxreplacestringCV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGH = "|wxGROW|wxALIGN_CENTER_HORIZONTAL,"
             $wxreplacestringGH = "|wxGROW,"
             #write-host the search and replace part now for 7-GH..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGH,$wxreplacestringGH )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGRH = "|wxGROW|wxALIGN_CENTER_HORIZONTAL|"
             $wxreplacestringGRH = "|wxGROW|"
             #write-host the search and replace part now for 8-GRH..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGRH,$wxreplacestringGRH )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGRH = "wxGROW|wxALIGN_CENTER_HORIZONTAL|" 
             $wxreplacestringGRH = "wxGROW|"
             #write-host the search and replace part now for 9-GRH..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGRH,$wxreplacestringGRH )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGCH = "wxGROW|wxALIGN_CENTER_HORIZONTAL,"
             $wxreplacestringGCH = "wxGROW,"
             #write-host the search and replace part now for 10-GCH..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGCH,$wxreplacestringGCH )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGRV = "wxGROW|wxALIGN_CENTER_VERTICAL|"
             $wxreplacestringGRV = "wxGROW|"
             #write-host the search and replace part now for 11-GRV..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGRV,$wxreplacestringGRV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGRRV = "wxGROW|wxALIGN_CENTER_VERTICAL,"
             $wxreplacestringGRRV = "wxGROW,"
             #write-host the search and replace part now for 12-GRRV..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGRRV,$wxreplacestringGRRV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGVV = "wxALIGN_CENTER_VERTICAL|"
             $wxreplacestringGVV = ""
             #write-host the search and replace part now for 13-GVV..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGVV,$wxreplacestringGVV )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringGHH = "wxALIGN_CENTER_HORIZONTAL|"
             $wxreplacestringGHH = ""
             #write-host the search and replace part now for 14-GHH..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringGHH,$wxreplacestringGHH )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringH0 = ", wxALIGN_CENTER_HORIZONTAL," 
             $wxreplacestringH0 = ", 0,"
             #write-host the search and replace part now for 15-H0..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringH0,$wxreplacestringH0 )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             # whm 19Mar2019 removed search-replace instance below
             #$wxsearchstringR0 = "|wxALIGN_RIGHT"
             #$wxreplacestringR0 = ""
             #write-host the search and replace part now for 16-R0..........................................
             #$content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringR0,$wxreplacestringR0 )
             #[System.IO.File]::WriteAllText( $i, $content )
             #
             $wxsearchstringNWN = "wxNORMAL, wxNORMAL"
             $wxreplacestringNWN = "wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL"
             #write-host the search and replace part now for 17-NWN..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringNWN,$wxreplacestringNWN )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringNN = "wxNORMAL,"
             $wxreplacestringNN = "wxFONTSTYLE_NORMAL,"
             #write-host the search and replace part now for 18-NN..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringNN,$wxreplacestringNN )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringNFN = ", wxNORMAL )"
             $wxreplacestringNFN = ", wxFONTWEIGHT_NORMAL )"
             #write-host the search and replace part now for 19-NFN..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringNFN,$wxreplacestringNFN )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringSF = "wxSWISS"
             $wxreplacestringSF = "wxFONTFAMILY_SWISS"
             #write-host the search and replace part now for 20-SF..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringSF,$wxreplacestringSF )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringRF = "wxROMAN"
             $wxreplacestringRF = "wxFONTFAMILY_ROMAN"
             #write-host the search and replace part now for 21-RF..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringRF,$wxreplacestringRF )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringBF = "wxBOLD"
             $wxreplacestringBF = "wxFONTWEIGHT_BOLD"
             #write-host the search and replace part now for 22-BF..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringBF,$wxreplacestringBF )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             $wxsearchstringIF = "wxITALIC"
             $wxreplacestringIF = "wxFONTSTYLE_ITALIC"
             #write-host the search and replace part now for 23-IF..........................................
             $content = [System.IO.File]::ReadAllText( $i ).Replace( $wxsearchstringIF,$wxreplacestringIF )
             [System.IO.File]::WriteAllText( $i, $content )
			 write-host "." -NoNewline
             #
             # lastly, prepend the header string to show the file was processed to the top of the file. 
			 # whm 19Mar2019 modify to use [System.IO.File]::WriteAllText ( $i $header_string )
			 #   then [System.IO.File]::AppendAllText( $i, $content )
			 [System.IO.File]::WriteAllText( $i, $header_string + ([Environment]::NewLine))
			 [System.IO.File]::AppendAllText( $i, $content )
			 write-host "."
			 #
             # whm removed the rename-item process below
             #@( $header_string ) + ( Get-Content $i ) | Set-Content "bk.cpp"
             #remove-item $i
             #
             # with the prepended 'processed string ' in place rename the temp file back to the original
             # name
             #
             #switch( $filenumbercount )
             #{ 
             #        0 { 
             #             rename-item "bk.cpp" -newname "Adapt_It_wdr.cpp"
             #             write-host done Adapt_It_wdr.cpp
             #             break
             #          }
             #        1 { 
             #             rename-item "bk.cpp" -newname "ExportSaveAsDlg.cpp"
             #             write-host done ExportSaveAsDlg.cpp
             #             break 
             #          }
             #        2 { 
             #             rename-item "bk.cpp" -newname "PeekAtFile.cpp"
             #             write-host done PeekAtFile.cpp
             #             break 
             #          }
             #}
             $filesprocessedcount += 1
         }
         else
         {
             write-host $filename was already processed by the conversion script
         }
         $filenumbercount += 1
    }
	write-host "Number of files processed: $filesprocessedcount of $filenumbercount"
}
doTAGreplaces
