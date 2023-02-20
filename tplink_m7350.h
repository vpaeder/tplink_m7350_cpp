/** \file tplink_m7350.h
 *  This is a minimal C++ interface to communicate with the TP-Link M7350 modem's
 *  web gateway interface. This was tested to work with modem
 *  version 5 and firmware version 1.0.10.
 *  Author: Vincent Paeder
 *  License: GPL v3
 */
#ifndef TPLINK_M7350_H
#define TPLINK_M7350_H

#include <string>
#include <vector>
#include <iostream>
#include <rapidjson/document.h>

namespace tplink {

  template <typename... Args> void tp_logger(const char * level, const char * file, const int line, const Args... args) {
      std::cout << level << ":[" << file << "|" << line << "] ";
      ([&]{std::cout << args;}(), ...);
      std::cout << std::endl;
  }
  #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
  #ifndef NDEBUG
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
  
  /** \enum TPLINK_CONFIG_ENUM
   *  \brief Shared command codes for TP-Link M7350 web_cgi modules.
   */
  enum TPLINK_CONFIG_ENUM {
    /** Get configuration */
    TPLINK_GET_CONFIG = 0,
    /** Set configuration */
    TPLINK_SET_CONFIG = 1
  };
  
  /** \enum AUTH_ERROR_CODE_ENUM
   *  \brief Return codes for TP-Link M7350 auth_cgi.
   */
  enum AUTH_ERROR_CODE_ENUM {
    /** Command was executed successfully */
    AUTH_SUCCESS = 0,
    /** One or more parameters were incorrect */
    AUTH_NOT_MATCH = 1,
    /** Command failed to execute */
    AUTH_FAILURE = 2
  };
  
  /** \enum WEB_ERROR_CODE_ENUM
   *  \brief Return codes for TP-Link M7350 web_cgi.
   */
  enum WEB_ERROR_CODE_ENUM {
    /** Command was executed successfully */
    WEB_SUCCESS = 0,
    /** Given token validity has been cancelled */
    WEB_KICKED_OUT = -2,
    /** Given token doesn't match stored one */
    WEB_TOKEN_ERROR = -3
  };

  /** \enum AUTHENTICATOR_MODULE_ENUM
   *  \brief Command codes for TP-Link M7350 authenticator module.
   */
  enum AUTHENTICATOR_MODULE_ENUM {
    /** Load */
    AUTHENTICATOR_LOAD = 0,
    /** Perform login */
    AUTHENTICATOR_LOGIN = 1,
    /** Get number of login attempts */
    AUTHENTICATOR_GET_ATTEMPT = 2,
    /** Perform logout */
    AUTHENTICATOR_LOGOUT = 3,
    /** Perform page update */
    AUTHENTICATOR_UPDATE = 4
  };

  /** \enum MESSAGE_MODULE_ENUM
   *  \brief Command codes for TP-Link M7350 message module.
   */
  enum MESSAGE_MODULE_ENUM {
    /** Get message module configuration */
    MESSAGE_GET_CONFIG = 0,
    /** Set message module configuration */
    MESSAGE_SET_CONFIG = 1,
    /** Read stored message list */
    MESSAGE_READ_MSG = 2,
    /** Send message */
    MESSAGE_SEND_MSG = 3,
    /** Save message */
    MESSAGE_SAVE_MSG = 4,
    /** Delete message(s) */
    MESSAGE_DEL_MSG = 5,
    /** Mark message(s) as read */
    MESSAGE_MARK_READ = 6,
    /** Request current status */
    MESSAGE_GET_SEND_STATUS = 7
  };

  /** \enum SEND_MESSAGE_ENUM
   *  \brief Return codes for TP-Link M7350 'send message' function.
   */
  enum SEND_MESSAGE_ENUM {
    /** Message sent and saved successfully */
     SMS_SEND_SUCCESS_SAVE_SUCCESS = 0,
    /** Message sent successfully but not saved */
     SMS_SEND_SUCCESS_SAVE_FAIL = 1,
    /** Message save successfully but not sent */
     SMS_SEND_FAIL_SAVE_SUCCESS = 2,
    /** Message not sent and not saved */
     SMS_SEND_FAIL_SAVE_FAIL = 3,
    /** Currently sending */
     SMS_SENDING = 4
  };
  
