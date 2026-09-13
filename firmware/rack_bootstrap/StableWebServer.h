#pragma once
#include <WebServer.h>

// Arduino-ESP32 3.3.8 stores only one Digest challenge. Its standard
// requestAuthentication() replaces that challenge on every 401, so concurrent
// browsers/pollers invalidate each other. Keep one random challenge per boot;
// rotate it explicitly when credentials change. Password/URI/digest validation
// remains in the pinned WebServer implementation.
class StableWebServer : public WebServer {
 public:
  using WebServer::WebServer;
  void resetWebChallenge() {
    _srealm="4VRS Display";
    _snonce=_getRandomHexString();
    _sopaque=_getRandomHexString();
  }
  bool authenticateWeb(const char *user,const char *password) {
    if(_snonce.isEmpty())resetWebChallenge();
    return authenticate(user,password);
  }
  void requestWebAuthentication() {
    if(_snonce.isEmpty())resetWebChallenge();
    String auth=header("Authorization");
    String nonce=_extractParam(auth,"nonce=\"",'"');
    // Cached challenges from before reboot/password change should trigger an
    // automatic browser retry, not another password dialog. A wrong password
    // using the current challenge is NOT marked stale.
    bool stale=auth.startsWith("Digest ")&&!nonce.isEmpty()&&nonce!=_snonce;
    String challenge="Digest realm=\""+_srealm+"\", qop=\"auth\", nonce=\""+_snonce+"\", opaque=\""+_sopaque+"\", algorithm=MD5";
    if(stale)challenge+=", stale=true";
    sendHeader("WWW-Authenticate",challenge);
    sendHeader("Cache-Control","no-store");
    send(401,"text/plain; charset=utf-8","Authentication required.");
  }
};
