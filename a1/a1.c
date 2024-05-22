#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define PATHBUFSIZE 50000
#define BUFSIZE 50000
#define MAGIC_VALUE 'X'
#define OLDEST_VERSION 67
#define NEWEST_VERSION 118
#define VERSION_INTERVAL_SMALL 3
#define VERSION_INTERVAL_BIG 19
#define VERSION_UNIQUE_VAL 2
#define TYPE_ARRAY {36, 43, 33, 57, 35}
#define TYPE_ARRAY_LENGTH 5
#define MINIMUM_NUMBER_OF_SECTIONS_FOR_FINDALL 1
#define SECTION_TYPE_REQUESTED_FOR_FINDALL 33

typedef struct _SECTION
{
    int section_offset;
    int section_size;
    int section_type;
    char *section_name;

} SECTION;
typedef struct _HEADER
{
    SECTION *section_array;
    int version;
    int number_of_sections;
    int header_size;
    char magic;

} HEADER;

/**
 * Converts a null-terminated string [0 to length - 1] to an integer
 * @param s the string to be converted
 * @return the integer corresponding to s
*/
int convertToInt(char *s)
{
    int number = 0;
    for (int i = 0; s[i]; i++)
    {
        if ((int)s[i] - (int)'0' < 0 || (int)s[i] - (int)'0' > 9)
            return -1;
        number = number * 10 + ((int)s[i] - (int)'0');
    }
    return number;
}

/**
 * Lists the file entries inside a directory
 * @param directory_name the name of the directory we are looking through
 * @param write_criterion set to 1 if the files have to be filtered by the user's writing permission
 * @param size_criterion set to any positive integer if the files have to be filtered by a minimum size, stores the minimum size, as well
 * @param recursive validates recursive calls in sub-directories if set to 1
 * @return nothing, may print in console
*/
void list_simpl(char *directory_name, int write_criterion, int size_criterion, int recursive)
{
    //declarations
    DIR *dir;
    struct dirent *directory_entry;
    struct stat inode;
    char name[PATHBUFSIZE];

    //open the directory and check for errors
    if ((dir = opendir(directory_name)) == 0) {
        printf("ERROR\ninvalid directory path\n");
        return;
    }

    //go through the files/sub-directories inside
    while ((directory_entry = readdir(dir)) != 0)
    {
        //declare flags for listing filters
        int flag_w = 1, flag_s = 1;

        //reset filtering flags if there are active
        if (write_criterion == 1) flag_w = 0;
        if (size_criterion >= 0) flag_s = 0;

        //prepare the output path string
        snprintf(name, PATHBUFSIZE, "%s/%s", directory_name, directory_entry->d_name);

        //get information about the symbolink link of the file
        if(lstat(name, &inode)) {
            printf("Error fetching info about element of directory\n");
            return;
        }

        //remove . and .. 
        if (strcmp(directory_entry->d_name, ".") && strcmp(directory_entry->d_name, ".."))
        {
            //check for recursive filter and start recursive calls if necessary
            if (recursive == 1 && S_ISDIR(inode.st_mode)) list_simpl(name, write_criterion, size_criterion, recursive);
            
            //set filtering flags if the filtering is necessary and the requirements are met
            if (flag_s == 0 && inode.st_size > size_criterion) flag_s = 1;
            if (flag_w == 0 && (inode.st_mode & S_IWUSR)) flag_w = 1;
            
            //if current entry is valid, print it
            if (flag_s == 1 && flag_w == 1) printf("%s\n", name);
        }
    }
    
    //end of function
    closedir(dir);
    return;
}

/**
 * Main function that takes care of the list operation
 * @param path the path to the directory we want to look into
 * @param write_criterion set to 1 if the files have to be filtered by the user's writing permission
 * @param size_criterion set to any positive integer if the files have to be filtered by a minimum size, stores the minimum size, as well
 * @param recursive validates recursive calls in sub-directories if set to 1
 * @return 0 if successful, -1 if fail, may print in console
*/
int execute_list_operation(char *path, int write_criterion, int size_criterion, int recursive)
{
    struct stat fileMetadata;
    // get metadata of the file in the path
    if (stat(path, &fileMetadata) < 0) {
        printf("ERROR\ninvalid directory path\n");
        return -1;
    }

    printf("SUCCESS\n");

    // check if directory
    if (S_ISDIR(fileMetadata.st_mode)) list_simpl(path, write_criterion, size_criterion, recursive);
    return 0;
}

