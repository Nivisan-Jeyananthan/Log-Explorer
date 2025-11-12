project('log-explorer', 'c',
  version : '0.1',
  default_options : ['warning_level=2']
)

# Suppress GTK deprecation warnings for now (UI uses some deprecated GtkTreeView APIs).
# Long-term: port src/ui.c to GtkListView/GListModel/GtkColumnView.
add_project_arguments('-Wno-deprecated-declarations', language : 'c')

sqlite_dep = dependency('sqlite3', required: true)

gtk_dep = dependency('gtk4', required: false)

if gtk_dep.found()
  executable('log-explorer',
    'src/main.c',
    'src/ui.c',
    'src/db.c',
    'src/indexer.c',
    include_directories : include_directories('src'),
    dependencies : [gtk_dep, sqlite_dep],
    install : true
  )
else
  message('gtk4 not found; skipping GUI build. You can still build the CLI test with "log-explorer-cli"')
endif

executable('log-explorer-cli',
  'src/cli_test.c',
  'src/db.c',
  'src/indexer.c',
  include_directories : include_directories('src'),
  dependencies : [sqlite_dep],
  install : true
)

executable('insert-sample',
  'tools/insert_sample.c',
  'src/db.c',
  include_directories : include_directories('src'),
  dependencies : [sqlite_dep],
  install : true
)