  /** \enum MAILBOX_ENUM
   *  \brief Mailbox codes for TP-Link M7350.
   */
  enum MAILBOX_ENUM {
  /** Inbox */
   MAILBOX_INBOX = 0,
  /** Outbox */
   MAILBOX_OUTBOX = 1
  };
  
  namespace rj = rapidjson;
  
  /** \fn std::string get_md5_hash(std::string & str)
   *  \brief Compute MD5 hash of input string.
   *  \param str : string to compute hash for.
   *  \returns MD5 hash of input string.
   */
  std::string get_md5_hash(const std::string & str);
    
  /** \fn rj::Document post_data(std::string & url, std::string & data)
   *  \brief Sends a HTTP POST request to given URL with given POST data and returns reply parsed as JSON.
   *  \param url : URL to send request to.
   *  \param data : data to join with the POST request.
   *  \returns a RapidJSON document object containing parsed server reply.
   */
  rj::Document post_data(const std::string & url, const std::string & data);

  /** \class TPLink_M7350
   *  \brief Class handling communication with a TP-Link M7350 v5 web interface.
   */
  class TPLink_M7350 {
  private:
    /** \property std::string auth_url
     *  \brief URL of authenticator interface
     */
    std::string auth_url = "http://192.168.0.1/cgi-bin/auth_cgi";
    
    /** \property std::string web_url
     *  \brief URL of web interface
     */
    std::string web_url = "http://192.168.0.1/cgi-bin/web_cgi";
    
    /** \property std::string password
     *  \brief Modem administrator password
     */
    std::string password;
  
    /** \property bool logged_in
     *  \brief True if the object is authenticated with the modem
     */
    bool logged_in = false;
    
    /** \property std::string token
     *  \brief Authentication token
     */
    std::string token;
  
    /** \fn rj::Document build_request_object(std::string module, int action)
     *  \brief Builds object to produce a JSON request
     *  \param module : name of module to query
     *  \param action : code of action to perform
     *  \returns a RapidJSON object containing necessary items.
     */
    rj::Document build_request_object(const std::string & module, const int action);
    
    /** \fn rj::Document send_request(std::string module, int action)
     *  \brief Sends a request to the modem's web gateway interface and returns reply.
     *  \param module : name of module to query
     *  \param action : code of action to perform
     *  \returns a RapidJSON object containing modem's reply, or an empty object if request failed.
     */
    rj::Document send_request(const std::string & module, const int action);
    
    /** \fn bool send_data(std::string module, int action, rj::Document & data)
     *  \brief Sends data to the modem's web gateway interface.
     *  \param module : name of module to send data to
     *  \param action : code of action to perform with data
     *  \param data : JSON object containing data to be sent
     *  \returns true if operation was successful, false otherwise.
     */
    bool send_data(const std::string & module, const int action, const rj::Document & data);
    
    /** \fn rj::Document get_data_array(rj::Document & request, std::string field)
     *  \brief Retrieves data array from modem's web gateway interface.
     *  \param request : JSON object containing request parameters to reach required array.
     *  \param field : name of data array.
     *  \returns a JSON object containing the requested data array, or an empty object if request failed.
     */
    rj::Document get_data_array(rj::Document & request, const std::string & field);
    
  public:
    TPLink_M7350() = default;
    
    /** \fn TPLink_M7350(std::string & modem_address, std::string & password)
     *  \brief Instance constructor.
     *  \param modem_address : IP or DNS address of modem
     *  \param password : modem admin password
     */
    TPLink_M7350(const std::string & modem_address, const std::string & password) {
      this->set_address(modem_address);
      this->set_password(password);
    }
    
