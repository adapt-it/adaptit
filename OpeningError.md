# Introduction #

A current email indicated the following problem:

I have got an issue with Adapit. We are using version 3.3 here in the Mbeya Iringa Cluster Project in the UTB Branch.

After the installation, everything goes well, I open adaptit and I get the following error:

Failed Setting Current directory in GetPossibleAdaptationProjects() and it allows me to click OK

The I get a Visual C++ Runtime error saying:

Runtime Error

Program \.....\adapit Unicode\English\_AdaptitIU.exe

# Solution to access Adapt It Projects and Documents #

The  basic Adapt It configuration file has become corrupted somehow. You can have Adapt It Unicode (and the Regular version too) bypass the bad configuration files on next launch, by holding down the SHIFT key when you launch. Keep it held down until you are past the project page of the wizard.

If you want to bypass the project's configuration file too, then keep it held down until you are looking at the Document page of the wizard. It will use "safe" configuration settings which are hard coded, and then you should be able to access your project and its documents.

Once you have a document on the screen, exit Adapt It Unicode. On exit it will write new copies of both project and basic configuration files to the hard drive. After that all should be well.

This particular error happens if AI or AIU is using a path which is not a valid path to the Adapt It Work folder (for the Regular version), or to the Adapt It Unicode Work folder (for the Unicode version).