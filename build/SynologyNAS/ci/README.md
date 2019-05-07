This directory contains scripts to build Synology packages on a continuous 
integration server such as Jenkins.

`trigger.sh` and `wait.sh` are run by a regular user where `trigger.sh`
simply touches a file named `start` to indicate that the build should start.

`wait.sh` can then be run to check whether the build has finished by virtue
of the modified time stamp of a file named `result`.

The actual build is performed by running `build.sh` as a root user. This
build script is run through a daemon user when the `start` file changes its
modified time stamp. Once finished, the build script will update the modified
time stamp of `result` and populate the file with the status of the build 
artifacts.

The workflow is as follows:
# Start daemon process that watches for `start`
# `trigger.sh` is run that touches `start`
# The daemon process commences the build
# `wait.sh` is run to wait for the build to finish
# The build finishes
# `wait.sh` is notified and exits
