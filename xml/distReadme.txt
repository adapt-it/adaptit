This small text file, distReadme.txt is used as a "placeholder" file by 
the Linux Makefile as ot creates a "dist" directory at the install location: 
"/usr/share/adaptit/dist" or "/usr/local/share/adaptit/dist"
depending on the value of m_PathPrefix. 
The Linux Makefile creates the "dist" directory which initially is empty,
except that the Makefile copies this distReadme.txt file into that directory.
The "dist" directory is used on all 3 platforms (Windows, Linux and Mac)
as a location where Adapt It stores various .dat files that it writes 
and reads in the KB Sharing setup and running processes.