    /** \fn void set_address(std::string & modem_address)
     *  \brief Sets modem IP address or domain name.
     *  \param modem_address : modem IP address or domain name
     */
    void set_address(const std::string & modem_address);
    
    /** \fn void set_password(std::string & password)
     *  \brief Sets modem admin password.
     *  \param password : modem admin password.
     */
    void set_password(const std::string & password);
    
    /** \fn bool login()
     *  \brief Attempts to log in to the modem's web interface.
     *  \returns true if operation was successful, false otherwise.
     */
    bool login();
    
    /** \fn bool logout()
     *  \brief Attempts to log out from the modem's web interface.
     *  \returns true if operation was successful, false otherwise.
     */
    bool logout();
    
    /** \fn rj::Document get_web_server()
     *  \brief Retrieves configuration settings for webServer module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_web_server();
    
    /** \fn rj::Document get_status()
     *  \brief Retrieves information from status module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_status();
    
    /** \fn rj::Document get_wan()
     *  \brief Retrieves configuration settings for wan module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_wan();
    
    /** \fn rj::Document get_sim_lock()
     *  \brief Retrieves configuration settings for simLock module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_sim_lock();
    
    /** \fn rj::Document get_wps()
     *  \brief Retrieves configuration settings for wps module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_wps();
    
    /** \fn rj::Document get_power_save()
     *  \brief Retrieves configuration settings for power_save module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_power_save();
    
    /** \fn rj::Document get_flow_stat()
     *  \brief Retrieves configuration settings for flowstat module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_flow_stat();
    
    /** \fn rj::Document get_connected_devices()
     *  \brief Retrieves information for connected devices.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_connected_devices();
    
    /** \fn rj::Document get_mac_filters()
     *  \brief Retrieves configuration settings for macFilters module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_mac_filters();
    
    /** \fn rj::Document get_lan()
     *  \brief Retrieves configuration settings for lan module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_lan();
    
    /** \fn rj::Document get_update()
     *  \brief Retrieves configuration settings for update module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_update();
    
    /** \fn rj::Document get_storage_share()
     *  \brief Retrieves configuration settings for storageShare module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_storage_share();
    
    /** \fn rj::Document get_time()
     *  \brief Retrieves configuration settings for time module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_time();
    
    /** \fn rj::Document get_log()
     *  \brief Retrieves modem logs.
     *  \returns JSON object with log entries.
     */
    rj::Document get_log();
    
    /** \fn rj::Document get_voice()
     *  \brief Retrieves configuration settings for voice module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_voice();
    
    /** \fn rj::Document get_upnp()
     *  \brief Retrieves configuration settings for upnp module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_upnp();
    
    /** \fn rj::Document get_dmz()
     *  \brief Retrieves configuration settings for dmz module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_dmz();
    
    /** \fn rj::Document get_alg()
     *  \brief Retrieves configuration settings for alg module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_alg();
    
    /** \fn rj::Document get_virtual_server()
     *  \brief Retrieves configuration settings for virtualServer module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_virtual_server();
    
    /** \fn rj::Document get_port_triggering()
     *  \brief Retrieves configuration settings for portTrigger module.
     *  \returns JSON object with modem's reply.
     */
    rj::Document get_port_triggering();
    
