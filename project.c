#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int debug = 0; // debug is used to determine that if we are in debug mode or not

struct mac {
char mname[8]; // macro name
char param[10][4]; // maximum 10 parameters and each parameter is maximum 3 characters
char macro[256]; // macro body
};

struct mac buffer[10]; // memory buffer for 10 macros
int m_count = 0; // number of macros

char field[10][7]; // maximum 10 fields in a line and each field is maximum 6 characters
int field_count = 0; // number of fields
int current_line = 0; // number of the current line that is being read

struct pt {
char mname[8]; // macro name
int nparams; // number of parameters
char dummy[10][4]; // maximum 10 parameters and each parameter is maximum 3 characters
char actual[10][4]; // maximum 10 parameters and each parameter is maximum 3 characters
};

FILE* output; // output file

struct pt PT; // structure for parameters
char temp[10][5]; // temp array is used instead of actual array since actual array causes problems with 4 digit parameter names like '100H'

int read(const char* filename) {
    FILE* file = fopen(filename, "r");

    if(file == NULL) {
        printf("Failed to open file in read function: %s\n", filename);
        return 0;
    }

    char line[256]; // actual line from the file
    char line_copy[256]; // copy line that is used to make token work correctly
    int macro_start = 0; // macro start is used to determine that if we are reading the beginning of a macro or the end of a macro

    for(int i = 0; i < 10; i++) { // set read arrays to null
            for(int j = 0; j < 8; j++)
                buffer[i].mname[j] = '\0';
            for(int j = 0; j < 10; j++) {
                for(int k = 0; k < 4; k++)
                    buffer[i].param[j][k] = '\0';
            }
            for(int j = 0; j < 256; j++)
                buffer[i].macro[j] = '\0';
    }

    while(fgets(line, sizeof(line), file)) {
        if(line[0] == 'P' && line[1] == 'R' && line[2] == 'O' && line[3] == 'G') { // read function reads file until 'PROG' part
            fclose(file);

            if(debug) {
                printf("Macro count: %d\n\n", m_count);

                for(int i = 0; i < m_count; i++)
                    printf("Macro name %d: %s\n", i+1, buffer[i].mname);
                printf("\n");

                for(int i = 0; i < m_count; i++) {
                    printf("Macro parameters of %s:\n", buffer[i].mname);

                    for(int j = 0; j < 10; j++) {
                        if(buffer[i].param[j][0] != '\0')
                            printf("Macro parameter %d: %s\n", j+1, buffer[i].param[j]);
                    }
                    printf("\n");
                }

                for(int i=0; i < m_count; i++)
                    printf("Macro body of %s:\n%s\n", buffer[i].mname, buffer[i].macro);
            }

            return m_count;
        }

        if(line[0] == '#') { // detect lines that start with '#'
            if(macro_start == 0) {
                macro_start = 1;

                strcpy(line_copy, line);
                sscanf(strtok(line_copy, ":"), "#%s", buffer[m_count].mname); // read first line until ':' and save macro name into 'mname'

                char* token = strtok(line, " "); // token reads line after 'MACRO' word
                token = strtok(NULL, " ");
                token = strtok(NULL, " ");

                int parameter_count = 0; // number of parameters
                while(token != NULL) {
                    token[strlen(token) - 1] = '\0'; // remove ',' or '\n' from macro parameter
                    strcpy(buffer[m_count].param[parameter_count], token); // save macro parameter into 'param'
                    parameter_count++;
                    token = strtok(NULL, " "); // read next macro parameter
                }
            }

            else {
                macro_start = 0;
                m_count++;
            }
        }

        else if(macro_start == 1)
            strcat(buffer[m_count].macro, line); // save macro body into 'macro'
    }
}

