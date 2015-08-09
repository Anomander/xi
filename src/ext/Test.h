#pragma once

#define STATIC_ASSERT_TEST(...) static_assert(__VA_ARGS__::value, "FAILURE");
