require "bundler/gem_tasks"

task :default => [:test]

require 'rake/testtask'
Rake::TestTask.new do |t|
  `cd ext/posix && ruby extconf.rb && make`
  t.libs << "test"
  t.pattern = "test/**/*_test.rb"
end
