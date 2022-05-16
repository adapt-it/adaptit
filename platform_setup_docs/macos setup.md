# Adapt It setup

These are a couple other setup-related items I've run across with macOS. 

## Suppress debug asserts in Release mode

We have to make one change to the wxWidgets library source before building; the developers made a tweak a few releases back that causes debug ASSERTs to show up in release builds. Here’s how to turn that off.

1. Open file “./build/osx/setup/cocoa/include/wx/setup.h” and search for the #define for NDEBUG (should be around line 96). 
2. Uncomment out part of the block so that release asserts are set to have a wxDEBUG_LEVEL of 0:

    #ifdef NDEBUG
    #define wxDEBUG_LEVEL 0
    // #else
    //  #define wxDEBUG_LEVEL 2
    #endif

## Setting the wxwidgets build path 

We’re using a static build of wxWidgets for osx. Part of this setup requires a known path for the library when built -- a configuration that Apple deems "legacy" these days. 

1. Open Xcode’s Settings > Locations tab, and click the Advanced button. 
2. In the popup that appears, select the Legacy radio button.

*Side note:* I’ve needed to jump back and forth between the legacy build location and the default one when moving from Adapt It and Adapt It Mobile, respectively. You'll likely only need to do this if you've got other/newer apps you support along with Adapt It.

## Command line build / archive

I’ve had some issues with my Xcode build setup over the years that have _not_ been issues from the command line. Provided you’ve got the command line tools for Xcode installed (it should prompt you), you can build from the command line using the following commands:

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
