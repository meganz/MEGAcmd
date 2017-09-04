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

#endif
