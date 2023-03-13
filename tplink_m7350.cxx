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
#include <iostream>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/param_build.h>
#include <openssl/err.h>

namespace tplink {
	
	/** \brief Compute the MD5 hash of a string.
	 *	\param str: string to compute MD5 hash for.
	 *	\returns MD5 hash in hexadecimal format.
	 */
	static std::string compute_md5_hash(const std::string & str) {
		unsigned char digest[16];
		// digest functions from OpenSSL
		auto ctx = UniquePointer<EVP_MD_CTX, EVP_MD_CTX_free>(EVP_MD_CTX_new());
		EVP_MD_CTX_init(ctx.get());
		EVP_DigestInit_ex(ctx.get(), EVP_md5(), nullptr);
		EVP_DigestUpdate(ctx.get(), str.data(), str.size());
		EVP_DigestFinal_ex(ctx.get(), digest, nullptr);
		
		std::ostringstream res;
		for (int i=0; i<16; i++)
			res << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(digest[i]);
		
		return res.str();
	}

	/** \brief Encode a string in base64.
	 *	\param data: string to encode.
	 *	\returns base64-encoded string.
	 */
	static std::string b64_encode(const std::string & data) {
		auto b64 = UniquePointer<BIO, BIO_free>(BIO_new(BIO_f_base64()));
		BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
		auto sink = UniquePointer<BIO, BIO_free>(BIO_new(BIO_s_mem()));
		BIO_push(b64.get(), sink.get());
		BIO_write(b64.get(), data.data(), data.size());
		BIO_flush(b64.get());
		char* encoded;
		auto len = BIO_get_mem_data(sink.get(), &encoded);
		return std::string(encoded, len);
	}

	/** \brief Decode a base64-encoded string.
	 *	\param data: base64-encoded string.
	 *	\returns decoded string.
	 */
	static std::string b64_decode(const std::string & data) {
		auto b64 = UniquePointer<BIO, BIO_free>(BIO_new(BIO_f_base64()));
		BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
		auto source = UniquePointer<BIO, BIO_free>(BIO_new(BIO_s_mem()));
		auto sink = UniquePointer<BIO, BIO_free>(BIO_new(BIO_s_mem()));
		BIO_puts(source.get(), data.c_str());
		char buffer[data.size()];
		BIO_push(b64.get(), source.get());
		auto len = BIO_read(b64.get(), buffer, data.size());
		BIO_flush(b64.get());
		BIO_write(sink.get(), buffer, len);
		char* decoded;
		len = BIO_get_mem_data(sink.get(), &decoded);
		return std::string(decoded, len);
	}

	/* CURL error buffer */
	static char error_buffer[CURL_ERROR_SIZE];

	/* CURL writer callback (collect server response) */
	static int writer(char *data, size_t size, size_t nmemb, std::string *writer_data) {
		if (writer_data == nullptr)
			return 0;

		writer_data->append(data, size*nmemb);
		return size*nmemb;
	}
	
	/** \brief Converts a RapidJSON object to string.
	 *	\param d : RapidJSON object to stringify
	 *	\returns a string containing the input JSON object.
	 */
	static std::string stringify(const rj::Document & d) {
		rj::StringBuffer s;
		rj::Writer<rj::StringBuffer> writer(s);
		d.Accept(writer);
		return s.GetString();
	}
	
	
	void TPLink_M7350::initialize() {
		this->conn = UniquePointer<CURL, curl_easy_cleanup>(curl_easy_init());
		// set error buffer for CURL
		assert(curl_easy_setopt(this->conn.get(), CURLOPT_ERRORBUFFER, &error_buffer) == CURLE_OK);
		// set data writer function
		assert(curl_easy_setopt(this->conn.get(), CURLOPT_WRITEFUNCTION, writer)  == CURLE_OK);
	}

	
	TPLink_M7350::TPLink_M7350() {
		this->initialize();
	}


