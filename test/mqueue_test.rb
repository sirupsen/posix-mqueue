require 'test_helper'

class MqueueTest < MiniTest::Unit::TestCase
  def test_send_and_receive_single_message
    m = POSIX::Mqueue.new("/whatever")
    m.send "hello"
    assert_equal "hello", m.receive
  end

  def test_send_and_receive_multiple_messages
    m = POSIX::Mqueue.new("/whatever")
    m.send "hello"
    m.send "world"

    assert_equal "hello", m.receive
    assert_equal "world", m.receive
  end

  def test_receiver_blocks
    m = POSIX::Mqueue.new("/whatever")
    m.send "hello"

    assert_equal "hello", m.receive

    fork { POSIX::Mqueue.new("/whatever").send("world") }

    assert_equal "world", m.receive
  end
end
