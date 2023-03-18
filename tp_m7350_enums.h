/** \file tp_m7350_enums.h
 *  This is a minimal C++ interface to communicate with the TP-Link M7350 modem
 *  web gateway interface. Constants definition.
 *  Note that I used structs with static members to be able to define one option
 *  group per module and use them with the same processing functions
 *  without explicit conversion. An alternative would be enum classes with
 *  std::to_underlying (requires C++23) or static_cast.
 *  Author: Vincent Paeder
 *  License: GPL v3
 */
#pragma once
#include <cstdint>
#include <string>

namespace tplink {

    /** \brief List of available modules. */
    struct Modules {
        static const std::string
        Authenticator, WebServer, Status,
        WAN, SimLock, Message, WLAN, WPS,
        PowerSave, FlowStat, ConnectedDevices,
        MACFilters, LAN, Update, StorageShare, Reboot,
        RestoreConf, Time, Log, APBridge, Voice,
        UPnP, DMZ, ALG, VirtualServer, PortTriggering;
    };
    const std::string Modules::Authenticator = "authenticator";
    const std::string Modules::WebServer = "webServer";
    const std::string Modules::Status = "status";
    const std::string Modules::WAN = "wan";
    const std::string Modules::SimLock = "simLock";
    const std::string Modules::Message = "message";
    const std::string Modules::WLAN = "wlan";
    const std::string Modules::WPS = "wps";
    const std::string Modules::PowerSave = "power_save";
    const std::string Modules::FlowStat = "flowstat";
    const std::string Modules::ConnectedDevices = "connectedDevices";
    const std::string Modules::MACFilters = "macFilters";
    const std::string Modules::LAN = "lan";
    const std::string Modules::Update = "update";
    const std::string Modules::StorageShare = "storageShare";
    const std::string Modules::Reboot = "reboot";
    const std::string Modules::RestoreConf = "restoreDefaults";
    const std::string Modules::Time = "time";
    const std::string Modules::Log = "log";
    const std::string Modules::APBridge = "apBridge";
    const std::string Modules::Voice = "voice";
    const std::string Modules::UPnP = "upnp";
    const std::string Modules::DMZ = "dmz";
    const std::string Modules::ALG = "alg";
    const std::string Modules::VirtualServer = "virtualServer";
    const std::string Modules::PortTriggering = "portTrigger";


    /** \brief Options for WLAN security type. */
    struct APSecurity {
        static const std::string
        NoPassword, WEP, WPA_TKIP, WPA_AES, WPA2_TKIP, WPA2_AES, WPA_WPA2, IEEE8201X, Unknown;
    };
    const std::string APSecurity::NoPassword = "noPassword";
    const std::string APSecurity::WEP = "wepSecurity";
    const std::string APSecurity::WPA_TKIP = "wpaTkipSecurity";
    const std::string APSecurity::WPA_AES = "wpaAesSecurity";
    const std::string APSecurity::WPA2_TKIP = "wpa2TkipSecurity";
    const std::string APSecurity::WPA2_AES = "wpa2AesSecurity";
    const std::string APSecurity::WPA_WPA2 = "wpaWpa2Security";
    const std::string APSecurity::IEEE8201X = "ieee8021XSecurity";
    const std::string APSecurity::Unknown = "unknownSecurity";

