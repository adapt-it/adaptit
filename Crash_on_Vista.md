Try changing the default language to English US and/or region to US

# Introduction #

Adapt It Unicode crash when first attempting to run it on Vista.



# Details #

The user reported: "When we install and run it, it crashes immediately and gives an error:
wxWidgets Fatal Error
This program uses Unicode and requires Windows NT/2000/XP. Program aborted.

The user (a software developer)as reported: "In researching it, it looks like widgets is thinking that the system is windows 98, but it’s really Vista Home Basic SP1."
And: "**It turns out that it was due to the default language being “English (India)” (or the default location being “India” — we’re not sure, because we changed them both to “US” and it started working)**"

The problem is caused by an unrecognised region setting. The work-around, until the fix is available, is for Vista users to set the regional setting to one of the other recognized "English (xxx)" settings such as English (UK), English (Australia) or English (United States).