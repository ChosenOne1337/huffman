#ifndef ARCHIVER_H
#define ARCHIVER_H

typedef enum MenuOption {
    AddToArchive,
    ExtractFromArchive,
    ExtractAll,
    RemoveFromArchive,
    RemoveAll,
    CheckIntegrity,
    PrintInfo,
    InvalidOption
} MenuOption;

void choice_menu(char *arch_name, char **file_names, unsigned file_num, MenuOption opt);

#endif // ARCHIVER_H
