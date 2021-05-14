#include <WString.h>

class WifiSetting {
public:
    WifiSetting(const char* path, const unsigned char* key, size_t keyLength);
    virtual ~WifiSetting();

    bool readAndEncrypt();
    const char* getEssid() const {
        return m_essid.c_str();
    }
    const char* getPassword() const {
        return m_password.c_str();
    }
private:
    bool decrypt(unsigned char* encryptedBuf, size_t encryptedBufSize);
    bool encryptAndSave(unsigned char* decryptedBuf, size_t decryptedBufSize);
    bool saveEncrypted(unsigned char* encryptedBuf, size_t encryptedBufSize);

    String m_path;
    String m_essid;
    String m_password;

    unsigned char* m_key;
    size_t m_keyLength;
};
