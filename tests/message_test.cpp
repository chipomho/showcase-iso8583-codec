#include <gtest/gtest.h>

#include <vector>

#include "iso8583/message.hpp"
#include "iso8583/print.hpp"
#include "test_support.hpp"

namespace iso8583 {
namespace {

using test::hex;

TEST(Message, StartsWithJustItsMti) {
    const Message message{Mti{"0800"}};
    EXPECT_EQ(message.mti(), Mti{"0800"});
    EXPECT_TRUE(message.fields().empty());
    EXPECT_FALSE(message.has(2));
    EXPECT_EQ(message.find(2), nullptr);
}

TEST(Message, SetReplacesAnExistingValue) {
    Message message{Mti{"0200"}};
    message.set(3, "000000").set(3, "200000");
    EXPECT_EQ(message.text(3), "200000");
    EXPECT_EQ(message.fields().size(), 1U);
}

TEST(Message, TypedAccessRespectsTheStoredType) {
    Message message{Mti{"0200"}};
    message.set(4, "2500").set(52, hex("9F3A"));

    EXPECT_EQ(message.text(4), "2500");
    EXPECT_FALSE(message.binary(4));

    const auto pin = message.binary(52);
    ASSERT_TRUE(pin);
    EXPECT_EQ(to_hex(*pin), "9F3A");
    EXPECT_FALSE(message.text(52));

    EXPECT_FALSE(message.text(99));
    EXPECT_FALSE(message.binary(99));
}

TEST(Message, EraseReportsWhetherTheFieldWasThere) {
    Message message{Mti{"0200"}};
    message.set(11, "000001");
    EXPECT_TRUE(message.erase(11));
    EXPECT_FALSE(message.erase(11));
    EXPECT_FALSE(message.has(11));
}

TEST(Message, IteratesInWireOrder) {
    Message message{Mti{"0200"}};
    message.set(41, "TERM0001").set(128, hex("00")).set(2, "4111").set(11, "000001");

    std::vector<int> order;
    for (const auto& entry : message.fields()) {
        order.push_back(entry.first);
    }
    EXPECT_EQ(order, (std::vector<int>{2, 11, 41, 128}));
}

TEST(Message, EqualityComparesMtiAndEveryField) {
    Message a{Mti{"0200"}};
    a.set(3, "000000");
    Message b = a;
    EXPECT_EQ(a, b);

    b.set(3, "200000");
    EXPECT_NE(a, b);

    const Message c{Mti{"0210"}};
    EXPECT_NE(Message{Mti{"0200"}}, c);
}

} // namespace
} // namespace iso8583
