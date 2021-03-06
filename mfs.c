/*
  Assignment 4 - FAT32
  Team Members:
    -> Yunika Upadhayaya, ID:1001631183
    -> Suman Thapa Magar, ID:1001643016
*/

#define _GNU_SOURCE

//Require header files
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5 // Mav shell only supports five arguments

#define MAX_SIZE_FILE 16

/* struct declaration for Directory*/
struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

FILE *ptr_file;

//These section holds the information for reserved section
char BS_OEMName[8];
int16_t BPB_BytesPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
int32_t BPB_FATSz32;
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSector = 0;
int32_t FirstSectorofCluster = 0;
int32_t root_address;

//Checks if the file is open or not.
int open_file = 0;

//Professor's code to check the LBAToOffset.
int LBAToOffset(int sector)
{
  return ((sector - 2) * BPB_BytesPerSec) +
         (BPB_BytesPerSec * BPB_RsvdSecCnt) +
         (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}

//Professor's code to compare the filename.
//Compares filename like bar.txt with BAR    TXT
int compare(char IMG_Name[], char input[])
{
  char expanded_name[12];
  memset(expanded_name, ' ', 12);

  char temp[12];

  strcpy(temp, input);

  char *token = strtok(temp, ".");

  if (token == NULL)
  {
    strncpy(expanded_name, "..", strlen(".."));
  }
  else
  {
    strncpy(expanded_name, token, strlen(token));
  }

  token = strtok(NULL, ".");

  if (token)
  {
    strncpy((char *)(expanded_name + 8), token, strlen(token));
  }

  expanded_name[11] = '\0';

  int i;
  for (i = 0; i < 11; i++)
  {
    expanded_name[i] = toupper(expanded_name[i]);
  }

  if (strncmp(expanded_name, IMG_Name, 11) == 0)
  {
    return 1;
  }

  return 0;
}

//This function checks if the file directory exists.
//If the file exits, it returns back its index.
//Takes in struct dir and file directory as input.
int match(struct DirectoryEntry dir[], char token[])
{
  int index = 0;
  while (index < MAX_SIZE_FILE)
  {
    if ((dir[index].DIR_Name[0] != 0xffffffe5) &&
        (compare(dir[index].DIR_Name, token)) &&
        (dir[index].DIR_Attr == 0x01 ||
         dir[index].DIR_Attr == 0x10 ||
         dir[index].DIR_Attr == 0x20 ||
         dir[index].DIR_Name[0] == 0x2e))
    {
      return index;
    }
    index++;
  }
  return -2;
}

int main()
{
  //A struct to hold all the attributes of the directory.
  struct DirectoryEntry dir[16];

  char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);

  while (1)
  {
    // Print out the mfs prompt
    printf("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
      ;

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str = strdup(cmd_str);

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

    //Continues if there is no user input, this checks for fragmentation
    if (token[0] == NULL)
    {
      continue;
    }

    //opens the file
    else if (strcmp("open", token[0]) == 0)
    {
      //Check if there is no file name
      if (token[1] == NULL)
      {
        printf("Error: Please enter the name of file to open.\n");
        continue;
      }
      else
      {
        ptr_file = fopen(token[1], "r");
        //Check if the given file name exists
        if (ptr_file == NULL)
        {
          printf("Error: File system image not found.\n");
          continue;
        }
        //Check if the file is already open
        else if (open_file == 1)
        {
          printf("Error: File system image already open.\n");
          continue;
        }
        else
        {
          //Read in from the file
          fseek(ptr_file, 11, SEEK_SET);
          fread(&BPB_BytesPerSec, 2, 1, ptr_file);

          fseek(ptr_file, 13, SEEK_SET);
          fread(&BPB_SecPerClus, 1, 1, ptr_file);

          fseek(ptr_file, 14, SEEK_SET);
          fread(&BPB_RsvdSecCnt, 1, 2, ptr_file);

          fseek(ptr_file, 16, SEEK_SET);
          fread(&BPB_NumFATs, 1, 1, ptr_file);

          fseek(ptr_file, 36, SEEK_SET);
          fread(&BPB_FATSz32, 1, 4, ptr_file);

          root_address = (BPB_BytesPerSec * BPB_RsvdSecCnt) +
                         (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);

          fseek(ptr_file, root_address, SEEK_SET);
          int i = 0;
          for (i = 0; i < 16; i++)
          {
            fread(&dir[i], sizeof(dir[i]), 1, ptr_file);
          }

          printf("File successfully opened!!\n");
          open_file = 1;
        }
      }
    }

    //Closes the file that is open
    else if (strcmp("close", token[0]) == 0)
    {
      if (open_file == 1)
      {
        fclose(ptr_file);
        ptr_file = NULL;
        printf("File successfully closed!!\n");
        open_file = 0;
      }
      else
      {
        printf("Error: File system not open.\n");
      }
      continue;
    }

    //Exits from the mfs shell
    else if (strcmp("exit", token[0]) == 0)
    {
      if (open_file == 1)
      {
        fclose(ptr_file);
      }
      printf("Bye! Exiting....\n");
      break;
    }

    //info, stat, ls, get, cd, read works only when the fat32 image is open.
    else if (((strcmp("info", token[0]) == 0) || (strcmp("stat", token[0]) == 0) ||
              (strcmp("ls", token[0]) == 0) || (strcmp("cd", token[0]) == 0) ||
              (strcmp("get", token[0]) == 0) || (strcmp("read", token[0]) == 0)) &&
             (open_file == 0))
    {
      printf("Error: File system must be opened first.\n");
    }

    else if (((strcmp("info", token[0]) == 0) || (strcmp("stat", token[0]) == 0) ||
              (strcmp("ls", token[0]) == 0) || (strcmp("cd", token[0]) == 0) ||
              (strcmp("get", token[0]) == 0) || (strcmp("read", token[0]) == 0)) &&
             (open_file == 1))
    {
      //Prints out all the information abou the fat32 image file.
      if (strcmp("info", token[0]) == 0)
      {
        printf("BPB_BytesPerSec : %d\n", BPB_BytesPerSec);
        printf("BPB_BytesPerSec : %x\n", BPB_BytesPerSec);
        printf("\n");

        printf("BPB_SecPerClus : %d\n", BPB_SecPerClus);
        printf("BPB_SecPerClus : %x\n", BPB_SecPerClus);
        printf("\n");

        printf("BPB_RsvdSecCnt : %d\n", BPB_RsvdSecCnt);
        printf("BPB_RsvdSecCnt : %x\n", BPB_RsvdSecCnt);
        printf("\n");

        printf("BPB_NumFATs : %d\n", BPB_NumFATs);
        printf("BPB_NumFATs : %x\n", BPB_NumFATs);
        printf("\n");

        printf("BPB_FATSz32 : %d\n", BPB_FATSz32);
        printf("BPB_FATSz32 : %x\n", BPB_FATSz32);
        printf("\n");

        continue;
      }

      //Prints out the attribute, size, and lower cluster number
      //for the specified folder or file.
      else if (strcmp("stat", token[0]) == 0)
      {
        if (token[1] == NULL)
        {
          printf("Please specify the name of the directory.\n");
        }
        int index_counter = match(dir, token[1]);
        if (index_counter == -2)
        {
          printf("Error: File not found.\n");
        }
        else
        {
          printf("Attribute\tSize\tStarting Cluster Number\n");
          printf("%d\t\t%d\t%d\n\n", dir[index_counter].DIR_Attr,
                 dir[index_counter].DIR_FileSize, dir[index_counter].DIR_FirstClusterLow);
        }
        continue;
      }

      //Implementing ls function
      //List downs the files from a current directory when fat32 image file is open.
      else if (strcmp("ls", token[0]) == 0)
      {
        int i = 0;
        while (i < 16)
        {
          char word[12];
          memset(&word, 0, 12);
          //Checks if the fils are read only, subdirectories, 0X30.
          //Does not print the deleted file (signed char)0Xe5.
          if ((dir[i].DIR_Attr == 0x01 ||
               dir[i].DIR_Attr == 0x10 ||
               dir[i].DIR_Attr == 0x20 ||
               dir[i].DIR_Attr == 0x30) &&
              dir[i].DIR_Name[0] != (signed char)0xe5)
          {
            strncpy(word, dir[i].DIR_Name, 11);
            printf("%s\n", word);
          }
          i++;
        }
        continue;
      }

      //Implementing cd function.
      //Supports absolute and relative path.
      //Changes the directory as directed.
      else if (strcmp("cd", token[0]) == 0)
      {
        int find = 0;
        if (token[1] == NULL)
        {
          printf("Error: Please enter the name of the directory.\n");
        }
        else
        {
          int counter = 0;
          //Using strcmp to check if there is "." or "..".
          if (!strcmp(token[1], "..") || !strcmp(token[1], "."))
          {
            while (counter < 16)
            {
              //Compare function didn't work for "." and "..",
              //so using strstr to compare.
              if (strstr(dir[counter].DIR_Name, token[1]) != NULL)
              {
                //We need to set the cluster 20 2, if our parent
                //and roor directory are same.
                if (dir[counter].DIR_FirstClusterLow == 0)
                {
                  dir[counter].DIR_FirstClusterLow = 2;
                }
                fseek(ptr_file, LBAToOffset(dir[counter].DIR_FirstClusterLow), SEEK_SET);
                int i = 0;
                for (i = 0; i < 16; i++)
                {
                  fread(&dir[i], sizeof(dir[i]), 1, ptr_file);
                }
                break;
              }
              counter++;
            }
          }
          else
          {
            //Using compare function to compare the file name
            // and implemented cd.
            while (counter < 16)
            {
              char word[100];
              strncpy(word, token[1], strlen(token[1]));
              if (dir[counter].DIR_Attr != 0x20 && compare(dir[counter].DIR_Name, word))
              {
                fseek(ptr_file, LBAToOffset(dir[counter].DIR_FirstClusterLow), SEEK_SET);
                int i = 0;
                for (i = 0; i < 16; i++)
                {
                  fread(&dir[i], sizeof(dir[i]), 1, ptr_file);
                }
                find = 1;
                break;
              }
              counter++;
            }
            if (find == 0)
            {
              printf("Error: No such file or directory.\n");
            }
          }
        }
        continue;
      }

      //Get functions reads all the contents from the files in
      //fat32 image system and write that files into our local directory.
      else if (strcmp("get", token[0]) == 0)
      {
        if (token[1] == NULL)
        {
          printf("Please enter the name of the file in the ");
          printf("following format: get <filename>.\n");
        }
        else
        {
          int index_counter = match(dir, token[1]);
          if (index_counter == -2)
          {
            printf("Error: File not found.\n");
          }
          else
          {
            int cluster = dir[index_counter].DIR_FirstClusterLow;
            int size = dir[index_counter].DIR_FileSize;
            FILE *file_ptr = fopen(token[1], "w");
            fseek(ptr_file, LBAToOffset(cluster), SEEK_SET);
            char *temp_ptr = malloc(size);
            fread(temp_ptr, size, 1, ptr_file);
            fwrite(temp_ptr, size, 1, file_ptr);
            free(temp_ptr);
            fclose(file_ptr);
          }
        }
      }

      //Read function: read <filename> <position> <bytes>
      //Outputs the content of the file from that position
      //to the total number of bytes specified.
      else if (strcmp("read", token[0]) == 0)
      {
        if (token[1] == NULL || token[2] == NULL || token[3] == NULL)
        {
          printf("Please enter the file to read in the following format: ");
          printf("read <filename> <position> <number of bytes>.\n");
        }
        else
        {
          int index_counter= match(dir,token[1]);
          if(index_counter==-2)
          {
            printf("Error: File not found \n");

          }
          else
          {
            int position = atoi(token[2]);
            int NumOfBytes= atoi(token[3]);

            int cluster = dir[index_counter].DIR_FirstClusterLow;

            fseek(ptr_file, position + LBAToOffset(cluster), SEEK_SET);

            char *temp_str = malloc(NumOfBytes);

            fread(temp_str,NumOfBytes,1,ptr_file);

            printf("%s\n",temp_str);

            free(temp_str);

          }
        }
      }
    }
    //Any other comments- except for open, close, stat, info, ls, cd, get, reead.
    else
    {
      printf("Error: Invalid Command.\n");
      continue;
    }
    free(working_root);
  }
  return 0;
}