	TPLink_M7350::TPLink_M7350(const std::string & modem_address, const std::string & password) {
		this->initialize();
		this->set_address(modem_address);
		this->set_password(password);
	}


	std::string TPLink_M7350::post_request(const std::string & url, const std::string & data) const {
		assert(this->conn != nullptr);
		// set data buffer for CURL
		std::string buffer;
		assert(curl_easy_setopt(this->conn.get(), CURLOPT_WRITEDATA, &buffer) == CURLE_OK);
		// set URL
		assert(curl_easy_setopt(this->conn.get(), CURLOPT_URL, url.c_str()) == CURLE_OK);
		// set POST data
		curl_easy_setopt(this->conn.get(), CURLOPT_POSTFIELDSIZE, data.size());
		curl_easy_setopt(this->conn.get(), CURLOPT_POSTFIELDS, data.c_str());
		// access page
		assert(curl_easy_perform(this->conn.get()) == CURLE_OK);
		return buffer;
	}

	
	rj::Document TPLink_M7350::parse_response(const std::string & data, const bool is_encrypted) const {
		rj::Document d;
		if (is_encrypted)
			d.Parse(this->aes_decrypt(data).c_str());
		else
			d.Parse(data.c_str());
		
		return d;
	}


	rj::Document TPLink_M7350::get_data_array(rj::Document & request, const std::string & field) const {
		// create response container
		rj::Document response;
		response.SetObject();
		// requested data is an array -> create empty array and add to container
		rj::Value a(rj::kArrayType);
		rj::Document::AllocatorType & allocator = response.GetAllocator();
		response.AddMember(rj::Value(rj::kStringType).SetString(field.c_str(), field.size(), allocator), a, allocator);
		
		// request data once to obtain the number of items in the array
		auto req_json = this->encrypt(stringify(request), false);
		auto d = this->parse_response(this->post_request(this->web_url, req_json), true);
		if (!d.HasMember("totalNumber")) return rj::Document();
		auto cnt = d["totalNumber"].GetInt();
		int page_n = 2; // page 1 has just been loaded
		for (;;) {
			if (d.HasMember(field.c_str())) {
				for (rj::Value::ValueIterator itr = d[field.c_str()].Begin(); itr != d[field.c_str()].End(); ++itr) {
					rj::Value obj(rj::kObjectType);
					obj.CopyFrom(*itr, allocator);
					response[field.c_str()].PushBack(obj, allocator);
				}
			}
			cnt -= 8;
			if (cnt<=0) break;
			request["pageNumber"] = page_n++;
			req_json = this->encrypt(stringify(request), false);
			d = this->parse_response(post_request(this->web_url, req_json), true);
		}
		
		return response;
	}
	

	void TPLink_M7350::set_address(const std::string & modem_address) {
		// URLs of modem interface
		this->modem_address = "http://" + modem_address;
		this->auth_url = this->modem_address + "/cgi-bin/auth_cgi";
		this->web_url = this->modem_address + "/cgi-bin/web_cgi";
	}

	
	void TPLink_M7350::set_password(const std::string & password) {
		this->password = password;
		#if NEW_FIRMWARE==1
		this->hash = compute_md5_hash("admin"+this->password);
		#endif
	}

	
#if NEW_FIRMWARE==1
	void TPLink_M7350::generate_aes_keys() {
		assert(RAND_bytes(this->aes_key, 16));
		assert(RAND_bytes(this->aes_iv, 16));
	}


