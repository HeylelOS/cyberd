project('cyberd', 'c',
	default_options : [ 'c_std=c2x', 'c_args=-D_DEFAULT_SOURCE' ],
	license : 'BSD-3-Clause'
)

#################
# Configuration #
#################

configuration = configuration_data()
configuration.set('CONFIG_CONFIGURATION_DIRECTORY', get_option('sysconfdir') / 'daemons', description: 'Directory where cyberd will look for daemon configuration files')
configuration.set('CONFIG_DAEMON_CONF_LIST_DEFAULT_CAPACITY', 16, description: 'Default capacity of the buffer holding a daemon configuration list')
configuration.set('CONFIG_DAEMON_CONF_DEFAULT_UMASK', '022', description: 'Default umask for daemons')
configuration.set('CONFIG_ENDPOINTS_DIRECTORY', get_option('runstatedir') / 'init', description: 'Directory where cyberd will create its endpoints')
configuration.set('CONFIG_ENDPOINTS_ROOT', 'initctl', description: 'Name of the first endpoint to be created')
configuration.set('CONFIG_ENDPOINTS_CONNECTIONS', 64, description: 'Maximum connections an endpoint should listen to')
configuration.set('CONFIG_CONNECTIONS_READ_BUFFER_SIZE', 512, description: 'Size of the buffer used to read from endpoints')

###########
# Targets #
###########

subdir('include')
subdir('src/cyberctl')
subdir('src/cyberd')

#########
# Tests #
#########

subdir('test')

########
# Docs #
########

subdir('docs')
