#!/bin/sh
CONF=/etc/config/qpkg.conf
QPKG_NAME="MEGAcmd"
QPKG_ROOT=`/sbin/getcfg $QPKG_NAME Install_Path -f ${CONF}`
export QNAP_QPKG=$QPKG_NAME

case "$1" in
  start)
    ENABLED=$(/sbin/getcfg $QPKG_NAME Enable -u -d FALSE -f $CONF)
    if [ "$ENABLED" != "TRUE" ]; then
        echo "$QPKG_NAME is disabled."
        exit 1
    fi
    echo "MEGA-cmd is a console tool, and can be invoked via an ssh connection.  Executables are at %QPKG_ROOT"
    ;;

  stop)
    echo "MEGA-cmd is a console tool"
    ;;

  restart)
    $0 stop
    $0 start
    ;;

  *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
esac

exit 0