	std::string TPLink_M7350::rsa_encrypt(const std::string & data) const {
		auto params_build = UniquePointer<OSSL_PARAM_BLD, OSSL_PARAM_BLD_free>(OSSL_PARAM_BLD_new());
		assert(params_build);
		auto bn_mod = BN_new();
		auto bn_exp = BN_new();
		BN_hex2bn(&bn_mod, this->rsa_mod.c_str());
		assert(bn_mod);
		BN_hex2bn(&bn_exp, this->rsa_exp.c_str());
		assert(bn_exp);

		assert(OSSL_PARAM_BLD_push_BN(params_build.get(), "n", bn_mod)==1);
		assert(OSSL_PARAM_BLD_push_BN(params_build.get(), "e", bn_exp)==1);
		assert(OSSL_PARAM_BLD_push_BN(params_build.get(), "d", nullptr)==1);
		auto params = UniquePointer<OSSL_PARAM, OSSL_PARAM_free>(OSSL_PARAM_BLD_to_param(params_build.get()));
		assert(params);

		// create key object
		auto ctx = UniquePointer<EVP_PKEY_CTX, EVP_PKEY_CTX_free>(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
		assert(ctx);
		assert(EVP_PKEY_fromdata_init(ctx.get())==1);
		auto pkey = EVP_PKEY_new();
		assert(EVP_PKEY_fromdata(ctx.get(), &pkey, EVP_PKEY_PUBLIC_KEY, params.get())==1);
		assert(pkey);
		auto key_size = EVP_PKEY_get_bits(pkey)/8;

		// create encryption context
		ctx = UniquePointer<EVP_PKEY_CTX, EVP_PKEY_CTX_free>(EVP_PKEY_CTX_new(pkey, nullptr));
		assert(EVP_PKEY_encrypt_init(ctx.get())>0);
		assert(EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_NO_PADDING)>0);
		// RSA with no padding can only encode strings that are the same size as the modulus;
		// need to split data into chunks and pad last chunk
		unsigned int last_chunk_size = data.size() % key_size;
		unsigned int n_chunks = data.size()/key_size + (last_chunk_size>0);
		auto encrypted_size = n_chunks*key_size;
		std::vector<unsigned char> ciphertext;
		ciphertext.resize(encrypted_size);
		size_t offset = 0;
		while (n_chunks--) {
			size_t ciphertext_len = 0;
			if (n_chunks == 0) {
				std::vector<unsigned char> plaintext;
				plaintext.resize(key_size);
				std::copy(data.begin()+offset, data.end(), plaintext.begin());
				EVP_PKEY_encrypt(ctx.get(), &ciphertext[offset], &ciphertext_len, plaintext.data(), key_size);
			} else {
				EVP_PKEY_encrypt(ctx.get(), &ciphertext[offset], &ciphertext_len, reinterpret_cast<const unsigned char*>(&data[offset]), key_size);
				offset += key_size;
			}
		}

		EVP_PKEY_free(pkey);
		BN_free(bn_mod);
		BN_free(bn_exp);

		auto result = std::string(reinterpret_cast<char*>(ciphertext.data()), encrypted_size);
		return result;
	}


	std::string TPLink_M7350::rsa_sign(const int increment, const bool include_aes_key) const {
		// generate signature
		auto seq = this->seq + increment;
		std::string s;
		if (include_aes_key) {
			auto escaped_key = curl_easy_escape(this->conn.get(), reinterpret_cast<const char*>(this->aes_key), 16);
			auto escaped_iv = curl_easy_escape(this->conn.get(), reinterpret_cast<const char*>(this->aes_iv), 16);
			s = "key=" + std::string(escaped_key) + "&iv=" + escaped_iv
					+ "&h=" + this->hash + "&s=" + std::to_string(seq);
		} else {
			s = "&h=" + this->hash + "&s=" + std::to_string(seq);
		}
		// encrypt signature
		return this->rsa_encrypt(s);
	}


	std::string TPLink_M7350::aes_encrypt(const std::string & data) const {
		auto ctx = UniquePointer<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>(EVP_CIPHER_CTX_new());
		assert(ctx);

		auto plaintext_len = data.size();

		unsigned char ciphertext[plaintext_len];
		int ciphertext_len = 0, len = 0;
		assert(EVP_EncryptInit_ex(ctx.get(), EVP_aes_128_cbc(), nullptr, this->aes_key, this->aes_iv)==1);
		assert(EVP_EncryptUpdate(ctx.get(), ciphertext, &ciphertext_len, reinterpret_cast<const unsigned char*>(data.c_str()), plaintext_len)==1);
		assert(EVP_EncryptFinal_ex(ctx.get(), ciphertext + ciphertext_len, &len)==1);
		ciphertext_len += len;

		auto result = b64_encode(std::string(reinterpret_cast<char*>(ciphertext), ciphertext_len));
		return result;
	}
#endif // NEW_FIRMWARE


