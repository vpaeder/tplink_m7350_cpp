/** \file tplink_m7350.h
 *  This is a minimal C++ interface to communicate with the TP-Link M7350 modem
 *  web gateway interface. This was tested to work with modem version 5.
 *  Author: Vincent Paeder
 *  License: GPL v3
 */
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <curl/curl.h>

#include "tp_m7350_enums.h"

namespace tplink {

  #ifndef NDEBUG
  template <typename... Args> void tp_logger(const char * level, const char * file, const int line, const Args... args) {
    std::cout << level << ":[" << file << "|" << line << "] ";
    ([&]{std::cout << args;}(), ...);
    std::cout << std::endl;
  }
  #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
  #define LOG_V(...) tp_logger("V", __FILENAME__, __LINE__, __VA_ARGS__)
  #define LOG_D(...) tp_logger("D", __FILENAME__, __LINE__, __VA_ARGS__)
  #define LOG_E(...) tp_logger("E", __FILENAME__, __LINE__, __VA_ARGS__)
  #define LOG_I(...) tp_logger("I", __FILENAME__, __LINE__, __VA_ARGS__)
  #else
  #define LOG_V(...)
  #define LOG_D(...)
  #define LOG_E(...)
  #define LOG_I(...)
  #endif
	
	/** \brief Wrap a C deleter function for use with smart pointers.
	 * 
	 *  This relies on the behaviour of the call operator that assumes
	 *  that the left-hand side argument should be a function, and therefore
	 *  gets cast to function pointer. At the same time, the integral_constant
	 *  instance gives the wrapped value type as its own type.
	 * 
	 *  \tparam DeleterFunction: C deleter function.
	 */
	template <auto DeleterFunction>
	using CustomDeleter = std::integral_constant<std::decay_t<decltype(DeleterFunction)>, DeleterFunction>;

	/** \brief A pointer that wraps an object and defines a deleter from a deleter function.
	 *  \tparam WrappedType: object type.
	 *  \tparam DeleterFunction: deleter function.
	 */
	template <typename WrappedType, auto DeleterFunction>
	using UniquePointer = std::unique_ptr<WrappedType, CustomDeleter<DeleterFunction> >;

	namespace rj = rapidjson;
	
	/** \brief Class handling communication with a TP-Link M7350 v5 web interface. */
	class TPLink_M7350 {
	private:
    /** \brief CURL object used to handle HTTP connections */
    UniquePointer<CURL, curl_easy_cleanup> conn; 

  	/* CURL error buffer */
    std::string error_buffer;

    /** \brief modem base URL */
    std::string modem_address = "http://192.168.0.1";

    /** \brief URL of authenticator interface */
    std::string auth_url = "http://192.168.0.1/cgi-bin/auth_cgi";
    
    /** \brief URL of web interface */
    std::string web_url = "http://192.168.0.1/cgi-bin/web_cgi";
    
    /** \brief Modem administrator password */
    std::string password{};

    /** \brief True if the object is authenticated with the modem */
    bool logged_in = false;
    
    /** \brief Authentication token */
    std::string token{};

    /** \brief Initialize instance */
    void initialize();
    
  #if NEW_FIRMWARE==1
    /** \brief Hashed password, used to generate message signatures */
    std::string hash{};

    /** \brief AES key */
    unsigned char aes_key[16];
    /** \brief AES initialization vector */
    unsigned char aes_iv[16];
    /** \brief RSA key module */
    std::string rsa_mod{};
    /** \brief RSA key exponent */
    std::string rsa_exp{};
    /** \brief Salt for RSA sign */
    unsigned int seq;

    /** \brief Initialize instance. */
    void generate_aes_keys();

    /** \brief Encrypt given data using RSA.
     *  \param data: data to encrypt.
     *  \returns encrypted data.
    */
    std::string rsa_encrypt(const std::string & data) const;

    /** \brief Generate a message signature using RSA.
     *  \param increment: a number added to the signature salt.
     *  \param include_aes_key: if true, include AES key/iv in signature.
     *  \returns signature string.
    */
    std::string rsa_sign(const int increment, const bool include_aes_key) const;

    /** \brief Encrypt given data with AES.
     *  \param data: data to encrypt.
     *  \returns encrypted data.
     */
    std::string aes_encrypt(const std::string & data) const;
  #endif // NEW_FIRMWARE

    /** \brief Decrypt given data with AES.
     *  This does something only if NEW_FIRMWARE is set to Yes. Otherwise, this
     *  is just a pass-through method.
     *  \param data: data to decrypt.
     *  \returns decrypted data.
     */
    std::string aes_decrypt(const std::string & data) const;

