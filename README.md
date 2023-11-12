# mounter-elite-plus
Port of bash shell program mounter_elite to C++ 
https://github.com/siyia2/mounter_elite


![2023-11-07-012549_grim](https://github.com/siyia2/mounter-elite-plus/assets/46220960/343e1ce4-1e27-4885-83c1-c4341a9aeb48)




State of the art secure and blazing fast terminal image mounter/converter written in C++, it allows mounting `ISOs` and/or converting `BIN&IMG&MDF` files to `ISO`. Supports Tab completion on disk cache generation, a curtesy of readline the library. 
You can add multiple mounts and/or conversion paths. All paths are mounted under `/mnt/iso_*` format and all conversions take place at their local source directories, script requires `ROOT` access to `mount&unmount ISOs`. 

If you get: `Error: filesystem error: cannot increment recursive directory iterator: Permission denied`.
Make sure your path has `755` and upwards permissions for better cache generation, if not, you can set the executable as an `SUID`, or you can always run it as `ROOT` from the beginning if need be.

Added Features:
* Cached ISO management to reduce disk thrashing.
* Sanitized shell commands for improved security.
* Supports all filenames, including special charactes and gaps and ''.
* Extra checks and error controls so that you don't erase your / or /mnt accidentally (joking).
* More robust menu experience.
* Improved selection procedures.
* Proper multithreading support added it can now utilize as many cores as are available for maximum performance.
* Faster search times and cache generetions.
* Faster mounting&unmounting times, from my tests i found it can mount, up to 30 ISO files in under 5s.
* Dropped manual mode of mounting/converting, since caching is way faster and easier to manage.
* Added support for MDF conversion to ISO by utilizing mdf2iso.
* Clean codebase, just in case someone decides to contribute in the future.

Tested the program on my rig: `mounting` at the same time `125 ISOS` and then `unmounting` them, did this a couple of times and it didn't even break a sweat, Rock Solid!!! :)

Compilation requires the readline library installed from your distro. 
Once dependancies are met, open a terminal insede the source folder and just run `g++ -O2 mounter_elite_plus.cpp conversion_tools.cpp sanitization_readline.cpp -lreadline -fopenmp -o "whatever_name"`.
Make it executable and it can run from anywhere.

You can also download the binaries from my releases, or if you are on arch or on an archbased distro you can install with `yay -S mounter-elite-plus`.
