#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include "archiver.h"
#include "file_processing.h"
#include "huffman_tree.h"

//print message & error

void print_msg(const char *msg, ...) {
    va_list argptr;
    va_start(argptr, msg);
    vfprintf(stdout, msg, argptr);
    va_end(argptr);
}

void print_error(const char *msg, ...) {
    va_list argptr;
    va_start(argptr, msg);
    vfprintf(stderr, msg, argptr);
    va_end(argptr);
}

//file signature

const char magic_num[] = "MAGIC_NUMBER";

//archive positions

#define MAGIC_NUM_FILEPOS   0
#define CHECKSUM_FILEPOS    (MAGIC_NUM_FILEPOS + sizeof(magic_num) - 1)
#define FILE_NUM_FILEPOS    (CHECKSUM_FILEPOS + sizeof(uint32_t))
#define FILE_INFO_FILEPOS   (FILE_NUM_FILEPOS + sizeof(int))

//read file signature & checksum & number of files

int check_magic_num(FILE *arch) {
    file_set_pos(arch, MAGIC_NUM_FILEPOS);
    static char buf[sizeof(magic_num)] = {0};
    fread(buf, sizeof(magic_num) - 1, 1, arch);
    rewind(arch);
    return !strcmp(magic_num, buf);
}

uint32_t read_checksum(FILE *arch) {
    file_set_pos(arch, CHECKSUM_FILEPOS);
    uint32_t checksum = 0;
    fread(&checksum, sizeof(uint32_t), 1, arch);
    return checksum;
}

unsigned read_num_of_files(FILE *arch) {
    file_set_pos(arch, FILE_NUM_FILEPOS);
    unsigned file_num = 0;
    fread(&file_num, sizeof(int), 1, arch);
    return file_num;
}

//write info to the header

void write_magic_number(FILE *arch) {
    file_set_pos(arch, MAGIC_NUM_FILEPOS);
    fwrite(magic_num, sizeof(magic_num) - 1, 1, arch);
}

void write_checksum(FILE *arch, uint32_t checksum) {
    file_set_pos(arch, CHECKSUM_FILEPOS);
    fwrite(&checksum, sizeof(uint32_t), 1, arch);
}

void write_num_of_files(FILE *arch, unsigned file_num) {
    file_set_pos(arch, FILE_NUM_FILEPOS);
    fwrite(&file_num, sizeof(int), 1, arch);
}

void refresh_checksum(FILE *arch) {
    //set the position right after the checksum
    file_set_pos(arch, FILE_NUM_FILEPOS);
    //count & write the checksum to the header
    write_checksum(arch, get_checksum(arch));
}

void write_file_info(FILE *arch, char *file_name, unsigned file_size, unsigned comp_size, time_t add_time) {
    unsigned char name_size = strlen(file_name) + 1;
    fwrite(&name_size, sizeof(char), 1, arch);
    fwrite(file_name, sizeof(char), name_size, arch);
    fwrite(&file_size, sizeof(int), 1, arch);
    fwrite(&comp_size, sizeof(int), 1, arch);
    fwrite(&add_time, sizeof(time_t), 1, arch);
}

//archive header

typedef struct Header {
    char file_signature[sizeof(magic_num)];
    uint32_t checksum;
    unsigned file_num;
    char **file_name;
    unsigned *file_size;
    unsigned *comp_size;
    time_t *add_time;
} Header;

