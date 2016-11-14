/*

ESP8266 file system builder

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

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

const gulp = require('gulp');
const htmlmin = require('gulp-htmlmin');
const cleancss = require('gulp-clean-css');
const uglify = require('gulp-uglify');
const gzip = require('gulp-gzip');
const del = require('del');
const inline = require('gulp-inline');

/* Clean destination folder */
gulp.task('clean', function() {
    del(['data/*']);
    return true;
});

/* Copy static files */
gulp.task('files', function() {
    return gulp.src([
            'html/**/*.{jpg,jpeg,png,ico,gif}',
            'html/fsversion'
        ])
        .pipe(gulp.dest('data/'));
});


/* Process HTML, CSS, JS  --- INLINE --- */
gulp.task('inline', function() {
    return gulp.src('html/*.html')
        .pipe(inline({
            base: 'html/',
            js: uglify,
            css: cleancss,
            disabledTypes: ['svg', 'img']
        }))
        .pipe(htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        }))
        .pipe(gzip())
        .pipe(gulp.dest('data'));
})

/* Build file system */
gulp.task('buildfs', ['clean', 'files', 'inline']);
gulp.task('default', ['buildfs']);
