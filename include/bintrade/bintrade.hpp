#pragma once

// Core
#include <bintrade/core/config.hpp>
#include <bintrade/core/error.hpp>
#include <bintrade/core/spsc_queue.hpp>
#include <bintrade/core/types.hpp>

// Auth
#include <bintrade/auth/credentials.hpp>
#include <bintrade/auth/signer.hpp>

// Models
#include <bintrade/models/account.hpp>
#include <bintrade/models/common.hpp>
#include <bintrade/models/market_data.hpp>
#include <bintrade/models/order.hpp>
#include <bintrade/models/trade.hpp>

// REST
#include <bintrade/rest/account_client.hpp>
#include <bintrade/rest/client.hpp>
#include <bintrade/rest/market_data_client.hpp>
#include <bintrade/rest/trading_client.hpp>

// WebSocket
#include <bintrade/ws/client.hpp>
#include <bintrade/ws/handler_traits.hpp>
#include <bintrade/ws/market_stream.hpp>
#include <bintrade/ws/typed_market_stream.hpp>
#include <bintrade/ws/typed_user_stream.hpp>
#include <bintrade/ws/user_stream.hpp>

// Version
#include <bintrade/version.hpp>
