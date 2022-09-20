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

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "cm_napi_common.h"

#include "cm_napi_get_system_cert_list.h"
#include "cm_napi_get_system_cert_info.h"
#include "cm_napi_set_cert_status.h"
#include "cm_napi_install_app_cert.h"
#include "cm_napi_uninstall_app_cert.h"
#include "cm_napi_uninstall_all_app_cert.h"
#include "cm_napi_get_app_cert_list.h"
#include "cm_napi_get_app_cert_info.h"


namespace CMNapi {
    inline void AddInt32Property(napi_env env, napi_value object, const char *name, int32_t value)
    {
        napi_value property = nullptr;
        NAPI_CALL_RETURN_VOID(env, napi_create_int32(env, value, &property));
        NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, object, name, property));
    }

    static void AddCMErrorCodePart(napi_env env, napi_value errorCode)
    {
        AddInt32Property(env, errorCode, "CM_SUCCESS", CM_SUCCESS);
        AddInt32Property(env, errorCode, "CM_FAILURE", CM_FAILURE);
        AddInt32Property(env, errorCode, "CMR_ERROR_NOT_PERMITTED", CMR_ERROR_NOT_PERMITTED);
        AddInt32Property(env, errorCode, "CMR_ERROR_NOT_SUPPORTED", CMR_ERROR_NOT_SUPPORTED);
        AddInt32Property(env, errorCode, "CMR_ERROR_STORAGE", CMR_ERROR_STORAGE);
        AddInt32Property(env, errorCode, "CMR_ERROR_NOT_FOUND", CMR_ERROR_NOT_FOUND);
        AddInt32Property(env, errorCode, "CMR_ERROR_NULL_POINTER", CMR_ERROR_NULL_POINTER);
        AddInt32Property(env, errorCode, "CMR_ERROR_INVALID_ARGUMENT", CMR_ERROR_INVALID_ARGUMENT);
        AddInt32Property(env, errorCode, "CMR_ERROR_MAKE_DIR_FAIL", CMR_ERROR_MAKE_DIR_FAIL);
        AddInt32Property(env, errorCode, "CMR_ERROR_INVALID_OPERATION", CMR_ERROR_INVALID_OPERATION);
        AddInt32Property(env, errorCode, "CMR_ERROR_OPEN_FILE_FAIL", CMR_ERROR_OPEN_FILE_FAIL);
        AddInt32Property(env, errorCode, "CMR_ERROR_READ_FILE_ERROR", CMR_ERROR_READ_FILE_ERROR);
        AddInt32Property(env, errorCode, "CMR_ERROR_WRITE_FILE_FAIL", CMR_ERROR_WRITE_FILE_FAIL);
        AddInt32Property(env, errorCode, "CMR_ERROR_REMOVE_FILE_FAIL", CMR_ERROR_REMOVE_FILE_FAIL);
        AddInt32Property(env, errorCode, "CMR_ERROR_CLOSE_FILE_FAIL", CMR_ERROR_CLOSE_FILE_FAIL);
        AddInt32Property(env, errorCode, "CMR_ERROR_MALLOC_FAIL", CMR_ERROR_MALLOC_FAIL);
        AddInt32Property(env, errorCode, "CMR_ERROR_NOT_EXIST", CMR_ERROR_NOT_EXIST);
        AddInt32Property(env, errorCode, "CMR_ERROR_ALREADY_EXISTS", CMR_ERROR_ALREADY_EXISTS);
        AddInt32Property(env, errorCode, "CMR_ERROR_INSUFFICIENT_DATA", CMR_ERROR_INSUFFICIENT_DATA);
        AddInt32Property(env, errorCode, "CMR_ERROR_BUFFER_TOO_SMALL", CMR_ERROR_BUFFER_TOO_SMALL);
        AddInt32Property(env, errorCode, "CMR_ERROR_INVALID_CERT_FORMAT", CMR_ERROR_INVALID_CERT_FORMAT);
        AddInt32Property(env, errorCode, "CMR_ERROR_PARAM_NOT_EXIST", CMR_ERROR_PARAM_NOT_EXIST);
    }

    static napi_value CreateCMErrorCode(napi_env env)
    {
        napi_value errorCode = nullptr;
        NAPI_CALL(env, napi_create_object(env, &errorCode));

        AddCMErrorCodePart(env, errorCode);

        return errorCode;
    }
}  // namespace CertManagerNapi

using namespace CMNapi;

extern "C" {
    static napi_value CMNapiRegister(napi_env env, napi_value exports)
    {
        napi_property_descriptor desc[] = {
            DECLARE_NAPI_PROPERTY("CMErrorCode", CreateCMErrorCode(env)),

            DECLARE_NAPI_FUNCTION("getSystemTrustedCertificateList", CMNapiGetSystemCertList),
            DECLARE_NAPI_FUNCTION("getSystemTrustedCertificate", CMNapiGetSystemCertInfo),
            DECLARE_NAPI_FUNCTION("setCertificateStatus", CMNapiSetCertStatus),
            DECLARE_NAPI_FUNCTION("installAppCertificate", CMNapiInstallAppCert),
            DECLARE_NAPI_FUNCTION("uninstallAllAppCertificate", CMNapiUninstallAllAppCert),
            DECLARE_NAPI_FUNCTION("uninstallAppCertificate", CMNapiUninstallAppCert),
            DECLARE_NAPI_FUNCTION("getAppCertificateList", CMNapiGetAppCertList),
            DECLARE_NAPI_FUNCTION("getAppCertificate", CMNapiGetAppCertInfo),
        };
        NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
        return exports;
    }

    static napi_module g_module = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = CMNapiRegister,
        .nm_modname = "security.certmanager",
        .nm_priv = ((void *)0),
        .reserved = {0},
    };

    __attribute__((constructor)) void CertManagerRegister(void)
    {
        napi_module_register(&g_module);
    }
}