void parse(const char* filename) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        printf("Failed to open file in parse function: %s\n", filename);
        return;
    }

    char line[256];
    int inside_current_line = 0; // the number of the current line inside the function

    for(int i = 0; i < 10; i++) { // set parse arrays to null
        for(int j = 0; j < 7; j++)
            field[i][j] = '\0';
    }

    while(fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // remove '\n' from line
        field_count = 0;
        inside_current_line++;

        char* token = strtok(line, " "); // token reads line word by word by spaces
        while(token != NULL && field_count < 10) {
            strcpy(field[field_count], token);
            field_count++;
            token = strtok(NULL, " \t\n(),=’");
        }

        if(inside_current_line == current_line) { // break the loop when function reads specific line
            if(field[0][0] == '#' && field[0][1] != 'i' && field[0][2] != 'f') {
                int field_len_of_first = 0;

                for(int i = 0; i < sizeof(field[0])/sizeof(field[0][0]); i++) { // get the visible size
                    if(field[0][i] == '\0')
                        break;

                    field_len_of_first++;
                }

                for(int i = 0; i < field_len_of_first; i++) { // remove '#' from macro name
                    if(i != field_len_of_first - 1)
                        field[0][i] = field[0][i + 1];

                    else
                        field[0][field_len_of_first - 1] = '\0';
                }
            }

            break;
        }
    }

    if(debug) {
        printf("Field of line %d: ", current_line);
        for(int i = 0; i < field_count; i++)
            printf("%s ", field[i]);
        printf("\n");
    }

    fclose(file);
}

void createPT() {
    for(int i = 0; i < 8; i++) // set createPT arrays to null
        PT.mname[i] = '\0';
    for(int i = 0; i < 10; i++) {
        for(int j = 0; j < 4; j++) {
            PT.dummy[i][j] = '\0';
            PT.actual[i][j] = '\0';
        }
    }

    int index = 0;
    if(!strcmp(field[0], "#if"))
        index = 3;

    strcpy(PT.mname, field[index]); // get macro name from field array

    PT.nparams = field_count - (index + 1); // get parameter count from field count

    for(int i = 0; i < 10; i++) { // get dummy parameters from macro array
        if(!strcmp(PT.mname, buffer[i].mname))
            for(int j = 0; j < PT.nparams; j++)
                strcpy(PT.dummy[j], buffer[i].param[j]);
    }

    for(int i = 1; i < field_count - index; i++) // get actual parameters from field array
        strcpy(PT.actual[i - 1], field[i + index]);

    for(int i = 0; i < 10; i++) { // copy actual parameters to temp array
        for(int j = 0; j < 4; j++) {
            temp[i][j] = PT.actual[i][j];
            temp[i][4] = '\0';
        }
    }

    if(debug) {
        printf("\nMacro name: %s\n", PT.mname);

        printf("Parameter count: %d\n", PT.nparams);
        for(int i = 0; i < PT.nparams; i++)
            printf("Dummy parameter %d: %s\n", i+1, PT.dummy[i]);
        for(int i = 0; i < PT.nparams; i++)
            printf("Actual parameter %d: %s\n", i+1, temp[i]);
    }
}

