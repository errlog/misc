#define INITGUID
#include <windows.h>
#include <iostream>
#include <string>
#include <GPEdit.h>


int main(int argc, char *argv[])
{
    DWORD val, val_size = sizeof(DWORD);

    if (argc != 2)
    {
        std::cout << "Error. Not enough arguments. Valid arguments are \'on\' (enable updates) and \'off\' (disable updates)" << std::endl;

        return ERROR_BAD_COMMAND;
    }
    std::string cmd(argv[1]);
    std::cout << "Received Comand: " << cmd << std::endl;
    if (cmd.compare(std::string("on")) == 0)
    {
        val = 0;
    }
    else
    if (cmd.compare(std::string("off")) == 0)
    {
        val = 1;
    }
    else
    {
        std::cout << "Invalid argument. Valid arguments are \'on\' (enable updates) and \'off\' (disable updates)" << std::endl;
        return ERROR_BAD_COMMAND;
    }


    HRESULT hr;
    IGroupPolicyObject* pGPObj = nullptr;
    HKEY lm_key, wu_key;
    GUID reg_guid = REGISTRY_EXTENSION_GUID;    
    GUID snapin_guid = CLSID_GPESnapIn;

    std::cout << "COM Init..";
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (hr != S_OK)
    {
        std::cout << "Error Initializing COM" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        return 1;
    }
    

                     
    hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL, CLSCTX_INPROC_SERVER, IID_IGroupPolicyObject, (LPVOID*) &pGPObj);
    if (hr != S_OK)
    {
        std::cout << "Error with CoCreateInstance" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;

    std::cout << "Opening Machine GPO..";
    hr = pGPObj->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);
    if (hr != S_OK)
    {
        std::cout << "Error with OpenLocalMachineGPO" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;

    std::cout << "Geting Key..";
    hr = pGPObj->GetRegistryKey(GPO_SECTION_MACHINE, &lm_key);

    if (hr != S_OK)
    {
        std::cout << "Error with GetRegistryKey" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        hr = pGPObj->Release();
        return 1;
    }
    

    hr = RegCreateKeyEx(lm_key, "Software\\Policies\\Microsoft\\Windows\\WindowsUpdate", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &wu_key, NULL);
    if (hr != ERROR_SUCCESS)
    {
        std::cout << "Error with RegCreateKeyEx" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        hr = RegCloseKey(lm_key);
        hr = pGPObj->Release();
        return 1;
    }
    std::cout << "Done" << std::endl;

    std::cout << "Setting Key..";
    hr = RegSetValueEx(wu_key, "DisableWindowsUpdateAccess", NULL, REG_DWORD, (const BYTE *)(&val), sizeof(val));
    if (hr != ERROR_SUCCESS)
    {
        std::cout << "Error with RegSetKeyValue" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        hr = RegCloseKey(lm_key);
        hr = pGPObj->Release();
        return 1;
    }
    std::cout << "Done" << std::endl;

    std::cout << "Cleaning up..";
    hr = RegCloseKey(wu_key);
    if (hr != ERROR_SUCCESS)
    {
        std::cout << "Error with RegCloseKey(wu_key)" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;        
        hr = pGPObj->Release();
        return 1;
    }

    hr = pGPObj->Save(TRUE, TRUE, &reg_guid, &snapin_guid);
    if (hr != S_OK)
    {
        std::cout << "Error with pLGPO->Save" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        hr = pGPObj->Release();
        return 1;
    }

    hr = RegCloseKey(lm_key);
    if (hr != ERROR_SUCCESS)
    {
        std::cout << "Error with RegCloseKey(lm_key)" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;
        hr = pGPObj->Release();
        return 1;
    }

    hr = pGPObj->Release();
    if (hr != S_OK)
    {
        std::cout << "Error with Release" << std::endl;
        std::cout << "HRESULT:" << std::hex << hr << std::endl;        
        return 1;
    }
    std::cout << "Done" << std::endl;
    return 0;
}