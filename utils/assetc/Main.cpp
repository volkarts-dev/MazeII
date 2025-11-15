#include "AssetCompiler.h"
#include <CLI/CLI.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Options
{
    std::string outputFile;
    std::string _namespace;
    std::vector<std::string> strips;
    std::vector<std::string> inputFiles;

    bool quite{};
};

int parseCommandLine(int argc, char** argv, Options& options)
{
    CLI::App app{"Asset compiler for the ENGINE", "assetc"};

    app.add_option("-o", options.outputFile, "Output file path");
    app.add_option("-n", options._namespace, "Namespace of generated assets");

    app.add_flag("-q", options.quite, "Don't output anything but errors");

    app.add_option("-s", options.strips, "File path prefixes to strip from source files");

    app.add_option_function<std::vector<std::string>>("asset_file", [&options](const std::vector<std::string>& fileNames) {
        for (const auto& f : fileNames)
        {
            options.inputFiles.emplace_back(f);
        }
    })
    ->required();

    try
    {
        app.parse(argc, argv);
        return 0;
    }
    catch (const CLI::ParseError& e)
    {
        return app.exit(e);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    Options options;
    int ret = parseCommandLine(argc, argv, options);
    if (ret != 0)
        return ret;

    if (options.outputFile.empty())
    {
        std::cerr << "No output file specified" << std::endl;
        return 1;
    }

    std::sort(options.strips.begin(), options.strips.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.size() > rhs.size(); // longest first
    });

    AssetCompiler compiler(options._namespace);

    for (const auto& entry : options.inputFiles)
    {
        if (!compiler.addFileAsset(options.strips, entry))
            return 1;
    }

    std::ofstream fileOutput;
    fileOutput.open(options.outputFile, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fileOutput.good())
    {
        std::cerr << "Failed to open output file " << options.outputFile << std::endl;
        return 1;
    }

    if (!compiler.compile(fileOutput))
    {
        fileOutput.close();
        std::filesystem::remove(options.outputFile);
        return 1;
    }

    return 0;
}
