#include <stdio.h>
#include <malloc.h>
#include <string.h>

void write_line(FILE* f, const char* line) {
    fprintf(f, "\"%s\\n\"\n", line);
}

char* clone_string(const char* str) {
    char* out = calloc(1, strlen(str) + 3);
    strcpy(out, str);

    return out;
}

void string_mk_header_name(char* str) {
    char* period = strrchr(str, '.');
    char* end = str + strlen(str);
    if (period != NULL) {
        end = period;
    }
    else {
        *end = '.'; // Period is missing, add it.
    }

    // Make the file extension ".h" and terminate the string
    end[1] = 'h';
    end[2] = 0x00;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: txt2h infile\n");
        return 1;
    }
    const char* in_path = argv[1];
    char* out_path = clone_string(in_path);
    string_mk_header_name(out_path);

    FILE* in_file = fopen(in_path, "rb");
    FILE* out_file = fopen(out_path, "wb");

    if (in_file == NULL) {
        printf("Error opening input %s\n", in_path);
        return 1;
    }
    if (out_file == NULL) {
        printf("Error opening output %s\n", out_path);
        return 1;
    }

    /*
    // File & array header
    fprintf(out_file, "// Header auto-generated by txt2h, 2003-2006 by ScottManDeath\n");
    fprintf(out_file, "#ifndef  SHADER_%s_H\n", array_name);
    fprintf(out_file, "#define  SHADER_%s_H\n", array_name);
    fprintf(out_file, "\n\n");
    fprintf(out_file, "const char %s [] =\n", array_name);
    */

    // Copy lines to output with string formatting
    char buff[1024] = {0};
    while (fgets(buff, sizeof(buff), in_file)) {
        // Handle Windows carriage return first, since it comes before \n.
        char* newline = strchr(buff, '\r');
        if (newline != NULL) {
            *newline = 0x00; // Terminate the string at the line ending
        }
        else {
            // We only delete \n if there's no carriage return
            newline = strchr(buff, '\n');
            if (newline != NULL) {
                *newline = 0x00;
            }
        }
        write_line(out_file, buff);
    }

    /*
    // End the file
    fprintf(out_file, ";");
    fprintf(in_file, "\n\n");
    fprintf(out_file, "#endif  // #ifdef SHADER_%s_H\n", array_name);
     */

    // Cleanup
    fclose(out_file);
    fclose(in_file);
}
