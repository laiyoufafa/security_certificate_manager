/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "securec.h"

#include "cm_log.h"
#include "cert_manager_status.h"
#include "cert_manager_uri.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IS_TYPE_VALID(t) ((t) <= CM_URI_TYPE_MAX)

#define SCHEME "oh:"
#define P_OBJECT "o="
#define P_TYPE "t="
#define P_USER "u="
#define P_APP "a="
#define Q_MAC "m="
#define Q_CLIENT_USER "cu="
#define Q_CLIENT_APP "ca="

// characters do not need to be encoded in path, other than digits and algabets
#define P_RES_AVAIL "-._~:[]@!$'()*+,=&"
// characters do not need to be encoded in query, other than digits and algabets
#define Q_RES_AVAIL "-._~:[]@!$'()*+,=/?|"

int32_t CertManagerFreeUri(struct CMUri *uri)
{
    if (uri == NULL) {
        return CMR_OK;
    }
    FREE_PTR(uri->object);
    FREE_PTR(uri->user);
    FREE_PTR(uri->app);
    FREE_PTR(uri->mac);
    FREE_PTR(uri->clientUser);
    FREE_PTR(uri->clientApp);
    return CMR_OK;
}

inline bool CertManagerIsKeyObjectType(uint32_t type)
{
    return (type == CM_URI_TYPE_APP_KEY || type == CM_URI_TYPE_WLAN_KEY);
}

static int IsUnreserved(const char *resAvail, size_t resAvailLen, char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return 1;
    }
    if (resAvail != NULL) {
        for (size_t i = 0; i < resAvailLen; i++) {
            if (c == resAvail[i]) {
                return 1;
            }
        }
    }
    return 0;
}

static uint32_t GetComponentEncodedLen(
    const char *key, const char *value,
    const char *resAvail, uint32_t *sep)
{
    if (value == NULL) {
        return 0;
    }
    size_t resAvailLen = strlen(resAvail);
    size_t keyLen = strlen(key);
    size_t valueLen = strlen(value);
    size_t reserved = 0;
    for (size_t i = 0; i < valueLen; i++) {
        if (!IsUnreserved(resAvail, resAvailLen, value[i])) {
            reserved++;
        }
    }
    // each reserved character requires 2 extra bytes to percent-encode
    uint32_t len = (uint32_t) (keyLen + valueLen + reserved * 2 + *sep);
    *sep = 1;
    return len;
}

static uint32_t GetEncodedLen(const struct CMUri *uri)
{
    if (uri == NULL) {
        return 0;
    }

    uint32_t sep = 0;
    uint32_t len = strlen(SCHEME);

    len += GetComponentEncodedLen(P_TYPE, g_types[uri->type], P_RES_AVAIL, &sep);
    len += GetComponentEncodedLen(P_OBJECT, uri->object, P_RES_AVAIL, &sep);
    len += GetComponentEncodedLen(P_USER, uri->user, P_RES_AVAIL, &sep);
    len += GetComponentEncodedLen(P_APP, uri->app, P_RES_AVAIL, &sep);

    uint32_t qlen = 0;
    sep = 0;
    qlen += GetComponentEncodedLen(Q_CLIENT_USER, uri->clientUser, Q_RES_AVAIL, &sep);
    qlen += GetComponentEncodedLen(Q_CLIENT_APP, uri->clientApp, Q_RES_AVAIL, &sep);
    qlen += GetComponentEncodedLen(Q_MAC, uri->mac, Q_RES_AVAIL, &sep);

    return len + sep + qlen;
}

// encode the last 4 bits of an integer to a hex char
static inline uint32_t HexEncode(uint32_t v)
{
    v &= 0xf;
    if (v < DEC_LEN) {
        return ('0' + v);
    } else {
        return ('A' + v - DEC_LEN);
    }
}

static int32_t EncodeComp(
    char *buf, uint32_t *offset, uint32_t *available,
    const char *key, const char *value,
    const char *resAvail,
    uint32_t *sep, char sepChar)
{
    if (value == NULL) {
        return CMR_OK;
    }

    size_t resAvailLen = strlen(resAvail);
    size_t keyLen = strlen(key);
    size_t valueLen = strlen(value);
    uint32_t off = *offset;
    uint32_t avail = *available;

    if (avail < *sep + keyLen + valueLen) {
        return CMR_ERROR;
    }

    if (*sep) {
        buf[off] = sepChar;
        off++;
    }

    if (EOK != memcpy_s(buf + off, avail, key, keyLen)) {
        return CMR_ERROR;
    }
    off += keyLen;
    avail -= keyLen;

    for (size_t i = 0; i < valueLen; i++) {
        if (IsUnreserved(resAvail, resAvailLen, value[i])) {
            if (avail < 1) {
                return CMR_ERROR;
            }
            buf[off] = value[i];
            off++;
            avail--;
        } else {
            // each percent-encoded character requires 3 bytes
            if (avail < 3) {
                return CMR_ERROR;
            }
            buf[off] = '%';
            off++;
            buf[off] = (char) HexEncode(value[i] >> 4); // higher 4 bits of the char byte
            off++;
            buf[off] = (char) HexEncode(value[i]); // lower 4 bits of the char byte
            off++;
            // each percent-encoded character requires 3 bytes
            avail -= 3;
        }
    }

    *sep = 1;
    *offset = off;
    *available = avail;
    return CMR_OK;
}