void expand() {
    createPT();
    output = fopen("f1.asm", "a");

    int macro_number; // macro number is used to determine which macro is used in 'PROG' part
    for(int i = 0; i < m_count; i++) {
        if(!strcmp(PT.mname, buffer[i].mname))
            macro_number = i;
    }

    char macro_copy[256] = {'\0'}; // macro_copy is used to keep the original macro body unchanged
    strcpy(macro_copy, buffer[macro_number].macro);
    char* dummyParams[4] = {PT.dummy[0], PT.dummy[1], PT.dummy[2]};
    char* actualParams[4] = {temp[0], temp[1], temp[2]};

    char new_code[256] = {'\0'}; // new and expanded macro body
    char temp_code[256] = {'\0'}; // temp macro body
    char* start_position = &macro_copy[0]; // beginning of the concatenation
    char* end_position = &macro_copy[255]; // end of the macro body
    char* current_position = &macro_copy[0]; // end of the concatenation

    int i = 0;
    while(i < 3) {
        char name[4] = {'\0'}; // name of the current dummy parameter
        strcpy(name, dummyParams[i]);
        int len = strlen(name); // length of the current dummy parameter

        if(current_position != NULL)
            current_position = strstr(current_position, dummyParams[i]); // find next dummy parameter

        if(current_position != NULL && end_position - current_position >= 0) {
            if(len == 1) {
                if(*(current_position - 1) == ' ' && *current_position == name[0] && (*(current_position + 1) == '\n' || *(current_position + 1) == '\0')) { // find result matches the current dummy parameter
                    strncat(new_code, start_position, current_position - start_position);
                    strncat(new_code, actualParams[i], strlen(actualParams[i]));
                    current_position = current_position + strlen(dummyParams[i]);
                    start_position = current_position;
                }

                else { // find result does not match the current dummy parameter
                    strncat(new_code, start_position, current_position - start_position + 1);
                    start_position = current_position + 1;
                    current_position = strstr(current_position + 1, dummyParams[i]);
                }
            }

            else if(len == 2) {
                if(*(current_position - 1) == ' ' && *current_position == name[0] && *(current_position + 1) == name[1] && (*(current_position + 2) == '\n' || *(current_position + 2) == '\0')) { // find result matches the current dummy parameter
                    strncat(new_code, start_position, current_position - start_position);
                    strncat(new_code, actualParams[i], strlen(actualParams[i]));
                    current_position = current_position + strlen(dummyParams[i]);
                    start_position = current_position;
                }

                else { // find result does not match the current dummy parameter
                    strncat(new_code, start_position, current_position - start_position + 1);
                    start_position = current_position + 1;
                    current_position = strstr(current_position + 1, dummyParams[i]);
                }
            }

            else if(len == 3) {
                if(*(current_position - 1) == ' ' && *current_position == name[0] && *(current_position + 1) == name[1] && *(current_position + 2) == name[2] && (*(current_position + 3) == '\n' || *(current_position + 3) == '\0')) { // find result matches the current dummy parameter
                    strncat(new_code, start_position, current_position - start_position);
                    strncat(new_code, actualParams[i], strlen(actualParams[i]));
                    current_position = current_position + strlen(dummyParams[i]);
                    start_position = current_position;
                }

                else { // find result does not match the current dummy parameter
                    strncat(new_code, start_position, current_position - start_position + 1);
                    start_position = current_position + 1;
                    current_position = strstr(current_position + 1, dummyParams[i]);
                }
            }
        }

        else { // copy new code to temp code and start searching for the next dummy parameter
            strncat(new_code, start_position, end_position - start_position);
            strcpy(temp_code, new_code);
            i++;
            start_position = &temp_code[0];
            end_position = &temp_code[255];
            current_position = &temp_code[0];
            if(i < 3) {
                for(int j = 0; j < 256; j++)
                    new_code[j] = '\0';
            }
        }
    }

    fprintf(output, "\n%s\n", new_code);

    if(debug)
        printf("\nExpanded body:\n%s\n", new_code);
}

void is_macro(int argc, char argv[], const char* line) {
    output = fopen("f1.asm", "a");

    if(line[0] == '#' && line[1] != 'i') // macro lines
        expand();

    else if(line[0] == '#' && line[1] == 'i' && line[2] == 'f') { // condition lines
        int argument_number = atoi(&field[1][1]); // the index of the condition number in the argv array
        int current_number = 0; // the current index
        char expected_value[256] = {'\0'}; // expected value for the condition
        strcpy(expected_value, field[2]);
        char entered_value[256] = {'\0'}; // entered value for the condition by the user

        int index = 0;
        for(int i = 0; i < 256; i++) { // get entered value from argv array
            if(argv[i] == 0) {
                current_number++;
                while(argument_number == current_number) {
                    i++;
                    if(argv[i] == 0)
                        break;
                    entered_value[index] = argv[i];
                    index++;
                }
            }
        }

        if(argument_number >= 1 && argument_number < argc && !strcmp(expected_value, entered_value)) // if the entered value satisfies the condition expand the macro
            expand();

        if(debug) {
            printf("argc: %d\n", argc);
            printf("Argument number: %d\n", argument_number);
            printf("Expected value: %s\n", expected_value);
            printf("Entered value: %s\n\n", entered_value);
        }
    }

    else
        fprintf(output, "%s", line);

    fclose(output);
}

int main(int argc, char* argv[]) {
    debug = 0;
    FILE* file = fopen(argv[1], "r");
    char line[256];
    int prog_start = 0; // parse and is_macro function reads file after 'PROG' part

    read(argv[1]);

    while(fgets(line, sizeof(line), file)) { // read file line by line
        current_line++;

        if(line[0] == 'P' && line[1] == 'R' && line[2] == 'O' && line[3] == 'G')
            prog_start = 1;

        if(prog_start) {
            parse(argv[1]);
            is_macro(argc, *argv, line);
        }
    }
};