#include <vector>

#include <windows.h>

#include "memscan.hpp"
#include "ResourceWriter.h"

namespace
{
    ResourceWriter rw;
} // end of unnamed namespace

void ExtractBMP (unsigned long address)
{
    std::vector<unsigned char> bmp;
    unsigned long length = *(unsigned long *)(address+2); // length of entire bmp

    // The 0x28 is a kludge.  We need to know all possible values.
    if ( length != 0 && length != 0xFFFFFFFF && *(unsigned short *)(address+0x0E) == 0x28 )
    {
        for (unsigned long i=0; i < length; ++i)
        {
            bmp.push_back(*(unsigned char *)(address++));
        }
        rw.SaveResource(bmp, "", ".bmp");
    }
}

void ExtractGIF (unsigned long address)
{
    std::vector<unsigned char> gif;
    unsigned short x = *(unsigned short *)(address+6);
    unsigned short y = *(unsigned short *)(address+8);

    if (x == 0 || y == 0 || *(unsigned char *)(address+4) == 0)
    {
        return; // Invalid
    }

    while (*(unsigned char *)(address) != 0x3B)
    {
        gif.push_back(*(unsigned char *)(address++) );
    }
    gif.push_back(*(unsigned char *)(address));
    rw.SaveResource(gif, "", ".gif");
}

/**
 * Extracts a png from a given address
 */
void ExtractPNG (unsigned long address)
{
    const int HEADER_LENGTH = 8; // length of PNG header
    std::vector<unsigned char> png;
        
    // Write the header
    for (int i=0; i < HEADER_LENGTH; ++i)
    {
        png.push_back( *(unsigned char *)(address+i) );
    }
    address += 8;

    // Write the image chunk by chunk
    unsigned int length = 0;
    length |= (unsigned int)(*(unsigned char*)(address))   << 24;
    length |= (unsigned int)(*(unsigned char*)(address+1)) << 16;
    length |= (unsigned int)(*(unsigned char*)(address+2)) << 8;
    length |= (unsigned int)(*(unsigned char*)(address+3)) << 0;
    bool firstChunk = true;
    bool lastChunk  = false;
    while ((length != 0 || lastChunk) && length < 0x05000000)
    {
        // If first chunk isn't IHDR, quit
        if (firstChunk && *(unsigned long*)(address+4) != 0x52444849)
        {
            //break;
        }
        lastChunk = *(unsigned long *)(address+4) == 0x444E4549;

        for (unsigned int i=0; i < (4+4+length+4); ++i)
        {
            png.push_back( *(unsigned char*)(address++) );
        }

        // We're done!
        if (lastChunk)
        {
            break;
        }

        // Else keep going...
        length = 0;
        length |= (unsigned int)(*(unsigned char*)(address))   << 24;
        length |= (unsigned int)(*(unsigned char*)(address+1)) << 16;
        length |= (unsigned int)(*(unsigned char*)(address+2)) << 8;
        length |= (unsigned int)(*(unsigned char*)(address+3)) << 0;
        firstChunk = false; // not the first chunk anymore
    }

    // Done with extraction.  Let's write to file now
    if (png.size() > 8) 
    {
        rw.SaveResource(png, "", ".png");
    }
}

void ExtractJPEG(unsigned long address)
{
    std::vector<unsigned char> jpeg;
    unsigned short header;
    unsigned long prevAddress = 0;

    if (*(unsigned short *)(address+4) == 0)
    {
        return; // invalid
    }

    do
    {
        unsigned short len; // length of segment
        header = *(unsigned short *)(address);

        if (header != 0xD8FF && header != 0xD0FF && header != 0xD1FF &&
            header != 0xD2FF && header != 0xD3FF && header != 0xD4FF &&
            header != 0xD5FF && header != 0xD6FF && header != 0xD7FF &&
            header != 0xD9FF && header != 0xDAFF)
        {
            len = 0;
            len |= (unsigned int)(*(unsigned char *)(address+2)) << 8;
            len |= (unsigned int)(*(unsigned char *)(address+3)) << 0;
            len += 2;
        }
        else
        {
            len = 2;
        }

        if ((header & 0x00FF) != 0x00FF)
        {
//            MessageBoxA( 0, wxString::Format("Failed at: %08X", prevAddress).mb_str(), "", 0);
            return; // ERROR
        }
        prevAddress = address;
        if (header != 0xDAFF)
        {
            for (unsigned short i=0; i < len; ++i)
            {
                jpeg.push_back(*(unsigned char *)(address++));
            }
        }
        else // Start of Scan doesn't have a size marker.  Just look for 0xDAFF
        {
            while (header != 0xD9FF)
            {
                jpeg.push_back(*(unsigned char *)(address++));
                header = *(unsigned short *)(address);
            }
            jpeg.push_back('\xFF');
            jpeg.push_back('\xD9');
        }

    } while (header != 0xD9FF && jpeg.size() < 20000000); // 20 MB

    // Done with extraction.  Let's write to file now
    if (jpeg.size() > 2) 
    {
        rw.SaveResource(jpeg, "", ".jpg");
    }
}

void ExtractWAV(unsigned long address)
{
    unsigned long chunkSize = *(unsigned long *)(address+4);
    std::vector<unsigned char> wav;

    if (chunkSize > 0 && *(unsigned long *)(address+8) == 0x45564157)
    {
        for (unsigned long i=0; i < (chunkSize+8); ++i)
        {
            wav.push_back( *(unsigned char *)(address++) );
        }
        rw.SaveResource(wav, "", ".wav");
    }
}

