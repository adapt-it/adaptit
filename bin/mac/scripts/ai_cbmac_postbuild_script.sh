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

# Sanity check -- make sure we have the output directory passed in as a parameter
if [ $# -ne 1 ]
then
    echo "No output dir parameter specified -- exiting..."
    exit 1
fi

outputDir=$1

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

# Iterate through the languages in aiMoLangs creating dirs if necessary, and copying 
# the lang.mo files to the appropriate locale dirs with appropriate Adapt It.mo name.
count=0
for lang in $aiMoLangs
do
   echo "Copying localization files for language $lang to bundle."
   mkdir -p "${localeDir}/${lang}/LC_MESSAGES"
   cp "${poDir}/${lang}.mo" "${localeDir}/${lang}/LC_MESSAGES/Adapt It.mo"
done

# Iterate through the languages in wxMoLangs creating dirs if necessary, and copying 
# the wxstd-lang.mo files to the appropriate locale dirs with appropriate wxstd.mo name.
count=0
for lang in $wxMoLangs
do
   echo "Copying wxWidgets localization files for language $lang to bundle."
   mkdir -p "${localeDir}/${lang}/LC_MESSAGES"
   cp "${poDir}/wxstd-${lang}.mo" "${localeDir}/${lang}/LC_MESSAGES/wxstd.mo"
done

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

# Use the tar command to get a "clean" set of dirs and files from hlp into
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

# Copy the .htb html help file to Adapt It.app/Contents/SharedSupport/
cp "$htbFile" "${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/SharedSupport/${PRODUCT_NAME}.htb"

# Set up variables for copying of docs to Resources
resourcesDir="${PROJECT_DIR}/${outputDir}/${PRODUCT_NAME}.app/Contents/Resources"
docsDir="${PROJECT_DIR}/../../docs"
licenseDirSrc="${PROJECT_DIR}/../../license"
licenseDir="${resourcesDir}/license"
imagesDir="${resourcesDir}/images/Adapt It Quick Start"
imagesDirSrc="${docsDir}/images/Adapt It Quick Start"

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
mkdir -p "$imagesDir"
cp "$imagesDirSrc/back_button.gif" "$imagesDir"
cp "$imagesDirSrc/Image2.gif" "$imagesDir"
cp "$imagesDirSrc/Image3.gif" "$imagesDir"
cp "$imagesDirSrc/Image4.gif" "$imagesDir"
cp "$imagesDirSrc/Image5.gif" "$imagesDir"
cp "$imagesDirSrc/Image6.gif" "$imagesDir"
cp "$imagesDirSrc/Image7.gif" "$imagesDir"
cp "$imagesDirSrc/Image8.gif" "$imagesDir"
cp "$imagesDirSrc/Image9.gif" "$imagesDir"
cp "$imagesDirSrc/Image10.gif" "$imagesDir"
cp "$imagesDirSrc/Image11.gif" "$imagesDir"
cp "$imagesDirSrc/Image12.gif" "$imagesDir"
cp "$imagesDirSrc/Image13.gif" "$imagesDir"
cp "$imagesDirSrc/Image14.gif" "$imagesDir"
cp "$imagesDirSrc/Image15.gif" "$imagesDir"
cp "$imagesDirSrc/Image16.gif" "$imagesDir"
cp "$imagesDirSrc/Image17.gif" "$imagesDir"
cp "$imagesDirSrc/Image19.gif" "$imagesDir"
cp "$imagesDirSrc/Image20.gif" "$imagesDir"

