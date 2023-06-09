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

#include "cm_log.h"
#include "cm_mem.h"
#include "cm_type_inner.h"
#include "cm_report_wrapper.h"

int32_t ReportFaultEvent(const char *funcName, const struct CmContext *cmContext, const char *certName, int32_t errorCode)
{
    if (errorCode == CM_SUCCESS) {
        return CM_SUCCESS;
    }
    int32_t ret;

    struct EventValues eventValues = { cmContext->userId, cmContext->uid, certName, errorCode };
    ret = WriteEvent(funcName, &eventValues);
    if (ret != CM_SUCCESS) {
        CM_LOG_E("ReportFaultEvent failed, ret = %d", ret);
    }
    return ret;
}

void CmReport(const char *funcName, const struct CmContext *cmContext, const char *certName, int32_t errorCode)
{
    int32_t ret = ReportFaultEvent(funcName, cmContext, certName, errorCode);
    if (ret != CM_SUCCESS) {
        CM_LOG_E("report fault event failed, ret = %d", ret);
    }
}