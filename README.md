# Posix::Mqueue

Minimal wrapper around the [POSIX message queue](pmq). The POSIX message queue
offers:

* Persistence. Push messages while nothing is listening.
* Simplicity. Nothing to set up. Built into Linux.
* IPC. Blazingly fast communication between processes on the same machine.
* Blocking and non-blocking. Listeners block until a message arrives on the
  queue. No polling. Sending messages doesn't block.

Add `gem 'posix-mqueue'` to your favorite Gemfile.

## Usage

```ruby
m = POSIX::Mqueue.new("/whatever")
m.send "hello"
puts m.receive
# => "hello"

fork { POSIX::Mqueue.new("/whatever").send("world") }

# Blocks until the forked process pushes to the queue
m.receive
# => "world"
```

[pmq]: http://man7.org/linux/man-pages/man7/mq_overview.7.html
