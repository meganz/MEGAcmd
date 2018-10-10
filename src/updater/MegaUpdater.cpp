#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <sstream>
#include <time.h>
#include "UpdateTask.h"
#include "Preferences.h"

using namespace std;

int main(int argc, char **argv)
{
    time_t currentTime = time(NULL);
    cout << "Process started at " << ctime(&currentTime) << endl;
    srand(currentTime);

    UpdateTask updater;
    bool updated = updater.checkForUpdates();

    currentTime = time(NULL);
    cout << "Process finished at " << ctime(&currentTime) << endl;
    return updated;
}