    /** \brief Encrypt given data.
     *  This does something only if NEW_FIRMWARE is set to Yes. Otherwise, this
     *  is just a pass-through method.
     *  \param data: data to encrypt.
     *  \param include_aes_key: if true, include AES key/iv in signature.
     *  \returns encrypted data.
    */
    std::string encrypt(const std::string & data, const bool include_aes_key) const;

    /** \brief Build object to produce a JSON request
     *  \param module: name of module to query
     *  \param action: code of action to perform
     *  \returns a RapidJSON object containing necessary items.
     */
    rj::Document build_request_object(const std::string & module, const int action) const;
    
    /** \brief Send a HTTP POST request to given URL with given POST data and return reply.
     *  \param url: URL to send request to.
     *  \param data: data to join with the POST request.
     *  \returns server reply as string.
     */
    std::string post_request(const std::string & url, const std::string & data) const;

    /** \brief Parse a server response, assuming it is in JSON format.
     *  \param data: server response as string.
     *  \param is_encrypted: if true, assume response must be decrypted first.
     *  \returns a RapidJSON document object containing parsed server reply.
     */
    rj::Document parse_response(const std::string & data, const bool is_encrypted) const;

    /** \brief Send a request to the modem web gateway interface and return reply.
     *  \param module: name of module to query
     *  \param action: code of action to perform
     *  \returns a RapidJSON object containing modem reply, or an empty object if request failed.
     */
    rj::Document do_request(const std::string & module, const int action) const;
    
    /** \brief Send data to the modem web gateway interface.
     *  \param module: name of module to send data to
     *  \param action: code of action to perform with data
     *  \param data: JSON object containing data to be sent
     *  \returns true if operation was successful, false otherwise.
     */
    bool send_data(const std::string & module, const int action, const rj::Document & data) const;
    
    /** \brief Retrieve data array from modem web gateway interface.
     *  \param request: JSON object containing request parameters to reach required array.
     *  \param field: name of data array.
     *  \returns a JSON object containing the requested data array, or an empty object if request failed.
     */
    rj::Document get_data_array(rj::Document & request, const std::string & field) const;

	public:
    /** \brief Default constructor. */
    TPLink_M7350();
    
    /** \brief Constructor with parameters.
     *  \param modem_address: IP or DNS address of modem
     *  \param password: modem admin password
     */
    TPLink_M7350(const std::string & modem_address, const std::string & password);

    /** \fn void set_address(std::string & modem_address)
     *  \brief Set modem IP address or domain name.
     *  \param modem_address: modem IP address or domain name
     */
    void set_address(const std::string & modem_address);
    
    /** \fn void set_password(std::string & password)
     *  \brief Set modem admin password.
     *  \param password: modem admin password.
     */
    void set_password(const std::string & password);
    
    /** \brief Retrieve settings for alg module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_alg_settings() const;
    
    /** \brief Set configuration for alg module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_alg_settings(const rj::Document & data) const;

    /** \brief Retrieve settings for AP bridge module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_ap_bridge_settings() const;

    /** \brief Set configuration for AP bridge module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_ap_bridge_settings(const rj::Document & data) const;

    /** \brief Connect with access point with specified parameters.
     *  Access point details must be as follows:
     *  {"apSSID": string, "apPassword": string, "apSecurity": see APSecurity enum, "ap8021xType": string}
     *  \param data: JSON object containing access point details.
     *  \returns true if action was successful, false otherwise.
     */
    bool connect_ap(const rj::Document & data) const;

    /** \brief Scan for access points.
     *  \returns JSON object with modem reply.
     */
    rj::Document scan_ap() const;

    /** \brief Check access point connection status.
     *  \returns JSON object with modem reply.
     */
    rj::Document check_ap_connection_status() const;

    /** \brief Attempt to log in into modem web interface.
     *  \returns true if operation was successful, false otherwise.
     */
    bool login();
    
    /** \fn bool logout()
     *  \brief Attempt to log out from modem web interface.
     *  \returns true if operation was successful, false otherwise.
     */
    bool logout();
    
    /** \brief Get number of login attempts.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_login_attempt_count() const;

    /** \brief Attempt to change admin password.
     *  \param old_password: password currently set.
     *  \param new_password: new password.
     *  \returns true if action was successful, false otherwise.
     */
    bool change_password(const std::string & old_password, const std::string & new_password);
    
