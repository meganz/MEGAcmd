#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <sstream>
#include <time.h>
#include "UpdateTask.h"
#include "Preferences.h"

using namespace std;

#ifdef _WIN32
int CALLBACK WinMain(_In_ HINSTANCE, _In_ HINSTANCE, _In_ LPSTR, _In_ int)
#else
int main(int argc, char *argv[])
#endif
{
    time_t currentTime = time(NULL);
    cout << "Process started at " << ctime(&currentTime);
    srand(currentTime);

    UpdateTask updater;
    updater.checkForUpdates();

    currentTime = time(NULL);
    cout << "Process finished at " << ctime(&currentTime);
    return 0;
}
