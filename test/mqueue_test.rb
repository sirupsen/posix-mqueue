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

    with_queue "/other-test-queue" do |q|
      q.send "world"
      assert_equal "world", q.receive
    end

    assert_equal "hello", @queue.receive
  end

  def test_timedsend_raises_exception_instead_of_blocking
    10.times { @queue.timedsend "walrus", 0, 0 }

    assert_raises POSIX::Mqueue::QueueFull do
      @queue.timedsend("penguin")
    end
  end

  def test_timedreceive_raises_exception_instead_of_blocking
    assert_raises POSIX::Mqueue::QueueEmpty do
      @queue.timedreceive
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
    with_queue "/big-queue", msgsize: 2 ** 13 do |q|
      assert_equal 2 ** 13, q.msgsize

      q.send('c' * (2 ** 13))
    end
  end

  def test_custom_max_queue_size
    with_queue "/small-queue", maxmsg: 2 do |q|
      2.times { q.send "narwhal" }

      assert_raises POSIX::Mqueue::QueueFull do
        q.timedsend("narwhal", 0, 0)
      end
    end
  end

  def test_count_in_queue
    assert_equal 0, @queue.size

    @queue.send "first"
    @queue.send "second"
    @queue.send "third"

    assert_equal 3, @queue.size
  end

  def test_to_io
    assert_instance_of IO, @queue.to_io
  end

  private
  def with_queue(name, options = {})
    queue = POSIX::Mqueue.new(name, options)
    begin
      yield(queue)
    ensure
      queue.unlink
    end
  end
end
