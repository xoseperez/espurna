const PurgecssPlugin = require("purgecss-webpack-plugin");
const CompressionPlugin = require("compression-webpack-plugin");
const glob = require("glob-all");
const path = require("path");

module.exports = {
    //runtimeCompiler: true,
    configureWebpack: {
        plugins: [
            new PurgecssPlugin({
                paths: glob.sync([
                    path.join(__dirname, "./src/index.html"),
                    path.join(__dirname, "./**/*.vue"),
                    path.join(__dirname, "./src/**/*.js")
                ])
            }),
            new CompressionPlugin({
                test: /index\.html?$/i,
            }),
        ],
        devtool: false,
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


        config.module.rule("vue").use("webpack-conditional-loader").loader("webpack-conditional-loader");

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
    },
};