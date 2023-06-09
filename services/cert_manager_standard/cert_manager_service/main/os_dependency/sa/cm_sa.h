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

#ifndef CM_SA_H
#define CM_SA_H

#include "iremote_broker.h"
#include "iremote_stub.h"
#include "nocopyable.h"
#include "system_ability.h"

namespace OHOS {
namespace Security {
namespace CertManager {
enum ServiceRunningState {
    STATE_NOT_START,
    STATE_RUNNING
};
enum ResponseCode {
    HW_NO_ERROR =  0,
    HW_SYSTEM_ERROR = -1,
    HW_PERMISSION_DENIED = -2,
};

constexpr int SA_ID_KEYSTORE_SERVICE = 3512;

class ICertManagerService : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.security.cm.service");
};

class CertManagerService : public SystemAbility, public IRemoteStub<ICertManagerService> {
    DECLEAR_SYSTEM_ABILITY(CertManagerService)

public:
    DISALLOW_COPY_AND_MOVE(CertManagerService);
    CertManagerService(int saId, bool runOnCreate);
    virtual ~CertManagerService();

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    static sptr<CertManagerService> GetInstance();

protected:
    void OnStart() override;
    void OnStop() override;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;

private:
    CertManagerService();
    bool Init();

    bool registerToService_;
    ServiceRunningState runningState_;
    static std::mutex instanceLock;
    static sptr<CertManagerService> instance;
};
} // namespace CertManager
} // namespace Security
} // namespace OHOS

#endif // CM_SA_H