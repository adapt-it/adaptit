#! /bin/sh

# uncomment the commented out lines below for better debugging of this shell script
set -v on
#set -x on

# Set up variables for file processing
currentDir=`pwd`
echo ${currentDir}
downloadsDir="downloads"
adaptitDir="${downloadsDir}/adaptit"
binDir="${adaptitDir}/bin"
linuxDir="${binDir}/linux"
macDir="${binDir}/mac"
win32Dir="${binDir}/win32"
ai_regular_setupDir="${binDir}/Adapt It WX Regular Setup"
ai_unicode_setupDir="${binDir}/Adapt It WX Unicode Setup"
docsDir="${adaptitDir}/docs"
hlpDir="${adaptitDir}/hlp"
licenseDir="${adaptitDir}/license"
platform_setup_docsDir="${adaptitDir}/platform_setup_docs"
echo ${platform_setup_docsDir}
poDir="${adaptitDir}/po"
resDir="${adaptitDir}/res"
res32x32Dir="${resDir}/32x32"
res48x48Dir="${resDir}/48x48"
sourceDir="${adaptitDir}/source"
testdocsDir="${adaptitDir}/testdocs"
xmlDir="${adaptitDir}/xml"

# If the downloads directory exists make sure it is empty; if it doesn't exist create it
if [ -d ${currentDir}/${downloadsDir} ]
then
	rm -r ${currentDir}/${downloadsDir}/*
else
	mkdir -p ${currentDir}/${downloadsDir}
fi

# Copy/update files but not sub-directories to the adaptitDir folder
mkdir -p ${currentDir}/${adaptitDir}
cp ${currentDir}/* ${currentDir}/${adaptitDir}

# Copy/update appropriate linux mac and win32 subfolders to the "downloads\adaptit\bin\" folder
mkdir -p "${currentDir}/${ai_regular_setupDir}"
mkdir -p "${currentDir}/${ai_unicode_setupDir}"
mkdir -p "${currentDir}/${linuxDir}"
mkdir -p "${currentDir}/${macDir}"
mkdir -p "${currentDir}/${macDir}/AdaptIt.xcodeproj"
mkdir -p "${currentDir}/${macDir}/scripts"
mkdir -p "${currentDir}/${win32Dir}" 
cp "${currentDir}/bin/Adapt It WX Regular Setup/Adapt It WX Regular Setup.vdproj" "${ai_regular_setupDir}"
cp "${currentDir}/bin/Adapt It WX Unicode Setup/Adapt It WX Unicode Setup.vdproj" "${ai_unicode_setupDir}"
cp "${currentDir}/bin/linux/config.guess" "${linuxDir}"
cp "${currentDir}/bin/linux/config.rpath" "${linuxDir}"
cp "${currentDir}/bin/linux/config.sub" "${linuxDir}"
cp "${currentDir}/bin/linux/configure" "${linuxDir}"
cp "${currentDir}/bin/linux/configure.in" "${linuxDir}"
cp "${currentDir}/bin/linux/Makefile.am" "${linuxDir}"
cp "${currentDir}/bin/linux/Makefile.in" "${linuxDir}"
cp "${currentDir}/bin/linux/ChangeLog" "${linuxDir}"
cp "${currentDir}/bin/linux/ABOUT-NLS" "${linuxDir}"
cp "${currentDir}/bin/linux/AUTHORS" "${linuxDir}"
cp "${currentDir}/bin/linux/INSTALL" "${linuxDir}"
cp "${currentDir}/bin/linux/NEWS" "${linuxDir}"
cp "${currentDir}/bin/linux/README" "${linuxDir}"
cp "${currentDir}/bin/linux/COPYING" "${linuxDir}"
cp "${currentDir}/bin/linux/aclocal.m4" "${linuxDir}"
cp "${currentDir}/bin/linux/adaptit.cbp" "${linuxDir}"
cp "${currentDir}/bin/linux/autogen.sh" "${linuxDir}"
cp "${currentDir}/bin/linux/ltmain.sh" "${linuxDir}"
cp "${currentDir}/bin/linux/install-sh" "${linuxDir}"
cp "${currentDir}/bin/linux/missing" "${linuxDir}"
cp "${currentDir}/bin/linux/po" "${linuxDir}"
cp "${currentDir}/bin/mac/adaptit.cbp" "${currentDir}/${macDir}"
cp "${currentDir}/bin/mac/Info.plist" "${currentDir}/${macDir}"
cp "${currentDir}/bin/mac/PkgInfo" "${currentDir}/${macDir}"
cp "${currentDir}/bin/mac/AdaptIt.xcodeproj/project.pbxproj" "${currentDir}/${macDir}/AdaptIt.xcodeproj"
cp "${currentDir}/bin/mac/scripts/ai_build_phase_script.sh" "${currentDir}/${macDir}/scripts"
cp "${currentDir}/bin/mac/scripts/ai_cbmac_postbuild_script.sh" "${currentDir}/${macDir}/scripts"
cp "${currentDir}/bin/win32/Adapt_It.cbp" "${currentDir}/${win32Dir}"
cp "${currentDir}/bin/win32/Adapt_It.rc" "${currentDir}/${win32Dir}"
cp "${currentDir}/bin/win32/Adapt_It.sln" "${currentDir}/${win32Dir}"
cp "${currentDir}/bin/win32/Adapt_It.vcproj" "${currentDir}/${win32Dir}"
cp "${currentDir}/bin/win32/Adapt_It_vs05.sln" "${currentDir}/${win32Dir}"
cp "${currentDir}/bin/win32/Adapt_It_vs05.vcproj" "${currentDir}/${win32Dir}"
cp "${currentDir}/bin/win32/Equivalents_for_reference.h" "${currentDir}/${win32Dir}"

# The debian folder should not be included in the distributable source tree

# The design-docs folder should not be included in the distributable source tree

# Copy docs files to the "downloads\adaptit\docs\" folder

mkdir -p "${currentDir}/${docsDir}"
rsync -aC --exclude=.svn/ docs/ "${docsDir}"
#copy "docs\Adapt It changes.txt" "downloads\adaptit\ChangeLog"
#DOS2UNIX.EXE "downloads\adaptit\ChangeLog"

# Copy hlp files to the "downloads\adaptit\hlp\" folder

mkdir -p "${currentDir}/${hlpDir}"
rsync -aC --exclude=.svn/ hlp/ "${hlpDir}"

# The Inno Setup Scripts folder should not be included in the distributable source tree

# Copy license files to the "downloads\adaptit\license\" folder

mkdir -p "${currentDir}/${licenseDir}"
rsync -aC --exclude=.svn/ license/ "${licenseDir}"

# The packaging and distribution docs folder should not be included in the distributable source tree

# The php folder should not be included in the distributable source tree

# Copy platform setup docs files to the "downloads\adaptit\platform setup docs\" folder

mkdir -p "${currentDir}/${platform_setup_docsDir}"
rsync -aC --exclude=.svn/ platform_setup_docs/ "${platform_setup_docsDir}"

# Copy po files to the "downloads\adaptit\po\" folder

# The source tree needs to contain both mo and po files for Windows plus others specific to Linux and the Mac
mkdir -p "${currentDir}/${poDir}"
rsync -aC --exclude=.svn/ po/ "${poDir}"

# The rdwrtp7 folder should not be included in the distributable source tree (rdwrtp7.exe is in the xml folder)

# Copy res files to the "downloads\adaptit\res\" folder

# There are many other image files in res that are not required for distribution of the app
mkdir -p "${currentDir}/${resDir}"
mkdir -p "${currentDir}/${res32x32Dir}"
mkdir -p "${currentDir}/${res48x48Dir}"
# Use regular copy for changing name during copy of ai_32.ico to adapt-it.ico below
cp "res/adapt_it.ico" "${currentDir}/${resDir}"
cp "res/ai_32.ico" "${currentDir}/${resDir}"
cp "res/32x32/adaptit.png" "${currentDir}/${res32x32Dir}"
cp "res/48x48/adaptit.png" "${currentDir}/${res48x48Dir}"

# Copy source files to the "downloads\adaptit\source\" folder

mkdir -p "${currentDir}/${sourceDir}"
rsync -aC --exclude=.svn/ source/ "${sourceDir}"

# Copy testdocs files to the "downloads\adaptit\testdocs\" folder

mkdir -p "${currentDir}/${testdocsDir}"
rsync -aC --exclude=.svn/ testdocs/ "${testdocsDir}"

# Copy xml and cct files to the "downloads\adaptit\xml\" folder
# The Linux sources need not include the rdwrtp7.exe and 5 Paratext dll files that are needed for Windows
mkdir -p "${currentDir}/${xmlDir}"
cp "xml/AI_UserProfiles.xml" "${currentDir}/${xmlDir}"
cp "xml/AI_USFM.xml" "${currentDir}/${xmlDir}"
cp "xml/AI_USFM_full.xml" "${currentDir}/${xmlDir}"
cp "xml/books.xml" "${currentDir}/${xmlDir}"
cp "xml/UsfmXml.cct" "${currentDir}/${xmlDir}"
cp "xml/UsfmXmlTidy.cct" "${currentDir}/${xmlDir}"

# Creating the adaptit_src-x.x.x.tar.gz archive for linux in the "downloads" folder

cd downloads
# The next line invokes the tar command to create the archive.
# The syntax is:
#   c                         create an archive
#   v                         verbose listing of processed files
#   f                         use archive file
#   z                         compress the archive
#   adaptit_src-x.x.x.tar.gz  is the name of the archive to create
#   adaptit                   the directory of sources to use for the archive
tar cfz adaptit_src-x.x.x.tar.gz adaptit
cd ..



