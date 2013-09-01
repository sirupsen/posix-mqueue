#include <ruby.h>

#include <mqueue.h>
#include <fcntl.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>

mqd_t
rb_mqueue_fd(const char *queue, const struct mq_attr *attr) {
  mqd_t fd = mq_open("/mqueue", O_CREAT | O_RDWR, S_IRWXU | S_IRWXO | S_IRWXG, attr);

  if (fd == (mqd_t)-1) {
    rb_sys_fail("Failed opening the message queue");
  }

  return fd;
}

typedef struct {
  mqd_t fd;
  struct mq_attr attr;
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
  xfree(ptr);
}

static size_t
mqueue_memsize(const void* ptr)
{
  return sizeof(mqueue_t);
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
  return TypedData_Make_Struct(klass, mqueue_t, &mqueue_type, data);
}

VALUE posix_mqueue_send(VALUE self, VALUE message)
{
  int err;
  mqueue_t* data;

  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  if (!RB_TYPE_P(message, T_STRING)) { 
    rb_raise(rb_eTypeError, "Message must be a string"); 
  }

  // FIXME: is rstring_len with or without \0?
  // TODO: Custom priority
  err = mq_send(data->fd, RSTRING_PTR(message), RSTRING_LEN(message), 10);

  if (err < 0) {
    rb_sys_fail("Message sending failed");
  }
  
  return Qtrue;
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
    rb_sys_fail("Message retrieval failed");
  }

  str = rb_str_new(buf, err);
  free(buf);

  return str;
}

VALUE posix_mqueue_initialize(VALUE self, VALUE queue)
{
  // TODO: Modify these options from initialize arguments
  // TODO: Set nonblock and handle error in #push
  struct mq_attr attr = {
    .mq_flags   = 0,    // Flags, 0 or O_NONBLOCK
    .mq_maxmsg  = 100,  // Max messages in queue
    .mq_msgsize = 512,  // Max message size (bytes)
    .mq_curmsgs = 0     // # currently in queue
  };

  mqueue_t* data;
  TypedData_Get_Struct(self, mqueue_t, &mqueue_type, data);

  data->attr = attr;

  // FIXME: This is probably dangerous since I don't whether the value is
  // actually a string.
  data->fd = rb_mqueue_fd(StringValueCStr(queue), &data->attr);

  return self;
}

void Init_mqueue()
{
  VALUE posix = rb_define_module("POSIX");
  VALUE mqueue = rb_define_class_under(posix, "Mqueue", rb_cObject);
  rb_define_alloc_func(mqueue, posix_mqueue_alloc);
  rb_define_method(mqueue, "initialize", posix_mqueue_initialize, 1);
  rb_define_method(mqueue, "send", posix_mqueue_send, 1);
  rb_define_method(mqueue, "receive", posix_mqueue_receive, 0);
}

