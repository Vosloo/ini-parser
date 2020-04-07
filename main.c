#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char* getSection(const char* section_key) {
    int sectionSize = 0;
    for (int i = 0; i < strlen(section_key); i++)
    {
        if(section_key[i] == '.')
            break;
        sectionSize++;
    }

    if(sectionSize == 0)
        return NULL;
    
    char section[sectionSize + 2];

    section[0] = '[';
    for (int i = 0; i < sectionSize; i++)
    {
        section[i + 1] = section_key[i];
    }
    section[sectionSize + 1] = ']';

    return strndup(section, sectionSize + 2);
}

char* getPrintSection(const char* section) {
    int sectionSize = strlen(section) - 2;
    char printSection[sectionSize];
    for (int i = 0; i < sectionSize; i++)
    {
        printSection[i] = section[i + 1];
    }

    return strndup(printSection, sectionSize);
}

char* getKey(const char* section_key) {
    int keySize = 0;
    int start = 0;
    bool begin = false;
    for (int i = 0; i < strlen(section_key); i++)
    {
        if(begin)
            keySize++;

        if(section_key[i] == '.')
        {
            start = i + 1;
            begin = true;
        }
    }

    if(keySize == 0)
        return NULL;

    char key[keySize];
    for (int i = start; i < start + keySize; i++)
    {
        key[i - start] = section_key[i];
    }

    return strndup(key, keySize);
}

void processLine(char line[], size_t* length) {
    *length = strlen(line);
    if(line[*length - 1] == '\n')
    {
        line[*length - 1] = 0;
        *length = strlen(line);
    }
}

bool checkKey(char* key, char* tmp_line) {
    for(int i = 0; i < strlen(key); i++)
    {
        if(key[i] != tmp_line[i])
            return false;
    }
    return true;
}

void processFile(const char* filename, const char* section_key) {
    FILE *file = fopen(filename, "r");
    if(file == NULL)
    {
        perror(filename);
        return; // No such file
    }
    
    char* section = getSection(section_key);
    char* key = getKey(section_key);
    if(section == NULL || key == NULL)
    {
        printf("Invalid argument: \"%s\" for file: \"%s\".\n", section_key, filename);
        return;
    }
    char* printSection = getPrintSection(section);

    bool sectionFound = false;
    bool keyFound = false;
    bool searchingKey = false;

    char* lineHolder = NULL;
    size_t length = 0;
    __ssize_t read_chars;
    while((read_chars = getline(&lineHolder, &length, file)) != -1)
    {
        if(read_chars == 1 || *lineHolder == ';')
            continue;
        
        char tmp_line[read_chars];
        strcpy(tmp_line, lineHolder);
        processLine(tmp_line, &length);

        if(searchingKey)
        {
            if(tmp_line[0] == '[')
                searchingKey = false;
            else
            {
                bool valid_key = checkKey(key, tmp_line);
                if(valid_key)
                {
                    keyFound = true;
                }
            }
        }

        if(strcmp(tmp_line, section) == 0)
        {
            sectionFound = true;
            searchingKey = true;
        }
    }

    if(!sectionFound)
        printf("Section \"%s\" not found for file: \"%s\".\n", printSection, filename);

    if(sectionFound && !keyFound)
    {
        printf("Key \"%s\" not found in "
            "section \"%s\" for file: \"%s\".\n", key, printSection, filename
        );
    }

    if(keyFound)
    {
        printf("Success!\n");
    }
    
    free(lineHolder);
    fclose(file);
    return;
}

int main(int argc, const char* argv[]) {
    if(argc < 3)
    {
        printf("At least one .ini file "
            "with section.key must be passed as an argument!\n"
        );
        return -1;
    }

    if((argc - 1) % 2 != 0)
    {
        printf("Every .ini file must be supplied with "
            "section.key as a next argument!\n"
        );
        return -1;
    }

    for(int i = 1; i < argc; i += 2)
    {
        processFile(argv[i], argv[i + 1]);
    }

    return 0;
}
