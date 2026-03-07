// turnstile_validator.h
// Server-side Cloudflare Turnstile token verification.
//
// HOW TURNSTILE WORKS (two independent steps):
//
//   STEP 1 - CLIENT SIDE (runs in the browser, no code needed here):
//     The Turnstile widget at challenges.cloudflare.com runs a JS challenge
//     inside an iframe on the page. When it passes, it writes a one-time
//     token to a hidden form field (cf-turnstile-response) or calls a
//     callback. The browser sends that token to YOUR server with the form.
//
//   STEP 2 - SERVER SIDE (this file):
//     YOUR server sends the token to Cloudflare's siteverify API to confirm
//     it is valid before allowing the action (login, billing, etc.).
//
// This code handles Step 2. Anthropic runs Step 2 on their own servers.
// You only need this file if you are adding Turnstile to your OWN pages.
// It does NOT affect whether your browser can pass Anthropic's widget.
//
// Bug in the original code: CURLOPT_URL was "https://challenges.cloudflare.com"
// which is the widget host, not the API. The correct URL is below.

#pragma once
#include <iostream>
#include <string>
#include <cstdlib>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class TurnstileValidator {
public:
    // Set to false in production. When true the "always pass" test secret is
    // used so every token succeeds. Never ship with is_dev_mode = true.
    bool is_dev_mode = false;

    std::string get_secret_key() const {
        if (is_dev_mode) {
            // Cloudflare official "always pass" test secret.
            // Any sitekey + this secret = always succeeds at siteverify.
            return "1x0000000000000000000000000000000AA";
        }
        // Read production key from environment. Never hardcode it in source.
        const char* prod_key = std::getenv("TURNSTILE_SECRET_KEY");
        if (!prod_key || prod_key[0] == '\0') {
            std::cerr << "[turnstile] TURNSTILE_SECRET_KEY env var not set\n";
            return "";
        }
        return prod_key;
    }

    // Verify the token that the browser widget produced.
    // token       -- the cf-turnstile-response value from the form/callback
    // remote_ip   -- optional: the visitor's IP for extra verification
    // Returns true if Cloudflare confirms the token is valid.
    bool verify_token(const std::string& token,
                      const std::string& remote_ip = "") const {
        if (token.empty()) {
            std::cerr << "[turnstile] empty token\n";
            return false;
        }

        std::string secret = get_secret_key();
        if (secret.empty()) return false;

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "[turnstile] curl_easy_init failed\n";
            return false;
        }

        // URL-encode the token (tokens can contain + and = characters).
        char* enc_token  = curl_easy_escape(curl, token.c_str(),
                                             static_cast<int>(token.size()));
        char* enc_secret = curl_easy_escape(curl, secret.c_str(),
                                             static_cast<int>(secret.size()));

        std::string post_fields = std::string("secret=")   + enc_secret
                                + "&response=" + enc_token;

        if (!remote_ip.empty()) {
            char* enc_ip = curl_easy_escape(curl, remote_ip.c_str(),
                                             static_cast<int>(remote_ip.size()));
            post_fields += "&remoteip=";
            post_fields += enc_ip;
            curl_free(enc_ip);
        }
        curl_free(enc_token);
        curl_free(enc_secret);

        std::string read_buffer;

        // CORRECT endpoint: /turnstile/v0/siteverify
        // (previous code used "https://challenges.cloudflare.com" with no path
        //  which returns 404 and an unparseable body)
        curl_easy_setopt(curl, CURLOPT_URL,
            "https://challenges.cloudflare.com/turnstile/v0/siteverify");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](void* contents, size_t size, size_t nmemb, std::string* s) -> size_t {
                s->append(static_cast<char*>(contents), size * nmemb);
                return size * nmemb;
            });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "[turnstile] curl error: " << curl_easy_strerror(res) << "\n";
            return false;
        }

        try {
            auto response_json = json::parse(read_buffer);
            bool success = response_json.value("success", false);
            if (!success) {
                auto errors = response_json.value("error-codes", json::array());
                std::cerr << "[turnstile] verify failed: " << errors.dump() << "\n";
            }
            return success;
        } catch (const std::exception& e) {
            std::cerr << "[turnstile] json parse error: " << e.what()
                      << " | body: " << read_buffer.substr(0, 200) << "\n";
            return false;
        }
    }
};
