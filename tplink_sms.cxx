/** \file tplink_sms.cxx
 *	This is a short example code that uses the TP-Link M7350 modem interface
 *	to send a SMS using the command line. This was tested to work with modem
 *	version 5 and firmware version 1.0.10.
 *	Author: Vincent Paeder
 *	License: GPL v3
 */

#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <openssl/md5.h>

namespace tplink_sms {

	/** \enum AUTHENTICATOR_MODULE_ENUM
	 *	\brief Command codes for TP-Link M7350 authenticator module.
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
	 *	\brief Command codes for TP-Link M7350 message module.
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
	 *	\brief Return codes for TP-Link M7350 'send message' function.
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

	/** \fn std::string get_md5_hash(std::string & str)
	 *	\brief Generates a hex digest of the MD5 hash of the given string.
	 *	\param str : string to compute the MD5 hash from.
	 *	\returns a string containing the hex digest of the MD5 hash of the input string.
	 */
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

	/** \fn rapidjson::Document post_data(std::string & url, std::string & data)
	 *	\brief Requests given URL with given data using POST method.
	 *	\param url : URL to request
	 *	\param data : data string to join to the request
	 *	\returns a rapidjson::Document object containing the server response parsed as JSON.
	 */
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

	/** \fn bool send_sms(std::string modem_address, std::string passwd, std::string phone_number, std::string message)
	 *	\brief Sends a SMS through the TP-Link M7350 interface.
	 *	\param modem_address : modem IP address or DNS
	 *	\param passwd : modem admin password
	 *	\param phone_number : recipient number
	 *	\param message : text message to send
	 *	\returns true if successful, false otherwise.
	 */
	bool send_sms(std::string modem_address, std::string passwd, std::string phone_number, std::string message) {
		/* Working principle of TP-Link M7350v5 web interface (firmware v.1.0.10)
				Contact server on cgi-bin/auth_cgi to obtain a password salt:
					Parameters to pass: {"module":"authenticator","action":0}
					Returned data: {"authedIP":"0.0.0.0","nonce":"password salt",result:1}
				Authenticate:
					Parameters to pass: {"module":"authenticator","action":1,"digest":"md5 hex digest of password:salt"}
					Returned data: {"token":"session token","authedIP":"IP address","factoryDefault":"1",result:0}
				Send message:
					Parameters to pass: {"token":"session token","module":"message","action":3,"sendMessage":{"to":"phone number","textContent":"message content","sendTime":"YYYY,MM,dd,HH,mm,ss"}}
					Returned data: {"result":0}
				Check that message has been sent:
					Parameters to pass: {"token":"session token","module":"message","action":7}
					Returned data: {"cause":0,"result":0}
		*/
		namespace rj = rapidjson;
		// URLs of modem interface
		std::string auth_url = "http://" + modem_address + "/cgi-bin/auth_cgi";
		std::string web_url = "http://" + modem_address + "/cgi-bin/web_cgi";
		
		std::string req_data; // for POST request data
		rj::Document d; // for server replies

		/* get password salt */
		{
			// build JSON request object
			rj::StringBuffer s;
			rj::Writer<rj::StringBuffer> writer(s);
			rj::Document req_json;
			req_json.SetObject();
			req_json.AddMember("module", "authenticator", req_json.GetAllocator());
			req_json.AddMember("action", AUTHENTICATOR_LOAD, req_json.GetAllocator());
			// serialize
			req_json.Accept(writer);
			req_data = s.GetString();
		}
		// send request
		d = post_data(auth_url,req_data);
		// check that server returned a valid password salt
		if (!d.HasMember("nonce")) return false;
		if (!strcmp(d["nonce"].GetString(),"")) return false;
		
		/* log in */
		{
			// create salted password MD5 digest
			std::string spwd = passwd+":"+d["nonce"].GetString();
			std::string auth_digest = get_md5_hash(spwd);
			// build JSON request object
			rj::StringBuffer s;
			rj::Writer<rj::StringBuffer> writer(s);
			rj::Document req_json;
			req_json.SetObject();
			req_json.AddMember("module", "authenticator", req_json.GetAllocator());
			req_json.AddMember("action", AUTHENTICATOR_LOGIN, req_json.GetAllocator());
			req_json.AddMember("digest", "", req_json.GetAllocator());
			req_json["digest"].SetString(auth_digest.c_str(), auth_digest.size());
			// serialize
			req_json.Accept(writer);
			req_data = s.GetString();
		}
		// send request
		d = post_data(auth_url, req_data);
		// check that server returned a valid auth token
		if (!d.HasMember("token")) return false;
		if (!strcmp(d["token"].GetString(),"")) return false;
		std::string token = d["token"].GetString();

		/* send message */
		{
			// create time stamp
			time_t now = std::time( 0 );
			tm * t_now = std::localtime( &now );
			char timestamp[20];
			std::sprintf( timestamp, "%04d,%02d,%02d,%02d,%02d,%02d", 1900+t_now->tm_year, t_now->tm_mon, t_now->tm_mday, t_now->tm_hour, t_now->tm_min, t_now->tm_sec );
			// build JSON request object
			rj::StringBuffer s;
			rj::Writer<rj::StringBuffer> writer(s);
			rj::Document req_json;
			req_json.SetObject();
			req_json.AddMember("module","message",req_json.GetAllocator());
			req_json.AddMember("action", MESSAGE_SEND_MSG,req_json.GetAllocator());
			req_json.AddMember("token","",req_json.GetAllocator());
			req_json["token"].SetString(token.c_str(), token.size());
			// create message sub-object
			rj::Value msg(rj::kObjectType);
			msg.AddMember("to","",req_json.GetAllocator());
			msg.AddMember("textContent","",req_json.GetAllocator());
			msg.AddMember("sendTime","",req_json.GetAllocator());
			msg["to"].SetString(phone_number.c_str(), phone_number.size());
			msg["textContent"].SetString(message.c_str(), message.size());
			msg["sendTime"].SetString(timestamp,19);
			req_json.AddMember("sendMessage",msg,req_json.GetAllocator());
			// serialize
			req_json.Accept(writer);
			req_data = s.GetString();

			d = post_data(web_url, req_data);
		}
		
		/* wait until message has been sent */
		{
			// build JSON request object
			rj::StringBuffer s;
			rj::Writer<rj::StringBuffer> writer(s);
			rj::Document req_json;
			req_json.SetObject();
			req_json.AddMember("module","message",req_json.GetAllocator());
			req_json.AddMember("action", MESSAGE_GET_SEND_STATUS,req_json.GetAllocator());
			req_json.AddMember("token","",req_json.GetAllocator());
			req_json["token"].SetString(token.c_str(), token.size());
			// serialize
			req_json.Accept(writer);
			req_data = s.GetString();
		}
		// send request repeatedly
		do { d = post_data(web_url, req_data); } while ( d["result"].GetInt() == SMS_SENDING );
		return d["result"].GetInt() == SMS_SEND_SUCCESS_SAVE_SUCCESS;
	}

}

