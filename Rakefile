require 'bundler/gem_tasks'

require 'rake/extensiontask'
Rake::ExtensionTask.new('jsminc_ext')

task :irb => [:clobber, :compile] do
  exec "irb -Ilib -rjsminc"
end