/**
 * checks if the given character is the required magic value
 * @param character to be checked
 * @return 0 if the character matches the magic value, -1 otherwise
*/
int check_magic(char character) {
    if(character == MAGIC_VALUE)
        return 0;
    return -1;
}

/**
 * checks if the given number is part of the valid versions
 * @param number to be checked
 * @return 0 if the version is valid, -1 otherwise
*/
int check_version(int number) {
    if(OLDEST_VERSION < number && number < NEWEST_VERSION)
        return 0;
    return -1;
}

/**
 * checks if the given number is a valid number of sections
 * @param number to be checked
 * @return 0 if the number represents an accepted number of sections, -1 otherwise
*/
int check_number_of_sections(int number) {
    if(VERSION_INTERVAL_SMALL < number && number < VERSION_INTERVAL_BIG)
        return 0;
    if(VERSION_UNIQUE_VAL == number)
        return 0;
    return -1;
}

/**
 * checks if the given number is a valid section type
 * @param number to be checked
 * @return 0 if the section type is valid, -1 otherwise
*/
int check_section_type(int number) {
    int array[] = TYPE_ARRAY;
    for(int i = 0; i < TYPE_ARRAY_LENGTH; i++)
        if(array[i] == number)
            return 0;
    return -1;
}

/**
 * converts an array of 4 hexadecimal integers to its decimal value - little endian representation 
 * @param arr[] the array of hexadecimal values represented as decimal integers
 * @return the corresponding integer value
*/
int getN(int * arr) {
    for(int i = 0; i < 4; i++)
        if(arr[i] < 0)
            arr[i] += 256;
    return arr[0] + (arr[1] << 8) + (arr[2] << 16) + (arr[3] << 24);
}

