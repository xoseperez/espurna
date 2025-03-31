# clone-stats

Safely clone node's
[`fs.Stats`](http://nodejs.org/api/fs.html#fs_class_fs_stats) instances without
losing their class methods, i.e. `stat.isDirectory()` and co.

This is a fork of [hughsk/clone-stats](https://github.com/hughsk/clone-stats) but adapted to work on Node v22+ without a deprecation warning

## Usage ##

[![clone-stats](https://nodei.co/npm/clone-stats.png?mini=true)](https://nodei.co/npm/clone-stats)

### `copy = require('clone-stats')(stat)` ###

Returns a clone of the original `fs.Stats` instance (`stat`).

## Monkey-patching other modules that depend on the original version ##

### `require('module-alias').addAlias('clone-stats', 'clone-stats-node22');` ###

## License ##

MIT. See [LICENSE.md](http://github.com/hughsk/clone-stats/blob/master/LICENSE.md) for details.
