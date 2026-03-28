#pragma once

// Forwarder: ws_parse.hpp was moved to the public include tree so that
// header-only CRTP templates (TypedMarketStream, TypedUserStream) can
// access the inline parse functions.  Existing src/ code includes this
// path unchanged.
#include <bintrade/ws/detail/ws_parse.hpp>
