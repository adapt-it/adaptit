# Adapt It setup

These are a couple other setup-related items I've run across with macOS. This file is meant to supplement the "Setting up Adapt It on OS X..." files, which are a bit of a mess right now (mostly because an errant edit of mine clipped out the back half of the file). Your mileage may vary.

## Suppress debug asserts in Release mode

We have to make one change to the wxWidgets library source before building; the developers made a tweak a few releases back that causes debug ASSERTs to show up in release builds. Here’s how to turn that off.

1. Open file “./build/osx/setup/cocoa/include/wx/setup.h” and search for the #define for NDEBUG (should be around line 96). 
2. Uncomment out part of the block so that release asserts are set to have a wxDEBUG_LEVEL of 0:

    #ifdef NDEBUG
    #define wxDEBUG_LEVEL 0
    // #else
    //  #define wxDEBUG_LEVEL 2
    #endif

## Set the xcode build path

We’re using a static build of wxWidgets for osx. Part of this setup requires a known path for the library when built -- a configuration that Apple deems "legacy" these days. 

1. Open Xcode’s Settings > Locations tab, and click the Advanced button. 
2. In the popup that appears, select the Legacy radio button.

*Side note:* I’ve needed to jump back and forth between the legacy build location and the default one when moving from Adapt It and Adapt It Mobile, respectively. You'll likely only need to do this if you've got other/newer apps you support along with Adapt It.

## Build wxwidgets

As of May 2022 / wxWidgets 3.1.6 / Adapt It 6.10.7, we want to build for both Intel (x64) and Mac Silicon:

1. Open wxcocoa.xcodeproj and select the wxcocoa project>static target.
2. From the build path / configuration bar, select static > Any Mac (Intel, Apple Silicon).
3. Select the Project > Build For > Running menu command to build the debug version.
4. Select the Project > Build For > Profileing menu command to build the release version. 

If all goes well, the builds will succeed and end up in the osx>build>Debug and osx>build>Release subdirectories, respectively, as the file `libwx_osx_cocoa.static.a`.

## Environment variables

Setting the environment variables properly is a perennial topic in Adapt It's macOS port discussions. Apple has changed how this is done several times over the years, causing no end of hair-pulling and "how do I do this now?" conversations.

### 12.3 Monterey

As of May 2022, there are two pieces needed to get a good build from _both_ the command line and Xcode itself.

#### ~/.zshenv

1. Create this file if needed: `touch ~/.zshenv`
2. Open the file in your favorite editor and copy in the stuff below, modifying to match your library directories. We're doing two things:
  - setting the command line environment vars, so we can build from the command line
  - telling xcode to use environment.plist (more on that below). Note that this needs to be run each time you log in, before Xcode is launched.

    #set the wxwidgets and boost enviroment vars for the command line
    export WXWIN=~/dev/3pt/wxWidgets-3.1.6
    export BOOST_ROOT=~/dev/3pt/boost_1_77_0
    export WXVER=316

    #set xcode so that it reads the environment vars from environment.plist
    do_xcode_config ()
    {
    defaults write com.apple.dt.Xcode UseSanitizedBuildSystemEnvironment -bool NO
    }

    do_xcode_config
    echo "Xcode config set"


#### ~/Library/LaunchAgents/environment.plist

1. Same thing here -- create this file if needed: `touch ~/Library/LaunchAgents/environment.plist`
2. Open the file in your favorite editor and copy in the stuff below, modifying to match your library directories.

    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <!--
    environment.plist
    -->
    <plist version="1.0">
    <dict>
    <key>Label</key>
    <string>my.startup</string>
    <key>ProgramArguments</key>
    <array>
        <string>sh</string>
        <string>-c</string>
        <string>xsx
            launchctl setenv BOOST_ROOT /Users/erik/dev/3pt/boost_1_77_0
            launchctl setenv WXVER 30
            launchctl setenv WXWIN /Users/erik/dev/3pt/wxWidgets-3.1.6
            launchctl setenv WXWIN30 /Users/erik/dev/3pt//wxWidgets-3.1.6
            launchctl setenv WXWINSVN /Users/erik/dev/3pt/wxWidgets-svn
        </string>
    </array>  
    <key>RunAtLoad</key>
    <true/>
    </dict>
    </plist>

## Command line build / archive

Provided you’ve got the command line tools for Xcode installed (the command line should prompt to install if not), you can build from the command line using the following commands:

    $ cd dev/adaptit/bin/mac
    $ xcodebuild -project adaptit.xcodeproj -scheme AdaptIt build
    $ xcodebuild -project adaptit.xcodeproj -scheme AdaptIt archive

## Creating a notarized version of Adapt It

This is more of a distribution item, but I've put it here as well. We have a couple other distribution hoops on the macOS platform that aren't on the others (though I suspect Linux/Windows will eventually recommend/require something similar in the future); app certification and app notarization:

1. In Xcode, select the Window>Organizer menu command. The App Organizer window will appear.
2. In the Organizer window, click on the Archives item in the left column, select the appropriate AdaptIt archive in the list, then click the Distribute button. The Distribute app wizard will display. (Note that these next steps require a developer account with Apple.)
3. Select the Developer ID radio button and click the Next button
4. Select Upload to notarize the app with the Apple notary service and click the Next button. Note that you might be prompted to re-apply the certificate; follow the prompts if needed (mine is set to automatically manage the certs).
5. Verify the info on the final wizard page, then click Upload to upload Adapt It to Apple for notarization.

## Exporting the notarized app

Once you get a notification from Apple that Adapt It was successfully notarized, you can export it from Xcode:

1. In Xcode, select the Window>Organizer menu command. The App Organizer window will appear.
2. In the Organizer window, click on the Archives item in the left column, select the appropriate AdaptIt archive in the list, then click the Export Notarized App button in the lower right area of the Organizer window. (You can verify the app using that spctl command: ` spctl -a -t exec -vv ./Adapt\ It.app `).
