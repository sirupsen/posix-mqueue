#include <ruby.h>
#include <ruby/util.h>

#include <mqueue.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <stdlib.h>
#include <stdio.h>

VALUE rb_cQueueFull = Qnil;
VALUE rb_cQueueEmpty = Qnil;

typedef struct {
  mqd_t fd;
  struct mq_attr attr;
  size_t queue_len;
  char *queue;
}
mqueue_t;

static void
mqueue_mark(void* ptr)
{
  (void)ptr;
}

static void
mqueue_free(void* ptr)
{
  mqueue_t* data = ptr;
  mq_close(data->fd);
  xfree(data->queue);
  xfree(ptr);
}

static size_t
mqueue_memsize(const void* ptr)
{
  const mqueue_t* data = ptr;
  return sizeof(mqueue_t) + sizeof(char) * data->queue_len;
}

static const rb_data_type_t
mqueue_type = {
  "mqueue_type",
  {
    mqueue_mark,
    mqueue_free,
    mqueue_memsize
  }
};

static VALUE
posix_mqueue_alloc(VALUE klass)
{
  mqueue_t* data;
  VALUE obj = TypedData_Make_Struct(klass, mqueue_t, &mqueue_type, data);

  data->fd = -1;
  data->queue = NULL;
  data->queue_len = 0;

  return obj;
}

VALUE posix_mqueue_unlink(VALUE self)
{
  mqueue_t* data;

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (mq_unlink(data->queue) == -1) {
    rb_sys_fail("Message queue unlinking failed, please consume mq_unlink(3)");
  }

  return Qtrue;
}

VALUE posix_mqueue_send(VALUE self, VALUE message)
{
  int err;
  mqueue_t* data;

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (!RB_TYPE_P(message, T_STRING)) { 
    rb_raise(rb_eTypeError, "Message must be a string"); 
  }

  // TODO: Custom priority
  err = mq_send(data->fd, RSTRING_PTR(message), RSTRING_LEN(message), 10);

  if (err < 0) {
    rb_sys_fail("Message sending failed, please consult mq_send(3)");
  }
  
  return Qtrue;
}

VALUE posix_mqueue_timedreceive(VALUE self, VALUE args)
{
  int err;
  mqueue_t* data;
  size_t buf_size;
  char *buf;
  struct timespec timeout;
  VALUE str;
  VALUE seconds = rb_ary_entry(args, 0);
  VALUE nanoseconds = rb_ary_entry(args, 1);

  if (seconds == Qnil) seconds = INT2FIX(0);
  if (nanoseconds == Qnil) nanoseconds = INT2FIX(0);

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (!RB_TYPE_P(seconds, T_FIXNUM)) { 
    rb_raise(rb_eTypeError, "First argument must be a Fixnum"); 
  }

  if (!RB_TYPE_P(nanoseconds, T_FIXNUM)) { 
    rb_raise(rb_eTypeError, "Second argument must be a Fixnum"); 
  }

  timeout.tv_sec  = FIX2ULONG(seconds);
  timeout.tv_nsec = FIX2ULONG(nanoseconds);

  buf_size = data->attr.mq_msgsize + 1;

  // Make sure the buffer is capable
  buf = (char*)malloc(buf_size);

  // TODO: Specify priority
  err = mq_timedreceive(data->fd, buf, buf_size, NULL, &timeout);

  if (err < 0) {
    if(errno == 110) {
      rb_raise(rb_cQueueEmpty, "Queue empty");
    } else {
      rb_sys_fail("Message sending failed, please consult mq_send(3)");
    }
  }

  str = rb_str_new(buf, err);
  free(buf);

  return str;
}

