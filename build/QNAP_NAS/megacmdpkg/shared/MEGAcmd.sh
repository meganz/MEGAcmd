#!/bin/sh

service_start()
{
        web_config_add
        web_config_reload
}

service_stop()
{
        web_config_remove
        web_config_reload
}

web_config_add()
{
        local getcfg="/sbin/getcfg"
        local name="MEGAcmd"
        local conf="/etc/config/qpkg.conf"
        local root="$($getcfg $name Install_Path -f $conf)"

        ln -sf $root /home/httpd/cgi-bin/qpkg
        ln -sf $root/web/apache-megacmd.conf /etc/default_config/apache/extra
}

web_config_reload()
{
        /etc/init.d/thttpd.sh reload
        /etc/init.d/stunnel.sh reload
}

web_config_remove()
{
        rm -f /etc/default_config/apache/extra/apache-megacmd.conf
        rm -f /home/httpd/cgi-bin/qpkg/MEGAcmd
}

case "$1" in
start)
        service_start
        ;;
stop)
        service_stop
        ;;
restart)
        service_stop
        service_start
        ;;
*)
        echo "Usage: $@ {start|stop|restart}"
        exit 1
        ;;
esac

exit 0

