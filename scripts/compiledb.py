from SCons.Script import Default, Import

Import("env")

env.Tool("compilation_db")
compilation_db = env.CompilationDatabase("$COMPILATIONDB_PATH")
Default(compilation_db)
