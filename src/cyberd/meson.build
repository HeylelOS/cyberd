cyberd = executable('cyberd',
	include_directories : headers,
	install : true,
	install_dir : get_option('sbindir'),
	sources : [
		'configuration.c',
		'daemon.c',
		'daemon_conf.c',
		'log.c',
		'main.c',
		'signals.c',
		'socket_connection_node.c',
		'socket_endpoint_node.c',
		'socket_node.c',
		'socket_switch.c',
		'spawns.c',
		'tree.c',
	]
)
