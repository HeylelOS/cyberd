hvn_man = find_program('hvn-man', native : true, required : false)
doxygen = find_program('doxygen', native : true, required : false)

if hvn_man.found()
	man_command = [ hvn_man, '-i', '@INPUT@', '-o', '@OUTPUT@' ]

	man_1_cyberctl = custom_target('cyberctl(1)',
		output : 'cyberctl.1', input : 'cyberctl.1.md', command : man_command,
		install : true, install_dir : get_option('mandir') / 'man1', install_tag : 'man'
	)

	man_5_cyberd = custom_target('cyberd(5)',
		output : 'cyberd.5', input : 'cyberd.5.md', command : man_command,
		install : true, install_dir : get_option('mandir') / 'man5', install_tag : 'man'
	)

	man_8_cyberd = custom_target('cyberd(8)',
		output : 'cyberd.8', input : 'cyberd.8.md', command : man_command,
		install : true, install_dir : get_option('mandir') / 'man8', install_tag : 'man'
	)
endif

if doxygen.found()
	doxyfile = configure_file(configuration : {
		'PROJECT_NAME' : meson.project_name(),
		'INPUT' : ' '.join([
			meson.project_source_root() / 'include/capabilities.h',
			meson.project_source_root() / 'src',
			meson.current_source_dir() / 'endpoints.md',
		]),
	}, input : 'Doxyfile.in', output : 'Doxyfile')

	doc = custom_target('html',
		output : 'html',
		depend_files : [ doxyfile ],
		depends : [ cyberd ],
		command : [ doxygen, doxyfile ],
		install : true, install_dir : get_option('datadir') / 'html',
		install_tag : 'doc'
	)
endif
