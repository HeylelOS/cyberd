cover = dependency('cover', required : false)

if cover.found()
	names = [
		'test-cyberd-tree',
	]

	foreach name : names
		test(name,
			executable(name,
				dependencies : cover,
				include_directories : '../src',
				sources : [ name + '.c' ]
			),
			args : [ '-tap', '-' ],
			protocol : 'tap'
		)
	endforeach
endif
