Builder script is using **Gulp**

- https://gulpjs.com
- https://www.npmjs.com/package/gulp

Additional tools used
- [eslint](https://eslint.org) to validate javascript files
- [html-validate](https://html-validate.org) to validate html file
- [vitest](https://vitest.dev/) testing framework

All commands must be called from `code/`
```console
$ pwd
/home/dev/espurna/code
```

When `gulp` command is not available globally
```console
$ ./node_modules/.bin/gulp
```

# Files

- `code/espurna/static/` - .html.gz.h, used in the firmware code
- `code/gulpfile.mjs` - builder script entrypoint
- `code/html/build` - intermediate build results
- `code/html/spec` - vitest 'spec' files
- `code/html/src` - source .mjs, .html, etc.
- `code/node_modules` - build dependencies
- `code/package{,-lock}.json` - build dependencies metadata

# Installation

*(recommended)* Install **exact** dependencies listed in the `package-lock.json`, from the time the builder code was last updated
```console
$ npm ci
```

*(optional)* Install dependencies listed in the `package.json`, possibly updating packages listed in the `package-lock.json`
```console
$ npm install
```

# Building

Build every .html.gz.h (`default` task)

```console
$ gulp
```

Build specific flavour (`webui_...` tasks)

```console
$ gulp webui_small
```

List all available tasks
```console
$ gulp --tasks
```

# Development

Launches development server on port 8080 with the resulting .html bundle. No minification or compression, using `webui_all` as base

```console
$ gulp dev
```

Only the `default` task depends on the test and lint tasks, call them manually

```console
$ gulp eslint html-validate vitest
```
```console
$ gulp eslint
$ gulp html-validate
$ gulp vitest
```
