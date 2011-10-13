require 'mkmf'

extension_name = 'jsminc'

dir_config extension_name

if RUBY_VERSION =~ /1.9/ then
  $CPPFLAGS += "-DRUBY_19"
end

with_cflags '-funsigned-char' do
  create_makefile extension_name
end