/**
 * checks if a file is a valid section file
 * @param path the relative path to the file
 * @return 0 if valid section file, -1 otherwise 
*/
int execute_parse_operation(char * path, int flag, HEADER * header, int section_number, int line_number) {
    //declarations
    struct stat inode;
    int file_descriptor;
    unsigned long int buffer_size;
    char * buffer;

    //initializations
    header->header_size = -1;
    header->magic = 0;
    header->number_of_sections = -1;
    header->version = -1;
    lstat(path, &inode);

    //get the size of the file for dynamic memory allocation
    buffer_size = inode.st_size;
    buffer = (char *)malloc(sizeof(char) * (buffer_size + 1));
    
    //open file for reading
    if((file_descriptor = open(path, O_RDONLY)) < 0) {
        if(!flag)
            printf("ERROR\ninvalid file\n");
           // printf("File does not exist!\n");
        free(buffer), buffer = NULL;
        return -1;
    }

    //read data in the buffer
    if(read(file_descriptor, buffer, buffer_size) < 0) {
        if(flag != 2)
            printf("Cannot read data!\n");
        free(buffer), buffer = NULL;
        close(file_descriptor);
        return -1;
    }
    
    close(file_descriptor);

    //go through the buffer in reverse order, compute magic and header_size
    for(int i = buffer_size - 1; i >= 0; i--) {
            if(header->magic == 0) {
                header->magic = buffer[i];
                continue;
            }
            if(header->header_size == -1) {
                int ch1 = (unsigned int)buffer[i];
                int ch2 = (unsigned int)buffer[i - 1];

                //fix negative representations
                if(ch1 < 0) ch1 += 256;
                if(ch2 < 0) ch2 += 256;
                
                //construct the header_size out of the 2 bytes
                header->header_size = (ch1 << 8) + ch2;
                break;
            }
        }

    //check magic number
    if(check_magic(header->magic) < 0) {
        if(flag != 2)
            printf("ERROR\nwrong magic\n");
        free(buffer), buffer = NULL;
        return -1;
    }
    
    //go through the buffer in normal order, compute version and number_of_sections
    for(int i = buffer_size - header->header_size; i < buffer_size - 3; ++i) {
            //printf("entered\n");
            if(header->version == -1) {
                int ch1 = (int)buffer[i];
                int ch2 = (int)buffer[i + 1];

                //fix the negative representations
                if(ch1 < 0) ch1 += 256;
                if(ch2 < 0) ch2 += 256;

                //construct the header_version
                header->version = ch1 + (ch2 << 8);
                ++i;
                continue;
            }
            if(header->number_of_sections == -1) {
                int ch = (int)buffer[i];

                //fix the negative representation
                if(ch < 0) ch += 256;

                //construct the number_of_sections
                header->number_of_sections = ch;
                break;
            }
    }
    
    //check version
    if(check_version(header->version) < 0) {
        if(flag != 2)
            printf("ERROR\nwrong version\n");
        free(buffer), buffer = NULL;
        return -1;
    }

    //check number of sections
    if(check_number_of_sections(header->number_of_sections) < 0) {
        if(flag != 2)
            printf("ERROR\nwrong sect_nr\n");
        free(buffer), buffer = NULL;
        return -1;
    }

    //allocate enough space for the sections
    header->section_array = (SECTION *)malloc(sizeof(SECTION) * header->number_of_sections);

    //declare an index of the section_array[] inside the struct header
    int index = 0;

    //go through the buffer in reverse order, compute sections
    int min_index = buffer_size - header->header_size + 3;
    for(int i = buffer_size - 4; i >= min_index; --i) {
        //declarations
        int arr[4];
        int d = 3; //size of arr[]

        //construct arr[] for section_size
        for(; i >= min_index; --i) {
            arr[d--] = (int)buffer[i];
            if(d == -1)
               break;
        }
        header->section_array[index].section_size = getN(arr); 

        //reconstruct arr[] for section_offset
        i--;
        d = 3;
        for(; i >= min_index; --i) {
            arr[d--] = (int)buffer[i];
            if(d == -1)
                break;
        }
        header->section_array[index].section_offset = getN(arr);

        //reconstruct arr[] for section_type
        i--;
        d = 3;
        for(; i >= min_index; --i) {
            arr[d--] = (int)buffer[i];
            if(d == -1)
                break;
        }
        header->section_array[index].section_type = getN(arr); 

        //declarations
        char dummy_name[BUFSIZE];
        int dummy_size = 0; //size of dummy_name[]

        //construct the section_name in the static array dummy_name[]
        for(int k = i - 19; k < i; k++)
            dummy_name[dummy_size++] = buffer[k];
        dummy_name[dummy_size] = '\0';
        i -= 19; //update index of buffer[]

        //allocate and save the section_name
        header->section_array[index].section_name = (char*)malloc(BUFSIZE);
        if (header->section_array[index].section_name == NULL) {
            printf("Not enough storage space!\n");
            for(int j = 0; j < index; j++)
                free(header->section_array[j].section_name);
            free(header->section_array), free(buffer), buffer = NULL;
            return -1;
        }
        strcpy(header->section_array[index++].section_name, dummy_name);

        // check section type
        if(check_section_type(header->section_array[index - 1].section_type) < 0) {
            if(flag != 2)
                printf("ERROR\nwrong sect_types\n");
            for(int j = 0; j < index; j++)
                free(header->section_array[j].section_name);
            free(header->section_array), free(buffer), buffer = NULL;
            return -1;
        }
    }

    //findall format
    if(flag == 2) {
        //declarations
        //keeps track of how many sections with the desired type there are
        int counter = 0;

        //loop through the sections
        for(int i = 0; i < header->number_of_sections; ++i) {
            
            //check if the section_type of the current section is valid
            if(header->section_array[i].section_type == SECTION_TYPE_REQUESTED_FOR_FINDALL)
                counter++;
            
            //if the minimum number of sections is counted
            if(counter > MINIMUM_NUMBER_OF_SECTIONS_FOR_FINDALL) {
                //print the path
                printf("%s\n", path);

                //free memory
                for(int j = 0; j < index; j++)
                    free(header->section_array[j].section_name);
                free(buffer), buffer = NULL;
                free(header->section_array);
                return 0;
            }
        }
        for(int j = 0; j < index; j++)
            free(header->section_array[j].section_name);
        free(buffer), buffer = NULL;
        free(header->section_array);
        return 0;
    }

    //parse format
    if(flag == 1) {
        //print format
        printf("SUCCESS\n");
        printf("version=%d\n", header->version);
        printf("nr_sections=%d\n", header->number_of_sections);
        for(int j = header->number_of_sections - 1; j >= 0; j--)
            printf("section%d: %s %d %d\n", header->number_of_sections - j, header->section_array[j].section_name, 
                                            header->section_array[j].section_type, header->section_array[j].section_size);
    
        //end of program
        for(int j = 0; j < index; j++)
            free(header->section_array[j].section_name);
        free(buffer), buffer = NULL;
        free(header->section_array);
        return 0;
    }

    //extract format
    if(flag == 0) {
        //computing the index of the section in the header->section_array
        int i = header->number_of_sections - section_number;

        //check section validity
        if(i < 0 || i > header->number_of_sections) {
            printf("ERROR\ninvalid section\n");
            free(buffer); buffer = NULL;
            return -1;
        }

        //declarations
        //flag to check if something is printed
        int printed = 0; 

        //compute indices of the section
        int starting_index = header->section_array[i].section_offset + header->section_array[i].section_size - 1;
        int end_index = header->section_array[i].section_offset;

        //check indices for validity
        if(starting_index < 0 || starting_index > buffer_size) {
            printf("ERROR\ninvalid section\n");
            free(buffer); buffer = NULL;
            return -1;
        }
        
        if(line_number <= 0) {
            printf("ERROR\ninvalid line\n");
            free(buffer); buffer = NULL;
            return -1;
        }

        //loop through the buffer and search for the line
        for(int k = starting_index; k >= end_index; --k) {
            //if 0xA is encountered we are 1 line closer to the desired line
            if(buffer[k] == 0xA || buffer[k] == 0x0){
                if(line_number > 1)
                    starting_index = k - 1;
                line_number--;
            }

            //we found the desired line
            if(!line_number)
                break; 
        }
        if(line_number > 1) {
            printf("ERROR\ninvalid line\n");
            free(buffer); buffer = NULL;
            return -1;
        }

        //print format
        printf("SUCCESS\n");

        //print the line
        for(int k = starting_index; buffer[k] != 0xA && k >= 0 && buffer[k]; --k) {
            printf("%c", buffer[k]);
            printed = 1;   
        }
    
        if(!printed) {
            free(buffer); buffer = NULL;    
            return -1;
        }
        else
            printf("\n");
    }

    free(buffer); buffer = NULL;    
    return 0;
}

