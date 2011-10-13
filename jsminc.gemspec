# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)
require "jsminc/version"

Gem::Specification.new do |s|
  s.name        = "jsminc"
  s.version     = JSMinC::VERSION
  s.authors     = ["Ryan Fitzgerald"]
  s.email       = ["rwfitzge@gmail.com"]
  s.homepage    = "http://rynftz.gr/"
  s.summary     = %q{A fast JavaScript minifier written in C (by Douglas Crockford)}
  s.description = %q{JSMinC is the original C version of JSMin, embedded in Ruby.}

  s.rubyforge_project = "jsminc"

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.executables   = `git ls-files -- bin/*`.split("\n").map{ |f| File.basename(f) }
  s.extensions    = `git ls-files -- ext/**/extconf.rb`.split("\n")
  s.require_paths = ["lib"]

  s.add_development_dependency 'rake-compiler'
end
