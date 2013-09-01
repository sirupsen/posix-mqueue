require 'test_helper'

class MqueueTest < MiniTest::Unit::TestCase
  def test_send_and_receive
    m = POSIX::Mqueue.new("/whatever")
    m.send("hello")
    assert_equal "hello", m.receive
  end
end