    /** \brief Retrieve information for connected devices.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_connected_devices() const;
    
    /** \brief Retrieve settings for DMZ module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_dmz_settings() const;
    
    /** \brief Set configuration for DMZ module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_dmz_settings(const rj::Document & data) const;
    
    /** \brief Retrieve settings for flowstat module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_flow_stat_settings() const;
    
    /** \brief Set configuration for flowstat module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_flow_stat_settings(const rj::Document & data) const;
    
    /** \brief Retrieve settings for lan module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_lan_settings() const;
    
    /** \brief Set configuration for lan module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_lan_settings(const rj::Document & data) const;
    
    /** \brief Retrieve modem logs.
     *  \returns JSON object with log entries.
     */
    rj::Document get_log() const;
    
    /** \brief Clear logs.
     *  \returns true if action was successful, false otherwise.
     */
    bool clear_log() const;

    /** \brief Retrieve settings for macFilters module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_mac_filters() const;

    /** \brief Set configuration for macFilters module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successSet configurationful, false otherwise.
     */
    bool set_mac_filters(const rj::Document & data) const;
    
    /** \brief Reads messages from given mailbox.
     *  \param box: mailbox number (see MAILBOX_ENUM)
     *  \returns a JSON object containing retrieved messages, or an empty object if request failed.
     */
    rj::Document read_sms(const MailboxCode box) const;
    
    /** \brief Sends a SMS through the TP-Link M7350 interface.
     *  \param phone_number: recipient number
     *  \param message: text message to send
     *  \returns true if successful, false otherwise.
     */
    bool send_sms(const std::string & phone_number, const std::string & message) const;
    
    /** \brief Deletes messages stored in the TP-Link M7350 memory.
     *  \param box: mailbox number (see MAILBOX_ENUM)
     *  \param indices: a list of message indices to delete.
     *  \returns true if successful, false otherwise.
     */
    bool delete_sms(const MailboxCode box, const std::vector<int> & indices) const;

    /** \brief Retrieve settings for portTrigger module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_port_triggering_settings() const;
    
    /** \brief Set configuration for portTrigger module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_port_triggering_settings(const rj::Document & data) const;

    /** \brief Delete entry from portTrigger module.
     *  \param data: JSON object containing entry details.
     *  \returns true if action was successful, false otherwise.
     */
    bool delete_port_triggering_entry(const rj::Document & data) const;

    /** \brief Retrieve settings for power_save module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_power_save_settings() const;
      
    /** \brief Set configuration for power_save module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_power_save_settings(const rj::Document & data) const;
    
    /** \brief Attempt to reboot modem.
     *  \returns true if action was successful, false otherwise.
     */
    bool reboot();
    
    /** \brief Attempt to shutdown modem.
     *  \returns true if action was successful, false otherwise.
     */
    bool shutdown();
    
    /** \brief Retrieve settings for simLock module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_sim_lock_settings() const;

    /** \brief Retrieve information from status module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_status() const;
    
    /** \brief Retrieve settings for storageShare module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_storage_share_settings() const;
    
    /** \brief Set configuration for storageShare module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_storage_share_settings(const rj::Document & data) const;

    /** \brief Retrieve settings for time module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_time_settings() const;
    
    /** \brief Set configuration for time module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_time_settings(const rj::Document & data) const;
    
    /** \brief Retrieve settings for update module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_firmware_update_settings() const;

    /** \brief Retrieve settings for upnp module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_upnp_settings() const;
    
    /** \brief Set configuration for upnp module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_upnp_settings(const rj::Document & data) const;
    
    /** \brief Retrieve settings for virtualServer module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_virtual_server_settings() const;
    
    /** \brief Set configuration for virtualServer module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_virtual_server_settings(const rj::Document & data) const;
    
    /** \brief Retrieve settings for voice module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_voice_settings() const;
    
    /** \brief Retrieve settings for wan module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_wan_settings() const;
    
    /** \brief Set configuration for wan module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_wan_settings(const rj::Document & data) const;
      
    /** \brief Retrieve settings for webServer module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_web_server_info() const;
    
    /** \brief Retrieve settings for WLAN module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_wlan_settings() const;
    
    /** \brief Set configuration for WLAN module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_wlan_settings(const rj::Document & data) const;

    /** \brief Retrieve settings for wps module.
     *  \returns JSON object with modem reply.
     */
    rj::Document get_wps_settings() const;
    
    /** \brief Set configuration for wps module.
     *  \param data: JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_wps_settings(const rj::Document & data) const;
      
    /** \brief Restore factory defaults.
     *  \returns true if action was successful, false otherwise.
     */
    bool restore_defaults() const;
    
	};
}
