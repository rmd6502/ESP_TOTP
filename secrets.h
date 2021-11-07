#include <Arduino.h>

#include <vector>

struct Secret {
  String username;
  String issuer;
  String secret;
  std::vector<uint8_t> secretBytes;
};

std::vector<uint8_t> base32Decode(String secret);
std::vector<Secret> readSecrets();
