#include "megacmdplatform.h"
#include <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <iostream>

#include <sstream>
#include <string>
#include <sys/stat.h>
#ifdef __MACH__

char *runWithRootPrivileges(char *command)
{
    OSStatus status;
    AuthorizationRef authorizationRef;

    const char *prompt = "MEGAcmd. ";

    char *result = NULL;

    char* args[3];
    args [0] = "-e";
    args [1] = command;
    args [2] = NULL;

    FILE *pipe = NULL;

    AuthorizationItem kAuthEnv[] = {/* { kAuthorizationEnvironmentIcon, strlen(icon), (void*)icon, 0 },*/
        {kAuthorizationEnvironmentPrompt, strlen(prompt), (char *) prompt, 0}};
    AuthorizationEnvironment myAuthorizationEnvironment = { 1, kAuthEnv };

    AuthorizationItem right = {kAuthorizationRightExecute, 0, NULL, 0};
    AuthorizationRights rights = {1, &right};
    AuthorizationFlags flags = kAuthorizationFlagDefaults |
    kAuthorizationFlagInteractionAllowed |
    kAuthorizationFlagPreAuthorize |
    kAuthorizationFlagExtendRights;

    status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment,
                                     kAuthorizationFlagDefaults, &authorizationRef);
    if (status != errAuthorizationSuccess)
    {
        return NULL;
    }

    // Call AuthorizationCopyRights to determine rights.
    status = AuthorizationCopyRights(authorizationRef, &rights, &myAuthorizationEnvironment, flags, NULL);
    if (status != errAuthorizationSuccess)
    {
        return NULL;
    }

    status = AuthorizationExecuteWithPrivileges(authorizationRef, "/usr/bin/osascript",
                                                kAuthorizationFlagDefaults, args, &pipe);
    AuthorizationFree(authorizationRef, kAuthorizationFlagDestroyRights);
    if (status == errAuthorizationSuccess)
    {
        result = new char[1024];
        fread(result, 1024, 1, pipe);
        fclose(pipe);
    }

    return result;
}


bool registerUpdateDaemon()
{
    NSDictionary *plistd = @{
            @"Label": @"megacmd.mac.megaupdater",
            @"ProgramArguments": @[@"/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdUpdater", @"--emergency-update"],
            @"StartInterval": @7200,
            @"RunAtLoad": @true,
            @"StandardErrorPath": @"/dev/null",
            @"StandardOutPath": @"/dev/null",
     };

    const char* home = getenv("HOME");
    if (!home)
    {
        return false;
    }

    NSString *homepath = [NSString stringWithUTF8String:home];
    if (!homepath)
    {
        return false;
    }

    NSString *fullpath = [homepath stringByAppendingString:@"/Library/LaunchAgents/megacmd.mac.megaupdater.plist"];
    if ([plistd writeToFile:fullpath atomically:YES] == NO)
    {
        return false;
    }
    std::string path = [fullpath UTF8String];
    chmod(path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int pid = fork();
    if (pid)
    {
        int status = 0;
        waitpid(pid, &status, 0);

        if ( WIFEXITED(status) )
        {
            int exit_code = WEXITSTATUS(status);
            return (!exit_code);
        }
        else
        {
            std::cerr << " failed return. errno=" << errno << std::endl;
            return false;
        }
    }
    else
    {
        char **argv = new char*[4];
        int i = 0;
        argv[i++]="/bin/bash";
        argv[i++]="-c";
        std::string ls="launchctl unload ";
        ls.append(path);
        ls.append(" && launchctl load ");
        ls.append(path);
        argv[i++]=(char *)ls.c_str();
        argv[i++]=NULL;
        execv(argv[0],argv);
    }

    return false;
}


#endif
