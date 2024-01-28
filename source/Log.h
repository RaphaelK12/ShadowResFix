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
        mLogFile.flush();
        isDirt = true;
    }

    static const char* c_str() {
        return LogText.c_str();
    }

    static void Warning(const std::string message) {
        isDirt = true;
        LogText += "Warning: " + message + "\n";
        mLogFile << "Warning: " + message + "\n";
        mLogFile.flush();
    }

    static void Error(const std::string message) {
        isDirt = true;
        LogText += "ERROR: " + message + "\n";
        mLogFile << "ERROR: " + message + "\n";
        mLogFile.flush();
    }
    static void Text(const std::string message) {
        isDirt = true;
        LogText += "		" + message + "\n";
        mLogFile << "		" + message + "\n";
        mLogFile.flush();
    }
    static size_t length() {
        return LogText.length();
    }
    static void clear() {
        LogText.clear();
    }
    static bool isDirt;
private:
    static std::string LogText;
    static std::ofstream  mLogFile;
};
