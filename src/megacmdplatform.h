#ifndef MEGACMDPLATFORM_H
#define MEGACMDPLATFORM_H


#ifdef __MACH__

char *runWithRootPrivileges(char *command);
bool registerUpdateDaemon();

#endif

#endif // MEGACMDPLATFORM_H
