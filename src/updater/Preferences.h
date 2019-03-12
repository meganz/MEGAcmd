#ifndef PREFERENCES_H
#define PREFERENCES_H


const char CLIENT_KEY[] = "BdARkQSQ";
const char USER_AGENT[] = "MEGA/MEGAcmdUpdaterTask";

#ifdef _WIN32
const char UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/upd/wcmd/v.txt";
const char EMERGENCY_UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/eupd/wcmd/v.txt";
#else
const char UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/upd/mcmd/v.txt";
const char EMERGENCY_UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/eupd/mcmd/v.txt";

const char APP_DIR_BUNDLE[] = "/Applications/MEGAcmd.app/";
#endif

const char UPDATE_PUBLIC_KEY[] = "EACuusrffoqCAsjS824EsQ04pvuDKHUd8PCAfPLsowLBMGWmcxmX-tgk05EXkdyHZPixEda96XadqudlQOEYLBlxMgcAwDkewKoW1XtBQVm3wXeHIvEeBimpWWU6Z6qYP5BiCbW-wCTDS67f4LdkrbA4hcJiVTxQxRUxoJU-I-DCh0ZLCxdrXHpWBrXGv9hFfxMFhr01YGxcW1tgqRx5r6q3YPpFaGo-q9ib3YIBE2HMAc_n_ayz4g5HMAq-C61WT_vhHzPnoCUwpRH68TCoop-3VlYTo8Ps-Xzno1OazCH6VM5MiCLS0X5jhh54RveVZn44MRMLDdj3jzUed4-hFgOXNYLLX1kDRlXiDwGtht-wPJF7BLNVkRolfOUrsONmw3Tbz_ywB-TK7pjhlYoMo7enwABT1iBQoub80OEi-1ltMnzFRWu6o7KCsCqnDtJpALG04B6E1aWvO2HK0-UQNIAPfbErtG4pVVirIaSq4vX5BlmYJ_XCAsaFuLufJvBQBIEwCJRdQ96o4plgzHuJwlaEeYd2dMEjafOi3DBPV7OjN9H7yI0NyHDrqV_hDAmMqHHPu3hdhqkdrSFmcMVGsxXkaOnXDNmJfdwr_mostPY-Uwx_g2vRHKgLpkntlIlvh0GALEzLpukdmnKgXrUNRXrIyyjoRXGNMlBTdV8vAMlGVQAFEQ";
const char UPDATE_FILENAME[] = "v.txt";
const char UPDATE_FOLDER_NAME[] = "eupdate";
const char BACKUP_FOLDER_NAME[] = "ebackup";
const char VERSION_FILE_NAME[] = "megacmd.version";
#endif // PREFERENCES_H
