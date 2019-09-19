#include "crypto.hpp"
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

// returns the number of bytes written to data, or total dataLen needed if data is NULL
size_t Base58Decode(uint8_t *data, size_t dataLen, const char *str)
{
    size_t i = 0, j, len, zcount = 0;

    assert(str != NULL);
    while (str && *str == '1') str++, zcount++; // count leading zeroes

    uint8_t buf[(str) ? strlen(str)*733/1000 + 1 : 0]; // log(58)/log(256), rounded up

    memset(buf, 0, sizeof(buf));

    while (str && *str) {
        uint32_t carry = *(const uint8_t *)(str++);

        switch (carry) {
            case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                carry -= '1';
                break;

            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
                carry += 9 - 'A';
                break;

            case 'J': case 'K': case 'L': case 'M': case 'N':
                carry += 17 - 'J';
                break;

            case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y':
            case 'Z':
                carry += 22 - 'P';
                break;

            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k':
                carry += 33 - 'a';
                break;

            case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v':
            case 'w': case 'x': case 'y': case 'z':
                carry += 44 - 'm';
                break;

            default:
                carry = UINT32_MAX;
        }

        if (carry >= 58) break; // invalid base58 digit

        for (j = sizeof(buf); j > 0; j--) {
            carry += (uint32_t)buf[j - 1]*58;
            buf[j - 1] = carry & 0xff;
            carry >>= 8;
        }

        var_clean(&carry);
    }

    while (i < sizeof(buf) && buf[i] == 0) i++; // skip leading zeroes
    len = zcount + sizeof(buf) - i;

    if (data && len <= dataLen) {
        if (zcount > 0) memset(data, 0, zcount);
        memcpy(&data[zcount], &buf[i], sizeof(buf) - i);
    }

    mem_clean(buf, sizeof(buf));
    return (! data || len <= dataLen) ? len : 0;
}

// returns the number of bytes written to data, or total dataLen needed if data is NULL
size_t Base58CheckDecode(uint8_t *data, size_t dataLen, const char *str)
{
    size_t len, bufLen = (str) ? strlen(str) : 0;
    uint8_t md[256/8], _buf[(bufLen <= 0x1000) ? bufLen : 0], *buf = (bufLen <= 0x1000) ? _buf : (uint8_t *)malloc
            (bufLen);

    assert(str != NULL);
    assert(buf != NULL);
    len = Base58Decode(buf, bufLen, str);

    if (len >= 4) {
        len -= 4;
        SHA256_2(md, buf, len);
        if (memcmp(&buf[len], md, sizeof(uint32_t)) != 0) len = 0; // verify checksum
        if (data && len <= dataLen) memcpy(data, buf, len);
    }
    else len = 0;

    mem_clean(buf, bufLen);
    if (buf != _buf) free(buf);
    return (! data || len <= dataLen) ? len : 0;
}