	std::string TPLink_M7350::aes_decrypt(const std::string & data) const {
	#if NEW_FIRMWARE==1
		auto ctx = UniquePointer<EVP_CIPHER_CTX, EVP_CIPHER_CTX_free>(EVP_CIPHER_CTX_new());
		assert(ctx);

		auto ciphertext = b64_decode(data);
		unsigned char plaintext[data.size()];
		int plaintext_len = 0, len = 0;
		assert(EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_cbc(), nullptr, this->aes_key, this->aes_iv)==1);
		assert(EVP_DecryptUpdate(ctx.get(), plaintext, &plaintext_len, reinterpret_cast<const unsigned char*>(ciphertext.c_str()), ciphertext.size())==1);
		assert(EVP_DecryptFinal_ex(ctx.get(), plaintext + plaintext_len, &len)==1);
		plaintext_len += len;

		auto result = std::string(reinterpret_cast<char*>(plaintext), plaintext_len);
		return result;
	#else
		return data;
	#endif // NEW_FIRMWARE
	}


	std::string TPLink_M7350::encrypt(const std::string & data, const bool include_aes_key) const {
	#if NEW_FIRMWARE==1
		auto encrypted = this->aes_encrypt(data);
		auto signature = this->rsa_sign(encrypted.size(), include_aes_key);
		return "{\"data\":\""+encrypted+"\",\"sign\":\""+signature+"\"}";
	#else
		return data;
	#endif // NEW_FIRMWARE
	}


	rj::Document TPLink_M7350::build_request_object(const std::string & module, const int action) const {
		// this function creates a basic JSON object with commonly required fields
		// {"module":"module name", "action":action_code, "token":"authentication token"}
		rj::Document req;
		req.SetObject();
		req.AddMember("module","",req.GetAllocator());
		req["module"].SetString(module.c_str(), module.size());
		req.AddMember("action", action, req.GetAllocator());
		if (this->token.size()>0) {
			req.AddMember("token","",req.GetAllocator());
			req["token"].SetString(this->token.c_str(), this->token.size());
		}
		return req;
	}
	

	rj::Document TPLink_M7350::do_request(const std::string & module, const int action) const {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return nullptr;
		}
		
		auto req = this->build_request_object(module, action);
		auto req_json = this->encrypt(stringify(req), false);

		return this->parse_response(this->post_request(this->web_url, req_json), true);
	}

  
	bool TPLink_M7350::send_data(const std::string & module, const int action, const rj::Document & data) const {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return false;
		}
		
		// create a basic request object
		auto req = this->build_request_object(module, action);
		// add provided data to the object
		for (auto itr = data.MemberBegin(); itr != data.MemberEnd(); ++itr) {
			auto name = rj::Value(itr->name, req.GetAllocator());
			auto value = rj::Value(itr->value, req.GetAllocator());
			req.AddMember(name.Move(), value.Move(), req.GetAllocator());
		}
		
		auto req_json = this->encrypt(stringify(req), false);
		
		auto d = this->parse_response(this->post_request(this->web_url, req_json), true);
		
		return d["result"] == WebReturnCode::Success;
	}


	/* ALG module */
	rj::Document TPLink_M7350::get_alg_settings() const {
		return this->do_request(Modules::ALG, ALGOptions::GetConfiguration);
	}
  
	bool TPLink_M7350::set_alg_settings(const rj::Document & data) const {
		return this->send_data(Modules::ALG, ALGOptions::SetConfiguration, data);
	}
	
	/* APBridge module */
	rj::Document TPLink_M7350::get_ap_bridge_settings() const {
		return this->do_request(Modules::APBridge, APBridgeOptions::GetConfiguration);
	}

	bool TPLink_M7350::set_ap_bridge_settings(const rj::Document & data) const {
		return this->send_data(Modules::APBridge, APBridgeOptions::SetConfiguration, data);
	}
	
	bool TPLink_M7350::connect_ap(const rj::Document & data) const {
		return this->send_data(Modules::APBridge, APBridgeOptions::ConnectAP, data);
	}

	rj::Document TPLink_M7350::scan_ap() const {
		return this->do_request(Modules::APBridge, APBridgeOptions::ScanAP);
	}

	rj::Document TPLink_M7350::check_ap_connection_status() const {
		return this->do_request(Modules::APBridge, APBridgeOptions::CheckConnectionStatus);
	}
	
	/* Authenticator module */
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
		
		LOG_I("Attempting login into ", this->auth_url, " ...");
	
		/* get password salt */
		{
			// build JSON request object
			auto req = this->build_request_object(Modules::Authenticator, AuthenticatorOptions::Load);
			// serialize
			req_json = stringify(req);
		}
		// send request
		d = this->parse_response(post_request(this->auth_url, req_json), false);
		// check that response is valid
		if (!d.IsObject()) {
			LOG_E("Modem didn't return a valid reply.");
			return false;
		}
		if (!d.HasMember("nonce")) {
			LOG_E("Modem reply contains no 'nonce' field.");
			return false;
		}
	
		if (!strcmp(d["nonce"].GetString(),"")) {
			LOG_E("Modem reply contains an empty 'nonce' field.");
			return false;
		}
		
		LOG_I("Got a valid reply from modem. Trying to authenticate...");
		/* log in */
		{
		#if NEW_FIRMWARE==1
			// generate new AES keys
			this->generate_aes_keys();
			// store RSA key and salt
			this->rsa_mod = d["rsaMod"].GetString();
			this->rsa_exp = d["rsaPubKey"].GetString();
			auto seq_str = d["seqNum"].GetString();
			if (strlen(seq_str)>0)
				this->seq = std::stoi(seq_str);
		#endif // NEW_FIRMWARE
			// create salted password MD5 digest
			auto spwd = this->password+":"+d["nonce"].GetString();
			auto auth_digest =	compute_md5_hash(spwd);
			// build JSON request object
			auto req = this->build_request_object(Modules::Authenticator, AuthenticatorOptions::Login);
			req.AddMember("digest", "", req.GetAllocator());
			req["digest"].SetString(auth_digest.c_str(), auth_digest.size());
			// serialize
			req_json = this->encrypt(stringify(req), true);
		}
		// send request
		d = this->parse_response(post_request(this->auth_url, req_json), true);
		// check that server returned a valid auth token
		if (!d.IsObject()) {
			LOG_E("Modem didn't return a valid reply.");
			return false;
		}
		if (!d.HasMember("token")) {
			LOG_E("Modem didn't return an authentication token.");
			return false;
		}
		if (!strcmp(d["token"].GetString(),"")) {
			LOG_E("Modem returned an empty authentication token.");
			return false;
		}
		this->token = d["token"].GetString();
		
		this->logged_in = true;
		LOG_I("Login successful.");
		return true;
	}
	
	bool TPLink_M7350::logout() {
		if (!this->logged_in) {
			LOG_I("Not logged in.");
			return true;
		}
		LOG_I("Attempting to log out...");
		auto req = this->build_request_object(Modules::Authenticator, AuthenticatorOptions::Logout);
		auto req_json = this->encrypt(stringify(req), false);
		auto d = this->parse_response(this->post_request(this->auth_url, req_json), true);
		this->logged_in = !(d["result"].GetInt() == AuthReturnCode::Success);
		if (this->logged_in)
			LOG_E("Couldn't log out!");
	
		return !(this->logged_in);
	}

	rj::Document TPLink_M7350::get_login_attempt_count() const {
		return this->do_request(Modules::Authenticator, AuthenticatorOptions::GetAttempts);
	}

	bool TPLink_M7350::change_password(const std::string & old_password, const std::string & new_password) {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return false;
		}
		auto req = this->build_request_object(Modules::Authenticator, AuthenticatorOptions::Update);
		req.AddMember("password", "", req.GetAllocator());
		req.AddMember("newPassword", "", req.GetAllocator());
		req["password"].SetString(old_password.c_str(), old_password.size());
		req["newPassword"].SetString(new_password.c_str(), new_password.size());
		
		auto req_json = this->encrypt(stringify(req), false);
		
		auto d = this->parse_response(this->post_request(this->auth_url, req_json), true);
		
		if (!d.HasMember("result")) return false;
		auto result = d["result"] == AuthReturnCode::Success;
		if (result)
			this->set_password(new_password);

		return result;
	}

	/* Connected devices module */
	rj::Document TPLink_M7350::get_connected_devices() const {
		return this->do_request(Modules::ConnectedDevices, ConnectedDevicesOptions::GetConfiguration);
	}

	/* DMZ module */
	rj::Document TPLink_M7350::get_dmz_settings() const {
		return this->do_request(Modules::DMZ, DMZOptions::GetConfiguration);
	}

	bool TPLink_M7350::set_dmz_settings(const rj::Document & data) const {
		return this->send_data(Modules::DMZ, DMZOptions::SetConfiguration, data);
	}
	
	/* Flow stat module */
	rj::Document TPLink_M7350::get_flow_stat_settings() const {
		return this->do_request(Modules::FlowStat, FlowStatOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_flow_stat_settings(const rj::Document & data) const {
		return this->send_data(Modules::FlowStat, FlowStatOptions::SetConfiguration, data);
	}
  
	/* LAN module */
	rj::Document TPLink_M7350::get_lan_settings() const {
		return this->do_request(Modules::LAN, LANOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_lan_settings(const rj::Document & data) const {
		return this->send_data(Modules::LAN, LANOptions::SetConfiguration, data);
	}

	/* Log module */
	rj::Document TPLink_M7350::get_log() const {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return nullptr;
		}
		
		auto req = this->build_request_object(Modules::Log, LogOptions::GetLog);
		req.AddMember("amountPerPage", 8, req.GetAllocator());
		req.AddMember("pageNumber", 1, req.GetAllocator());
		req.AddMember("type", 0, req.GetAllocator());
		req.AddMember("level", 0, req.GetAllocator());
		
		return this->get_data_array(req, "logList");
	}

	bool TPLink_M7350::clear_log() const {
		auto d = this->do_request(Modules::Log, LogOptions::ClearLog);
		if (!d.HasMember("result")) return false;
		return d["result"] == WebReturnCode::Success;
	}


	/* MAC filter module */
	rj::Document TPLink_M7350::get_mac_filters() const {
		return this->do_request(Modules::MACFilters, MACFiltersOptions::GetBlackList);
	}
	
	bool TPLink_M7350::set_mac_filters(const rj::Document & data) const {
		return this->send_data(Modules::MACFilters, MACFiltersOptions::SetBlackList, data);
	}
	
  
	/* Message module */
	rj::Document TPLink_M7350::read_sms(const MailboxCode box) const {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return nullptr;
		}
		
		auto req = this->build_request_object(Modules::Message, MessageOptions::ReadMessage);
		req.AddMember("amountPerPage", 8, req.GetAllocator());
		req.AddMember("pageNumber", 1, req.GetAllocator());
		req.AddMember("box", static_cast<uint8_t>(box), req.GetAllocator());
		
		return this->get_data_array(req, "messageList");
	}

	bool TPLink_M7350::send_sms(const std::string & phone_number, const std::string & message) const {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return false;
		}
		
		std::string req_json; // for POST request data
		rj::Document d; // for server replies
		
		/* send message */
		{
			// create time stamp
			auto now = std::time(0);
			auto t_now = std::localtime(&now);
			char timestamp[20];
			std::sprintf(timestamp, "%04d,%02d,%02d,%02d,%02d,%02d", 1900+t_now->tm_year, t_now->tm_mon, t_now->tm_mday, t_now->tm_hour, t_now->tm_min, t_now->tm_sec);
			// build JSON request object
			auto req = this->build_request_object(Modules::Message, MessageOptions::SendMessage);
			// create message sub-object
			rj::Value msg(rj::kObjectType);
			msg.AddMember("to", "", req.GetAllocator());
			msg.AddMember("textContent", "", req.GetAllocator());
			msg.AddMember("sendTime", "", req.GetAllocator());
			msg["to"].SetString(phone_number.c_str(), phone_number.size());
			msg["textContent"].SetString(message.c_str(), message.size());
			msg["sendTime"].SetString(timestamp, 19);
			req.AddMember("sendMessage", msg, req.GetAllocator());
			// serialize
			req_json = this->encrypt(stringify(req), false);

			d = this->parse_response(this->post_request(this->web_url, req_json), true);
		}
		
		/* wait until message has been sent */
		{
			// build JSON request object
			auto req = this->build_request_object(Modules::Message, MessageOptions::GetSendStatus);
			// serialize
			req_json = this->encrypt(stringify(req), false);
		}
		// send request repeatedly
		do {
			d = this->parse_response(this->post_request(this->web_url, req_json), true);
		} while (d["result"].GetInt() == MessageReturnCode::Sending);
	
		return d["result"].GetInt() == MessageReturnCode::SendSuccessSaveSuccess;
	}
	
	bool TPLink_M7350::delete_sms(const MailboxCode box, const std::vector<int> & indices) const {
		if (!this->logged_in) {
			LOG_E("Not logged in! Try logging in first.");
			return false;
		}
		
		auto req = this->build_request_object(Modules::Message, MessageOptions::DeleteMessage);
		req.AddMember("box", static_cast<uint8_t>(box), req.GetAllocator());
		// copy message indices into JSON object
		// NOTE: message ids are obtained with the #TPLink_M7350::read_sms function
		rj::Value a(rj::kArrayType);
		for (auto i: indices)
			a.PushBack(i, req.GetAllocator());
	
		req.AddMember("deleteMessages", a, req.GetAllocator());
		auto req_json = this->encrypt(stringify(req), false);
		auto d = this->parse_response(this->post_request(this->web_url, req_json), true);
		
		return d["result"].GetInt() == MessageReturnCode::SendSuccessSaveSuccess;
	}

	/* Port triggering module */
	rj::Document TPLink_M7350::get_port_triggering_settings() const {
		return this->do_request(Modules::PortTriggering, PortTriggeringOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_port_triggering_settings(const rj::Document & data) const {
		return this->send_data(Modules::PortTriggering, PortTriggeringOptions::SetConfiguration, data);
	}

	bool TPLink_M7350::delete_port_triggering_entry(const rj::Document & data) const {
		return this->send_data(Modules::PortTriggering, PortTriggeringOptions::DeleteEntry, data);
	}

	/* Power save module */
	rj::Document TPLink_M7350::get_power_save_settings() const {
		return this->do_request(Modules::PowerSave, PowerSavingOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_power_save_settings(const rj::Document & data) const {
		return this->send_data(Modules::PowerSave, PowerSavingOptions::SetConfiguration, data);
	}
  
	/* Reboot module */
	bool TPLink_M7350::reboot() {
		auto d = this->do_request(Modules::Reboot, RebootOptions::Reboot);
		if (!d.HasMember("result")) return false;
		auto result = d["result"] == WebReturnCode::Success;
		if (result)
			this->logged_in = false;
		
		return result;
	}
  
	bool TPLink_M7350::shutdown() {
		auto d = this->do_request(Modules::Reboot, RebootOptions::Shutdown);
		if (!d.HasMember("result")) return false;
		auto result = d["result"] == WebReturnCode::Success;
		if (result)
			this->logged_in = false;
		
		return result;
	}

	/* SIM lock module */
	rj::Document TPLink_M7350::get_sim_lock_settings() const {
		return this->do_request(Modules::SimLock, SIMLockOptions::GetConfiguration);
	}
	
	/* Status module */
	rj::Document TPLink_M7350::get_status() const {
		return this->do_request(Modules::Status, 0);
	}
	
	/* Storage share settings */
	rj::Document TPLink_M7350::get_storage_share_settings() const {
		return this->do_request(Modules::StorageShare, StorageShareOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_storage_share_settings(const rj::Document & data) const {
		return this->send_data(Modules::StorageShare, StorageShareOptions::SetConfiguration, data);
	}

	/* Time module */
	rj::Document TPLink_M7350::get_time_settings() const {
		return this->do_request(Modules::Time, TimeOptions::GetConfiguration);
	}

	bool TPLink_M7350::set_time_settings(const rj::Document & data) const {
		return this->send_data(Modules::Time, TimeOptions::SetConfiguration, data);
	}
	
	/* Update module */
	rj::Document TPLink_M7350::get_firmware_update_settings() const {
		return this->do_request(Modules::Update, FirmwareUpdateOptions::GetConfiguration);
	}
	
	/* UPnP module */
	rj::Document TPLink_M7350::get_upnp_settings() const {
		return this->do_request(Modules::UPnP, UPnPOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_upnp_settings(const rj::Document & data) const {
		return this->send_data(Modules::UPnP, UPnPOptions::SetConfiguration, data);
	}
	
	/* Virtual server module */
	rj::Document TPLink_M7350::get_virtual_server_settings() const {
		return this->do_request(Modules::VirtualServer, VirtualServerOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_virtual_server_settings(const rj::Document & data) const {
		return this->send_data(Modules::VirtualServer, VirtualServerOptions::SetConfiguration, data);
	}
	
	/* Voice module */
	rj::Document TPLink_M7350::get_voice_settings() const {
		return this->do_request(Modules::Voice, VoiceOptions::GetConfiguration);
	}
	
	/* WAN module */
	rj::Document TPLink_M7350::get_wan_settings() const {
		return this->do_request(Modules::WAN, WANOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_wan_settings(const rj::Document & data) const {
		return this->send_data(Modules::WAN, WANOptions::SetConfiguration, data);
	}
	
	/* WebServer module */
	rj::Document TPLink_M7350::get_web_server_info() const {
		if (this->logged_in) {
			return this->do_request(Modules::WebServer, WebServerOptions::GetFeatureList);
		} else {
		#if NEW_FIRMWARE==1
			auto req = this->build_request_object(Modules::WebServer, WebServerOptions::GetInfoWithoutAuthentication);
			auto req_json = stringify(req);
			return this->parse_response(this->post_request(this->web_url, req_json), false);
		#else
			LOG_E("Not logged in! Try logging in first.");
			return nullptr;
		#endif
		}
	}
	
	/* WLAN module */
	rj::Document TPLink_M7350::get_wlan_settings() const {
		return this->do_request(Modules::WLAN, WLANOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_wlan_settings(const rj::Document & data) const {
		return this->send_data(Modules::WLAN, WLANOptions::SetConfiguration, data);
	}

	/* WPS module */
	rj::Document TPLink_M7350::get_wps_settings() const {
		return this->do_request(Modules::WPS, WPSOptions::GetConfiguration);
	}
	
	bool TPLink_M7350::set_wps_settings(const rj::Document & data) const {
		return this->send_data(Modules::WPS, WPSOptions::SetConfiguration, data);
	}

	/* Restore conf module */
	bool TPLink_M7350::restore_defaults() const {
		auto d = this->do_request(Modules::RestoreConf, 0);
		if (!d.HasMember("result")) return false;
		return d["result"] == WebReturnCode::Success;
	}

}
