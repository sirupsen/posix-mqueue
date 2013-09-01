require 'minitest/unit'
require 'minitest/autorun'

$: << File.dirname(__FILE__) + '/../ext'

require "posix/mqueue"