VALUE posix_mqueue_timedsend(VALUE self, VALUE args)
{
  int err;
  mqueue_t* data;
  struct timespec timeout;
  VALUE message = rb_ary_entry(args, 0);
  VALUE seconds = rb_ary_entry(args, 1);
  VALUE nanoseconds = rb_ary_entry(args, 2);

  if (seconds == Qnil) seconds = INT2FIX(0);
  if (nanoseconds == Qnil) nanoseconds = INT2FIX(0);

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (!RB_TYPE_P(message, T_STRING)) { 
    rb_raise(rb_eTypeError, "Message must be a string"); 
  }

  if (!RB_TYPE_P(seconds, T_FIXNUM)) {
    rb_raise(rb_eTypeError, "First argument must be a Fixnum"); 
  }

  if (!RB_TYPE_P(nanoseconds, T_FIXNUM)) { 
    rb_raise(rb_eTypeError, "Second argument must be a Fixnum"); 
  }

  timeout.tv_sec  = FIX2ULONG(seconds);
  timeout.tv_nsec = FIX2ULONG(nanoseconds);

  err = mq_timedsend(data->fd, RSTRING_PTR(message), RSTRING_LEN(message), 10, &timeout);

  if (err < 0) {
    if(errno == 110) {
      rb_raise(rb_cQueueFull, "Queue full, most likely you want to bump /proc/sys/fs/mqueue/msg_max from the default maximum queue size of 10.");
    } else {
      rb_sys_fail("Message sending failed, please consult mq_send(3)");
    }
  }
  
  return Qtrue;
}

VALUE posix_mqueue_size(VALUE self)
{
  mqueue_t* data;
  struct mq_attr queue;

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (mq_getattr(data->fd, &queue) < 0) {
    rb_sys_fail("Failed reading queue attributes, please consult mq_getattr(3)");
  }

  return INT2FIX(queue.mq_curmsgs);
}

VALUE posix_mqueue_receive(VALUE self)
{
  int err;
  size_t buf_size;
  char *buf;
  VALUE str;

  mqueue_t* data;

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  buf_size = data->attr.mq_msgsize + 1;

  // Make sure the buffer is capable
  buf = (char*)malloc(buf_size);

  // TODO: Specify priority
  err = mq_receive(data->fd, buf, buf_size, NULL);

  if (err < 0) {
    rb_sys_fail("Message retrieval failed, please consult mq_receive(3)");
  }

  str = rb_str_new(buf, err);
  free(buf);

  return str;
}

VALUE posix_mqueue_initialize(VALUE self, VALUE args)
{
  VALUE options = rb_ary_entry(args, 1);
  if (options == Qnil) options = rb_hash_new();

  int msgsize = FIX2INT(rb_hash_lookup2(options, ID2SYM(rb_intern("msgsize")), INT2FIX(4096)));
  int maxmsg = FIX2INT(rb_hash_lookup2(options, ID2SYM(rb_intern("maxmsg")), INT2FIX(10)));
  VALUE queue = rb_ary_entry(args, 0);

  struct mq_attr attr = {
    .mq_flags   = 0,          // Flags, 0 or O_NONBLOCK
    .mq_maxmsg  = maxmsg,     // Max messages in queue
    .mq_msgsize = msgsize,    // Max message size (bytes)
    .mq_curmsgs = 0           // # currently in queue
  };

  mqueue_t* data;
  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (data->fd != -1) {
    // This would cause a memleak otherwise
    rb_raise(rb_eRuntimeError, "Illegal reinitialization");
  }

  data->attr = attr;
  data->queue_len = RSTRING_LEN(queue);
  data->queue = ruby_strdup(StringValueCStr(queue));
  data->fd = mq_open(data->queue, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &data->attr);

  if (data->fd == (mqd_t)-1) {
    rb_sys_fail("Failed opening the message queue, please consult mq_open(3)");
  }

  return self;
}

void Init_mqueue()
{
  VALUE posix = rb_define_module("POSIX");
  VALUE mqueue = rb_define_class_under(posix, "Mqueue", rb_cObject);
  rb_cQueueFull = rb_define_class_under(mqueue, "QueueFull", rb_eStandardError);
  rb_cQueueEmpty = rb_define_class_under(mqueue, "QueueEmpty", rb_eStandardError);

  rb_define_alloc_func(mqueue, posix_mqueue_alloc);
  rb_define_method(mqueue, "initialize", posix_mqueue_initialize, -2);
  rb_define_method(mqueue, "send", posix_mqueue_send, 1);
  rb_define_method(mqueue, "receive", posix_mqueue_receive, 0);
  rb_define_method(mqueue, "timedsend", posix_mqueue_timedsend, -2);
  rb_define_method(mqueue, "timedreceive", posix_mqueue_timedreceive, -2);
  rb_define_method(mqueue, "size", posix_mqueue_size, 0);
  rb_define_method(mqueue, "unlink", posix_mqueue_unlink, 0);
}

