# GDPC

A command line tool to manipulate Godot's package files (.pck).

#### Usage:

`gdpc [-celuv] [--longoption ...] [[file ...] dest]`

#### Operation mode:

| Flag | Description |
| ---- | ----------- |
| --list, -l | Lists all the files in the package(s). |
| --extract, -e | Extracts files from the package(s). |
| --create, -c | Creates a new package file. |
| --update, -u | Modifies or appends files to a package. |

#### Extract options

| Flag | Description |
| ---- | ----------- |
| --convert | Convert resource files to their original asset. |
| -w="path" | Adds file(s) to the whitelist. By default, all files are whitelisted. | 
| -b="path" | Adds file(s) to the blacklist. By default, no files are blacklisted. |
| --ignore-resources | Adds all resource files to the blacklist. Equivalent to `-b=*.stex -b=*.image -b=*.res -b=*.texarr -b=*.tex3d` |

#### Create options

| Flag | Description |
| ---- | ----------- |
| -v=X.X.X | Specify the engine version |

#### General Options:
| Flag | Description |
| ---- | ----------- |
| --verbose, -v | Prints additional information. |
| --help, -h | Prints a short help message. No arguments allowed. |