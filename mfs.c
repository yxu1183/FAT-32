#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define MAX_IMAGE_FILE_NAME 50

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
char file_open[MAX_IMAGE_FILE_NAME][MAX_COMMAND_SIZE];
int file_counter = 0;
int open_file = 0;

int LBAToOffset(int sector)
{
  return ((sector -2) * BPB_BytesPerSec)+(BPB_BytesPerSec* BPB_RsvdSecCnt)+(BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}

int compare(char IMG_Name[MAX_IMAGE_FILE_NAME],char input[MAX_IMAGE_FILE_NAME] )
{
  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok( input, "." );

  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
  {
    printf("They matched\n");
  }

  return 0;
}

int main()
{
  struct DirectoryEntry dir[16];

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }
    
    if(token[0] == NULL)
    {
      continue;
    }

    else if(strcmp("open",token[0])== 0)
    {
      if(token[1]==NULL)
      {
        printf("Error: Please enter the name of file to open.\n");
        continue;
      }
      else
      {
        ptr_file = fopen(token[1],"r");
        if(ptr_file == NULL)
        {
          printf("Error: No file system with the given name is found.\n");
          continue;
        }
        else if(open_file == 1)
        {
          printf("Error: File system with the given names is already open.\n");
          continue;
        }
        else
        {
          file_counter++;
          fseek(ptr_file,11,SEEK_SET);
          fread(&BPB_BytesPerSec, 2, 1, ptr_file);

          fseek(ptr_file, 13, SEEK_SET);
          fread(&BPB_SecPerClus, 1, 1, ptr_file);

          fseek(ptr_file, 14, SEEK_SET);
          fread(&BPB_RsvdSecCnt, 1, 2, ptr_file); 

          fseek(ptr_file, 16, SEEK_SET);
          fread(&BPB_NumFATs, 1, 1, ptr_file);

          fseek(ptr_file, 36, SEEK_SET);
          fread(&BPB_FATSz32, 1, 4, ptr_file);

          root_address = (BPB_BytesPerSec* BPB_RsvdSecCnt)+(BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);

          fseek(ptr_file,root_address,SEEK_SET);
          int i = 0;
          for(i = 0; i < 16; i++)
          {
            fread(&dir[i],sizeof(dir[i]),1,ptr_file);  
          }

          //printf("%d\n",file_open);
          printf("File successfully opened!!\n");
          for(i= 0; i < file_counter; i++)
          {
            strcpy(file_open[i],token[1]);
          }  
          open_file = 1;
          //printf("%d\n",file_open);  
        }
      }
    }

    else if(strcmp("close",token[0])== 0)
    {
      int i = 0;
      for(i = 0; i < file_counter; i++)
      {
        if(token[1] == NULL || strcmp(file_open[i],token[1]) !=0)
        {
          printf("Error: No file system with the given name is open.\n");
          continue;
        }
        else
        {
          fclose(ptr_file);
          printf("File successfully closed!\n");
        }
      }
    }

    else if(strcmp("exit",token[0])== 0)
    {
      //fclose(ptr_file);
      printf("Bye! Exiting....\n");
      return 0;
    }
    else
    {
      printf("Invalid Command.\n");
      continue;
    }
    

    free(working_root);
  }
  return 0;
}

