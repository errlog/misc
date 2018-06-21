/*
    
    Ping stats

    A simple Fixed-heap program.

    Probably needs some hardening around system APIs..

    Author: Kapil Kapre

*/
#define NOMINMAX
#include <windows.h>  
#include <stdlib.h>  
#include <string.h>  
#include <tchar.h>  
#include <iphlpapi.h>
#include <IcmpAPI.h>

#include <string>
#include <vector>

#include <cstdint>
#include <cassert>

#define IVT_TIMER1 0x12345

int window_width = 0;
int window_height = 0;
HDC handle_virtual_device_context = NULL;
HBITMAP handle_bitmap = NULL;
HBRUSH handle_brush = NULL;
RECT computed_rect;


HINSTANCE handle_app_instance;
SIZE text_size;
HFONT font_fixed_width;
static TCHAR class_name[] = _T("ping_ip");
static TCHAR window_title[] = _T("pingpingping");


HANDLE handle_event_exit_thread;
HANDLE handle_event_thread_shut_complete;
HANDLE handle_event_init_once;
HANDLE handle_thread;
HANDLE icmp_handle;

CRITICAL_SECTION buffer_swap_critical_section;


const char icmp_send_buffer[] = "FillerDataThisIs";
const uint16_t icmp_send_size = sizeof(icmp_send_buffer);
const uint16_t icmp_reply_size = sizeof(ICMP_ECHO_REPLY) + sizeof(icmp_send_buffer) + 8;
char icmp_reply_buffer[icmp_reply_size];
std::string results_text_output;

struct Host_Info
{
    std::string     name;
    std::string     ip_address;
};

struct Host_Result
{
    Host_Info       host;

    unsigned long   handle_ip_address   = 0;

    std::uint64_t   number_sent         = 0;
    std::uint64_t   number_responses    = 0;
    std::uint64_t   number_timeouts     = 0;
    std::uint64_t   average_rtt         = 0;
    std::uint16_t   success_rate        = 0;

    time_t          last_time           = 0;
    time_t          last_fail_time      = 0;
    time_t          last_success_time   = 0;
};

std::vector<Host_Result> ping_results;


BOOL GetErrorMessage(DWORD error_code, LPTSTR ptr_buffer, DWORD buffer_len) noexcept
{
    if (buffer_len == 0) { return FALSE; }

    DWORD num_chars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, 
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        ptr_buffer,
        buffer_len,
        NULL);
    return (num_chars > 0);
}


void ProcessHost(Host_Result &result)
{
    if (result.number_sent == UINT64_MAX)
    {
        result.number_sent = 0;
        result.number_responses = 0;
        result.number_timeouts = 0;
        result.average_rtt = 0;
        result.success_rate = 0;
    }
    

    ZeroMemory(icmp_reply_buffer, icmp_reply_size);

    DWORD icmp_return_val = IcmpSendEcho(icmp_handle, result.handle_ip_address, 
                            (LPVOID)icmp_send_buffer, icmp_send_size, 
                            NULL, icmp_reply_buffer, icmp_reply_size, 1000);

    assert(icmp_return_val != ERROR_INSUFFICIENT_BUFFER);
    assert(icmp_return_val != ERROR_INVALID_PARAMETER);
    assert(icmp_return_val != IP_BUF_TOO_SMALL);
    
    assert(time(&result.last_fail_time) != -1);
    result.number_sent++;

    if (icmp_return_val != 0)
    {
        PICMP_ECHO_REPLY ptr_echo_reply = (PICMP_ECHO_REPLY)icmp_reply_buffer;
        assert(time(&result.last_success_time) != -1);
        result.number_responses++;

        std::uint64_t this_rtt = ptr_echo_reply->RoundTripTime;
        float temp_avg = (float)result.average_rtt * (float)(result.number_responses - 1) / (float)(result.number_responses) + (float)this_rtt / (float)(result.number_responses);        
        result.average_rtt = static_cast<std::uint64_t>(std::round(temp_avg));
    }
    else
    {
        assert(time(&result.last_fail_time) != -1);
        result.number_timeouts++;
    }
    float success = 100.0f * (float)result.number_responses / (float)result.number_sent;
    result.success_rate = static_cast<std::uint16_t>(std::round(success));
}