int32_t CertManagerUriEncode(char *encoded, uint32_t *encodedLen, const struct CMUri *uri)
{
    ASSERT_ARGS(encodedLen);
    ASSERT_ARGS(uri && IS_TYPE_VALID(uri->type));

    uint32_t encLen = GetEncodedLen(uri) + 1;
    if (encoded == NULL) {
        *encodedLen = encLen;
        return CMR_OK;
    }

    if (*encodedLen < encLen) {
        CM_LOG_W("Buffer to small for encoded URI (%u < %u).\n", *encodedLen, encLen);
        return CMR_ERROR_BUFFER_TOO_SMALL;
    }

    uint32_t sep = 0;
    uint32_t off = 0;
    uint32_t avail = *encodedLen;

    if (EOK != memcpy_s(encoded, avail, SCHEME, strlen(SCHEME))) {
        return CMR_ERROR;
    }
    off += strlen(SCHEME);
    avail -= strlen(SCHEME);

    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, P_TYPE, g_types[uri->type], P_RES_AVAIL, &sep, ';'));
    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, P_OBJECT, uri->object, P_RES_AVAIL, &sep, ';'));
    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, P_USER, uri->user, P_RES_AVAIL, &sep, ';'));
    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, P_APP, uri->app, P_RES_AVAIL, &sep, ';'));

    if (uri->clientUser == NULL && uri->clientApp == NULL && uri->mac == NULL) {
        // no query. we are done.
        *encodedLen = off;
        return CMR_OK;
    }

    encoded[off] = '?';
    off++;
    avail--;
    sep = 0;
    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, Q_CLIENT_USER, uri->clientUser, Q_RES_AVAIL, &sep, '&'));
    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, Q_CLIENT_APP, uri->clientApp, Q_RES_AVAIL, &sep, '&'));
    ASSERT_FUNC(EncodeComp(encoded, &off, &avail, Q_MAC, uri->mac, Q_RES_AVAIL, &sep, '&'));

    *encodedLen = off;
    return CMR_OK;
}

static uint32_t HexDecode(uint32_t h)
{
    h &= 0xff;
    if (h >= '0' && h <= '9') {
        return h - '0';
    }
    if (h >= 'a' && h <= 'f') {
        return h - 'a' + DEC_LEN;
    }
    if (h >= 'A' && h <= 'F') {
        return h - 'A' + DEC_LEN;
    }
    return 0;
}

static inline uint32_t HexDecode2(uint32_t h1, uint32_t h2)
{
    return ((HexDecode(h1) << 4) | HexDecode(h2)) & 0xff; /* 4 is number of shifts */
}

static inline uint32_t IndexOf(char sep, const char *data, uint32_t start, uint32_t end)
{
    for (uint32_t i = start; i < end; i++) {
        if (data[i] == sep) {
            return i;
        }
    }
    return end;
}

static char *DecodeValue(const char *s, uint32_t off, uint32_t len)
{
    if (s == NULL || len == 0) {
        return NULL;
    }
    char *buf = MALLOC(len + 1);
    (void)memset_s(buf, len + 1, 0, len + 1);

    uint32_t bufOff = 0;
    for (uint32_t i = off; i < off + len; i++, bufOff++) {
        if (s[i] != '%') {
            buf[bufOff] = s[i];
        } else {
            buf[bufOff] = HexDecode2(s[i+1], s[i+2]); /* 2 is array index */
            i += 2; /* 2 is array index */
        }
    }
    char *ret = strndup(buf, bufOff);
    free(buf);
    return ret;
}

static uint32_t DecodeEnum(const char *s, uint32_t off, uint32_t len, const char *values[], uint32_t valueCount)
{
    for (uint32_t i = 0; i < valueCount; i++) {
        size_t valLen = strlen(values[i]);
        if (valLen == len && memcmp(s + off, values[i], len) == 0) {
            return i;
        }
    }
    // no match found, default value is an invalid enum value
    return valueCount + 1;
}

