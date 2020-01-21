/** \file tplink_m7350.cxx
 *	This is a minimal C++ interface to communicate with the TP-Link M7350 modem's
 *  web gateway interface. This was tested to work with modem
 *	version 5 and firmware version 1.0.10.
 *	Author: Vincent Paeder
 *	License: GPL v3
 */
#include "tplink_m7350.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <curl/curl.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <openssl/md5.h>

namespace tplink {
	std::string get_md5_hash(std::string & str) {
		unsigned char digest[16];
		// MD5 functions from OpenSSL
		MD5_CTX ctx;
		MD5_Init(&ctx);
		MD5_Update(&ctx, str.c_str(), str.size());
		MD5_Final(digest, &ctx);
		
		std::ostringstream res;
		for (int i=0; i<16; i++)
			res << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)digest[i];
		
		return res.str();
	}

	/* CURL error buffer */
	static char errorBuffer[CURL_ERROR_SIZE];

	/* CURL writer callback (collects server response) */
	static int writer(char *data, size_t size, size_t nmemb, std::string *writerData) {
		if ( writerData == NULL )
			return 0;

		writerData->append( data, size*nmemb );

		return size * nmemb;
	}
	
	/** \fn static std::string stringify(rj::Document & d)
	 *	\brief Converts a RapidJSON object to string.
	 *	\param d : RapidJSON object to stringify
	 *	\returns a string containing the input JSON object.
	 */
	static std::string stringify(rj::Document & d) {
		rj::StringBuffer s;
		rj::Writer<rj::StringBuffer> writer(s);
		d.Accept(writer);
		return s.GetString();
	}

	rapidjson::Document post_data(std::string & url, std::string & data) {
		CURL *conn = NULL;
		CURLcode code;
		rapidjson::Document d;
		conn = curl_easy_init();
		if ( conn == NULL ) return d;

		// set error buffer for CURL
		//std::string errorBuffer;
		code = curl_easy_setopt( conn, CURLOPT_ERRORBUFFER, & errorBuffer );
		if ( code != CURLE_OK ) return d;

		// set data buffer for CURL
		std::string buffer;
		code = curl_easy_setopt( conn, CURLOPT_WRITEDATA, & buffer );
		if ( code != CURLE_OK ) return d;

		// set data writer function
		code = curl_easy_setopt( conn, CURLOPT_WRITEFUNCTION, writer );
		if ( code != CURLE_OK ) return d;

		// set URL
		code = curl_easy_setopt( conn, CURLOPT_URL, url.c_str() );
		if ( code != CURLE_OK ) return d;

		// set POST data
		curl_easy_setopt( conn, CURLOPT_POSTFIELDSIZE, data.size() );
		curl_easy_setopt( conn, CURLOPT_POSTFIELDS, data.c_str() );

		// access page
		code = curl_easy_perform( conn );
		if ( code != CURLE_OK ) return d;
		d.Parse(buffer.c_str());
		
		curl_easy_cleanup( conn );
		return d;

	}
	
	rj::Document TPLink_M7350::get_data_array(rj::Document & request, std::string field) {
		// create response container
		rj::Document response;
		response.SetObject();
		// requested data is an array -> create empty array and add to container
		rj::Value a(rj::kArrayType);
		rj::Document::AllocatorType & allocator = response.GetAllocator();
		response.AddMember(rj::Value(rj::kStringType).SetString(field.c_str(), field.size(), allocator), a, allocator);
		
		// request data once to obtain the number of items in the array
		std::string req_json = stringify(request);
		rj::Document d = post_data(this->web_url, req_json);
		if (!d.HasMember("totalNumber")) return rj::Document();
		int cnt = d["totalNumber"].GetInt();
		int page_n = 2; // page 1 has just been loaded
		while (1) {
			if (d.HasMember(field.c_str())) {
				for (rapidjson::Value::ValueIterator itr = d[field.c_str()].Begin(); itr != d[field.c_str()].End(); ++itr) {
					rj::Value obj(rj::kObjectType);
					obj.CopyFrom(*itr, allocator);
					response[field.c_str()].PushBack(obj, allocator);
				}
			}
			cnt -= 8;
			if (cnt<=0) break;
			request["pageNumber"] = page_n++;
			req_json = stringify(request);
			d = post_data(this->web_url, req_json);
		}
		
		return response;
	}
	
	void TPLink_M7350::set_address(std::string & modem_address) {
		// URLs of modem interface
		this->auth_url = "http://" + modem_address + "/cgi-bin/auth_cgi";
		this->web_url = "http://" + modem_address + "/cgi-bin/web_cgi";
	}
	
	void TPLink_M7350::set_password(std::string & password) {
		this->password = password;
	}
	
	rj::Document TPLink_M7350::build_request_object(std::string module, int action) {
		// this function creates a basic JSON object with commonly required fields
		// {"module":"module name", "action":action_code, "token":"authentication token"}
		rj::Document req;
		req.SetObject();
		req.AddMember("module","",req.GetAllocator());
		req["module"].SetString(module.c_str(), module.size());
		req.AddMember("action", action,req.GetAllocator());
		req.AddMember("token","",req.GetAllocator());
		req["token"].SetString(this->token.c_str(), this->token.size());
		return req;
	}
	
	bool TPLink_M7350::login() {
		/* Working principle of log in procedure
		Contact server on cgi-bin/auth_cgi to obtain a password salt:
			Parameters to pass: {"module":"authenticator","action":0}
			Returned data: {"authedIP":"0.0.0.0","nonce":"password salt",result:1}
		Authenticate:
			Parameters to pass: {"module":"authenticator","action":1,"digest":"md5 hex digest of password:salt"}
			Returned data: {"token":"session token","authedIP":"IP address","factoryDefault":"1",result:0}
		*/
		std::string req_json; // for POST request data
		rj::Document d; // for server replies
		
		/* get password salt */
		{
			// build JSON request object
			rj::Document req = this->build_request_object("authenticator", AUTHENTICATOR_LOAD);
			// serialize
			req_json = stringify(req);
		}
		// send request
		d = post_data(this->auth_url, req_json);
		// check that server returned a valid reply
		if (!d.HasMember("nonce")) return false;
		if (!strcmp(d["nonce"].GetString(),"")) return false;
		
		/* log in */
		{
			// create salted password MD5 digest
			std::string spwd = this->password+":"+d["nonce"].GetString();
			std::string auth_digest =	get_md5_hash(spwd);
			// build JSON request object
			rj::Document req = this->build_request_object("authenticator", AUTHENTICATOR_LOGIN);
			req.AddMember("digest", "", req.GetAllocator());
			req["digest"].SetString(auth_digest.c_str(), auth_digest.size());
			// serialize
			req_json = stringify(req);
		}
		// send request
		d = post_data(this->auth_url, req_json);
		// check that server returned a valid auth token
		if (!d.HasMember("token")) return false;
		if (!strcmp(d["token"].GetString(),"")) return false;
		this->token = d["token"].GetString();
		
		this->logged_in = true;
		return true;
	}
	
	bool TPLink_M7350::logout() {
		rj::Document req = this->build_request_object("authenticator", AUTHENTICATOR_LOGOUT);
		std::string req_json = stringify(req);
		rj::Document d = post_data(this->auth_url, req_json);
		
		this->logged_in = !(d["result"].GetInt() == AUTH_SUCCESS);
		return !(this->logged_in);
	}
	
	rj::Document TPLink_M7350::send_request(std::string module, int action) {
		if (!this->logged_in)
			if (!this->login()) return nullptr;
		
		rj::Document req = this->build_request_object(module, action);
		std::string req_json = stringify(req);
		
		return post_data(this->web_url, req_json);
	}
		
	rj::Document TPLink_M7350::get_web_server() {
		return this->send_request("webServer", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_status() {
		return this->send_request("status", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_wan() {
		return this->send_request("wan", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_sim_lock() {
		return this->send_request("simLock", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_wps() {
		return this->send_request("wps", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_power_save() {
		return this->send_request("power_save", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_flow_stat() {
		return this->send_request("flowstat", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_connected_devices() {
		return this->send_request("connectedDevices", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_mac_filters() {
		return this->send_request("macFilters", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_lan() {
		return this->send_request("lan", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_update() {
		return this->send_request("update", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_storage_share() {
		return this->send_request("storageShare", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_time() {
		return this->send_request("time", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_log() {
		if (!this->logged_in)
			if (!this->login()) return nullptr;
		
		rj::Document req = this->build_request_object("log", 0);
		req.AddMember("amountPerPage", 8, req.GetAllocator());
		req.AddMember("pageNumber", 1, req.GetAllocator());
		req.AddMember("type", 0, req.GetAllocator());
		req.AddMember("level", 0, req.GetAllocator());
		
		return this->get_data_array(req, "logList");
	}
	
	rj::Document TPLink_M7350::get_voice() {
		return this->send_request("voice", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_upnp() {
		return this->send_request("upnp", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_dmz() {
		return this->send_request("dmz", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_alg() {
		return this->send_request("alg", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_virtual_server() {
		return this->send_request("virtualServer", TPLINK_GET_CONFIG);
	}
	
	rj::Document TPLink_M7350::get_port_triggering() {
		return this->send_request("portTrigger", TPLINK_GET_CONFIG);
	}
	
	bool TPLink_M7350::send_data(std::string module, int action, rj::Document & data) {
		if (!this->logged_in)
			if (!this->login()) return false;
		
		// create a basic request object
		rj::Document req = this->build_request_object(module, action);
		// add provided data to the object
		for (rapidjson::Value::MemberIterator itr = data.MemberBegin(); itr != data.MemberEnd(); ++itr)
			req.AddMember(itr->name, itr->value, req.GetAllocator());
		
		std::string req_json = stringify(req);
		
		rj::Document d = post_data(this->web_url, req_json);
		
		return d["result"] == WEB_SUCCESS;
		
	}
	
	bool TPLink_M7350::set_web_server(rj::Document & data) {
		return this->send_data("webServer", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_wan(rj::Document & data) {
		return this->send_data("wan", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_sim_lock(rj::Document & data) {
		return this->send_data("simLock", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_wps(rj::Document & data) {
		return this->send_data("wps", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_power_save(rj::Document & data) {
		return this->send_data("power_save", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_flow_stat(rj::Document & data) {
		return this->send_data("flowstat", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_mac_filters(rj::Document & data) {
		return this->send_data("macFilters", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_lan(rj::Document & data) {
		return this->send_data("lan", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_time(rj::Document & data) {
		return this->send_data("time", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_voice(rj::Document & data) {
		return this->send_data("voice", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_upnp(rj::Document & data) {
		return this->send_data("upnp", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_dmz(rj::Document & data) {
		return this->send_data("dmz", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_alg(rj::Document & data) {
		return this->send_data("alg", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_virtual_server(rj::Document & data) {
		return this->send_data("virtualServer", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::set_port_triggering(rj::Document & data) {
		return this->send_data("portTrigger", TPLINK_SET_CONFIG, data);
	}
	
	bool TPLink_M7350::reboot() {
		rj::Document d = this->send_request("reboot", 0);
		if (!d.HasMember("result")) return false;
		return d["result"] == WEB_SUCCESS;
	}
	
	bool TPLink_M7350::shutdown() {
		rj::Document d = this->send_request("reboot", 1);
		if (!d.HasMember("result")) return false;
		return d["result"] == WEB_SUCCESS;
	}
	
	bool TPLink_M7350::restore_defaults() {
		rj::Document d = this->send_request("restoreDefaults", 0);
		if (!d.HasMember("result")) return false;
		return d["result"] == WEB_SUCCESS;
	}
	
	bool TPLink_M7350::clear_log() {
		rj::Document d = this->send_request("log", 1);
		if (!d.HasMember("result")) return false;
		return d["result"] == WEB_SUCCESS;
	}
	
	bool TPLink_M7350::change_password(std::string & old_password, std::string & new_password) {
		rj::Document req = this->build_request_object("authenticator", AUTHENTICATOR_UPDATE);
		req.AddMember("password", "", req.GetAllocator());
		req.AddMember("newPassword", "", req.GetAllocator());
		req["password"].SetString(old_password.c_str(), old_password.size());
		req["newPassword"].SetString(new_password.c_str(), new_password.size());
		
		std::string req_json = stringify(req);
		
		rj::Document d = post_data(this->auth_url, req_json);
		
		if (!d.HasMember("result")) return false;
		return d["result"] == AUTH_SUCCESS;
	}
	
	rj::Document TPLink_M7350::read_sms(int box) {
		if (!this->logged_in)
			if (!this->login()) return rj::Document();
		
		rj::Document req = this->build_request_object("message", MESSAGE_READ_MSG);
		req.AddMember("amountPerPage", 8, req.GetAllocator());
		req.AddMember("pageNumber", 1, req.GetAllocator());
		req.AddMember("box", box, req.GetAllocator());
		
		return this->get_data_array(req, "messageList");
	}
	
	bool TPLink_M7350::send_sms(std::string phone_number, std::string message) {
		if (!this->logged_in)
			if (!this->login()) return false;
		
		std::string req_json; // for POST request data
		rj::Document d; // for server replies
		
		/* send message */
		{
			// create time stamp
			time_t now = std::time( 0 );
			tm * t_now = std::localtime( &now );
			char timestamp[20];
			std::sprintf( timestamp, "%04d,%02d,%02d,%02d,%02d,%02d", 1900+t_now->tm_year, t_now->tm_mon, t_now->tm_mday, t_now->tm_hour, t_now->tm_min, t_now->tm_sec );
			// build JSON request object
			rj::Document req = this->build_request_object("message", MESSAGE_SEND_MSG);
			// create message sub-object
			rj::Value msg(rj::kObjectType);
			msg.AddMember("to","",req.GetAllocator());
			msg.AddMember("textContent","",req.GetAllocator());
			msg.AddMember("sendTime","",req.GetAllocator());
			msg["to"].SetString(phone_number.c_str(), phone_number.size());
			msg["textContent"].SetString(message.c_str(), message.size());
			msg["sendTime"].SetString(timestamp,19);
			req.AddMember("sendMessage",msg,req.GetAllocator());
			// serialize
			req_json = stringify(req);

			d = post_data(this->web_url, req_json);
		}
		
		/* wait until message has been sent */
		{
			// build JSON request object
			rj::Document req = this->build_request_object("message", MESSAGE_GET_SEND_STATUS);
			// serialize
			req_json = stringify(req);
		}
		// send request repeatedly
		do { d = post_data(this->web_url, req_json); } while ( d["result"].GetInt() == SMS_SENDING );
		return d["result"].GetInt() == SMS_SEND_SUCCESS_SAVE_SUCCESS;
	}
	
	bool TPLink_M7350::delete_sms(int box, std::vector<int> & indices) {
		if (!this->logged_in)
			if (!this->login()) return false;
		
		rj::Document req = this->build_request_object("message", MESSAGE_DEL_MSG);
		req.AddMember("box", box, req.GetAllocator());
		// copy message indices into JSON object
		// NOTE: message ids are obtained with the #TPLink_M7350::read_sms function
		rj::Value a(rj::kArrayType);
		for (int & i: indices)
			a.PushBack(i, req.GetAllocator());
		req.AddMember("deleteMessages", a, req.GetAllocator());
		
		std::string req_json = stringify(req);
		
		rj::Document d = post_data(this->web_url, req_json);
		
		return d["result"].GetInt() == SMS_SEND_SUCCESS_SAVE_SUCCESS;
	}

}
