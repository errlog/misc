
//Set 800 dpi on Razer Diamondback 3G

#include <windows.h>
#include <iostream>

using namespace std;



void main()
{
        char rzFile[] = "\\\\.\\razerlow";
        HANDLE device;

        device = CreateFile(rzFile, GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                0, 0);
        if(device==INVALID_HANDLE_VALUE)
        {
                cout << "Invalid handle";
                return;
        }

        LPOVERLAPPED overlapped = 0;
        DWORD inBSize = 4;
        DWORD outBSize = 0;
        DWORD controlCode = 0x222450;
        DWORD nBytes;
        char *inBuff;
        char *outBuff=0;

        inBuff = new char[4];

        inBuff[0] = 0x00;
        inBuff[1] = 0x00;
        inBuff[2] = 0x00;
        inBuff[3] = 0x00;

        DeviceIoControl(device, controlCode,
                        inBuff, inBSize,
                        outBuff, outBSize,
                        &nBytes, overlapped);


        delete[] inBuff;

        CloseHandle(device);

}
