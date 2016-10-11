// Copyright (C) 2014-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <CommonAPI/SomeIP/ProxyManager.hpp>
#include <CommonAPI/Logger.hpp>

namespace CommonAPI {
namespace SomeIP {

ProxyManager::ProxyManager(Proxy &_proxy, const std::string &_interfaceName,
                           const service_id_t &_serviceId) :
    proxy_(_proxy),
    interfaceId_(_interfaceName),
    instanceAvailabilityStatusEvent_(
            std::make_shared<
                    CommonAPI::SomeIP::InstanceAvailabilityStatusChangedEvent>(
                    _interfaceName)),
    availabilityHandlerId_(0),
    observedInterfaceServiceId_(_serviceId) {

    std::function<void(service_id_t, instance_id_t, bool)> availabilityHandler =
            std::bind(
                    &CommonAPI::SomeIP::InstanceAvailabilityStatusChangedEvent::onServiceInstanceStatus,
                    instanceAvailabilityStatusEvent_, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3);
    Address wildcardAddress(_serviceId, vsomeip::ANY_INSTANCE, vsomeip::ANY_MAJOR, vsomeip::ANY_MINOR);

    availabilityHandlerId_ = proxy_.getConnection()->registerAvailabilityHandler(
                                wildcardAddress, availabilityHandler);
}

ProxyManager::~ProxyManager() {
    Address wildcardAddress(observedInterfaceServiceId_, vsomeip::ANY_INSTANCE,  vsomeip::ANY_MAJOR, vsomeip::ANY_MINOR);

    proxy_.getConnection()->unregisterAvailabilityHandler(
            wildcardAddress, availabilityHandlerId_);
}

const std::string &
ProxyManager::getDomain() const {
    static std::string domain("local");
    return domain;
}

const std::string &
ProxyManager::getInterface() const {
    return interfaceId_;
}

const ConnectionId_t &
ProxyManager::getConnectionId() const {
    // Every connection is created as Connection
    // in Factory::createProxy and is only stored as ProxyConnection
    return std::static_pointer_cast<Connection>(proxy_.getConnection())->getConnectionId();
}

void
ProxyManager::getAvailableInstances(CommonAPI::CallStatus &_callStatus,
                      std::vector<std::string> &_instances) {
    instanceAvailabilityStatusEvent_->getAvailableInstances(&_instances);
    _callStatus = CommonAPI::CallStatus::SUCCESS;
}

std::future<CallStatus>
ProxyManager::getAvailableInstancesAsync(
        CommonAPI::ProxyManager::GetAvailableInstancesCallback _callback) {
    std::thread t([this, _callback](){
        std::vector<std::string> instances;
        instanceAvailabilityStatusEvent_->getAvailableInstances(&instances);
        _callback(CommonAPI::CallStatus::SUCCESS, instances);
    });
    t.detach();
    std::promise<CallStatus> promise;
    promise.set_value(CallStatus::SUCCESS);
    return promise.get_future();
}

void
ProxyManager::getInstanceAvailabilityStatus(const std::string &_instanceAddress,
                                            CallStatus &_callStatus,
                                            AvailabilityStatus &_availabilityStatus) {
    CommonAPI::Address itsAddress("local", interfaceId_, _instanceAddress);
    instanceAvailabilityStatusEvent_->getInstanceAvailabilityStatus(
            itsAddress.getAddress(), &_availabilityStatus);
    _callStatus = CommonAPI::CallStatus::SUCCESS;
}

std::future<CallStatus>
ProxyManager::getInstanceAvailabilityStatusAsync(
        const std::string &_instanceAddress,
        CommonAPI::ProxyManager::GetInstanceAvailabilityStatusCallback _callback) {
    std::thread t([this, _instanceAddress, _callback](){
        CommonAPI::Address itsAddress("local", interfaceId_, _instanceAddress);
        std::vector<std::string> instances;
        CommonAPI::AvailabilityStatus availablityStatus;
        instanceAvailabilityStatusEvent_->getInstanceAvailabilityStatus(
                itsAddress.getAddress(), &availablityStatus);
        _callback(CommonAPI::CallStatus::SUCCESS, availablityStatus);
    });
    t.detach();
    std::promise<CallStatus> promise;
    promise.set_value(CallStatus::SUCCESS);
    return promise.get_future();
}

ProxyManager::InstanceAvailabilityStatusChangedEvent&
ProxyManager::getInstanceAvailabilityStatusChangedEvent() {
    return *instanceAvailabilityStatusEvent_;
}

} // namespace SomeIP
} // namespace CommonAPI
