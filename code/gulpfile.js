var gulp = require('gulp');
var plumber = require('gulp-plumber');
var htmlmin = require('gulp-htmlmin');
var cleancss = require('gulp-clean-css');
var uglify = require('gulp-uglify');
var gzip = require('gulp-gzip');
var del = require('del');
var useref = require('gulp-useref');
var gulpif = require('gulp-if');

/* Clean destination folder */
gulp.task('clean', function() {
    return del(['data/*']);
});

/* Copy static files */
gulp.task('files', function() {
    return gulp.src([
            'html/**/*.{jpg,jpeg,png,ico,gif}',
            'html/fsversion'
        ])
        .pipe(gulp.dest('data/'));
});

/* Process HTML, CSS, JS */
gulp.task('html', function() {
    return gulp.src('html/*.html')
        .pipe(useref())
        .pipe(plumber())
        .pipe(gulpif('*.css', cleancss()))
        .pipe(gulpif('*.js', uglify()))
        .pipe(gulpif('*.html', htmlmin({
            collapseWhitespace: true,
            removeComments: true,
            minifyCSS: true,
            minifyJS: true
        })))
        .pipe(gzip())
        .pipe(gulp.dest('data'));
});

/* Default Task */
gulp.task('default', ['clean', 'files', 'html']);
