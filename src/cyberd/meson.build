cyberd = executable('cyberd',
	include_directories : headers,
	install : true,
	install_dir : get_option('sbindir'),
	sources : [
		'configuration.c',
		'daemon.c',
		'daemon_conf.c',
		'dispatcher.c',
		'dispatcher_node.c',
		'hash.c',
		'log.c',
		'main.c',
		'scheduler.c',
		'signals.c',
		'spawns.c',
		'tree.c',
	]
)