/**
 * Prints the line_number-th line in the section_number-th section of the file specified by path
 * @param path the path to the file to contain the searched line
 * @param section_number the number of the section where the line is
 * @param line_number the number of the desired line
 * @return 0 if success, -1 otherswise
*/
int execute_extract_operation(char * path, int section_number, int line_number) {
    //declarations
    HEADER * header = (HEADER*)malloc(sizeof(HEADER));
    
    //call parse with flag 0 and specific parameters
    if(execute_parse_operation(path, 0, header, section_number, line_number) < 0) {
        for(int j = 0; j < header->number_of_sections; j++)
            free(header->section_array[j].section_name);
        free(header->section_array);
        free(header); header = NULL;
        return -1;
    }
    
    for(int j = 0; j < header->number_of_sections; j++)
        free(header->section_array[j].section_name);
    free(header->section_array); 
    free(header); header = NULL;
    return 0;
}

/** searches through the file tree recursively for section files that meet some criteria starting at directory specified by path 
 * @param path the relative path to the starting directory
 * @param print_flag used to know when to print "SUCCESS"
 * @return 0 if success, -1 if errors are encountered
*/
int execute_findall_operation(char * path, int print_flag) {
    //declarations
    struct stat fileMetadata;
    
    //get metadata about directory
    if (stat(path, &fileMetadata) < 0) {
        printf("ERROR\ninvalid directory path\n");
        return -1;
    }

    //if we encounter a directory
    if(S_ISDIR(fileMetadata.st_mode)) {
        //declarations to process the directory
        DIR *dir;
        struct dirent *directory_entry;
        struct stat inode;
        char newPath[PATHBUFSIZE];

        //open the directory
        if ((dir = opendir(path)) == 0) {
            printf("ERROR\ninvalid directory path\n");
            return -1;
        }
        
        //if in first call of the function print 
        if(print_flag == 1)
            printf("SUCCESS\n");

        //iterate through the directory entries
        while((directory_entry = readdir(dir)) != 0) {
            //create the path of the entry
            snprintf(newPath, PATHBUFSIZE, "%s/%s", path, directory_entry->d_name);

            //get metadata about the file
            if(lstat(newPath, &inode)) {
                printf("Error fetching data!\n");
                closedir(dir);
                return -1;
            }
            
            //if the entry is a directory recursive call, otherwise parse the file with flag 2
            if(strcmp(directory_entry->d_name, ".") && strcmp(directory_entry->d_name, "..")) {
                if (S_ISDIR(inode.st_mode))
                    execute_findall_operation(newPath, 0);
                else if(S_ISREG(inode.st_mode)) {
                    HEADER * header = (HEADER *)malloc(1 * sizeof(HEADER));
                    execute_parse_operation(newPath, 2, header, -1, -1);
                    free(header); header = NULL;
                }
            }
        }
        closedir(dir);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        exit(0); //return 0 may not be required
    }

    //first task
    if (strcmp(argv[1], "variant") == 0) printf("37962\n");

    //allocate memory for the path
    char *path = (char *)malloc(PATHBUFSIZE * sizeof(char));
    path[0] = '\0';

    //second task
    if (strcmp(argv[1], "list") == 0)
    {
        //check for command integrity: too many or not enough arguments
        if (argc > 5 || argc < 3) {
            free(path);
            path = NULL;
            exit(0);
        }

        //flag declarations
        int recursive = 0;
        int size_filter = -1;
        int write_perm_filter = 0;

        //set the flags based on command line arguments
        for (int i = 2; i < argc; ++i)
            if (strcmp(argv[i], "recursive") == 0) recursive = 1;
            else if (strstr(argv[i], "size_greater=") != NULL) {
                // if the numeric value is not valid the filter is invalidated
                size_filter = convertToInt(argv[i] + strlen("size_greater="));
            }
            else if (strcmp(argv[i], "has_perm_write") == 0) write_perm_filter = 1;
            else if (strstr(argv[i], "path=") != NULL) strcpy(path, argv[i] + strlen("path="));
            else {
                free(path);
                path = NULL;
                exit(0);
            }

        // both filters active, not required, may be tested
        if (size_filter > 0 && write_perm_filter) {
            free(path);
            path = NULL;
            exit(0);
        }

        // no path
        if (path[0] == '\0') {
            free(path);
            path = NULL;
            exit(0);
        }

        //execute operation
        if (execute_list_operation(path, write_perm_filter, size_filter, recursive) < 0) {
            free(path);
            path = NULL;
            exit(0);
        }
    }

    //third task
    else if (strcmp(argv[1], "parse") == 0) {
        //check for command integrity: if not enough or too many arguments or the argument does not contain path 
        if(argc !=  3 || strstr(argv[2], "path=") == NULL) {
            free(path);
            path = NULL;
            exit(0);
        }
        
        //store path
        strcpy(path, argv[2] + strlen("path="));

        HEADER * header = (HEADER*)malloc(sizeof(HEADER));
        if(execute_parse_operation(path, 1, header, -1, -1) < 0) {
            free(path), free(header);
            path = NULL, header = NULL;
            exit(0);
        }
        free(header), header = NULL;
    }

    //forth task
    else if (strcmp(argv[1], "extract") == 0) {
        //check for command integrity
        if(argc != 5) {
            free(path);
            path = NULL;
            exit(0);
        }

        //declarations
        int section_number = -1;
        int line_number = -1;

        //compute section_number, path and line_number
        for (int i = 2; i < argc; ++i)
            if (strstr(argv[i], "section=") != NULL) section_number = convertToInt(argv[i] + strlen("section="));
            else if (strstr(argv[i], "line=") != NULL) line_number = convertToInt(argv[i] + strlen("line="));
            else if (strstr(argv[i], "path=") != NULL) strcpy(path, argv[i] + strlen("path="));
            else {
                free(path);
                path = NULL;
                exit(0);
            }

        if(execute_extract_operation(path, section_number, line_number) < 0) {
            free(path);
            path = NULL;
            exit(0);
        }
    }

    //fifth task
    else if (strcmp(argv[1], "findall") == 0) {
        //check for command integrity: if not enough or too many arguments or the argument does not contain path 
        if(argc !=  3 || strstr(argv[2], "path=") == NULL) {
            free(path);
            path = NULL;
            exit(0);
        }

        //save the path
        strcpy(path, argv[2] + strlen("path="));

        if(execute_findall_operation(path, 1) < 0) {
            free(path);
            path = NULL;
            exit(0);
        }
    }
    
    //wrong command prompt
    else {
        free(path);
        path = NULL;
        exit(0);
    }

    free(path);
    path = NULL;
    return 0;
}