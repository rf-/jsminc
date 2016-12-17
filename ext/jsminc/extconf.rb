require 'mkmf'

extension_name = 'jsminc'

dir_config extension_name

with_cflags '-funsigned-char' do
  create_makefile extension_name
end
