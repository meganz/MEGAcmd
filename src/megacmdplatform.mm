#include "megacmdplatform.h"
#include <Cocoa/Cocoa.h>

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
            @"ProgramArguments": @[@"/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdUpdater"],
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

    QString path = QString::fromUtf8([fullpath UTF8String]);
    QFile(path).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);

    QStringList scriptArgs;
    scriptArgs << QString::fromUtf8("-c")
               << QString::fromUtf8("launchctl unload %1 && launchctl load %1").arg(path);

    QProcess p;
    p.start(QString::fromAscii("bash"), scriptArgs);
    if (!p.waitForFinished(2000))
    {
        return false;
    }

    return p.exitCode();
}


#endif
