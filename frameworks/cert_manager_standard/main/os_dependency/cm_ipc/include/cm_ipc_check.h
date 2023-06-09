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

#ifndef CM_CRYPTO_CHECK_H
#define CM_CRYPTO_CHECK_H

#include "cm_type_inner.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t CopyUint32ToBuffer(uint32_t value, const struct CmBlob *destBlob, uint32_t *destOffset);

int32_t GetUint32FromBuffer(uint32_t *value, const struct CmBlob *srcBlob, uint32_t *srcOffset);

int32_t CmGetBlobFromBuffer(struct CmBlob *blob, const struct CmBlob *srcBlob, uint32_t *srcOffset);

int32_t CopyBlobToBuffer(const struct CmBlob *blob, const struct CmBlob *destBlob, uint32_t *destOffset);

int32_t CheckCertificateListPara(const struct CmBlob *inBlob, const struct CmBlob *outBlob,
    const struct CmContext *cmContext, const uint32_t store, const struct CertList *certificateList);

int32_t CheckCertificateInfoPara(const struct CmBlob *inBlob, const struct CmBlob *outBlob,
    const struct CmContext *cmContext, const uint32_t store, const struct CertInfo *certificateInfo);

#ifdef __cplusplus
}
#endif

#endif