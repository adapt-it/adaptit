#
# program name 	  :   bc20.ps1 powershell script for Bill Martins' wx widget C++ build layout conversion routine
# date written    :   11/3/2019
# written by      :   L.Pearl C/- B.Waters - 
# revised by      :   B.Martin 19Mar2019 - 
#                       Updated patterns to the current list in sedCommandFile.txt.
#                       Modified to process sublist of 3 .cpp source $inputfiles (mainly Adapt_It_wdr.cpp).
#                       Modified to in-place .Replace operations of the file in relative path ..\source\,
#                       assuming that the powershell script is located in the adaptit\scripts\ dir.
#                       Added dot progress indicators for each file processed.
#                 :   B.Martin 20Mar2019 - 
#                       Adjusted paths to work when called as Pre-Build Event from Visual Studio IDE, or
#                       from a .../scripts/ dir with the .cpp sources in a sibling .../source/ directory.
#                       Modified to accept two optional parameters to designate <sourc-dir> and <write-dir>.
#                       If only <source-dir> parameter is used, assume <write-dir> is same as <source-dir>.
#                  :  B.Martin 21Mar2019
#                       Modified to read and parse the find and replace patterns from the sedCommandFile.txt
#                       instead of hard coding them into this script file. Adjusted pathe to work when called
#                       from a random directory containing all files needed for testing - with input and 
#                       output all happening in that random directory i.e., during script testing.
#
# With 22 search & replace strings defined to run across 3 input files
# I tried an inline recursive process which always returned an 'out of memory' exception
# so I left the major function as a simple loop running each search & replace call separately
# performed  ... so , at some stage with an update to the .NET management libraries , there might 
# be a better future procedure for this.
#
# All of the sed/awk inline functions were written to allow proper memory recursive control
# of the calls ... powershell looks like it was never intended to be more than a better DOS
# batch processor ... but with some OO functionality thrown in .... hence some of t he java/perl
# code control constructs .... hopeless , why not just build in the way sed/awk work ???
#
# L.Pearl .. 28/2/2019
#
# the code is invoked as follows from an administrator priviledge commandline shell :-
# powershell.exe -ExecutionPolicy Bypass "c:\path-to-the-script-folder\bc20.ps1" [input-dir] [output-dir]


$dirPathSeparator = [IO.Path]::DirectorySeparatorChar

# whm 20Mar2019 Notes:
# When this script bc20.ps1 is called from Visual Studio IDE, the default working directory path (absolute) is:
#  C:\Users\Bill\Documents\adaptit\bin\win32 [where the VS project file Adapt_It.vcxproj resides].
#  The adaptit sources reside in C:\Users\Bill\Documents\adaptit\source [which is ..\..\source\ relative to IDE's project file, but ..\source\ relative to location of this script]
#  This powershell script resides in C:\Users\Bill\Documents\adaptit\scripts [..\..\scripts\ relative to IDE's project file]
# The batch call from VS's Pre-Build Event is as follows: 
#   IF NOT EXIST "C:\Program Files\PowerShell\6" ( echo *** PowerShell NOT installed, bypassing wxDesigner source processing ) Else (powershell.exe -ExecutionPolicy Bypass ..\..\scripts\bc20.ps1)
# Hence, powershell is only invoked during IDE builds when powershell is installed, and not invoked
# during IDE builds if powershell is not installed on the computer. 
# The script bc20.ps1 is invoked by the IDE without any input parameters into the script.
#
# When this script bc20.ps1 is called from the .../adaptit/scripts dir, the working directory path (absolute) is:
#  C:\Users\Bill\Documents\adaptit\scripts [where the bc20.ps1 script resides].
# The adaptit sources reside in C:\Users\Bill\Documents\adaptit\source [..\source\ relative to location of this script]

# Normalize things a bit and ensure that the current working dir is set to the location of the 
# bc20.ps1 script itself, usually the project's .../adaptit/scripts/ directory.
# Get the $scriptPath and change the current dir to it:
$scriptPath = split-path -parent $MyInvocation.MyCommand.Definition
cd $scriptPath

# Set up some default values and relative paths. These relative paths may be changed by input 
# parameters below, and in any case, will be changed to absolute paths by Resolve-Path in code below.
# The $sourcepath starts as a relative path as calculated relative to a sibling 'scripts' directory in the project.
$sourcepath = ".." + $dirPathSeparator + "source" + $dirPathSeparator # ../source/ or ..\source\

# The $outputpath starts as a relative path that is the same as the $sourcepath (.cpp file(s) written back to their
# same location.
$outputpath = $sourcepath

