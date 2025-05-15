#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <cstdarg>

/**
 * A simplified version of the LogFile class for standalone testing
 * Mimics the functionality of the actual LogFile without dependencies
 */
class SimpleLogFile {
public:
    SimpleLogFile() {
        s_allLogFiles.push_back(this);
        m_pFile = nullptr;
    }

    ~SimpleLogFile() {
        close();

        // Remove from global registry
        for (size_t i = 0; i < s_allLogFiles.size(); i++) {
            if (s_allLogFiles[i] == this) {
                size_t last = s_allLogFiles.size() - 1;
                s_allLogFiles[i] = s_allLogFiles[last];
                s_allLogFiles.resize(last);
                break;
            }
        }
    }

    bool open(const std::string &fileName, bool overwrite = false) {
        if (m_pFile) {
            std::cout << "Error: A log file with name '" << m_fileName << "' has already been opened" << std::endl;
            return false;
        }

        // Check if file already exists
        FILE *pFile = fopen(fileName.c_str(), "rt");
        if (pFile != nullptr) {
            fclose(pFile);
            
            // If overwrite is not enabled, return an error
            if (!overwrite) {
                std::cout << "Error: Specified log file " << fileName << " already exists" << std::endl;
                return false;
            }
        }

        pFile = fopen(fileName.c_str(), "wt");
        if (pFile == nullptr) {
            std::cout << "Error: Unable to open " << fileName << " for writing" << std::endl;
            return false;
        }

        m_pFile = pFile;
        m_fileName = fileName;
        return true;
    }

    void close() {
        if (m_pFile == nullptr)
            return;
        fclose(m_pFile);
        m_pFile = nullptr;
        m_fileName = "";
    }

    void print(const char *format, ...) {
        if (m_pFile == nullptr)
            return;

        va_list ap;
        va_start(ap, format);
        vfprintf(m_pFile, format, ap);
        va_end(ap);
        
        fprintf(m_pFile, "\n");
        fflush(m_pFile);
    }

    void printNoNewLine(const char *format, ...) {
        if (m_pFile == nullptr)
            return;

        va_list ap;
        va_start(ap, format);
        vfprintf(m_pFile, format, ap);
        va_end(ap);
        
        fflush(m_pFile);
    }

    std::string getFileName() const {
        return m_fileName;
    }

    bool isOpen() const {
        return m_pFile != nullptr;
    }

    static void writeToAllLogFiles(const std::string &str) {
        for (size_t i = 0; i < s_allLogFiles.size(); i++) {
            FILE *pFile = s_allLogFiles[i]->m_pFile;
            if (pFile) {
                fprintf(pFile, "%s\n", str.c_str());
                fflush(pFile);
            }
        }
    }

private:
    FILE *m_pFile;
    std::string m_fileName;
    static std::vector<SimpleLogFile *> s_allLogFiles;
};

// Initialize the static variable
std::vector<SimpleLogFile *> SimpleLogFile::s_allLogFiles;

// Function to check if a file exists
bool fileExists(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

int main() {
    // Test 1: Basic file creation and writing
    std::cout << "Test 1: Basic file creation and writing\n";
    {
        SimpleLogFile log1;
        bool result = log1.open("test_log1.txt");
        
        if (!result) {
            std::cout << "  Failed to open log file." << std::endl;
            return 1;
        }
        
        std::cout << "  Log file opened: " << log1.getFileName() << std::endl;
        log1.print("This is a test message with a number: %d", 42);
        log1.printNoNewLine("This is part 1");
        log1.printNoNewLine(" and this is part 2");
        log1.print(""); // Just add a newline
        
        log1.close();
        std::cout << "  Log file closed\n";
    }
    
    // Test 2: Overwrite protection
    std::cout << "\nTest 2: Overwrite protection\n";
    {
        SimpleLogFile log2;
        bool result = log2.open("test_log1.txt", false); // Try to open without overwrite
        
        if (!result) {
            std::cout << "  Expected error: File already exists and overwrite is disabled\n";
        } else {
            std::cout << "  ERROR: Should not have been able to open file without overwrite flag!\n";
            log2.close();
        }
        
        // Now try with overwrite flag
        result = log2.open("test_log1.txt", true);
        if (!result) {
            std::cout << "  Unexpected error with overwrite enabled\n";
            return 1;
        }
        
        std::cout << "  Successfully opened file with overwrite flag\n";
        log2.print("This is new content after overwrite");
        log2.close();
    }
    
    // Test 3: Multiple log files and global write
    std::cout << "\nTest 3: Multiple log files and global write\n";
    {
        SimpleLogFile log3, log4;
        log3.open("test_log3.txt", true);
        log4.open("test_log4.txt", true);
        
        log3.print("Message specific to log3");
        log4.print("Message specific to log4");
        
        std::cout << "  Writing to all logs simultaneously\n";
        SimpleLogFile::writeToAllLogFiles("This message should appear in both logs");
        
        log3.close();
        log4.close();
    }
    
    // Test 4: Check file existence
    std::cout << "\nTest 4: Verifying files were created\n";
    std::cout << "  test_log1.txt exists: " << (fileExists("test_log1.txt") ? "Yes" : "No") << std::endl;
    std::cout << "  test_log3.txt exists: " << (fileExists("test_log3.txt") ? "Yes" : "No") << std::endl;
    std::cout << "  test_log4.txt exists: " << (fileExists("test_log4.txt") ? "Yes" : "No") << std::endl;
    
    std::cout << "\nAll tests completed. Check the log files for content verification.\n";
    return 0;
}
