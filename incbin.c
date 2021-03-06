#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static int fline(char **line, size_t *n, FILE *fp) {
    if (!line || !n || !fp)
        return -1;
    if (!*line)
        if (!(*line = (char *)malloc((*n=64))))
            return -1;
    int chr = *n;
    char *pos = *line;
    for (;;) {
        int c = fgetc(fp);
        if (chr < 2) {
            *n += (*n > 16) ? *n : 64;
            chr = *n + *line - pos;
            if (!(*line = (char *)realloc(*line,*n)))
                return -1;
            pos = *n - chr + *line;
        }
        if (ferror(fp))
            return -1;
        if (c == EOF) {
            if (pos == *line)
                return -1;
            else
                break;
        }
        *pos++ = c;
        chr--;
        if (c == '\n')
            break;
    }
    *pos = '\0';
    return pos - *line;
}

int main(int argc, char **argv) {
    argc--;
    argv++;

    int ret = 0;

    if (argc == 0) {
usage:
        fprintf(stderr, "%s [-help] | <files> | [-o output]\n", argv[-1]);
        fprintf(stderr, "   -o       - output file [default is \"data.c\"]\n");
        fprintf(stderr, "   -help    - this\n");
        fprintf(stderr, "example:\n");
        fprintf(stderr, "   %s icon.png music.mp3 -o file.c\n", argv[-1]);
        return 1;
    }

    char outfile[FILENAME_MAX] = "data.c";
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-o")) {
            if (i + 1 < argc) {
                strcpy(outfile, argv[i + 1]);
                memmove(argv+i, argv+i+2, (argc-i) * sizeof(char*));
                argc -= 2; /* Eliminate "-o <thing>" */
                continue;
            }
        }
        if (!strcmp(argv[i], "-help"))
            goto usage;
    }

    FILE *out = fopen(outfile, "w");
    if (!out) {
        fprintf(stderr, "failed to open `%s' for output\n", outfile);
        return 1;
    }

    fprintf(out, "/* File automatically generated by incbin */\n");
    fprintf(out, "#ifdef __cplusplus\n");
    fprintf(out, "extern \"C\" {\n");
    fprintf(out, "#endif\n\n");

    for (int i = 0; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            fprintf(stderr, "failed to open `%s' for reading\n", argv[i]);
            fclose(out);
            return 1;
        }
        char *line = NULL;
        size_t size = 0;
        while (fline(&line, &size, fp) != -1) {
            char *inc = strstr(line, "INCBIN");
            if (!inc) continue;
            char *beg = strchr(inc, '(');
            if (!beg) continue;
            char *sep = strchr(beg, ',');
            if (!sep) continue;
            char *end = strchr(sep, ')');
            if (!end) continue;
            char *name = beg + 1;
            char *file = sep + 1;
            while (isspace(*name)) name++;
            while (isspace(*file)) file++;
            sep--;
            while (isspace(*sep)) sep--;
            *++sep = '\0';
            end--;
            while (isspace(*end)) end--;
            *++end = '\0';
            fprintf(out, "/* INCBIN(%s, %s); */\n", name, file);
            fprintf(out, "unsigned char g%sData[] = {\n    ", name);
            *--end = '\0';
            file++;
            FILE *f = fopen(file, "rb");
            if (!f) {
                fprintf(stderr, "failed to include data `%s'\n", file);
                goto end;
            } else {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);
                unsigned char *data = (unsigned char *)malloc(size);
                if (!data) {
                    fprintf(stderr, "out of memory\n");
                    fclose(f);
                    ret = 1;
                    goto end;
                }
                if (fread(data, size, 1, f) != 1) {
                    fprintf(stderr, "failed reading include data `%s'\n", file);
                    free(data);
                    fclose(f);
                    ret = 1;
                    goto end;
                }
                unsigned char count = 0;
                for (long i = 0; i < size; i++) {
                    if (count == 12) {
                        fprintf(out, "\n    ");
                        count = 0;
                    }
                    fprintf(out, i != size - 1 ? "0x%02X, " : "0x%02X", data[i]);
                    count++;
                }
                free(data);
                fclose(f);
            }
            fprintf(out, "\n};\n");
            fprintf(out, "unsigned char *g%sEnd = g%sData + sizeof(g%sData);\n", name, name, name);
            fprintf(out, "unsigned int g%sSize = sizeof(g%sData);\n", name, name);
        }
end:
        free(line);
        fclose(fp);
    }

    if (ret == 0) {
        fprintf(out, "\n#ifdef __cplusplus\n");
        fprintf(out, "}\n");
        fprintf(out, "#endif\n");
        fclose(out);
        return 0;
    }

    fclose(out);
    remove(outfile);
    return 1;
}