DWORD WINAPI DoAllPingsAndSignal(LPVOID param)
{
    
    #define __NAME              "          NAME         "
    #define __IP                "        IP         "
    #define __LAST_REPLY        "     LAST REPLY       "
    #define __SUCCESS           " SUCCESS  "
    #define __TOTAL_PINGS       " TOTAL PINGS  "
    #define __AVERAGE_TIME      " AVERAGE TIME "

    DWORD result;    

    const char name_head[] = __NAME;
    const char ip_head[] = __IP;
    const char reply_time_head[] = __LAST_REPLY;
    const char sucess_head[] = __SUCCESS;
    const char total_pings_head[] = __TOTAL_PINGS;
    const char average_time_head[] = __AVERAGE_TIME;

    const int position_name = 1;
    const int position_ip_address = position_name + strlen(name_head) + 1;
    const int position_reply = position_ip_address + strlen(ip_head) + 1;
    const int position_success = position_reply + strlen(reply_time_head) + 1;
    const int position_total_pings = position_success + strlen(sucess_head) + 1;
    const int position_average_time = position_total_pings + strlen(total_pings_head) + 1;


    const char row_00[] = "-------------------------------------------------------------------------------------------------------------\n";
    const char row_01[] = "|" __NAME "|" __IP "|" __LAST_REPLY "|" __SUCCESS "|" __TOTAL_PINGS "|" __AVERAGE_TIME "|\n";
    const char row_02[] = "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n";
    
    const int row_stride = strlen(row_00);
    
    assert(strlen(row_00) == strlen(row_01));
    assert(strlen(row_01) == strlen(row_02));
    assert(strlen(row_02) == strlen(row_00));

    const int scratch_buffer_size = 128;
    char scratch_sprintf_buffer[scratch_buffer_size] = { 0 };

    const int total_data_rows = ping_results.size();
    const int total_rows = total_data_rows + 4;
    const int buffer_size = (row_stride * (total_rows)) + 1;
    char *buffer = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buffer_size);
    
    for (int i=0,  pos=0;i<total_rows;i++,pos+=row_stride)
    {
        CopyMemory(buffer + pos, row_02, row_stride);
    }

    CopyMemory(buffer,                                          row_00, row_stride);    
    CopyMemory(buffer + row_stride,                             row_01, row_stride);
    CopyMemory(buffer + row_stride + row_stride,                row_00, row_stride);
    CopyMemory(buffer + buffer_size -1 - row_stride,            row_00, row_stride);
    
    while (true)
    {
        char *buffer_pos = buffer;
        buffer_pos += row_stride;
        buffer_pos += row_stride;
        buffer_pos += row_stride;

        struct tm local_time_temp;

        for (Host_Result& current_host : ping_results)
        {
            ProcessHost(current_host);

            localtime_s(&local_time_temp, &current_host.last_success_time);            
            ZeroMemory(scratch_sprintf_buffer, scratch_buffer_size);

            snprintf(scratch_sprintf_buffer, scratch_buffer_size, "%*s", (int)strlen(name_head), current_host.host.name.c_str());
            CopyMemory(buffer_pos + position_name, scratch_sprintf_buffer, strlen(name_head));
            
            snprintf(scratch_sprintf_buffer, scratch_buffer_size, "%*s", (int)strlen(ip_head), current_host.host.ip_address.c_str());
            CopyMemory(buffer_pos + position_ip_address, scratch_sprintf_buffer, strlen(ip_head));

            size_t date_size = strftime(scratch_sprintf_buffer, scratch_buffer_size, "     %x %H:%M:%S", &local_time_temp);
            CopyMemory(buffer_pos + position_reply, scratch_sprintf_buffer, date_size);

            snprintf(scratch_sprintf_buffer, scratch_buffer_size, "%*d", (int)strlen(sucess_head), (int)current_host.success_rate);
            CopyMemory(buffer_pos + position_success, scratch_sprintf_buffer, strlen(sucess_head));

            snprintf(scratch_sprintf_buffer, scratch_buffer_size, "%*d", (int)strlen(total_pings_head), (int)current_host.number_sent);
            CopyMemory(buffer_pos + position_total_pings, scratch_sprintf_buffer, strlen(total_pings_head));

            snprintf(scratch_sprintf_buffer, scratch_buffer_size, "%*d", (int)strlen(average_time_head), (int)current_host.average_rtt);
            CopyMemory(buffer_pos + position_average_time, scratch_sprintf_buffer, strlen(average_time_head));

            buffer_pos += row_stride;
        }
        buffer[buffer_size - 1] = 0x0;
        
        EnterCriticalSection(&buffer_swap_critical_section);
        results_text_output = std::string(buffer);
        LeaveCriticalSection(&buffer_swap_critical_section);

        result = WaitForSingleObject(handle_event_exit_thread, 30000);
        if (result == WAIT_OBJECT_0)
        {
            break;
        }        
    }
    SetEvent(handle_event_thread_shut_complete);

    HeapFree(GetProcessHeap(), 0, (LPVOID)buffer);
    return TRUE;
}

