#pragma once
#include <fstream>
#include <string>

class Log {
public:
    static bool Initialize() {
        isDirt = true;
        mLogFile.open("Log_ShadowResFix.txt");
        if(!mLogFile) {
            return false;
        }

        return true;
    }

    static void Info(const std::string message) {
        LogText += "Info: " + message + "\n";
        mLogFile << "Info: " + message + "\n";
        isDirt = true;
    }

    static const char* c_str() {
        return LogText.c_str();
    }

    static void Warning(const std::string message) {
        isDirt = true;
        LogText += "Warning: " + message + "\n";
        mLogFile << "Warning: " + message + "\n";
    }

    static void Error(const std::string message) {
        isDirt = true;
        LogText += "ERROR: " + message + "\n";
        mLogFile << "ERROR: " + message + "\n";
    }
    static void Text(const std::string message) {
        isDirt = true;
        LogText += "		" + message + "\n";
        mLogFile << "		" + message + "\n";
    }
    static size_t length() {
        return LogText.length();
    }
    static bool isDirt;
private:
    static std::string LogText;
    static std::ofstream  mLogFile;
};
