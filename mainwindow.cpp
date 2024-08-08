#include "mainwindow.h"
#include <commdlg.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <cstring>

MainWindow::MainWindow(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MainWindow::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = TEXT("MainWindowClass");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    hWnd = CreateWindowEx(0, TEXT("MainWindowClass"), TEXT("File Encryptor"),
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 500, 200,
                          nullptr, nullptr, hInstance, this);

    hSelectFileButton = CreateWindow(TEXT("button"), TEXT("Select File"),
                                     WS_CHILD | WS_VISIBLE, 10, 10, 100, 30,
                                     hWnd, (HMENU)1, hInstance, nullptr);

    hFilePathEdit = CreateWindow(TEXT("edit"), TEXT(""),
                                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                                 120, 10, 300, 30,
                                 hWnd, nullptr, hInstance, nullptr);

    hStartButton = CreateWindow(TEXT("button"), TEXT("Start"),
                                WS_CHILD | WS_VISIBLE, 10, 50, 100, 30,
                                hWnd, (HMENU)2, hInstance, nullptr);

    hStatusLabel = CreateWindow(TEXT("static"), TEXT(""),
                                WS_CHILD | WS_VISIBLE,
                                10, 90, 400, 30,
                                hWnd, nullptr, hInstance, nullptr);
}

MainWindow::~MainWindow()
{
    DestroyWindow(hWnd);
}

void MainWindow::show(int nCmdShow)
{
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MainWindow *mainWindow;
    if (message == WM_CREATE)
    {
        mainWindow = (MainWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)mainWindow);
    }
    else
    {
        mainWindow = (MainWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (mainWindow)
    {
        switch (message)
        {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case 1:
                mainWindow->onSelectFileButtonClicked();
                break;
            case 2:
                mainWindow->onStartButtonClicked();
                break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void MainWindow::onSelectFileButtonClicked()
{
    OPENFILENAME ofn;
    char szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE)
    {
        SetWindowText(hFilePathEdit, ofn.lpstrFile);
        selectedFilePath = ofn.lpstrFile;
    }
}

void MainWindow::onStartButtonClicked()
{
    if (selectedFilePath.empty())
    {
        SetWindowText(hStatusLabel, TEXT("Please select a file first."));
        return;
    }

    std::ifstream input(selectedFilePath, std::ios::binary);
    if (!input)
    {
        SetWindowText(hStatusLabel, TEXT("Failed to open file."));
        return;
    }
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    std::string key = generate_random_key(1024);
    std::vector<unsigned char> encrypted_data = xor_encrypt(data, key);

    std::ostringstream hex_data;
    hex_data << std::hex << std::setfill('0');
    for (const auto &byte : encrypted_data)
    {
        hex_data << std::setw(2) << static_cast<int>(byte);
    }

    std::string stub_code = R"cpp(
#include <iostream>
#include <vector>
#include <windows.h>
#include <cstring>
#include <fstream>
#include <sstream>

std::vector<unsigned char> hex_to_bytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<unsigned char> xor_decrypt(const std::vector<unsigned char>& data, const std::string& key) {
    std::vector<unsigned char> decrypted_data(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        decrypted_data[i] = data[i] ^ key[i % key.size()];
    }
    return decrypted_data;
}

const std::string encrypted_data_hex = ")cpp" +
                         hex_data.str() + R"cpp(";

int main() {
    std::string key = ")cpp" +
                         key + R"cpp(";
    std::vector<unsigned char> encrypted_data = hex_to_bytes(encrypted_data_hex);
    std::vector<unsigned char> decrypted_data = xor_decrypt(encrypted_data, key);

    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    std::string temp_exe = std::string(temp_path) + "temp_executable.exe";
    std::ofstream temp_file(temp_exe, std::ios::binary);
    temp_file.write(reinterpret_cast<const char*>(decrypted_data.data()), decrypted_data.size());
    temp_file.close();

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(temp_exe.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        std::cerr << "Failed to create process for the decrypted executable" << std::endl;
        return -1;
    }

    exit(0);

    return 0;
}
)cpp";

    std::ofstream stub_file("stub.cpp");
    stub_file << stub_code;
    stub_file.close();

    system("g++ -m64 -std=c++11 stub.cpp -o encrypted_executable.exe -static-libgcc -static-libstdc++ -lpthread -static");

    SetWindowText(hStatusLabel, TEXT("Executable generated successfully."));
}

std::vector<unsigned char> MainWindow::xor_encrypt(const std::vector<unsigned char>& data, const std::string& key) {
    std::vector<unsigned char> encrypted_data(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        encrypted_data[i] = data[i] ^ key[i % key.size()];
    }
    return encrypted_data;
}

std::string MainWindow::generate_random_key(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    std::string key;
    for (size_t i = 0; i < length; ++i) {
        key += chars[dis(gen)];
    }
    return key;
}
