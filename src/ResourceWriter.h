#ifndef __RESOURCE_WRITER
#define __RESOURCE_WRITER

#include <fstream>
#include <string>
#include <vector>

#include <Windows.h>

/**
 * A class used to write resources to a file
 */
class ResourceWriter
{
    // Current resource number.  Starts from 1
private: unsigned int number;

    // The base filename (e.g. C:/fuu/extracted_resources/)
private: std::string cwd;

public: ResourceWriter()
    {
        number = 0;
        char buffer[MAX_PATH];
        
        if (GetCurrentDirectoryA(MAX_PATH, buffer) > 0)
        {
            cwd = buffer;
            cwd += "/extracted_resources/";
            CreateDirectoryA(cwd.c_str(), NULL);
        }
    }

    /**
     * Saves a resource from memory to a file.
     */
public: void SaveResource( const std::vector<unsigned char> &data, 
        const std::string &folder, const std::string &suffix )
    {
        char buffer[12];
        itoa( number++, buffer, 10 );
        std::ofstream file;
        file.open(cwd + folder + std::string(buffer) + suffix, std::ios::out | std::ios::binary );
        if (file.is_open())
        {
            file.write( (char *)data.data(), data.size() );
        }
        file.close();
    }

public: std::string GetLocation()
        {
            return cwd;
        }
};

#endif //__RESOURCE_WRITER
