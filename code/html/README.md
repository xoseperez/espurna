Builder script is using **Gulp**

- https://gulpjs.com
- https://www.npmjs.com/package/gulp

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
- `code/html/build` - intermediate build results
- `code/package{,-lock}.json` - build dependencies metadata
- `code/node_modules` - build dependencies
- `code/gulpfile.mjs` - builder script entrypoint

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

Only the `default` task invokes [eslint](https://eslint.org) and [html-validate](https://html-validate.org) before building, call them manually

```console
$ gulp eslint html_validate
```
```console
$ gulp eslint
$ gulp html_validate
```
