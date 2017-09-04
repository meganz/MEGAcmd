#ifndef MEGACMDPLATFORM_H
#define MEGACMDPLATFORM_H


#ifdef __MACH__

char *runWithRootPrivileges(char *command);
#endif

#endif // MEGACMDPLATFORM_H
