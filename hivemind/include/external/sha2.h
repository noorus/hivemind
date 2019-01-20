#pragma once
#include <cstdint>

namespace hivemind {

  namespace sha2 {

    // This SHA256 implementation originally by Brad Conte, placed into public domain.
    // https://github.com/B-Con/crypto-algorithms
    // This version has been modified and does not represent the original source.

    struct context {
      uint8_t data[64];
      size_t datalen;
      unsigned long long bitlen;
      uint32_t state[8];
    };

    void sha256_init( context* ctx );
    void sha256_update( context* ctx, const uint8_t data[], size_t len );
    void sha256_final( context* ctx, uint8_t hash[] );

    void sha256_hash( const uint8_t data[], size_t len, uint8_t hash_out[] );

  }

}