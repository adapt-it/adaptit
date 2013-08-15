#! /bin/sh

#-------------------------------------------------------------------------------
#    Name:	ai_cbmac_postbuild_script.sh
#    Date:	19 January 2010
#  Author:	Erik Brommers
# Purpose:	Postbuild script for Code::Blocks builds on the Mac OS X platform. 
#			This script creates the Mac application bundle for the Adapt It
#			Mac port as follows:
#			1. The Plistbuddy utility is called to populate the 
#			Info.plist file with build information (Xcode uses an internal
#			<com.apple.tools.info-plist-utility> to do this). 
#			2. The following files / directories of note are created / copied 
#			over (this isn't a complete list):
#			Adapt It.app
#			- Contents
#			  - Info.plist
#			  - Pkginfo (text file with file type / creator)
#			  - Resources
#			  	- Adapt It.icns
#				- images
#				- license
#				- locale (localized resources)
#			  - MacOS
#				- Adapt It (the binary we built in Code::Blocks)
#			  - SharedSupport
#				- Adapt It.htb
#-------------------------------------------------------------------------------

# Sanity check -- make sure we have the right number of parameters passed in
if [ $# -ne 3 ]
then
    echo "Wrong number of parameters -- exiting..."
    exit 1
fi

outputDir=$1
PROJECT_DIR=$2
PRODUCT_NAME=$3

# uncomment the commented out lines below for better debugging of this shell script
set -v on
set -x on
echo "Project settings:"
echo $PROJECT_DIR
echo ${outputDir}
echo $PRODUCT_NAME
echo "--------"

# Set up variables for localization file processing
poDir="${PROJECT_DIR}/../../po"
localeDir="${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Resources/locale"
aiMoLangs="es fr id pt ru tpi zh az"
wxMoLangs="es fr pt ru tpi zh"
binDir="${PROJECT_DIR}/${outputDir}${PRODUCT_NAME}.app/Contents/MacOS"

# Create the Adapt It.app directory structures
mkdir -p "${PROJECT_DIR}/${outputDir}${PRODUCT_NAME}.app/Contents/SharedSupport"
mkdir -p "${PROJECT_DIR}/${outputDir}${PRODUCT_NAME}.app/Contents/Resources"
mkdir -p "${binDir}"

# Make a copy of Info.plist and convert it to binary, so we can manipulate the 
# string values with the PlistBuddy utility
# NOTE: PlistBuddy is installed in /usr/libexec for Leopard - earlier 
# installations of OSX might not have this utility (although I suspect you can 
# download it from ADC).
cp "${PROJECT_DIR}/Info.plist" "${PROJECT_DIR}/${outputDir}/Info.plist"

plutil -convert binary1 "${PROJECT_DIR}/${outputDir}/Info.plist"
# Populate the Info.plist info with our build info
/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable ${PRODUCT_NAME}" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleGetInfoString ${PRODUCT_NAME} version ${PRODUCT_VERSION}, (c) ${COPYRIGHT_DATE} Bruce Waters, Bill Martin, SIL International" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier org.adapt-it.${PRODUCT_NAME}" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleLongVersionString ${PRODUCT_VERSION}, (c) ${COPYRIGHT_DATE} Bruce Waters, Bill Martin, SIL International" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleName ${PRODUCT_NAME}" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${PRODUCT_VERSION}" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${PRODUCT_VERSION}" "${PROJECT_DIR}/${outputDir}/Info.plist"
/usr/libexec/PlistBuddy -c "Set :NSHumanReadableCopyright Copyright ${COPYRIGHT_DATE} Bruce Waters, Bill Martin, SIL International" "${PROJECT_DIR}/${outputDir}/Info.plist"

# Start moving / copying files over

# Populated Info.plist to the Adapt It.app/Contents
mv "${PROJECT_DIR}/${outputDir}/Info.plist" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Info.plist"
# Pkginfo file (plain text file with "AAPL????" as the only text)
cp "${PROJECT_DIR}/Pkginfo" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Pkginfo"
# Icon and xml files
basedir=${PROJECT_DIR}/../..
cp "${basedir}/res/Adapt It.icns" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Resources/Adapt It.icns"
cp "${basedir}/res/AIWX.gif" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Resources/AIWX.gif"
cp "${basedir}/xml/AI_USFM.xml" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Resources/AI_USFM.xml"
cp "${basedir}/xml/books.xml" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Resources/books.xml"
# Binary application file
mv "${PROJECT_DIR}/${outputDir}${PRODUCT_NAME}" "${PROJECT_DIR}/${outputDir}${PRODUCT_NAME}.app/Contents/MacOS/"

# Iterate through the languages in wxMoLangs creating dirs if necessary, and copying 
# the wxstd-lang.mo files to the appropriate locale dirs with appropriate wxstd.mo name.
count=0
for lang in $wxMoLangs
do
   echo "Copying wxWidgets localization files for language $lang to bundle."
   mkdir -p "${localeDir}/${lang}/LC_MESSAGES"
   cp "${poDir}/wxstd-${lang}.mo" "${localeDir}/${lang}/LC_MESSAGES/wxstd.mo"
done

# uncomment the commented out lines below for better debugging of this shell script
#set -v on
#set -x on

# Set up variables for HTML Help file processing
hlpDir="${PROJECT_DIR}/../../hlp"
excludeFiles="$hlpDir/mac_excl.lst"
tempDir="${PROJECT_DIR}/../../hlp_temp"
htbFile="$tempDir/${PRODUCT_NAME}.htb"
tarFile="$tempDir/archive.tar.gz"
echo Processing Html Help File with zip into 

# If the tempDir exists make sure it is empty; if it doesn't exist create it
if [ -d "$tempDir" ]
then
	rm -r "$tempDir"/*
else
	mkdir -p "$tempDir"
fi

# If the htbFile exists remove it before preparing a freshly zipped one
if [ -f "$htbFile" ]
then
	rm "$htbFile"
fi

# Use the gnutar command to get a "clean" set of dirs and files from hlp into
# the hlp_temp directory, leaving out the .svn dirs and other stuff specified
# in the excludeFiles file.
cd "$hlpDir"
gnutar -czf "$tarFile" * -X "$excludeFiles"
cd "$tempDir"
gnutar -xzvf "$tarFile"
rm "$tarFile"

# Now zip the present contents of hlpDir excluding the hidden .svn dirs
# and other undesirable stuff, into a new htbFile. Do it from inside the
# hlp directory so we don't include the hlp directory name in the zip
# archive.
cd "$tempDir"
zip -r "$htbFile" *

#Finally copy the .htb html help file to Adapt It.app/Contents/SharedResources/
sharedSuppDir="${CONFIGURATION_BUILD_DIR}/${SHARED_SUPPORT_FOLDER_PATH}"
mkdir -p "$sharedSuppDir"
cp "$htbFile" "$sharedSuppDir/${PRODUCT_NAME}.htb"

# Set up variables for copying of docs to Resources
resourcesDir="${PROJECT_DIR}${outputDir}${CONFIGURATION_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Resources"
docsDir="${PROJECT_DIR}/../../docs"
licenseDirSrc="${PROJECT_DIR}/../../license"
licenseDir="${resourcesDir}/license"

cp "$docsDir/iso639-3codes.txt" "$resourcesDir"
cp "$docsDir/curl-ca-bundle.crt" "$resourcesDir"
cp "$docsDir/Adapt It Reference.doc" "$resourcesDir"
cp "$docsDir/Adapt It Tutorial.doc" "$resourcesDir"
cp "$docsDir/Adapt It changes.txt" "$resourcesDir"
cp "$docsDir/KJV 1Jn 2.12-17.txt" "$resourcesDir"
cp "$docsDir/Known Issues and Limitations.txt" "$resourcesDir"
cp "$docsDir/Localization_Readme.txt" "$resourcesDir"
cp "$docsDir/Readme_Unicode_Version.txt" "$resourcesDir"
cp "$docsDir/Tok Pisin fragment 1John.txt" "$resourcesDir"
mkdir -p "$licenseDir"
cp "$licenseDirSrc/LICENSING.txt" "$licenseDir"
cp "$licenseDirSrc/License_CPLv05.txt" "$licenseDir"
cp "$licenseDirSrc/License_GPLv2.txt" "$licenseDir"
cp "$licenseDirSrc/License_LGPLv21.txt" "$licenseDir"

# Set up variables for copying of Quick Start help files and images to Shared Support
imagesQSDir="${sharedSuppDir}/images/Adapt_It_Quick_Start"
imagesQSDirSrc="${docsDir}/images/Adapt_It_Quick_Start"
mkdir -p "$imagesQSDir"
cp "$docsDir/Adapt_It_Quick_Start.htm" "$sharedSuppDir"
cp $imagesQSDirSrc/*.gif "$imagesQSDir"

# Set up variables for copying of Help for Admin help files and images to Shared Support
imagesHADir="${sharedSuppDir}/images/Admin_help"
imagesHADirSrc="${docsDir}/images/Admin_help"
mkdir -p "$imagesHADir"
cp "$docsDir/Help_for_Administrators.htm" "$sharedSuppDir"
cp $imagesHADirSrc/*.gif "$imagesHADir"