Header *read_header(FILE *arch) {
    Header *file_header = (Header*)calloc(1, sizeof(Header));
    file_set_pos(arch, MAGIC_NUM_FILEPOS);
    //magic number
    fread(file_header->file_signature, sizeof(magic_num) - 1, 1, arch);
    //checksum
    fread(&file_header->checksum, sizeof(uint32_t), 1, arch);
    //number of files
    fread(&file_header->file_num, sizeof(int), 1, arch);
    //file info
    file_header->file_name = (char**)calloc(file_header->file_num, sizeof(char*));
    file_header->file_size = (unsigned*)calloc(file_header->file_num, sizeof(int));
    file_header->comp_size = (unsigned*)calloc(file_header->file_num, sizeof(int));
    file_header->add_time = (time_t*)calloc(file_header->file_num, sizeof(time_t));
    unsigned char name_size = 0;
    for (unsigned i = 0; i < file_header->file_num; ++i) {
        //file name size
        fread(&name_size, sizeof(char), 1, arch);
        file_header->file_name[i] = (char*)calloc(name_size, sizeof(char));
        //read file name
        fread(file_header->file_name[i], sizeof(char), name_size, arch);
        //read file size
        fread(&file_header->file_size[i], sizeof(int), 1, arch);
        //read compressed file size
        fread(&file_header->comp_size[i], sizeof(int), 1, arch);
        //read add time
        fread(&file_header->add_time[i], sizeof(time_t), 1, arch);
    }
    return file_header;
}

void skip_file_info(FILE *arch) {
    unsigned char name_size = 0;
    fread(&name_size, sizeof(char), 1, arch);
    file_shift_pos(arch, name_size + 2 * sizeof(int) + sizeof(time_t));
}

void skip_header(FILE *arch) {
    unsigned file_num = read_num_of_files(arch);
    file_set_pos(arch, FILE_INFO_FILEPOS);;
    for (unsigned i = 0; i < file_num; ++i) {
        skip_file_info(arch);
    }
}

void destroy_header(Header *file_header) {
    if (file_header) {
        for (unsigned i = 0; i < file_header->file_num; ++i) {
            free(file_header->file_name[i]);
        }
        free(file_header->file_name);
        free(file_header->file_size);
        free(file_header->comp_size);
        free(file_header->add_time);
        free(file_header);
    }
}

//append to archive

unsigned compress_files(FILE *arch, FILE *temp_file, char **file_names, unsigned file_num) {
    //returns the number of successfully compressed files
    FILE *file_in = NULL;
    unsigned file_cnt = 0;
    unsigned file_beg_pos = ftell(temp_file);
    //compress the requested files
    for (unsigned i = 0; i < file_num; ++i) {
        //open an input file
        if ((file_in = fopen(file_names[i], "rb"))) {
            //compress the input file
            encode_file(file_in, temp_file);
            //write input file info to the archive
            write_file_info(arch, file_names[i], ftell(file_in),
                            ftell(temp_file) - file_beg_pos, time(NULL));
            //close the input file
            file_close(file_in);
            //change the number of compressed files & the beginning position in temp_file
            file_beg_pos = ftell(temp_file);
            ++file_cnt;
            print_msg("\t<<%s>>: added!\n", file_names[i]);
        }
        else {
            print_error("\t<<%s>>: failed to open!\n", file_names[i]);
        }
    }
    return file_cnt;
}

unsigned append_to_archive(FILE *arch, char **file_names, unsigned file_num) {
    //skip archive's header
    skip_header(arch);
    //open temporary file
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        print_error("\nFailed to add the files to the archive!\n");
        return 0;
    }
    //copy the archive to the temporary file & skip archive's header again
    concat_files(temp_file, arch);
    skip_header(arch);
    //compress the requested files & add new info to the archive's header
    unsigned file_cnt = compress_files(arch, temp_file, file_names, file_num);
    //rewind the temporary file and concatenate with the archive
    rewind(temp_file);
    concat_files(arch, temp_file);
    //close the temporary file
    file_close(temp_file);
    //rewrite the number of files
    write_num_of_files(arch, file_cnt + read_num_of_files(arch));
    //refresh the checksum
    refresh_checksum(arch);
    return file_cnt;
}

//create archive

int create_archive(const char *arch_name) {
    //open a file
    FILE *arch = fopen(arch_name, "wb+");
    if (arch == NULL) {
        return 1;
    }
    //write file signature & zero checksum & zero number of files as a placeholder
    write_magic_number(arch);
    write_checksum(arch, 0);
    write_num_of_files(arch, 0);
    //refresh the checksum
    refresh_checksum(arch);
    //close the file
    file_close(arch);
    return 0;
}

//extract

#define FILENAME_LEN 256

char strbuf[FILENAME_LEN] = {0};

