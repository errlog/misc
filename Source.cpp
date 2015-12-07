/*
    Title       : Logitech G100s DPI changer.
    Description : Simple tool to replace the useless logitech shit that runs on startup. 
    License     : Do what ever you want with it, but don't blame me.
    Author      : Kapil Kapre
*/

#define INITGUID
#include <Windows.h>
#include <Hidsdi.h>
#include <Usbioctl.h>
#include <setupapi.h>
#include <Objbase.h>

#include <tchar.h>
#include <Ntddmou.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <devpropdef.h>
#include <Devpkey.h>


DEFINE_GUID(GUID_DEVINTERFACE_MOUSE_USB_FILTER, 0x2c133284, 0x7f10, 0x40c2, 0xb0, 0x07, 0xa4, 0x3c, 0xd2, 0xf8, 0x02, 0x79);
DEFINE_GUID(GUID_DEVINTERFACE_MOUSE_LGS_FILTER, 0x83fdf06a, 0xedbb, 0x4213, 0x94, 0xc8, 0xa5, 0x39, 0x59, 0x3a, 0xab, 0xac);

std::wstring get_device_path_for_filter(GUID guid)
{
    std::wstring dev_path;
    
    HDEVINFO handle_dev_info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE | DIGCF_DEVICEINTERFACE);

    if (handle_dev_info == INVALID_HANDLE_VALUE)
    {
        return dev_path;
    }

    SP_DEVICE_INTERFACE_DATA dev_interface_data;
    SP_DEVINFO_DATA dev_info_data;

    dev_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dev_info_data.cbSize = sizeof(dev_info_data);

    DWORD reqSize;
    DWORD size;

    int dev_idx = 0;
    SetLastError(ERROR_SUCCESS);
    while (GetLastError() != ERROR_NO_MORE_ITEMS)
    {
        if (!SetupDiEnumDeviceInterfaces(handle_dev_info, NULL, &guid, dev_idx++, &dev_interface_data))
        {
            if (dev_idx == 1)
            {
                DWORD res = GetLastError();
                SetupDiDestroyDeviceInfoList(handle_dev_info);                
                return dev_path;
            }
            break;
        }
        SetupDiGetDeviceInterfaceDetail(handle_dev_info, &dev_interface_data, NULL, 0, &size, NULL);
        PSP_DEVICE_INTERFACE_DETAIL_DATA dev_interface_detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        dev_interface_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (SetupDiGetDeviceInterfaceDetail(handle_dev_info, &dev_interface_data, dev_interface_detail, size, &reqSize, &dev_info_data))
        {
            if (_tcsstr(dev_interface_detail->DevicePath, L"vid_046d&pid_c247") != NULL)
            {
                dev_path = std::wstring((WCHAR *)dev_interface_detail->DevicePath);
                HeapFree(GetProcessHeap(), 0, dev_interface_detail);
                break;
            }
        }
        HeapFree(GetProcessHeap(), 0, dev_interface_detail);
    }
    SetupDiDestroyDeviceInfoList(handle_dev_info);
    
    return dev_path;
}

bool send_ioctls(std::wstring usbfilt, std::wstring lgsfilt)
{
    HANDLE handle_usb_filter = CreateFileW(usbfilt.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (handle_usb_filter == INVALID_HANDLE_VALUE)
    {
        std::cout << "Error in CreateFile(handle_usb_filter)\n";
        return false;
    }

    HANDLE handle_lgs_filter = CreateFileW(lgsfilt.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (handle_lgs_filter == INVALID_HANDLE_VALUE)
    {
        std::cout << "Error in CreateFile(handle_lgs_filter)\n";
        return false;
    }

    char cmd1[] = "\x00\x00\x00\x00\x40\x02\x0e\x00\x1a\x00\x00\x00";
    char cmd2[] = "\x34\x08\x00\x00\x10\x00\x09\x00\x04\x00\x04\x00";
    char cmd3[] = "\x00\x00\x00\x00\x40\x02\x0e\x00\x1a\x00\x00\x00";
    char cmd4[] = "\x00\x00\x00\x00\x40\x02\x0b\x00\x02\x00\x00\x00";
    char cmd5[] = "\x00\x00\x00\x00\x40\x02\x0e\x00\x12\x00\x00\x00";
    char cmd6[] = "\x98\x08\x00\x00\xcc\x0c\x00\x00\xcc\x0c\x00\x00";
    char cmd7[] = "\x00\x00\x00\x00\x40\x02\x0e\x00\x1a\x00\x00\x00";
    char cmd8[] = "\x34\x08\x00\x00\x10\x00\x09\x00\x04\x00\x04\x00";
    char cmd9[] = "\x00\x00\x00\x00\x40\x02\x0e\x00\x1a\x00\x00\x00";
    char cmd10[] = "\x00\x00\x00\x00\x40\x02\x0b\x00\x02\x00\x00\x00";
    char cmd11[] = "\x00\x00\x00\x00\x40\x02\x0e\x00\x12\x00\x00\x00";
    char cmd12[] = "\x98\x08\x00\x00\xcc\x0c\x00\x00\xcc\x0c\x00\x00";

    char sm1[] = "\x34\x08\x00\x00";
    char sm2[] = "\x9a\x08\x00\x00\x00\x00\x00\x00";
    char sm3[] = "\x34\x08\x00\x00";
    char sm4[] = "\x9a\x08\x00\x00\x00\x00\x00\x00";

    char out[8];

    DWORD ret;

    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd1, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, sm1, 4, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, cmd2, 12, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd3, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd4, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd5, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, cmd6, 12, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, sm2, 8, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd7, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, sm3, 4, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, cmd8, 12, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd9, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd10, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_usb_filter, IOCTL_USB_DIAGNOSTIC_MODE_ON, cmd11, 12, out, 8, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, cmd12, 12, NULL, 0, &ret, NULL);
    DeviceIoControl(handle_lgs_filter, IOCTL_USB_GET_NODE_CONNECTION_NAME, sm4, 8, NULL, 0, &ret, NULL);

    CloseHandle(handle_usb_filter);
    CloseHandle(handle_lgs_filter);

    return true;
}

int main()
{
    std::cout << "Checking if the g100s is present...\n";
    std::wstring g100_device_path(get_device_path_for_filter(GUID_DEVINTERFACE_MOUSE));
    if (g100_device_path.size() <= 1)
    {
        std::cout << "Not found !\n";
        return 0;
    };
    std::wcout << g100_device_path << std::endl;
    std::cout << "Found !\n";
    std::cout << "Checking for USB Filter...\n";
    std::wstring g100_device_path_usbfilt(get_device_path_for_filter(GUID_DEVINTERFACE_MOUSE_USB_FILTER));
    if (g100_device_path_usbfilt.size() <= 1)
    {
        std::cout << "Not found !\n";
        return 0;
    };
    std::wcout << g100_device_path_usbfilt << std::endl;
    std::cout << "Found !\n";
    std::cout << "Checking for LG Filter...\n";
    std::wstring g100_device_path_lgsfilt(get_device_path_for_filter(GUID_DEVINTERFACE_MOUSE_LGS_FILTER));
    if (g100_device_path_lgsfilt.size() <= 1)
    {
        std::cout << "Not found !\n";
        return 0;
    };
    std::wcout << g100_device_path_lgsfilt << std::endl;;
    std::cout << "Found !\n";
    std::cout << "Sending Commands...\n";
    if (!send_ioctls(g100_device_path_usbfilt, g100_device_path_lgsfilt))
    {
        std::cout << "Error !";
        return 0;
    }
    std::cout << "Done !";
    return 0;
}