void DrawUI(HWND handle_window, HDC handle_device_context, RECT& client_rectangle)
{
    std::string local_results_string;
    EnterCriticalSection(&buffer_swap_critical_section);
    local_results_string = results_text_output;
    LeaveCriticalSection(&buffer_swap_critical_section);

    if (local_results_string.size() < 20) return;

    SelectObject(handle_device_context, font_fixed_width);

    DWORD init_check_result = WaitForSingleObject(handle_event_init_once, 1);

    if ( init_check_result != WAIT_OBJECT_0)
    {
        DrawTextA(handle_device_context, &local_results_string[0], local_results_string.size(), &computed_rect, DT_CALCRECT | DT_LEFT);
        SetEvent(handle_event_init_once);        
        SendMessage(handle_window, WM_SIZE, NULL, NULL);
    }

    DrawTextA(handle_device_context, &local_results_string[0], local_results_string.size(), &client_rectangle, DT_LEFT);
}

void AddHosts()
{
    {
        Host_Result hr;
        hr.host.name = std::string("Gateway 1");
        hr.host.ip_address = "192.168.128.1";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }
    {
        Host_Result hr;
        hr.host.name = std::string("Gateway 2");
        hr.host.ip_address = "192.168.129.1";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }
    {
        Host_Result hr;
        hr.host.name = std::string("DEPT_Printer");
        hr.host.ip_address = "192.168.128.151";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }
    {
        Host_Result hr;
        hr.host.name = std::string("MEGA_Printer3");
        hr.host.ip_address = "192.168.129.128";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }
    {
        Host_Result hr;
        hr.host.name = std::string("CAN CAN");
        hr.host.ip_address = "192.168.129.43";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }


    {
        Host_Result hr;
        hr.host.name = std::string("HP");
        hr.host.ip_address = "192.168.129.197";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }
    {
        Host_Result hr;
        hr.host.name = std::string("Dell");
        hr.host.ip_address = "192.168.128.115";
        hr.handle_ip_address = inet_addr(hr.host.ip_address.c_str());
        ping_results.push_back(hr);
    }


}