char *get_file_name(const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(strbuf, FILENAME_LEN, format, argptr);
    va_end(argptr);
    return strbuf;
}

unsigned extract_files(FILE *arch, Header *header, char *files_to_extract) {
    FILE *file = NULL;
    unsigned file_cnt = 0;
    unsigned beg_pos = ftell(arch), shift = 0;
    for (unsigned i = 0; i < header->file_num; ++i) {
        file_set_pos(arch, beg_pos + shift);
        if (files_to_extract[i]) {
            file = fopen(header->file_name[i], "wb");
            if (file == NULL) {
                print_error("\t<<%s>>: failed!\n", header->file_name[i]);
            }
            else {
                decode_file(arch, file, header->file_size[i]);
                file_close(file);
                ++file_cnt;
                print_msg("\t<<%s>>: extracted!\n", header->file_name[i]);
            }
        }
        shift += header->comp_size[i];
    }
    return file_cnt;
}


unsigned extract_from_archive(FILE *arch, char **file_names, unsigned file_num) {
    //read archive header
    Header *header = read_header(arch);
    //files to extract
    char files_to_extract[header->file_num];
    memset(files_to_extract, 0, header->file_num);
    //find files to extract
    for (unsigned i = 0, j = 0; i < file_num; ++i) {
        for (j = 0; j < header->file_num; ++j) {
            if (!strcmp(header->file_name[j], file_names[i])) {
                //file was found in the archive
                files_to_extract[j] = 1;
                break;
            }
        }
        if (j == header->file_num) {
            //file was not found
            print_error("\t<<%s>> was not found in the archive!\n", file_names[i]);
        }
    }
    //extract files
    unsigned file_cnt = extract_files(arch, header, files_to_extract);
    //free resources
    destroy_header(header);
    return file_cnt;
}

unsigned extract_all(FILE *arch) {
    //read archive header
    Header *header = read_header(arch);
    //files to extract
    char files_to_extract[header->file_num]; //kostyl
    memset(files_to_extract, 1, header->file_num);
    //extract files
    unsigned file_cnt = extract_files(arch, header, files_to_extract);
    //free resources
    destroy_header(header);
    return file_cnt;
}

//remove from archive

unsigned delete_files(FILE *arch, FILE *temp_file, Header *header, char *files_to_delete) {
    //write header
    for (unsigned i = 0; i < header->file_num; ++i) {
        if (!files_to_delete[i]) {
            write_file_info(temp_file, header->file_name[i], header->file_size[i],
                            header->comp_size[i], header->add_time[i]);
        }
    }
    //write the files except from deleted
    unsigned file_cnt = 0;
    unsigned beg_pos = ftell(arch), shift = 0;
    for (unsigned i = 0; i < header->file_num; ++i) {
        file_set_pos(arch, beg_pos + shift);
        if (files_to_delete[i]) {
            print_msg("\t<<%s>>: deleted!\n", header->file_name[i]);
            ++file_cnt;
        }
        else {
            file_copy_block(arch, temp_file, header->comp_size[i]);
        }
        shift += header->comp_size[i];
    }
    return file_cnt;
}

unsigned remove_from_archive(FILE *arch, char *arch_name, char **file_names, unsigned file_num) {
    //read archive header
    Header *header = read_header(arch);
    //files to delete
    char files_to_delete[header->file_num];
    memset(files_to_delete, 0, header->file_num);
    //find files to delete
    for (unsigned i = 0, j = 0; i < file_num; ++i) {
        for (j = 0; j < header->file_num; ++j) {
            if (!strcmp(header->file_name[j], file_names[i])) {
                //file was found in the archive
                files_to_delete[j] = 1;
                break;
            }
        }
        if (j == header->file_num) {
            //file was not found
            print_error("\t<<%s>> was not found in the archive!\n", file_names[i]);
        }
    }
    //open temporary fi;e
    FILE *temp_file = tmpfile();
    if (temp_file == NULL) {
        print_error("\tFailed to delete files from the archive!\n");
        return 0;
    }
    //a placeholder for the magic number, checksum and number of files
    write_magic_number(temp_file);
    write_checksum(temp_file, 0);
    write_num_of_files(temp_file, 0);
    //delete files
    unsigned file_cnt = delete_files(arch, temp_file, header, files_to_delete);
    //rewrite the number of files and checksum
    write_num_of_files(temp_file, header->file_num - file_cnt);
    refresh_checksum(temp_file);
    rewind(temp_file);

    //close the archive & open to write the temporary file there
    file_close(arch);
    arch = fopen(arch_name, "wb");
    if (arch == NULL) {
        print_error("\tOoops...");
    }
    else {
        concat_files(arch, temp_file);
    }
    file_close(temp_file);

    //free resources
    destroy_header(header);
    return file_cnt;
}