# The $scriptspath starts as a relative path as calculated relative to the .../bin/win32/ IDE project directory.
# Note: This $scriptspath is essentially unused since the script now calls Set-Location (Split-Path -Path $profile) 
#  below which changes the working directory/location to the directory of the executing script - even though the
#  visual studio IDE initially calls the script from the .../bin/win32/ directory.
$scriptspath = ".." + $dirPathSeparator + ".." + $dirPathSeparator + "scripts" + $dirPathSeparator # ../../scripts/ or ..\..\scripts\

# Note: All the path variables defined above terminate with a $dirPathSeparator / or \

$inputfiles = "Adapt_It_wdr.cpp", "ExportSaveAsDlg.cpp", "PeekAtFile.cpp" # array of filenames to process
$header_string = "// This file was processed by the powershell conversion script"
$sedCommandFilePath = $scriptPath + $dirPathSeparator + "sedCommandFile.txt" # expect its location to be in same directory as this script

$curworkdir = $PWD # Should be same as $scriptPath determined above
$parentdir = Split-Path -Leaf $curworkdir # get the parent container/dir - usually "scripts"
if ($parentdir -eq "win32")
{
    # The $parentdir will not now be "win32" unless this script was located in the "win32" directory
    # and called from there.
    cd "$scriptspath" # change working directory from C:\Users\Bill\Documents\adaptit\bin\win32 to: C:\Users\Bill\Documents\adaptit\scripts
    write-host "Changed to $PWD"
}
elseif ($parentdir -eq "scripts") # the normal case
{
    # The $parentdir is "scripts" 
    cd . # Keep current working dir
    #write-host "Debug: This script was called from $PWD"
}
else
{
    # The script is being called from some unknown current dir, so we assume the
    # source files are in the current working dir, and the corresponding output 
    # files are to be written to the same current working dir - unless changed by
    # input parameters into this script (below).
    cd . # Keep current working dir
    write-host "Debug: This script was called from unknown dir $PWD"
}

# Get absolute paths for the relative paths: $sourcepath and $outputpath, calculated from
# the current working dir (normally C:\Users\Bill\Documents\adaptit\scripts), 
# by adding the $sourcepath relative path to the $curworkdir and resolving it to an 
# absolute path. 
# Check if the relative $sourcepath exists calculated from $curworkdir or not - set default paths appropriately.
# Catch any exception generated by Resolve-Path trying to resolve a non-existent path
try
{
    $sourcepath = Resolve-Path -ErrorAction Stop $curworkdir$dirPathSeparator$sourcepath
} 
catch
{
    # Couldn't resolve the relative $sourcePath calculated from $curworkdir.
    # Not a fatal error, just use the $curworkdir terminated by $dirPathSeparator
    # and assume input and output will be done in $curworkdir
    $sourcepath = "$curworkdir" + "$dirPathSeparator"
}
#The $outputpath is initially set to be the same as  the $sourcepath.
$outputpath = $sourcepath
write-host "  The sourcepath is: $sourcepath"
write-host "  The outputpath is: $outputpath"

# Check for parameter usage and handle appropriately.
$parameter1 = ""
$paremeter2 = ""
if ( -not ([string]::IsNullOrEmpty($args[0])) )
{
    $parameter1 = $args[0]
    # Ensure that $parameter1 is terminated with / or \
    if ($parameter1 -notmatch '.+?\\$' -and $parameter1 -notmatch '.+?/$')
    {
        $parameter1 = "$parameter1" + "$dirPathSeparator"
    }
    write-host "  The sourcepath changed by parameter 1 to: $parameter1"
    if(!(Test-Path -Path $parameter1))
    {
        write-host "ERROR: Parameter 1 points to non-existent directory."
		write-host "Parameters must be absolute paths to a directory."
        write-host "Cannot execute script. Aborting..."
        exit 1
    }
    $parameter1 = Resolve-Path $parameter1 # make it an absolute path
    #write-host "Debug: parameter1 resolved is: $parameter1"
    $sourcepath = $parameter1
}

if ( -not ([string]::IsNullOrEmpty($args[1])) )
{
    $parameter2 = $args[1]
    # Ensure that $parameter2 is terminated with / or \
    if ($parameter2 -notmatch '.+?\\$' -and $parameter2 -notmatch '.+?/$')
    {
        $parameter2 = "$parameter2" + "$dirPathSeparator"
    }
    write-host "  The outputpath changed by parameter 2 to: $parameter2"
    if(!(Test-Path -Path $parameter2))
    {
        write-host "ERROR: Parameter 2 points to non-existent directory."
		write-host "Parameters must be absolute paths to a directory."
        write-host "Cannot execute script. Aborting..."
        exit 1
    }
    $parameter2 = Resolve-Path $parameter2 # make it an absolute path
    #write-host "Debug: parameter2 resolved is: $parameter2"
    $outputpath = $parameter2 
}
else
{
    # parameter2 was null/empty, so if parameter1 was not empty assume
    # parameter2 and outputpath should be same directory location as parameter1
    if ( $parameter1 -ne "" )
    {
        $parameter2 = $parameter1
        $outputpath = $parameter2
    }
}