unsigned long ExtractZIP(unsigned long address)
{
    const unsigned long MAGIC_FILE_SIG = 0x04034B50;
    const unsigned long MAGIC_DESCRIPTOR_SIG = 0x08074B50;
    const unsigned long MAGIC_CENTRAL_DIRECTORY_SIG = 0x02014B50;
    const unsigned long MAGIC_END_OF_CENTRAL_DIRECTORY_SIG = 0x06054B50;

    std::vector<unsigned char> zip;
    unsigned long magicNumber = *(unsigned long *)(address);

    if (*(unsigned short *)(address+4) == 0)
    {
        return address; // invalid
    }

    do
    {
        if (magicNumber == MAGIC_FILE_SIG)
        {
            unsigned long compressedSize = *(unsigned long *)(address+18);
            unsigned long n = *(unsigned short *)(address+26);
            unsigned long m = *(unsigned short *)(address+28);

            // Write file
            for (unsigned long i=0; i < (30+n+m+compressedSize); ++i)
            {
                zip.push_back( *(unsigned char *)(address++) );
            }
        }
        else if (magicNumber == MAGIC_DESCRIPTOR_SIG)
        {

        }
        else if (magicNumber == MAGIC_CENTRAL_DIRECTORY_SIG)
        {
            unsigned long n = *(unsigned short *)(address+28);
            unsigned long m = *(unsigned short *)(address+30);
            unsigned long k = *(unsigned short *)(address+32);

            // Write file
            for (unsigned long i=0; i < (46+n+m+k); ++i)
            {
                zip.push_back( *(unsigned char *)(address++) );
            }
        }
        else if (magicNumber == MAGIC_END_OF_CENTRAL_DIRECTORY_SIG)
        {
            unsigned long n = *(unsigned short *)(address+20);

            // Write file
            for (unsigned long i=0; i < (22+n); ++i)
            {
                zip.push_back( *(unsigned char *)(address++) );
            }
        }
        magicNumber = *(unsigned long *)(address);
    } while ( (magicNumber == MAGIC_FILE_SIG ||
               magicNumber == MAGIC_DESCRIPTOR_SIG ||
               magicNumber == MAGIC_CENTRAL_DIRECTORY_SIG ||
               magicNumber == MAGIC_END_OF_CENTRAL_DIRECTORY_SIG) && 
               zip.size() < 900000000); // 900 MB

    // Done with extraction.  Let's write to file now
    if (zip.size() > 4) 
    {
        rw.SaveResource(zip, "", ".zip");
    }

    return address;
}

void ExtractBMPSE(unsigned long address)
{
    __try
    {
        ExtractBMP(address);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void ExtractGIFSE(unsigned long address)
{
    __try
    {
        ExtractGIF(address);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void ExtractPNGSE(unsigned long address)
{
    __try
    {
        ExtractPNG(address);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void ExtractJPEGSE(unsigned long address)
{
    __try
    {
        ExtractJPEG(address);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void ExtractWAVSE(unsigned long address)
{
    __try
    {
        //ExtractWAV(address);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

unsigned long ExtractZIPSE(unsigned long address)
{
    __try
    {
        return ExtractZIP(address);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return address; // Failed
}

void WINAPI MyThread ( )
{
    // Search for PNGs and extract them
    Scanner s;
    std::list<unsigned long> addresses;
    std::list<unsigned long>::iterator iter;

    //BMP
    addresses = s.GetListFromAoB("\x42\x4D\x00\x00\x00\x00\x00\x00\x00\x00", "xx????xxxx");
    iter = addresses.begin();
    for (; iter != addresses.end(); ++iter)
    {
        ExtractBMPSE(*iter);
    }

    //GIF
    addresses = s.GetListFromAoB("\x47\x49\x46\x38\x00\x61", "xxxx?x");
    iter = addresses.begin();
    for (; iter != addresses.end(); ++iter)
    {
        ExtractGIFSE(*iter);
    }

    //PNG
    addresses = s.GetListFromAoB("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", "xxxxxxxx");
    iter = addresses.begin();
    for (; iter != addresses.end(); ++iter)
    {
        ExtractPNGSE(*iter);
    }

    //JFIF
    addresses = s.GetListFromAoB("\xFF\xD8\xFF\xE0\x00\x00\x4A\x46\x49\x46\x00", "xxxx??xxxxx");
    iter = addresses.begin();
    for (; iter != addresses.end(); ++iter)
    {
        ExtractJPEGSE(*iter);
    }

    //EXIF
    addresses = s.GetListFromAoB("\xFF\xD8\xFF\x00\x00\x00\x45\x78\x69\x66\x00", "xxx???xxxxx");
    iter = addresses.begin();
    for (; iter != addresses.end(); ++iter)
    {
        ExtractJPEGSE(*iter);
    }

    //SWF
    addresses = s.GetListFromAoB("\x46\x57\x53", "xxx");

    //WAV
    addresses = s.GetListFromAoB("\x52\x49\x46\x46\x00\x00\x00\x00\x57\x41", "xxxx");
    iter = addresses.begin();
    for (; iter != addresses.end(); ++iter)
    {
        ExtractWAVSE(*iter);
    }

    //ZIP
    addresses = s.GetListFromAoB("\x50\x4B\x03\x04\x00", "xxxx?");
    iter = addresses.begin();
    unsigned long endingAddress = 0; // .zip can contain many files.  This helps us not "recreate" the same file
    for (; iter != addresses.end(); ++iter)
    {
        if (*iter > endingAddress)
        {
            endingAddress = ExtractZIPSE(*iter);
        }
    }

    MessageBoxA(0, (std::string("Finished extracting to: ") + rw.GetLocation()).c_str(), "Finished!", 0);
}

BOOL WINAPI DllMain ( HMODULE hModule, DWORD dwReason, LPVOID lpvReserved )
{
    switch ( dwReason ) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        if ( CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MyThread, NULL, 0, NULL) == NULL ) {
            // We did not successfully create a thread.
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}