    /** \brief Options for Application Layer Gateway module. */
    struct ALGOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration; ///< Set configuration
    };
    const uint8_t ALGOptions::GetConfiguration = 0;
    const uint8_t ALGOptions::SetConfiguration = 1;


    /** \brief Options for access point bridge module. */
    struct APBridgeOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        ConnectAP, ///< Connect access point
        ScanAP, ///< Scan for access points
        CheckConnectionStatus; ///< Check connection status
    };
    const uint8_t APBridgeOptions::GetConfiguration = 0;
    const uint8_t APBridgeOptions::SetConfiguration = 1;
    const uint8_t APBridgeOptions::ConnectAP = 2;
    const uint8_t APBridgeOptions::ScanAP = 3;
    const uint8_t APBridgeOptions::CheckConnectionStatus = 4;


    /** \brief Options for authenticator module. */
    struct AuthenticatorOptions {
        static const uint8_t
        Load, ///< Load
        Login, ///< Log in
        GetAttempts, ///< Get number of failed login attempts
        Logout, ///< Log out
        Update; ///< Update
    };
    const uint8_t AuthenticatorOptions::Load = 0;
    const uint8_t AuthenticatorOptions::Login = 1;
    const uint8_t AuthenticatorOptions::GetAttempts = 2;
    const uint8_t AuthenticatorOptions::Logout = 3;
    const uint8_t AuthenticatorOptions::Update = 4;


    /** \brief Options for ConnectedDevices module. */
    struct ConnectedDevicesOptions {
        static const uint8_t
        GetConfiguration ///< Get configuration
    #if NEW_FIRMWARE==1
        , EditName ///< Edit name
    #endif
        ;
    };
    const uint8_t ConnectedDevicesOptions::GetConfiguration = 0;
    #if NEW_FIRMWARE==1
    const uint8_t ConnectedDevicesOptions::EditName = 1;
    #endif


    /** \brief Options for demilitarized zone module. */
    struct DMZOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration; ///< Set configuration
    };
    const uint8_t DMZOptions::GetConfiguration = 0;
    const uint8_t DMZOptions::SetConfiguration = 1;


    /** \brief Options for flow statistics module. */
    struct FlowStatOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration; ///< Set configuration
    };
    const uint8_t FlowStatOptions::GetConfiguration = 0;
    const uint8_t FlowStatOptions::SetConfiguration = 1;


    /** \brief Options for LAN module. */
    struct LANOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration; ///< Set configuration
    };
    const uint8_t LANOptions::GetConfiguration = 0;
    const uint8_t LANOptions::SetConfiguration = 1;


    /** \brief Options for log module. */
    struct LogOptions {
        static const uint8_t
        GetLog, ///< Get log
        ClearLog, ///< Clear log
        SaveLog, ///< Save log
        Refresh, ///< Refresh log
        GetMdLog, ///< Get log settings
        SetMdLog; ///< Set log settings
    };
    const uint8_t LogOptions::GetLog = 0;
    const uint8_t LogOptions::ClearLog = 1;
    const uint8_t LogOptions::SaveLog = 2;
    const uint8_t LogOptions::Refresh = 3;
    const uint8_t LogOptions::GetMdLog = 4;
    const uint8_t LogOptions::SetMdLog = 5;


    /** \brief Options for MAC filter module. */
    struct MACFiltersOptions {
        static const uint8_t
        GetBlackList, ///< Get black list
        SetBlackList; ///< Set black list
    };
    const uint8_t MACFiltersOptions::GetBlackList = 0;
    const uint8_t MACFiltersOptions::SetBlackList = 1;


    /** \brief Options for message (SMS) module. */
    struct MessageOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        ReadMessage, ///< Read message
        SendMessage, ///< Send message
        SaveMessage, ///< Save message
        DeleteMessage, ///< Delete message
        MarkAsRead, ///< Mark message as read
        GetSendStatus; ///< Get send status
    };
    const uint8_t MessageOptions::GetConfiguration = 0;
    const uint8_t MessageOptions::SetConfiguration = 1;
    const uint8_t MessageOptions::ReadMessage = 2;
    const uint8_t MessageOptions::SendMessage = 3;
    const uint8_t MessageOptions::SaveMessage = 4;
    const uint8_t MessageOptions::DeleteMessage = 5;
    const uint8_t MessageOptions::MarkAsRead = 6;
    const uint8_t MessageOptions::GetSendStatus = 7;


    /** \brief Options for port triggering module. */
    struct PortTriggeringOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        DeleteEntry; ///< Delete port triggering entry
    };
    const uint8_t PortTriggeringOptions::GetConfiguration = 0;
    const uint8_t PortTriggeringOptions::SetConfiguration = 1;
    const uint8_t PortTriggeringOptions::DeleteEntry = 2;


    /** \brief Options for power saving module. */
    struct PowerSavingOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration; ///< Set configuration
    };
    const uint8_t PowerSavingOptions::GetConfiguration = 0;
    const uint8_t PowerSavingOptions::SetConfiguration = 1;


    /** \brief Options for reboot module. */
    struct RebootOptions {
        static const uint8_t
        Reboot, ///< Reboot
        Shutdown; ///< Shutdown
    };
    const uint8_t RebootOptions::Reboot = 0;
    const uint8_t RebootOptions::Shutdown = 1;


    /** \brief Options for SIM lock module. */
    struct SIMLockOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        EnablePIN, ///< Enable PIN
        DisablePIN, ///< Disable PIN
        UpdatePIN, ///< Update PIN
        UnlockPIN, ///< Unlock PIN
        UnlockPUK, ///< Unlock PUK
        AutoUnlock; ///< Auto unlock
    };
    const uint8_t SIMLockOptions::GetConfiguration = 0;
    const uint8_t SIMLockOptions::EnablePIN = 1;
    const uint8_t SIMLockOptions::DisablePIN = 2;
    const uint8_t SIMLockOptions::UpdatePIN = 3;
    const uint8_t SIMLockOptions::UnlockPIN = 4;
    const uint8_t SIMLockOptions::UnlockPUK = 5;
    const uint8_t SIMLockOptions::AutoUnlock = 6;


    /** \brief Options for storage sharing module. */
    struct StorageShareOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration; ///< Set configuration
    };
    const uint8_t StorageShareOptions::GetConfiguration = 0;
    const uint8_t StorageShareOptions::SetConfiguration = 1;


    /** \brief Options for time module. */
    struct TimeOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        QueryTime; ///< Query time
    };
    const uint8_t TimeOptions::GetConfiguration = 0;
    const uint8_t TimeOptions::SetConfiguration = 1;
    const uint8_t TimeOptions::QueryTime = 2;


    /** \brief Options for firmware update module. */
    struct FirmwareUpdateOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        CheckNew, ///< Check for new version
        ServerUpdate, ///< Server update
        PauseLoad, ///< Pause loading
        RequestLoadPercentage, ///< Request load percentage
        CheckUploadResult, ///< Check upload result
        StartUpgrade, ///< Start upgrade
        ClearCache; ///< Clear cache
    };
    const uint8_t FirmwareUpdateOptions::GetConfiguration = 0;
    const uint8_t FirmwareUpdateOptions::CheckNew = 1;
    const uint8_t FirmwareUpdateOptions::ServerUpdate = 2;
    const uint8_t FirmwareUpdateOptions::PauseLoad = 3;
    const uint8_t FirmwareUpdateOptions::RequestLoadPercentage = 4;
    const uint8_t FirmwareUpdateOptions::CheckUploadResult = 5;
    const uint8_t FirmwareUpdateOptions::StartUpgrade = 6;
    const uint8_t FirmwareUpdateOptions::ClearCache = 7;


    /** \brief Options for UPnP module. */
    struct UPnPOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        GetUPnPDeviceList; ///< Get UPnP device list
    };
    const uint8_t UPnPOptions::GetConfiguration = 0;
    const uint8_t UPnPOptions::SetConfiguration = 1;
    const uint8_t UPnPOptions::GetUPnPDeviceList = 2;


    /** \brief Options for virtual server module. */
    struct VirtualServerOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        DeleteVirtualServer; ///< Delete virtual server
    };
    const uint8_t VirtualServerOptions::GetConfiguration = 0;
    const uint8_t VirtualServerOptions::SetConfiguration = 1;
    const uint8_t VirtualServerOptions::DeleteVirtualServer = 2;


    /** \brief Options for voice module. */
    struct VoiceOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SendUSSD, ///< Send USSD
        CancelUSSD, ///< Cancel USSD
        GetSendStatus; ///< Get send status
    };
    const uint8_t VoiceOptions::GetConfiguration = 0;
    const uint8_t VoiceOptions::SendUSSD = 1;
    const uint8_t VoiceOptions::CancelUSSD = 2;
    const uint8_t VoiceOptions::GetSendStatus = 3;


    /** \brief Options for WAN module. */
    struct WANOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        AddProfile, ///< Add profile
        DeleteProfile, ///< Delete profile
        SetNetworkSelectionMode, ///< Set network selection mode
        QueryAvailableNetworks, ///< Query available networks
        GetNetworkSelectionStatus, ///< Get network selection status
        GetDisconnectionReason, ///< Get disconnection reason
        CancelSearch, ///< Cancel search
        UpdateISP ///< Update ISP
    #if NEW_FIRMWARE==1
        , BandSearch, ///< Band search
        GetBandSearchStatus, ///< Get band search status
        SetSelectedBand, ///< Set selected band
        CancelBandSearch ///< Cancel band search
    #endif
        ;
    };
    const uint8_t WANOptions::GetConfiguration = 0;
    const uint8_t WANOptions::SetConfiguration = 1;
    const uint8_t WANOptions::AddProfile = 2;
    const uint8_t WANOptions::DeleteProfile = 3;
    const uint8_t WANOptions::SetNetworkSelectionMode = 8;
    const uint8_t WANOptions::QueryAvailableNetworks = 9;
    const uint8_t WANOptions::GetNetworkSelectionStatus = 10;
    const uint8_t WANOptions::GetDisconnectionReason = 11;
    const uint8_t WANOptions::CancelSearch = 14;
    const uint8_t WANOptions::UpdateISP = 15;
