require 'test_helper'

class MqueueTest < MiniTest::Unit::TestCase
  def setup
    @queue_name = "/test-queue"
    @queue = POSIX::Mqueue.new(@queue_name)
  end

  def teardown
    @queue.unlink
  end

  def test_send_and_receive_single_message
    @queue.send "hello"
    assert_equal "hello", @queue.receive
  end

  def test_send_and_receive_multiple_messages
    @queue.send "hello"
    @queue.send "world"

    assert_equal "hello", @queue.receive
    assert_equal "world", @queue.receive
  end

  def test_receiver_blocks
    @queue.send "hello"

    assert_equal "hello", @queue.receive

    fork { POSIX::Mqueue.new(@queue_name).send("world") }

    assert_equal "world", @queue.receive
  end

  def test_multiple_queues
    @queue.send "hello"

    other = POSIX::Mqueue.new("/other-test-queue")
    other.send "world"

    assert_equal "world", other.receive
    assert_equal "hello", @queue.receive

    other.unlink
  end
end
