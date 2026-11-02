#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <system_error>
#include <vector>

#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace {

    std::string replace(std::string string, const std::string &from, const std::string &to) {
        if (!from.empty())
            for (size_t pos = 0; (pos = string.find(from, pos)) != std::string::npos; pos += to.size())
                string.replace(pos, from.size(), to);
        return string;
    }

    std::string toPathString(std::string string) {
        // Replace all backslashes with forward slashes on Windows
#if defined(_WIN32)
        string = replace(string, "\\", "/");
#endif

        // Replace all " with \"
        string = replace(string, "\"", "\\\"");

        return string;
    }

    // The resources are read into memory before they are written: a file that
    // cannot be read stops the generator with the path that failed instead of
    // writing an image with a hole in it.
    bool readFile(const fs::path &path, std::vector<char> &bytes) {
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            std::printf("[libromfs] cannot open %s\n", path.string().c_str());
            return false;
        }
        input.seekg(0, std::ios::end);
        const std::streamoff size = input.tellg();
        if (size < 0) {
            std::printf("[libromfs] cannot size %s\n", path.string().c_str());
            return false;
        }
        input.seekg(0, std::ios::beg);
        bytes.resize(static_cast<std::size_t>(size));
        if (size > 0 && !input.read(bytes.data(), size)) {
            std::printf("[libromfs] cannot read %s\n", path.string().c_str());
            return false;
        }
        return true;
    }

}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::printf("./libromfs-generator <LIBROMFS_PROJECT_NAME> <LIBROMFS_RESOURCE_LOCATION>");
        return 0;
    }

    const std::string project = argv[1];
    const fs::path root(argv[2]);

    // The order the directory iterator returns is not fixed; sorting keeps the
    // generated file identical between runs and between file systems.
    std::vector<fs::path> resources;
    std::error_code error;
    for (const auto &entry : fs::recursive_directory_iterator(root, error)) {
        if (error) break;
        if (!fs::is_regular_file(entry.path(), error)) continue;
        if (entry.path().filename().string() == ".DS_Store") {
            std::printf("[libromfs] SKIP: %s\n", entry.path().string().c_str());
            continue;
        }
        resources.push_back(entry.path());
    }
    if (error) {
        std::printf("[libromfs] cannot walk %s: %s\n", root.string().c_str(), error.message().c_str());
        return 1;
    }
    std::sort(resources.begin(), resources.end());

    std::ofstream outputFile("libromfs_resources.cpp");
    if (!outputFile) {
        std::printf("[libromfs] cannot write libromfs_resources.cpp\n");
        return 1;
    }

    std::printf("[libromfs] Resource Folder: %s\n", root.string().c_str());

    outputFile << "#include <romfs/romfs.hpp>\n\n";
    outputFile << "#include <array>\n";
    outputFile << "#include <map>\n";
    outputFile << "\n\n";
    outputFile << "/* Resource definitions */\n";

    std::vector<std::string> paths;
    for (std::size_t identifier = 0; identifier < resources.size(); identifier++) {
        std::vector<char> bytes;
        if (!readFile(resources[identifier], bytes)) return 1;

        paths.push_back(fs::relative(resources[identifier], root).generic_string());

        outputFile << "static std::array<std::uint8_t, " << bytes.size() + 1 << "> "
                   << "resource_" << project << "_" << identifier << " = {\n";
        outputFile << "    ";
        outputFile << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
        for (char byte : bytes) {
            outputFile << "0x" << static_cast<std::uint32_t>(static_cast<unsigned char>(byte)) << ", ";
        }
        outputFile << std::dec << std::nouppercase << std::setfill(' ') << std::setw(0);
        outputFile << "\n 0x00 };\n\n";
    }

    outputFile << "\n";

    {
        outputFile << "/* Resource map */\n";
        outputFile << "const std::map<fs::path, romfs::Resource>& RomFs_" << project << "_get_resources() {\n";
        outputFile << "    static std::map<fs::path, romfs::Resource> resources = {\n";

        for (std::size_t identifier = 0; identifier < resources.size(); identifier++) {
            std::printf("[libromfs] Bundling resource: %s\n", paths[identifier].c_str());

            outputFile << "        " << "{ \"" << toPathString(paths[identifier]) << "\", romfs::Resource({ "
                       << "reinterpret_cast<std::byte*>(resource_" << project << "_" << identifier << ".data()), "
                       << "resource_" << project << "_" << identifier << ".size() - 1 }) },\n";
        }
        outputFile << "    };";

        outputFile << "\n\n    return resources;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* Resource paths */\n";
        outputFile << "const std::vector<fs::path>& RomFs_" << project << "_get_paths() {\n";
        outputFile << "    static std::vector<fs::path> paths = {\n";

        for (const auto &path : paths) {
            outputFile << "        \"" << toPathString(path) << "\",\n";
        }
        outputFile << "    };";

        outputFile << "\n\n    return paths;\n";
        outputFile << "}\n\n";
    }

    outputFile << "\n\n";

    {
        outputFile << "/* RomFS name */\n";
        outputFile << "const std::string& RomFs_" << project << "_get_name() {\n";
        outputFile << "    static std::string name = \"" << project << "\";\n";
        outputFile << "    return name;\n";
        outputFile << "}\n\n";
    }

    return 0;
}