#if NEW_FIRMWARE==1
    const uint8_t WANOptions::BandSearch = 16;
    const uint8_t WANOptions::GetBandSearchStatus = 17;
    const uint8_t WANOptions::SetSelectedBand = 18;
    const uint8_t WANOptions::CancelBandSearch = 19;
#endif


    /** \brief Options for web server module. */
    struct WebServerOptions {
        static const uint8_t
        GetLanguage, ///< Get language
        SetLanguage, ///< Set language
        KeepAlive, ///< Keep alive
        UnsetDefault, ///< Unset default
        GetModuleList, ///< Get module list
        GetFeatureList ///< Get feature list
    #if NEW_FIRMWARE==1
        , GetInfoWithoutAuthentication ///< Get info without authentication
    #endif
        ;
    };
    const uint8_t WebServerOptions::GetLanguage = 0;
    const uint8_t WebServerOptions::SetLanguage = 1;
    const uint8_t WebServerOptions::KeepAlive = 2;
    const uint8_t WebServerOptions::UnsetDefault = 3;
    const uint8_t WebServerOptions::GetModuleList = 4;
    const uint8_t WebServerOptions::GetFeatureList = 5;
    #if NEW_FIRMWARE==1
    const uint8_t WebServerOptions::GetInfoWithoutAuthentication = 6;
    #endif

    /** \brief Options for WLAN module. */
    struct WLANOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        SetNoWLAN; ///< Set no WLAN
    };
    const uint8_t WLANOptions::GetConfiguration = 0;
    const uint8_t WLANOptions::SetConfiguration = 1;
    const uint8_t WLANOptions::SetNoWLAN = 2;


    /** \brief Options for WPS module. */
    struct WPSOptions {
        static const uint8_t
        GetConfiguration, ///< Get configuration
        SetConfiguration, ///< Set configuration
        Start, ///< Start
        Cancel; ///< Cancel
    };
    const uint8_t WPSOptions::GetConfiguration = 0;
    const uint8_t WPSOptions::SetConfiguration = 1;
    const uint8_t WPSOptions::Start = 2;
    const uint8_t WPSOptions::Cancel = 3;


    ///< \brief Return codes for auth_cgi access point. */
    struct AuthReturnCode {
        static const int8_t
        Success, ///< Command was executed successfully
        DontMatch, ///< One or more parameters were incorrect
        Failure; ///< Command failed to execute
    };
    const int8_t AuthReturnCode::Success = 0;
    const int8_t AuthReturnCode::DontMatch = 1;
    const int8_t AuthReturnCode::Failure = 2;

	///< \brief Return codes for web_cgi access point. */
    struct WebReturnCode {
        static const int8_t
        Success, ///< Command was executed successfully
        KickedOut, ///< Given token validity has been cancelled
        TokenError; ///< Given token doesn't match stored one
    };
    const int8_t WebReturnCode::Success = 0;
    const int8_t WebReturnCode::KickedOut = -2;
    const int8_t WebReturnCode::TokenError = -3;
    
	///< \brief Return codes for 'send message' function. */
    struct MessageReturnCode {
        static const int8_t
        SendSuccessSaveSuccess, ///< Message sent and saved successfully
        SendSuccessSaveFailure, ///< Message sent successfully but not saved
        SendFailureSaveSuccess, ///< Message save successfully but not sent
        SendFailureSaveFailure, ///< Message not sent and not saved
        Sending; ///< Currently sending
    };
    const int8_t MessageReturnCode::SendSuccessSaveSuccess = 0;
    const int8_t MessageReturnCode::SendSuccessSaveFailure = 1;
    const int8_t MessageReturnCode::SendFailureSaveSuccess = 2;
    const int8_t MessageReturnCode::SendFailureSaveFailure = 3;
    const int8_t MessageReturnCode::Sending = 4;

	///< \brief Mailbox codes. */
    enum class MailboxCode : uint8_t {
        Inbox = 0, ///< Inbox
        Outbox = 1 ///< Outbox
    };
}
