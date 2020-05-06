# FAT32 File System
### Operating System/Assignment-4: Created by [Dr. Trevor Bakker](https://github.com/trevorbakker-uta)

## Descirption
A user space shell application that interprets FAT32 file system image.

## Funtionality
* Opens and closes the FAT32 image file.
* Navigates through the image file and prints out following information in both base 10 and base 16:
    *  BPB_BytsPerSec
    *  BPB_SecPerClus
    *  BPB_RsvdSecCnt
    *  BPB_NumFats
    *  BPB_FATSz32
* Prints out the attributes and starting cluster number of the file or directory name.
* Retrives the file from the FAT 32 image and places it onto the current working directory.
* Implements "cd" command to change the current working directory - supports relative and absolute paths.
* Implements "ls" command to lists the directory contents.
* Reads from the given file at the position, in bytes, specified by the position parameter annd output the number of bytes specified.

## Things I learned
* Structure and information about the boot sector.
* Structure about the individual files in FAT32 system.
* Extracting files from the FAT 32 image file.

## Compilation Instructions
The application is built in an omega server at UTA.
In terminal:
```
gcc mfs.c -o mfs 
mfs
```

## Acknowledgements
* [Suman Thapa Magar](https://github.com/suman2020) - Team Member
* [Dr. Trevor Bakker](https://github.com/trevorbakker-uta) for providing small stub [program](https://github.com/CSE3320/FAT32-Assignment) to get started and his guidance throughout the assignment. 
