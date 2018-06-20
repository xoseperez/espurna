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
// File system builder
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
const log = require('fancy-log');
const csslint = require('gulp-csslint');
const crass = require('gulp-crass');
const replace = require('gulp-replace');
const remover = require('gulp-remove-code');

const dataFolder = 'espurna/data/';
const staticFolder = 'espurna/static/';

var toHeader = function(filename) {

    var source = dataFolder + filename;
    var destination = staticFolder + filename + '.h';
    var safename = filename.split('.').join('_');

    var wstream = fs.createWriteStream(destination);
    wstream.on('error', function (err) {
        log.error(err);
    });

    var data = fs.readFileSync(source);

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

    var fstat = fs.statSync(source);
    console.log("index.html.gz size: " + fstat.size + " bytes.");

};

var htmllintReporter = function(filepath, issues) {
	if (issues.length > 0) {
		issues.forEach(function (issue) {
			log.info(
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

gulp.task('build_certs', function() {
    toHeader('server.cer');
    toHeader('server.key');
});

gulp.task('csslint', function() {
    gulp.src('html/*.css').
        pipe(csslint({ids: false})).
        pipe(csslint.formatter());
});

gulp.task('buildfs_embeded', ['buildfs_inline'], function() {
    toHeader('index.html.gz');
});

gulp.task('buildfs_inline', function() {

    var remover_config = {
        sensor: false,
        light: false,
        rfbridge: false
    };
    var modules = 'WEBUI_MODULES' in process.env ? process.env.WEBUI_MODULES : false;
    if (false === modules || "all" === modules) {
        for (var i in remover_config) {
            remover_config[i] = true;
        }
    } else {
        var list = modules.split(' ');
        for (var i in list) {
            if (list[i] != "") {
                remover_config[list[i]] = true;
            }
        }
    }
    log.info("Modules: " + JSON.stringify(remover_config));

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
        pipe(remover(remover_config)).
        pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        })).
        pipe(replace('pure-', 'p-')).
        pipe(gzip()).
        pipe(gulp.dest(dataFolder));
});


gulp.task('test', function() {
    return gulp.src('html/custom.js').
        pipe(remover({
            sensor: false,
            light: false,
            rfbridge: false
        })).
        pipe(gulp.dest('/home/xose/tmp/'));
});


gulp.task('default', ['buildfs_embeded']);
