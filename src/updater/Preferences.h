#ifndef PREFERENCES_H
#define PREFERENCES_H

//TODO: update Keys

const char CLIENT_KEY[] = "FhMgXbqb";
const char USER_AGENT[] = "MEGA/MEGAUpdaterTask";

#ifdef _WIN32
const char UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/upd/wcmd/v.txt";
const char EMERGENCY_UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/eupd/wcmd/v.txt";
#else
const char UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/upd/mcmd/v.txt";
const char EMERGENCY_UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/eupd/mcmd/v.txt";

const char APP_DIR_BUNDLE[] = "/Applications/MEGAcmd.app/";
#endif

const char UPDATE_PUBLIC_KEY[] = "EACTzXPE8fdMhm6LizLe1FxV2DncybVh2cXpW3momTb8tpzRNT833r1RfySz5uHe8gdoXN1W0eM5Bk8X-LefygYYDS9RyXrRZ8qXrr9ITJ4r8ATnFIEThO5vqaCpGWTVi5pOPI5FUTJuhghVKTyAels2SpYT5CmfSQIkMKv7YVldaV7A-kY060GfrNg4--ETyIzhvaSZ_jyw-gmzYl_dwfT9kSzrrWy1vQG8JPNjKVPC4MCTZJx9SNvp1fVi77hhgT-Mc5PLcDIfjustlJkDBHtmGEjyaDnaWQf49rGq94q23mLc56MSjKpjOR1TtpsCY31d1Oy2fEXFgghM0R-1UkKswVuWhEEd8nO2PimJOl4u9ZJ2PWtJL1Ro0Hlw9OemJ12klIAxtGV-61Z60XoErbqThwWT5Uu3D2gjK9e6rL9dufSoqjC7UA2C0h7KNtfUcUHw0UWzahlR8XBNFXaLWx9Z8fRtA_a4seZcr0AhIA7JdQG5i8tOZo966KcFnkU77pfQTSprnJhCfEmYbWm9EZA122LJBWq2UrSQQN3pKc9goNaaNxy5PYU1yXyiAfMVsBDmDonhRWQh2XhdV-FWJ3rOGMe25zOwV4z1XkNBuW4T1JF2FgqGR6_q74B2ccFC8vrNGvlTEcs3MSxTI_EKLXQvBYy7hxG8EPUkrMVCaWzzTQAFEQ";
const char UPDATE_FILENAME[] = "v.txt";
const char UPDATE_FOLDER_NAME[] = "eupdate";
const char BACKUP_FOLDER_NAME[] = "ebackup";
const char VERSION_FILE_NAME[] = "megacmd.version"; //TODO: ensure this files is properly created

#endif // PREFERENCES_H
