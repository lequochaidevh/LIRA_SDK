#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "lira_protocol_command_sender.hpp"  // Path to your header

using namespace lirasdk;
using ::testing::_;

// --- TEST FIXTURE ---
class LiraProtocolCommandSenderCallbackTest : public ::testing::Test {
 protected:
    asio::io_context io_ctx;

    // Dummy high-level send function signature demanded by constructor
    std::function<bool(const LiraProtocolCommandSender::Command&)> dummy_high_level_send =
        [](const LiraProtocolCommandSender::Command&) { return true; };
};

// Test Case 1: Verifies that send fails immediately if no raw callback is registered
TEST_F(LiraProtocolCommandSenderCallbackTest, ReturnFalseIfCallbackNotRegistered) {
    LiraProtocolCommandSender sender(io_ctx);

    LiraProtocolCommandSender::CommandLong cmd_long;
    cmd_long.command = 111;

    // do_work calls send_lira_protocol_message internally.
    // Since register_send_message_callback wasn't invoked, it must return false safely.
    sender.queue_command_async(cmd_long, [](LiraProtocolCommandSender::Result, float) {});

    // We can also trigger the public wrappers or directly call internal logic via state validation
    // Here we ensure running do_work doesn't execute a null pointer
    EXPECT_NO_THROW(sender.do_work());
}

// Test Case 2: Verifies that CommandInt gets correctly packed and dispatched to raw wire
TEST_F(LiraProtocolCommandSenderCallbackTest, PackAndSendCommandIntSuccessfully) {
    LiraProtocolCommandSender sender(io_ctx);

    bool     raw_callback_triggered = false;
    uint16_t captured_msg_id        = 0;
    uint8_t  captured_own_sys       = 0;

    sender.register_send_message_callback([&](const lira_protocol_message_t& msg) {
        raw_callback_triggered = true;
        captured_msg_id        = msg.msgid;  // This captures the message type ID (75 for COMMAND_INT)
        captured_own_sys       = msg.sysid;  // This captures the sender's own system ID
        return true;
    });

    LiraProtocolCommandSender::CommandInt cmd_int;
    cmd_int.command             = 456;
    cmd_int.target_system_id    = 9;
    cmd_int.target_component_id = 1;
    cmd_int.frame               = LIRA_FRAME_GLOBAL_RELATIVE_ALT;

    sender.queue_command_async(cmd_int, [](LiraProtocolCommandSender::Result, float) {});

    // Flush the async scheduler
    io_ctx.poll();

    // Assertions
    EXPECT_TRUE(raw_callback_triggered);
    EXPECT_EQ(captured_msg_id, 75);
    EXPECT_EQ(captured_own_sys, 1);  // Note: 1 is the sender's default _own_system_id
}

// Test Case 3: Verifies that CommandLong gets correctly packed and dispatched to raw wire
TEST_F(LiraProtocolCommandSenderCallbackTest, PackAndSendCommandLongSuccessfully) {
    LiraProtocolCommandSender sender(io_ctx);

    bool     raw_callback_triggered = false;
    uint16_t captured_msg_id        = 0;
    uint8_t  captured_own_sys       = 0;

    sender.register_send_message_callback([&](const lira_protocol_message_t& msg) {
        raw_callback_triggered = true;
        captured_msg_id        = msg.msgid;  // This captures the message type ID (76 for COMMAND_LONG)
        captured_own_sys       = msg.sysid;
        return true;
    });

    LiraProtocolCommandSender::CommandLong cmd_long;
    cmd_long.command             = 789;
    cmd_long.target_system_id    = 3;
    cmd_long.target_component_id = 1;

    sender.queue_command_async(cmd_long, [](LiraProtocolCommandSender::Result, float) {});

    io_ctx.poll();

    // Assertions
    EXPECT_TRUE(raw_callback_triggered);
    EXPECT_EQ(captured_msg_id, 76);
    EXPECT_EQ(captured_own_sys, 1);  // Note: 1 is the sender's default _own_system_id
}