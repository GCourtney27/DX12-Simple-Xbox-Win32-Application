// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.201113.7

#ifndef WINRT_Windows_Phone_System_UserProfile_GameServices_Core_0_H
#define WINRT_Windows_Phone_System_UserProfile_GameServices_Core_0_H
WINRT_EXPORT namespace winrt::Windows::Foundation
{
    template <typename TResult> struct __declspec(empty_bases) IAsyncOperation;
    struct Uri;
}
WINRT_EXPORT namespace winrt::Windows::Storage::Streams
{
    struct IBuffer;
}
WINRT_EXPORT namespace winrt::Windows::Phone::System::UserProfile::GameServices::Core
{
    enum class GameServiceGameOutcome : int32_t
    {
        None = 0,
        Win = 1,
        Loss = 2,
        Tie = 3,
    };
    enum class GameServiceScoreKind : int32_t
    {
        Number = 0,
        Time = 1,
    };
    struct IGameService;
    struct IGameService2;
    struct IGameServicePropertyCollection;
    struct GameService;
    struct GameServicePropertyCollection;
}
namespace winrt::impl
{
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::IGameService>{ using type = interface_category; };
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::IGameService2>{ using type = interface_category; };
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::IGameServicePropertyCollection>{ using type = interface_category; };
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::GameService>{ using type = class_category; };
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::GameServicePropertyCollection>{ using type = class_category; };
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::GameServiceGameOutcome>{ using type = enum_category; };
    template <> struct category<Windows::Phone::System::UserProfile::GameServices::Core::GameServiceScoreKind>{ using type = enum_category; };
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::GameService> = L"Windows.Phone.System.UserProfile.GameServices.Core.GameService";
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::GameServicePropertyCollection> = L"Windows.Phone.System.UserProfile.GameServices.Core.GameServicePropertyCollection";
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::GameServiceGameOutcome> = L"Windows.Phone.System.UserProfile.GameServices.Core.GameServiceGameOutcome";
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::GameServiceScoreKind> = L"Windows.Phone.System.UserProfile.GameServices.Core.GameServiceScoreKind";
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::IGameService> = L"Windows.Phone.System.UserProfile.GameServices.Core.IGameService";
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::IGameService2> = L"Windows.Phone.System.UserProfile.GameServices.Core.IGameService2";
    template <> inline constexpr auto& name_v<Windows::Phone::System::UserProfile::GameServices::Core::IGameServicePropertyCollection> = L"Windows.Phone.System.UserProfile.GameServices.Core.IGameServicePropertyCollection";
    template <> inline constexpr guid guid_v<Windows::Phone::System::UserProfile::GameServices::Core::IGameService>{ 0x2E2D5098,0x48A9,0x4EFC,{ 0xAF,0xD6,0x8E,0x6D,0xA0,0x90,0x03,0xFB } }; // 2E2D5098-48A9-4EFC-AFD6-8E6DA09003FB
    template <> inline constexpr guid guid_v<Windows::Phone::System::UserProfile::GameServices::Core::IGameService2>{ 0xD2364EF6,0xEA17,0x4BE5,{ 0x8D,0x8A,0xC8,0x60,0x88,0x5E,0x05,0x1F } }; // D2364EF6-EA17-4BE5-8D8A-C860885E051F
    template <> inline constexpr guid guid_v<Windows::Phone::System::UserProfile::GameServices::Core::IGameServicePropertyCollection>{ 0x07E57FC8,0xDEBB,0x4609,{ 0x9C,0xC8,0x52,0x9D,0x16,0xBC,0x2B,0xD9 } }; // 07E57FC8-DEBB-4609-9CC8-529D16BC2BD9
    template <> struct default_interface<Windows::Phone::System::UserProfile::GameServices::Core::GameServicePropertyCollection>{ using type = Windows::Phone::System::UserProfile::GameServices::Core::IGameServicePropertyCollection; };
    template <> struct abi<Windows::Phone::System::UserProfile::GameServices::Core::IGameService>
    {
        struct __declspec(novtable) type : inspectable_abi
        {
            virtual int32_t __stdcall get_ServiceUri(void**) noexcept = 0;
            virtual int32_t __stdcall GetGamerProfileAsync(void**) noexcept = 0;
            virtual int32_t __stdcall GetInstalledGameItemsAsync(void**) noexcept = 0;
            virtual int32_t __stdcall GetPartnerTokenAsync(void*, void**) noexcept = 0;
            virtual int32_t __stdcall GetPrivilegesAsync(void**) noexcept = 0;
            virtual int32_t __stdcall GrantAchievement(uint32_t) noexcept = 0;
            virtual int32_t __stdcall GrantAvatarAward(uint32_t) noexcept = 0;
            virtual int32_t __stdcall PostResult(uint32_t, int32_t, int64_t, int32_t, void*) noexcept = 0;
        };
    };
    template <> struct abi<Windows::Phone::System::UserProfile::GameServices::Core::IGameService2>
    {
        struct __declspec(novtable) type : inspectable_abi
        {
            virtual int32_t __stdcall NotifyPartnerTokenExpired(void*) noexcept = 0;
            virtual int32_t __stdcall GetAuthenticationStatus(uint32_t*) noexcept = 0;
        };
    };
    template <> struct abi<Windows::Phone::System::UserProfile::GameServices::Core::IGameServicePropertyCollection>
    {
        struct __declspec(novtable) type : inspectable_abi
        {
            virtual int32_t __stdcall GetPropertyAsync(void*, void**) noexcept = 0;
        };
    };
    template <typename D>
    struct consume_Windows_Phone_System_UserProfile_GameServices_Core_IGameService
    {
        [[nodiscard]] WINRT_IMPL_AUTO(Windows::Foundation::Uri) ServiceUri() const;
        WINRT_IMPL_AUTO(Windows::Foundation::IAsyncOperation<Windows::Phone::System::UserProfile::GameServices::Core::GameServicePropertyCollection>) GetGamerProfileAsync() const;
        WINRT_IMPL_AUTO(Windows::Foundation::IAsyncOperation<Windows::Phone::System::UserProfile::GameServices::Core::GameServicePropertyCollection>) GetInstalledGameItemsAsync() const;
        WINRT_IMPL_AUTO(Windows::Foundation::IAsyncOperation<hstring>) GetPartnerTokenAsync(Windows::Foundation::Uri const& audienceUri) const;
        WINRT_IMPL_AUTO(Windows::Foundation::IAsyncOperation<hstring>) GetPrivilegesAsync() const;
        WINRT_IMPL_AUTO(void) GrantAchievement(uint32_t achievementId) const;
        WINRT_IMPL_AUTO(void) GrantAvatarAward(uint32_t avatarAwardId) const;
        WINRT_IMPL_AUTO(void) PostResult(uint32_t gameVariant, Windows::Phone::System::UserProfile::GameServices::Core::GameServiceScoreKind const& scoreKind, int64_t scoreValue, Windows::Phone::System::UserProfile::GameServices::Core::GameServiceGameOutcome const& gameOutcome, Windows::Storage::Streams::IBuffer const& buffer) const;
    };
    template <> struct consume<Windows::Phone::System::UserProfile::GameServices::Core::IGameService>
    {
        template <typename D> using type = consume_Windows_Phone_System_UserProfile_GameServices_Core_IGameService<D>;
    };
    template <typename D>
    struct consume_Windows_Phone_System_UserProfile_GameServices_Core_IGameService2
    {
        WINRT_IMPL_AUTO(void) NotifyPartnerTokenExpired(Windows::Foundation::Uri const& audienceUri) const;
        WINRT_IMPL_AUTO(uint32_t) GetAuthenticationStatus() const;
    };
    template <> struct consume<Windows::Phone::System::UserProfile::GameServices::Core::IGameService2>
    {
        template <typename D> using type = consume_Windows_Phone_System_UserProfile_GameServices_Core_IGameService2<D>;
    };
    template <typename D>
    struct consume_Windows_Phone_System_UserProfile_GameServices_Core_IGameServicePropertyCollection
    {
        WINRT_IMPL_AUTO(Windows::Foundation::IAsyncOperation<Windows::Foundation::IInspectable>) GetPropertyAsync(param::hstring const& propertyName) const;
    };
    template <> struct consume<Windows::Phone::System::UserProfile::GameServices::Core::IGameServicePropertyCollection>
    {
        template <typename D> using type = consume_Windows_Phone_System_UserProfile_GameServices_Core_IGameServicePropertyCollection<D>;
    };
}
#endif
