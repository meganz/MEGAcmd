#ifndef MEGACMDPLATFORM_H
#define MEGACMDPLATFORM_H

namespace megacmd {

#ifdef __MACH__

char *runWithRootPrivileges(char *command);
bool registerUpdateDaemon();

#endif

}//end namespace
#endif // MEGACMDPLATFORM_H
