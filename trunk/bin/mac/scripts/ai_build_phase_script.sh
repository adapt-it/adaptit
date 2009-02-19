#! /bin/sh

# uncomment the commented out lines below for better debugging of this shell script
#set -v on
#set -x on
#echo $PROJECT_DIR
#echo $CONFIGURATION_BUILD_DIR
#echo $PRODUCT_NAME

# Set up variables for localization file processing
poDir="${PROJECT_DIR}/../../po"
localeDir="${CONFIGURATION_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Resources/locale"
aiMoLangs="es fr id pt ru tpi zh az"
wxMoLangs="es fr pt ru tpi zh"

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

# Use the tar command to get a "clean" set of dirs and files from hlp into
# the hlp_temp directory, leaving out the .svn dirs and other stuff specified
# in the excludeFiles file.
cd "$hlpDir"
tar -czf "$tarFile" * -X "$excludeFiles"
cd "$tempDir"
tar -xzvf "$tarFile"
rm "$tarFile"

# Now zip the present contents of hlpDir excluding the hidden .svn dirs
# and other undesirable stuff, into a new htbFile. Do it from inside the
# hlp directory so we don't include the hlp directory name in the zip
# archive.
cd "$tempDir"
zip -r "$htbFile" *

#Finally copy the .htb html help file to Adapt It.app/Contents/SharedResources/
mkdir -p "${CONFIGURATION_BUILD_DIR}/${SHARED_SUPPORT_FOLDER_PATH}"
cp "$htbFile" "${CONFIGURATION_BUILD_DIR}/${SHARED_SUPPORT_FOLDER_PATH}/${PRODUCT_NAME}.htb"

# Set up variables for copying of docs to Resources
resourcesDir="${CONFIGURATION_BUILD_DIR}/${PRODUCT_NAME}.app/Contents/Resources"
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
