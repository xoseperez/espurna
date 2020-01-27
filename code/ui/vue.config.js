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

const maybeStringToBoolean = (string) => {
    switch (string.toString().toLowerCase().trim()) {
        case "true":
        case "yes":
        case "1":
            return true;
        case "false":
        case "no":
        case "0":
        case null:
            return false;
        default:
            return string;
    }
};

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
        compress: {
            drop_console: true,
            unsafe_methods: true,
            unsafe_proto: true,
        },
        output: {
            comments: false,
            beautify: false
        },
    },
    extractComments: false
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
                        mangleWasmImports: true,
                        removeAvailableModules: false,
                        mergeDuplicateChunks: true,
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

        let prepro_opts = {
            params: {
                ENV: process.env.NODE_ENV,
            },
            verbose: false,
        };

        Object.keys(process.env).forEach((k) => {
            const matches = k.match(/^VUE_APP_(.*)|MODULE_(.*)$/i);
            if (matches) {
                prepro_opts.params[(matches[1] || matches[2]).toUpperCase()] = maybeStringToBoolean(process.env[k]);
            }
        });

        config.module.rule("vue").use("webpack-preprocessor-loader").loader("webpack-preprocessor-loader").options(prepro_opts);
        config.module.rule("js").use("webpack-preprocessor-loader").loader("webpack-preprocessor-loader").options(prepro_opts);

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
