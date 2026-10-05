// Adapter: fetches a screen image from the server over HTTP.
#pragma once

#include "app/ports.h"

namespace epb {

class HttpScreenClient : public IScreenClient {
  public:
    FetchResult fetch(const char* url, const char* etag, uint8_t* frameOut, size_t frameCapacity) override;
};

}  // namespace epb
