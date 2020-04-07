#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct Data {
    char* section;

    char* key;
    char* keyValue;

    const char* filename;

    bool sectionFound;
    bool keyFound;
}Data;


char* getSection(const char* section_key) {
    int sectionSize = 0;
    for (size_t i = 0; i < strlen(section_key); i++)
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
    for (size_t i = 0; i < strlen(section_key); i++)
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

bool isApprovedChar(char chr)
{
    char approvedChars[] = {'[', ']', ' ', '=', '\n'};
    for (size_t i = 0; i < strlen(approvedChars); i++)
    {
        if (chr == approvedChars[i])
            return true;
    }
    return false;
}

void processLine(char line[], size_t* length, const char* filename) {
    *length = strlen(line);

    if(line[*length - 1] == '\n')
    {
        line[*length - 1] = 0;
        *length = strlen(line);
    }

    for (size_t i = 0; i < *length; i++)
    {
        if(isalpha(line[i]) == 0 && isdigit(line[i]) == 0 && !isApprovedChar(line[i]))
        {
            printf("Line \"%s\" containts corrupted value for file \"%s\".\n", line, filename);
            break;
        }
    }
}

bool checkKey(char* key, char* tmp_line) {
    size_t keySize = strlen(key);
    bool theSame = false;
    for (size_t i = 0; i < strlen(tmp_line); i++)
    {
        if (i == keySize && tmp_line[i] != ' ')
            break;

        if (i == keySize && tmp_line[i] == ' ')
        {
            theSame = true;
            break;
        }  

        if (key[i] != tmp_line[i])
            break;
    }

    return theSame;
}

char* getKeyValue(char* tmp_line) {
    int len = strlen(tmp_line) + 1; 
    int valueInd = -1;
    for(int i = 0; i < len; i++)
    {
        if (tmp_line[i] == '=')
        {
            valueInd = i + 2;
            break;
        }
    }

    if (valueInd == -1 || valueInd >= len - 1) 
        return NULL;

    int valueSize = len - valueInd;
    char value[valueSize];
    for (int i = valueInd; i < len; i++)
    {
        value[i - valueInd] = tmp_line[i];
    }
    return strndup(value, valueSize);
}

void printData(Data data) {
    if (!data.sectionFound)
    {
        printf("Section \"%s\" not found for file: \"%s\".\n", data.section, data.filename);
        return;
    }

    if (!data.keyFound)
    {
        printf("Key \"%s\" not found in "
            "section \"%s\" for file: \"%s\".\n", data.key, data.section, data.filename
        );
        return;
    }

    if (data.keyValue == NULL)
    {
        printf("File \"%s\" contains corrupted value in section: "
               "\"%s\" for key \"%s\"!\n", data.filename, data.section, data.key);
        return;
    }
    
    printf("Value for \"%s\" in section \"%s\" is: \"%s\".\n", 
        data.key, data.section, data.keyValue);
}

void freeMemory(char* lineHolder, char* section, char* key, 
                char* keyValue, Data data) {
    free(lineHolder);
    free(section);
    free(key);
    free(keyValue);
    free(data.section);
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

    Data data = {.section = getPrintSection(section), .key = key, .keyValue = NULL,
                .filename = filename, .sectionFound = false, .keyFound = false};
    
    char* keyValue = NULL;

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

        processLine(tmp_line, &length, filename);

        if(searchingKey)
        {
            if(tmp_line[0] == '[')
                searchingKey = false;
            else
            {
                bool valid_key = checkKey(key, tmp_line);
                if(valid_key)
                {
                    data.keyFound = true;

                    keyValue = getKeyValue(tmp_line); 
                    if (keyValue == NULL)
                        break;

                    data.keyValue = keyValue;
                }
            }
        }

        if(strcmp(tmp_line, section) == 0)
        {
            data.sectionFound = true;
            searchingKey = true;
        }
    }

    printData(data);
    
    freeMemory(lineHolder, section, key, keyValue, data);
    fclose(file);

    return;
}

int main(int argc, const char* argv[]) {
    if(argc < 3)
    {
        printf("\nAt least one .ini file "
            "with section.key must be passed as an argument!\n\n"
        );
        return -1;
    }

    if((argc - 1) % 2 != 0)
    {
        printf("\nEvery .ini file must be supplied with "
            "section.key as a next argument!\n\n"
        );
        return -1;
    }

    for(int i = 1; i < argc; i += 2)
    {
        printf("\n");
        processFile(argv[i], argv[i + 1]);
        printf("\n");
    }

    return 0;
}