# Read the sedCommandFile.txt lines into an array of strings, used in doTABreplaces function below
$arrayOfPatterns = [IO.File]::ReadAllLines($sedCommandFilePath)

function doTAGreplaces 
{
    $filenumbercount = 0
	$filesprocessedcount = 0
    foreach( $sourcefile in $inputfiles )
    {
		 $i = "$sourcepath" + "$sourcefile"
         $o = "$outputpath" + "$sourcefile"
         #write-host "Debug: i is: $i"
         #write-host "Debug: o is: $o"

		 $dirname = [System.IO.Path]::GetDirectoryName("$i")
		 $filename = [System.IO.Path]::GetFileName("$i")
         #
         # find a pattern match from a read on the entire input .cpp file
         # and if there is no match the .Length mthod returns ca zero
         #
         $matchcount = Get-ChildItem -Path $i | Select-String -Pattern '// this file was processed by the powershell conversion script'
         if( $matchcount.Matches.Length -lt 1 )
         {
             # Note $i and $o are absolute paths for ReadAllText(), WriteAllText(), and AppendAllText() calls below.
             #
             # see the last part of this code block for the header string prepending
             # set up the search and replace strings to use 
             # read the entire content into RAM and do the bulk search/replace command over the lot just
             # like perl / ruby / python do in a 'slurp' read/write
			 #
			 # whm 21Mar2019 simplified to read the sedCommandFile.txt file and do the ReadAllText, .Replace(),
			 # and WriteAllText operations within a foreach loop, thus eliminating the hard coded command patterns.
             #
             # If $i and $o are different directories we need to first call ReadAllText( $i ) to get a copy of
             # the source file  at $i into our $content variable (without doing a .Replace operation), and call 
             # WriteAllText( $o ) to get an initial unchanged copy at the destination location $o, then
             # do all of our subsequent reading from $o and writing back to the $o location, i.e., ReadAllText( $o )  
             # and WriteAllText( $o ). Otherwise, we repeatedly read from an $i location and write to different $o 
             # location it would result in only the last $pattern getting written to $o out of the 22 patterns 
             # getting changed at $o.
             if ( $o -ne $i )
             {
                 write-host "***Note: Parameters set the input and output at different directory locations."
                 write-host "***Copying input to destination, then processing all changes at destination."
                 $content = [System.IO.File]::ReadAllText( $i )
                 [System.IO.File]::WriteAllText( $o, $content )
                 $i = $o  # Get all input below from $o and write to same $o location
             }

		     write-host "Processing $filename " -NoNewline
             $ct = 1
             foreach( $pattern in $arrayOfPatterns )
             {
                 # parse $pattern line into findStringArray and replaceStringArray
	             $pattern = $pattern.Replace("s/","") # remove the beginning sed token
	             $pattern = $pattern.Replace("/g","") # remove the ending sed token
	             $findString,$replaceString = $pattern.Split("/") # split $pattern into the $findString and $replaceString
	
	             # Do the actual Replace operation for this $pattern on the wxDesigner file's $content.
                 #
			     # whm 19Mar2019 Note: Due to memory thrashing issues in doing something like:
			     #   $content = $content -Replace $wxsearchstringRV,$wxreplacestringRV
			     # for each of the replacement strings, Leon's design turns out to be much faster
			     # in simply reading the source file repeatedly from disk, doing the .Replace 
			     # method and calling WriteAllText for each of the 22 patterns.
			 
                 #write-host "Debug: $ct [$findString] replace with [$replaceString]"
                 $content = [System.IO.File]::ReadAllText( $i ).Replace( $findString,$replaceString )
                 [System.IO.File]::WriteAllText( $o, $content )
			     write-host "." -NoNewline
	             $ct = $ct + 1
             }

             # lastly, write the header string as first line and append the $content to show the 
             # file was processed by the powershell conversion script. 
			 # whm 19Mar2019 modify to use [System.IO.File]::WriteAllText ( $o $header_string )
			 #   then [System.IO.File]::AppendAllText( $o, $content )
			 [System.IO.File]::WriteAllText( $o, $header_string + ([Environment]::NewLine))
			 [System.IO.File]::AppendAllText( $o, $content )
			 write-host "."
			 #
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
