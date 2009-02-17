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
