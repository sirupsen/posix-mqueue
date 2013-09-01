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

  def test_timedsend_raises_exception_instead_of_blocking
    10.times { @queue.timedsend "walrus", 0, 0 }

    assert_raises POSIX::Mqueue::QueueFull do
      @queue.timedsend("penguin", 0, 0)
    end
  end

  def test_timedreceive_raises_exception_instead_of_blocking
    assert_raises POSIX::Mqueue::QueueEmpty do
      @queue.timedreceive(0, 0)
    end
  end

  def test_errors_when_queue_name_is_not_slash_prefixed
    assert_raises Errno::EINVAL do
      POSIX::Mqueue.new("notvalid")
    end
  end

  def test_custom_message_size
    assert_raises Errno::EMSGSIZE do
      @queue.send('c' * 4097) # one byte too large
    end

    # Set to the maximum for Linux
    w = POSIX::Mqueue.new("/big-queue", msgsize: 2 ** 13)
    w.send('c' * (2 ** 13))
    w.unlink
  end

  def test_custom_max_queue_size
    w = POSIX::Mqueue.new("/small-queue", maxmsg: 2)
    2.times { w.send "narwhal" }

    assert_raises POSIX::Mqueue::QueueFull do
      w.timedsend("narwhal", 0, 0)
    end

    w.unlink
  end

  def test_count_in_queue
    assert_equal 0, @queue.size

    @queue.send "first"
    @queue.send "second"
    @queue.send "third"

    assert_equal 3, @queue.size
  end
end