    /** \fn bool set_web_server(rj::Document & data)
     *  \brief Sets configuration settings for webServer module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_web_server(rj::Document & data);
    
    /** \fn bool set_wan(rj::Document & data)
     *  \brief Sets configuration settings for wan module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_wan(rj::Document & data);
    
    /** \fn bool set_sim_lock(rj::Document & data)
     *  \brief Sets configuration settings for simLock module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_sim_lock(rj::Document & data);
    
    /** \fn bool set_wps(rj::Document & data)
     *  \brief Sets configuration settings for wps module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_wps(rj::Document & data);
    
    /** \fn bool set_power_save(rj::Document & data)
     *  \brief Sets configuration settings for power_save module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_power_save(rj::Document & data);
    
    /** \fn bool set_flow_stat(rj::Document & data)
     *  \brief Sets configuration settings for flowstat module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_flow_stat(rj::Document & data);
    
    /** \fn bool set_mac_filters(rj::Document & data)
     *  \brief Sets configuration settings for macFilters module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_mac_filters(rj::Document & data);
    
    /** \fn bool set_lan(rj::Document & data)
     *  \brief Sets configuration settings for lan module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_lan(rj::Document & data);
    
    /** \fn bool set_storage_share(rj::Document & data)
     *  \brief Sets configuration settings for storageShare module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_storage_share(rj::Document & data);
    
    /** \fn bool set_time(rj::Document & data)
     *  \brief Sets configuration settings for time module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_time(rj::Document & data);
    
    /** \fn bool set_voice(rj::Document & data)
     *  \brief Sets configuration settings for voice module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_voice(rj::Document & data);
    
    /** \fn bool set_upnp(rj::Document & data)
     *  \brief Sets configuration settings for upnp module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_upnp(rj::Document & data);
    
    /** \fn bool set_dmz(rj::Document & data)
     *  \brief Sets configuration settings for dmz module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_dmz(rj::Document & data);
    
    /** \fn bool set_alg(rj::Document & data)
     *  \brief Sets configuration settings for alg module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_alg(rj::Document & data);
    
    /** \fn bool set_virtual_server(rj::Document & data)
     *  \brief Sets configuration settings for virtualServer module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_virtual_server(rj::Document & data);
    
    /** \fn bool set_port_triggering(rj::Document & data)
     *  \brief Sets configuration settings for portTrigger module.
     *  \param data : JSON object containing configuration settings.
     *  \returns true if action was successful, false otherwise.
     */
    bool set_port_triggering(rj::Document & data);
    
    /** \fn bool reboot()
     *  \brief Attempts to reboot the modem.
     *  \returns true if action was successful, false otherwise.
     */
    bool reboot();
    
    /** \fn bool shutdown()
     *  \brief Attempts to shutdown the modem.
     *  \returns true if action was successful, false otherwise.
     */
    bool shutdown();
    
    /** \fn bool restore_defaults()
     *  \brief Attempts to restore factory defaults.
     *  \returns true if action was successful, false otherwise.
     */
    bool restore_defaults();
    
    /** \fn bool clear_log()
     *  \brief Attempts to clear modem logs.
     *  \returns true if action was successful, false otherwise.
     */
    bool clear_log();
    
    /** \fn bool change_password(std::string & old_password, std::string & new_password)
     *  \brief Attempts to change admin password.
     *  \param old_password : password currently set.
     *  \param new_password : new password.
     *  \returns true if action was successful, false otherwise.
     */
    bool change_password(const std::string & old_password, const std::string & new_password);
    
    /** \fn rj::Document read_sms(int box)
     *  \brief Reads messages from given mailbox.
     *  \param box : mailbox number (see MAILBOX_ENUM)
     *  \returns a JSON object containing retrieved messages, or an empty object if request failed.
     */
    rj::Document read_sms(const int box);
    
    /** \fn bool send_sms(std::string phone_number, std::string message)
     *  \brief Sends a SMS through the TP-Link M7350 interface.
     *  \param phone_number : recipient number
     *  \param message : text message to send
     *  \returns true if successful, false otherwise.
     */
    bool send_sms(const std::string & phone_number, const std::string & message);
    
    /** \fn bool delete_sms(int box, std::vector<int> & indices)
     *  \brief Deletes messages stored in the TP-Link M7350 memory.
     *  \param box : mailbox number (see MAILBOX_ENUM)
     *  \param indices : a list of message indices to delete.
     *  \returns true if successful, false otherwise.
     */
    bool delete_sms(const int box, const std::vector<int> & indices);
    
  };
}

#endif // TPLINK_M7350_H
