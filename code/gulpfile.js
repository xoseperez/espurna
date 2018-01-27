/*

ESP8266 file system builder

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

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

// -----------------------------------------------------------------------------
// File system builder
// -----------------------------------------------------------------------------

const fs = require('fs');
const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const cleancss = require('gulp-clean-css');
const uglify = require('gulp-uglify');
const gzip = require('gulp-gzip');
const inline = require('gulp-inline');
const inlineImages = require('gulp-css-base64');
const favicon = require('gulp-base64-favicon');
<<<<<<< Updated upstream
=======
const htmllint = require('gulp-htmllint');
const gutil = require('gulp-util');
const csslint = require('gulp-csslint');
const i18n = require('gulp-international');

// -----------------------------------------------------------------------------
>>>>>>> Stashed changes

const dataFolder = 'espurna/data/';
const staticFolder = 'espurna/static/';

// -----------------------------------------------------------------------------

const map = require('map-stream');

var buildHeaderFile = function() {

    String.prototype.replaceAll = function(search, replacement) {
        var target = this;
        return target.split(search).join(replacement);
    };

    return map(function(file, cb) {

        var parts = file.path.split("/");
        var filename = parts[parts.length - 1];
        var destination = staticFolder + filename + ".h";
        var safename = filename.replaceAll('.', '_');

        var wstream = fs.createWriteStream(destination);
        wstream.on('error', function (err) {
            console.log(err);
        });

<<<<<<< Updated upstream
    wstream.write('#define ' + safename + '_len ' + data.length + '\n');
    wstream.write('const uint8_t ' + safename + '[] PROGMEM = {')

    for (i=0; i<data.length; i++) {
        if (i % 20 == 0) wstream.write("\n");
        wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
        if (i<data.length-1) wstream.write(',');
    }

    wstream.write('\n};')
    wstream.end();
=======
        var data = fs.readFileSync(file.path);

        wstream.write('#define ' + safename + '_len ' + data.length + '\n');
        wstream.write('const uint8_t ' + safename + '[] PROGMEM = {');

        for (var i=0; i<data.length; i++) {
            if (i % 20 == 0) wstream.write('\n');
            wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
            if (i<data.length-1) {
              wstream.write(',');
            }
        }

        wstream.write('\n};');
        wstream.end();

        cb(0, file);

   });
>>>>>>> Stashed changes

}

gulp.task('build_certs', function() {
    toHeader('server.cer');
    toHeader('server.key');
});

gulp.task('buildfs_embeded', ['buildfs_inline'], function() {
    gulp.src(dataFolder + 'index.*')
        .pipe(buildHeaderFile());
});

gulp.task('buildfs_inline', function() {
    return gulp.src('html/*.html')
        .pipe(favicon())
        .pipe(inline({
            base: 'html/',
            js: uglify,
            css: [cleancss, inlineImages],
            disabledTypes: ['svg', 'img']
        }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        }))
        .pipe(i18n({
            warn: true,
            whitelist: ['ca_ES'],
            filename: '${name}.${lang}.${ext}',
            locales: './html/locales/'
        }))
        .pipe(gzip())
        .pipe(gulp.dest(dataFolder));
})

gulp.task('default', ['buildfs_embeded']);
