/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CERT_MANAGER_SERVICE_H
#define CERT_MANAGER_SERVICE_H

#include "cm_type.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t CmServiceGrantAppCertificate(const struct CmContext *context, const struct CmBlob *keyUri,
    uint32_t appUid, struct CmBlob *authUri);

int32_t CmServiceGetAuthorizedAppList(const struct CmContext *context, const struct CmBlob *keyUri,
    struct CmAppUidList *appUidList);

int32_t CmServiceIsAuthorizedApp(const struct CmContext *context, const struct CmBlob *authUri);

int32_t CmServiceRemoveGrantedApp(const struct CmContext *context, const struct CmBlob *keyUri, uint32_t appUid);

int32_t CmServiceInit(const struct CmContext *context, const struct CmBlob *authUri,
    const struct CmSignatureSpec *spec, struct CmBlob *handle);

int32_t CmServiceUpdate(const struct CmContext *context, const struct CmBlob *handle,
    const struct CmBlob *inData);

int32_t CmServiceFinish(const struct CmContext *context, const struct CmBlob *handle,
    const struct CmBlob *inData, struct CmBlob *outData);

int32_t CmServiceAbort(const struct CmContext *context, const struct CmBlob *handle);

#ifdef __cplusplus
}
#endif

#endif /* CERT_MANAGER_SERVICE_H */