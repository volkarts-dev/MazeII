#pragma once

#include <string>
#include <vector>

class AssetCompiler
{
public:
    AssetCompiler(std::string  ns);
    ~AssetCompiler() = default;

    bool addFileAsset(const std::vector<std::string>& strips, const std::string& fileName);

    bool createHeader(std::ostream& output);
    bool compile(std::ostream& output);

private:
    struct FileDefinition
    {
        std::string fileName;
        std::string assetName;
        std::string identifier;
    };

    bool compileFile(std::ostream& output, const FileDefinition& file);

    std::string namespace_;
    std::vector<FileDefinition> files_;
};
