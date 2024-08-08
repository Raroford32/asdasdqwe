#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <vector>
#include <string>

class MainWindow
{
public:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();

    void show(int nCmdShow);

private:
    HWND hWnd;
    HWND hSelectFileButton;
    HWND hFilePathEdit;
    HWND hStartButton;
    HWND hStatusLabel;
    std::string selectedFilePath;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void onSelectFileButtonClicked();
    void onStartButtonClicked();
    
    std::vector<unsigned char> xor_encrypt(const std::vector<unsigned char>& data, const std::string& key);
    std::vector<unsigned char> xor_decrypt(const std::vector<unsigned char>& data, const std::string& key);
    std::string generate_random_key(size_t length);
};

#endif // MAINWINDOW_H