/* main function - returns 0 if execution went fine, 1 otherwise */
int main( int argc, char** argv ) {
	using namespace tplink_sms;

	// flags indicating whether arguments have been set
	bool address_set = false, pw_set = false, number_set = false, message_set = false;
	// argument values
	std::string address, passwd, phone_number, message;

	// parse command line for arguments
	int opt;
	while ( ( opt = getopt ( argc, argv, "ha:p:n:m:" ) ) != -1 ) {
		switch ( opt ) {
			case 'h':
				std::cout << "Usage:" << std::endl;
				std::cout << argv[0] << " -a modem_address -p password -n phone_number -m message" << std::endl;
				std::cout << argv[0] << " -h" << std::endl;
				break;

			case 'a':
				address = optarg;
				address_set = true;
				break;

			case 'p':
				passwd = optarg;
				pw_set = true;
				break;

			case 'n':
				phone_number = optarg;
				number_set = true;
				break;

			case 'm':
				message = optarg;
				message_set = true;
				break;
		}
	}
	if (!address_set || !pw_set || !number_set || !message_set) {
		std::cout << "One of the required arguments has not been set." << std::endl;
		std::cout << "Type " << argv[0] << " -h for help" << std::endl;
		return 1;
	}

	return send_sms(address, passwd, phone_number, message) ? 0 : 1;
}
