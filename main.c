#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "archiver.h"

void print_info(void) {
    printf("Info:\n");
}

void print_usage(char *app_path) {
    char *app_name = basename(app_path);
    printf("\n\tUsage:\n\n"
           ">> %s [-h]: \n\tprint application information;\n\n"
           ">> %s [-a] archive_file file_1 .. file_n: \n\tadd files to an existing archive (create it otherwise);\n\n"
           ">> %s [-x] archive_file file_1 .. file_n: \n\textract files from an existing archive;\n\n"
           ">> %s [-xall] archive_file: \n\textract all files from an existing archive;\n\n"
           ">> %s [-d] archive_file file_1 .. file_n: \n\tdelete files from an existing archive;\n\n"
           ">> %s [-dall] archive_file: \n\tdelete all files from an existing archive;\n\n"
           ">> %s [-l] archive_file: \n\ttest archive integrity;\n\n"
           ">> %s [-t] archive_file: \n\tprint archive information.\n\n",
            app_name, app_name, app_name, app_name, app_name,
            app_name, app_name, app_name);
}

int main(int argc, char *argv[])
{
    //print info
    if (argc >= 2 && !strcmp(argv[1], "-h")) {
        print_info();
        exit(0);
    }
    //print usage
    if (argc <= 2) {
        print_usage(argv[0]);
        exit(0);
    }
    MenuOption opt = InvalidOption;
    //add to archive
    if (!strcmp(argv[1], "-a")) {
        opt = AddToArchive;
    }
    //extract from archive
    else if (!strcmp(argv[1], "-x")) {
        opt = ExtractFromArchive;
    }
    //extract all files from archive
    else if (!strcmp(argv[1], "-xall")) {
        opt = ExtractAll;
    }
    //remove from archive
    else if (!strcmp(argv[1], "-d")) {
        opt = RemoveFromArchive;
    }
    //remove all files from archive
    else if (!strcmp(argv[1], "-dall")) {
        opt = RemoveFromArchive;
    }
    //check archive's integrity
    else if (!strcmp(argv[1], "-t")) {
        opt = CheckIntegrity;
    }
    //print archive's info
    else if (!strcmp(argv[1], "-l")) {
        opt = PrintInfo;
    }

    if (opt == InvalidOption) {
        //print usage
        print_usage(argv[0]);
    }
    else {
        choice_menu(argv[2], argv + 3, argc - 3, opt);
    }

    return 0;
}
