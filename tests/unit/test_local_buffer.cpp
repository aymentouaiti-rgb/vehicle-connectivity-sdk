#include <gtest/gtest.h>
#include "vcsdk/buffer/LocalBuffer.hpp"
#include "vcsdk/core/ConnectorError.hpp"

using namespace vcsdk::buffer;
using namespace vcsdk::core;

TEST(LocalBuffer, PushEtPopRespectentLOrdreFIFO) {
    LocalBuffer<int> buf(10);
    buf.push(1);
    buf.push(2);
    buf.push(3);
    EXPECT_EQ(buf.pop().value(), 1);
    EXPECT_EQ(buf.pop().value(), 2);
    EXPECT_EQ(buf.pop().value(), 3);
}

TEST(LocalBuffer, PopSurBufferVideRetourneNullopt) {
    LocalBuffer<int> buf(10);
    EXPECT_FALSE(buf.pop().has_value());
}

TEST(LocalBuffer, LeveOverflowQuandPlein) {
    LocalBuffer<int> buf(2);
    buf.push(1);
    buf.push(2);
    EXPECT_THROW(buf.push(3), ConnectorError);
}

TEST(LocalBuffer, SizeEtEmptyReussissent) {
    LocalBuffer<int> buf(10);
    EXPECT_TRUE(buf.empty());
    buf.push(42);
    EXPECT_EQ(buf.size(), 1u);
    EXPECT_FALSE(buf.empty());
}