static int32_t DecodePath(struct CMUri *uri, const char *path, uint32_t start, uint32_t end)
{
    while (start < end) {
        uint32_t i = IndexOf(';', path, start, end);
        if (i <= start) {
            // something is wrong
            CM_LOG_W("Invalid uri path: %s\n", path);
            return CMR_ERROR_INVALID_ARGUMENT;
        }

        uint32_t valueOff = 0;
        uint32_t valueLen = 0;

        // for string field
        char **field = NULL;

        // for enum field
        uint32_t *e = NULL;
        const char **values = NULL;
        uint32_t valueCount = 0;

        if (!strncmp(P_OBJECT, path + start, strlen(P_OBJECT))) {
            valueOff = start + strlen(P_OBJECT);
            valueLen = i - start - strlen(P_OBJECT);
            field = &uri->object;
        } else if (!strncmp(P_TYPE, path + start, strlen(P_TYPE))) {
            valueOff = start + strlen(P_TYPE);
            valueLen = i - start - strlen(P_TYPE);
            e = &uri->type;
            values = g_types;
            valueCount = TYPE_COUNT;
        } else if (!strncmp(P_USER, path + start, strlen(P_USER))) {
            valueOff = start + strlen(P_USER);
            valueLen = i - start - strlen(P_USER);
            field = &uri->user;
        } else if (!strncmp(P_APP, path + start, strlen(P_APP))) {
            valueOff = start + strlen(P_APP);
            valueLen = i - start - strlen(P_APP);
            field = &uri->app;
        }

        if (field != NULL) {
            if (valueLen == 0) {
                *field = NULL;
            } else {
                *field = DecodeValue(path, valueOff, valueLen);
            }
        } else if (e != NULL) {
            *e = DecodeEnum(path, valueOff, valueLen, values, valueCount);
        } else {
            CM_LOG_W("Invalid field in path: %s\n", path);
            return CMR_ERROR_INVALID_ARGUMENT;
        }

        start = i + 1;
    }

    return CMR_OK;
}

static int32_t DecodeQuery(struct CMUri *uri, const char *query, uint32_t start, uint32_t end)
{
    while (start < end) {
        uint32_t i = IndexOf('&', query, start, end);
        if (i <= start) {
            // something is wrong
            CM_LOG_W("Invalid uri query: %s\n", query);
            return CMR_ERROR_INVALID_ARGUMENT;
        }

        uint32_t valueOff = 0;
        uint32_t valueLen = 0;
        char **field = NULL;
        if (!strncmp(Q_CLIENT_USER, query + start, strlen(Q_CLIENT_USER))) {
            valueOff = start + strlen(Q_CLIENT_USER);
            valueLen = i - start - strlen(Q_CLIENT_USER);
            field = &uri->clientUser;
        } else if (!strncmp(Q_CLIENT_APP, query + start, strlen(Q_CLIENT_APP))) {
            valueOff = start + strlen(Q_CLIENT_APP);
            valueLen = i - start - strlen(Q_CLIENT_APP);
            field = &uri->clientApp;
        } else if (!strncmp(Q_MAC, query + start, strlen(Q_MAC))) {
            valueOff = start + strlen(Q_MAC);
            valueLen = i - start - strlen(Q_MAC);
            field = &uri->mac;
        }

        if (field != NULL) {
            if (valueLen == 0) {
                *field = NULL;
            } else {
                *field = DecodeValue(query, valueOff, valueLen);
            }
        } else {
            CM_LOG_W("Invalid field in query: %s\n", query);
            return CMR_ERROR_INVALID_ARGUMENT;
        }

        start = i + 1;
    }
    return CMR_OK;
}

int32_t CertManagerUriDecode(struct CMUri *uri, const char *encoded)
{
    ASSERT_ARGS(uri);
    ASSERT_ARGS(encoded);

    uri->type = CM_URI_TYPE_INVALID;

    uint32_t len = strlen(encoded);
    uint32_t off = 0;
    CM_LOG_I("CertManagerUriDecode keyUri:%s, %s, len:%d, schem:%d", encoded, SCHEME, len, strlen(SCHEME));
    if (len < strlen(SCHEME) || memcmp(encoded, SCHEME, strlen(SCHEME))) {
        CM_LOG_W("Scheme mismatch. Not a cert manager URI: %s\n", encoded);
        return CMR_ERROR_INVALID_ARGUMENT;
    }
    off += strlen(SCHEME);

    uint32_t pathStart = off;
    uint32_t pathEnd = IndexOf('?', encoded, off, len);
    uint32_t queryStart = pathEnd == len ? len : pathEnd + 1;
    uint32_t queryEnd = len;

    ASSERT_FUNC(DecodePath(uri, encoded, pathStart, pathEnd));
    ASSERT_FUNC(DecodeQuery(uri, encoded, queryStart, queryEnd));

    return CMR_OK;
}

int32_t CertManagerGetUidFromUri(const struct CmBlob *uri, uint32_t *uid)
{
    struct CMUri uriObj;
    (void)memset_s(&uriObj, sizeof(uriObj), 0, sizeof(uriObj));
    int32_t ret = CertManagerUriDecode(&uriObj, (char *)uri->data);
    if (ret != CM_SUCCESS) {
        CM_LOG_E("uri decode failed, ret = %d", ret);
        return ret;
    }

    if (uriObj.app == NULL) {
        CM_LOG_E("uri app invalid");
        (void)CertManagerFreeUri(&uriObj);
        return CMR_ERROR_INVALID_ARGUMENT;
    }

    *uid = atoi(uriObj.app);
    (void)CertManagerFreeUri(&uriObj);
    return CM_SUCCESS;
}

#ifdef __cplusplus
}
#endif
