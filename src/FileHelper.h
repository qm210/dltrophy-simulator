//
// Created by qm210 on 14.05.2025.
//

#ifndef DLTROPHY_SIMULATOR_FILEHELPER_H
#define DLTROPHY_SIMULATOR_FILEHELPER_H

#include <string>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>

namespace FileHelper {

    static void ensure(const std::string& path) {
        if (std::filesystem::exists(path))
            return;

        auto absolute_path =
                std::filesystem::absolute(
                        std::filesystem::path(path)
                ).string();
        auto message = std::format("File not found: {0}", absolute_path);
        throw std::runtime_error(message);
    }

    static std::string first_if_exists(const std::string& customPath, const std::string& defaultPath) {
        return std::filesystem::exists(customPath)
                ? customPath
                : defaultPath;
    }

};

#endif //DLTROPHY_SIMULATOR_FILEHELPER_H
