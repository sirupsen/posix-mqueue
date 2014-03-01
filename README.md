# posix-mqueue [![Build Status](https://travis-ci.org/Sirupsen/posix-mqueue.png?branch=master)](https://travis-ci.org/Sirupsen/posix-mqueue)

Minimal wrapper around the [POSIX message queue](pmq). The POSIX message queue
offers:

* Persistence. Push messages while nothing is listening.
* Simplicity. Nothing to set up. Built into Linux.
* IPC. Blazingly fast communication between processes on the same machine.
* Blocking and non-blocking. Listeners block until a message arrives on the
  queue. No polling. Sending messages doesn't block.


Note that this requires no third party message broker. The messages are handled
by the kernel of your computer. Not all kernels have support for POSIX message
queues, a notably example is Darwin (OS X). Darwin implements the older System V
IPC API. See my [SysV MQ wrapper](https://github.com/Sirupsen/sysvmq).

## Usage

In your Gemfile:

`gem 'posix-mqueue'`

### Important notes

1. This will not work on OS X, but on Linux and probably BSD (not tested).
2. `send` and `receive` block. `timedsend` and `timedreceive` do not.
3. The default message size is `4096` bytes.
4. Linux's default queue size is `10` bytes.

Read on for details.

### Example

```ruby
require 'posix/mqueue'

# On Linux the queue name must be prefixed with a slash. Note it is not a file
# created at `/whatever`. It's just the name of the queue.
# Set maximum default Linux options. See next section to push those limits.
# Default options are msgsize: 10 and maxmsg: 4096
m = POSIX::Mqueue.new("/whatever", msgsize: 10, maxmsg: 8192)
m.send "hello"
m.receive
# => "hello"

fork { POSIX::Mqueue.new("/whatever").send("world") }

# Blocks until the forked process pushes to the queue
m.receive
# => "world"

# Queue is now full by default Linux settings, see below on how to increase it.
10.times { m.send rand(100).to_s }

# #size returns the size of the queue
m.size
# => 10

# #send will block until something is popped off the now full queue.
# timesend takes timeout arguments (first one is seconds, second is
# nanoseconds). Pass 0 for for both to not block, this is default.

assert_raises POSIX::Mqueue::QueueFull do
  m.timedsend "I will fail"
end

# Empty the queue again
10.times { m.receive }

# Like timedsend, timedreceive takes timeout arguments and will raise
# POSIX::Mqueue::Queueempty when it would otherwise block.
assert_raises POSIX::Mqueue::QueueEmpty do
  m.timedreceive
end

# Deletes the queue and any messages remaining.
# None in this case. If not unlinked, the queue will persist till reboot.
m.unlink

```

### mqueue

Most important information from the manpages, with a little added information
about the behavior of `posix-mqueue`.

### /proc interfaces

Linux has some default limits you can easily change.

1. `/proc/sys/fs/mqueue/msg_max`. Contains the maximum number of messages in a
   single queue. Defaults to 10. You should increase that number. `#send` will
   eventually block if the queue is full. `#timedsend` will throw `QueueFull`.
2. `/proc/sys/fs/mqueue/msgsize_max`. Maximum size of a single message. Defaults
   to 8192 bytes. `posix-mqueue` allows up to 4096 bytes. Overwrite this by
   passing `{maxmsg: 8192}` as the second argument when initializing.
3. `/proc/sys/fs/mqueue/queues_max`. Maximum number of queues on the system.
   Defaults to 256.

## Virtual filesystem

The message queue is created as a virtual file system. That means you can mount
it:

```bash
# sudo mkdir /dev/queue
# sudo mount -t mqueue none /dev/queue
```

Add a queue and a few tasks, count the characters (19):

```ruby
$ irb
> require 'posix/mqueue'
=> true
> m = POSIX::Mqueue.new("/queue")
=> #<POSIX::Mqueue:0xb8c9fe88>
> m.send "narwhal"
=> true
> m.send "walrus"
=> true
> m.send "ponies"
=> true
```

Inspect the mounted filesystem:

```bash
$ ls /dev/queue/
important  mails  queue
$ cat /dev/queue/queue
QSIZE:19         NOTIFY:0     SIGNO:0     NOTIFY_PID:0
```

Here `QSIZE` is the bytes of data in the queue. The other flags are about
notifications which `posix-mqueue` does not support currently, read about them
in [mq_overview(7)][pmq].

[pmq]: http://man7.org/linux/man-pages/man7/mq_overview.7.html
