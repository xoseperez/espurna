/*

ESP8266 file system builder

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

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

/*eslint quotes: ["error", "single"]*/
/*eslint-env es6*/

// -----------------------------------------------------------------------------
// Dependencies
// -----------------------------------------------------------------------------

const fs = require('fs');
const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const uglify = require('gulp-uglify');
const gzip = require('gulp-gzip');
const inline = require('gulp-inline');
const inlineImages = require('gulp-css-base64');
const favicon = require('gulp-base64-favicon');
const htmllint = require('gulp-htmllint');
const csslint = require('gulp-csslint');
const crass = require('gulp-crass');
const replace = require('gulp-replace');
const remover = require('gulp-remove-code');
const map = require('map-stream');
const rename = require('gulp-rename');
const runSequence = require('run-sequence');

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

const dataFolder = 'espurna/data/';
const staticFolder = 'espurna/static/';

// -----------------------------------------------------------------------------
// Methods
// -----------------------------------------------------------------------------

var buildHeaderFile = function() {

    String.prototype.replaceAll = function(search, replacement) {
        var target = this;
        return target.split(search).join(replacement);
    };

    return map(function(file, cb) {

        var parts = file.path.split("/");
        var filename = parts[parts.length - 1];
        var destination = staticFolder + filename + ".h";
        var safename = "webui_image";

        var wstream = fs.createWriteStream(destination);
        wstream.on('error', function (err) {
            console.error(err);
        });

        var data = fs.readFileSync(file.path);

        wstream.write('#define ' + safename + '_len ' + data.length + '\n');
        wstream.write('const uint8_t ' + safename + '[] PROGMEM = {');

        for (var i=0; i<data.length; i++) {
            if (0 === (i % 20)) {
                wstream.write('\n');
            }
            wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
            if (i < (data.length - 1)) {
                wstream.write(',');
            }
        }

        wstream.write('\n};');
        wstream.end();

        var fstat = fs.statSync(file.path);
        console.log("Created '" + filename + "' size: " + fstat.size + " bytes");

        cb(0, destination);

    });

}

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

var buildWebUI = function(module) {

    var modules = {"light": false, "sensor": false, "rfbridge": false, "rfm69": false};
    if ("all" == module) {
        modules["light"] = true;
        modules["sensor"] = true;
        modules["rfbridge"] = false;   // we will never be adding this except when building RFBRIDGE
        modules["rfm69"] = false;   // we will never be adding this except when building RFM69GW
    } else if ("small" != module) {
        modules[module] = true;
    }

    return gulp.src('html/*.html').
        pipe(htmllint({
            'failOnError': true,
            'rules': {
                'id-class-style': false,
                'label-req-for': false,
            }
        }, htmllintReporter)).
        pipe(favicon()).
        pipe(inline({
            base: 'html/',
            js: [],
            css: [crass, inlineImages],
            disabledTypes: ['svg', 'img']
        })).
        pipe(remover(modules)).
        pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        })).
        pipe(replace('pure-', 'p-')).
        pipe(gzip()).
        pipe(rename("index." + module + ".html.gz")).
        pipe(gulp.dest(dataFolder));

};

// -----------------------------------------------------------------------------
// Tasks
// -----------------------------------------------------------------------------

gulp.task('build_certs', function() {
    gulp.src(dataFolder + 'server.*').
        pipe(buildHeaderFile());
});

gulp.task('csslint', function() {
    gulp.src('html/*.css').
        pipe(csslint({ids: false})).
        pipe(csslint.formatter());
});

gulp.task('build_webui_small', function() {
    return buildWebUI("small");
})

gulp.task('build_webui_sensor', function() {
    return buildWebUI("sensor");
})

gulp.task('build_webui_light', function() {
    return buildWebUI("light");
})

gulp.task('build_webui_rfbridge', function() {
    return buildWebUI("rfbridge");
})

gulp.task('build_webui_rfm69', function() {
    return buildWebUI("rfm69");
})

gulp.task('build_webui_all', function() {
    return buildWebUI("all");
})

gulp.task('buildfs_inline', function(cb) {
    runSequence([
        'build_webui_small',
        'build_webui_sensor',
        'build_webui_light',
        'build_webui_rfbridge',
        'build_webui_rfm69',
        'build_webui_all'
    ], cb);
});

gulp.task('buildfs_embeded', ['buildfs_inline'], function() {
    gulp.src(dataFolder + 'index.*').
        pipe(buildHeaderFile());
});

gulp.task('default', ['buildfs_embeded']);
