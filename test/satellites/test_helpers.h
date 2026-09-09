#include <filesystem>
// Get path relative to this source file
inline std::string GetTestDataPath(const char* sourceFile, const std::string& filename) {
    std::filesystem::path source_file(sourceFile);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}