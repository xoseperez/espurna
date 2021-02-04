/*

ESP8266 file system builder

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*eslint quotes: ['error', 'single']*/
/*eslint-env es6*/

// -----------------------------------------------------------------------------
// Dependencies
// -----------------------------------------------------------------------------

const path = require('path');

const gulp = require('gulp');
const through = require('through2');

const htmllint = require('gulp-htmllint');
const csslint = require('gulp-csslint');

const htmlmin = require('html-minifier');

const gzip = require('gulp-gzip');
const inline = require('gulp-inline-source-html');
const rename = require('gulp-rename');
const replace = require('gulp-replace');

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

const htmlFolder = 'html/';
const dataFolder = 'espurna/data/';
const staticFolder = 'espurna/static/';

// -----------------------------------------------------------------------------
// Methods
// -----------------------------------------------------------------------------

var toMinifiedHtml = function(options) {
    return through.obj(function (source, encoding, callback) {
        if (source.isNull()) {
            callback(null, source);
            return;
        }

        var contents = source.contents.toString();
        source.contents = Buffer.from(htmlmin.minify(contents, options));
        callback(null, source);
    });
}

var toHeader = function(name, debug) {

    return through.obj(function (source, encoding, callback) {

        var parts = source.path.split(path.sep);
        var filename = parts[parts.length - 1];
        var safename = name || filename.split('.').join('_');

        // Generate output
        var output = '';
        output += '#define ' + safename + '_len ' + source.contents.length + '\n';
        output += 'const uint8_t ' + safename + '[] PROGMEM = {';
        for (var i=0; i<source.contents.length; i++) {
            if (i > 0) { output += ','; }
            if (0 === (i % 20)) { output += '\n'; }
            output += '0x' + ('00' + source.contents[i].toString(16)).slice(-2);
        }
        output += '\n};';

        // clone the contents
        var destination = source.clone();
        destination.path = source.path + '.h';
        destination.contents = Buffer.from(output);

        if (debug) {
            console.info('Image ' + filename + ' \tsize: ' + source.contents.length + ' bytes');
        }

        callback(null, destination);

    });

};

var htmllintReporter = function(filepath, issues) {
    if (issues.length > 0) {
        issues.forEach(function (issue) {
            console.info(
                '[gulp-htmllint] ' +
                filepath + ' [' +
                issue.line + ',' +
                issue.column + ']: ' +
                '(' + issue.code + ') ' +
                issue.msg
            );
        });
        process.exitCode = 1;
    }
};

// TODO: this is a roughly equivalent port of the gulp-remove-code,
// which also uses regexp rules to filter in-between specially-formatted comment blocks

var jsRegexp = function(module) {
    return '//\\s*removeIf\\(!' + module + '\\)'
            + '\\s*(\n|\r|.)*?'
            + '//\\s*endRemoveIf\\(!' + module + '\\)';
}

var cssRegexp = function(module) {
    return '/\\*\\s*removeIf\\(!' + module + '\\)\\s*\\*/'
            + '\\s*(\n|\r|.)*?'
            + '/\\*\\s*endRemoveIf\\(!' + module + '\\)\\s*\\*/';
}

var htmlRegexp = function(module) {
    return '<!--\\s*removeIf\\(!' + module + '\\)\\s*-->'
            + '\\s*(\n|\r|.)*?'
            + '<!--\\s*endRemoveIf\\(!' + module + '\\)\\s*-->';
}

var generateRegexps = function(modules, func) {
    var regexps = new Set();
    for (const [module, enabled] of Object.entries(modules)) {
        if (enabled) {
            continue;
        }

        const expression = func(module);
        const re = new RegExp(expression, 'gm');

        regexps.add(re);
    }

    return regexps;
}

// TODO: use html parser here?
// TODO: separate js files to include js, html & css and avoid 2 step regexp?

var htmlRemover = function(modules) {
    const regexps = generateRegexps(modules, htmlRegexp);

    return through.obj(function (source, _, callback) {
        if (source.isNull()) {
            callback(null, source);
            return;
        }

        var contents = source.contents.toString();
        for (var regexp of regexps) {
            contents = contents.replace(regexp, '');
        }
        source.contents = Buffer.from(contents);
        callback(null, source);
    });
}