void InitProgram()
{
    font_fixed_width = CreateFont(12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Courier"));
    AddHosts();
    InitializeCriticalSection(&buffer_swap_critical_section);
    icmp_handle = IcmpCreateFile();
    assert(icmp_handle != INVALID_HANDLE_VALUE);
    handle_thread = CreateThread(NULL, 0, DoAllPingsAndSignal, NULL, 0, NULL);    
    handle_event_exit_thread = CreateEvent(NULL, TRUE, FALSE, _T("EXIT_EVENT"));
    handle_event_thread_shut_complete = CreateEvent(NULL, TRUE, FALSE, _T("THREAD_CLOSE_EVENT"));
    handle_event_init_once = CreateEvent(NULL, TRUE, FALSE, _T("INIT_ONCE_EVENT"));
}

void ExitProgram()
{
    DeleteObject(font_fixed_width);
    SetEvent(handle_event_exit_thread);
    WaitForSingleObject(handle_event_thread_shut_complete, INFINITE);

    DeleteCriticalSection(&buffer_swap_critical_section);
    IcmpCloseHandle(icmp_handle);
    CloseHandle(handle_event_exit_thread);
    CloseHandle(handle_event_thread_shut_complete);
    CloseHandle(handle_event_init_once);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {    
    case WM_SIZE:
        RECT window_rect;
        GetWindowRect(hWnd, &window_rect);
        if ((window_rect.right - window_rect.left) < (computed_rect.right - computed_rect.left + 20) ||
            (window_rect.bottom - window_rect.top) < (computed_rect.bottom - computed_rect.top + 30))
        {
            SetWindowPos(hWnd, NULL,
                window_rect.left,
                window_rect.top,
                computed_rect.right - computed_rect.left + 20,
                computed_rect.bottom - computed_rect.top + 30,
                0);
        }
        break;
    case WM_TIMER:
        switch (wParam)
        {
        case IVT_TIMER1:
            InvalidateRect(hWnd, NULL, TRUE);
                return 0;
            default:
                break;
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC handle_device_context;  
            RECT client_rectangle;

            handle_device_context = BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &client_rectangle);
            int width = client_rectangle.right - client_rectangle.left;
            int height = client_rectangle.bottom - client_rectangle.top;

            if (abs(width - window_width) > 5 || abs(height - window_height) > 5)
            {
                if (handle_brush) DeleteObject(handle_brush);
                if (handle_bitmap) DeleteObject(handle_bitmap);
                if (handle_virtual_device_context) DeleteDC(handle_virtual_device_context);

                handle_virtual_device_context = CreateCompatibleDC(handle_device_context);
                handle_bitmap = CreateCompatibleBitmap(handle_device_context, width, height);
                SelectObject(handle_virtual_device_context, handle_bitmap);
                handle_brush = CreateSolidBrush(RGB(255, 255, 255));
                window_height = height;
                window_width = width;
            }

            if (handle_virtual_device_context == NULL || handle_bitmap == NULL || handle_brush == NULL) break;
            
            int state_hdc = SaveDC(handle_virtual_device_context);
            FillRect(handle_virtual_device_context, &client_rectangle, handle_brush);

            DrawUI(hWnd, handle_virtual_device_context, client_rectangle);
            BitBlt(handle_device_context, 0, 0, width, height, handle_virtual_device_context, 0, 0, SRCCOPY);
            RestoreDC(handle_device_context, state_hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        KillTimer(hWnd, IVT_TIMER1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

int CALLBACK WinMain(
    _In_ HINSTANCE handle_instance,
    _In_opt_ HINSTANCE handle_previous_instance,
    _In_ LPSTR     command_line_string,
    _In_ int       window_startup_state
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = handle_instance;    
    wcex.hIcon = LoadIcon(handle_instance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);    
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = class_name;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hbrBackground = NULL;

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("RegisterClassEx failed!"), _T("Blash"), NULL);
        return 1;
    }

    handle_app_instance = handle_instance; 

    HWND hWnd = CreateWindow(class_name, window_title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, handle_instance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, _T("CreateWindow failed!"), _T("Blah"), NULL);
        return 1;
    }
    
    if (!SetTimer(hWnd, IVT_TIMER1, 1000, (TIMERPROC)NULL))
    {
        MessageBox(NULL, _T("SetTimer failed!"), _T("Blah"), NULL);
        return 1;
    }

    InitProgram();

    ShowWindow(hWnd, window_startup_state);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ExitProgram();

    return (int)msg.wParam;
}