//print archive information

void print_arch_info(FILE *arch, char *arch_name) {
    Header *file_header = read_header(arch);
    //print name
    print_msg("\n\t>>Archive name: <<%s>>\n", arch_name);
    //print checksum
    print_msg("\n\t>>Checksum: 0x%08X\n", file_header->checksum);
    //print number of files
    print_msg("\n\t>>Number of files: %u\n", file_header->file_num);
    //print file info
    if (file_header->file_num > 0) {
        print_msg("\n\t\t***File list***\n\n");
    }
    for (unsigned i = 0; i < file_header->file_num; ++i) {
        //file name
        print_msg("\t<<%s>>\n", file_header->file_name[i]);
        //file size
        print_msg("\t*File size: %u bytes\n", file_header->file_size[i]);
        //compressed file size
        print_msg("\t*Compressed file size: %u bytes\n", file_header->comp_size[i]);
        //compression ratio
        print_msg("\t*Compression: %u%%\n",
               (unsigned)((1.0 - (double)file_header->comp_size[i] / file_header->file_size[i]) * 100.0));
        //add time
        print_msg("\t*Add time: %s\n", ctime(&file_header->add_time[i]));
    }
    destroy_header(file_header);
}

//check archive's integrity

int check_archive_checksum(FILE *arch) {
    //read checksum
    uint32_t checksum = read_checksum(arch);
    //get checksum & compare
    file_set_pos(arch, FILE_NUM_FILEPOS);
    return checksum == get_checksum(arch);
}

//archiver menu

void choice_menu(char *arch_name, char **file_names, unsigned file_num, MenuOption opt) {
    if (access(arch_name, R_OK) != 0) {
        //an archive does not exist
        print_msg("\tThe file <<%s>> does not exist. Creating...\n", arch_name);
        if (create_archive(arch_name)) {
            print_error("\tFailed to create an archive!\n");
            return;
        }
    }
    print_msg("\tOpening <<%s>>...\n", arch_name);
    FILE *arch = fopen(arch_name, "rb+");
    if (arch == NULL) {
        print_error("\tFailed to open <<%s>>!\n", arch_name);
        return;
    }
    if (!check_magic_num(arch)) {
        print_error("\tThe file <<%s>> is not an archive!\n", arch_name);
        goto close_files;
    }
    if (!check_archive_checksum(arch)) {
        print_error("\tThe archive <<%s>> is corrupted!\n", arch_name);
        goto close_files;
    }
    rewind(arch);
    switch (opt) {
        case AddToArchive:
            print_msg("\tFiles added: %u\n",
                      append_to_archive(arch, file_names, file_num));
            break;
        case ExtractFromArchive:
            print_msg("\tFiles extracted: %u\n",
                       extract_from_archive(arch, file_names, file_num));
            break;
        case ExtractAll:
            print_msg("\tFiles extracted: %u\n",
                       extract_all(arch));
            break;
        case RemoveFromArchive:
            print_msg("\tFiles removed: %u\n",
                      remove_from_archive(arch, arch_name, file_names, file_num));
            break;
        case CheckIntegrity:
            print_msg("\tThe archive <<%s>> is OK!\n", arch_name);
            break;
        case PrintInfo:
            print_arch_info(arch, arch_name);
            break;
        default:
            print_error("Invalid option!\n");
            break;
    }
    close_files:
    file_close(arch);
}
