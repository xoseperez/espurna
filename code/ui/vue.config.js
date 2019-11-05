const PurgecssPlugin = require("purgecss-webpack-plugin");
const CompressionPlugin = require("compression-webpack-plugin");
const TerserPlugin = require('terser-webpack-plugin');
const glob = require("glob-all");
const path = require("path");
const {GenerateSW} = require('workbox-webpack-plugin');
const FaviconsWebpackPlugin = require('favicons-webpack-plugin');

const purgecss = new PurgecssPlugin({
    paths: glob.sync([
        path.join(__dirname, "./src/index.html"),
        path.join(__dirname, "./**/*.vue"),
        path.join(__dirname, "./src/**/*.js")
    ])
});

const faviconsPlugin = new FaviconsWebpackPlugin({
    logo: "./public/icons/icon.svg",
    prefix: 'icons/',
    mode: 'webapp',
    devMode: 'light',
    favicons: {
        start_url: '',
        icons: {
            coast: false,
            yandex: false
        }
    }
});

const pwa = new GenerateSW(
    {
        exclude: [
            /\.map$/,
            /favicon\.ico$/,
            /^manifest.*\.js?$/
        ],
        cacheId: 'ui'
    }
);

const terser = new TerserPlugin({
    terserOptions: {
        ecma: 6,
        compress: true,
        output: {
            comments: false,
            beautify: false
        }
    }
});

module.exports = {
    publicPath: '',
    pwa: {
        themeColor: '#479fd6',
        workboxPluginMode: 'InjectManifest',
        workboxOptions: {
            // swSrc is required in InjectManifest mode.
            swSrc: 'public/service-worker.js',
            // ...other Workbox options...
        },
        iconPaths: {
            favicon32: 'icons/favicon32.png',
            favicon16: 'icons/favicon16.png',
            appleTouchIcon: 'icons/apple-touch-icon-152x152.png',
            maskIcon: 'icons/icon.svg',
            msTileImage: 'icons/mstile-144x144.png'
        }
    },
    //runtimeCompiler: true,
    configureWebpack() {
        switch (process.env.NODE_ENV) {
            case 'production':
                return {
                    plugins: [
                        purgecss,
                        new CompressionPlugin({
                            test: /index\.html?$/i,
                        }),
                    ],
                    devtool: false,
                    optimization: {
                        minimizer: [
                            terser
                        ]
                    }
                };
            case 'pwa':
                return {
                    mode: 'production',
                    node: {
                        setImmediate: true
                    },
                    devtool: false,
                    optimization: {
                        minimizer: []
                    },
                    plugins: [
                        faviconsPlugin,
                        pwa
                    ],
                };
            default:
            case 'development':
                return {
                    mode: 'development',
                    node: {
                        setImmediate: true
                    },
                    plugins: [
                        faviconsPlugin,
                        purgecss,
                        pwa
                    ],
                }
        }
    },
    chainWebpack(config) {
        config.module
            .rule("vue")
            .use("vue-loader")
            .loader("vue-loader")
            .tap(options => {
                options.compilerOptions.whitespace = "condense";
                return options
            });


        const svgRule = config.module.rule('svg');

        svgRule.uses.clear();

        svgRule
            .use('vue-svg-loader')
            .loader('vue-svg-loader');

        config.module.rule('ico').test(/\.ico$/).use("url-loader").loader("url-loader");

        config.module.rule("vue").use("webpack-conditional-loader").loader("webpack-conditional-loader");
        config.module.rule("js").use("webpack-conditional-loader").loader("webpack-conditional-loader");

        if (process.env.NODE_ENV === 'production') {

            config.plugins.delete('pwa');
            config.plugins.delete('workbox');

            config.plugin("preload")
                .tap(args => {
                    args[0].fileBlacklist.push(/\.css/, /\.js/);
                    return args
                });

            config.plugin("inline-source")
                .use(require("html-webpack-inline-source-plugin"));

            config.plugin("html")
                .tap(args => {
                    args[0].inlineSource = ".(js|css)$"; // embed all javascript and css inline
                    args.minify = true;
                    return args;
                })
        } else {
        }
    },
};