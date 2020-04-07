#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct Data
{
    char* section;
    char* key;

    char* keyValue;
};


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

void processLine(char line[], size_t* length) {
    *length = strlen(line);
    if(line[*length - 1] == '\n')
    {
        line[*length - 1] = 0;
        *length = strlen(line);
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
    {
        printf("File contains corrupted value of key!\n");
        return NULL;
    }

    int valueSize = len - valueInd;
    char value[valueSize];
    for (int i = valueInd; i < len; i++)
    {
        value[i - valueInd] = tmp_line[i];
    }
    return strndup(value, valueSize);
}

// TODO: print values from 'data';
void printData(struct Data data) {
    if (data.section != NULL)
        printf(data.section);
    if (data.key != NULL)
        printf(data.key);
    if (data.keyValue != NULL)
        printf(data.keyValue);
}

void freeMemory(char* lineHolder, char* section, char* printSection, 
                char* key, char* keyValue) {
    free(lineHolder);
    free(section);
    free(printSection);
    free(key);
    free(keyValue);
}

void processFile(const char* filename, const char* section_key) {
    FILE *file = fopen(filename, "r");
    if(file == NULL)
    {
        perror(filename);
        return; // No such file
    }
    
    struct Data data = {.section = NULL, .key = NULL, .keyValue = NULL};

    char* section = getSection(section_key);
    char* key = getKey(section_key);
    if(section == NULL || key == NULL)
    {
        printf("Invalid argument: \"%s\" for file: \"%s\".\n", section_key, filename);
        return;
    }
    char* printSection = getPrintSection(section);
    char* keyValue = NULL;

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
                    data.key = tmp_line;

                    keyValue = getKeyValue(tmp_line); 
                    if (keyValue == NULL)
                        return;
                    
                    data.keyValue = keyValue;
                    keyFound = true;
                }
            }
        }

        if(strcmp(tmp_line, section) == 0)
        {
            data.section = tmp_line;
            sectionFound = true;
            searchingKey = true;
        }
    }

    // printData(data);

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
        printf("Value for \"%s\" in section \"%s\" is: \"%s\"\n", 
            key, printSection, keyValue);
    }
    
    freeMemory(lineHolder, section, printSection, key, keyValue);
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
