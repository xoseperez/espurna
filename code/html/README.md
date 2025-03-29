# Building

All commands must be called from `code/`
```console
$ pwd
/home/dev/espurna/code
```

Install dependencies before using the builder
```console
$ npm install
```

Build every .html.gz.h

```console
$ gulp 
```

Build specific flavour

```console
$ gulp webui_small
```

List available tasks
```console
$ gulp --tasks
```

# Development

Launches development server on port 8080 with the resulting .html bundle. No minification or compression, using `webui_all` as base

```console
$ gulp dev
```

Call [eslint](https://eslint.org) and [html-validate](https://html-validate.org) before building

```console
$ gulp eslint
$ gulp html_validate
```

# Directories

- `code/espurna/static/` - .html.gz.h, used in the firmware code
- `code/html/build` - intermediate build results
- `node_modules` - build dependencies

