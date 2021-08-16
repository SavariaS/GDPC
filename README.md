# GDPC

A command line tool to manipulate Godot's package files (.pck).

#### Usage:

`gdpc [-aceiluv] [--longoption ...] [[file ...] dest]`

#### Operation mode:

| Flag | Description |
| ---- | ----------- |
| --list, -l | Lists all the files in the package(s). |
| --extract, -e | Extracts files from the package(s). |
| --create, -c | Creates a new package file. |
| --update, -u | Modifies or appends files to a package. |

#### Extraction mode:

| Flag | Description |
| ---- | ----------- |
| *default* | Extracts all files. |
| --import, -i | Extracts non-Godot files and .import files. Extracts assets from resource files. Useful for easy re-importing of assets with the engine. | 
| --assets-only, -r | Ignores all Godot files and extracts assets from resource files. |

#### Options:
| Flag | Description |
| ---- | ----------- |
| --verbose, -v | Prints additional information. |
| --help, -h | Prints a short help message. No arguments allowed. |