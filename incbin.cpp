#include <cstdio>
#include <cstdarg>

#include <fstream>
#include <vector>
#include <string>
#include <regex>

struct include {
    include(const std::string &name_, const std::string &filename_)
        : name(name_)
        , filename(filename_)
    {
    }
    std::string name;
    std::string filename;
    std::vector<unsigned char> contents;
};

struct file {
    file(const char *name_)
        : name(name_)
    {
    }
    const char *name;
    std::vector<include> includes;
};

const char *format(const char *fmt, ...) {
    static char buffer[4096];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);
    return buffer;
}

int main(int argc, char **argv) {
    --argc;
    ++argv;

    if (argc == 0) {
usage:
        fprintf(stderr, "%s [-help] | <files> | [-o output]\n", argv[-1]);
        fprintf(stderr, "   -o       - output file [default is \"incbin.c\"]\n");
        fprintf(stderr, "   -help    - this\n");
        fprintf(stderr, "example:\n");
        fprintf(stderr, "   %s icon.png music.mp3 -o file.c\n", argv[-1]);
        return 1;
    }

    const char *outfile = "incbin.c";
    std::vector<file> files;
    for (int i = 0; i < argc; i++) {
        // [-o output]
        if (!strcmp(argv[i], "-o")) {
            if (i + 1 < argc)
                outfile = argv[i + 1];
            else
                goto usage;
            continue;
        }

        // [-help]
        if (!strcmp(argv[i], "-help"))
            goto usage;

        // [files...]
        std::ifstream input(argv[i]);
        if (input.is_open()) {
            file next(argv[i]);
            std::regex incbin(R"(INCBIN\(([\s\S]+),([\s\S]+)\))");
            for (std::string line; getline(input, line); ) {
                std::smatch matches;
                if (!std::regex_search(line, matches, incbin))
                    continue;
                if (matches.size() != 3)
                    continue;
                std::string name = matches[1].str();
                std::string filename = matches[2].str();
                // Slow but works
                while (isspace(name[0]))
                    name = name.substr(1);
                while (isspace(filename[0]))
                    filename = filename.substr(1);
                // Trim quotes around file name
                if (filename.front() == '"' && filename.back() == '"') {
                    filename = filename.substr(1);
                    filename.pop_back();
                }
                next.includes.push_back({ name, filename });
            }
            files.push_back(next);
        } else {
            fprintf(stderr, "failed to open `%s'\n", argv[i]);
            return 1;
        }
    }

    // Load the INCBIN contents
    for (auto &it : files) {
        for (auto &jt : it.includes) {
            std::ifstream data(jt.filename);
            if (!data.is_open()) {
                fprintf(stderr, "failed to load `%s'\n", jt.filename.c_str());
                return 1;
            }
            data.seekg(0, std::ios::end);
            std::streampos length(data.tellg());
            data.seekg(0, std::ios::beg);
            jt.contents.resize(static_cast<size_t>(length));
            data.read((char *)&jt.contents[0], static_cast<size_t>(length));
            data.close();
        }
    }

    // Generate the INCBIN source
    std::string contents =
R"(/* File automatically generated by incbin */
#ifdef __cplusplus
extern "C" {
#endif
)";
    for (auto &it : files) {
        for (auto &jt : it.includes) {
            const char *name = jt.name.c_str();
            contents += format("\n/* INCBIN(%s, \"%s\"); */\n", name, jt.filename.c_str());
            contents += format("const unsigned char g%sData[] = {\n    ", name);
            unsigned char count = 0;
            for (auto zt = jt.contents.begin(); zt != jt.contents.end(); ++zt) {
                if (count == 12) {
                    contents += "\n    ";
                    count = 0;
                }
                contents += format(zt != jt.contents.end() - 1 ? "0x%02X, " : "0x%02X", *zt);
                count++;
            }
            contents += "\n};\n";
            contents += format("const unsigned char *g%sEnd = g%sData + sizeof(g%sData);\n", name, name, name);
            contents += format("const unsigned int g%sSize = sizeof(g%sData);\n", name, name);
        }
    }
    contents +=
R"(
#ifdef __cplusplus
}
#endif
)";
    // Write the file
    std::ofstream output(outfile, std::ofstream::binary);
    output.write(&contents[0], contents.size());
    output.close();

    return 0;
}
