require 'mkmf'
have_header('mqueue.h')
have_library("rt")
create_makefile('posix/mqueue')
