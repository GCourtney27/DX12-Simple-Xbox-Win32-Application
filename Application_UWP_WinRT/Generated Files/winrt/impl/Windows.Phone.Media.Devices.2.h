// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.201113.7

#ifndef WINRT_Windows_Phone_Media_Devices_2_H
#define WINRT_Windows_Phone_Media_Devices_2_H
#include "winrt/impl/Windows.Phone.Media.Devices.1.h"
WINRT_EXPORT namespace winrt::Windows::Phone::Media::Devices
{
    struct __declspec(empty_bases) AudioRoutingManager : Windows::Phone::Media::Devices::IAudioRoutingManager
    {
        AudioRoutingManager(std::nullptr_t) noexcept {}
        AudioRoutingManager(void* ptr, take_ownership_from_abi_t) noexcept : Windows::Phone::Media::Devices::IAudioRoutingManager(ptr, take_ownership_from_abi) {}
        static auto GetDefault();
    };
}
#endif
