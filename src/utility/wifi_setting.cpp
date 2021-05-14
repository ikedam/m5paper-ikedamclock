#include "wifi_setting.hpp"
#include <SPIFFS.h>
#include <hwcrypto/aes.h>

WifiSetting::WifiSetting(const char* path, const unsigned char* key, size_t keyLength)
    : m_path(path)
    , m_keyLength(keyLength)
{
    m_key = new unsigned char[m_keyLength];
    memcpy(m_key, key, m_keyLength);
}

WifiSetting::~WifiSetting() {
    m_password.clear();
    delete[] m_key;
}

bool WifiSetting::readAndEncrypt() {
    if (!SPIFFS.exists(m_path)) {
        log_e(
            "No %s is available."
            " You must upload %s with the ESSID and password separated with LF to SPIFFS"
            " to use NTP.",
            m_path.c_str(),
            m_path.c_str()
        );
        return false;
    }

    File file = SPIFFS.open(m_path.c_str(), "rb");
    if (!file.find('\n')) {
        log_e(
            "Bad file format. No LF found."
            " %s should contain the ESSID and password separated with LF.",
            m_path.c_str()
        );
        file.close();
        return false;
    }

    size_t essidLength = file.position() - 1;   // position() points '\n'
    if (essidLength == 0) {
        log_e(
            "Bad file format. No ESSID specified."
        );
        file.close();
        return false;
    }
    if (file.size() <= essidLength + 1) {
        log_e(
            "Bad file format. No passowrd specified."
        );
        file.close();
        return false;
    }

    {
        char* essidBuf = new char[essidLength + 1];
        file.seek(0);
        size_t read = file.readBytes(essidBuf, essidLength);
        if (read != essidLength) {
            log_e(
                "Failed to read essid from %s. (expected %s bytes, but actual %s bytes)",
                m_path.c_str(),
                essidLength,
                read
            );
            delete[] essidBuf;
            file.close();
            return false;
        }
        essidBuf[essidLength] = 0;
        m_essid = essidBuf;
        delete[] essidBuf;
    }

    file.seek(essidLength + 1); // skip LF.
    if (file.read() == 0) {
        // encrypted
        size_t encryptedBufSize = file.size() - essidLength - 2;
        if (encryptedBufSize == 0 || encryptedBufSize % 16 != 0) {
            log_e(
                "Bad file format. Strange encoded password size: %d",
                encryptedBufSize
            );
            file.close();
            return false;
        }
        unsigned char* encryptedBuf = new unsigned char[encryptedBufSize];
        size_t read = file.readBytes(reinterpret_cast<char*>(encryptedBuf), encryptedBufSize);
        if (read != encryptedBufSize) {
            log_e(
                "Failed to read password from %s. (expected %d bytes, but actual %d bytes)",
                m_path.c_str(),
                encryptedBufSize,
                read
            );
            delete[] encryptedBuf;
            file.close();
            return false;
        }
        file.close();

        if (!decrypt(encryptedBuf, encryptedBufSize)) {
            delete[] encryptedBuf;
            return false;
        }
        delete[] encryptedBuf;
    } else {
        // not encrypted
        size_t passwordLength = file.size() - essidLength - 1;
        if (passwordLength == 0) {
            log_e(
                "Bad file format. No password exists"
            );
            file.close();
            return false;
        }

        // encryption buffer should be multiple of 16.
        size_t passwordBufSize = (passwordLength % 16 == 0)
            ? passwordLength
            : passwordLength + 16 - (passwordLength % 16);

        char* passwordBuf = new char[passwordBufSize + 1];
        memset(passwordBuf, 0, passwordBufSize + 1);
        file.seek(essidLength + 1); // skip LF.

        size_t read = file.readBytes(passwordBuf, passwordLength);
        if (read != passwordLength) {
            log_e(
                "Failed to read password from %s. (expected %d bytes, but actual %d bytes)",
                m_path,
                read,
                passwordLength
            );
            delete[] passwordBuf;
            file.close();
            return false;
        }
        file.close();
        m_password = passwordBuf;

        if (!encryptAndSave(reinterpret_cast<unsigned char*>(passwordBuf), passwordBufSize)) {
            delete[] passwordBuf;
            return false;
        }
    }
    return true;
}

bool WifiSetting::decrypt(unsigned char* encryptedBuf, size_t encryptedBufSize) {
    esp_aes_context ctx;
    esp_aes_init(&ctx);
    if (esp_aes_setkey(&ctx, m_key, m_keyLength) != 0) {
        log_e("Error in esp_aes_setkey");
        esp_aes_free(&ctx);
        return false;
    }

    char* decrypted = new char[encryptedBufSize + 1];
    memset(decrypted, 0, encryptedBufSize + 1);

    unsigned char iv[16];
    memset(iv, 0, sizeof(iv));

    if (esp_aes_crypt_cbc(
        &ctx,
        ESP_AES_DECRYPT,
        encryptedBufSize,
        iv,
        encryptedBuf,
        reinterpret_cast<unsigned char*>(decrypted)
    ) != 0) {
        log_e("Error in esp_aes_crypt_cbc");
        delete[] decrypted;
        esp_aes_free(&ctx);
        return false;
    }

    esp_aes_free(&ctx);

    m_password = decrypted;
    delete[] decrypted;
    return true;
}

bool WifiSetting::encryptAndSave(unsigned char* decryptedBuf, size_t decryptedBufSize) {
    log_e(
        "%s is not encrypted. Now it will be encrypted.",
        m_path.c_str()
    );

    esp_aes_context ctx;
    esp_aes_init(&ctx);
    if (esp_aes_setkey(&ctx, m_key, m_keyLength) != 0) {
        log_e("Error in esp_aes_setkey");
        esp_aes_free(&ctx);
        return false;
    }

    unsigned char* encrypted = new unsigned char[decryptedBufSize];
    unsigned char iv[16];
    memset(iv, 0, sizeof(iv));
    if (esp_aes_crypt_cbc(
        &ctx,
        ESP_AES_ENCRYPT,
        decryptedBufSize,
        iv,
        decryptedBuf,
        encrypted
    ) != 0) {
        log_e("Error in esp_aes_crypt_cbc");
        delete[] encrypted;
        esp_aes_free(&ctx);
        return false;
    }
    esp_aes_free(&ctx);

    if (!saveEncrypted(encrypted, decryptedBufSize)) {
        delete[] encrypted;
        return false;
    }
    delete[] encrypted;
    return true;
}

bool WifiSetting::saveEncrypted(unsigned char* encryptedBuf, size_t encryptedBufSize) {
    File file = SPIFFS.open(m_path.c_str(), "wb");
    {
        size_t written = file.write(reinterpret_cast<const uint8_t*>(m_essid.c_str()), m_essid.length());
        if (written != m_essid.length()) {
            log_e(
                "Failed to write essid to %s. (expected %d bytes, but actual %d bytes)",
                m_path.c_str(),
                m_essid.length(),
                written
            );
            file.close();
            return false;
        }
    }
    if (file.write('\n') != 1) {
        log_e(
            "Failed to write LF to %s.",
            m_path.c_str()
        );
        file.close();
        return false;
    }
    if (file.write(0) != 1) {
        log_e(
            "Failed to write NULL to %s.",
            m_path.c_str()
        );
        file.close();
        return false;
    }
    {
        size_t written = file.write(encryptedBuf, encryptedBufSize);
        if (written != encryptedBufSize) {
            log_e(
                "Failed to write password to %s. (expected %d bytes, but actual %d bytes)",
                m_path.c_str(),
                encryptedBufSize,
                written
            );
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}
