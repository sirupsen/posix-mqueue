require "bundler/gem_tasks"

require 'rake/testtask'
Rake::TestTask.new do |t|
  system("cd ext/ && ruby extconf.rb && make")
  t.libs << "test"
  t.pattern = "test/**/*_test.rb"
end
