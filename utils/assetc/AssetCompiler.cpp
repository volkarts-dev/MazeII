#include "AssetCompiler.h"

#include "Helper.h"
#include <spdlog/fmt/fmt.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

namespace {

class FormatSaver
{
public:
    explicit FormatSaver(std::ios& ios) :
        ios_(ios),
        def_(nullptr)
    {
        def_.copyfmt(ios_);
    }

    ~FormatSaver()
    {
        ios_.copyfmt(def_);
    }

private:
    std::ios& ios_;
    std::ios def_;

    FormatSaver(const FormatSaver&) = delete;
    FormatSaver& operator=(const FormatSaver&) = delete;
    FormatSaver(FormatSaver&&) = delete;
    FormatSaver& operator=(FormatSaver&&) = delete;
};

} // namespace

AssetCompiler::AssetCompiler(std::string ns) :
    namespace_(std::move(ns))
{
}

bool AssetCompiler::addFileAsset(const std::vector<std::string>& strips,
                                 const std::string& fileName)
{
    const fs::path path(fileName);

    std::string assetName;
    if (!strips.empty())
    {
        std::string parent = path.parent_path().string();
        for (const auto& strip : strips)
        {
            if (parent.substr(0, strip.length()) == strip && strip.length() < parent.length())
            {
                assetName = parent.substr(strip.length() + ((strip.substr(strip.size() - 1) != "/") ? 1 : 0));
                break;
            }
        }
    }
    if (!assetName.empty() && !endsWith(assetName, "/"))
        assetName += "/";
    assetName += path.filename().string();

    std::string identifier = sanitizeIdentifier(assetName);

    files_.emplace_back(fileName, assetName, identifier);

    return true;
}

bool AssetCompiler::compile(std::ostream& output)
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    output << "// auto created at " << std::put_time(std::localtime(&now), "%Y-%m-%d %X") << " by assetc\n\n";

    output << "#include <array>\n";
    output << "#include <optional>\n";
    output << "#include <span>\n\n";

    if (!namespace_.empty())
        output << "namespace " << namespace_ << " {\n\n";

    output << "namespace {\n\n";

    for (const auto& file : files_)
    {
        if (!compileFile(output, file))
            return false;
    }

    output << "} // namespace\n\n";

    for (const auto& file : files_)
    {
        output << "std::span<unsigned char> " << file.identifier << "() { return _" << file.identifier << "_data_; }\n";
    }

    if (!namespace_.empty())
        output << "\n} // namespace " << namespace_ << "\n";

    return true;
}

bool AssetCompiler::compileFile(std::ostream& output, const FileDefinition& file)
{
    std::ifstream input(file.fileName, std::ios::in | std::ios::binary);
    if (!input.good())
    {
        std::cerr << "Failed to open " << file.fileName << std::endl;
        return false;
    }

    std::ios defStreamFmt(nullptr);

    input.seekg(0, std::ios::end);
    std::size_t fileSize = static_cast<std::size_t>(input.tellg());
    input.seekg(0, std::ios::beg);

    output << "std::array<unsigned char, " << fileSize << "> _" << file.identifier << "_data_{\n";
    {
        FormatSaver fsg(output);

        output << std::setfill('0') << std::right << std::hex;

        std::vector<char> buffer(0x1000);
        size_t lineRunner = 0;
        while (!input.eof())
        {
            input.read(buffer.data(), static_cast<ssize_t>(buffer.size()));
            auto slen = input.gcount();
            if (slen < 0)
                return false;
            auto len = static_cast<size_t>(slen);

            output << "    ";
            for (size_t i = 0; i < len; i++)
            {
                if (lineRunner >= 16)
                {
                    output << "\n    ";
                    lineRunner = 0;
                }

                output << fmt::format("0x{:02X}, ", static_cast<std::byte>(buffer[i]));

                lineRunner++;
            }
        }
    }
    output << "\n};\n\n";

    return true;
}
