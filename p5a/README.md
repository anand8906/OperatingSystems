
# File System Checking

In this assignment, you will be developing a working file system checker. A
checker reads in a file system image and makes sure that it is
consistent. When it isn't, the checker takes steps to repair the problems it
sees; however, you won't be doing any repairs to keep this project a little
simpler.

## A Basic Checker

For this project, you will use the xv6 file system image as the basic image
that you will be reading and checking. The file `include/fs.h` includes the
basic structures you need to understand, including the superblock, on disk
inode format (`struct dinode`), and directory entry format (`struct
dirent`). The tool `tools/mkfs.c` will also be useful to look at, in order to
see how an empty file-system image is created.

Much of this project will be puzzling out the exact on-disk format xv6 uses
for its simple file system, and then writing checks to see if various parts of
that structure are consistent. Thus, reading through `mkfs.c` and the file
system code itself will help you understand how xv6 uses the bits in the image
to record persistent information.

Your checker should read through the file system image and determine the
consistency of a number of things, including the following. When a problem is
detected, print the error message (shown below) to **standard error** and
exit immediately with **exit code 1** (i.e., call `exit(1)`). 

1. Each inode is either unallocated or one of the valid types (`T_FILE`, `T_DIR`,
`T_DEV`). If not, print `ERROR: bad inode.`

1. For in-use inodes, each address that is used by inode is valid (points to a
valid datablock address within the image). If the direct block is used and is
invalid, print `ERROR: bad direct address in inode.`; if the indirect block is
in use and is invalid, print `ERROR: bad indirect address in inode.`

1. Root directory exists, its inode number is 1, and the parent of the root
directory is itself. If not, print `ERROR: root directory does not exist.`

1. Each directory contains `.` and `..` entries, and the `.` entry points to the
directory itself. If not, print `ERROR: directory not properly formatted.`

1. For in-use inodes, each address in use is also marked in use in the
  bitmap. If not, print `ERROR: address used by inode but marked free in bitmap.`

1. For blocks marked in-use in bitmap, the block should actually be in-use in
an inode or indirect block somewhere. If not, print `ERROR: bitmap marks block in use but it is not in use.`

1. For in-use inodes, each direct address in use is only used once. If not,
  print `ERROR: direct address used more than once.`

1. For in-use inodes, each indirect address in use is only used once. If not,
  print `ERROR: indirect address used more than once.`

1. For all inodes marked in use, each must be referred to in at least one directory. 
  If not, print `ERROR: inode marked use but not found in a directory.`

1. For each inode number that is referred to in a valid directory, it is actually
  marked in use. If not, print `ERROR: inode referred to in directory but marked free.`

1. Reference counts (number of links) for regular files match the number of times
  file is referred to in directories (i.e., hard links work correctly). 
  If not, print `ERROR: bad reference count for file.`

1. No extra links allowed for directories (each directory only appears in one
  other directory). If not, print `ERROR: directory appears more than once in file system.`


## Other Specifications

Your checker program, called `xcheck`, must be invoked exactly as follows:

```
prompt> xcheck file_system_image
```

The image file is a file that contains the file system image. If no image file
is provided, you should print the usage error shown below:
```
prompt> xcheck 
Usage: xcheck <file_system_image> 
```
This output must be printed to standard error and exit with the error code of 1. 

If the file system image does not exist, you should print the error `image not
found.` to standard error and exit with the error code of 1. 

If the checker detects any one of the 12 errors above, it should print the
specific error to standard error and exit with error code 1.

If the checker detects none of the problems listed above, it should exit with
return code of 0 and not print anything. 
