# llextfs
Ext file system driver for low-level (embedded) systems

llextfs provides a very basic ext 2, 3 & 4 file system implementation to read files, for example to be used in the firmware of an embedded device. It was able to extract most small files on the disk images used for testing. It is far from feature complete, but extending it shouldn't be hard.

To integrate llextfs in the firmware of a device instead of using it under a regular operating system, a bit of glue code is required. An example of this is given in extfs_glue.c. The file examples/find_passwd_embedded.c shows an example how to initialize and call llextfs from the firmware.

## Todo

- Test with other block sizes then 1024
- Parse the indirect block maps
- Support extents
- Support inodes with inline data