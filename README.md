
# Build Instructions On Windows

Microsoft Visual Studio is required to build this product on Windows platforms. Open the Native Tools Command Prompt, for example "VS2015 x86 Native Tools Command Prompt" and perform the following steps:

1) Locate the supp folder in the root directory.
2) In the supp folder, run setup_supp.bat
3) In the supp folder, run make_all.bat.
4) Next, locate the application folder in the root directory; this folder is prefixed "app" with a suffix specfic to the product.
5) Inside the application folder, locate the sln folder.
6) In the sln folder, open the build.sln file with Visual Studio and build it.
   -or-
   Build it with msbuild.
