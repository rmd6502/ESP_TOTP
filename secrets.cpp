#include "secrets.h"

#include <Arduino.h>
#include <SPIFFS.h>

using namespace std;

vector<uint8_t> base32Decode(String secret) {
    uint8_t bitpos = 0;
    uint8_t index = 0;
    vector<uint8_t> *ret = new vector<uint8_t>();

    ret->push_back(0);
    for (int i=0; i < secret.length(); ++i) {
      char c = secret[i];
      uint8_t bits = 0;
      if (c >= 'A' && c <= 'Z') {
        bits = c - 'A';
      } else {
        bits = c - 24;
      }
      if (bitpos <= 3) {
        (*ret)[index] |= bits << (3 - bitpos);
        bitpos += 5; 
      } else {
        (*ret)[index++] |= bits >> (bitpos - 3);
        bits &= (0xff >> (11 - bitpos));
        ret->push_back(bits << (11 - bitpos));
        bitpos = bitpos - 3;
      }
    }
    return *ret;
}

std::vector<Secret> readSecrets() {
  auto secrets = new std::vector<Secret>();
  File fp = SPIFFS.open("/secrets");
  if (!fp) {
    return *secrets;
  }
  String content;
  while (fp.available()) {
    content += fp.readString();
  }
  int idx;
  while ((idx = content.indexOf('\n')) != -1) {
    Secret *secret = new Secret();
    int c1 = content.indexOf(',');
    secret->username = content.substring(0,c1);
    int c2 = content.indexOf(',',c1+1);
    secret->issuer = content.substring(c1+1,c2);
    secret->secret = content.substring(c2+1,idx);
    secret->secretBytes = base32Decode(secret->secret);
    content = content.substring(idx+1);
    secrets->push_back(*secret);
  }
  return *secrets;
}
