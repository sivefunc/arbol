// Libraries needed, here's a quick summary of why i'm using it:
#define _DEFAULT_SOURCE // scandir and alphasor enabling it.
#include <dirent.h>     // To read directory
#include <sys/stat.h>   // To get file properties
#include <stdbool.h>    // bool, true and false macros.
#include <stdlib.h>     // malloc, free and others.
#include <inttypes.h>   // intN_t, uintN_t and PRI macros.
#include <stdio.h>      // fprintf, printf and more.
#include <locale.h>     // unicode support
#include <string.h>     // strcpy, strcat.
#include <errno.h>      // global error variable "errno" and error macros.
#include <argp.h>       // parsing of arguments on command line.
#include <argz.h>       // argz list.

// Default characters used for graphics
#define PIPE_VER    "\U00002502"
#define PIPE_HOR    "\U00002500"
#define PIPE_CORNER "\U00002514"
#define PIPE_T_FORM "\U0000251c"

// Default argument values for the options on command line.
#define DEFAULT_TAB_SIZE 4
#define DEFAULT_MAX_LEVEL UINT16_MAX    // I was thinking of using -1 instead
                                        // for inf recursion but I don't think
                                        // that is possible to have 65535 levels
                                        // of depth except for stress testing?
#define DEFAULT_ALL false
// End of default values for options

// General message containing prog_name, author, license and year.
const char *argp_program_version =
"arbol v1.0.0\n"
"Copyright (C) 2024 Sivefunc\n"
"License GPLv3+: GNU GPL version 3 or later"
    "<https://gnu.org/licenses/gpl.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"\n"
"Written by a human\n";

// CLI options.
static struct argp_option options[] =
{
    // Quick summary
    {
        "help",                         // Long name of option
        'h',                            // Short name of option
        0,                              // Name of the arg, e.g NUM.
        0,                              // Flags
        "display this help and exit",   // Doc. of option useful for help.
        -1                              // Order of appearance in help
                                        // -1 means last, pos. value get first.
    },
    {"version", 'v', 0, 0, "output version information and exit", -1},

    {"tabsize", 't', "NUM", 0,
        "number of character used at each indentation level, "
         "if size <= 1 no character are printed.", 1},

    {"max_level", 'L', "NUM", 0,
        "max display depth of the directory tree." , 1},

    {"all", 'a', 0, 0,
        "all files are printed. By default arbol does not print hidden files "
        "(those  beginning with a dot `.').  In no event does arbol print the "
        "file system constructs `.' (current directory) and  `..'  (previous "
        "directory).", 1},

    {0}
};

// args for options.
typedef struct Arguments
{
    char *dirs;
    size_t dirs_len;
    uint16_t tab_size;
    uint16_t max_level;
    bool all;
} Arguments;

// Prototypes
// parse arguments given to options or alone.
static error_t parse_opt(int32_t key, char *arg, struct argp_state *state);

// Recursive find files and print.
int16_t get_files(
        char *path,
        uint16_t current_level,
        uint16_t max_level,
        uint16_t tab_size,
        bool all);

// Used in get_files to print file.
void print_file(
        char *file_name,
        uint16_t tab_size,
        uint16_t current_level,
        bool is_last);

// Global variables used in get_files
static uint16_t TOTAL_DIRECTORIES;
static uint16_t TOTAL_FILES;
static bool *DIR_TREE;
static uint16_t DIR_TREE_DEPTH;

// DIR_TREE is used for correct graphic printing.
// 0 aka false means that in this indentation level there are files on directory
// 1 aka true means that in this indent. level there are not files on dir.
//
// e.g:
//
// dir1
// |--- dir2
// |    |
// |----|----file1
// |---------file2 [end of dir]
// |    #
// |    #
// |----file3 [ end of dir]
//
// the hash symbol '#' means there are not files on that indent level so 
// a pipe vertical won't be printed. DIR_TREE at that level would be set to
// 1 aka true.

// Entry point
int32_t main(int32_t argc, char *argv[])
{
    struct argp argp =
    {
        options,
        parse_opt,
        0,
        "list contents of directories in a tree-like format\v"
        "Written by Sivefunc",
        0,
        0,
        0
    };

    Arguments args =
    {
        .dirs = NULL,
        .dirs_len = 0,
        .tab_size = DEFAULT_TAB_SIZE,
        .max_level = DEFAULT_MAX_LEVEL,
        .all = DEFAULT_ALL
    };

    // Succesfull parsing
    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &args) == 0)
    {
        // Enabling unicode printing
        setlocale(LC_ALL, "");

        if (args.dirs_len == 0)
        {
            args.dirs_len = 2;
            args.dirs = "./\0"; // Search current directory
        }

        uint16_t current_level;
        char *dir = NULL;
        DIR_TREE = NULL;
        while ((dir = argz_next(args.dirs, args.dirs_len, dir)) != NULL)
        {
            // Restarting global and local variables for each directory.
            free(DIR_TREE);
            DIR_TREE_DEPTH = 1;
            DIR_TREE = malloc(sizeof(bool) * DIR_TREE_DEPTH);
            TOTAL_FILES = 0;
            TOTAL_DIRECTORIES = 1;
            current_level = 1;

            // It's null terminated, so it's safe >:)
            uint16_t dir_name_len = strlen(dir);
            char *file_name = malloc(sizeof(char) * (dir_name_len + 1));
            strcpy(file_name, dir);

            // Add the '/' symbol to make it a directory.
            if (dir[dir_name_len - 1] != '/')
            {
                file_name = realloc(file_name,
                        sizeof(char) * (dir_name_len + 2));
                strcat(file_name, "/");
            }

            printf("%s\n", file_name);
            get_files(file_name, current_level, args.max_level, args.tab_size,
                    args.all);

            printf("%"PRIu16 " directories, %"PRIu16 " files\n",
                    TOTAL_DIRECTORIES, TOTAL_FILES);
            free(file_name);
        }
    }
    return EXIT_SUCCESS;
}