var inlineHandler = function(modules) {
    return function(source) {
        if (((source.sourcepath === 'custom.css') || (source.sourcepath === 'custom.js'))) {
            const filter = (source.type === 'css') ? cssRegexp : jsRegexp;
            const regexps = generateRegexps(modules, filter);

            var content = source.fileContent;
            for (var regexp of regexps) {
                content = content.replace(regexp, '');
            }

            source.fileContent = content;
            return;
        }

        if (source.sourcepath === "favicon.ico") {
            source.format = "x-icon";
            return;
        }

        if (source.content) {
            return;
        }

        // Just ignore the vendored libs, repackaging makes things worse for the size
        const path = source.sourcepath;
        if (path.endsWith('.min.js')) {
            source.compress = false;
        } else if (path.endsWith('.min.css')) {
            source.compress = false;
        }
    };
}

var buildWebUI = function(module) {

    // Declare some modules as optional to remove with
    // removeIf(!name) ...code... endRemoveIf(!name) sections
    // (via gulp-remove-code)
    var modules = {
        'light': false,
        'sensor': false,
        'rfbridge': false,
        'rfm69': false,
        'garland': false,
        'thermostat': false,
        'lightfox': false,
        'curtain': false
    };

    // Note: only build these when specified as module arg
    var excludeAll = [
        'rfm69',
        'lightfox'
    ];

    // 'all' to include all *but* excludeAll
    // '<module>' to include a single module
    // 'small' is the default state (all disabled)
    if ('all' === module) {
        Object.keys(modules).
            filter(function(key) {
                return excludeAll.indexOf(key) < 0;
            }).
            forEach(function(key) {
                modules[key] = true;
            });
    } else if ('small' !== module) {
        modules[module] = true;
    }

    return gulp.src(htmlFolder + '*.html').
        pipe(htmllint({
            'failOnError': true,
            'rules': {
                'id-class-style': false,
                'label-req-for': false,
                'line-end-style': false,
                'attr-req-value': false
            }
        }, htmllintReporter)).
        pipe(htmlRemover(modules)).
        pipe(inline({handlers: [inlineHandler(modules)]})).
        pipe(toMinifiedHtml({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: false,
            minifyJS: false
        })).
        pipe(replace('pure-', 'p-')).
        pipe(gzip({ gzipOptions: { level: 9 } })).
        pipe(rename('index.' + module + '.html.gz')).
        pipe(gulp.dest(dataFolder)).
        pipe(toHeader('webui_image', true)).
        pipe(gulp.dest(staticFolder));

};

// -----------------------------------------------------------------------------
// Tasks
// -----------------------------------------------------------------------------

gulp.task('certs', function() {
    gulp.src(dataFolder + 'server.*').
        pipe(toHeader('', false)).
        pipe(gulp.dest(staticFolder));
});

gulp.task('csslint', function() {
    gulp.src(htmlFolder + '*.css').
        pipe(csslint({ids: false})).
        pipe(csslint.formatter());
});

gulp.task('webui_small', function() {
    return buildWebUI('small');
});

gulp.task('webui_sensor', function() {
    return buildWebUI('sensor');
});

gulp.task('webui_light', function() {
    return buildWebUI('light');
});

gulp.task('webui_rfbridge', function() {
    return buildWebUI('rfbridge');
});

gulp.task('webui_rfm69', function() {
    return buildWebUI('rfm69');
});

gulp.task('webui_lightfox', function() {
    return buildWebUI('lightfox');
});

gulp.task('webui_garland', function() {
    return buildWebUI('garland');
});

gulp.task('webui_thermostat', function() {
    return buildWebUI('thermostat');
});

gulp.task('webui_curtain', function() {
    return buildWebUI('curtain');
});

gulp.task('webui_all', function() {
    return buildWebUI('all');
});

gulp.task('webui',
    gulp.parallel(
        'webui_small',
        'webui_sensor',
        'webui_light',
        'webui_rfbridge',
        'webui_rfm69',
        'webui_lightfox',
        'webui_garland',
        'webui_thermostat',
        'webui_curtain',
        'webui_all'
    )
);

gulp.task('default', gulp.series('webui'));