// Recursive find files and print.
int16_t get_files(
        char *path,
        uint16_t current_level,
        uint16_t max_level,
        uint16_t tab_size,
        bool all)
{
    struct dirent **list_of_files;
    int16_t file_count = scandir(path, &list_of_files, 0, alphasort);
    
    // Current file
    struct stat properties;
    char *file_path;
    char *file_name;
    uint16_t file_path_len;

    // No files on directory or directory doesn't exist
    if (file_count <= 0)
    {
        stat(path, &properties);
        if ((properties.st_mode & S_IFMT) != S_IFREG)
        {
            fprintf(stdout, "%s  [error opening dir]\n", path);
        }
    }

    for (int16_t file = 0; file < file_count; file++)
    {
        file_name = list_of_files[file] -> d_name;
        file_path_len = strlen(path) + strlen(file_name);
        file_path = (char *)calloc(file_path_len + 2, 1); // null terminator and
                                                          // possible '/' char.
        strcpy(file_path, path);
        strcat(file_path, file_name);
        stat(file_path, &properties); // Get properties of current file

        // is this the last file on this indentation level?
        DIR_TREE[current_level - 1] = file == (file_count - 1);

        // Delete this and you will end up on infinite recursion
        // why would u wnat to know . and .. files?
        // in that case put it on body of S_IFDIR.
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0)
        {
            free(file_path);
            continue;
        }

        // Hidden files flag not enabled.
        if (file_name[0] == '.' && !all)
        {
            free(file_path);
            continue;
        }

        if ((properties.st_mode & S_IFMT) == S_IFDIR) 
        {
            if (current_level >= max_level)
            {
                free(file_path);
                continue;
            }

            print_file(file_name, tab_size,
                    current_level, file == file_count - 1);

            TOTAL_DIRECTORIES++;
            if (file_path[file_path_len - 1] != '/')
            {
                file_path[file_path_len] = '/';
            }
            
            if (current_level + 1 > DIR_TREE_DEPTH)
            {
                DIR_TREE_DEPTH = current_level + 1;
                DIR_TREE = realloc(DIR_TREE, DIR_TREE_DEPTH * sizeof(bool));
                DIR_TREE[DIR_TREE_DEPTH - 1] = false;
            }

            get_files(file_path, current_level + 1, max_level,
                    tab_size, all);
        }

        else
        {
            print_file(file_name, tab_size,
                    current_level, file == file_count - 1);

            TOTAL_FILES++;
        }
        free(file_path);
    }

    return file_count;
}

// Used in get_files to print file.
void print_file(
        char *file_name,
        uint16_t tab_size,
        uint16_t current_level,
        bool is_last)
{
    // no characters are printed if tab_size <= 1, just the file_name.
    if (tab_size > 1)
    {
        // Add indentation except for the last level
        // check down.
        for (uint16_t i = 0; i < current_level - 1; i++)
        {
            // add vertical pipe then white spaces.
            // do not add vertical pipe at a given level if it doesn't contain
            // files. (in case of doubt, check global variables declaration)
            for (uint16_t j = 0; j < tab_size; j++)
            {
                printf("%s", DIR_TREE[i] == false && j % tab_size == 0 ? 
                        PIPE_VER : " ");
            }
        }   

        // for the last indentation add this.
        //
        // ?___ file_name, where '?' is PIPE_CORNER
        // if this is the last file on directory else a PIPE_T_FORM to continue
        // to the next file.
        printf("%s", is_last ? PIPE_CORNER : PIPE_T_FORM);
        for (uint16_t i = 0; i < tab_size - 2; i++)
        {
            printf("%s", PIPE_HOR);
        }

        // original tree command on my machine added a blank space before
        // writing the file_name.
        printf(" ");
    }
    printf("%s\n", file_name);
}

// parses each argument, check if that argument was placed with a option
// identified by int32_t key.
static error_t parse_opt(int32_t key, char *arg, struct argp_state *state)
{
    Arguments *args = state -> input;
    errno = 0;
    char *endptr = NULL;
    switch (key) 
    {
        case 'h':
            argp_state_help(state, state -> out_stream, ARGP_HELP_STD_HELP);
            break;

        case 'v':
            fprintf(state -> out_stream, "%s", argp_program_version);
            exit(EXIT_SUCCESS);
            break;
        
        case 't':
            int32_t t_size = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (t_size < 0)
            {
                fprintf(state -> out_stream, "tab size can't be negative\n");
                exit(EXIT_FAILURE);
            }

            args -> tab_size = (uint16_t)t_size;
            break;

        case 'L':
            int32_t level = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (level < 0)
            {
                fprintf(state -> out_stream, "level depth can't be negative\n");
                exit(EXIT_FAILURE);
            }

            args -> max_level = (uint16_t)level; 
            break;

        case 'a':
            args -> all = true;
            break;

        case ARGP_KEY_ARG:
            return argz_add(&(args -> dirs), &(args -> dirs_len), arg);

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}
